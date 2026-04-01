# Implementación de Protocolos UDP y TCP en C y Assembly

## Integrantes / GRUPO 1

* Juan Diego Quintero Álvarez
* María Inés Velásquez
* Nicolás Salazar Sánchez

---

## Descripción del Proyecto

Este proyecto consiste en la implementación de los protocolos de comunicación **UDP (User Datagram Protocol)** y **TCP (Transmission Control Protocol)** utilizando los lenguajes **C** y **Assembly**.

El objetivo principal es analizar y comprender el funcionamiento de los protocolos de transporte mediante implementaciones prácticas que permiten observar cómo se realiza la comunicación entre procesos a través de la red.

## Resumen de estructura

* **tcp/**: Implementación del sistema Pub/Sub sobre TCP en C y Assembly, con capturas de tráfico para análisis.
* **udp/**: Implementación del sistema Pub/Sub sobre UDP en C y Assembly, junto con múltiples capturas de pruebas de rendimiento y escenarios de red.
* **ISIS_2311_Lab3.pdf**: Documento del laboratorio.

Para instrucciones de compilación y ejecución, revisar los README específicos en cada subcarpeta (`tcp/README_tcp.md`, `udp/README.md`, `udp/udp_C/README.md` y `udp/udp_asm/README.md`).

---

## Estructura Completa del Proyecto

```text
ISIS_2311_Lab3/
├── .gitignore
├── ISIS_2311_Lab3.pdf
├── README.md
├── tcp/
│   ├── README_tcp.md
│   ├── tcp_asm/
│   │   ├── broker_tcp.asm
│   │   ├── broker_tcp.o
│   │   ├── publisher_tcp.asm
│   │   ├── publisher_tcp.o
│   │   ├── subscriber_tcp.asm
│   │   └── subscriber_tcp.o
│   ├── tcp_C/
│   │   ├── broker_tcp.c
│   │   ├── publisher_tcp.c
│   │   └── subscriber_tcp.c
│   └── tcp_pcap_captures/
│       └── tcp_pubsub.pcap
└── udp/
	├── README.md
	├── udp_asm/
	│   ├── broker_udp.s
	│   ├── broker_udp_asm
	│   ├── gen_asm.sh
	│   ├── Makefile
	│   ├── publisher_udp.s
	│   ├── publisher_udp_asm
	│   ├── README.md
	│   ├── subscriber_udp.s
	│   └── subscriber_udp_asm
	├── udp_C/
	│   ├── broker_udp.c
	│   ├── Makefile
	│   ├── publisher_udp.c
	│   ├── pubsub_udp.h
	│   ├── README.md
	│   └── subscriber_udp.c
	└── udp_pcap_captures/
		├── 100k_EqA_vsEqB_normal_net_fast.pcap
		├── 10k_EqA_vsEqB_normal_net_fast.pcap
		├── 12_andMine_Each_net_fast.pcap
		├── 12_andMine_Each_slow_net_fast.pcap
		├── 12_Each_net_fast.pcap
		├── 15k_Each_net_fast_CDfirst.pcap
		├── 15k_Each_net_fast_v2.pcap
		├── 25k_Each_net_fast.pcap
		├── 25k_Each_net_fast_v2.pcap
		├── 50k_EqA_vsEqB_normal_net_fast.pcap
		├── 5k_each_EqA_vsEqB_normal_net_fast.pcap
		├── tcpdupm_udp_fast_normal_net_150k_full.pcap
		├── tcpdupm_udp_fast_normal_net_50k_each.pcap
		├── tcpdupm_udp_fast_normal_net_50k_each_v2.pcap
		└── tcpdupm_udp_fast_slow_net_150k_full.pcap
```
