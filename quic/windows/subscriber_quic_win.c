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

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    srand((unsigned int)time(NULL));
    uint32_t my_conn_id = rand() % 10000;

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in broker_addr;
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(8080);
    broker_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("[SUBSCRIBER] Iniciando con Connection ID: %u\n", my_conn_id);

    mini_quic_packet sub_req;
    memset(&sub_req, 0, sizeof(sub_req));
    sub_req.type = PKT_HANDSHAKE;
    sub_req.conn_id = my_conn_id;
    sub_req.stream_id = 0;
    sub_req.pkt_num = 1;
    strncpy(sub_req.payload, "SUBSCRIBE", sizeof(sub_req.payload) - 1);
    sendto(sock, (char*)&sub_req, sizeof(mini_quic_packet), 0, 
           (struct sockaddr*)&broker_addr, sizeof(broker_addr));

    printf("[SUBSCRIBER] Suscripcion enviada. Esperando noticias...\n");

    mini_quic_packet incoming;
    while (1) {
        struct sockaddr_in from_addr;
        int from_len = sizeof(from_addr);

        int received = recvfrom(sock, (char*)&incoming, sizeof(mini_quic_packet), 0, 
                                (struct sockaddr*)&from_addr, &from_len);
        
        if (received > 0) {
            if (incoming.conn_id == my_conn_id && incoming.type == PKT_DATA) {
                printf(">>> [ALERTA] Noticia del Stream %u: %s (Paquete #%u)\n", 
                       incoming.stream_id, incoming.payload, incoming.pkt_num);

                // ACK al broker para habilitar retransmisión broker->subscriber
                mini_quic_packet ack;
                memset(&ack, 0, sizeof(ack));
                ack.type = PKT_ACK;
                ack.conn_id = my_conn_id;
                ack.stream_id = incoming.stream_id;
                ack.pkt_num = incoming.pkt_num;
                memcpy(ack.payload, "ACK", 3);
                (void)sendto(sock, (char*)&ack, sizeof(ack), 0, (struct sockaddr*)&broker_addr, sizeof(broker_addr));
            }
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}