#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

typedef struct {
    uint8_t  type;
    uint32_t conn_id;
    uint32_t stream_id;
    uint32_t pkt_num;
    char     payload[256];
} mini_quic_packet;

int main() {
    srand(time(NULL));
    uint32_t my_conn_id = rand() % 10000; // Connection ID del Suscriptor

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in broker_addr;
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &broker_addr.sin_addr);

    printf("[SUBSCRIBER] Iniciando con Connection ID: %u\n", my_conn_id);

    // 1. Simular Handshake para suscribirse
    // Usamos Stream 0 para decirle al Broker "Soy un suscriptor, registrame"
    mini_quic_packet sub_req = {1, my_conn_id, 0, 1, "SUBSCRIBE"};
    sendto(sock, &sub_req, sizeof(mini_quic_packet), 0, 
           (struct sockaddr*)&broker_addr, sizeof(broker_addr));

    printf("[SUBSCRIBER] Suscripción enviada. Esperando noticias...\n");

    mini_quic_packet incoming;
    while (1) {
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);

        // 2. Esperar bloqueado hasta que el Broker retransmita algo
        ssize_t received = recvfrom(sock, &incoming, sizeof(mini_quic_packet), 0, 
                                    (struct sockaddr*)&from_addr, &from_len);
        
        if (received > 0) {
            // Verificar que el mensaje viene dirigido a nuestra conexión
            if (incoming.conn_id == my_conn_id && incoming.type == 2) {
                // El diseño QUIC nos permite saber por qué canal (Stream) llegó la noticia
                printf(">>> [ALERTA] Noticia del Stream %u: %s (Paquete #%u)\n", 
                       incoming.stream_id, incoming.payload, incoming.pkt_num);
            }
        }
    }

    close(sock);
    return 0;
}