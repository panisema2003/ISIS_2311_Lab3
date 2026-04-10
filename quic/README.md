# Laboratorio 3: Implementación de Mini-QUIC sobre UDP

Este repositorio contiene la implementación del bono del Laboratorio #3 (Capa de Transporte y Sockets). Consiste en un prototipo funcional del protocolo QUIC ("Mini-QUIC") construido íntegramente en C, utilizando UDP como protocolo de transporte subyacente.

## 1. Arquitectura y Elementos Implementados

Dado que QUIC es un protocolo que opera en el espacio de usuario, se utilizó UDP (`SOCK_DGRAM`) como mecanismo base. La lógica de QUIC se programó en la capa de aplicación inyectando una cabecera personalizada de 13 bytes (`mini_quic_packet`) al inicio de cada datagrama.

Los principios de QUIC implementados son:

* **Connection IDs (Independencia de Red):**
  * A diferencia de TCP, que identifica una conexión por la tupla IP/Puerto, esta implementación utiliza un identificador numérico único (`conn_id`). Los clientes generan su propio ID al iniciar. El Broker registra a los suscriptores y filtra el tráfico basándose estrictamente en este ID, permitiendo teóricamente el cambio de red sin perder la sesión.
* **Multiplexación mediante Streams (Prevención de Head-of-Line Blocking):**
  * La conexión se divide en canales paralelos lógicos (`stream_id`). El **Stream 0** se utiliza como canal de control exclusivo para el registro de nuevos suscriptores (Handshake). El Publicador envía los eventos del partido a través del **Stream 5**.
* **Handshake de Baja Latencia (1-RTT):**
  * Se evita el saludo de tres vías de TCP. El campo `type = 1` representa un Handshake. Cuando el Broker recibe este paquete, registra al usuario inmediatamente y queda listo para enviarle datos, eliminando latencias innecesarias.
* **Numeración Estricta de Paquetes:**
  * Implementado en la variable `pkt_num`. El Publicador incrementa un contador por cada evento enviado, emulando la capacidad de QUIC para rastrear la pérdida de paquetes sobre UDP.

## 2. Librerías Utilizadas (C Nativo)

Para cumplir con los requisitos académicos, no se utilizaron dependencias externas (como Rust o librerías de terceros). Se empleó **C estándar** y las APIs de red del sistema operativo:

* `<stdio.h>`, `<stdlib.h>`, `<string.h>`: Entrada/salida estándar, manejo de memoria y manipulación de payloads.
* `<time.h>`: Generación de semillas aleatorias (`srand`) para garantizar `conn_id` únicos.
* **APIs de Red:** `winsock2.h` (para la versión de Windows) o `sys/socket.h` (para la versión Linux/POSIX), gestionando `socket()`, `bind()`, `sendto()` y `recvfrom()`.

## 3. Migración de Código: Linux (POSIX) a Windows (Winsock)

El código original en POSIX fue adaptado para compilar nativamente en Windows (`.exe`). Los cambios estructurales incluyeron:

1. **Cabeceras:** Reemplazo de `<sys/socket.h>` y `<arpa/inet.h>` por `<winsock2.h>` y `<ws2tcpip.h>`.
2. **Subsistema de Red:** Inclusión obligatoria de `WSAStartup(MAKEWORD(2, 2), &wsa)` al inicio y `WSACleanup()` al final.
3. **Tipos de Datos:** Uso de `SOCKET` en lugar de `int` para los descriptores, y `int` en lugar de `socklen_t` para los tamaños de dirección.
4. **Funciones del SO:** Reemplazo de `close()` por `closesocket()` y `sleep()` (segundos) por `Sleep()` (milisegundos).
5. **Enlazado:** Uso de la bandera `-lws2_32` en GCC para vincular las implementaciones binarias de Winsock.

## 4. Instrucciones de Compilación y Ejecución (Windows)

**Requisitos:** Compilador GCC instalado (MinGW).

### Compilación
Abre la terminal en la carpeta del proyecto y ejecuta:
```bash
gcc broker_quic.c -o broker.exe -lws2_32
gcc publisher_quic.c -o publisher.exe -lws2_32
gcc subscriber_quic.c -o subscriber.exe -lws2_32