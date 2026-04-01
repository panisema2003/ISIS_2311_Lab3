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
// pthread.h: librería POSIX de hilos. Necesaria para crear hilos y usar mutex
#include <pthread.h>

#define PORT_PUB    9000
#define PORT_SUB    9001
#define MAX_CLIENTS 50
#define BUFFER_SIZE 1024

// Arreglo que guarda el socket de cada subscriber conectado
int subscribers[MAX_CLIENTS];
int num_sub = 0;

/*
 * pthread_mutex_t: tipo de dato que representa un mutex (mutual exclusion).
 * Se usa para que solo un hilo a la vez pueda modificar subscribers[].
 * Sin esto, dos hilos podrían modificar el arreglo al mismo tiempo y corromperse.
 * PTHREAD_MUTEX_INITIALIZER: macro que inicializa el mutex en estado desbloqueado.
 */
pthread_mutex_t sub_mutex = PTHREAD_MUTEX_INITIALIZER;

// Envía un mensaje a todos los subscribers conectados
void broadcast(const char *msg, int len) {
    /*
     * pthread_mutex_lock(mutex): bloquea el mutex.
     * Si otro hilo ya lo tiene bloqueado, este se queda esperando hasta que se libere.
     * Parámetro: puntero al mutex a bloquear.
     */
    pthread_mutex_lock(&sub_mutex);

    for (int i = 0; i < num_sub; i++) {
        /*
         * send(socket, buffer, longitud, flags): envía datos por un socket TCP.
         * - socket: descriptor del socket destino.
         * - buffer: datos a enviar.
         * - longitud: cantidad de bytes a enviar.
         * - flags: 0 para comportamiento estándar.
         * Retorna: bytes enviados, o -1 si hay error.
         */
        send(subscribers[i], msg, len, 0);
    }

    /*
     * pthread_mutex_unlock(mutex): libera el mutex para que otros hilos puedan usarlo.
     * Parámetro: puntero al mutex a liberar.
     */
    pthread_mutex_unlock(&sub_mutex);
}

// Hilo que atiende a un publisher: recibe sus mensajes y los redistribuye
void *handle_publisher(void *arg) {
    int sock = *(int *)arg;
    free(arg);  // liberamos la memoria que se reservó para pasar el socket al hilo

    char buffer[BUFFER_SIZE];
    int bytes;

    /*
     * recv(socket, buffer, tamaño, flags): recibe datos de un socket TCP.
     * - socket: descriptor del socket del que se lee.
     * - buffer: donde se guardan los datos recibidos.
     * - tamaño: máximo de bytes a leer (usamos -1 para dejar espacio al '\0').
     * - flags: 0 para comportamiento estándar.
     * Retorna: bytes recibidos, 0 si el otro lado cerró la conexión, -1 si error.
     */
    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';  // terminamos el string para poder imprimirlo
        printf("[Broker] Recibido: %s\n", buffer);
        broadcast(buffer, bytes);
    }

    printf("[Broker] Publisher desconectado (socket %d)\n", sock);
    /*
     * close(socket): cierra el socket y libera sus recursos del sistema operativo.
     * En TCP esto envía un paquete FIN al otro extremo para cerrar limpiamente.
     * Parámetro: descriptor del socket a cerrar.
     */
    close(sock);
    return NULL;
}

// Hilo que atiende a un subscriber: lo registra y espera hasta que se desconecte
void *handle_subscriber(void *arg) {
    int sock = *(int *)arg;
    free(arg);

    // Agregar el subscriber a la lista protegiendo con mutex
    pthread_mutex_lock(&sub_mutex);
    subscribers[num_sub++] = sock;
    printf("[Broker] Subscriber conectado (socket %d). Total: %d\n", sock, num_sub);
    pthread_mutex_unlock(&sub_mutex);

    /*
     * Los subscribers solo reciben mensajes, no envían.
     * Usamos recv() de todas formas para detectar cuándo se desconectan:
     * cuando retorna 0, significa que el subscriber cerró la conexión.
     */
    char tmp[1];
    while (recv(sock, tmp, 1, 0) > 0);

    // El subscriber se desconectó, lo sacamos de la lista
    pthread_mutex_lock(&sub_mutex);
    for (int i = 0; i < num_sub; i++) {
        if (subscribers[i] == sock) {
            // Reemplazamos con el último para no dejar huecos en el arreglo
            subscribers[i] = subscribers[--num_sub];
            break;
        }
    }
    printf("[Broker] Subscriber desconectado (socket %d). Total: %d\n", sock, num_sub);
    pthread_mutex_unlock(&sub_mutex);

    close(sock);
    return NULL;
}

// Hilo que acepta conexiones entrantes de publishers
void *accept_publishers(void *arg) {
    int server = *(int *)arg;

    while (1) {
        /*
         * accept(socket, addr, addrlen): extrae la primera conexión pendiente de la cola
         * y crea un nuevo socket dedicado a ese cliente.
         * - socket: el socket servidor que está escuchando.
         * - addr: dirección del cliente (NULL si no nos interesa).
         * - addrlen: tamaño de addr (NULL si no nos interesa).
         * Retorna: nuevo descriptor de socket para ese cliente, -1 si error.
         * BLOQUEA hasta que llegue una conexión.
         */
        int client = accept(server, NULL, NULL);
        if (client < 0) continue;

        /*
         * pthread_create(tid, attr, funcion, arg): crea un nuevo hilo.
         * - tid: identificador del hilo creado (lo necesitamos para detach/join).
         * - attr: NULL para atributos por defecto.
         * - funcion: función que ejecutará el hilo.
         * - arg: argumento que se le pasa a la función.
         * Retorna: 0 si ok, número de error si falla.
         */
        pthread_t tid;
        int *sock_ptr = malloc(sizeof(int));
        *sock_ptr = client;
        pthread_create(&tid, NULL, handle_publisher, sock_ptr);

        /*
         * pthread_detach(tid): indica que el hilo se limpiará solo al terminar.
         * Sin esto habría que hacer pthread_join() para liberar sus recursos.
         * Parámetro: identificador del hilo a desacoplar.
         */
        pthread_detach(tid);
    }
    return NULL;
}

// Hilo que acepta conexiones entrantes de subscribers
void *accept_subscribers(void *arg) {
    int server = *(int *)arg;

    while (1) {
        int client = accept(server, NULL, NULL);
        if (client < 0) continue;

        pthread_t tid;
        int *sock_ptr = malloc(sizeof(int));
        *sock_ptr = client;
        pthread_create(&tid, NULL, handle_subscriber, sock_ptr);
        pthread_detach(tid);
    }
    return NULL;
}

// Crea y configura un socket servidor TCP en el puerto dado
int create_server(int port) {
    /*
     * socket(domain, type, protocol): crea un nuevo socket.
     * - domain: AF_INET para usar IPv4.
     * - type: SOCK_STREAM para TCP (orientado a conexión, confiable).
     * - protocol: 0 para que el sistema elija automáticamente (TCP para SOCK_STREAM).
     * Retorna: descriptor del socket, -1 si error.
     */
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    /*
     * setsockopt(socket, nivel, opción, valor, tamaño): configura opciones del socket.
     * - SOL_SOCKET: nivel del socket general.
     * - SO_REUSEADDR: permite reusar el puerto inmediatamente al reiniciar el programa,
     *   evitando el error "Address already in use".
     */
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    /*
     * memset(ptr, valor, tamaño): llena un bloque de memoria con un valor.
     * Usamos 0 para inicializar la estructura addr y evitar valores basura.
     */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;     // familia de direcciones IPv4
    addr.sin_addr.s_addr = INADDR_ANY;  // acepta conexiones en cualquier interfaz de red
    /*
     * htons(puerto): convierte el número de puerto del formato del host
     * al formato de red (big-endian). Necesario para que funcione entre
     * máquinas con distintas arquitecturas.
     */
    addr.sin_port = htons(port);

    /*
     * bind(socket, addr, tamaño): asocia el socket a la dirección IP y puerto definidos.
     * Sin esto el sistema operativo no sabe en qué puerto escuchar.
     * Retorna: 0 si ok, -1 si error.
     */
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));

    /*
     * listen(socket, backlog): marca el socket como pasivo (modo servidor).
     * - backlog: máximo de conexiones que pueden esperar en cola (aquí 10).
     * Retorna: 0 si ok, -1 si error.
     */
    listen(sock, 10);

    printf("[Broker] Escuchando en puerto %d\n", port);
    return sock;
}

int main() {
    int server_pub = create_server(PORT_PUB);
    int server_sub = create_server(PORT_SUB);
    printf("[Broker] Listo. Esperando conexiones...\n\n");

    // Lanzar los dos hilos que aceptan conexiones en paralelo
    pthread_t pub_thread, sub_thread;
    pthread_create(&pub_thread, NULL, accept_publishers,  &server_pub);
    pthread_create(&sub_thread, NULL, accept_subscribers, &server_sub);

    /*
     * pthread_join(tid, retval): espera a que un hilo termine.
     * - tid: identificador del hilo a esperar.
     * - retval: donde guardar el valor de retorno del hilo (NULL si no interesa).
     * En este caso los hilos nunca terminan, así que el broker corre indefinidamente.
     */
    pthread_join(pub_thread, NULL);
    pthread_join(sub_thread, NULL);

    close(server_pub);
    close(server_sub);
    return 0;
}
