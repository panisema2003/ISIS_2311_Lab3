; subscriber.asm
; Suscriptor del sistema de noticias deportivas — x86-64 Linux, sintaxis Intel
;
; Compilar:
;   nasm -f elf64 subscriber.asm -o subscriber.o
;   ld subscriber.o -o subscriber
;
; SYSCALLS UTILIZADAS:
;   socket()   : syscall 41 — crea el socket TCP
;   connect()  : syscall 42 — conecta al broker (handshake TCP)
;   recvfrom() : syscall 45 — recibe datos del broker
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

    ; sockaddr_in del broker (16 bytes):
    ;   sin_family = AF_INET = 2
    ;   sin_port   = 9001 en big-endian = 0x2923
    ;   sin_addr   = 127.0.0.1 = 0x0100007F
    broker_addr:
        dw 2
        dw 0x2923           ; puerto 9001 en big-endian
        dd 0x0100007F       ; 127.0.0.1
        dq 0

    str_conectado db '[Subscriber] Conectado. Esperando actualizaciones...', 0xA
    len_conectado equ $ - str_conectado

    str_prefix db '[Subscriber] >> '
    len_prefix equ $ - str_prefix

    str_cerrado db '[Subscriber] Conexion cerrada.', 0xA
    len_cerrado equ $ - str_cerrado

    str_error db 'Error al conectar con el broker.', 0xA
    len_error equ $ - str_error

section .bss
    sockfd  resq 1          ; descriptor del socket
    buffer  resb 1024       ; buffer donde se guardan los mensajes recibidos

section .text
    global _start

_start:
    ; ── 1. Crear el socket TCP ────────────────────────────────────
    ; socket(AF_INET=2, SOCK_STREAM=1, protocolo=0)
    mov rax, 41             ; syscall 41 = socket()
    mov rdi, 2              ; AF_INET: IPv4
    mov rsi, 1              ; SOCK_STREAM: TCP
    mov rdx, 0              ; protocolo automático
    syscall
    mov [sockfd], rax       ; guardar descriptor del socket

    ; ── 2. Conectarse al broker ───────────────────────────────────
    ; connect(sockfd, &broker_addr, 16)
    mov rax, 42             ; syscall 42 = connect()
    mov rdi, [sockfd]       ; descriptor del socket
    lea rsi, [broker_addr]  ; puntero a sockaddr_in
    mov rdx, 16             ; tamaño de sockaddr_in
    syscall                 ; ejecuta handshake TCP (SYN, SYN-ACK, ACK)

    cmp rax, 0
    jl .error               ; si retorna negativo, hubo error

    ; Imprimir mensaje de conexión exitosa
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_conectado]
    mov rdx, len_conectado
    syscall

    ; ── 3. Recibir mensajes en un loop ────────────────────────────
    ; Nos quedamos escuchando hasta que el broker cierre la conexión

.loop_recv:
    ; recvfrom(sockfd, buffer, 1024, flags=0, NULL, NULL)
    ; recvfrom es la syscall que implementa recv() en Linux
    mov rax, 45             ; syscall 45 = recvfrom()
    mov rdi, [sockfd]       ; socket del que recibir
    lea rsi, [buffer]       ; buffer donde guardar los datos
    mov rdx, 1024           ; máximo de bytes a leer
    mov r10, 0              ; flags = 0 (comportamiento estándar)
    mov r8,  0              ; src_addr = NULL (no nos importa quién envió)
    mov r9,  0              ; addrlen = NULL
    syscall                 ; bloquea hasta que lleguen datos

    ; rax = bytes recibidos
    ; si rax <= 0, el broker cerró la conexión
    cmp rax, 0
    jle .fin

    ; Imprimir prefijo "[Subscriber] >> "
    mov rdx, rax            ; guardar longitud recibida en rdx
    push rdx                ; guardar en stack para usarlo después
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_prefix]
    mov rdx, len_prefix
    syscall

    ; Imprimir el mensaje recibido
    ; write(stdout, buffer, bytes_recibidos)
    pop rdx                 ; recuperar longitud del mensaje
    mov rax, 1
    mov rdi, 1
    lea rsi, [buffer]
    syscall

    jmp .loop_recv          ; volver a esperar el siguiente mensaje

.fin:
    ; El broker cerró la conexión
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_cerrado]
    mov rdx, len_cerrado
    syscall

    ; ── 4. Cerrar el socket ───────────────────────────────────────
    ; close(sockfd)
    mov rax, 3              ; syscall 3 = close()
    mov rdi, [sockfd]       ; descriptor a cerrar
    syscall

    ; ── 5. Terminar el programa ───────────────────────────────────
    mov rax, 60             ; syscall 60 = exit()
    mov rdi, 0              ; código 0 = éxito
    syscall

.error:
    mov rax, 1
    mov rdi, 2
    lea rsi, [str_error]
    mov rdx, len_error
    syscall

    mov rax, 60
    mov rdi, 1
    syscall
