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

## Documentación detallada

La documentación de cada función — incluyendo parámetros, valor de retorno y comportamiento — se encuentra directamente en los archivos fuente:

- `broker_tcp.c`
- `publisher_tcp.c`
- `subscriber_tcp.c`

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
