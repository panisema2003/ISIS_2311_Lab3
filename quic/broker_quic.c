#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>

// Definición de nuestra cabecera Mini-QUIC
// Nota: esto NO es QUIC real; es un "mini-quic" académico sobre UDP.
typedef enum {
    PKT_HANDSHAKE = 1,
    PKT_DATA      = 2,
    PKT_ACK       = 3
} mini_quic_type;

typedef struct __attribute__((packed)) {
    uint8_t  type;
    uint32_t conn_id;
    uint32_t stream_id;
    uint32_t pkt_num;
    char     payload[256];
} mini_quic_packet;

// Estructura para guardar a los suscriptores y poder reenviarles los mensajes
typedef struct {
    uint32_t conn_id;
    struct sockaddr_in addr;
    int has_outstanding;
    mini_quic_packet outstanding;
    uint64_t last_send_ms;
    int retries;
} subscriber_t;

static uint64_t now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
}

static void send_ack(int sock, const struct sockaddr_in *to, socklen_t to_len, const mini_quic_packet *for_pkt) {
    mini_quic_packet ack;
    memset(&ack, 0, sizeof(ack));
    ack.type = PKT_ACK;
    ack.conn_id = for_pkt->conn_id;
    ack.stream_id = for_pkt->stream_id;
    ack.pkt_num = for_pkt->pkt_num;
    memcpy(ack.payload, "ACK", 3);
    (void)sendto(sock, &ack, sizeof(ack), 0, (const struct sockaddr*)to, to_len);
}

int main() {
    // 1. Creación del Socket UDP (La base de QUIC)
    // AF_INET: IPv4 | SOCK_DGRAM: Datagramas (UDP) | 0: Protocolo por defecto
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("Error creando socket"); exit(1); }

    // 2. Configuración de la dirección del Broker
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Escucha en todas las interfaces
    server_addr.sin_port = htons(8080);       // Puerto 8080

    // 3. Bind: Asociar el socket al puerto 8080 del sistema operativo
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind"); exit(1);
    }

    printf("[BROKER] Mini-QUIC iniciado en puerto 8080...\n");

    // Arreglo rudimentario para guardar hasta 10 suscriptores
    subscriber_t subs[10];
    int subs_count = 0;
    mini_quic_packet packet;
    memset(subs, 0, sizeof(subs));

    const uint64_t RETX_TIMEOUT_MS = 500;
    const int MAX_RETRIES = 5;

    while (1) {
        // Usamos select() para no bloquear indefinidamente:
        // nos deja procesar timers de retransmisión en el mismo hilo.
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100 * 1000; // 100ms

        int sel = select(sock + 1, &rfds, NULL, NULL, &timeout);
        if (sel > 0 && FD_ISSET(sock, &rfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            ssize_t received = recvfrom(sock, &packet, sizeof(packet), 0,
                                        (struct sockaddr*)&client_addr, &client_len);

            if (received > 0) {
                if (packet.type == PKT_HANDSHAKE) {
                    printf("[BROKER] Handshake recibido. CID: %u Stream: %u\n", packet.conn_id, packet.stream_id);

                    // Si es el stream 0, asumimos que es un Suscriptor queriendo registrarse
                    if (packet.stream_id == 0) {
                        int already = 0;
                        for (int i = 0; i < subs_count; i++) {
                            if (subs[i].conn_id == packet.conn_id) { already = 1; break; }
                        }
                        if (!already && subs_count < 10) {
                            subs[subs_count].conn_id = packet.conn_id;
                            subs[subs_count].addr = client_addr;
                            subs[subs_count].has_outstanding = 0;
                            subs[subs_count].retries = 0;
                            subs[subs_count].last_send_ms = 0;
                            subs_count++;
                            printf("[BROKER] Suscriptor registrado (Total: %d)\n", subs_count);
                        }
                    }

                    // ACK del handshake (para que el cliente pueda retransmitir si se pierde)
                    send_ack(sock, &client_addr, client_len, &packet);
                } else if (packet.type == PKT_DATA) {
                    // DATA desde publisher o reenvío (pero aquí actuamos como broker)
                    printf("[BROKER] DATA Stream %u (CID: %u Pkt: %u): %s\n",
                           packet.stream_id, packet.conn_id, packet.pkt_num, packet.payload);

                    // ACK al emisor (publisher) para confiabilidad publisher->broker
                    send_ack(sock, &client_addr, client_len, &packet);

                    // Reenviar a todos los suscriptores y marcar como "outstanding"
                    for (int i = 0; i < subs_count; i++) {
                        mini_quic_packet fwd_pkt;
                        memset(&fwd_pkt, 0, sizeof(fwd_pkt));
                        fwd_pkt.type = PKT_DATA;
                        fwd_pkt.conn_id = subs[i].conn_id;      // dirigido a ese subscriber
                        fwd_pkt.stream_id = packet.stream_id;   // preservamos stream
                        fwd_pkt.pkt_num = packet.pkt_num;       // mismo número
                        strncpy(fwd_pkt.payload, packet.payload, sizeof(fwd_pkt.payload) - 1);

                        (void)sendto(sock, &fwd_pkt, sizeof(fwd_pkt), 0,
                                     (struct sockaddr*)&subs[i].addr, sizeof(subs[i].addr));

                        subs[i].has_outstanding = 1;
                        subs[i].outstanding = fwd_pkt;
                        subs[i].last_send_ms = now_ms();
                        subs[i].retries = 0;
                    }
                } else if (packet.type == PKT_ACK) {
                    // ACK desde subscriber o publisher.
                    // Publisher ACKs los manejamos en su lado; aquí sólo nos interesa subscriber->broker.
                    for (int i = 0; i < subs_count; i++) {
                        if (subs[i].conn_id == packet.conn_id && subs[i].has_outstanding) {
                            if (subs[i].outstanding.pkt_num == packet.pkt_num &&
                                subs[i].outstanding.stream_id == packet.stream_id) {
                                subs[i].has_outstanding = 0;
                                subs[i].retries = 0;
                                subs[i].last_send_ms = 0;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Timer de retransmisión broker->subscriber
        uint64_t now = now_ms();
        for (int i = 0; i < subs_count; i++) {
            if (!subs[i].has_outstanding) continue;
            if (now - subs[i].last_send_ms < RETX_TIMEOUT_MS) continue;

            if (subs[i].retries >= MAX_RETRIES) {
                // dejamos de insistir en este paquete
                subs[i].has_outstanding = 0;
                continue;
            }

            (void)sendto(sock, &subs[i].outstanding, sizeof(subs[i].outstanding), 0,
                         (struct sockaddr*)&subs[i].addr, sizeof(subs[i].addr));
            subs[i].retries++;
            subs[i].last_send_ms = now;
        }
    }
    close(sock);
    return 0;
}