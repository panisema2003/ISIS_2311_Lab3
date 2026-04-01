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
     * connect(socket, addr, tamaño): inicia la conexión TCP con el servidor.
     * Internamente ejecuta el handshake de tres vías: SYN, SYN-ACK, ACK.
     * - socket: descriptor del socket a conectar.
     * - addr: dirección y puerto del servidor al que conectarse.
     * - tamaño: tamaño de la estructura addr.
     * Retorna: 0 si la conexión fue exitosa, -1 si error.
     * BLOQUEA hasta que el servidor acepte o rechace la conexión.
     */
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
    int total = sizeof(eventos) / sizeof(eventos[0]);  // = 10

    for (int i = 0; i < total; i++) {
        /*
         * snprintf(buffer, tamaño, formato, ...): escribe una cadena formateada en buffer.
         * A diferencia de sprintf, limita la escritura a tamaño-1 caracteres,
         * evitando desbordamiento de buffer.
         */
        snprintf(mensaje, sizeof(mensaje), "[%s] %s", partido, eventos[i]);

        /*
         * send(socket, buffer, longitud, flags): envía datos por el socket TCP.
         * - socket: descriptor del socket por donde se envía.
         * - buffer: datos a enviar.
         * - longitud: cantidad de bytes. Usamos strlen() para no enviar el '\0'.
         * - flags: 0 para comportamiento estándar.
         * TCP garantiza que los datos llegan completos y en orden.
         * Retorna: bytes enviados, -1 si error.
         */
        send(sock, mensaje, strlen(mensaje), 0);
        printf("[Publisher] Enviado: %s\n", mensaje);

        /*
         * sleep(segundos): suspende la ejecución del proceso por ese tiempo.
         * Simulamos 1 segundo entre eventos para imitar una transmisión en tiempo real.
         */
        sleep(1);
    }

    printf("\n[Publisher] Listo.\n");
    /*
     * close(socket): cierra el socket y libera sus recursos.
     * En TCP envía un paquete FIN al broker para cerrar la conexión limpiamente.
     */
    close(sock);
    return 0;
}
