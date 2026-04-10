#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

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

int main() {
    srand(time(NULL));
    uint32_t my_conn_id = rand() % 10000; // Connection ID del Suscriptor

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("Error creando socket"); exit(1); }
    
    struct sockaddr_in broker_addr;
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &broker_addr.sin_addr);

    printf("[SUBSCRIBER] Iniciando con Connection ID: %u\n", my_conn_id);

    // 1. Simular Handshake para suscribirse
    // Usamos Stream 0 para decirle al Broker "Soy un suscriptor, registrame"
    mini_quic_packet sub_req;
    memset(&sub_req, 0, sizeof(sub_req));
    sub_req.type = PKT_HANDSHAKE;
    sub_req.conn_id = my_conn_id;
    sub_req.stream_id = 0;
    sub_req.pkt_num = 1;
    strncpy(sub_req.payload, "SUBSCRIBE", sizeof(sub_req.payload) - 1);
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
            if (incoming.conn_id == my_conn_id && incoming.type == PKT_DATA) {
                // El diseño QUIC nos permite saber por qué canal (Stream) llegó la noticia
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
                (void)sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr*)&broker_addr, sizeof(broker_addr));
            }
        }
    }

    close(sock);
    return 0;
}