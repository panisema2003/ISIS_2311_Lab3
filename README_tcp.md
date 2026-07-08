# README — Implementación TCP

## Librerías utilizadas

Todas las librerías usadas son **POSIX estándar**, incluidas por defecto en cualquier sistema Linux. No se usaron librerías externas.

| Librería | Funciones utilizadas |
|---|---|
| `<stdio.h>` | `printf()`, `snprintf()`, `fflush()` |
| `<stdlib.h>` | `exit()`, `malloc()`, `free()` |
| `<string.h>` | `memset()`, `strlen()` |
| `<unistd.h>` | `close()`, `sleep()` |
| `<sys/socket.h>` | `socket()`, `bind()`, `listen()`, `accept()`, `connect()`, `send()`, `recv()`, `setsockopt()` |
| `<netinet/in.h>` | `struct sockaddr_in`, `htons()`, `INADDR_ANY` |
| `<arpa/inet.h>` | `inet_addr()` |
| `<pthread.h>` | `pthread_create()`, `pthread_join()`, `pthread_detach()`, `pthread_mutex_lock()`, `pthread_mutex_unlock()` |

---

## Documentación de funciones

### `socket(int domain, int type, int protocol)`
Crea un nuevo socket y retorna su descriptor.
- `domain`: `AF_INET` para usar IPv4.
- `type`: `SOCK_STREAM` para TCP (orientado a conexión, confiable).
- `protocol`: `0` para que el sistema elija automáticamente.
- **Retorna:** descriptor >= 0 si ok, -1 si error.

---

### `setsockopt(int sock, int level, int optname, void *val, socklen_t len)`
Configura opciones del socket.
- `SOL_SOCKET` + `SO_REUSEADDR`: permite reusar el puerto inmediatamente al reiniciar el programa, evitando el error "Address already in use".

---

### `memset(void *ptr, int valor, size_t n)`
Llena n bytes de memoria con un valor. Se usa para inicializar la estructura `sockaddr_in` en ceros antes de asignar sus campos, evitando valores basura.

---

### `htons(uint16_t puerto)`
Convierte el número de puerto del formato del host al formato de red (big-endian). Necesario para compatibilidad entre máquinas con distintas arquitecturas.

---

### `inet_addr(const char *ip)`
Convierte una cadena con una dirección IP (ej. `"127.0.0.1"`) al entero de 32 bits en formato de red que requiere `sockaddr_in`.
- **Retorna:** la IP en formato binario, o `INADDR_NONE` si la cadena es inválida.

---

### `bind(int sock, struct sockaddr *addr, socklen_t len)`
Asocia el socket a una dirección IP y puerto específicos. Con `INADDR_ANY` acepta conexiones en cualquier interfaz de red.
- **Retorna:** 0 si ok, -1 si error.

---

### `listen(int sock, int backlog)`
Pone el socket en modo servidor, listo para aceptar conexiones entrantes.
- `backlog`: máximo de conexiones que pueden esperar en cola.
- **Retorna:** 0 si ok, -1 si error.

---

### `accept(int sock, struct sockaddr *addr, socklen_t *len)`
Acepta una conexión entrante y retorna un socket nuevo dedicado a ese cliente. El socket original sigue escuchando.
- `addr` y `len` pueden ser `NULL` si no interesa la dirección del cliente.
- **Retorna:** nuevo descriptor de socket, -1 si error.
- **Bloquea** hasta que llegue una conexión.

---

### `connect(int sock, struct sockaddr *addr, socklen_t len)`
Inicia la conexión TCP con el servidor. Ejecuta el handshake de tres vías: SYN → SYN-ACK → ACK.
- **Retorna:** 0 si exitoso, -1 si error.
- **Bloquea** hasta que el servidor acepte o rechace la conexión.

---

### `send(int sock, void *buf, size_t len, int flags)`
Envía datos por el socket TCP.
- `flags`: `0` para comportamiento estándar.
- TCP garantiza que los datos llegan completos y en orden.
- **Retorna:** bytes enviados, -1 si error.

---

### `recv(int sock, void *buf, size_t len, int flags)`
Recibe datos del socket TCP y los escribe en `buf`.
- `flags`: `0` para comportamiento estándar.
- **Retorna:** bytes recibidos (>0), 0 si el otro lado cerró la conexión, -1 si error.
- **Bloquea** hasta que lleguen datos.

---

### `close(int sock)`
Cierra el socket y libera sus recursos. En TCP envía un paquete FIN al otro extremo para cerrar la conexión limpiamente.

---

### `snprintf(char *buf, size_t n, const char *fmt, ...)`
Escribe una cadena formateada en `buf` con un máximo de `n-1` caracteres, evitando desbordamiento de buffer.

---

### `strlen(const char *s)`
Retorna la longitud de la cadena `s` sin contar el `'\0'` final. Se usa para indicarle a `send()` cuántos bytes enviar.

---

### `fflush(FILE *stream)`
Vacía el buffer de salida y fuerza la escritura inmediata en pantalla. Sin esto, `printf()` podría acumular texto y no mostrarlo de inmediato.

---

### `sleep(unsigned int segundos)`
Suspende la ejecución del proceso por el número de segundos dado. Se usa para simular eventos en tiempo real entre mensajes.

---

### `pthread_create(pthread_t *tid, pthread_attr_t *attr, void *(*func)(void*), void *arg)`
Crea un nuevo hilo que ejecuta la función `func` con el argumento `arg`.
- `attr`: `NULL` para atributos por defecto.
- **Retorna:** 0 si ok, número de error si falla.

---

### `pthread_join(pthread_t tid, void **retval)`
Espera a que el hilo identificado por `tid` termine.
- `retval`: `NULL` si no interesa el valor de retorno.

---

### `pthread_detach(pthread_t tid)`
Indica que el hilo se limpiará solo al terminar, sin necesidad de `pthread_join()`.

---

### `pthread_mutex_lock(pthread_mutex_t *mutex)`
Bloquea el mutex para acceso exclusivo a un recurso compartido. Si otro hilo ya lo tiene bloqueado, espera hasta que se libere.

---

### `pthread_mutex_unlock(pthread_mutex_t *mutex)`
Libera el mutex para que otros hilos puedan acceder al recurso compartido.

---

### `malloc(size_t size)`
Reserva `size` bytes en el heap y retorna un puntero al bloque. Se usa para pasar el descriptor de socket a cada hilo de forma segura.

---

### `free(void *ptr)`
Libera la memoria reservada con `malloc()`.

---

## Compilación

### Archivos en C
```bash
gcc -o broker broker_tcp.c -lpthread
gcc -o publisher publisher_tcp.c
gcc -o subscriber subscriber_tcp.c
```

### Archivos en Assembly (x86-64)
```bash
nasm -f elf64 broker_tcp.asm -o broker_tcp.o && ld broker_tcp.o -o broker_tcp
nasm -f elf64 publisher_tcp.asm -o publisher_tcp.o && ld publisher_tcp.o -o publisher_tcp
nasm -f elf64 subscriber_tcp.asm -o subscriber_tcp.o && ld subscriber_tcp.o -o subscriber_tcp
```

## Ejecución

Abrir 4 terminales y ejecutar en orden:

```bash
# Terminal 1 — Broker
./broker

# Terminal 2 — Subscriber 1
./subscriber "Hincha1"

# Terminal 3 — Subscriber 2
./subscriber "Hincha2"

# Terminal 4 — Publisher
./publisher "Colombia vs Brasil"
```

## Puertos utilizados

| Puerto | Uso |
|---|---|
| 9000 | Publishers → Broker |
| 9001 | Broker → Subscribers |
