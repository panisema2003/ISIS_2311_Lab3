/**
 * subscriber_udp.c — Suscriptor UDP (aficionado que sigue uno o varios partidos).
 *
 * Registra en el broker la suscripción a cada tema con SUB <tema> y luego
 * imprime en consola las noticias que el broker reenvía (NEWS ...).
 *
 * Uso:
 *   ./subscriber_udp <ip_broker> <puerto_broker> <tema1> [tema2 ...]
 *
 * Ejemplo:
 *   ./subscriber_udp 127.0.0.1 9000 EquipoA_vs_EquipoB EquipoC_vs_EquipoD
 *
 * Nota: no se usa connect(UDP) al broker (evita rechazos en el kernel por ICMP/origen).
 * Tras recvfrom se aceptan solo datagramas cuyo origen usa el **puerto del broker**
 * (p. ej. 9000): el broker envía OK SUB / NEWS desde el socket enlazado a ese puerto.
 */

#include "pubsub_udp.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * Devuelve verdadero si el datagrama parece venir del broker: mismo puerto de origen
 * que el puerto de servicio (p. ej. 9000). No exige coincidir la IP con argv.
 */
static int udp_from_broker(const struct sockaddr_in *from, const struct sockaddr_in *broker) {
    return from->sin_family == AF_INET && broker->sin_family == AF_INET &&
           from->sin_port == broker->sin_port;
}

/* Arma SUB <tema>\n y lo envía con sendto al broker. */
static int send_sub(int sock, const struct sockaddr_in *broker, const char *topic) {
    char buf[PUBSUB_UDP_MAX_MSG + 1];
    int w = snprintf(buf, sizeof buf, "%s%s\n", PUBSUB_UDP_PREFIX_SUB, topic);
    if (w < 0 || (size_t)w >= sizeof buf) {
        fprintf(stderr, "[subscriber] SUB demasiado largo.\n");
        return -1;
    }
    ssize_t n = sendto(sock, buf, (size_t)w, 0, (const struct sockaddr *)broker,
                       (socklen_t)sizeof(*broker));
    if (n < 0) {
        perror("[subscriber] sendto SUB");
        return -1;
    }
    printf("[subscriber] Registrado SUB '%s' (%zd bytes enviados al broker)\n", topic, n);
    fflush(stdout);
    return 0;
}

/* Si buf empieza por OK SUB , muestra confirmación de suscripción en consola. */
static void print_sub_ok_line(const char *buf, const char *fromstr, uint16_t port) {
    const size_t plen = strlen(PUBSUB_UDP_PREFIX_SUB_OK);
    if (strncmp(buf, PUBSUB_UDP_PREFIX_SUB_OK, plen) != 0) {
        return;
    }
    const char *rest = buf + plen;
    size_t k = strcspn(rest, "\r\n");
    printf("\n[subscriber] Suscripcion OK desde %s:%u -> partido \"%.*s\"\n", fromstr,
           (unsigned)port, (int)k, rest);
    fflush(stdout);
}

/* Si buf empieza por NEWS , separa tema y cuerpo por '|' y los imprime formateados. */
static void print_news_line(const char *buf, const char *fromstr, uint16_t port) {
    const size_t plen = strlen(PUBSUB_UDP_PREFIX_NEWS);
    if (strncmp(buf, PUBSUB_UDP_PREFIX_NEWS, plen) != 0) {
        return;
    }
    const char *p = buf + plen;
    const char *pipe = strchr(p, '|');
    if (pipe == NULL) {
        printf("\n[subscriber] [%s:%u] %s", fromstr, (unsigned)port, buf);
        fflush(stdout);
        return;
    }
    size_t tlen = (size_t)(pipe - p);
    char topic_disp[64];
    if (tlen >= sizeof topic_disp) {
        tlen = sizeof topic_disp - 1U;
    }
    memcpy(topic_disp, p, tlen);
    topic_disp[tlen] = '\0';
    const char *body = pipe + 1;
    printf("\n---------- NOTICIA EN VIVO ----------\n");
    printf("  Partido: %s\n", topic_disp);
    printf("  Origen:  %s:%u\n", fromstr, (unsigned)port);
    printf("  Texto:   %s", body);
    if (body[0] != '\0' && strchr(body, '\n') == NULL) {
        printf("\n");
    }
    printf("---------------------------------------\n");
    fflush(stdout);
}

/* Ayuda de uso por stderr. */
static void usage(const char *argv0) {
    fprintf(stderr,
            "Uso: %s <ip_broker> <puerto_broker> <tema1> [tema2 ...]\n"
            "  Cada <tema> es un partido (sin espacios), ej. EquipoA_vs_EquipoB\n",
            argv0);
}

/*
 * Valida argumentos, crea socket UDP, envía un SUB por cada tema en argv,
 * y entra en bucle recvfrom filtrando por puerto del broker y mostrando OK SUB / NEWS.
 */
int main(int argc, char **argv) {
    (void)setvbuf(stdout, NULL, _IONBF, 0);
    (void)setvbuf(stderr, NULL, _IONBF, 0);

    /* Mínimo: programa ip puerto un_tema  =>  argc == 4 */
    if (argc < 4) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *broker_ip = argv[1];
    char *port_end = NULL;
    unsigned long port_ul = strtoul(argv[2], &port_end, 10);
    if (port_end == argv[2] || *port_end != '\0' || port_ul == 0UL ||
        port_ul > 65535UL) {
        fprintf(stderr, "Puerto inválido: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in broker;
    memset(&broker, 0, sizeof broker);
    broker.sin_family = AF_INET;
    broker.sin_port = htons((uint16_t)port_ul);
    /* inet_pton: IP del broker (p. ej. otra VM) en formato texto → sin_addr. */
    if (inet_pton(AF_INET, broker_ip, &broker.sin_addr) != 1) {
        fprintf(stderr, "Dirección IPv4 inválida: %s\n", broker_ip);
        close(sock);
        return EXIT_FAILURE;
    }

    for (int i = 3; i < argc; i++) {
        const char *topic = argv[i];
        if (strchr(topic, ' ') != NULL) {
            fprintf(stderr, "El tema no debe contener espacios: %s\n", topic);
            close(sock);
            return EXIT_FAILURE;
        }
        if (strlen(topic) > PUBSUB_UDP_MAX_TOPIC_LEN) {
            fprintf(stderr, "Tema demasiado largo: %s\n", topic);
            close(sock);
            return EXIT_FAILURE;
        }
        if (send_sub(sock, &broker, topic) != 0) {
            close(sock);
            return EXIT_FAILURE;
        }
    }

    printf("[subscriber] Suscripciones enviadas. Esperando noticias (Ctrl+C)...\n");
    fflush(stdout);

    for (;;) {
        char buf[PUBSUB_UDP_MAX_MSG + 1];
        struct sockaddr_in from;
        socklen_t flen = sizeof from;
        /* recvfrom: OK SUB / NEWS del broker (u otro); "from" documenta el remitente. */
        ssize_t n = recvfrom(sock, buf, PUBSUB_UDP_MAX_MSG, 0,
                             (struct sockaddr *)&from, &flen);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }
        buf[n] = '\0';

        if (!udp_from_broker(&from, &broker)) {
            continue;
        }

        char fromstr[INET_ADDRSTRLEN];
        /* inet_ntop: copia la IPv4 a fromstr (seguro concurrente vs inet_ntoa). */
        inet_ntop(AF_INET, &from.sin_addr, fromstr, sizeof fromstr);

        uint16_t sport = ntohs(from.sin_port);
        if (strncmp(buf, PUBSUB_UDP_PREFIX_SUB_OK, strlen(PUBSUB_UDP_PREFIX_SUB_OK)) == 0) {
            print_sub_ok_line(buf, fromstr, sport);
        } else if (strncmp(buf, PUBSUB_UDP_PREFIX_NEWS,
                           strlen(PUBSUB_UDP_PREFIX_NEWS)) == 0) {
            print_news_line(buf, fromstr, sport);
        } else {
            printf("\n[subscriber] [%s:%u] (datagrama no estándar) %s", fromstr,
                   (unsigned)sport, buf);
            fflush(stdout);
        }
    }

    close(sock);
    return EXIT_SUCCESS;
}
