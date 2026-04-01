/**
 * pubsub_udp.h — Definiciones compartidas del sistema de noticias deportivas (UDP).
 *
 *
 * Formato de mensajes (texto, terminación recomendada con \\n):
 *
 *   Suscriptor -> Broker:  SUB <tema>\\n
 *      <tema> identifica el partido (ej. EquipoA_vs_EquipoB). Sin espacios.
 *
 *   Publicador -> Broker:  PUB <tema>|<cuerpo>\\n
 *      <cuerpo> es el texto del aviso (puede contener espacios). El primer '|'
 *      separa tema y cuerpo.
 *
 *   Broker -> Suscriptor (tras SUB):  OK SUB <tema>\\n
 *      (texto de aplicación; no confundir con flags TCP ACK)
 *
 *   Broker -> Suscriptor (tras PUB):   NEWS <tema>|<cuerpo>\\n
 *
 * Límites: tamaños acotados para evitar fragmentación excesiva en UDP.
 *
 * Cabeceras y macros (stddef, PUBSUB_UDP_*): ver README.md
 */
#ifndef PUBSUB_UDP_H
#define PUBSUB_UDP_H

#include <stddef.h>

/** Puerto por defecto del broker si no se pasa otro por línea de comandos. */
#define PUBSUB_UDP_DEFAULT_PORT 9000

/** Tamaño máximo de un datagrama de aplicación (carga útil legible). */
#define PUBSUB_UDP_MAX_MSG 1400

/** Longitud máxima del nombre de tema/partido (sin terminador). */
#define PUBSUB_UDP_MAX_TOPIC_LEN 63

/** Suscriptores máximos por tema en esta implementación de laboratorio. */
#define PUBSUB_UDP_MAX_SUBS_PER_TOPIC 128

/** Temas distintos que el broker puede registrar a la vez. */
#define PUBSUB_UDP_MAX_TOPICS 64

#define PUBSUB_UDP_PREFIX_SUB  "SUB "
#define PUBSUB_UDP_PREFIX_PUB  "PUB "
#define PUBSUB_UDP_PREFIX_SUB_OK "OK SUB "
#define PUBSUB_UDP_PREFIX_NEWS "NEWS "

#endif /* PUBSUB_UDP_H */
