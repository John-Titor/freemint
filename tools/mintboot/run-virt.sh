#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

require_file() {
	local path=$1
	local name=$2
	if [[ ! -f "$path" ]]; then
		echo "$name not found: $path" >&2
		exit 1
	fi
}

require_executable() {
	local path=$1
	local name=$2
	if [[ ! -x "$path" ]]; then
		echo "$name not executable: $path" >&2
		exit 1
	fi
}

require_cmd() {
	local cmd=$1
	if ! command -v "$cmd" >/dev/null 2>&1; then
		echo "$cmd not found" >&2
		exit 1
	fi
}

need_ramdisk_regen() {
	local ramdisk=$1
	local ramdisk_dir=$2
	local kernel_src=$3

	if [[ ! -f "$ramdisk" ]]; then
		return 0
	fi
	if find "$ramdisk_dir" -type f -newer "$ramdisk" -print -quit | grep -q .; then
		return 0
	fi
	if [[ "$kernel_src" -nt "$ramdisk" ]]; then
		return 0
	fi
	return 1
}

kernel_version_path() {
	local version_h=$1
	local mint_maj mint_min mint_patch mint_status_cvs

	mint_maj=$(awk '/^#define[[:space:]]+MINT_MAJ_VERSION/ {print $3}' "$version_h")
	mint_min=$(awk '/^#define[[:space:]]+MINT_MIN_VERSION/ {print $3}' "$version_h")
	mint_patch=$(awk '/^#define[[:space:]]+MINT_PATCH_LEVEL/ {print $3}' "$version_h")
	mint_status_cvs=$(awk '/^#define[[:space:]]+MINT_STATUS_CVS/ {print $3}' "$version_h")
	if [[ "$mint_status_cvs" -ne 0 ]]; then
		echo "${mint_maj}-${mint_min}-cur"
	else
		echo "${mint_maj}-${mint_min}-${mint_patch}"
	fi
}

build_ramdisk() {
	local ramdisk=$1
	local ramdisk_dir=$2
	local kernel_src=$3
	local version_h=$4
	local ramdisk_size_mib=$5
	local ramdisk_sector_size=$6
	local ramdisk_first_sector=$7
	local ramdisk_size total_sectors part_sectors part_offset
	local start_lba secs start_le size_le entry mint_vers_path

	require_cmd mformat
	require_cmd mcopy
	require_cmd mmd

	ramdisk_size=$((ramdisk_size_mib * 1024 * 1024))
	total_sectors=$((ramdisk_size / ramdisk_sector_size))
	part_sectors=$((total_sectors - ramdisk_first_sector))
	part_offset=$((ramdisk_first_sector * ramdisk_sector_size))

	dd if=/dev/zero of="$ramdisk" bs=1048576 count="$ramdisk_size_mib" 2>/dev/null

	start_lba=$ramdisk_first_sector
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
	printf "%b%b%b" "$entry" "$start_le" "$size_le" | dd of="$ramdisk" bs=1 seek=446 conv=notrunc 2>/dev/null
	printf '\x55\xaa' | dd of="$ramdisk" bs=1 seek=510 conv=notrunc 2>/dev/null

	mformat -i "$ramdisk@@$part_offset" -r 512 -c 1 ::

	mint_vers_path=$(kernel_version_path "$version_h")
	mmd -i "$ramdisk@@$part_offset" ::/MINT 2>/dev/null || true
	mmd -i "$ramdisk@@$part_offset" "::/MINT/$mint_vers_path" 2>/dev/null || true
	mcopy -o -i "$ramdisk@@$part_offset" "$kernel_src" "::/MINT/$mint_vers_path/MINT040.PRG"

	if find "$ramdisk_dir" -type f -print -quit | grep -q .; then
		mcopy -i "$ramdisk@@$part_offset" -s "$ramdisk_dir"/* ::
	fi
}

ensure_mem_file() {
	local ramdisk=$1
	local mem_file=$2
	local base_mem_size_mib=$3
	local ramdisk_bytes ramdisk_mib_rounded mem_size_mib mem_size_bytes mem_size

	ramdisk_bytes=$(wc -c < "$ramdisk" | tr -d '[:space:]')
	ramdisk_mib_rounded=$(((ramdisk_bytes + 1048576 - 1) / 1048576))
	mem_size_mib=$((base_mem_size_mib + ramdisk_mib_rounded))
	mem_size_bytes=$((mem_size_mib * 1048576))

	if [[ ! -f "$mem_file" ]]; then
		dd if=/dev/zero of="$mem_file" bs=1048576 count="$mem_size_mib" 2>/dev/null
	else
		mem_size=$(wc -c < "$mem_file")
		if [[ "$mem_size" -ne "$mem_size_bytes" ]]; then
			dd if=/dev/zero of="$mem_file" bs=1048576 count="$mem_size_mib" 2>/dev/null
		fi
	fi

	echo "$mem_size_bytes"
}

run_qemu() {
	local qemu=$1
	local mem_size_bytes=$2
	local mem_file=$3
	local elf=$4
	local ramdisk=$5
	local qemu_trace=$6
	local cmdline=$7
	local run_log=$8
	local qemu_pipe tee_pid rc
	local -a qemu_cmd trace_args

	qemu_cmd=(
		"$qemu"
		-M "virt,memory-backend=ram"
		-object "memory-backend-file,id=ram,size=$mem_size_bytes,mem-path=$mem_file,share=on"
		-cpu m68040
		-nographic
		-serial mon:stdio
		-kernel "$elf"
		-initrd "$ramdisk"
	)

	if [[ -n "$qemu_trace" ]]; then
		# Intentionally split QEMU_TRACE into distinct arguments.
		read -r -a trace_args <<<"$qemu_trace"
		qemu_cmd+=("${trace_args[@]}")
	fi
	if [[ -n "$cmdline" ]]; then
		qemu_cmd+=(-append "$cmdline")
	fi

	mkdir -p "$(dirname "$run_log")"
	rm -f "$run_log"

	qemu_pipe=$(mktemp /tmp/mintboot-qemu.XXXXXX)
	rm -f "$qemu_pipe"
	mkfifo "$qemu_pipe"
	tee "$run_log" < "$qemu_pipe" &
	tee_pid=$!

	set +e
	"${qemu_cmd[@]}" >"$qemu_pipe" 2>&1
	rc=$?
	set -e

	wait "$tee_pid" || true
	rm -f "$qemu_pipe"
	return "$rc"
}

process_coverage() {
	local run_log=$1
	local cov_stream=$2
	local root=$3

	if ! grep -q '^MB-COV-BEGIN' "$run_log"; then
		return
	fi

	require_cmd xxd
	require_cmd m68k-atari-mintelf-gcov-tool

	rm -f "$cov_stream"
	awk '/^MB-COV:/{ line=$0; sub(/\r$/, "", line); print substr(line,8); }' "$run_log" \
		| tr -d '\n' \
		| xxd -r -p > "$cov_stream"

	if [[ -s "$cov_stream" ]]; then
		m68k-atari-mintelf-gcov-tool merge-stream "$cov_stream"
		echo "coverage stream merged into object directories (e.g. $root/.compile_virt)"
	else
		echo "coverage markers found but stream was empty" >&2
	fi
}

main() {
	local qemu_default=/Users/agent/work/Agent/M68K/_Emulators/qemu/qemu/build/qemu-system-m68k-unsigned
	local qemu="${QEMU:-$qemu_default}"
	local elf="${ELF:-$ROOT/.compile_virt/mintboot-virt.elf}"
	local ramdisk="${RAMDISK:-$ROOT/ramdisk.img}"
	local ramdisk_dir="${RAMDISK_DIR:-$ROOT/ramdisk.d}"
	local mem_file="${MEM_FILE:-/tmp/mintboot-virt.mem}"
	local cmdline="${CMDLINE:-}"
	local qemu_trace="${QEMU_TRACE:-}"
	local run_log="${RUN_LOG:-$ROOT/.compile_virt/run-virt.log}"
	local cov_stream="${COV_STREAM:-$ROOT/.compile_virt/coverage.stream}"
	local version_h="${VERSION_H:-$ROOT/../../sys/buildinfo/version.h}"
	local kernel_src="${KERNEL_SRC:-$ROOT/../../sys/compile.040/mint040.prg}"
	local ramdisk_size_mib="${RAMDISK_SIZE_MIB:-32}"
	local ramdisk_sector_size="${RAMDISK_SECTOR_SIZE:-512}"
	local ramdisk_first_sector="${RAMDISK_FIRST_SECTOR:-2048}"
	local base_mem_size_mib="${BASE_MEM_SIZE_MIB:-64}"
	local mem_size_bytes qemu_rc

	if [[ ! -f "$kernel_src" ]]; then
		kernel_src="$ROOT/../../sys/.compile_040/mint040.prg"
	fi

	if (($# > 0)); then
		cmdline="$*"
	fi

	require_file "$elf" "mintboot-virt.elf"
	require_executable "$qemu" "qemu binary"
	require_file "$kernel_src" "kernel"
	require_file "$version_h" "version header"

	mkdir -p "$ramdisk_dir"
	if need_ramdisk_regen "$ramdisk" "$ramdisk_dir" "$kernel_src"; then
		build_ramdisk "$ramdisk" "$ramdisk_dir" "$kernel_src" "$version_h" \
			"$ramdisk_size_mib" "$ramdisk_sector_size" "$ramdisk_first_sector"
	fi

	mem_size_bytes=$(ensure_mem_file "$ramdisk" "$mem_file" "$base_mem_size_mib")

	set +e
	run_qemu "$qemu" "$mem_size_bytes" "$mem_file" "$elf" "$ramdisk" "$qemu_trace" "$cmdline" "$run_log"
	qemu_rc=$?
	set -e

	process_coverage "$run_log" "$cov_stream" "$ROOT"

	if [[ "$qemu_rc" -eq 0 ]]; then
		echo "mintboot: qemu exited with zero status"
	else
		echo "mintboot: qemu exited with non-zero status ($qemu_rc)"
	fi
	echo "mintboot: preserved guest RAM image at $mem_file"
	exit "$qemu_rc"
}

main "$@"
