/*
 * publisher_tcp.c
 * Uso: ./publisher_tcp "Colombia vs Brasil"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BROKER_IP   "127.0.0.1"
#define BROKER_PORT 9000
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    char *partido = (argc > 1) ? argv[1] : "Partido General";

    // socket(): crea el socket TCP. AF_INET=IPv4, SOCK_STREAM=TCP
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    // memset(): inicializa la estructura en ceros para evitar valores basura
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    // htons(): convierte el puerto a formato de red (big-endian)
    addr.sin_port = htons(BROKER_PORT);
    // inet_addr(): convierte "127.0.0.1" al entero de 32 bits que usa sockaddr_in
    addr.sin_addr.s_addr = inet_addr(BROKER_IP);

    // connect(): inicia la conexión TCP con el broker (handshake SYN/SYN-ACK/ACK)
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); exit(1);
    }
    printf("[Publisher] Conectado. Transmitiendo: %s\n\n", partido);

    const char *eventos[] = {
        "Inicio del partido",
        "Tiro de esquina para el equipo local",
        "GOOOL del equipo local al minuto 12",
        "Tarjeta amarilla al numero 8 del visitante",
        "Fin del primer tiempo. Marcador: 1-0",
        "Inicio del segundo tiempo",
        "Cambio: jugador 11 entra por jugador 7",
        "GOOOL del visitante al minuto 67. Empate 1-1",
        "Tarjeta roja al numero 4 del local al minuto 88",
        "Fin del partido. Resultado final: 1-1"
    };

    char mensaje[BUFFER_SIZE];
    int total = sizeof(eventos) / sizeof(eventos[0]);

    for (int i = 0; i < total; i++) {
        // snprintf(): construye "[partido] evento" de forma segura sin desbordar el buffer
        snprintf(mensaje, sizeof(mensaje), "[%s] %s", partido, eventos[i]);
        // strlen(): calcula cuántos bytes enviar
        // send(): envía el mensaje por TCP. flags=0 comportamiento estándar
        send(sock, mensaje, strlen(mensaje), 0);
        printf("[Publisher] Enviado: %s\n", mensaje);
        // sleep(): pausa 1 segundo para simular eventos en tiempo real
        sleep(1);
    }

    printf("\n[Publisher] Listo.\n");
    // close(): cierra la conexión, el broker recibe FIN
    close(sock);
    return 0;
}
