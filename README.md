# Laboratorio 3 — Sistema pub/sub deportivo (UDP)

Implementación en C del broker, publicador y suscriptor sobre **datagramas UDP** (sockets POSIX). No se utilizan bibliotecas de terceros: solo **libc** (C estándar) y cabeceras **POSIX** habituales en Linux.

## Compilación

En el directorio del proyecto:

```bash
make
```

El `Makefile` define `-D_DEFAULT_SOURCE` para **glibc** en Linux: con `-std=c11`, sin eso `usleep` y `useconds_t` no aparecen en `<unistd.h>` y la compilación del publicador falla.

Genera `broker_udp`, `publisher_udp` y `subscriber_udp`. `make clean` elimina los binarios.

## Uso rápido

- Broker: `./broker_udp [puerto]` (por defecto puerto 9000 en `pubsub_udp.h`).
- Publicador: `./publisher_udp <ip_broker> <puerto> <tema> [-n N] [-f] [-r] [-q]` — `-n` hasta 100000; **`-f`** = sin `usleep` entre envíos (ráfaga); **`-r`** ≈ mitad de pausas en 0 ms (más irregular); **`-q`** = no imprime cada envío (evita saturar SSH/TCP al medir o capturar tráfico).
- Suscriptor: `./subscriber_udp <ip_broker> <puerto> <tema1> [tema2 ...]`

**Tema con dos equipos:** use el patrón `Local_vs_Visitante` (por ejemplo `EquipoC_vs_EquipoD`). El publicador arma los textos de la demo con esos nombres. Si no hay `_vs_`, el tema completo se trata como equipo local y se usa `Rival` como visita en la narración.

El **publicador** usa **`connect(UDP)`** al broker y **`send()`** para cada `PUB`. El **suscriptor** usa **`sendto()`** para cada `SUB` y **`recvfrom()`** sin `connect`, filtrando en aplicación los datagramas cuyo origen es la IP y puerto del broker (evita comportamientos rígidos de UDP conectado en Linux con `ACK`/`NEWS`).

**Firewall (UFW):** en la VM del broker debe existir `allow 9000/udp` o el tráfico no llegará al proceso aunque `tcpdump` lo vea en la interfaz.

---

## Si “envía” pero el broker no reacciona

1. **IP del broker:** desde la VM de clientes **no** uses `127.0.0.1` si el broker está en **otra** máquina; `127.0.0.1` es siempre la propia VM. Usa la IPv4 del broker (`hostname -I` en la VM del broker, interfaz compartida con los clientes).
2. **Firewall en la VM del broker** (Ubuntu): `sudo ufw allow 9000/udp` y `sudo ufw status` (o desactivar ufw en el lab).
3. **Red virtual:** con **NAT** a veces dos VMs no reciben UDP entrante entre sí. Prefiere **adaptador solo-anfitrión** o **puente** según el manual del laboratorio.
4. **Mismo puerto** en broker, publicador y suscriptores.
5. **Suscriptor antes o temprano:** si publicas un tema sin ningún `SUB` previo, el broker descarta el reenvío (ver mensaje en consola del broker).

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
| `<stdlib.h>` | `strtoul`, `EXIT_SUCCESS`, `EXIT_FAILURE` | Puerto desde `argv`; códigos de salida de `main`. |
| `<string.h>` | `strlen`, `strcmp`, `strncmp`, `strchr`, `memset` | Prefijos `SUB`/`PUB`, búsqueda de tema, limpieza de estructuras. |
| `<sys/socket.h>` | `socket`, `bind`, `sendto`, `recvfrom`, `setsockopt`, `AF_INET`, `SOCK_DGRAM`, `SOL_SOCKET`, `SO_REUSEADDR`, `ssize_t`, `socklen_t` | Ciclo de vida del socket UDP del broker. |
| `<netinet/in.h>` | `struct sockaddr_in`, `htons`, `htonl`, `INADDR_ANY` | Dirección de escucha; orden de red. |
| `<arpa/inet.h>` | `inet_ntoa` | IP del cliente en logs de suscripción. |
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
| `<stdlib.h>` | `strtoul`, `rand`, `srand`, `EXIT_*` | Puerto; flags `-n`/`-f`/`-r`; semilla aleatoria para `-r`; salida de `main`. |
| `<time.h>` | `time` | Semilla de `srand` junto con PID para modo `-r`. |
| `<string.h>` | `strlen`, `strcmp`, `strchr`, `memcpy`, `memset` | Validación de tema; parseo `_vs_`; limpiar `sockaddr_in`. |
| `<stdint.h>` | `uint16_t` | Cast del puerto para `htons`. |
| `<sys/socket.h>` | `socket`, `connect`, `send`, `AF_INET`, `SOCK_DGRAM`, `ssize_t`, `socklen_t` | UDP “conectado” al broker y envío con `send`. |
| `<netinet/in.h>` | `struct sockaddr_in`, `htons` | Dirección del broker. |
| `<arpa/inet.h>` | `inet_pton` | Cadena IPv4 (`argv`) → `sin_addr` binaria. |
| `<unistd.h>` | `close`, `usleep`, `useconds_t` | Cerrar socket; pausas entre eventos de la demo (microsegundos). |

**Sockets (detalle):**

- **`socket`** — UDP; luego **`connect`** al broker y **`send`** para cada `PUB`.
- **`memset` + `sin_family` + `htons` + `inet_pton`** — Dirección del broker antes de `connect`.
- **`close`** — Al terminar o ante error.

---

### `subscriber_udp.c`

| Cabecera | Funciones / símbolos | Rol en este programa |
|----------|----------------------|----------------------|
| `pubsub_udp.h` | Macros | `SUB`, `ACK`, `NEWS` y límites. |
| `<stdio.h>` | `snprintf`, `fprintf`, `printf`, `perror` | `SUB` en buffer, errores, impresión de noticias. |
| `<stdlib.h>` | `strtoul`, `EXIT_*` | Puerto desde `argv`. |
| `<string.h>` | `strchr`, `strlen`, `strncmp`, `memcpy`, `strcspn`, `memset` | Validación de tema; prefijos; parseo de `NEWS`; limpieza de `sockaddr_in`. |
| `<stdint.h>` | `uint16_t` | Cast para `htons`. |
| `<sys/socket.h>` | `socket`, `sendto`, `recvfrom`, `AF_INET`, `SOCK_DGRAM`, `ssize_t`, `socklen_t` | Sin `connect`: `sendto` de cada `SUB`; `recvfrom` y filtro por origen del broker. |
| `<netinet/in.h>` | `struct sockaddr_in`, `htons`, `ntohs`, `INET_ADDRSTRLEN` | Broker y presentación del puerto de origen. |
| `<arpa/inet.h>` | `inet_pton`, `inet_ntop` | Broker en binario; IP de origen en cadena. |
| `<unistd.h>` | `close` | Cierre en errores o ruta final teórica de `main`. |

**Sockets (detalle):**

- **`sendto`** — Envía cada `SUB` al broker (`sockaddr_in` del broker).
- **`recvfrom`** — Recibe datagramas; solo se procesan si el origen coincide con IP:puerto del broker (el resto se ignora).
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
