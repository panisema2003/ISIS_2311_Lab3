#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

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

static int wait_for_ack(SOCKET sock, uint32_t conn_id, uint32_t stream_id, uint32_t pkt_num, int timeout_ms) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int sel = select(0, &rfds, NULL, NULL, &tv); // nfds ignored on Winsock
    if (sel <= 0) return 0;

    mini_quic_packet incoming;
    struct sockaddr_in from;
    int from_len = sizeof(from);
    int r = recvfrom(sock, (char*)&incoming, sizeof(incoming), 0, (struct sockaddr*)&from, &from_len);
    if (r <= 0) return 0;

    return (incoming.type == PKT_ACK &&
            incoming.conn_id == conn_id &&
            incoming.stream_id == stream_id &&
            incoming.pkt_num == pkt_num);
}

static int send_with_retx(SOCKET sock, const struct sockaddr_in *to, int to_len, const mini_quic_packet *pkt) {
    const int TIMEOUT_MS = 500;
    const int MAX_RETRIES = 5;

    for (int attempt = 0; attempt <= MAX_RETRIES; attempt++) {
        (void)sendto(sock, (const char*)pkt, sizeof(*pkt), 0, (const struct sockaddr*)to, to_len);
        if (wait_for_ack(sock, pkt->conn_id, pkt->stream_id, pkt->pkt_num, TIMEOUT_MS)) {
            return 1;
        }
    }
    return 0;
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    srand((unsigned int)time(NULL));
    uint32_t my_conn_id = rand() % 10000;
    uint32_t pkt_counter = 1;

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in broker_addr;
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(8080);
    broker_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP Localhost

    printf("[PUBLISHER] Iniciando con Connection ID: %u\n", my_conn_id);

    mini_quic_packet handshake;
    memset(&handshake, 0, sizeof(handshake));
    handshake.type = PKT_HANDSHAKE;
    handshake.conn_id = my_conn_id;
    handshake.stream_id = 99;
    handshake.pkt_num = pkt_counter++;
    strncpy(handshake.payload, "HOLA_BROKER", sizeof(handshake.payload) - 1);

    if (!send_with_retx(sock, &broker_addr, sizeof(broker_addr), &handshake)) {
        printf("[PUBLISHER] Handshake sin ACK tras reintentos. Abortando.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    printf("[PUBLISHER] Handshake OK (ACK recibido).\n");

    char* eventos[] = {"Empieza el partido", "Tarjeta Amarilla Min 12", "GOL Min 32!!"};
    
    for(int i=0; i<3; i++) {
        mini_quic_packet data_pkt;
        memset(&data_pkt, 0, sizeof(data_pkt));
        data_pkt.type = PKT_DATA;
        data_pkt.conn_id = my_conn_id;
        data_pkt.stream_id = 5;
        data_pkt.pkt_num = pkt_counter++;
        strncpy(data_pkt.payload, eventos[i], sizeof(data_pkt.payload) - 1);

        if (!send_with_retx(sock, &broker_addr, sizeof(broker_addr), &data_pkt)) {
            printf("[PUBLISHER] DATA sin ACK (pkt=%u). Abortando.\n", data_pkt.pkt_num);
            closesocket(sock);
            WSACleanup();
            return 1;
        }
        printf("[PUBLISHER] Enviado y ACK recibido (Stream 5): %s\n", data_pkt.payload);
        Sleep(3000); // 3 segundos
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}