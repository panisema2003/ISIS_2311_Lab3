#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

typedef struct {
    uint8_t  type;
    uint32_t conn_id;
    uint32_t stream_id;
    uint32_t pkt_num;
    char     payload[256];
} mini_quic_packet;

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

    mini_quic_packet sub_req = {1, my_conn_id, 0, 1, "SUBSCRIBE"};
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
            if (incoming.conn_id == my_conn_id && incoming.type == 2) {
                printf(">>> [ALERTA] Noticia del Stream %u: %s (Paquete #%u)\n", 
                       incoming.stream_id, incoming.payload, incoming.pkt_num);
            }
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}