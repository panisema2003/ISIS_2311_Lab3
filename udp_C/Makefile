# Laboratorio 3 compilacion en linux (usaremos la vm del lab2)

CC = gcc
# -D_DEFAULT_SOURCE: glibc con -std=c11 expone usleep(3) y useconds_t en <unistd.h>
CFLAGS = -Wall -Wextra -std=c11 -O2 -D_DEFAULT_SOURCE

BINARIES = broker_udp publisher_udp subscriber_udp

all: $(BINARIES)

broker_udp: broker_udp.c pubsub_udp.h
	$(CC) $(CFLAGS) -o $@ broker_udp.c

publisher_udp: publisher_udp.c pubsub_udp.h
	$(CC) $(CFLAGS) -o $@ publisher_udp.c

subscriber_udp: subscriber_udp.c pubsub_udp.h
	$(CC) $(CFLAGS) -o $@ subscriber_udp.c

clean:
	rm -f $(BINARIES)

.PHONY: all clean
