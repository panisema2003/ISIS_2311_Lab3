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
    uint32_t pkt_counter = 1;

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in broker_addr;
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(8080);
    broker_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP Localhost

    printf("[PUBLISHER] Iniciando con Connection ID: %u\n", my_conn_id);

    mini_quic_packet handshake = {1, my_conn_id, 99, pkt_counter++, "HOLA_BROKER"};
    sendto(sock, (char*)&handshake, sizeof(mini_quic_packet), 0, 
           (struct sockaddr*)&broker_addr, sizeof(broker_addr));
    printf("[PUBLISHER] Handshake enviado.\n");
    
    Sleep(1000); // Sleep en Windows toma milisegundos

    char* eventos[] = {"Empieza el partido", "Tarjeta Amarilla Min 12", "GOL Min 32!!"};
    
    for(int i=0; i<3; i++) {
        mini_quic_packet data_pkt = {2, my_conn_id, 5, pkt_counter++, ""};
        strcpy(data_pkt.payload, eventos[i]);
        
        sendto(sock, (char*)&data_pkt, sizeof(mini_quic_packet), 0, 
               (struct sockaddr*)&broker_addr, sizeof(broker_addr));
        
        printf("[PUBLISHER] Enviado a Stream 5: %s\n", data_pkt.payload);
        Sleep(3000); // 3 segundos
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}