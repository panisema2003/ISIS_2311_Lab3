; publisher.asm
; Publicador del sistema de noticias deportivas — x86-64 Linux, sintaxis Intel
;
; Compilar:
;   nasm -f elf64 publisher.asm -o publisher.o
;   ld publisher.o -o publisher
;
; SYSCALLS UTILIZADAS:
;   socket()   : syscall 41 — crea el socket TCP
;   connect()  : syscall 42 — conecta al broker (handshake TCP)
;   sendto()   : syscall 44 — envía datos por el socket
;   nanosleep(): syscall 35 — pausa entre mensajes
;   write()    : syscall 1  — imprime en pantalla
;   close()    : syscall 3  — cierra el socket
;   exit()     : syscall 60 — termina el programa
;
; REGISTROS x86-64:
;   rax : número de syscall / valor de retorno
;   rdi : argumento 1
;   rsi : argumento 2
;   rdx : argumento 3
;   r10 : argumento 4
;   r8  : argumento 5
;   r9  : argumento 6

section .data

    ; ── Dirección del broker ──────────────────────────────────────
    ; sockaddr_in tiene este formato en memoria (16 bytes total):
    ;   2 bytes: sin_family (AF_INET = 2)
    ;   2 bytes: sin_port   (9000 en big-endian = 0x2328)
    ;   4 bytes: sin_addr   (127.0.0.1 = 0x0100007F en little-endian)
    ;   8 bytes: padding (ceros)
    broker_addr:
        dw 2                ; AF_INET
        dw 0x2823           ; puerto 9000 en big-endian (htons(9000))
        dd 0x0100007F       ; 127.0.0.1 en formato de red
        dq 0                ; padding

    ; ── Los 10 mensajes del partido ───────────────────────────────
    msg0  db '[Colombia vs Brasil] Inicio del partido', 0xA
    len0  equ $ - msg0
    msg1  db '[Colombia vs Brasil] Tiro de esquina para el equipo local', 0xA
    len1  equ $ - msg1
    msg2  db '[Colombia vs Brasil] GOOOL del equipo local al minuto 12', 0xA
    len2  equ $ - msg2
    msg3  db '[Colombia vs Brasil] Tarjeta amarilla al numero 8 del visitante', 0xA
    len3  equ $ - msg3
    msg4  db '[Colombia vs Brasil] Fin del primer tiempo. Marcador: 1-0', 0xA
    len4  equ $ - msg4
    msg5  db '[Colombia vs Brasil] Inicio del segundo tiempo', 0xA
    len5  equ $ - msg5
    msg6  db '[Colombia vs Brasil] Cambio: jugador 11 entra por jugador 7', 0xA
    len6  equ $ - msg6
    msg7  db '[Colombia vs Brasil] GOOOL del visitante al minuto 67. Empate 1-1', 0xA
    len7  equ $ - msg7
    msg8  db '[Colombia vs Brasil] Tarjeta roja al numero 4 del local al minuto 88', 0xA
    len8  equ $ - msg8
    msg9  db '[Colombia vs Brasil] Fin del partido. Resultado final: 1-1', 0xA
    len9  equ $ - msg9

    ; ── Mensajes de estado ────────────────────────────────────────
    str_conectado db '[Publisher] Conectado al broker.', 0xA
    len_conectado equ $ - str_conectado

    str_listo db '[Publisher] Transmision finalizada.', 0xA
    len_listo equ $ - str_listo

    str_error db 'Error al conectar con el broker.', 0xA
    len_error equ $ - str_error

    ; ── Tabla de mensajes: punteros y longitudes ──────────────────
    ; Usamos una tabla para recorrer los mensajes en un loop
    mensajes   dq msg0, msg1, msg2, msg3, msg4, msg5, msg6, msg7, msg8, msg9
    longitudes dq len0, len1, len2, len3, len4, len5, len6, len7, len8, len9

    ; ── Estructura para nanosleep (1 segundo) ─────────────────────
    ; nanosleep espera una estructura timespec: {segundos, nanosegundos}
    sleep_time:
        dq 1    ; 1 segundo
        dq 0    ; 0 nanosegundos

section .bss
    sockfd resq 1   ; reserva 8 bytes para guardar el descriptor del socket

section .text
    global _start

_start:
    ; ── 1. Crear el socket TCP ────────────────────────────────────
    ; socket(AF_INET=2, SOCK_STREAM=1, protocolo=0)
    mov rax, 41         ; syscall 41 = socket()
    mov rdi, 2          ; AF_INET: usar IPv4
    mov rsi, 1          ; SOCK_STREAM: TCP (orientado a conexión)
    mov rdx, 0          ; protocolo 0: el sistema lo elige automáticamente
    syscall             ; resultado queda en rax
    mov [sockfd], rax   ; guardar el descriptor del socket

    ; ── 2. Conectarse al broker ───────────────────────────────────
    ; connect(sockfd, &broker_addr, sizeof(sockaddr_in)=16)
    mov rax, 42             ; syscall 42 = connect()
    mov rdi, [sockfd]       ; descriptor del socket
    lea rsi, [broker_addr]  ; puntero a la estructura sockaddr_in
    mov rdx, 16             ; tamaño de sockaddr_in = 16 bytes
    syscall                 ; ejecuta el handshake TCP (SYN, SYN-ACK, ACK)

    cmp rax, 0
    jl .error               ; si retorna negativo, hubo error

    ; Imprimir mensaje de conexión exitosa
    ; write(stdout=1, mensaje, longitud)
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_conectado]
    mov rdx, len_conectado
    syscall

    ; ── 3. Enviar los 10 mensajes en un loop ──────────────────────
    mov rcx, 0          ; rcx = índice del mensaje actual

.loop_mensajes:
    cmp rcx, 10         ; ¿ya enviamos los 10 mensajes?
    jge .fin            ; si sí, saltar al final

    ; Obtener puntero al mensaje[i] de la tabla
    lea rbx, [mensajes]
    mov rsi, [rbx + rcx*8]

    ; Obtener longitud del mensaje[i]
    lea rbx, [longitudes]
    mov rdx, [rbx + rcx*8]

    ; sendto(sockfd, mensaje, longitud, flags=0, NULL, 0)
    ; En Linux, send() se implementa como sendto() con los últimos dos args en 0
    mov rax, 44             ; syscall 44 = sendto()
    mov rdi, [sockfd]       ; socket por donde enviar
    mov r10, 0              ; flags = 0 (comportamiento estándar)
    mov r8,  0              ; dest_addr = NULL (no aplica en TCP)
    mov r9,  0              ; addrlen = 0
    syscall

    ; Imprimir el mensaje en pantalla también
    lea rbx, [mensajes]
    mov rsi, [rbx + rcx*8]
    lea rbx, [longitudes]
    mov rdx, [rbx + rcx*8]
    mov rax, 1
    mov rdi, 1
    syscall

    ; Esperar 1 segundo antes del siguiente mensaje
    ; nanosleep(&sleep_time, NULL)
    mov rax, 35             ; syscall 35 = nanosleep()
    lea rdi, [sleep_time]   ; puntero a {segundos=1, nanosegundos=0}
    mov rsi, 0              ; rem = NULL (no nos importa el tiempo restante)
    syscall

    inc rcx                 ; siguiente mensaje
    jmp .loop_mensajes

.fin:
    ; Imprimir mensaje de fin
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_listo]
    mov rdx, len_listo
    syscall

    ; ── 4. Cerrar el socket ───────────────────────────────────────
    ; close(sockfd): cierra la conexión y envía FIN al broker
    mov rax, 3          ; syscall 3 = close()
    mov rdi, [sockfd]   ; descriptor del socket a cerrar
    syscall

    ; ── 5. Terminar el programa ───────────────────────────────────
    ; exit(0): termina el proceso con código 0 = éxito
    mov rax, 60         ; syscall 60 = exit()
    mov rdi, 0
    syscall

.error:
    mov rax, 1
    mov rdi, 2              ; stderr = 2
    lea rsi, [str_error]
    mov rdx, len_error
    syscall

    mov rax, 60
    mov rdi, 1              ; código de salida 1 = error
    syscall
