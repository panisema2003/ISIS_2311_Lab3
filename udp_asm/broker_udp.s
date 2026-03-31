	.file	"broker_udp.c"
	.text
	.type	strip_trailing_crlf, @function
strip_trailing_crlf:
	pushq	%rbx
	movq	%rdi, %rbx
	call	strlen
	movq	%rax, %rdx
	testq	%rax, %rax
	jne	.L2
	jmp	.L1
.L4:
	movb	$0, (%rbx,%rdx)
	testq	%rdx, %rdx
	je	.L1
.L2:
	subq	$1, %rdx
	movzbl	(%rbx,%rdx), %eax
	cmpb	$10, %al
	je	.L4
	cmpb	$13, %al
	je	.L4
.L1:
	popq	%rbx
	ret
	.size	strip_trailing_crlf, .-strip_trailing_crlf
	.type	find_topic_index, @function
find_topic_index:
	pushq	%r13
	pushq	%r12
	pushq	%rbp
	pushq	%rbx
	subq	$8, %rsp
	movl	g_n_topics(%rip), %r13d
	testl	%r13d, %r13d
	jle	.L10
	movq	%rdi, %r12
	movl	$g_topics, %ebp
	movl	$0, %ebx
.L9:
	movq	%r12, %rsi
	movq	%rbp, %rdi
	call	strcmp
	testl	%eax, %eax
	je	.L7
	addl	$1, %ebx
	addq	$2116, %rbp
	cmpl	%r13d, %ebx
	jne	.L9
	movl	$-1, %ebx
.L7:
	movl	%ebx, %eax
	addq	$8, %rsp
	popq	%rbx
	popq	%rbp
	popq	%r12
	popq	%r13
	ret
.L10:
	movl	$-1, %ebx
	jmp	.L7
	.size	find_topic_index, .-find_topic_index
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC0:
	.string	"[broker] Mensaje demasiado largo para un datagrama.\n"
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC1:
	.string	"[broker] sendto"
	.text
	.type	send_datagram, @function
send_datagram:
	pushq	%r12
	pushq	%rbp
	pushq	%rbx
	movl	%edi, %ebp
	movq	%rsi, %rbx
	movq	%rdx, %r12
	movq	%rsi, %rdi
	call	strlen
	cmpq	$1400, %rax
	ja	.L18
	movq	%rax, %rdx
	movl	$16, %r9d
	movq	%r12, %r8
	movl	$0, %ecx
	movq	%rbx, %rsi
	movl	%ebp, %edi
	call	sendto
	testq	%rax, %rax
	js	.L19
	movl	$0, %eax
.L13:
	popq	%rbx
	popq	%rbp
	popq	%r12
	ret
.L18:
	movq	stderr(%rip), %rcx
	movl	$52, %edx
	movl	$1, %esi
	movl	$.LC0, %edi
	call	fwrite
	movl	$-1, %eax
	jmp	.L13
.L19:
	movl	$.LC1, %edi
	call	perror
	movl	$-1, %eax
	jmp	.L13
	.size	send_datagram, .-send_datagram
	.section	.rodata.str1.1
.LC2:
	.string	"Uso: %s [puerto]\n"
	.section	.rodata.str1.8
	.align 8
.LC3:
	.string	"  Escucha datagramas UDP y distribuye mensajes por tema.\n"
	.section	.rodata.str1.1
.LC4:
	.string	"Puerto inv\303\241lido: %s\n"
.LC5:
	.string	"socket"
.LC6:
	.string	"setsockopt SO_REUSEADDR"
.LC7:
	.string	"bind"
	.section	.rodata.str1.8
	.align 8
.LC8:
	.string	"[broker] Escuchando UDP en el puerto %u. Ctrl+C para salir.\n"
	.section	.rodata.str1.1
.LC9:
	.string	"recvfrom"
.LC10:
	.string	"SUB "
	.section	.rodata.str1.8
	.align 8
.LC11:
	.string	"[broker] SUB sin tema, ignorado.\n"
	.align 8
.LC12:
	.string	"[broker] El tema no debe contener espacios: '%s'\n"
	.align 8
.LC13:
	.string	"[broker] Tema demasiado largo (m\303\241x %d caracteres).\n"
	.section	.rodata.str1.1
.LC14:
	.string	"%s"
	.section	.rodata.str1.8
	.align 8
.LC15:
	.string	"[broker] No se pudo registrar el tema (tabla llena).\n"
	.align 8
.LC16:
	.string	"[broker] Aviso: tema '%s' alcanz\303\263 el m\303\241ximo de extremos UDP.\n"
	.section	.rodata.str1.1
.LC17:
	.string	"ACK SUB "
.LC18:
	.string	"%s%s\n"
	.section	.rodata.str1.8
	.align 8
.LC19:
	.string	"[broker] ACK demasiado largo.\n"
	.align 8
.LC20:
	.string	"[broker] Suscripci\303\263n registrada: tema='%s' desde %s:%u (extremos IP:puerto en el tema: %d)\n"
	.section	.rodata.str1.1
.LC21:
	.string	"PUB "
	.section	.rodata.str1.8
	.align 8
.LC22:
	.string	"[broker] PUB sin '|' (formato PUB tema|cuerpo).\n"
	.align 8
.LC23:
	.string	"[broker] PUB con tema vac\303\255o.\n"
	.align 8
.LC24:
	.string	"[broker] PUB para tema sin extremos registrados: '%s' (mensaje descartado para reenv\303\255o)\n"
	.section	.rodata.str1.1
.LC25:
	.string	"NEWS "
.LC26:
	.string	"%s%s|%s\n"
	.section	.rodata.str1.8
	.align 8
.LC27:
	.string	"[broker] NEWS demasiado largo para un datagrama.\n"
	.align 8
.LC28:
	.string	"[broker] PUB tema='%s' reenviado a %d extremo(s) UDP.\n"
	.align 8
.LC29:
	.string	"[broker] Datagrama no reconocido (primeros bytes): %.40s...\n"
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbp
	pushq	%rbx
	subq	$2880, %rsp
	movq	%rsi, %rbx
	cmpl	$2, %edi
	jg	.L62
	movl	$9000, %r12d
	je	.L63
.L23:
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
	movl	$0, %edx
	movl	$2, %esi
	movl	$2, %edi
	call	socket
	movl	%eax, %ebp
	testl	%eax, %eax
	js	.L64
	movl	$1, 2876(%rsp)
	movl	$4, %r8d
	leaq	2876(%rsp), %rcx
	movl	$2, %edx
	movl	$1, %esi
	movl	%eax, %edi
	call	setsockopt
	testl	%eax, %eax
	js	.L65
	movq	$0, 2852(%rsp)
	movl	$0, 2860(%rsp)
	movw	$2, 2848(%rsp)
	movzwl	%r12w, %ebx
	rolw	$8, %r12w
	movw	%r12w, 2850(%rsp)
	movl	$16, %edx
	leaq	2848(%rsp), %rsi
	movl	%ebp, %edi
	call	bind
	testl	%eax, %eax
	js	.L66
	movl	%ebx, %esi
	movl	$.LC8, %edi
	movl	$0, %eax
	call	printf
	jmp	.L29
.L62:
	movq	(%rsi), %rdx
	movl	$.LC2, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	movq	stderr(%rip), %rcx
	movl	$57, %edx
	movl	$1, %esi
	movl	$.LC3, %edi
	call	fwrite
.L22:
	movl	$1, %eax
	addq	$2880, %rsp
	popq	%rbx
	popq	%rbp
	popq	%r12
	popq	%r13
	popq	%r14
	ret
.L63:
	movq	$0, 1440(%rsp)
	movq	8(%rsi), %rdi
	movl	$10, %edx
	leaq	1440(%rsp), %rsi
	call	strtoul
	movq	%rax, %r12
	movq	8(%rbx), %rdx
	movq	1440(%rsp), %rax
	cmpq	%rax, %rdx
	je	.L24
	cmpb	$0, (%rax)
	jne	.L24
	leaq	-1(%r12), %rax
	cmpq	$65534, %rax
	jbe	.L23
.L24:
	movl	$.LC4, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	jmp	.L22
.L64:
	movl	$.LC5, %edi
	call	perror
	jmp	.L22
.L65:
	movl	$.LC6, %edi
	call	perror
	movl	%ebp, %edi
	call	close
	jmp	.L22
.L66:
	movl	$.LC7, %edi
	call	perror
	movl	%ebp, %edi
	call	close
	jmp	.L22
.L67:
	movl	$.LC9, %edi
	call	perror
.L29:
	movl	$16, 12(%rsp)
	leaq	12(%rsp), %r9
	leaq	16(%rsp), %r8
	movl	$0, %ecx
	movl	$1400, %edx
	leaq	1440(%rsp), %rsi
	movl	%ebp, %edi
	call	recvfrom
	testq	%rax, %rax
	js	.L67
	je	.L29
	movb	$0, 1440(%rsp,%rax)
	movl	$4, %edx
	movl	$.LC10, %esi
	leaq	1440(%rsp), %rdi
	call	strncmp
	testl	%eax, %eax
	je	.L68
	movl	$4, %edx
	movl	$.LC21, %esi
	leaq	1440(%rsp), %rdi
	call	strncmp
	movl	%eax, %ebx
	testl	%eax, %eax
	jne	.L47
	leaq	1444(%rsp), %rdi
	call	strip_trailing_crlf
	movl	$124, %esi
	leaq	1444(%rsp), %rdi
	call	strchr
	movq	%rax, %r12
	testq	%rax, %rax
	je	.L69
	movb	$0, (%rax)
	cmpb	$0, 1444(%rsp)
	je	.L70
	leaq	1444(%rsp), %rdi
	call	find_topic_index
	movl	%eax, %r13d
	testl	%eax, %eax
	js	.L71
	leaq	1(%r12), %r9
	leaq	1444(%rsp), %r8
	movl	$.LC25, %ecx
	movl	$.LC26, %edx
	movl	$1400, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	cmpl	$1399, %eax
	ja	.L52
	movslq	%r13d, %rax
	imulq	$2116, %rax, %rax
	movl	%ebx, %r14d
	cmpl	$0, g_topics+2112(%rax)
	jle	.L54
	movslq	%r13d, %r12
	imulq	$2116, %r12, %r12
	addq	$g_topics+64, %r12
	movslq	%r13d, %r13
	imulq	$2116, %r13, %r13
	addq	$g_topics, %r13
.L56:
	movq	%r12, %rdx
	leaq	32(%rsp), %rsi
	movl	%ebp, %edi
	call	send_datagram
	cmpl	$1, %eax
	adcl	$0, %r14d
	addl	$1, %ebx
	addq	$16, %r12
	cmpl	2112(%r13), %ebx
	jl	.L56
.L54:
	movl	%r14d, %edx
	leaq	1444(%rsp), %rsi
	movl	$.LC28, %edi
	movl	$0, %eax
	call	printf
	jmp	.L29
.L68:
	leaq	1444(%rsp), %rdi
	call	strip_trailing_crlf
	cmpb	$0, 1444(%rsp)
	je	.L72
	movl	$32, %esi
	leaq	1444(%rsp), %rdi
	call	strchr
	testq	%rax, %rax
	jne	.L73
	leaq	1444(%rsp), %rdi
	call	strlen
	cmpq	$63, %rax
	ja	.L74
	leaq	1444(%rsp), %rdi
	call	find_topic_index
	movl	%eax, %ebx
	testl	%eax, %eax
	js	.L75
.L37:
	movslq	%ebx, %rax
	imulq	$2116, %rax, %rax
	movl	g_topics+2112(%rax), %edi
	testl	%edi, %edi
	jle	.L40
	movzwl	16(%rsp), %ecx
	movl	20(%rsp), %r8d
	movzwl	18(%rsp), %r9d
	movslq	%ebx, %rdx
	imulq	$2116, %rdx, %rdx
	leaq	g_topics+64(%rdx), %rax
	movslq	%edi, %rsi
	salq	$4, %rsi
	leaq	g_topics+64(%rdx,%rsi), %rsi
	jmp	.L43
.L72:
	movq	stderr(%rip), %rcx
	movl	$33, %edx
	movl	$1, %esi
	movl	$.LC11, %edi
	call	fwrite
	jmp	.L29
.L73:
	leaq	1444(%rsp), %rdx
	movl	$.LC12, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	jmp	.L29
.L74:
	movl	$63, %edx
	movl	$.LC13, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	jmp	.L29
.L75:
	movl	g_n_topics(%rip), %eax
	cmpl	$63, %eax
	jg	.L38
	cltq
	imulq	$2116, %rax, %rax
	leaq	g_topics(%rax), %rbx
	movq	$0, g_topics(%rax)
	movq	$0, g_topics+2108(%rax)
	leaq	g_topics+8(%rax), %rdi
	andq	$-8, %rdi
	movq	%rbx, %rcx
	subq	%rdi, %rcx
	addl	$2116, %ecx
	shrl	$3, %ecx
	movl	%ecx, %ecx
	movl	$0, %eax
	rep stosq
	leaq	1444(%rsp), %rcx
	movl	$.LC14, %edx
	movl	$64, %esi
	movq	%rbx, %rdi
	call	snprintf
	movl	$0, 2112(%rbx)
	movl	g_n_topics(%rip), %ebx
	leal	1(%rbx), %eax
	movl	%eax, g_n_topics(%rip)
	testl	%ebx, %ebx
	jns	.L37
.L38:
	movq	stderr(%rip), %rcx
	movl	$53, %edx
	movl	$1, %esi
	movl	$.LC15, %edi
	call	fwrite
	jmp	.L29
.L41:
	addq	$16, %rax
	cmpq	%rsi, %rax
	je	.L40
.L43:
	cmpw	%cx, (%rax)
	jne	.L41
	cmpl	%r8d, 4(%rax)
	jne	.L41
	cmpw	%r9w, 2(%rax)
	jne	.L41
.L42:
	leaq	1444(%rsp), %r8
	movl	$.LC17, %ecx
	movl	$.LC18, %edx
	movl	$1400, %esi
	leaq	32(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	cmpl	$1399, %eax
	ja	.L76
	leaq	16(%rsp), %rdx
	leaq	32(%rsp), %rsi
	movl	%ebp, %edi
	call	send_datagram
	movslq	%ebx, %rbx
	imulq	$2116, %rbx, %rbx
	movl	g_topics+2112(%rbx), %r12d
	movzwl	18(%rsp), %ebx
	rolw	$8, %bx
	movl	20(%rsp), %edi
	call	inet_ntoa
	movq	%rax, %rdx
	movzwl	%bx, %ecx
	movl	%r12d, %r8d
	leaq	1444(%rsp), %rsi
	movl	$.LC20, %edi
	movl	$0, %eax
	call	printf
	jmp	.L29
.L40:
	cmpl	$127, %edi
	jg	.L77
	movslq	%ebx, %rdx
	imulq	$2116, %rdx, %rdx
	leal	1(%rdi), %eax
	movl	%eax, g_topics+2112(%rdx)
	movslq	%edi, %rax
	addq	$4, %rax
	salq	$4, %rax
	movdqa	16(%rsp), %xmm0
	movups	%xmm0, g_topics(%rdx,%rax)
	jmp	.L42
.L77:
	movslq	%ebx, %rdx
	imulq	$2116, %rdx, %rdx
	addq	$g_topics, %rdx
	movl	$.LC16, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	jmp	.L42
.L76:
	movq	stderr(%rip), %rcx
	movl	$30, %edx
	movl	$1, %esi
	movl	$.LC19, %edi
	call	fwrite
	jmp	.L29
.L69:
	movq	stderr(%rip), %rcx
	movl	$48, %edx
	movl	$1, %esi
	movl	$.LC22, %edi
	call	fwrite
	jmp	.L29
.L70:
	movq	stderr(%rip), %rcx
	movl	$30, %edx
	movl	$1, %esi
	movl	$.LC23, %edi
	call	fwrite
	jmp	.L29
.L71:
	leaq	1444(%rsp), %rsi
	movl	$.LC24, %edi
	movl	$0, %eax
	call	printf
	jmp	.L29
.L52:
	movq	stderr(%rip), %rcx
	movl	$49, %edx
	movl	$1, %esi
	movl	$.LC27, %edi
	call	fwrite
	jmp	.L29
.L47:
	leaq	1440(%rsp), %rdx
	movl	$.LC29, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	jmp	.L29
	.size	main, .-main
	.local	g_n_topics
	.comm	g_n_topics,4,4
	.local	g_topics
	.comm	g_topics,135424,32
	.ident	"GCC: (GNU) 12.5.0"
	.section	.note.GNU-stack,"",@progbits
