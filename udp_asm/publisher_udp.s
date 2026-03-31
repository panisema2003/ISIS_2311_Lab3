	.file	"publisher_udp.c"
	.text
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"[#%d/%d] %s"
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC1:
	.string	"[publisher] Cuerpo con prefijo [#i/n] demasiado largo.\n"
	.section	.rodata.str1.1
.LC2:
	.string	"%s"
	.section	.rodata.str1.8
	.align 8
.LC3:
	.string	"[publisher] Cuerpo demasiado largo.\n"
	.section	.rodata.str1.1
.LC4:
	.string	"[%Y-%m-%d %H:%M:%S"
.LC5:
	.string	".%03ld] "
.LC6:
	.string	"%s%s"
	.section	.rodata.str1.8
	.align 8
.LC7:
	.string	"[publisher] Cuerpo con marca de tiempo demasiado largo para un datagrama.\n"
	.section	.rodata.str1.1
.LC8:
	.string	"PUB "
.LC9:
	.string	"%s%s|%s\n"
	.section	.rodata.str1.8
	.align 8
.LC10:
	.string	"[publisher] Mensaje demasiado largo.\n"
	.section	.rodata.str1.1
.LC11:
	.string	"[publisher] send"
	.section	.rodata.str1.8
	.align 8
.LC12:
	.string	"[publisher] Enviado (%zu bytes): %s"
	.text
	.type	send_pub, @function
send_pub:
	pushq	%r13
	pushq	%r12
	pushq	%rbp
	pushq	%rbx
	subq	$2952, %rsp
	movl	%edi, %r13d
	movl	%esi, %ebx
	movq	%rdx, %r12
	movq	%rcx, %rbp
	movl	2992(%rsp), %eax
	testl	%r8d, %r8d
	je	.L2
	testl	%eax, %eax
	jle	.L2
	leal	1(%r9), %ecx
	movq	%rbp, %r9
	movl	%eax, %r8d
	movl	$.LC0, %edx
	movl	$1400, %esi
	leaq	1536(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	cmpl	$1399, %eax
	ja	.L16
.L3:
	leaq	1536(%rsp), %r9
	movq	%r12, %r8
	movl	$.LC8, %ecx
	movl	$.LC9, %edx
	movl	$1401, %esi
	leaq	128(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	cmpl	$1400, %eax
	ja	.L10
	leaq	128(%rsp), %rdi
	call	strlen
	movq	%rax, %rbp
	movl	$0, %ecx
	movq	%rax, %rdx
	leaq	128(%rsp), %rsi
	movl	%r13d, %edi
	call	send
	testq	%rax, %rax
	js	.L17
	movl	$0, %eax
	testl	%ebx, %ebx
	je	.L18
.L1:
	addq	$2952, %rsp
	popq	%rbx
	popq	%rbp
	popq	%r12
	popq	%r13
	ret
.L16:
	movq	stderr(%rip), %rcx
	movl	$55, %edx
	movl	$1, %esi
	movl	$.LC1, %edi
	call	fwrite
	movl	$-1, %eax
	jmp	.L1
.L2:
	movl	$0, %esi
	movq	%rsp, %rdi
	call	gettimeofday
	testl	%eax, %eax
	jne	.L19
.L5:
	leaq	64(%rsp), %rsi
	movq	%rsp, %rdi
	call	localtime_r
	testq	%rax, %rax
	je	.L20
	leaq	64(%rsp), %rcx
	movl	$.LC4, %edx
	movl	$48, %esi
	leaq	16(%rsp), %rdi
	call	strftime
	testq	%rax, %rax
	jne	.L9
	movq	%rbp, %rcx
	movl	$.LC2, %edx
	movl	$1400, %esi
	leaq	1536(%rsp), %rdi
	call	snprintf
	cmpl	$1399, %eax
	jbe	.L3
	movq	stderr(%rip), %rcx
	movl	$36, %edx
	movl	$1, %esi
	movl	$.LC3, %edi
	call	fwrite
.L8:
	movl	$-1, %eax
	jmp	.L1
.L19:
	movl	$0, %edi
	call	time
	movq	%rax, (%rsp)
	movq	$0, 8(%rsp)
	jmp	.L5
.L20:
	movq	%rbp, %rcx
	movl	$.LC2, %edx
	movl	$1400, %esi
	leaq	1536(%rsp), %rdi
	call	snprintf
	cmpl	$1399, %eax
	jbe	.L3
	movq	stderr(%rip), %rcx
	movl	$36, %edx
	movl	$1, %esi
	movl	$.LC3, %edi
	call	fwrite
	jmp	.L8
.L9:
	leaq	16(%rsp), %rdi
	call	strlen
	movq	%rax, %rdi
	movq	8(%rsp), %rcx
	movabsq	$2361183241434822607, %rdx
	movq	%rcx, %rax
	imulq	%rdx
	sarq	$7, %rdx
	movq	%rcx, %rax
	sarq	$63, %rax
	subq	%rax, %rdx
	movq	%rdx, %rcx
	movl	$48, %esi
	subq	%rdi, %rsi
	leaq	16(%rsp), %rax
	addq	%rax, %rdi
	movl	$.LC5, %edx
	movl	$0, %eax
	call	snprintf
	movq	%rbp, %r8
	leaq	16(%rsp), %rcx
	movl	$.LC6, %edx
	movl	$1400, %esi
	leaq	1536(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	cmpl	$1399, %eax
	jbe	.L3
	movq	stderr(%rip), %rcx
	movl	$74, %edx
	movl	$1, %esi
	movl	$.LC7, %edi
	call	fwrite
	jmp	.L8
.L10:
	movq	stderr(%rip), %rcx
	movl	$37, %edx
	movl	$1, %esi
	movl	$.LC10, %edi
	call	fwrite
	movl	$-1, %eax
	jmp	.L1
.L17:
	movl	$.LC11, %edi
	call	perror
	movl	$-1, %eax
	jmp	.L1
.L18:
	leaq	128(%rsp), %rdx
	movq	%rbp, %rsi
	movl	$.LC12, %edi
	call	printf
	movq	stdout(%rip), %rdi
	call	fflush
	movl	%ebx, %eax
	jmp	.L1
	.size	send_pub, .-send_pub
	.section	.rodata.str1.8
	.align 8
.LC13:
	.ascii	"Uso: %s <ip_broker> <puerto_broker> <tema> [-n N] [-f] [-r] "
	.ascii	"[-q] [-d]\n  <tema>   partido sin espacios (ej. EquipoA_vs_E"
	.ascii	"quipoB)\n  -n N     eventos demo (defecto 12, m\303\241x 100"
	.ascii	"000)\n  -"
	.string	"f       sin pausas entre env\303\255os (r\303\241faga)\n  -r       ~50%% de pausas en 0 ms, resto seg\303\272n l\303\255nea de tiempo\n  -q       sin imprimir cada env\303\255o (recomendado con -n alto v\303\255a SSH)\n  -d       demo determinista [#i/n] y srand fijo con -r (comparar capturas)\n"
	.section	.rodata.str1.1
.LC14:
	.string	"-n"
.LC15:
	.string	"Valor inv\303\241lido para -n\n"
.LC16:
	.string	"-f"
.LC17:
	.string	"-r"
.LC18:
	.string	"-q"
.LC19:
	.string	"-d"
.LC20:
	.string	"Argumento desconocido: %s\n"
.LC21:
	.string	"Puerto inv\303\241lido: %s\n"
	.section	.rodata.str1.8
	.align 8
.LC22:
	.string	"El tema no debe contener espacios ni '|'.\n"
	.align 8
.LC23:
	.string	"Tema demasiado largo (m\303\241x %d caracteres).\n"
	.section	.rodata.str1.1
.LC24:
	.string	"socket"
	.section	.rodata.str1.8
	.align 8
.LC25:
	.string	"Direcci\303\263n IPv4 inv\303\241lida: %s\n"
	.align 8
.LC26:
	.string	"[publisher] connect UDP al broker"
	.section	.rodata.str1.1
.LC27:
	.string	"_vs_"
	.section	.rodata.str1.8
	.align 8
.LC28:
	.string	"[publisher] Broker %s:%lu, tema '%s', %d evento(s)"
	.section	.rodata.str1.1
.LC29:
	.string	" [modo -f: sin pausas]"
	.section	.rodata.str1.8
	.align 8
.LC30:
	.string	" [modo -r: ~50%% pausas en 0 ms]\n"
	.align 8
.LC31:
	.string	" (intervalos de la l\303\255nea de tiempo)."
	.align 8
.LC32:
	.string	"[publisher] Modo -d: payloads de la demo repetibles [#i/%d]; tras la demo, stdin sigue con hora real.\n"
	.align 8
.LC33:
	.string	"Inicia el partido: %s contra %s. Saque inicial para %s."
	.align 8
.LC34:
	.string	"Minuto 5: primera llegada clara, disparo desviado."
	.align 8
.LC35:
	.string	"Minuto 12: tarjeta amarilla al #8 de %s."
	.align 8
.LC36:
	.string	"Minuto 18: GOL de %s \342\200\224 asistencia del #10."
	.align 8
.LC37:
	.string	"Minuto 24: Cambio: entra #14, sale #7 (%s)."
	.align 8
.LC38:
	.string	"Minuto 31: GOL de %s de cabeza tras tiro de esquina."
	.align 8
.LC39:
	.string	"Minuto 38: Tarjeta amarilla al #10 de %s."
	.align 8
.LC40:
	.string	"Minuto 45+1: Fin del primer tiempo. Marcador 1-1 (%s - %s)."
	.align 8
.LC41:
	.string	"Minuto 58: GOL de %s al contraataque (marcador 2-1)."
	.align 8
.LC42:
	.string	"Minuto 67: Cambio doble en %s."
	.align 8
.LC43:
	.string	"Minuto 74: Tarjeta roja directa al #4 de %s."
	.align 8
.LC44:
	.string	"Minuto 81: GOL de %s de penal \342\200\224 marcador 3-1."
	.align 8
.LC45:
	.string	"Minuto 90: El \303\241rbitro a\303\261ade 4 minutos."
	.align 8
.LC46:
	.string	"Final del partido: victoria de %s 3-1 sobre %s."
	.align 8
.LC47:
	.string	"Actualizaci\303\263n en vivo (%s vs %s)."
	.align 8
.LC48:
	.string	"[publisher] %d mensaje(s) demo UDP enviado(s) (-q: sin eco por mensaje).\n"
	.align 8
.LC49:
	.string	"[publisher] Mensajes demo enviados. Escriba l\303\255neas adicionales (Enter para enviar, EOF/Ctrl+D para terminar):"
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbp
	pushq	%rbx
	subq	$1736, %rsp
	movl	%edi, %r12d
	movq	%rsi, %r13
	movl	$0, %ecx
	movl	$2, %edx
	movl	$0, %esi
	movq	stdout(%rip), %rdi
	call	setvbuf
	movl	$0, %ecx
	movl	$2, %edx
	movl	$0, %esi
	movq	stderr(%rip), %rdi
	call	setvbuf
	cmpl	$3, %r12d
	jle	.L22
	cmpl	$4, %r12d
	jle	.L98
	movl	$4, %ebx
	movl	$0, 12(%rsp)
	movl	$0, 16(%rsp)
	movl	$0, 4(%rsp)
	movl	$0, (%rsp)
	movl	$12, 8(%rsp)
	jmp	.L23
.L98:
	movl	$0, 12(%rsp)
	movl	$0, 16(%rsp)
	movl	$0, 4(%rsp)
	movl	$0, (%rsp)
	movl	$12, 8(%rsp)
	jmp	.L24
.L22:
	movq	0(%r13), %rdx
	movl	$.LC13, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	movl	$1, %r13d
	jmp	.L21
.L99:
	movq	$0, 32(%rsp)
	leaq	8(%r13,%r14), %rbx
	movq	(%rbx), %rdi
	movl	$10, %edx
	leaq	32(%rsp), %rsi
	call	strtoul
	movq	32(%rsp), %rdx
	cmpq	%rdx, (%rbx)
	je	.L28
	cmpb	$0, (%rdx)
	jne	.L28
	leaq	-1(%rax), %rdx
	cmpq	$99999, %rdx
	ja	.L28
	movl	%eax, 8(%rsp)
	movl	%r15d, %ebx
	jmp	.L30
.L28:
	movq	stderr(%rip), %rcx
	movl	$24, %edx
	movl	$1, %esi
	movl	$.LC15, %edi
	call	fwrite
	movl	$1, %r13d
	jmp	.L21
.L26:
	movl	$.LC16, %esi
	movq	%rbp, %rdi
	call	strcmp
	testl	%eax, %eax
	jne	.L82
	movl	$1, (%rsp)
.L30:
	addl	$1, %ebx
	cmpl	%ebx, %r12d
	jle	.L81
.L23:
	movslq	%ebx, %rax
	leaq	0(,%rax,8), %r14
	movq	0(%r13,%rax,8), %rbp
	movl	$.LC14, %esi
	movq	%rbp, %rdi
	call	strcmp
	testl	%eax, %eax
	jne	.L26
	leal	1(%rbx), %r15d
	cmpl	%r12d, %r15d
	jl	.L99
	movl	$.LC16, %esi
	movq	%rbp, %rdi
	call	strcmp
	testl	%eax, %eax
	je	.L100
.L82:
	movl	$.LC17, %esi
	movq	%rbp, %rdi
	call	strcmp
	testl	%eax, %eax
	je	.L87
	movl	$.LC18, %esi
	movq	%rbp, %rdi
	call	strcmp
	testl	%eax, %eax
	je	.L88
	movl	$.LC19, %esi
	movq	%rbp, %rdi
	call	strcmp
	testl	%eax, %eax
	jne	.L101
	movl	$1, 12(%rsp)
	jmp	.L30
.L101:
	movq	%rbp, %rdx
	movl	$.LC20, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	movq	0(%r13), %rdx
	movl	$.LC13, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	movl	$1, %r13d
	jmp	.L21
.L87:
	movl	$1, 4(%rsp)
	jmp	.L30
.L88:
	movl	$1, 16(%rsp)
	jmp	.L30
.L100:
	movl	$1, (%rsp)
.L81:
	cmpl	$0, 4(%rsp)
	je	.L24
	movl	12(%rsp), %ebx
	testl	%ebx, %ebx
	je	.L31
	movl	$1, %edi
	call	srand
	movl	%ebx, 4(%rsp)
.L24:
	movq	8(%r13), %rbp
	movq	$0, 1720(%rsp)
	movq	16(%r13), %rdi
	movl	$10, %edx
	leaq	1720(%rsp), %rsi
	call	strtoul
	movq	%rax, %rbx
	movq	16(%r13), %rdx
	movq	1720(%rsp), %rax
	cmpq	%rax, %rdx
	je	.L32
	cmpb	$0, (%rax)
	jne	.L32
	leaq	-1(%rbx), %rax
	cmpq	$65534, %rax
	ja	.L32
	movq	24(%r13), %r14
	movl	$32, %esi
	movq	%r14, %rdi
	call	strchr
	testq	%rax, %rax
	jne	.L34
	movl	$124, %esi
	movq	%r14, %rdi
	call	strchr
	testq	%rax, %rax
	je	.L35
.L34:
	movq	stderr(%rip), %rcx
	movl	$42, %edx
	movl	$1, %esi
	movl	$.LC22, %edi
	call	fwrite
	movl	$1, %r13d
	jmp	.L21
.L31:
	movl	$0, %edi
	call	time
	movq	%rax, %rbx
	call	getpid
	xorl	%ebx, %eax
	movl	%eax, %edi
	call	srand
	jmp	.L24
.L32:
	movl	$.LC21, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	movl	$1, %r13d
.L21:
	movl	%r13d, %eax
	addq	$1736, %rsp
	popq	%rbx
	popq	%rbp
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	ret
.L35:
	movq	%r14, %rdi
	call	strlen
	cmpq	$63, %rax
	ja	.L102
	movl	$0, %edx
	movl	$2, %esi
	movl	$2, %edi
	call	socket
	movl	%eax, %r12d
	testl	%eax, %eax
	js	.L103
	movq	$0, 1700(%rsp)
	movl	$0, 1708(%rsp)
	movw	$2, 1696(%rsp)
	movl	%ebx, %eax
	rolw	$8, %ax
	movw	%ax, 1698(%rsp)
	leaq	1700(%rsp), %rdx
	movq	%rbp, %rsi
	movl	$2, %edi
	call	inet_pton
	movl	%eax, %r13d
	cmpl	$1, %eax
	je	.L38
	movq	%rbp, %rdx
	movl	$.LC25, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	movl	%r12d, %edi
	call	close
	movl	$1, %r13d
	jmp	.L21
.L102:
	movl	$63, %edx
	movl	$.LC23, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	movl	$1, %r13d
	jmp	.L21
.L103:
	movl	$.LC24, %edi
	call	perror
	movl	$1, %r13d
	jmp	.L21
.L38:
	movl	$16, %edx
	leaq	1696(%rsp), %rsi
	movl	%r12d, %edi
	call	connect
	testl	%eax, %eax
	js	.L104
	movl	$.LC27, %esi
	movq	%r14, %rdi
	call	strstr
	testq	%rax, %rax
	je	.L93
	cmpq	%rax, %r14
	je	.L93
	movq	%rax, %rdx
	subq	%r14, %rdx
	movl	$63, %ecx
	cmpq	%rcx, %rdx
	cmova	%rcx, %rdx
	leaq	1632(%rsp), %r8
	movl	%edx, %ecx
	movq	%r8, %rdi
	movq	%r14, %rsi
	rep movsb
	movb	$0, 1632(%rsp,%rdx)
	leaq	4(%rax), %rcx
	movl	$.LC2, %edx
	movl	$64, %esi
	leaq	1568(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	cmpb	$0, 1568(%rsp)
	je	.L105
.L42:
	movl	8(%rsp), %r8d
	movq	%r14, %rcx
	movq	%rbx, %rdx
	movq	%rbp, %rsi
	movl	$.LC28, %edi
	movl	$0, %eax
	call	printf
	cmpl	$0, (%rsp)
	jne	.L106
	cmpl	$0, 4(%rsp)
	je	.L45
	movl	$.LC30, %edi
	movl	$0, %eax
	call	printf
.L44:
	cmpl	$0, 12(%rsp)
	jne	.L107
.L46:
	movl	$0, %r15d
	movl	$15, %ebp
	movl	%r13d, 28(%rsp)
	movl	8(%rsp), %r13d
	jmp	.L70
.L104:
	movl	$.LC26, %edi
	call	perror
	movl	%r12d, %edi
	call	close
	jmp	.L21
.L93:
	movq	%r14, %rcx
	movl	$.LC2, %edx
	movl	$64, %esi
	leaq	1632(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	movl	$1635150162, 1568(%rsp)
	movw	$108, 1572(%rsp)
	jmp	.L42
.L105:
	movl	$1635150162, 1568(%rsp)
	movw	$108, 1572(%rsp)
	jmp	.L42
.L106:
	movl	$.LC29, %edi
	call	puts
	jmp	.L44
.L45:
	movl	$.LC31, %edi
	call	puts
	jmp	.L44
.L107:
	movl	8(%rsp), %esi
	movl	$.LC32, %edi
	movl	$0, %eax
	call	printf
	jmp	.L46
.L90:
	movl	$0, 24(%rsp)
.L47:
	cmpl	$0, (%rsp)
	je	.L108
.L49:
	cmpq	$14, %rbx
	ja	.L51
	jmp	*.L53(,%rbx,8)
	.section	.rodata
	.align 8
	.align 4
.L53:
	.quad	.L67
	.quad	.L66
	.quad	.L65
	.quad	.L64
	.quad	.L63
	.quad	.L62
	.quad	.L61
	.quad	.L60
	.quad	.L59
	.quad	.L58
	.quad	.L57
	.quad	.L56
	.quad	.L55
	.quad	.L54
	.quad	.L52
	.text
.L108:
	cmpl	$0, 4(%rsp)
	jne	.L83
.L50:
	cmpl	$0, 24(%rsp)
	je	.L49
	jmp	.L84
.L92:
	movl	$2500, 24(%rsp)
.L83:
	call	rand
	testb	$1, %al
	jne	.L49
	jmp	.L50
.L67:
	leaq	1632(%rsp), %r9
	leaq	1568(%rsp), %r8
	movq	%r9, %rcx
	movl	$.LC33, %edx
	movl	$512, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
.L68:
	subq	$8, %rsp
	pushq	%r13
	movl	36(%rsp), %r9d
	movl	28(%rsp), %r8d
	leaq	48(%rsp), %rcx
	movq	%r14, %rdx
	movl	32(%rsp), %esi
	movl	%r12d, %edi
	call	send_pub
	addq	$1, %r15
	addq	$16, %rsp
	testl	%eax, %eax
	jne	.L109
.L70:
	movl	%r15d, 20(%rsp)
	cmpl	%r15d, %r13d
	jle	.L110
	movq	%r15, %rax
	movl	$0, %edx
	divq	%rbp
	movq	%rdx, %rbx
	testl	%r15d, %r15d
	je	.L90
	testq	%rdx, %rdx
	je	.L48
	movl	match_timeline(,%rdx,4), %eax
	movl	%eax, 24(%rsp)
	jmp	.L47
.L66:
	leaq	32(%rsp), %rax
	movl	$.LC34, %esi
	movl	$51, %ecx
	movq	%rax, %rdi
	rep movsb
	jmp	.L68
.L65:
	leaq	1568(%rsp), %rcx
	movl	$.LC35, %edx
	movl	$512, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	jmp	.L68
.L64:
	leaq	1632(%rsp), %rcx
	movl	$.LC36, %edx
	movl	$512, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	jmp	.L68
.L63:
	leaq	1568(%rsp), %rcx
	movl	$.LC37, %edx
	movl	$512, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	jmp	.L68
.L62:
	leaq	1568(%rsp), %rcx
	movl	$.LC38, %edx
	movl	$512, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	jmp	.L68
.L61:
	leaq	1632(%rsp), %rcx
	movl	$.LC39, %edx
	movl	$512, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	jmp	.L68
.L60:
	leaq	1568(%rsp), %r8
	leaq	1632(%rsp), %rcx
	movl	$.LC40, %edx
	movl	$512, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	jmp	.L68
.L59:
	movabsq	$7286931302352776777, %rax
	movabsq	$7236850738175090796, %rdx
	movq	%rax, 32(%rsp)
	movq	%rdx, 40(%rsp)
	movabsq	$2337197157207467379, %rax
	movabsq	$13070377591073140, %rdx
	movq	%rax, 42(%rsp)
	movq	%rdx, 50(%rsp)
	jmp	.L68
.L58:
	leaq	1632(%rsp), %rcx
	movl	$.LC41, %edx
	movl	$512, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	jmp	.L68
.L57:
	leaq	1568(%rsp), %rcx
	movl	$.LC42, %edx
	movl	$512, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	jmp	.L68
.L56:
	leaq	1568(%rsp), %rcx
	movl	$.LC43, %edx
	movl	$512, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	jmp	.L68
.L55:
	leaq	1632(%rsp), %rcx
	movl	$.LC44, %edx
	movl	$512, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	jmp	.L68
.L54:
	leaq	32(%rsp), %rax
	movl	$.LC45, %esi
	movl	$41, %ecx
	movq	%rax, %rdi
	rep movsb
	jmp	.L68
.L52:
	leaq	1568(%rsp), %r8
	leaq	1632(%rsp), %rcx
	movl	$.LC46, %edx
	movl	$512, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	jmp	.L68
.L51:
	leaq	1568(%rsp), %r8
	leaq	1632(%rsp), %rcx
	movl	$.LC47, %edx
	movl	$512, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	jmp	.L68
.L109:
	movl	28(%rsp), %r13d
	movl	%r12d, %edi
	call	close
	jmp	.L21
.L110:
	cmpl	$0, 16(%rsp)
	jne	.L111
.L72:
	movl	$.LC49, %edi
	call	puts
	leaq	544(%rsp), %rbx
	jmp	.L77
.L111:
	movl	8(%rsp), %esi
	movl	$.LC48, %edi
	movl	$0, %eax
	call	printf
	jmp	.L72
.L76:
	movb	$0, (%rax)
.L74:
	cmpq	%rbx, %rax
	je	.L75
	movzbl	-1(%rax), %edx
	subq	$1, %rax
	cmpb	$10, %dl
	je	.L76
	cmpb	$13, %dl
	je	.L76
.L75:
	cmpb	$0, 544(%rsp)
	je	.L77
	subq	$8, %rsp
	pushq	$0
	movl	$0, %r9d
	movl	28(%rsp), %r8d
	movq	%rbx, %rcx
	movq	%r14, %rdx
	movl	32(%rsp), %esi
	movl	%r12d, %edi
	call	send_pub
	addq	$16, %rsp
	testl	%eax, %eax
	jne	.L78
.L77:
	movq	stdin(%rip), %rdx
	movl	$1024, %esi
	movq	%rbx, %rdi
	call	fgets
	testq	%rax, %rax
	je	.L78
	movq	%rbx, %rdi
	call	strlen
	addq	%rbx, %rax
	jmp	.L74
.L78:
	movl	%r12d, %edi
	call	close
	movl	$0, %r13d
	jmp	.L21
.L48:
	cmpl	$0, (%rsp)
	jne	.L67
	cmpl	$0, 4(%rsp)
	jne	.L92
	movl	$2500, 24(%rsp)
.L84:
	imull	$1000, 24(%rsp), %edi
	call	usleep
	jmp	.L49
	.size	main, .-main
	.section	.rodata
	.align 32
	.type	match_timeline, @object
	.size	match_timeline, 60
match_timeline:
	.long	0
	.long	1150
	.long	3100
	.long	850
	.long	1950
	.long	1050
	.long	920
	.long	1550
	.long	2800
	.long	480
	.long	2250
	.long	780
	.long	1680
	.long	1320
	.long	1100
	.ident	"GCC: (GNU) 12.5.0"
	.section	.note.GNU-stack,"",@progbits
