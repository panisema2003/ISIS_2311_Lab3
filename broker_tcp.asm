; broker.asm
; Broker del sistema de noticias deportivas — x86-64 Linux, sintaxis Intel
;
; Compilar:
;   nasm -f elf64 broker.asm -o broker.o
;   ld broker.o -o broker
;
; Puertos:
;   9000 : publishers
;   9001 : subscribers
;
; En vez de hilos usamos fork() para manejar múltiples clientes:
;   - El proceso padre acepta conexiones y hace fork()
;   - El proceso hijo atiende al cliente
;   - Para el broadcast usamos un pipe compartido entre procesos
;
; SYSCALLS UTILIZADAS:
;   socket()   : syscall 41 — crea un socket TCP
;   setsockopt(): syscall 54 — configura SO_REUSEADDR
;   bind()     : syscall 49 — asocia el socket a IP y puerto
;   listen()   : syscall 50 — pone el socket en modo servidor
;   accept()   : syscall 43 — acepta una conexión entrante
;   fork()     : syscall 57 — crea un proceso hijo
;   recvfrom() : syscall 45 — recibe datos de un publisher
;   sendto()   : syscall 44 — envía datos a un subscriber
;   write()    : syscall 1  — imprime en pantalla
;   close()    : syscall 3  — cierra un socket
;   exit()     : syscall 60 — termina el proceso
;   pipe()     : syscall 22 — crea un canal de comunicación entre procesos
;   read()     : syscall 0  — lee datos del pipe
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

    ; ── Dirección para el servidor de publishers (puerto 9000) ────
    ; sockaddr_in: sin_family=2, sin_port=htons(9000)=0x2823, sin_addr=INADDR_ANY=0
    addr_pub:
        dw 2
        dw 0x2823           ; puerto 9000 en big-endian
        dd 0                ; INADDR_ANY: acepta en cualquier interfaz
        dq 0

    ; ── Dirección para el servidor de subscribers (puerto 9001) ───
    addr_sub:
        dw 2
        dw 0x2923           ; puerto 9001 en big-endian
        dd 0
        dq 0

    ; ── setsockopt: valor para SO_REUSEADDR ───────────────────────
    ; setsockopt necesita un puntero a un entero con valor 1
    opt_val dd 1

    ; ── Mensajes de estado ────────────────────────────────────────
    str_listo   db '[Broker] Listo. Esperando conexiones...', 0xA
    len_listo   equ $ - str_listo

    str_pub_ok  db '[Broker] Publisher conectado.', 0xA
    len_pub_ok  equ $ - str_pub_ok

    str_sub_ok  db '[Broker] Subscriber conectado.', 0xA
    len_sub_ok  equ $ - str_sub_ok

    str_recv    db '[Broker] Mensaje recibido y reenviado.', 0xA
    len_recv    equ $ - str_recv

section .bss
    sock_pub    resq 1      ; socket servidor para publishers
    sock_sub    resq 1      ; socket servidor para subscribers
    client_pub  resq 1      ; socket del publisher conectado
    client_sub  resq 1      ; socket del subscriber conectado
    buffer      resb 1024   ; buffer para mensajes
    bytes_recv  resq 1      ; cantidad de bytes recibidos
    ; pipe_fds[0] = extremo de lectura, pipe_fds[1] = extremo de escritura
    pipe_fds    resq 2

section .text
    global _start

; ── Macro: crear socket servidor en el puerto dado ────────────────
; Uso: no es una macro real, es una subrutina llamada con call
; Al entrar: rdi = puntero a sockaddr_in, r15 = puntero donde guardar el sockfd
; Al salir:  el socket creado, configurado, con bind y listen
create_server:
    push r15
    push rdi                ; guardar puntero a sockaddr_in

    ; socket(AF_INET=2, SOCK_STREAM=1, 0)
    mov rax, 41             ; syscall 41 = socket()
    mov rdi, 2              ; AF_INET
    mov rsi, 1              ; SOCK_STREAM = TCP
    mov rdx, 0
    syscall
    mov [r15], rax          ; guardar descriptor del socket creado

    ; setsockopt(sockfd, SOL_SOCKET=1, SO_REUSEADDR=2, &opt_val, 4)
    ; Evita "Address already in use" al reiniciar el broker rápido
    mov rax, 54             ; syscall 54 = setsockopt()
    mov rdi, [r15]          ; socket a configurar
    mov rsi, 1              ; SOL_SOCKET = nivel del socket general
    mov rdx, 2              ; SO_REUSEADDR = opción para reusar el puerto
    lea r10, [opt_val]      ; puntero al valor (1 = activar)
    mov r8, 4               ; tamaño del valor en bytes
    syscall

    ; bind(sockfd, &addr, 16)
    ; Asocia el socket a la dirección IP y puerto definidos en sockaddr_in
    pop rsi                 ; recuperar puntero a sockaddr_in
    mov rax, 49             ; syscall 49 = bind()
    mov rdi, [r15]          ; socket
    mov rdx, 16             ; tamaño de sockaddr_in
    syscall

    ; listen(sockfd, backlog=10)
    ; Pone el socket en modo servidor, acepta hasta 10 conexiones en cola
    mov rax, 50             ; syscall 50 = listen()
    mov rdi, [r15]          ; socket
    mov rsi, 10             ; backlog = 10 conexiones en cola
    syscall

    pop r15
    ret

_start:
    ; ── 1. Crear pipe para comunicación entre procesos ────────────
    ; pipe(pipe_fds): crea un canal unidireccional
    ; pipe_fds[0] = extremo de lectura (subscribers leen aquí)
    ; pipe_fds[1] = extremo de escritura (publishers escriben aquí)
    mov rax, 22             ; syscall 22 = pipe()
    lea rdi, [pipe_fds]     ; puntero al arreglo de 2 descriptores
    syscall

    ; ── 2. Crear servidor para publishers (puerto 9000) ───────────
    lea r15, [sock_pub]     ; r15 apunta a donde guardar el socket
    lea rdi, [addr_pub]     ; puntero a sockaddr_in del puerto 9000
    call create_server

    ; ── 3. Crear servidor para subscribers (puerto 9001) ──────────
    lea r15, [sock_sub]
    lea rdi, [addr_sub]
    call create_server

    ; Imprimir mensaje de inicio
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_listo]
    mov rdx, len_listo
    syscall

    ; ── 4. Loop principal: aceptar publishers ─────────────────────
    ; El broker acepta un publisher, hace fork().
    ; El hijo recibe sus mensajes y los escribe al pipe.
    ; Un proceso separado lee del pipe y reenvía a subscribers.

.loop_accept_pub:
    ; accept(sock_pub, NULL, NULL): espera una conexión de publisher
    ; BLOQUEA hasta que llegue un publisher
    mov rax, 43             ; syscall 43 = accept()
    mov rdi, [sock_pub]     ; socket servidor de publishers
    mov rsi, 0              ; addr = NULL (no nos importa la IP del cliente)
    mov rdx, 0              ; addrlen = NULL
    syscall
    mov [client_pub], rax   ; guardar socket del publisher conectado

    ; Imprimir confirmación
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_pub_ok]
    mov rdx, len_pub_ok
    syscall

    ; fork(): crear proceso hijo para atender a este publisher
    ; El hijo recibe 0 en rax, el padre recibe el PID del hijo
    mov rax, 57             ; syscall 57 = fork()
    syscall

    cmp rax, 0
    je .hijo_publisher      ; si rax=0, soy el hijo
    jmp .loop_accept_pub    ; si rax>0, soy el padre, vuelvo a aceptar

.hijo_publisher:
    ; ── Proceso hijo: recibe mensajes del publisher ───────────────

.loop_recv_pub:
    ; recvfrom(client_pub, buffer, 1024, 0, NULL, NULL)
    ; Recibe un mensaje del publisher
    mov rax, 45             ; syscall 45 = recvfrom()
    mov rdi, [client_pub]   ; socket del publisher
    lea rsi, [buffer]       ; buffer donde guardar el mensaje
    mov rdx, 1024           ; máximo bytes a leer
    mov r10, 0              ; flags = 0
    mov r8,  0
    mov r9,  0
    syscall

    cmp rax, 0
    jle .hijo_fin           ; publisher se desconectó, terminar hijo

    mov [bytes_recv], rax   ; guardar cuántos bytes recibimos

    ; Imprimir confirmación de recepción
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_recv]
    mov rdx, len_recv
    syscall

    ; write(pipe_fds[1], buffer, bytes_recv)
    ; Escribir el mensaje al pipe para que el proceso de subscribers lo lea
    mov rax, 1              ; syscall 1 = write()
    mov rdi, [pipe_fds+8]   ; extremo de escritura del pipe
    lea rsi, [buffer]       ; mensaje a escribir
    mov rdx, [bytes_recv]   ; longitud del mensaje
    syscall

    jmp .loop_recv_pub

.hijo_fin:
    ; Cerrar socket del publisher y terminar proceso hijo
    mov rax, 3
    mov rdi, [client_pub]
    syscall

    mov rax, 60
    mov rdi, 0
    syscall

    ; ── 5. Loop paralelo: aceptar subscribers ─────────────────────
    ; Este código lo ejecuta un proceso separado (fork del padre)
    ; que acepta subscribers y les reenvía lo que llega del pipe

.loop_accept_sub:
    ; accept(sock_sub, NULL, NULL): espera un subscriber
    mov rax, 43             ; syscall 43 = accept()
    mov rdi, [sock_sub]
    mov rsi, 0
    mov rdx, 0
    syscall
    mov [client_sub], rax

    mov rax, 1
    mov rdi, 1
    lea rsi, [str_sub_ok]
    mov rdx, len_sub_ok
    syscall

    ; fork() para atender a este subscriber en paralelo
    mov rax, 57
    syscall

    cmp rax, 0
    je .hijo_subscriber
    jmp .loop_accept_sub

.hijo_subscriber:
    ; ── Proceso hijo: reenvía mensajes del pipe al subscriber ─────

.loop_pipe:
    ; read(pipe_fds[0], buffer, 1024)
    ; Leer un mensaje del pipe (bloqueante hasta que llegue algo)
    mov rax, 0              ; syscall 0 = read()
    mov rdi, [pipe_fds]     ; extremo de lectura del pipe
    lea rsi, [buffer]       ; buffer donde guardar el mensaje
    mov rdx, 1024           ; máximo bytes a leer
    syscall

    cmp rax, 0
    jle .sub_fin            ; pipe cerrado, terminar

    mov [bytes_recv], rax

    ; sendto(client_sub, buffer, bytes_recv, 0, NULL, 0)
    ; Reenviar el mensaje al subscriber
    mov rax, 44             ; syscall 44 = sendto()
    mov rdi, [client_sub]   ; socket del subscriber
    lea rsi, [buffer]       ; mensaje a enviar
    mov rdx, [bytes_recv]   ; longitud
    mov r10, 0              ; flags = 0
    mov r8,  0
    mov r9,  0
    syscall

    jmp .loop_pipe

.sub_fin:
    mov rax, 3
    mov rdi, [client_sub]
    syscall

    mov rax, 60
    mov rdi, 0
    syscall
