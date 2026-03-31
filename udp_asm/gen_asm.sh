#!/usr/bin/env bash
# Regenera los .s en este directorio a partir de los .c del directorio padre (misma fuente que el lab en C).
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$DIR/.." && pwd)"
GCC="${CC:-gcc}"

cd "$ROOT"
for name in broker_udp publisher_udp subscriber_udp; do
  echo "Generando ${name}.s ..."
  "$GCC" -S -O1 -std=c11 -D_DEFAULT_SOURCE \
    -fno-asynchronous-unwind-tables \
    -o "$DIR/${name}.s" \
    "${name}.c"
done

echo "Listo: $DIR/*.s"
