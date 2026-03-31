# UDP en ensamblador (misma lógica que los `.c` del laboratorio)

Esta carpeta contiene **ensamblador x86-64 Linux** generado con **GCC** (`gcc -S`) a partir de `broker_udp.c`, `publisher_udp.c` y `subscriber_udp.c` en el directorio padre. El protocolo (`SUB`, `PUB`, `ACK SUB`, `NEWS`), los puertos y el uso de **sockets POSIX** son los mismos que en C, así que las pruebas con **`tcpdump`** / **Wireshark** (`udp port 9000`, etc.) aplican igual.

- **Sintaxis:** AT&T (la que usa GNU `as` por defecto).
- **Fidelidad:** es el mismo programa que compilar desde C; solo cambia la representación (ensamblador en lugar de fuente C). Si actualizás los `.c`, regenerá los `.s` con el script de abajo antes de entregar o comparar capturas.

## Regenerar los `.s` (después de editar los `.c`)

Desde la **raíz del repositorio** (donde están los `.c`):

```bash
chmod +x udp_asm/gen_asm.sh   # una vez
./udp_asm/gen_asm.sh
```

O manualmente:

```bash
gcc -S -O1 -std=c11 -D_DEFAULT_SOURCE -fno-asynchronous-unwind-tables \
  -o udp_asm/broker_udp.s broker_udp.c
# igual para publisher_udp.c y subscriber_udp.c
```

## Compilar los ejecutables a partir del ensamblador

```bash
cd udp_asm
make
```

Se generan:

- `broker_udp_asm`
- `publisher_udp_asm`
- `subscriber_udp_asm`

```bash
make clean   # borra esos tres binarios
```

## Cómo ejecutarlos (análogo a los binarios en C)

Misma secuencia que en el README principal; solo cambia el nombre del ejecutable y el directorio.

**Terminal 1 — broker (VM servidor):**

```bash
cd udp_asm
./broker_udp_asm
# o con puerto: ./broker_udp_asm 9000
```

**Terminal 2 — suscriptor:**

```bash
cd udp_asm
./subscriber_udp_asm <ip_broker> 9000 EquipoA_vs_EquipoB
```

**Terminal 3 — publicador:**

```bash
cd udp_asm
./publisher_udp_asm <ip_broker> 9000 EquipoA_vs_EquipoB -n 12 -q
# mismos flags que publisher_udp: -f, -r, -q, -d, etc.
```

**Captura en la VM del broker (igual que con el binario en C):**

```bash
sudo tcpdump -n -i any -s 0 -w captura_asm.pcap udp port 9000
```

## Nota sobre el entregable

Si el curso pide **documentar la interacción con cada función** de biblioteca: los `.s` llaman a los mismos símbolos de **libc** (`printf`, `socket`, `send`, `recvfrom`, …) que el código en C; podés citar la tabla del **README principal** del repo y añadir que la versión en ensamblador es la **emisión de GCC** de ese mismo programa, verificable con `objdump -d broker_udp_asm` o cruces con el listado.

## Requisito

**Linux x86-64** y **GCC** (misma VM del lab que usás para `make` en la raíz).
