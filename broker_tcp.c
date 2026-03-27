/*
 * broker_tcp.c
 * 
 * Puertos:
 *   9000 → publishers
 *   9001 → subscribers
 *
 * FUNCIONES DE LIBRERÍA UTILIZADAS (todas POSIX estándar)
 *
 *  socket()      → crea el socket. AF_INET=IPv4, SOCK_STREAM=TCP.
 *  setsockopt()  → con SO_REUSEADDR evita "Address already in use" al reiniciar.
 *  memset()      → inicializa sockaddr_in en ceros antes de asignar sus campos.
 *  htons()       → convierte el puerto de formato host a formato de red (big-endian).
 *  bind()        → asocia el socket a una IP y puerto. INADDR_ANY = cualquier interfaz.
 *  listen()      → pone el socket en modo servidor, listo para aceptar conexiones.
 *  accept()      → acepta una conexión entrante y retorna un socket nuevo para ese cliente.
 *  send()        → envía datos por el socket. flags=0 comportamiento estándar.
 *  recv()        → recibe datos. Retorna 0 si el otro lado cerró la conexión.
 *  close()       → cierra el socket. En TCP envía FIN al otro extremo.
 *  FD_ZERO()     → vacía el conjunto de sockets antes de usarlo con select().
 *  FD_SET()      → agrega un socket al conjunto para que select() lo vigile.
 *  FD_ISSET()    → consulta si un socket tuvo actividad después de select().
 *  select()      → espera actividad en múltiples sockets sin usar hilos.
 *                  nfds=descriptor_más_alto+1, NULL en wfds/efds/tv = solo lectura
 *                  sin timeout. Al retornar, el conjunto solo tiene los sockets listos.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT_PUB    9000
#define PORT_SUB    9001
#define MAX_CLIENTS 50
#define BUFFER_SIZE 1024

// Guardamos el socket de cada cliente conectado en estos arreglos
int publishers[MAX_CLIENTS];
int subscribers[MAX_CLIENTS];
int num_pub = 0;  // cuántos publishers hay conectados ahora
int num_sub = 0;  // cuántos subscribers hay conectados ahora

// Crea y configura un socket servidor en el puerto dado
int create_server(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);  // socket TCP

    // Evita el error "Address already in use" si reinicias el broker rápido
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Definir en qué IP y puerto va a escuchar
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));       // limpiar basura de memoria
    addr.sin_family      = AF_INET;       // IPv4
    addr.sin_addr.s_addr = INADDR_ANY;    // acepta conexiones en cualquier interfaz
    addr.sin_port        = htons(port);   // convertir puerto a formato de red

    bind(sock, (struct sockaddr *)&addr, sizeof(addr));  // asignar IP y puerto al socket
    listen(sock, 10);  // empezar a escuchar, hasta 10 conexiones en cola

    printf("[Broker] Escuchando en puerto %d\n", port);
    return sock;
}

// Manda el mensaje a todos los subscribers conectados
void broadcast(const char *msg, int len) {
    for (int i = 0; i < num_sub; i++) {
        send(subscribers[i], msg, len, 0);
    }
}

int main() {
    // Crear un servidor para publishers y otro para subscribers
    int server_pub = create_server(PORT_PUB);
    int server_sub = create_server(PORT_SUB);
    printf("[Broker] Listo. Esperando conexiones...\n\n");

    while (1) {
        // fd_set es el "conjunto de sockets" que select() va a vigilar
        fd_set read_fds;
        FD_ZERO(&read_fds);               // vaciar el conjunto antes de usarlo
        FD_SET(server_pub, &read_fds);    // agregar el servidor de publishers
        FD_SET(server_sub, &read_fds);    // agregar el servidor de subscribers

        // select() necesita saber cuál es el descriptor más alto
        int max_fd = server_pub > server_sub ? server_pub : server_sub;

        // También vigilar cada publisher ya conectado (por si mandan un mensaje)
        for (int i = 0; i < num_pub; i++) {
            FD_SET(publishers[i], &read_fds);
            if (publishers[i] > max_fd) max_fd = publishers[i];
        }

        // Pausar aquí hasta que algún socket tenga actividad
        // Cuando select() retorna, read_fds solo tiene los sockets con datos listos
        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        // ¿Llegó un nuevo publisher?
        if (FD_ISSET(server_pub, &read_fds)) {
            int new_pub = accept(server_pub, NULL, NULL);  // aceptar la conexión
            publishers[num_pub++] = new_pub;               // guardarlo en el arreglo
            printf("[Broker] Publisher conectado (socket %d). Total: %d\n", new_pub, num_pub);
        }

        // ¿Llegó un nuevo subscriber?
        if (FD_ISSET(server_sub, &read_fds)) {
            int new_sub = accept(server_sub, NULL, NULL);
            subscribers[num_sub++] = new_sub;
            printf("[Broker] Subscriber conectado (socket %d). Total: %d\n", new_sub, num_sub);
        }

        // Revisar si algún publisher mandó un mensaje
        for (int i = 0; i < num_pub; i++) {
            if (FD_ISSET(publishers[i], &read_fds)) {
                char buffer[BUFFER_SIZE];
                int bytes = recv(publishers[i], buffer, sizeof(buffer) - 1, 0);

                if (bytes <= 0) {
                    // recv retorna 0 cuando el publisher cerró la conexión
                    printf("[Broker] Publisher desconectado (socket %d)\n", publishers[i]);
                    close(publishers[i]);
                    publishers[i] = publishers[--num_pub];  // sacar del arreglo
                    i--;  // ajustar índice porque el arreglo se achicó
                } else {
                    buffer[bytes] = '\0';  // terminar el string correctamente
                    printf("[Broker] Recibido: %s\n", buffer);
                    broadcast(buffer, bytes);  // reenviar a todos los subscribers
                }
            }
        }
    }

    close(server_pub);
    close(server_sub);
    return 0;
}
