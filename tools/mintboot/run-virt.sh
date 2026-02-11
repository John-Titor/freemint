#!/bin/sh
set -e

QEMU=${QEMU:-qemu-system-m68k}
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ELF="$ROOT/mintboot-virt.elf"

if [ ! -f "$ELF" ]; then
	echo "mintboot-virt.elf not found: $ELF" >&2
	exit 1
fi

exec "$QEMU" \
	-M virt \
	-cpu m68040 \
	-nographic \
	-serial mon:stdio \
	-kernel "$ELF"
