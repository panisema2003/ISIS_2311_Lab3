#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // Directiva para enlazar la librería en algunos compiladores

typedef struct {
    uint8_t  type;
    uint32_t conn_id;
    uint32_t stream_id;
    uint32_t pkt_num;
    char     payload[256];
} mini_quic_packet;

typedef struct {
    uint32_t conn_id;
    struct sockaddr_in addr;
} subscriber_t;

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

    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr); // En Windows usamos int en lugar de socklen_t

        int received = recvfrom(sock, (char*)&packet, sizeof(mini_quic_packet), 0, 
                                (struct sockaddr*)&client_addr, &client_len);
        
        if (received > 0) {
            if (packet.type == 1) {
                printf("[BROKER] Handshake recibido. Nuevo Connection ID: %u\n", packet.conn_id);
                
                if (packet.stream_id == 0 && subs_count < 10) {
                    subs[subs_count].conn_id = packet.conn_id;
                    subs[subs_count].addr = client_addr;
                    subs_count++;
                    printf("[BROKER] Suscriptor registrado (Total: %d)\n", subs_count);
                }

                mini_quic_packet ack = {2, packet.conn_id, packet.stream_id, packet.pkt_num, "ACK"};
                sendto(sock, (char*)&ack, sizeof(mini_quic_packet), 0, (struct sockaddr*)&client_addr, client_len);
            
            } else if (packet.type == 2) {
                printf("[BROKER] Noticia recibida en Stream %u (CID: %u): %s\n", 
                       packet.stream_id, packet.conn_id, packet.payload);

                for (int i = 0; i < subs_count; i++) {
                    mini_quic_packet fwd_pkt = {2, subs[i].conn_id, packet.stream_id, packet.pkt_num, ""};
                    strcpy(fwd_pkt.payload, packet.payload);
                    
                    sendto(sock, (char*)&fwd_pkt, sizeof(mini_quic_packet), 0, 
                           (struct sockaddr*)&subs[i].addr, sizeof(subs[i].addr));
                }
            }
        }
    }
    
    // Limpieza de Windows
    closesocket(sock);
    WSACleanup();
    return 0;
}