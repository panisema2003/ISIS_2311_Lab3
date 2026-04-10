#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Definición de nuestra cabecera Mini-QUIC
typedef struct {
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
} subscriber_t;

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

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // 4. recvfrom: Escucha pasivamente CUALQUIER paquete UDP que llegue
        ssize_t received = recvfrom(sock, &packet, sizeof(mini_quic_packet), 0, 
                                    (struct sockaddr*)&client_addr, &client_len);
        
        if (received > 0) {
            // Lógica de la máquina de estados Mini-QUIC
            if (packet.type == 1) {
                // TIPO 1: Handshake (Un nuevo cliente se conectó)
                printf("[BROKER] Handshake recibido. Nuevo Connection ID: %u\n", packet.conn_id);
                
                // Si es el stream 0, asumimos que es un Suscriptor queriendo registrarse
                if (packet.stream_id == 0 && subs_count < 10) {
                    subs[subs_count].conn_id = packet.conn_id;
                    subs[subs_count].addr = client_addr;
                    subs_count++;
                    printf("[BROKER] Suscriptor registrado (Total: %d)\n", subs_count);
                }

                // Enviar ACK (Confirmación) al cliente
                mini_quic_packet ack = {2, packet.conn_id, packet.stream_id, packet.pkt_num, "ACK"};
                sendto(sock, &ack, sizeof(mini_quic_packet), 0, (struct sockaddr*)&client_addr, client_len);
            
            } else if (packet.type == 2) {
                // TIPO 2: Datos (Un Publisher envió una noticia)
                printf("[BROKER] Noticia recibida en Stream %u (CID: %u): %s\n", 
                       packet.stream_id, packet.conn_id, packet.payload);

                // Reenviar a todos los suscriptores simulando multiplexación
                for (int i = 0; i < subs_count; i++) {
                    // Mantenemos el Stream ID para que el suscriptor sepa de qué "canal" es
                    mini_quic_packet fwd_pkt = {2, subs[i].conn_id, packet.stream_id, packet.pkt_num, ""};
                    strcpy(fwd_pkt.payload, packet.payload);
                    
                    // 5. sendto: Enviar el paquete procesado hacia el suscriptor
                    sendto(sock, &fwd_pkt, sizeof(mini_quic_packet), 0, 
                           (struct sockaddr*)&subs[i].addr, sizeof(subs[i].addr));
                }
            }
        }
    }
    close(sock);
    return 0;
}