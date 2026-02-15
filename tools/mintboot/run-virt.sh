#!/bin/sh
set -e

QEMU_DEFAULT=/Users/agent/work/Agent/M68K/_Emulators/qemu/qemu/build/qemu-system-m68k-unsigned
QEMU=${QEMU:-$QEMU_DEFAULT}
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ELF=${ELF:-$ROOT/.compile_virt/mintboot-virt.elf}
RAMDISK="$ROOT/ramdisk.img"
RAMDISK_DIR="$ROOT/ramdisk.d"
RAMDISK_SIZE_MIB=32
RAMDISK_SECTOR_SIZE=512
RAMDISK_FIRST_SECTOR=2048
BASE_MEM_SIZE_MIB=64
MEM_FILE=${MEM_FILE:-/tmp/mintboot-virt.mem}
CMDLINE=${CMDLINE:-}
QEMU_TRACE=${QEMU_TRACE:-}
RUN_LOG=${RUN_LOG:-$ROOT/.compile_virt/run-virt.log}
COV_STREAM=${COV_STREAM:-$ROOT/.compile_virt/coverage.stream}
VERSION_H="$ROOT/../../sys/buildinfo/version.h"
KERNEL_SRC="$ROOT/../../sys/compile.040/mint040.prg"
if [ ! -f "$KERNEL_SRC" ]; then
	KERNEL_SRC="$ROOT/../../sys/.compile_040/mint040.prg"
fi

if [ ! -f "$ELF" ]; then
	echo "mintboot-virt.elf not found: $ELF" >&2
	exit 1
fi
if [ ! -x "$QEMU" ]; then
	echo "qemu binary not executable: $QEMU" >&2
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

ramdisk_bytes=$(wc -c < "$RAMDISK" | tr -d '[:space:]')
ramdisk_mib_rounded=$(((ramdisk_bytes + 1048576 - 1) / 1048576))
MEM_SIZE_MIB=$((BASE_MEM_SIZE_MIB + ramdisk_mib_rounded))
MEM_SIZE_BYTES=$((MEM_SIZE_MIB * 1048576))

if [ ! -f "$MEM_FILE" ]; then
	dd if=/dev/zero of="$MEM_FILE" bs=1048576 count="$MEM_SIZE_MIB" 2>/dev/null
else
	mem_size=$(wc -c < "$MEM_FILE")
	if [ "$mem_size" -ne "$MEM_SIZE_BYTES" ]; then
		dd if=/dev/zero of="$MEM_FILE" bs=1048576 count="$MEM_SIZE_MIB" 2>/dev/null
	fi
fi

mkdir -p "$(dirname "$RUN_LOG")"
rm -f "$RUN_LOG"

QEMU_PIPE=$(mktemp -u /tmp/mintboot-qemu.XXXXXX)
mkfifo "$QEMU_PIPE"
tee "$RUN_LOG" < "$QEMU_PIPE" &
TEE_PID=$!

set +e
"$QEMU" \
	-M virt,memory-backend=ram \
	-object memory-backend-file,id=ram,size="$MEM_SIZE_BYTES",mem-path="$MEM_FILE",share=on \
	-cpu m68040 \
	-nographic \
	-serial mon:stdio \
	-kernel "$ELF" \
	-initrd "$RAMDISK" \
	${QEMU_TRACE:+$QEMU_TRACE} \
	${CMDLINE:+-append "$CMDLINE"} > "$QEMU_PIPE" 2>&1
QEMU_RC=$?
set -e

wait "$TEE_PID"
rm -f "$QEMU_PIPE"

if grep -q '^MB-COV-BEGIN' "$RUN_LOG"; then
	if ! command -v xxd >/dev/null 2>&1; then
		echo "xxd not found; cannot decode coverage stream" >&2
		exit 1
	fi
	if ! command -v m68k-atari-mintelf-gcov-tool >/dev/null 2>&1; then
		echo "m68k-atari-mintelf-gcov-tool not found; cannot merge coverage stream" >&2
		exit 1
	fi

	rm -f "$COV_STREAM"

	awk '/^MB-COV:/{ line=$0; sub(/\r$/, "", line); print substr(line,8); }' "$RUN_LOG" \
		| tr -d '\n' \
		| xxd -r -p > "$COV_STREAM"

	if [ -s "$COV_STREAM" ]; then
		m68k-atari-mintelf-gcov-tool merge-stream "$COV_STREAM"
		echo "coverage stream merged into object directories (e.g. $ROOT/.compile_virt)"
	else
		echo "coverage markers found but stream was empty" >&2
	fi
fi

if [ "$QEMU_RC" -eq 0 ]; then
	echo "mintboot: qemu exited with zero status"
else
	echo "mintboot: qemu exited with non-zero status ($QEMU_RC)"
fi

echo "mintboot: preserved guest RAM image at $MEM_FILE"

exit "$QEMU_RC"
