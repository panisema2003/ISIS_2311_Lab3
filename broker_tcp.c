/*
 * broker_tcp.c
 * Broker TCP usando hilos — un hilo por cada cliente conectado.
 *
 * Puertos:
 *   9000 : publishers
 *   9001 : subscribers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
// pthread.h: librería POSIX de hilos. Provee pthread_create, pthread_mutex_lock, etc.
#include <pthread.h>

#define PORT_PUB    9000
#define PORT_SUB    9001
#define MAX_CLIENTS 50
#define BUFFER_SIZE 1024

// Lista de subscribers conectados
int subscribers[MAX_CLIENTS];
int num_sub = 0;

// pthread_mutex_t: mutex para proteger subscribers[] cuando varios hilos lo tocan
// PTHREAD_MUTEX_INITIALIZER: inicializa el mutex en estado desbloqueado
pthread_mutex_t sub_mutex = PTHREAD_MUTEX_INITIALIZER;

// Manda el mensaje a todos los subscribers conectados
void broadcast(const char *msg, int len) {
    // pthread_mutex_lock(): bloquea el mutex para acceso exclusivo a subscribers[]
    pthread_mutex_lock(&sub_mutex);
    for (int i = 0; i < num_sub; i++) {
        // send(): envía datos por TCP. flags=0 comportamiento estándar
        send(subscribers[i], msg, len, 0);
    }
    // pthread_mutex_unlock(): libera el mutex para que otros hilos puedan usarlo
    pthread_mutex_unlock(&sub_mutex);
}

// Hilo que atiende a un publisher: recibe sus mensajes y los redistribuye
void *handle_publisher(void *arg) {
    int sock = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    int bytes;

    // recv(): recibe mensajes del publisher. Retorna 0 cuando se desconecta
    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';
        printf("[Broker] Recibido: %s\n", buffer);
        broadcast(buffer, bytes);
    }

    printf("[Broker] Publisher desconectado (socket %d)\n", sock);
    // close(): cierra el socket y libera recursos
    close(sock);
    return NULL;
}

// Hilo que atiende a un subscriber: lo registra y espera hasta que se desconecte
void *handle_subscriber(void *arg) {
    int sock = *(int *)arg;
    free(arg);

    // Agregar el subscriber a la lista con acceso exclusivo
    pthread_mutex_lock(&sub_mutex);
    subscribers[num_sub++] = sock;
    printf("[Broker] Subscriber conectado (socket %d). Total: %d\n", sock, num_sub);
    pthread_mutex_unlock(&sub_mutex);

    // recv(): los subscribers no mandan datos, pero usamos recv() para detectar desconexión
    char tmp[1];
    while (recv(sock, tmp, 1, 0) > 0);

    // Se desconectó — sacarlo de la lista
    pthread_mutex_lock(&sub_mutex);
    for (int i = 0; i < num_sub; i++) {
        if (subscribers[i] == sock) {
            subscribers[i] = subscribers[--num_sub];
            break;
        }
    }
    printf("[Broker] Subscriber desconectado (socket %d). Total: %d\n", sock, num_sub);
    pthread_mutex_unlock(&sub_mutex);

    close(sock);
    return NULL;
}

// Hilo que acepta conexiones de publishers
void *accept_publishers(void *arg) {
    int server = *(int *)arg;

    while (1) {
        // accept(): espera y acepta una conexión entrante de un publisher
        int client = accept(server, NULL, NULL);
        if (client < 0) continue;

        // pthread_create(): lanza un hilo nuevo para atender a este publisher
        // handle_publisher es la función que ejecutará el hilo
        pthread_t tid;
        int *sock_ptr = malloc(sizeof(int));
        *sock_ptr = client;
        pthread_create(&tid, NULL, handle_publisher, sock_ptr);
        // pthread_detach(): el hilo se limpia solo al terminar, sin necesidad de join()
        pthread_detach(tid);
    }
    return NULL;
}

// Hilo que acepta conexiones de subscribers
void *accept_subscribers(void *arg) {
    int server = *(int *)arg;

    while (1) {
        // accept(): espera y acepta una conexión entrante de un subscriber
        int client = accept(server, NULL, NULL);
        if (client < 0) continue;

        pthread_t tid;
        int *sock_ptr = malloc(sizeof(int));
        *sock_ptr = client;
        // pthread_create(): lanza un hilo nuevo para atender a este subscriber
        pthread_create(&tid, NULL, handle_subscriber, sock_ptr);
        pthread_detach(tid);
    }
    return NULL;
}

// Crea y configura un socket servidor en el puerto dado
int create_server(int port) {
    // socket(): crea el socket. AF_INET=IPv4, SOCK_STREAM=TCP
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // setsockopt(): con SO_REUSEADDR evita "Address already in use" al reiniciar
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    // memset(): inicializa la estructura en ceros para evitar valores basura
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    // htons(): convierte el puerto a formato de red (big-endian)
    addr.sin_port        = htons(port);

    // bind(): asocia el socket a la IP y puerto definidos arriba
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    // listen(): pone el socket en modo servidor, hasta 10 conexiones en cola
    listen(sock, 10);

    printf("[Broker] Escuchando en puerto %d\n", port);
    return sock;
}

int main() {
    int server_pub = create_server(PORT_PUB);
    int server_sub = create_server(PORT_SUB);
    printf("[Broker] Listo. Esperando conexiones...\n\n");

    // Lanzar los hilos que aceptan conexiones entrantes
    pthread_t pub_thread, sub_thread;
    // pthread_create(): crea un hilo que corre accept_publishers en paralelo
    pthread_create(&pub_thread, NULL, accept_publishers, &server_pub);
    // pthread_create(): crea un hilo que corre accept_subscribers en paralelo
    pthread_create(&sub_thread, NULL, accept_subscribers, &server_sub);

    // pthread_join(): espera a que los hilos terminen (no terminan, el broker corre siempre)
    pthread_join(pub_thread, NULL);
    pthread_join(sub_thread, NULL);

    close(server_pub);
    close(server_sub);
    return 0;
}
