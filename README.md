# Laboratorio 3 — Sistema pub/sub deportivo (UDP)

Implementación del **modelo publicación–suscripción** sobre **datagramas UDP** en **C** (fuentes en `udp_C/`) y la **misma lógica** en **ensamblador x86-64 Linux** (`udp_asm/`), generado con `gcc -S` a partir de esos `.c`.

**Bibliotecas:** no se usan paquetes de terceros que encapsulen sockets. Solo **libc** (C estándar) y cabeceras **POSIX** habituales en Linux. La siguiente sección documenta **cada cabecera y símbolo relevante** con el que interactúa el programa, tal como exige el laboratorio.

---

## Estructura del repositorio

| Ruta | Contenido |
|------|-----------|
| **`udp_C/`** | `broker_udp.c`, `publisher_udp.c`, `subscriber_udp.c`, `pubsub_udp.h`, `Makefile` |
| **`udp_asm/`** | `broker_udp.s`, `publisher_udp.s`, `subscriber_udp.s`, `Makefile`, `gen_asm.sh` |

---

## Qué hace cada componente

- **Broker (`broker_udp` / `broker_udp_asm`):** escucha UDP en un puerto (por defecto 9000). Recibe `SUB <tema>` de suscriptores y registra su dirección (IP:puerto) por tema. Recibe `PUB <tema>|<cuerpo>` de publicadores y reenvía `NEWS <tema>|<cuerpo>` a todos los extremos suscritos a ese tema. Responde `ACK SUB <tema>` tras cada suscripción. **No altera** el cuerpo del mensaje del publicador.

- **Publicador (`publisher_udp` / `publisher_udp_asm`):** envía al broker líneas `PUB <tema>|<cuerpo>\n`. Incluye una demo con varios eventos deportivos; admite flags `-n`, `-f`, `-r`, `-q`, `-d` (ver más abajo).

- **Suscriptor (`subscriber_udp` / `subscriber_udp_asm`):** envía `SUB <tema>\n` por cada partido de interés y muestra en consola los `ACK` y `NEWS` que recibe del broker.

**Protocolo de aplicación (texto, terminación `\n`):**

- Suscriptor → broker: `SUB <tema>\n` (tema sin espacios).
- Publicador → broker: `PUB <tema>|<cuerpo>\n` (el primer `|` separa tema y cuerpo).
- Broker → suscriptor: `ACK SUB <tema>\n`, `NEWS <tema>|<cuerpo>\n`.

Límites y constantes: `pubsub_udp.h`.

---

## Versión en C (`udp_C/`)

### Compilar

```bash
cd udp_C
make
```

Se generan `broker_udp`, `publisher_udp` y `subscriber_udp`.

El `Makefile` usa `-std=c11 -D_DEFAULT_SOURCE`: en **glibc**, sin `_DEFAULT_SOURCE`, `usleep` y `useconds_t` pueden no declararse en `<unistd.h>` y falla la compilación del publicador.

```bash
make clean   # elimina los tres ejecutables
```

### Ejecutar (ejemplo en tres terminales)

**1 — Broker** (idealmente en la VM que actúa como servidor):

```bash
cd udp_C
./broker_udp              # puerto 9000 por defecto
# o: ./broker_udp 9000
```

**2 — Suscriptor:**

```bash
cd udp_C
./subscriber_udp <ip_broker> 9000 EquipoA_vs_EquipoB
# varios temas: ./subscriber_udp <ip> 9000 Tema1 Tema2
```

**3 — Publicador:**

```bash
cd udp_C
./publisher_udp <ip_broker> 9000 EquipoA_vs_EquipoB
```

**Flags del publicador:**

| Flag | Efecto |
|------|--------|
| `-n N` | Cantidad de mensajes demo (por defecto 12; máximo 100000). |
| `-f` | Sin pausas entre envíos (ráfaga). |
| `-r` | Aproximadamente la mitad de las pausas en 0 ms (mezcla irregular). |
| `-q` | No imprime cada envío (útil con `-n` grande vía SSH). |
| `-d` | Demo determinista: prefijo `[#i/N]` en el cuerpo; con `-r` usa `srand(1)`. |

**Temas con dos equipos:** patrón `Local_vs_Visitante` (p. ej. `EquipoC_vs_EquipoD`). Sin `_vs_`, el tema completo se usa como local y “Rival” como visita en la narración demo.

**Notas de red:** entre VMs usá la **IPv4 real del broker**, no `127.0.0.1` si el broker es otra máquina. En Ubuntu con UFW: `sudo ufw allow 9000/udp`. El publicador usa `connect(UDP)` + `send`; el suscriptor usa `sendto` + `recvfrom` y acepta datagramas cuyo **puerto de origen** es el del broker (p. ej. 9000), para no perder `ACK`/`NEWS` si la IP de origen difiere de la de `argv`.

### Si el broker “no recibe”

1. IP y puerto correctos; firewall en la VM del broker.  
2. Suscripción antes o poco después de publicar (sin `SUB`, el broker descarta el reenvío).  
3. NAT vs bridged/host-only según el manual del laboratorio.

---

## Versión en ensamblador (`udp_asm/`)

Los archivos `*.s` son **ensamblador x86-64** en sintaxis **AT&T** (GNU `as`), producidos por **GCC** con `gcc -S` a partir de **`udp_C/*.c`**. El protocolo UDP y las llamadas a la **misma libc** que en C hacen que **Wireshark/tcpdump** (`udp port 9000`, etc.) se usen igual que con los binarios compilados desde C.

### Regenerar los `.s` (tras editar `udp_C/*.c`)

Desde la **raíz del repositorio**:

```bash
chmod +x udp_asm/gen_asm.sh   # una vez
./udp_asm/gen_asm.sh
```

### Compilar ejecutables desde ensamblador

```bash
cd udp_asm
make
```

Genera `broker_udp_asm`, `publisher_udp_asm`, `subscriber_udp_asm`.

```bash
make clean
```

### Ejecutar (mismos argumentos que en C)

```bash
cd udp_asm
./broker_udp_asm
./subscriber_udp_asm <ip_broker> 9000 EquipoA_vs_EquipoB
./publisher_udp_asm <ip_broker> 9000 EquipoA_vs_EquipoB -n 12 -q -d
```

### Captura de tráfico (ejemplo)

```bash
sudo tcpdump -n -i any -s 0 -w captura_udp.pcap udp port 9000
```

**Requisitos:** Linux **x86-64** y **GCC** (misma cadena de herramientas que para `udp_C`).

**Relación con la documentación de bibliotecas:** el código ensamblado llama a los mismos símbolos externos de **libc** (`printf`, `socket`, `recvfrom`, …) que el fuente en C. Podés cruzar las tablas de la siguiente sección con `objdump -d broker_udp_asm | less` o el listado fuente en C.

---

## Documentación de cabeceras y funciones (requisito del laboratorio)

Se documenta la interacción del programa con **cada cabecera estándar** y los **símbolos usados**, sin capas externas que oculten la API de sockets.

### Política de dependencias

- **Implementación de sockets:** solo **API POSIX de Berkeley** vía `<sys/socket.h>`, `<netinet/in.h>`, `<arpa/inet.h>` y similares.
- **Enlazado:** `gcc` enlaza por defecto con **libc**; no se usan flags `-l` extra para este proyecto.
- **`pubsub_udp.h`:** cabecera **del proyecto** (macros y constantes), no una biblioteca externa.

### Resumen: cabeceras estándar usadas en el proyecto

| Cabecera | Estándar / origen | Archivos `.c` que la incluyen | Rol breve |
|----------|-------------------|-------------------------------|-----------|
| `<stddef.h>` | C estándar | vía `pubsub_udp.h` | Tipo `size_t`. |
| `<stdio.h>` | C estándar | broker, publisher, subscriber | Entrada/salida formateada, buffers de streams, lectura de líneas. |
| `<stdlib.h>` | C estándar | broker, publisher, subscriber | Conversión de cadenas a enteros, `rand`/`srand`, códigos de salida. |
| `<string.h>` | C estándar | broker, publisher, subscriber | Longitud, comparación, búsqueda de caracteres, copia de memoria, relleno. |
| `<stdint.h>` | C estándar | publisher, subscriber | Enteros de ancho fijo (`uint16_t`) para `htons` y puertos. |
| `<time.h>` | C estándar | publisher | Tiempo de calendario, `localtime_r`, `strftime`. |
| `<sys/time.h>` | POSIX | publisher | `gettimeofday`, `struct timeval` (ms en marca de tiempo). |
| `<unistd.h>` | POSIX | broker, publisher, subscriber | Descriptores: cierre, esperas `usleep`, PID, tipos asociados. |
| `<sys/socket.h>` | POSIX | broker, publisher, subscriber | Creación de socket, opciones, `bind`, `connect`, `send`/`sendto`, `recvfrom`, constantes de dominio/tipo. |
| `<netinet/in.h>` | POSIX | broker, publisher, subscriber | `struct sockaddr_in`, orden de red (`htons`, `htonl`, `INADDR_ANY`, etc.). |
| `<arpa/inet.h>` | POSIX | broker, publisher, subscriber | Conversión entre texto IPv4 y binario (`inet_pton`, `inet_ntop`, `inet_ntoa`). |

### `pubsub_udp.h` y `stddef.h`

| Símbolo / inclusión | Uso |
|---------------------|-----|
| `#include <stddef.h>` | Tipo `size_t` para tamaños. |
| `PUBSUB_UDP_DEFAULT_PORT` | Puerto por defecto del broker. |
| `PUBSUB_UDP_MAX_MSG` | Tope de carga por datagrama. |
| `PUBSUB_UDP_MAX_TOPIC_LEN` | Longitud máxima del nombre de tema. |
| `PUBSUB_UDP_MAX_SUBS_PER_TOPIC`, `PUBSUB_UDP_MAX_TOPICS` | Límites de tablas en el broker. |
| `PUBSUB_UDP_PREFIX_*` | Cadenas de protocolo: `SUB `, `PUB `, `ACK SUB `, `NEWS `. |

### `broker_udp.c`

| Cabecera | Funciones / símbolos | Rol en este programa |
|----------|----------------------|----------------------|
| `pubsub_udp.h` | Macros anteriores | Protocolo y límites. |
| `<stdio.h>` | `printf`, `fprintf`, `snprintf`, `perror`, `setvbuf`, `_IONBF` | Logs, errores, armado de `ACK`/`NEWS`; salida sin buffer para ver mensajes al instante. |
| `<stdlib.h>` | `strtoul`, `EXIT_SUCCESS`, `EXIT_FAILURE` | Puerto desde `argv`; salida de `main`. |
| `<string.h>` | `strlen`, `strcmp`, `strncmp`, `strchr`, `memset` | Prefijos, búsqueda de tema, inicialización de estructuras. |
| `<sys/socket.h>` | `socket`, `bind`, `sendto`, `recvfrom`, `setsockopt`, `AF_INET`, `SOCK_DGRAM`, `SOL_SOCKET`, `SO_REUSEADDR`, `ssize_t`, `socklen_t` | Socket UDP del broker. |
| `<netinet/in.h>` | `struct sockaddr_in`, `htons`, `htonl`, `INADDR_ANY` | Dirección de escucha. |
| `<arpa/inet.h>` | `inet_ntoa` | IP del cliente en logs. |
| `<unistd.h>` | `close` | Cerrar el descriptor del socket. |

**Sockets (detalle):** `socket` → `setsockopt(SO_REUSEADDR)` → `bind` → bucle `recvfrom` → según prefijo `SUB`/`PUB`, `sendto` de `ACK` o `NEWS`.

### `publisher_udp.c`

| Cabecera | Funciones / símbolos | Rol en este programa |
|----------|----------------------|----------------------|
| `pubsub_udp.h` | Macros | Prefijo `PUB` y límites. |
| `<stdio.h>` | `snprintf`, `fprintf`, `printf`, `perror`, `fgets` | Armar mensajes, errores, demo y líneas por stdin. |
| `<stdlib.h>` | `strtoul`, `rand`, `srand`, `EXIT_*` | Puerto, flags, semilla aleatoria (`-r` / modo no determinista). |
| `<time.h>` | `time`, `localtime_r`, `strftime`, `struct tm` | Reloj para `srand` y marca de tiempo en el cuerpo del `PUB`. |
| `<sys/time.h>` | `gettimeofday`, `struct timeval` | Fracción de segundo en la marca de tiempo. |
| `<string.h>` | `strlen`, `strcmp`, `strchr`, `memcpy`, `memset` | Validación de tema, `_vs_`, `sockaddr_in`. |
| `<stdint.h>` | `uint16_t` | Puerto en `htons`. |
| `<sys/socket.h>` | `socket`, `connect`, `send`, `AF_INET`, `SOCK_DGRAM`, `ssize_t`, `socklen_t` | UDP conectado al broker; envío con `send`. |
| `<netinet/in.h>` | `struct sockaddr_in`, `htons` | Dirección del broker. |
| `<arpa/inet.h>` | `inet_pton` | `argv` IP → binario. |
| `<unistd.h>` | `close`, `usleep`, `useconds_t`, `getpid` | Cerrar socket; pausas entre eventos; semilla de `srand` junto con `time`. |

**Marca de tiempo:** `gettimeofday` + `localtime_r` + `strftime` + `snprintf` para ms; con `-d`, prefijo fijo `[#i/n]` en lugar de reloj en la demo.

### `subscriber_udp.c`

| Cabecera | Funciones / símbolos | Rol en este programa |
|----------|----------------------|----------------------|
| `pubsub_udp.h` | Macros | `SUB`, `ACK`, `NEWS`, límites. |
| `<stdio.h>` | `snprintf`, `fprintf`, `printf`, `perror` | `SUB` en buffer, errores, impresión de noticias. |
| `<stdlib.h>` | `strtoul`, `EXIT_*` | Puerto desde `argv`. |
| `<string.h>` | `strchr`, `strlen`, `strncmp`, `memcpy`, `strcspn`, `memset` | Validación, prefijos, parseo de `NEWS`. |
| `<stdint.h>` | `uint16_t` | `htons` / presentación de puertos. |
| `<sys/socket.h>` | `socket`, `sendto`, `recvfrom`, `AF_INET`, `SOCK_DGRAM`, `ssize_t`, `socklen_t` | Sin `connect`; envío de `SUB` y recepción. |
| `<netinet/in.h>` | `struct sockaddr_in`, `htons`, `ntohs`, `INET_ADDRSTRLEN` | Broker y logs. |
| `<arpa/inet.h>` | `inet_pton`, `inet_ntop` | IP en texto ↔ binario. |
| `<unistd.h>` | `close` | Cerrar socket. |

**Recepción:** solo se procesan datagramas cuyo **puerto de origen** coincide con el del broker; el resto se descarta.

### Ensamblador (`udp_asm/*.s`)

| Componente | Descripción |
|------------|-------------|
| **GNU `as`** | Ensambla cada `.s` (sintaxis AT&T). |
| **GCC** | Invocado como conductor: `gcc archivo.s -o ejecutable` enlaza con **libc** y **ld-linux** como en un programa C. |
| **Símbolos externos** | Idénticos a los del listado anterior (`printf`, `socket`, `malloc` si el compilador lo inserta, etc.), resolviéndose en tiempo de enlace contra libc. |

No se añaden bibliotecas dinámicas o estáticas nuevas respecto a la versión en C.

### Herramientas de compilación (no son bibliotecas del programa)

| Herramienta | Uso en este proyecto |
|-------------|------------------------|
| **gcc** | Compilador C; emisión de ensamblador (`-S`) y enlace. |
| **make** | Orquesta reglas de compilación en `udp_C/Makefile` y `udp_asm/Makefile`. |
| **ld** (invocado por gcc) | Enlazado final. |

---

## Enlazado

Los ejecutables C y los generados desde `.s` enlazan con la **libc** del sistema mediante el proceso estándar de `gcc`. No se requieren `-l` adicionales para completar las referencias descritas arriba.
