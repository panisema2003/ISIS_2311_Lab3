#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib") // Directiva para enlazar la librería en algunos compiladores

typedef enum {
    PKT_HANDSHAKE = 1,
    PKT_DATA      = 2,
    PKT_ACK       = 3
} mini_quic_type;

#pragma pack(push, 1)
typedef struct {
    uint8_t  type;
    uint32_t conn_id;
    uint32_t stream_id;
    uint32_t pkt_num;
    char     payload[256];
} mini_quic_packet;
#pragma pack(pop)

typedef struct {
    uint32_t conn_id;
    struct sockaddr_in addr;
    int has_outstanding;
    mini_quic_packet outstanding;
    ULONGLONG last_send_ms;
    int retries;
} subscriber_t;

static ULONGLONG now_ms(void) {
    return GetTickCount64();
}

static void send_ack(SOCKET sock, const struct sockaddr_in *to, int to_len, const mini_quic_packet *for_pkt) {
    mini_quic_packet ack;
    memset(&ack, 0, sizeof(ack));
    ack.type = PKT_ACK;
    ack.conn_id = for_pkt->conn_id;
    ack.stream_id = for_pkt->stream_id;
    ack.pkt_num = for_pkt->pkt_num;
    memcpy(ack.payload, "ACK", 3);
    (void)sendto(sock, (const char*)&ack, sizeof(ack), 0, (const struct sockaddr*)to, to_len);
}

int main() {
    // 1. Inicializar Winsock (Requisito exclusivo de Windows)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Error inicializando Winsock. Codigo: %d\n", WSAGetLastError());
        return 1;
    }

    // 2. Crear Socket UDP
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Error creando socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    // 3. Bind
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Error en bind: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("[BROKER] Mini-QUIC (Windows) iniciado en puerto 8080...\n");

    subscriber_t subs[10];
    int subs_count = 0;
    mini_quic_packet packet;
    memset(subs, 0, sizeof(subs));

    const ULONGLONG RETX_TIMEOUT_MS = 500;
    const int MAX_RETRIES = 5;

    while (1) {
        // select() para poder procesar timers
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;

        int sel = select(0, &rfds, NULL, NULL, &tv); // nfds se ignora en Winsock
        if (sel > 0 && FD_ISSET(sock, &rfds)) {
            struct sockaddr_in client_addr;
            int client_len = sizeof(client_addr);

            int received = recvfrom(sock, (char*)&packet, sizeof(packet), 0,
                                    (struct sockaddr*)&client_addr, &client_len);
            if (received > 0) {
                if (packet.type == PKT_HANDSHAKE) {
                    printf("[BROKER] Handshake recibido. CID: %u Stream: %u\n", packet.conn_id, packet.stream_id);

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

                    send_ack(sock, &client_addr, client_len, &packet);
                } else if (packet.type == PKT_DATA) {
                    printf("[BROKER] DATA Stream %u (CID: %u Pkt: %u): %s\n",
                           packet.stream_id, packet.conn_id, packet.pkt_num, packet.payload);

                    // ACK al publisher
                    send_ack(sock, &client_addr, client_len, &packet);

                    for (int i = 0; i < subs_count; i++) {
                        mini_quic_packet fwd_pkt;
                        memset(&fwd_pkt, 0, sizeof(fwd_pkt));
                        fwd_pkt.type = PKT_DATA;
                        fwd_pkt.conn_id = subs[i].conn_id;
                        fwd_pkt.stream_id = packet.stream_id;
                        fwd_pkt.pkt_num = packet.pkt_num;
                        strncpy(fwd_pkt.payload, packet.payload, sizeof(fwd_pkt.payload) - 1);

                        (void)sendto(sock, (char*)&fwd_pkt, sizeof(fwd_pkt), 0,
                                     (struct sockaddr*)&subs[i].addr, sizeof(subs[i].addr));

                        subs[i].has_outstanding = 1;
                        subs[i].outstanding = fwd_pkt;
                        subs[i].last_send_ms = now_ms();
                        subs[i].retries = 0;
                    }
                } else if (packet.type == PKT_ACK) {
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
        ULONGLONG now = now_ms();
        for (int i = 0; i < subs_count; i++) {
            if (!subs[i].has_outstanding) continue;
            if (now - subs[i].last_send_ms < RETX_TIMEOUT_MS) continue;

            if (subs[i].retries >= MAX_RETRIES) {
                subs[i].has_outstanding = 0;
                continue;
            }

            (void)sendto(sock, (char*)&subs[i].outstanding, sizeof(subs[i].outstanding), 0,
                         (struct sockaddr*)&subs[i].addr, sizeof(subs[i].addr));
            subs[i].retries++;
            subs[i].last_send_ms = now;
        }
    }
    
    // Limpieza de Windows
    closesocket(sock);
    WSACleanup();
    return 0;
}