/*
 * subscriber_tcp.c
 *
 * Uso: ./subscriber_tcp "Hincha1"
 *
 * DOCUMENTACIÓN DE FUNCIONES DE LIBRERÍA UTILIZADAS
 *
 * LIBRERÍAS INCLUIDAS (todas POSIX estándar):
 *   <stdio.h>      → printf(), fflush()
 *   <stdlib.h>     → exit()
 *   <string.h>     → memset()
 *   <unistd.h>     → close()
 *   <sys/socket.h> → socket(), connect(), recv()
 *   <netinet/in.h> → struct sockaddr_in, htons()
 *   <arpa/inet.h>  → inet_addr()
 *
 * FUNCIONES DE LIBRERÍA UTILIZADAS (todas POSIX estándar)
 *
 *  socket()     → crea el socket TCP. AF_INET=IPv4, SOCK_STREAM=TCP.
 *  memset()     → inicializa sockaddr_in en ceros antes de asignar sus campos.
 *  htons()      → convierte el puerto a formato de red (big-endian).
 *  inet_addr()  → convierte "127.0.0.1" al entero de 32 bits que usa sockaddr_in.
 *  connect()    → establece la conexión TCP con el broker (handshake de tres vías).
 *  recv()       → recibe datos. Retorna >0 si hay datos, 0 si el broker cerró, <0 si error.
 *  fflush()     → fuerza que printf() imprima de inmediato sin acumular en buffer.
 *  close()      → cierra el socket y libera recursos del sistema.
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
    // Nombre del hincha, para identificar en pantalla quién recibe qué
    char *nombre = (argc > 1) ? argv[1] : "Suscriptor";

    // Crear el socket TCP
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // Configurar la dirección del broker
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));              // limpiar la estructura
    addr.sin_family      = AF_INET;              // IPv4
    addr.sin_port        = htons(BROKER_PORT);   // puerto en formato de red
    addr.sin_addr.s_addr = inet_addr(BROKER_IP); // IP del broker en binario

    // Conectarse al broker — handshake TCP
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); exit(1);
    }
    printf("[%s] Conectado. Esperando actualizaciones...\n\n", nombre);

    char buffer[BUFFER_SIZE];
    int bytes;

    // Quedarse escuchando hasta que el broker cierre la conexión
    // recv() bloquea aquí hasta que llegue algo
    // retorna 0 cuando el broker cierra → salimos del while
    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';               // terminar el string
        printf("[%s] >> %s\n", nombre, buffer);
        fflush(stdout);  // forzar que se imprima de inmediato (sin buffering)
    }

    printf("[%s] Conexión cerrada.\n", nombre);
    close(sock);
    return 0;
}
