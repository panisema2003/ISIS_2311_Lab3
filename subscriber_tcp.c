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

    /*
     * socket(domain, type, protocol): crea un nuevo socket.
     * - domain: AF_INET para usar IPv4.
     * - type: SOCK_STREAM para TCP (orientado a conexión, confiable).
     * - protocol: 0 para que el sistema elija automáticamente.
     * Retorna: descriptor del socket, -1 si error.
     */
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    /*
     * memset(ptr, valor, tamaño): llena un bloque de memoria con un valor.
     * Usamos 0 para inicializar addr y evitar valores basura en sus campos.
     */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    /*
     * htons(puerto): convierte el número de puerto del formato del host
     * al formato de red (big-endian). Necesario para compatibilidad entre
     * máquinas con distintas arquitecturas.
     */
    addr.sin_port = htons(BROKER_PORT);
    /*
     * inet_addr(ip): convierte una cadena con una dirección IP (ej. "127.0.0.1")
     * al entero de 32 bits en formato de red que requiere sockaddr_in.
     * Retorna: la IP en formato binario, o INADDR_NONE si la cadena es inválida.
     */
    addr.sin_addr.s_addr = inet_addr(BROKER_IP);

    /*
     * connect(socket, addr, tamaño): establece la conexión TCP con el broker.
     * Ejecuta el handshake de tres vías: SYN, SYN-ACK, ACK.
     * - socket: descriptor del socket a conectar.
     * - addr: dirección y puerto del servidor al que conectarse.
     * - tamaño: tamaño de la estructura addr.
     * Retorna: 0 si exitoso, -1 si error.
     * BLOQUEA hasta que el broker responda.
     */
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); exit(1);
    }
    printf("[%s] Conectado. Esperando actualizaciones...\n\n", nombre);

    char buffer[BUFFER_SIZE];
    int bytes;

    /*
     * recv(socket, buffer, tamaño, flags): recibe datos de un socket TCP.
     * - socket: descriptor del socket del que se lee.
     * - buffer: donde se guardan los datos recibidos.
     * - tamaño: máximo de bytes a leer (usamos -1 para dejar espacio al '\0').
     * - flags: 0 para comportamiento estándar.
     * Retorna: bytes recibidos (>0), 0 si el broker cerró la conexión, -1 si error.
     * BLOQUEA hasta que lleguen datos.
     */
    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';  // terminamos el string para poder imprimirlo
        printf("[%s] >> %s\n", nombre, buffer);
        /*
         * fflush(stream): vacía el buffer de salida y fuerza la escritura inmediata.
         * Sin esto printf() podría acumular texto y no mostrarlo de inmediato,
         * haciendo parecer que los mensajes no llegan en tiempo real.
         */
        fflush(stdout);
    }

    printf("[%s] Conexión cerrada.\n", nombre);
    /*
     * close(socket): cierra el socket y libera sus recursos del sistema operativo.
     * En TCP envía un paquete FIN al broker para cerrar la conexión limpiamente.
     */
    close(sock);
    return 0;
}
