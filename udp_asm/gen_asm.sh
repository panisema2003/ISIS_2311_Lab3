#!/usr/bin/env bash
# Regenera los .s a partir de los .c en udp_C/ (fuente canónica del laboratorio en C).
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$DIR/.." && pwd)"
SRC="$ROOT/udp_C"
GCC="${CC:-gcc}"

for name in broker_udp publisher_udp subscriber_udp; do
  echo "Generando ${name}.s desde udp_C/${name}.c ..."
  "$GCC" -S -O1 -std=c11 -D_DEFAULT_SOURCE \
    -fno-asynchronous-unwind-tables \
    -I"$SRC" \
    -o "$DIR/${name}.s" \
    "$SRC/${name}.c"
done

echo "Listo: $DIR/*.s"
