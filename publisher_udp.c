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

static int build_pub_datagram(char *out, size_t out_sz, const char *topic,
                              const char *body) {
    int w = snprintf(out, out_sz, "%s%s|%s\n", PUBSUB_UDP_PREFIX_PUB, topic, body);
    if (w < 0 || (size_t)w >= out_sz) {
        fprintf(stderr, "[publisher] Mensaje demasiado largo.\n");
        return -1;
    }
    return 0;
}

static int send_pub(int sock, const struct sockaddr_in *broker, const char *topic,
                    const char *body) {
    char buf[PUBSUB_UDP_MAX_MSG + 1];
    if (build_pub_datagram(buf, sizeof buf, topic, body) != 0) {
        return -1;
    }
    size_t len = strlen(buf);
    ssize_t n = sendto(sock, buf, len, 0, (const struct sockaddr *)broker,
                       (socklen_t)sizeof(*broker));
    if (n < 0) {
        perror("[publisher] sendto");
        return -1;
    }
    printf("[publisher] Enviado (%zu bytes): %s", len, buf);
    return 0;
}

static void usage(const char *argv0) {
    fprintf(stderr,
            "Uso: %s <ip_broker> <puerto_broker> <tema> [-n N]\n"
            "  <tema>   identificador del partido, sin espacios (ej. EquipoA_vs_EquipoB)\n"
            "  -n N     eventos de la línea de tiempo a enviar (por defecto 12)\n",
            argv0);
}

/**
 * Un evento de la transmisión: texto que se publica y milisegundos a esperar
 * antes de enviarlo (desde el envío anterior). El primero suele llevar 0.
 * Los intervalos son irregulares a propósito (silencios largos, rachas cortas).
 */
typedef struct {
    const char *text;
    unsigned delay_before_send_ms;
} MatchEvent;

static const MatchEvent match_timeline[] = {
    {"Inicia el partido, saque inicial Equipo A.", 0},
    {"Minuto 5: primera llegada clara, disparo desviado.", 1150},
    {"Minuto 12: tarjeta amarilla al #8 de Equipo B.", 3100},
    {"Minuto 18: GOL de Equipo A — asistencia del #10.", 850},
    {"Minuto 24: Cambio: entra #14, sale #7 (Equipo B).", 1950},
    {"Minuto 31: GOL de Equipo B de cabeza tras tiro de esquina.", 1050},
    {"Minuto 38: Tarjeta amarilla al #10 de Equipo A.", 920},
    {"Minuto 45+1: Fin del primer tiempo. Marcador 1-1.", 1550},
    {"Inicia el segundo tiempo.", 2800},
    {"Minuto 58: GOL de Equipo A al contraataque (marcador 2-1).", 480},
    {"Minuto 67: Cambio doble en Equipo B.", 2250},
    {"Minuto 74: Tarjeta roja directa al #4 de Equipo B.", 780},
    {"Minuto 81: GOL de Equipo A de penal — marcador 3-1.", 1680},
    {"Minuto 90: El árbitro añade 4 minutos.", 1320},
    {"Final del partido: victoria Equipo A 3-1.", 1100},
};

static const size_t match_timeline_len = sizeof match_timeline / sizeof match_timeline[0];

/** Pausa entre “vueltas” si -n pide más eventos de los definidos en la tabla. */
static const unsigned k_between_cycle_pause_ms = 2500u;

int main(int argc, char **argv) {
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
    if (inet_pton(AF_INET, broker_ip, &broker.sin_addr) != 1) {
        fprintf(stderr, "Dirección IPv4 inválida: %s\n", broker_ip);
        close(sock);
        return EXIT_FAILURE;
    }

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

        const char *msg = match_timeline[idx].text;
        if (send_pub(sock, &broker, topic, msg) != 0) {
            close(sock);
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
        if (send_pub(sock, &broker, topic, line) != 0) {
            break;
        }
    }

    close(sock);
    return EXIT_SUCCESS;
}
