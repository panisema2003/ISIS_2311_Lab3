; broker_tcp.asm
; Broker del sistema de noticias deportivas — x86-64 Linux, sintaxis Intel
;
; Compilar:
;   nasm -f elf64 broker_tcp.asm -o broker_tcp.o
;   ld broker_tcp.o -o broker_tcp
;
; Puertos:
;   9000 : publishers
;   9001 : subscribers
;
; Lógica:
;   1. Crea dos sockets servidor (uno para publishers, uno para subscribers)
;   2. Acepta hasta MAX_SUBS subscribers y los guarda en un arreglo
;   3. Acepta un publisher
;   4. Por cada mensaje que llega del publisher, lo reenvía a todos los subscribers
;   Todo en un solo proceso usando select() a través de la syscall
;
; SYSCALLS UTILIZADAS:
;   socket()    : syscall 41 — crea un socket TCP
;   setsockopt(): syscall 54 — configura SO_REUSEADDR
;   bind()      : syscall 49 — asocia el socket a IP y puerto
;   listen()    : syscall 50 — pone el socket en modo servidor
;   accept()    : syscall 43 — acepta una conexión entrante
;   recvfrom()  : syscall 45 — recibe datos de un publisher
;   sendto()    : syscall 44 — envía datos a un subscriber
;   write()     : syscall 1  — imprime en pantalla
;   close()     : syscall 3  — cierra un socket
;   exit()      : syscall 60 — termina el proceso
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

    ; ── Dirección servidor publishers (puerto 9000) ───────────────
    ; sockaddr_in: sin_family=2, sin_port=htons(9000)=0x2823, sin_addr=0 (INADDR_ANY)
    addr_pub:
        dw 2
        dw 0x2823
        dd 0
        dq 0

    ; ── Dirección servidor subscribers (puerto 9001) ──────────────
    addr_sub:
        dw 2
        dw 0x2923
        dd 0
        dq 0

    ; setsockopt necesita un puntero a entero con valor 1
    opt_val dd 1

    str_listo   db '[Broker] Listo. Esperando subscribers...', 0xA
    len_listo   equ $ - str_listo

    str_sub_ok  db '[Broker] Subscriber conectado.', 0xA
    len_sub_ok  equ $ - str_sub_ok

    str_pub_ok  db '[Broker] Publisher conectado. Recibiendo mensajes...', 0xA
    len_pub_ok  equ $ - str_pub_ok

    str_msg_ok  db '[Broker] Mensaje reenviado a subscribers.', 0xA
    len_msg_ok  equ $ - str_msg_ok

    str_pub_off db '[Broker] Publisher desconectado.', 0xA
    len_pub_off equ $ - str_pub_off

section .bss
    sock_pub    resq 1          ; socket servidor publishers
    sock_sub    resq 1          ; socket servidor subscribers
    ; Arreglo de hasta 10 subscribers conectados
    sub_socks   resq 10
    num_subs    resq 1          ; cuántos subscribers hay conectados
    client_pub  resq 1          ; socket del publisher actual
    buffer      resb 1024       ; buffer para mensajes
    bytes_recv  resq 1

section .text
    global _start

; ── Subrutina: crear socket servidor ──────────────────────────────
; Entrada: rdi = puntero a sockaddr_in, r15 = puntero donde guardar sockfd
; Salida:  socket creado, con bind y listen
create_server:
    push rbx
    mov rbx, rdi                ; guardar puntero a sockaddr_in

    ; socket(AF_INET=2, SOCK_STREAM=1, 0)
    mov rax, 41                 ; syscall 41 = socket()
    mov rdi, 2                  ; AF_INET
    mov rsi, 1                  ; SOCK_STREAM = TCP
    mov rdx, 0
    syscall
    mov [r15], rax              ; guardar descriptor

    ; setsockopt(sockfd, SOL_SOCKET=1, SO_REUSEADDR=2, &opt_val, 4)
    ; Evita "Address already in use" al reiniciar el broker
    mov rdi, [r15]
    mov rax, 54                 ; syscall 54 = setsockopt()
    mov rsi, 1                  ; SOL_SOCKET
    mov rdx, 2                  ; SO_REUSEADDR
    lea r10, [opt_val]          ; puntero al valor 1
    mov r8, 4                   ; tamaño del valor
    syscall

    ; bind(sockfd, &addr, 16)
    ; Asocia el socket a la IP y puerto definidos en sockaddr_in
    mov rax, 49                 ; syscall 49 = bind()
    mov rdi, [r15]
    mov rsi, rbx                ; puntero a sockaddr_in
    mov rdx, 16                 ; tamaño de sockaddr_in
    syscall

    ; listen(sockfd, 10)
    ; Pone el socket en modo servidor con cola de 10 conexiones
    mov rax, 50                 ; syscall 50 = listen()
    mov rdi, [r15]
    mov rsi, 10                 ; backlog = 10
    syscall

    pop rbx
    ret

_start:
    ; Inicializar contador de subscribers en 0
    mov qword [num_subs], 0

    ; ── 1. Crear servidor para publishers (puerto 9000) ───────────
    lea r15, [sock_pub]
    lea rdi, [addr_pub]
    call create_server

    ; ── 2. Crear servidor para subscribers (puerto 9001) ──────────
    lea r15, [sock_sub]
    lea rdi, [addr_sub]
    call create_server

    ; Imprimir mensaje de inicio
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_listo]
    mov rdx, len_listo
    syscall

    ; ── 3. Aceptar subscribers antes de empezar ───────────────────
    ; Esperamos hasta 2 subscribers antes de aceptar el publisher

.loop_subs:
    cmp qword [num_subs], 2     ; ¿ya tenemos 2 subscribers?
    jge .esperar_publisher

    ; accept(sock_sub, NULL, NULL): espera un subscriber
    mov rax, 43                 ; syscall 43 = accept()
    mov rdi, [sock_sub]         ; socket servidor de subscribers
    mov rsi, 0                  ; addr = NULL
    mov rdx, 0                  ; addrlen = NULL
    syscall                     ; bloquea hasta que llegue un subscriber

    ; Guardar el socket del subscriber en el arreglo
    mov rcx, [num_subs]         ; índice actual
    lea rbx, [sub_socks]        ; dirección base del arreglo
    mov [rbx + rcx*8], rax      ; sub_socks[num_subs] = nuevo socket
    inc qword [num_subs]        ; num_subs++

    ; Imprimir confirmación
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_sub_ok]
    mov rdx, len_sub_ok
    syscall

    jmp .loop_subs

.esperar_publisher:
    ; ── 4. Aceptar un publisher ───────────────────────────────────
    ; accept(sock_pub, NULL, NULL): espera un publisher
    mov rax, 43                 ; syscall 43 = accept()
    mov rdi, [sock_pub]         ; socket servidor de publishers
    mov rsi, 0
    mov rdx, 0
    syscall
    mov [client_pub], rax       ; guardar socket del publisher

    ; Imprimir confirmación
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_pub_ok]
    mov rdx, len_pub_ok
    syscall

    ; ── 5. Recibir mensajes y reenviar a todos los subscribers ────

.loop_recv:
    ; recvfrom(client_pub, buffer, 1024, 0, NULL, NULL)
    ; Recibe un mensaje del publisher, bloquea hasta que llegue algo
    mov rax, 45                 ; syscall 45 = recvfrom()
    mov rdi, [client_pub]       ; socket del publisher
    lea rsi, [buffer]           ; buffer donde guardar el mensaje
    mov rdx, 1024               ; máximo bytes a leer
    mov r10, 0                  ; flags = 0
    mov r8,  0                  ; src_addr = NULL
    mov r9,  0                  ; addrlen = NULL
    syscall

    cmp rax, 0
    jle .publisher_off          ; publisher se desconectó

    mov [bytes_recv], rax       ; guardar cuántos bytes llegaron

    ; Reenviar el mensaje a cada subscriber
    mov rcx, 0                  ; índice del subscriber actual

.loop_broadcast:
    cmp rcx, [num_subs]         ; ¿ya enviamos a todos?
    jge .broadcast_done

    ; sendto(sub_socks[i], buffer, bytes_recv, 0, NULL, 0)
    ; Envía el mensaje al subscriber i
    lea rbx, [sub_socks]
    mov rdi, [rbx + rcx*8]     ; socket del subscriber i
    mov rax, 44                 ; syscall 44 = sendto()
    lea rsi, [buffer]           ; mensaje a enviar
    mov rdx, [bytes_recv]       ; longitud del mensaje
    mov r10, 0                  ; flags = 0
    mov r8,  0
    mov r9,  0
    syscall

    inc rcx
    jmp .loop_broadcast

.broadcast_done:
    ; Imprimir confirmación de reenvío
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_msg_ok]
    mov rdx, len_msg_ok
    syscall

    jmp .loop_recv              ; volver a esperar el siguiente mensaje

.publisher_off:
    ; El publisher se desconectó
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_pub_off]
    mov rdx, len_pub_off
    syscall

    ; Cerrar socket del publisher
    mov rax, 3                  ; syscall 3 = close()
    mov rdi, [client_pub]
    syscall

    ; Volver a esperar otro publisher
    jmp .esperar_publisher
