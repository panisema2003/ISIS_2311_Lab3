	.file	"subscriber_udp.c"
	.text
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC0:
	.string	"Uso: %s <ip_broker> <puerto_broker> <tema1> [tema2 ...]\n  Cada <tema> es un partido (sin espacios), ej. EquipoA_vs_EquipoB\n"
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC1:
	.string	"Puerto inv\303\241lido: %s\n"
.LC2:
	.string	"socket"
	.section	.rodata.str1.8
	.align 8
.LC3:
	.string	"Direcci\303\263n IPv4 inv\303\241lida: %s\n"
	.align 8
.LC4:
	.string	"El tema no debe contener espacios: %s\n"
	.section	.rodata.str1.1
.LC5:
	.string	"Tema demasiado largo: %s\n"
.LC6:
	.string	"SUB "
.LC7:
	.string	"%s%s\n"
	.section	.rodata.str1.8
	.align 8
.LC8:
	.string	"[subscriber] SUB demasiado largo.\n"
	.section	.rodata.str1.1
.LC9:
	.string	"[subscriber] sendto SUB"
	.section	.rodata.str1.8
	.align 8
.LC10:
	.string	"[subscriber] Registrado SUB '%s' (%zd bytes enviados al broker)\n"
	.align 8
.LC11:
	.string	"[subscriber] Suscripciones enviadas. Esperando noticias (Ctrl+C)..."
	.section	.rodata.str1.1
.LC12:
	.string	"recvfrom"
.LC13:
	.string	"ACK SUB "
.LC14:
	.string	"\r\n"
	.section	.rodata.str1.8
	.align 8
.LC15:
	.string	"\n[subscriber] Suscripcion OK desde %s:%u -> partido \"%.*s\"\n"
	.section	.rodata.str1.1
.LC16:
	.string	"NEWS "
.LC17:
	.string	"\n[subscriber] [%s:%u] %s"
	.section	.rodata.str1.8
	.align 8
.LC18:
	.string	"\n---------- NOTICIA EN VIVO ----------"
	.section	.rodata.str1.1
.LC19:
	.string	"  Partido: %s\n"
.LC20:
	.string	"  Origen:  %s:%u\n"
.LC21:
	.string	"  Texto:   %s"
	.section	.rodata.str1.8
	.align 8
.LC22:
	.string	"---------------------------------------"
	.align 8
.LC23:
	.string	"\n[subscriber] [%s:%u] (datagrama no est\303\241ndar) %s"
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
	subq	$1560, %rsp
	movl	%edi, %r13d
	movq	%rsi, %rbp
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
	cmpl	$3, %r13d
	jle	.L27
	movq	8(%rbp), %r15
	movq	$0, 1544(%rsp)
	movq	16(%rbp), %rdi
	movl	$10, %edx
	leaq	1544(%rsp), %rsi
	call	strtoul
	movq	%rax, %r12
	movq	16(%rbp), %rdx
	movq	1544(%rsp), %rax
	cmpq	%rax, %rdx
	je	.L4
	cmpb	$0, (%rax)
	jne	.L4
	leaq	-1(%r12), %rax
	cmpq	$65534, %rax
	jbe	.L5
.L4:
	movl	$.LC1, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
.L25:
	movl	$1, %eax
	addq	$1560, %rsp
	popq	%rbx
	popq	%rbp
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	ret
.L27:
	movq	0(%rbp), %rdx
	movl	$.LC0, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	jmp	.L25
.L5:
	movl	$0, %edx
	movl	$2, %esi
	movl	$2, %edi
	call	socket
	movl	%eax, %ebx
	testl	%eax, %eax
	js	.L28
	movq	$0, 1524(%rsp)
	movl	$0, 1532(%rsp)
	movw	$2, 1520(%rsp)
	rolw	$8, %r12w
	movw	%r12w, 1522(%rsp)
	leaq	1524(%rsp), %rdx
	movq	%r15, %rsi
	movl	$2, %edi
	call	inet_pton
	movl	$3, %r14d
	cmpl	$1, %eax
	jne	.L29
.L7:
	movq	0(%rbp,%r14,8), %r12
	movl	$32, %esi
	movq	%r12, %rdi
	call	strchr
	testq	%rax, %rax
	jne	.L30
	movq	%r12, %rdi
	call	strlen
	cmpq	$63, %rax
	ja	.L31
	movq	%r12, %r8
	movl	$.LC6, %ecx
	movl	$.LC7, %edx
	movl	$1401, %esi
	leaq	112(%rsp), %rdi
	movl	$0, %eax
	call	snprintf
	cmpl	$1400, %eax
	ja	.L32
	movslq	%eax, %rdx
	movl	$16, %r9d
	leaq	1520(%rsp), %r8
	movl	$0, %ecx
	leaq	112(%rsp), %rsi
	movl	%ebx, %edi
	call	sendto
	movq	%rax, %rdx
	testq	%rax, %rax
	js	.L33
	movq	%r12, %rsi
	movl	$.LC10, %edi
	movl	$0, %eax
	call	printf
	movq	stdout(%rip), %rdi
	call	fflush
	addq	$1, %r14
	cmpl	%r14d, %r13d
	jg	.L7
	movl	$.LC11, %edi
	call	puts
	movq	stdout(%rip), %rdi
	call	fflush
	jmp	.L13
.L28:
	movl	$.LC2, %edi
	call	perror
	jmp	.L25
.L29:
	movq	%r15, %rdx
	movl	$.LC3, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	movl	%ebx, %edi
	call	close
	jmp	.L25
.L30:
	movq	%r12, %rdx
	movl	$.LC4, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	movl	%ebx, %edi
	call	close
	jmp	.L25
.L31:
	movq	%r12, %rdx
	movl	$.LC5, %esi
	movq	stderr(%rip), %rdi
	movl	$0, %eax
	call	fprintf
	movl	%ebx, %edi
	call	close
	jmp	.L25
.L32:
	movq	stderr(%rip), %rcx
	movl	$34, %edx
	movl	$1, %esi
	movl	$.LC8, %edi
	call	fwrite
.L11:
	movl	%ebx, %edi
	call	close
	jmp	.L25
.L33:
	movl	$.LC9, %edi
	call	perror
	jmp	.L11
.L34:
	movl	$.LC12, %edi
	call	perror
.L13:
	movl	$16, 12(%rsp)
	leaq	12(%rsp), %r9
	leaq	16(%rsp), %r8
	movl	$0, %ecx
	movl	$1400, %edx
	leaq	112(%rsp), %rsi
	movl	%ebx, %edi
	call	recvfrom
	testq	%rax, %rax
	js	.L34
	movb	$0, 112(%rsp,%rax)
	cmpw	$2, 16(%rsp)
	jne	.L13
	cmpw	$2, 1520(%rsp)
	jne	.L13
	movzwl	1522(%rsp), %eax
	cmpw	%ax, 18(%rsp)
	jne	.L13
	movl	$16, %ecx
	leaq	32(%rsp), %rdx
	leaq	20(%rsp), %rsi
	movl	$2, %edi
	call	inet_ntop
	movzwl	18(%rsp), %edx
	movl	%edx, %ebp
	rolw	$8, %bp
	movl	$8, %edx
	movl	$.LC13, %esi
	leaq	112(%rsp), %rdi
	call	strncmp
	testl	%eax, %eax
	je	.L35
	movl	$5, %edx
	movl	$.LC16, %esi
	leaq	112(%rsp), %rdi
	call	strncmp
	testl	%eax, %eax
	jne	.L18
	movl	$124, %esi
	leaq	117(%rsp), %rdi
	call	strchr
	movq	%rax, %r12
	testq	%rax, %rax
	je	.L36
	leaq	117(%rsp), %rsi
	subq	%rsi, %rax
	movl	$63, %edx
	cmpq	%rdx, %rax
	cmova	%rdx, %rax
	leaq	48(%rsp), %r13
	movl	%eax, %ecx
	movq	%r13, %rdi
	rep movsb
	movb	$0, 48(%rsp,%rax)
	leaq	1(%r12), %r14
	movl	$.LC18, %edi
	call	puts
	movq	%r13, %rsi
	movl	$.LC19, %edi
	movl	$0, %eax
	call	printf
	movzwl	%bp, %edx
	leaq	32(%rsp), %rsi
	movl	$.LC20, %edi
	movl	$0, %eax
	call	printf
	movq	%r14, %rsi
	movl	$.LC21, %edi
	movl	$0, %eax
	call	printf
	cmpb	$0, 1(%r12)
	je	.L20
	movl	$10, %esi
	movq	%r14, %rdi
	call	strchr
	testq	%rax, %rax
	je	.L37
.L20:
	movl	$.LC22, %edi
	call	puts
	movq	stdout(%rip), %rdi
	call	fflush
	jmp	.L13
.L35:
	movl	$.LC14, %esi
	leaq	120(%rsp), %rdi
	call	strcspn
	movzwl	%bp, %edx
	leaq	120(%rsp), %r8
	movl	%eax, %ecx
	leaq	32(%rsp), %rsi
	movl	$.LC15, %edi
	movl	$0, %eax
	call	printf
	movq	stdout(%rip), %rdi
	call	fflush
	jmp	.L13
.L36:
	movzwl	%bp, %edx
	leaq	112(%rsp), %rcx
	leaq	32(%rsp), %rsi
	movl	$.LC17, %edi
	movl	$0, %eax
	call	printf
	movq	stdout(%rip), %rdi
	call	fflush
	jmp	.L13
.L37:
	movl	$10, %edi
	call	putchar
	jmp	.L20
.L18:
	movzwl	%bp, %edx
	leaq	112(%rsp), %rcx
	leaq	32(%rsp), %rsi
	movl	$.LC23, %edi
	movl	$0, %eax
	call	printf
	movq	stdout(%rip), %rdi
	call	fflush
	jmp	.L13
	.size	main, .-main
	.ident	"GCC: (GNU) 12.5.0"
	.section	.note.GNU-stack,"",@progbits
