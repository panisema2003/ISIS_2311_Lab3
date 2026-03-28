/**
 * publisher_udp.c — Publicador UDP (periodista deportivo simulado).
 *
 * Envía al broker datagramas con el formato PUB <tema>|<cuerpo>, asociados a un
 * partido concreto (tema). Los mensajes demo siguen una línea de tiempo: cada
 * evento tiene una espera distinta antes de enviarse (ritmo irregular, como en
 * un partido real: ratos sin novedad y rachas de jugadas). Por defecto al menos
 * 10 envíos para el laboratorio; tras eso puede escribir líneas por stdin.
 *
 * Uso:
 *   ./publisher_udp <ip_broker> <puerto_broker> <tema> [-n N]
 *     -n N   cuántos eventos de la simulación enviar (por defecto 12).
 *
 * Ejemplo:
 *   ./publisher_udp 127.0.0.1 9000 EquipoA_vs_EquipoB
 *
 * Compilación (Linux): gcc -Wall -Wextra -std=c11 -o publisher_udp publisher_udp.c
 *
 * Documentación punto a punto de cabeceras estándar/POSIX y sockets: README.md
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

static void strip_trailing_crlf(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[n - 1] = '\0';
        n--;
    }
}

/**
 * Interpreta tema tipo "EquipoC_vs_EquipoD": equipos local y visita (slugs sin espacios).
 * Si no hay "_vs_", usa el tema completo como local y "Rival" como visita.
 */
static void topic_to_teams(const char *topic, char *home, size_t home_sz, char *away,
                           size_t away_sz) {
    const char *sep = strstr(topic, "_vs_");
    if (sep == NULL || sep == topic) {
        snprintf(home, home_sz, "%s", topic);
        snprintf(away, away_sz, "Rival");
        return;
    }
    size_t hlen = (size_t)(sep - topic);
    if (hlen >= home_sz) {
        hlen = home_sz - 1U;
    }
    memcpy(home, topic, hlen);
    home[hlen] = '\0';
    snprintf(away, away_sz, "%s", sep + 4);
    if (away[0] == '\0') {
        snprintf(away, away_sz, "Rival");
    }
}

/** Construye la línea de narración del índice idx según los equipos del tema. */
static int build_timeline_message(char *out, size_t out_sz, size_t idx, const char *home,
                                  const char *away) {
    switch (idx) {
    case 0:
        return snprintf(out, out_sz,
                        "Inicia el partido: %s contra %s. Saque inicial para %s.", home,
                        away, home);
    case 1:
        return snprintf(out, out_sz, "Minuto 5: primera llegada clara, disparo desviado.");
    case 2:
        return snprintf(out, out_sz, "Minuto 12: tarjeta amarilla al #8 de %s.", away);
    case 3:
        return snprintf(out, out_sz, "Minuto 18: GOL de %s — asistencia del #10.", home);
    case 4:
        return snprintf(out, out_sz, "Minuto 24: Cambio: entra #14, sale #7 (%s).", away);
    case 5:
        return snprintf(out, out_sz,
                        "Minuto 31: GOL de %s de cabeza tras tiro de esquina.", away);
    case 6:
        return snprintf(out, out_sz, "Minuto 38: Tarjeta amarilla al #10 de %s.", home);
    case 7:
        return snprintf(out, out_sz,
                        "Minuto 45+1: Fin del primer tiempo. Marcador 1-1 (%s - %s).", home,
                        away);
    case 8:
        return snprintf(out, out_sz, "Inicia el segundo tiempo.");
    case 9:
        return snprintf(out, out_sz,
                        "Minuto 58: GOL de %s al contraataque (marcador 2-1).", home);
    case 10:
        return snprintf(out, out_sz, "Minuto 67: Cambio doble en %s.", away);
    case 11:
        return snprintf(out, out_sz, "Minuto 74: Tarjeta roja directa al #4 de %s.", away);
    case 12:
        return snprintf(out, out_sz, "Minuto 81: GOL de %s de penal — marcador 3-1.", home);
    case 13:
        return snprintf(out, out_sz, "Minuto 90: El árbitro añade 4 minutos.");
    case 14:
        return snprintf(out, out_sz, "Final del partido: victoria de %s 3-1 sobre %s.", home,
                        away);
    default:
        return snprintf(out, out_sz, "Actualización en vivo (%s vs %s).", home, away);
    }
}

static int build_pub_datagram(char *out, size_t out_sz, const char *topic,
                              const char *body) {
    int w = snprintf(out, out_sz, "%s%s|%s\n", PUBSUB_UDP_PREFIX_PUB, topic, body);
    if (w < 0 || (size_t)w >= out_sz) {
        fprintf(stderr, "[publisher] Mensaje demasiado largo.\n");
        return -1;
    }
    return 0;
}

static int send_pub(int sock, const char *topic, const char *body) {
    char buf[PUBSUB_UDP_MAX_MSG + 1];
    if (build_pub_datagram(buf, sizeof buf, topic, body) != 0) {
        return -1;
    }
    size_t len = strlen(buf);
    /* Tras connect(UDP) al broker, send() entrega el datagrama al mismo destino. */
    ssize_t n = send(sock, buf, len, 0);
    if (n < 0) {
        perror("[publisher] send");
        return -1;
    }
    printf("[publisher] Enviado (%zu bytes): %s", len, buf);
    fflush(stdout);
    return 0;
}

static void usage(const char *argv0) {
    fprintf(stderr,
            "Uso: %s <ip_broker> <puerto_broker> <tema> [-n N]\n"
            "  <tema>   identificador del partido, sin espacios (ej. EquipoA_vs_EquipoB)\n"
            "  -n N     eventos de la línea de tiempo a enviar (por defecto 12)\n",
            argv0);
}

/** Solo retrasos entre eventos (el texto depende del tema vía build_timeline_message). */
typedef struct {
    unsigned delay_before_send_ms;
} MatchEvent;

static const MatchEvent match_timeline[] = {
    {0},    {1150}, {3100}, {850}, {1950}, {1050}, {920}, {1550}, {2800}, {480},
    {2250}, {780},  {1680}, {1320}, {1100},
};

static const size_t match_timeline_len = sizeof match_timeline / sizeof match_timeline[0];

/** Pausa entre “vueltas” si -n pide más eventos de los definidos en la tabla. */
static const unsigned k_between_cycle_pause_ms = 2500u;

int main(int argc, char **argv) {
    (void)setvbuf(stdout, NULL, _IONBF, 0);
    (void)setvbuf(stderr, NULL, _IONBF, 0);

    int auto_count = 12;
    int base_args = 4;

    if (argc < base_args) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            char *end = NULL;
            unsigned long v = strtoul(argv[i + 1], &end, 10);
            if (end == argv[i + 1] || *end != '\0' || v == 0UL || v > 10000UL) {
                fprintf(stderr, "Valor inválido para -n\n");
                return EXIT_FAILURE;
            }
            auto_count = (int)v;
            i++;
        } else {
            fprintf(stderr, "Argumento desconocido: %s\n", argv[i]);
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    const char *broker_ip = argv[1];
    if (strcmp(broker_ip, "127.0.0.1") == 0 || strcmp(broker_ip, "localhost") == 0) {
        fprintf(stderr,
                "[publisher] AVISO: 127.0.0.1/localhost es ESTA máquina. Si el broker está en "
                "OTRA VM, use su IPv4 (p. ej. salida de `hostname -I` en la VM del broker). "
                "Si no, los datagramas no llegarán al broker.\n");
    }
    char *port_end = NULL;
    unsigned long port_ul = strtoul(argv[2], &port_end, 10);
    if (port_end == argv[2] || *port_end != '\0' || port_ul == 0UL ||
        port_ul > 65535UL) {
        fprintf(stderr, "Puerto inválido: %s\n", argv[2]);
        return EXIT_FAILURE;
    }
    const char *topic = argv[3];
    if (strchr(topic, ' ') != NULL || strchr(topic, '|') != NULL) {
        fprintf(stderr, "El tema no debe contener espacios ni '|'.\n");
        return EXIT_FAILURE;
    }
    if (strlen(topic) > PUBSUB_UDP_MAX_TOPIC_LEN) {
        fprintf(stderr, "Tema demasiado largo (máx %d caracteres).\n",
                PUBSUB_UDP_MAX_TOPIC_LEN);
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
    /* inet_pton: dirección del broker desde línea de comandos → binario IPv4. */
    if (inet_pton(AF_INET, broker_ip, &broker.sin_addr) != 1) {
        fprintf(stderr, "Dirección IPv4 inválida: %s\n", broker_ip);
        close(sock); /* cerrar fd si no hubo dirección válida */
        return EXIT_FAILURE;
    }

    if (connect(sock, (const struct sockaddr *)&broker, sizeof broker) < 0) {
        perror("[publisher] connect UDP al broker");
        close(sock);
        return EXIT_FAILURE;
    }

    char home[64], away[64];
    topic_to_teams(topic, home, sizeof home, away, sizeof away);

    printf("[publisher] Broker %s:%lu, tema '%s'. Simulación: %d evento(s) "
           "(intervalos variables, estilo transmisión en vivo).\n",
           broker_ip, port_ul, topic, auto_count);

    for (int i = 0; i < auto_count; i++) {
        size_t idx = (size_t)i % match_timeline_len;
        unsigned wait_ms;
        if (i == 0) {
            wait_ms = match_timeline[0].delay_before_send_ms;
        } else if (idx == 0) {
            wait_ms = k_between_cycle_pause_ms;
        } else {
            wait_ms = match_timeline[idx].delay_before_send_ms;
        }
        usleep((useconds_t)(wait_ms * 1000u));

        char msgbuf[512];
        int mw = build_timeline_message(msgbuf, sizeof msgbuf, idx, home, away);
        if (mw < 0 || (size_t)mw >= sizeof msgbuf) {
            fprintf(stderr, "[publisher] Evento de línea de tiempo demasiado largo.\n");
            close(sock);
            return EXIT_FAILURE;
        }
        if (send_pub(sock, topic, msgbuf) != 0) {
            close(sock); /* sendto falló; liberar descriptor */
            return EXIT_FAILURE;
        }
    }

    printf("[publisher] Mensajes demo enviados. Escriba líneas adicionales (Enter para "
           "enviar, EOF/Ctrl+D para terminar):\n");
    char line[1024];
    while (fgets(line, sizeof line, stdin) != NULL) {
        strip_trailing_crlf(line);
        if (line[0] == '\0') {
            continue;
        }
        if (send_pub(sock, topic, line) != 0) {
            break;
        }
    }

    close(sock); /* fin normal: cerrar el socket UDP */
    return EXIT_SUCCESS;
}
