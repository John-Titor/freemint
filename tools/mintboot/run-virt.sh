#!/bin/sh
set -e

QEMU=${QEMU:-qemu-system-m68k}
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ELF="$ROOT/mintboot-virt.elf"
RAMDISK="$ROOT/ramdisk.img"
RAMDISK_DIR="$ROOT/ramdisk.d"
RAMDISK_SIZE_MIB=32
RAMDISK_SECTOR_SIZE=512
RAMDISK_FIRST_SECTOR=2048
CMDLINE=${CMDLINE:-}
VERSION_H="$ROOT/../../sys/buildinfo/version.h"
KERNEL_SRC="$ROOT/../../sys/compile.040/mint040.prg"
if [ ! -f "$KERNEL_SRC" ]; then
	KERNEL_SRC="$ROOT/../../sys/.compile_040/mint040.prg"
fi

if [ ! -f "$ELF" ]; then
	echo "mintboot-virt.elf not found: $ELF" >&2
	exit 1
fi
if [ ! -f "$KERNEL_SRC" ]; then
	echo "kernel not found: $ROOT/../../sys/compile.040/mint040.prg" >&2
	exit 1
fi
if [ ! -f "$VERSION_H" ]; then
	echo "version header not found: $VERSION_H" >&2
	exit 1
fi

if [ ! -d "$RAMDISK_DIR" ]; then
	mkdir -p "$RAMDISK_DIR"
fi

regen_ramdisk=0
if [ ! -f "$RAMDISK" ]; then
	regen_ramdisk=1
else
	if find "$RAMDISK_DIR" -type f -newer "$RAMDISK" -print -quit | grep -q .; then
		regen_ramdisk=1
	fi
	if [ "$KERNEL_SRC" -nt "$RAMDISK" ]; then
		regen_ramdisk=1
	fi
fi

if [ "$regen_ramdisk" -eq 1 ]; then
	if ! command -v mformat >/dev/null 2>&1; then
		echo "mformat not found; cannot build ramdisk image" >&2
		exit 1
	fi
	if ! command -v mcopy >/dev/null 2>&1; then
		echo "mcopy not found; cannot populate ramdisk image" >&2
		exit 1
	fi

	ramdisk_size=$((RAMDISK_SIZE_MIB * 1024 * 1024))
	total_sectors=$((ramdisk_size / RAMDISK_SECTOR_SIZE))
	part_sectors=$((total_sectors - RAMDISK_FIRST_SECTOR))
	part_offset=$((RAMDISK_FIRST_SECTOR * RAMDISK_SECTOR_SIZE))

	dd if=/dev/zero of="$RAMDISK" bs=1048576 count="$RAMDISK_SIZE_MIB" 2>/dev/null

	start_lba=$RAMDISK_FIRST_SECTOR
	secs=$part_sectors
	start_le=$(printf '\\x%02x\\x%02x\\x%02x\\x%02x' \
		$((start_lba & 0xff)) \
		$(((start_lba >> 8) & 0xff)) \
		$(((start_lba >> 16) & 0xff)) \
		$(((start_lba >> 24) & 0xff)))
	size_le=$(printf '\\x%02x\\x%02x\\x%02x\\x%02x' \
		$((secs & 0xff)) \
		$(((secs >> 8) & 0xff)) \
		$(((secs >> 16) & 0xff)) \
		$(((secs >> 24) & 0xff)))

	entry=$(printf '\\x00\\xff\\xff\\xff\\x06\\xff\\xff\\xff')
	printf "%b%b%b" "$entry" "$start_le" "$size_le" | dd of="$RAMDISK" bs=1 seek=446 conv=notrunc 2>/dev/null
	printf '\x55\xaa' | dd of="$RAMDISK" bs=1 seek=510 conv=notrunc 2>/dev/null

	mformat -i "$RAMDISK@@$part_offset" -r 512 -c 1 ::

	MINT_MAJ=$(awk '/^#define[[:space:]]+MINT_MAJ_VERSION/ {print $3}' "$VERSION_H")
	MINT_MIN=$(awk '/^#define[[:space:]]+MINT_MIN_VERSION/ {print $3}' "$VERSION_H")
	MINT_PATCH=$(awk '/^#define[[:space:]]+MINT_PATCH_LEVEL/ {print $3}' "$VERSION_H")
	MINT_STATUS_CVS=$(awk '/^#define[[:space:]]+MINT_STATUS_CVS/ {print $3}' "$VERSION_H")
	if [ "$MINT_STATUS_CVS" -ne 0 ]; then
		MINT_VERS_PATH="${MINT_MAJ}-${MINT_MIN}-cur"
	else
		MINT_VERS_PATH="${MINT_MAJ}-${MINT_MIN}-${MINT_PATCH}"
	fi

	mmd -i "$RAMDISK@@$part_offset" ::/MINT 2>/dev/null || true
	mmd -i "$RAMDISK@@$part_offset" "::/MINT/$MINT_VERS_PATH" 2>/dev/null || true
	mcopy -o -i "$RAMDISK@@$part_offset" "$KERNEL_SRC" "::/MINT/$MINT_VERS_PATH/MINT040.PRG"

	if find "$RAMDISK_DIR" -type f -print -quit | grep -q .; then
		mcopy -i "$RAMDISK@@$part_offset" -s "$RAMDISK_DIR"/* ::
	fi
fi

if [ $# -gt 0 ]; then
	CMDLINE="$*"
fi

exec "$QEMU" \
	-M virt \
	-cpu m68040 \
	-nographic \
	-serial mon:stdio \
	-kernel "$ELF" \
	-initrd "$RAMDISK" \
	${CMDLINE:+-append "$CMDLINE"}
