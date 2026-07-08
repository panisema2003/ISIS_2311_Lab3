; publisher_tcp.asm
; Publicador del sistema de noticias deportivas — x86-64 Linux, sintaxis Intel
;
; Compilar:
;   nasm -f elf64 publisher_tcp.asm -o publisher_tcp.o
;   ld publisher_tcp.o -o publisher_tcp
;
; SYSCALLS UTILIZADAS:
;   socket()   : syscall 41 — crea el socket TCP
;   connect()  : syscall 42 — conecta al broker (handshake TCP)
;   sendto()   : syscall 44 — envía datos por el socket
;   nanosleep(): syscall 35 — pausa de 1 segundo entre mensajes
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
    ; sockaddr_in (16 bytes):
    ;   sin_family = AF_INET = 2
    ;   sin_port   = htons(9000) = 0x2823
    ;   sin_addr   = 127.0.0.1  = 0x0100007F
    broker_addr:
        dw 2
        dw 0x2823
        dd 0x0100007F
        dq 0

    ; ── Estructura para nanosleep (1 segundo) ─────────────────────
    ; nanosleep recibe un puntero a {tv_sec, tv_nsec}
    sleep_time:
        dq 1        ; 1 segundo
        dq 0        ; 0 nanosegundos

    ; ── Los 10 mensajes del partido ───────────────────────────────
    msg0  db '[Colombia vs Brasil] Inicio del partido', 0xA, 0
    msg1  db '[Colombia vs Brasil] Tiro de esquina para el equipo local', 0xA, 0
    msg2  db '[Colombia vs Brasil] GOOOL del equipo local al minuto 12', 0xA, 0
    msg3  db '[Colombia vs Brasil] Tarjeta amarilla al numero 8 del visitante', 0xA, 0
    msg4  db '[Colombia vs Brasil] Fin del primer tiempo. Marcador: 1-0', 0xA, 0
    msg5  db '[Colombia vs Brasil] Inicio del segundo tiempo', 0xA, 0
    msg6  db '[Colombia vs Brasil] Cambio: jugador 11 entra por jugador 7', 0xA, 0
    msg7  db '[Colombia vs Brasil] GOOOL del visitante al minuto 67. Empate 1-1', 0xA, 0
    msg8  db '[Colombia vs Brasil] Tarjeta roja al numero 4 del local al minuto 88', 0xA, 0
    msg9  db '[Colombia vs Brasil] Fin del partido. Resultado final: 1-1', 0xA, 0

    str_conectado db '[Publisher] Conectado al broker.', 0xA
    len_conectado equ $ - str_conectado

    str_listo db '[Publisher] Transmision finalizada.', 0xA
    len_listo equ $ - str_listo

    str_error db 'Error al conectar con el broker.', 0xA
    len_error equ $ - str_error

section .bss
    sockfd resq 1

section .text
    global _start

; ── Subrutina: calcular longitud de string terminado en 0 ─────────
; Entrada: rsi = puntero al string
; Salida:  rdx = longitud (sin contar el 0 final)
strlen_sub:
    push rdi
    push rcx
    mov rdi, rsi
    xor rcx, rcx
.strlen_loop:
    cmp byte [rdi + rcx], 0     ; ¿llegamos al byte 0 final?
    je .strlen_done
    inc rcx
    jmp .strlen_loop
.strlen_done:
    mov rdx, rcx
    pop rcx
    pop rdi
    ret

; ── Subrutina: enviar mensaje y esperar 1 segundo ─────────────────
; Entrada: rsi = puntero al mensaje, [sockfd] = socket del broker
send_msg:
    push rsi

    ; Calcular longitud del mensaje
    call strlen_sub             ; rdx = longitud

    ; sendto(sockfd, mensaje, longitud, 0, NULL, 0)
    ; Envía los datos por TCP al broker
    mov rax, 44                 ; syscall 44 = sendto()
    mov rdi, [sockfd]           ; socket del broker
    mov r10, 0                  ; flags = 0
    mov r8,  0                  ; dest_addr = NULL (no aplica en TCP)
    mov r9,  0                  ; addrlen = 0
    syscall

    ; Imprimir el mensaje en pantalla también
    ; write(stdout=1, mensaje, longitud)
    pop rsi
    call strlen_sub
    mov rax, 1                  ; syscall 1 = write()
    mov rdi, 1                  ; stdout
    syscall

    ; nanosleep(&sleep_time, NULL): pausa 1 segundo entre eventos
    mov rax, 35                 ; syscall 35 = nanosleep()
    lea rdi, [sleep_time]       ; puntero a {segundos=1, nanosegundos=0}
    mov rsi, 0                  ; rem = NULL
    syscall

    ret

_start:
    ; ── 1. Crear el socket TCP ────────────────────────────────────
    ; socket(AF_INET=2, SOCK_STREAM=1, protocolo=0)
    mov rax, 41                 ; syscall 41 = socket()
    mov rdi, 2                  ; AF_INET: IPv4
    mov rsi, 1                  ; SOCK_STREAM: TCP
    mov rdx, 0                  ; protocolo automático
    syscall
    mov [sockfd], rax           ; guardar descriptor del socket

    ; ── 2. Conectarse al broker ───────────────────────────────────
    ; connect(sockfd, &broker_addr, 16)
    ; Ejecuta el handshake TCP: SYN → SYN-ACK → ACK
    mov rax, 42                 ; syscall 42 = connect()
    mov rdi, [sockfd]           ; descriptor del socket
    lea rsi, [broker_addr]      ; puntero a sockaddr_in
    mov rdx, 16                 ; tamaño de sockaddr_in = 16 bytes
    syscall

    cmp rax, 0
    jl .error                   ; retorno negativo = error de conexión

    ; Imprimir mensaje de conexión exitosa
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_conectado]
    mov rdx, len_conectado
    syscall

    ; ── 3. Enviar los 10 mensajes del partido ─────────────────────
    ; Llamamos send_msg con cada mensaje usando su dirección directa
    lea rsi, [msg0]
    call send_msg

    lea rsi, [msg1]
    call send_msg

    lea rsi, [msg2]
    call send_msg

    lea rsi, [msg3]
    call send_msg

    lea rsi, [msg4]
    call send_msg

    lea rsi, [msg5]
    call send_msg

    lea rsi, [msg6]
    call send_msg

    lea rsi, [msg7]
    call send_msg

    lea rsi, [msg8]
    call send_msg

    lea rsi, [msg9]
    call send_msg

    ; Imprimir mensaje de fin
    mov rax, 1
    mov rdi, 1
    lea rsi, [str_listo]
    mov rdx, len_listo
    syscall

    ; ── 4. Cerrar el socket ───────────────────────────────────────
    ; close(sockfd): envía FIN al broker para cerrar la conexión TCP
    mov rax, 3                  ; syscall 3 = close()
    mov rdi, [sockfd]
    syscall

    ; ── 5. Terminar el programa ───────────────────────────────────
    ; exit(0): termina el proceso con código 0 = éxito
    mov rax, 60                 ; syscall 60 = exit()
    mov rdi, 0
    syscall

.error:
    mov rax, 1
    mov rdi, 2                  ; stderr = 2
    lea rsi, [str_error]
    mov rdx, len_error
    syscall

    mov rax, 60
    mov rdi, 1                  ; código 1 = error
    syscall
