/*
 * subscriber_tcp.c
 * Uso: ./subscriber_tcp "Hincha1"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BROKER_IP   "127.0.0.1"
#define BROKER_PORT 9001
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    char *nombre = (argc > 1) ? argv[1] : "Suscriptor";

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

    // connect(): establece la conexión TCP con el broker (handshake de tres vías)
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); exit(1);
    }
    printf("[%s] Conectado. Esperando actualizaciones...\n\n", nombre);

    char buffer[BUFFER_SIZE];
    int bytes;

    // recv(): recibe datos del broker. Retorna 0 cuando el broker cierra la conexión
    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';
        printf("[%s] >> %s\n", nombre, buffer);
        // fflush(): fuerza que printf() imprima de inmediato sin acumular en buffer
        fflush(stdout);
    }

    printf("[%s] Conexión cerrada.\n", nombre);
    // close(): cierra el socket y libera recursos
    close(sock);
    return 0;
}
