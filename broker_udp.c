/**
 * broker_udp.c — Broker UDP del sistema de noticias deportivas (pub/sub).
 *
 * Rol: recibe datagramas de publicadores (PUB) y suscriptores (SUB), mantiene
 * en memoria qué direcciones UDP están interesadas en cada tema (partido) y
 * reenvía cada noticia solo a quienes se suscribieron a ese tema.
 *
 * Uso:
 *   ./broker_udp [puerto]
 *   Si no se indica puerto, se usa PUBSUB_UDP_DEFAULT_PORT (pubsub_udp.h).
 *
 * Notas sobre UDP:
 *   - No hay sesión TCP: cada envío es independiente; el broker identifica a
 *     los suscriptores por la dirección de origen del datagrama SUB.
 *   - Puede haber pérdida o desorden; este programa no retransmite ni ordena.
 *
 * Compilación (Linux): gcc -Wall -Wextra -std=c11 -o broker_udp broker_udp.c
 *
 * Documentación punto a punto de cabeceras estándar/POSIX y sockets: README.md
 */

#include "pubsub_udp.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct {
    char topic[PUBSUB_UDP_MAX_TOPIC_LEN + 1];
    struct sockaddr_in subs[PUBSUB_UDP_MAX_SUBS_PER_TOPIC];
    int n_subs;
} TopicBucket;

static TopicBucket g_topics[PUBSUB_UDP_MAX_TOPICS];
static int g_n_topics;

static void strip_trailing_crlf(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[n - 1] = '\0';
        n--;
    }
}

static int sockaddr_in_equal(const struct sockaddr_in *a,
                             const struct sockaddr_in *b) {
    return a->sin_family == b->sin_family &&
           a->sin_addr.s_addr == b->sin_addr.s_addr &&
           a->sin_port == b->sin_port;
}

static int find_topic_index(const char *topic) {
    for (int i = 0; i < g_n_topics; i++) {
        if (strcmp(g_topics[i].topic, topic) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * Registra un nuevo tema vacío. Retorna índice o -1 si la tabla está llena.
 */
static int add_topic_bucket(const char *topic) {
    if (g_n_topics >= PUBSUB_UDP_MAX_TOPICS) {
        return -1;
    }
    TopicBucket *b = &g_topics[g_n_topics];
    memset(b, 0, sizeof(*b));
    (void)snprintf(b->topic, sizeof b->topic, "%s", topic);
    b->n_subs = 0;
    g_n_topics++;
    return g_n_topics - 1;
}

static int ensure_topic_index(const char *topic) {
    int idx = find_topic_index(topic);
    if (idx >= 0) {
        return idx;
    }
    return add_topic_bucket(topic);
}

/**
 * Añade la dirección del suscriptor al tema si aún no estaba (evita duplicados).
 */
static void add_subscriber_to_topic(int topic_idx, const struct sockaddr_in *addr) {
    TopicBucket *b = &g_topics[topic_idx];
    for (int i = 0; i < b->n_subs; i++) {
        if (sockaddr_in_equal(&b->subs[i], addr)) {
            return;
        }
    }
    if (b->n_subs >= PUBSUB_UDP_MAX_SUBS_PER_TOPIC) {
        fprintf(stderr, "[broker] Aviso: tema '%s' alcanzó el máximo de suscriptores.\n",
                b->topic);
        return;
    }
    b->subs[b->n_subs++] = *addr;
}

static int send_datagram(int sock, const char *text, const struct sockaddr_in *to) {
    size_t len = strlen(text);
    if (len > PUBSUB_UDP_MAX_MSG) {
        fprintf(stderr, "[broker] Mensaje demasiado largo para un datagrama.\n");
        return -1;
    }
    /* sendto: un datagrama UDP hacia *to (IP:puerto ya en orden de red). */
    ssize_t n = sendto(sock, text, len, 0, (const struct sockaddr *)to,
                       (socklen_t)sizeof(*to));
    if (n < 0) {
        perror("[broker] sendto");
        return -1;
    }
    return 0;
}

/**
 * Procesa: SUB <tema>
 */
static void handle_subscribe(int sock, char *payload, const struct sockaddr_in *client) {
    strip_trailing_crlf(payload);
    if (payload[0] == '\0') {
        fprintf(stderr, "[broker] SUB sin tema, ignorado.\n");
        return;
    }
    /* Un solo token como tema (sin espacios), según especificación del protocolo. */
    if (strchr(payload, ' ') != NULL) {
        fprintf(stderr, "[broker] El tema no debe contener espacios: '%s'\n", payload);
        return;
    }
    if (strlen(payload) > PUBSUB_UDP_MAX_TOPIC_LEN) {
        fprintf(stderr, "[broker] Tema demasiado largo (máx %d caracteres).\n",
                PUBSUB_UDP_MAX_TOPIC_LEN);
        return;
    }

    int ti = ensure_topic_index(payload);
    if (ti < 0) {
        fprintf(stderr, "[broker] No se pudo registrar el tema (tabla llena).\n");
        return;
    }
    add_subscriber_to_topic(ti, client);

    char ack[PUBSUB_UDP_MAX_MSG];
    int w = snprintf(ack, sizeof ack, "%s%s\n", PUBSUB_UDP_PREFIX_ACK, payload);
    if (w < 0 || (size_t)w >= sizeof ack) {
        fprintf(stderr, "[broker] ACK demasiado largo.\n");
        return;
    }
    (void)send_datagram(sock, ack, client);
    /* inet_ntoa / ntohs: solo presentación humana de la dirección guardada. */
    printf("[broker] Suscripción registrada: tema='%s' desde %s:%u (total suscriptores en tema: %d)\n",
           payload, inet_ntoa(client->sin_addr), ntohs(client->sin_port),
           g_topics[ti].n_subs);
}

/**
 * Procesa: PUB <tema>|<cuerpo>
 */
static void handle_publish(int sock, char *payload) {
    strip_trailing_crlf(payload);
    char *sep = strchr(payload, '|');
    if (sep == NULL) {
        fprintf(stderr, "[broker] PUB sin '|' (formato PUB tema|cuerpo).\n");
        return;
    }
    *sep = '\0';
    const char *topic = payload;
    const char *body = sep + 1;
    if (topic[0] == '\0') {
        fprintf(stderr, "[broker] PUB con tema vacío.\n");
        return;
    }

    int ti = find_topic_index(topic);
    if (ti < 0) {
        printf("[broker] PUB para tema sin suscriptores registrados: '%s' (mensaje descartado para reenvío)\n",
               topic);
        return;
    }

    char out[PUBSUB_UDP_MAX_MSG];
    int w = snprintf(out, sizeof out, "%s%s|%s\n", PUBSUB_UDP_PREFIX_NEWS, topic, body);
    if (w < 0 || (size_t)w >= sizeof out) {
        fprintf(stderr, "[broker] NEWS demasiado largo para un datagrama.\n");
        return;
    }

    TopicBucket *b = &g_topics[ti];
    int sent = 0;
    for (int i = 0; i < b->n_subs; i++) {
        if (send_datagram(sock, out, &b->subs[i]) == 0) {
            sent++;
        }
    }
    printf("[broker] PUB tema='%s' reenviado a %d suscriptor(es).\n", topic, sent);
}

static void usage(const char *argv0) {
    fprintf(stderr, "Uso: %s [puerto]\n", argv0);
    fprintf(stderr, "  Escucha datagramas UDP y distribuye mensajes por tema.\n");
}

int main(int argc, char **argv) {
    unsigned short port = PUBSUB_UDP_DEFAULT_PORT;
    if (argc > 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 2) {
        char *end = NULL;
        unsigned long p = strtoul(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || p == 0UL || p > 65535UL) {
            fprintf(stderr, "Puerto inválido: %s\n", argv[1]);
            return EXIT_FAILURE;
        }
        port = (unsigned short)p;
    }

    /* socket: crear extremo UDP IPv4; el SO asigna un puerto local efímero si no bind. */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    /* setsockopt SO_REUSEADDR: facilita reinicios rápidos del broker en el mismo puerto. */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(sock);
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    /* INADDR_ANY + htonl: escuchar en todas las interfaces IPv4 de la máquina. */
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    /* bind: el broker queda registrado en <todas las IPs>:puerto para recvfrom entrantes. */
    if (bind(sock, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("bind");
        close(sock); /* puerto ocupado u otro error: liberar descriptor */
        return EXIT_FAILURE;
    }

    printf("[broker] Escuchando UDP en el puerto %u. Ctrl+C para salir.\n", port);

    for (;;) {
        char buf[PUBSUB_UDP_MAX_MSG + 1];
        struct sockaddr_in client;
        socklen_t clen = sizeof client;
        /* recvfrom: obtiene datagrama + dirección de origen (clave para registrar SUB). */
        ssize_t n = recvfrom(sock, buf, PUBSUB_UDP_MAX_MSG, 0,
                             (struct sockaddr *)&client, &clen);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }
        if (n == 0) {
            continue;
        }
        buf[n] = '\0';

        if (strncmp(buf, PUBSUB_UDP_PREFIX_SUB, strlen(PUBSUB_UDP_PREFIX_SUB)) == 0) {
            handle_subscribe(sock, buf + strlen(PUBSUB_UDP_PREFIX_SUB), &client);
        } else if (strncmp(buf, PUBSUB_UDP_PREFIX_PUB, strlen(PUBSUB_UDP_PREFIX_PUB)) ==
                   0) {
            handle_publish(sock, buf + strlen(PUBSUB_UDP_PREFIX_PUB));
        } else {
            fprintf(stderr, "[broker] Datagrama no reconocido (primeros bytes): %.40s...\n",
                    buf);
        }
    }

    /* No alcanzable con bucle infinito; close liberaría el fd si hubiera salida. */
    close(sock);
    return EXIT_SUCCESS;
}
