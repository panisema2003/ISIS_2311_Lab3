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
 * Compilación (Linux): gcc -Wall -Wextra -std=c11 -o subscriber_udp subscriber_udp.c
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
    printf("[subscriber] Registrado SUB '%s' (%zd bytes enviados)\n", topic, n);
    return 0;
}

static void usage(const char *argv0) {
    fprintf(stderr,
            "Uso: %s <ip_broker> <puerto_broker> <tema1> [tema2 ...]\n"
            "  Cada <tema> es un partido (sin espacios), ej. EquipoA_vs_EquipoB\n",
            argv0);
}

int main(int argc, char **argv) {
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

    printf("[subscriber] Esperando noticias (Ctrl+C para salir)...\n");

    for (;;) {
        char buf[PUBSUB_UDP_MAX_MSG + 1];
        struct sockaddr_in from;
        socklen_t flen = sizeof from;
        ssize_t n = recvfrom(sock, buf, PUBSUB_UDP_MAX_MSG, 0,
                             (struct sockaddr *)&from, &flen);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }
        buf[n] = '\0';

        char fromstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from.sin_addr, fromstr, sizeof fromstr);

        if (strncmp(buf, PUBSUB_UDP_PREFIX_ACK, strlen(PUBSUB_UDP_PREFIX_ACK)) == 0) {
            printf("[subscriber] Confirmación broker: %s", buf);
        } else if (strncmp(buf, PUBSUB_UDP_PREFIX_NEWS,
                           strlen(PUBSUB_UDP_PREFIX_NEWS)) == 0) {
            printf("[subscriber] [%s:%u] %s", fromstr, ntohs(from.sin_port), buf);
        } else {
            printf("[subscriber] [%s:%u] (datagrama) %s", fromstr, ntohs(from.sin_port),
                   buf);
        }
    }

    close(sock);
    return EXIT_SUCCESS;
}
