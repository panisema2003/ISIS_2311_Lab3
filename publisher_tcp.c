/*
 * publisher_tcp.c
 *
 * Uso: ./publisher_tcp "Colombia vs Brasil"
 *
 * DOCUMENTACIÓN DE FUNCIONES DE LIBRERÍA UTILIZADAS
 *
 * LIBRERÍAS INCLUIDAS (todas POSIX estándar):
 *   <stdio.h>      → printf(), snprintf()
 *   <stdlib.h>     → exit()
 *   <string.h>     → strlen(), memset()
 *   <unistd.h>     → close(), sleep()
 *   <sys/socket.h> → socket(), connect(), send()
 *   <netinet/in.h> → struct sockaddr_in, htons()
 *   <arpa/inet.h>  → inet_addr()
 *
 * FUNCIONES DE LIBRERÍA UTILIZADAS (todas POSIX estándar)
 *
 *  socket()     → crea el socket TCP. AF_INET=IPv4, SOCK_STREAM=TCP.
 *  memset()     → inicializa sockaddr_in en ceros antes de asignar sus campos.
 *  htons()      → convierte el puerto a formato de red (big-endian).
 *  inet_addr()  → convierte "127.0.0.1" al entero de 32 bits que usa sockaddr_in.
 *  connect()    → inicia la conexión TCP con el broker (handshake SYN/SYN-ACK/ACK).
 *  snprintf()   → construye el mensaje "[partido] evento" de forma segura.
 *  strlen()     → calcula cuántos bytes enviar con send().
 *  send()       → envía datos por TCP. flags=0 comportamiento estándar.
 *  sleep()      → pausa entre mensajes para simular eventos en tiempo real.
 *  close()      → cierra el socket y envía FIN al broker.
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
    // El nombre del partido viene como argumento, ej: ./publisher_tcp "Colombia vs Brasil"
    char *partido = (argc > 1) ? argv[1] : "Partido General";

    // Crear el socket TCP
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // Configurar la dirección del broker al que nos vamos a conectar
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));            // limpiar la estructura
    addr.sin_family      = AF_INET;            // IPv4
    addr.sin_port        = htons(BROKER_PORT); // puerto en formato de red
    addr.sin_addr.s_addr = inet_addr(BROKER_IP); // IP del broker en binario

    // Conectarse al broker — aquí ocurre el handshake TCP (SYN, SYN-ACK, ACK)
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); exit(1);
    }
    printf("[Publisher] Conectado. Transmitiendo: %s\n\n", partido);

    // Los 10 eventos que vamos a enviar simulando un partido en vivo
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
        // Construir el mensaje con formato "[partido] evento"
        snprintf(mensaje, sizeof(mensaje), "[%s] %s", partido, eventos[i]);

        // Enviar al broker — TCP garantiza que llega completo y en orden
        send(sock, mensaje, strlen(mensaje), 0);
        printf("[Publisher] Enviado: %s\n", mensaje);

        sleep(1);  // esperar 1 segundo para simular eventos en tiempo real
    }

    printf("\n[Publisher] Listo.\n");
    close(sock);  // cierra la conexión, el broker recibe FIN
    return 0;
}
