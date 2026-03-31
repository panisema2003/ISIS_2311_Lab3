# Binarios a partir de ensamblador generado por GCC (-S) desde los .c de la raíz del repo.
# Arquitectura: x86-64 Linux, sintaxis AT&T (ensamblador de GNU as).

CC       = gcc
CFLAGS   = -O1 -std=c11 -D_DEFAULT_SOURCE
# El .s de gcc -S usa relocaciones absolutas; el enlazador por defecto (-pie) falla en Ubuntu reciente.
LDFLAGS  = -no-pie

BINARIES = broker_udp_asm publisher_udp_asm subscriber_udp_asm

.PHONY: all clean

all: $(BINARIES)

broker_udp_asm: broker_udp.s
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ broker_udp.s

publisher_udp_asm: publisher_udp.s
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ publisher_udp.s

subscriber_udp_asm: subscriber_udp.s
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ subscriber_udp.s

clean:
	rm -f $(BINARIES)
