# Laboratorio 3 — Sistema pub/sub deportivo (UDP)

Implementación en C del broker, publicador y suscriptor sobre **datagramas UDP** (sockets POSIX). No se utilizan bibliotecas de terceros: solo **libc** (C estándar) y cabeceras **POSIX** habituales en Linux.

## Compilación

En el directorio del proyecto:

```bash
make
```

Genera `broker_udp`, `publisher_udp` y `subscriber_udp`. `make clean` elimina los binarios.

## Uso rápido

- Broker: `./broker_udp [puerto]` (por defecto puerto definido en `pubsub_udp.h`, típicamente 9000).
- Publicador: `./publisher_udp <ip_broker> <puerto> <tema> [-n N]`
- Suscriptor: `./subscriber_udp <ip_broker> <puerto> <tema1> [tema2 ...]`

---

## Documentación de cabeceras y funciones (requisito del laboratorio)

A continuación se describe **qué cabecera se incluye**, **qué función o símbolo se usa** y **para qué** en cada módulo. Esto documenta la interacción del programa con la API estándar (incluida la de sockets), sin capas externas ocultas.

### `pubsub_udp.h` (proyecto) y `<stddef.h>`

| Símbolo / inclusión | Uso |
|---------------------|-----|
| `#include <stddef.h>` | Tipo `size_t` (tamaños de objetos). Los `.c` también obtienen `size_t` vía `<stdio.h>` o `<string.h>`. |
| `PUBSUB_UDP_DEFAULT_PORT` | Puerto del broker si no se pasa por línea de comandos. |
| `PUBSUB_UDP_MAX_MSG` | Tope de carga útil por datagrama (reducir fragmentación UDP). |
| `PUBSUB_UDP_MAX_TOPIC_LEN` | Longitud máxima del nombre de tema/partido. |
| `PUBSUB_UDP_MAX_SUBS_PER_TOPIC`, `PUBSUB_UDP_MAX_TOPICS` | Límites de las tablas en memoria del broker. |
| `PUBSUB_UDP_PREFIX_*` | Literales de protocolo: `SUB `, `PUB `, `ACK SUB `, `NEWS `. |

El formato de mensajes de aplicación está descrito en el comentario inicial de `pubsub_udp.h`.

---

### `broker_udp.c`

| Cabecera | Funciones / símbolos | Rol en este programa |
|----------|----------------------|----------------------|
| `pubsub_udp.h` | Macros anteriores | Protocolo y límites de buffers/tablas. |
| `<stdio.h>` | `printf`, `fprintf`, `snprintf`, `perror` | Salida informativa, errores, armado de `ACK`/`NEWS` en buffer acotado; `perror` tras fallos de socket. |
| `<stdlib.h>` | `strtoul`, `EXIT_SUCCESS`, `EXIT_FAILURE` | Parseo del puerto desde `argv`; códigos de salida de `main`. |
| `<string.h>` | `strlen`, `strcmp`, `strncmp`, `strchr`, `strncpy`, `memset` | Prefijos `SUB`/`PUB`, búsqueda de tema, copia acotada de nombre de tema, limpieza de estructuras. |
| `<sys/socket.h>` | `socket`, `bind`, `sendto`, `recvfrom`, `setsockopt`, `AF_INET`, `SOCK_DGRAM`, `SOL_SOCKET`, `SO_REUSEADDR`, `ssize_t`, `socklen_t` | Ciclo de vida del socket UDP del broker. |
| `<netinet/in.h>` | `struct sockaddr_in`, `htons`, `htonl`, `INADDR_ANY` | Dirección de escucha; orden de red. |
| `<arpa/inet.h>` | `inet_ntoa` | Imprimir IP del cliente en logs (buffer estático interno de la función). |
| `<unistd.h>` | `close` | Cerrar el descriptor del socket en errores o salida teórica. |

**Sockets (detalle):**

- **`socket(AF_INET, SOCK_DGRAM, 0)`** — Crea extremo UDP IPv4; devuelve fd o `-1` (`errno`).
- **`setsockopt(..., SO_REUSEADDR, ...)`** — Permite reusar el puerto tras reinicios rápidos en desarrollo.
- **`htons` / `htonl`** — Convierten puerto y dirección al orden de red antes de `bind`.
- **`bind`** — Asocia el socket a `INADDR_ANY` y al puerto elegido para recibir datagramas.
- **`recvfrom`** — Recibe un datagrama y la dirección de origen (clave para registrar suscriptores).
- **`sendto`** — Envía `ACK` al suscriptor y `NEWS` a cada dirección registrada por tema.
- **`ntohs`** — Puerto en logs en orden host.
- **`close`** — Libera el fd del socket.

---

### `publisher_udp.c`

| Cabecera | Funciones / símbolos | Rol en este programa |
|----------|----------------------|----------------------|
| `pubsub_udp.h` | Macros | Prefijo `PUB` y límites de mensaje/tema. |
| `<stdio.h>` | `snprintf`, `fprintf`, `printf`, `perror`, `fgets` | Armar datagramas, errores, demo e interactivo por stdin. |
| `<stdlib.h>` | `strtoul`, `EXIT_*` | Puerto, opción `-n`, salida de `main`. |
| `<string.h>` | `strlen`, `strcmp`, `strchr`, `memset` | Validación de tema y argv; limpiar `sockaddr_in`. |
| `<stdint.h>` | `uint16_t` | Cast del puerto para `htons`. |
| `<sys/socket.h>` | `socket`, `sendto`, `AF_INET`, `SOCK_DGRAM`, `ssize_t`, `socklen_t` | Envío UDP al broker. |
| `<netinet/in.h>` | `struct sockaddr_in`, `htons` | Dirección del broker. |
| `<arpa/inet.h>` | `inet_pton` | Cadena IPv4 (`argv`) → `sin_addr` binaria. |
| `<unistd.h>` | `close`, `usleep`, `useconds_t` | Cerrar socket; pausas entre eventos de la demo (microsegundos). |

**Sockets (detalle):**

- **`socket`** — UDP sin `connect`; cada envío va con **`sendto`** explícito al broker.
- **`memset` + `sin_family` + `htons` + `inet_pton`** — Inicialización de `sockaddr_in` del broker.
- **`sendto`** — Datagrama `PUB tema|cuerpo\n`; errores con `errno` / `perror`.
- **`close`** — Al terminar o ante error.

---

### `subscriber_udp.c`

| Cabecera | Funciones / símbolos | Rol en este programa |
|----------|----------------------|----------------------|
| `pubsub_udp.h` | Macros | `SUB`, `ACK`, `NEWS` y límites. |
| `<stdio.h>` | `snprintf`, `fprintf`, `printf`, `perror` | `SUB` en buffer, errores, impresión de noticias. |
| `<stdlib.h>` | `strtoul`, `EXIT_*` | Puerto desde `argv`. |
| `<string.h>` | `strchr`, `strlen`, `strncmp`, `memset` | Validación de tema; detectar prefijos en datagramas recibidos. |
| `<stdint.h>` | `uint16_t` | Cast para `htons`. |
| `<sys/socket.h>` | `socket`, `sendto`, `recvfrom`, `AF_INET`, `SOCK_DGRAM`, `ssize_t`, `socklen_t` | Registro y recepción de datagramas. |
| `<netinet/in.h>` | `struct sockaddr_in`, `htons`, `ntohs`, `INET_ADDRSTRLEN` | Broker y presentación del puerto de origen. |
| `<arpa/inet.h>` | `inet_pton`, `inet_ntop` | Broker en binario; IP de origen en cadena (seguro frente a `inet_ntoa` en hilos). |
| `<unistd.h>` | `close` | Cierre en errores o ruta final teórica de `main`. |

**Sockets (detalle):**

- **`sendto`** — Cada `SUB tema\n` al broker; el origen del datagrama identifica al cliente para el broker.
- **`recvfrom`** — Recibe `ACK`, `NEWS` u otros; rellena `from` con el remitente.
- **`inet_ntop`** — Muestra la IPv4 del remitente en el log.
- **`ntohs`** — Puerto de origen legible.

---

### Enlazado

`gcc` enlaza por defecto con la **libc** del sistema, que implementa las funciones anteriores. No se requieren flags `-l` adicionales para este proyecto.

---

## Protocolo de aplicación (resumen)

- Suscriptor → broker: `SUB <tema>\n`
- Publicador → broker: `PUB <tema>|<cuerpo>\n`
- Broker → suscriptor: `ACK SUB <tema>\n`, `NEWS <tema>|<cuerpo>\n`

Detalle y límites: comentarios en `pubsub_udp.h`.
