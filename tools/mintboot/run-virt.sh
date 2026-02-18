#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

to_abs_path() {
	local path=$1
	case "$path" in
		/*) printf '%s\n' "$path" ;;
		*) printf '%s/%s\n' "$PWD" "$path" ;;
	esac
}

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

prepare_artifact_dir() {
	local artifact_dir=$1

	if [[ "$artifact_dir" != /tmp/* ]]; then
		echo "ARTIFACT_DIR must be under /tmp: $artifact_dir" >&2
		exit 1
	fi

	rm -rf "$artifact_dir"
	mkdir -p "$artifact_dir"
}

parse_last_hex_after_label() {
	local label=$1
	local file=$2
	local line value

	line=$(grep -E "$label[[:space:]]*0x[0-9a-fA-F]+" "$file" | tail -n1 || true)
	if [[ -z "$line" ]]; then
		return 1
	fi
	value=$(sed -E "s/.*$label[[:space:]]*0x([0-9a-fA-F]+).*/\\1/" <<<"$line")
	if [[ -z "$value" || "$value" == "$line" ]]; then
		return 1
	fi
	printf '%s\n' "$value"
}

section_vma_size() {
	local binary=$1
	local section=$2
	local objdump=${OBJDUMP:-m68k-atari-mintelf-objdump}
	local line size vma

	line=$("$objdump" -h "$binary" | awk -v sec="$section" '$2 == sec { print $3 " " $4; exit }')
	if [[ -z "$line" ]]; then
		return 1
	fi
	read -r size vma <<<"$line"
	printf '%s %s\n' "$vma" "$size"
}

dump_stack_window() {
	local label=$1
	local addr_hex=$2
	local mem_file=$3
	local mem_size=$4
	local addr start_raw end_raw start end len

	addr=$((16#$addr_hex))
	if (( addr >= mem_size )); then
		echo "mintboot: $label=0x$(printf '%08x' "$addr") outside RAM image (size=0x$(printf '%x' "$mem_size"))"
		return
	fi

	start_raw=$((addr >= 64 ? addr - 64 : 0))
	end_raw=$((addr + 64))
	if (( end_raw > mem_size )); then
		end_raw=$mem_size
	fi
	start=$((start_raw & ~0xf))
	end=$(((end_raw + 15) & ~0xf))
	if (( end > mem_size )); then
		end=$mem_size
	fi
	if (( end <= start )); then
		echo "mintboot: $label window empty"
		return
	fi
	len=$((end - start))

	echo "mintboot: $label hexdump around 0x$(printf '%08x' "$addr") [0x$(printf '%08x' "$start"),0x$(printf '%08x' "$end"))"
	xxd -g2 -u -s "$start" -l "$len" "$mem_file"
}

disassemble_function_for_pc() {
	local binary=$1
	local pc_bin=$2
	local text_end=$3
	local name=$4
	local disasm_cpu=$5
	local nm_tool=${NM:-m68k-atari-mintelf-nm}
	local objdump=${OBJDUMP:-m68k-atari-mintelf-objdump}
	local sym_addr=-1 next_addr=$text_end sym_name=""
	local addr type sym a

	while read -r addr type sym _; do
		[[ -z "${addr:-}" || -z "${type:-}" || -z "${sym:-}" ]] && continue
		case "$type" in
			t|T|w|W) ;;
			*) continue ;;
		esac
		[[ "$sym" == .L* ]] && continue
		a=$((16#$addr))
		if (( a <= pc_bin )); then
			sym_addr=$a
			sym_name=$sym
			continue
		fi
		next_addr=$a
		break
	done < <("$nm_tool" -n "$binary")

	if (( sym_addr < 0 )); then
		echo "mintboot: $name PC has no matching symbol in $binary"
		return
	fi
	if (( next_addr <= sym_addr )); then
		next_addr=$((sym_addr + 2))
	fi

	echo "mintboot: $name symbol $sym_name @ 0x$(printf '%x' "$sym_addr"), pc=0x$(printf '%x' "$pc_bin")"
	"$objdump" -d -M "$disasm_cpu" --start-address="$sym_addr" --stop-address="$next_addr" "$binary"
}

analyze_qemu_failure() {
	local run_log=$1
	local mem_file=$2
	local elf=$3
	local kernel_src=$4
	local disasm_cpu=$5
	local pc_hex usp_hex ssp_hex reloc_hex
	local mem_size
	local objdump=${OBJDUMP:-m68k-atari-mintelf-objdump}
	local mint_vma_hex mint_size_hex kernel_vma_hex kernel_size_hex
	local mint_vma mint_size mint_end kernel_vma kernel_size kernel_end
	local pc pc_bin reloc_base kernel_rt_start kernel_rt_end

	if [[ ! -f "$run_log" || ! -f "$mem_file" ]]; then
		return
	fi

	require_cmd xxd
	require_cmd "$objdump"
	require_cmd "${NM:-m68k-atari-mintelf-nm}"

	pc_hex=$(parse_last_hex_after_label "PC" "$run_log" || true)
	usp_hex=$(parse_last_hex_after_label "USP" "$run_log" || true)
	ssp_hex=$(parse_last_hex_after_label "SSP" "$run_log" || true)
	reloc_hex=$(grep -E 'PRG relocation base=[0-9a-fA-F]+' "$run_log" | tail -n1 | sed -E 's/.*=([0-9a-fA-F]+).*/\1/' || true)
	mem_size=$(wc -c < "$mem_file" | tr -d '[:space:]')

	echo "mintboot: qemu failure analysis from $run_log"
	if [[ -n "$pc_hex" ]]; then
		echo "mintboot: fault PC=0x$pc_hex"
	else
		echo "mintboot: fault PC not found in log"
	fi
	[[ -n "$usp_hex" ]] && dump_stack_window "USP" "$usp_hex" "$mem_file" "$mem_size"
	[[ -n "$ssp_hex" ]] && dump_stack_window "SSP" "$ssp_hex" "$mem_file" "$mem_size"

	if [[ -z "$pc_hex" ]]; then
		return
	fi
	pc=$((16#$pc_hex))

	read -r mint_vma_hex mint_size_hex < <(section_vma_size "$elf" ".text")
	mint_vma=$((16#$mint_vma_hex))
	mint_size=$((16#$mint_size_hex))
	mint_end=$((mint_vma + mint_size))

	if (( pc >= mint_vma && pc < mint_end )); then
		echo "mintboot: PC is in mintboot text [0x$(printf '%08x' "$mint_vma"),0x$(printf '%08x' "$mint_end"))"
		disassemble_function_for_pc "$elf" "$pc" "$mint_end" "mintboot" "$disasm_cpu"
		return
	fi

	if ! read -r kernel_vma_hex kernel_size_hex < <(section_vma_size "$kernel_src" ".text"); then
		echo "mintboot: kernel text section unavailable in $kernel_src"
		return
	fi
	if [[ -z "$reloc_hex" ]]; then
		echo "mintboot: kernel relocation base not found in log"
		return
	fi

	kernel_vma=$((16#$kernel_vma_hex))
	kernel_size=$((16#$kernel_size_hex))
	reloc_base=$((16#$reloc_hex))
	kernel_rt_start=$((reloc_base + kernel_vma))
	kernel_rt_end=$((kernel_rt_start + kernel_size))

	if (( pc >= kernel_rt_start && pc < kernel_rt_end )); then
		pc_bin=$((pc - reloc_base))
		kernel_end=$((kernel_vma + kernel_size))
		echo "mintboot: PC is in kernel text [0x$(printf '%08x' "$kernel_rt_start"),0x$(printf '%08x' "$kernel_rt_end")) reloc=0x$(printf '%08x' "$reloc_base")"
		disassemble_function_for_pc "$kernel_src" "$pc_bin" "$kernel_end" "kernel" "$disasm_cpu"
		return
	fi

	echo "mintboot: PC 0x$(printf '%08x' "$pc") is outside mintboot and kernel text ranges"
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
	if ! ramdisk_has_raw_fat_bpb "$ramdisk"; then
		return 0
	fi
	return 1
}

ramdisk_has_raw_fat_bpb() {
	local ramdisk=$1
	local boot_hex sig bps spc rsvd nfats fatsz16 fatsz32
	local off lo hi

	boot_hex=$(xxd -p -l 512 "$ramdisk" | tr -d '\n')
	if [[ ${#boot_hex} -lt 1024 ]]; then
		return 1
	fi

	off=$((510 * 2))
	sig=${boot_hex:$off:4}
	if [[ "$sig" != "55aa" ]]; then
		return 1
	fi

	off=$((11 * 2))
	lo=${boot_hex:$off:2}
	hi=${boot_hex:$((off + 2)):2}
	bps=$((16#$hi$lo))
	case "$bps" in
		512|1024|2048|4096) ;;
		*) return 1 ;;
	esac

	off=$((13 * 2))
	spc=$((16#${boot_hex:$off:2}))
	if (( spc == 0 || (spc & (spc - 1)) != 0 )); then
		return 1
	fi

	off=$((14 * 2))
	lo=${boot_hex:$off:2}
	hi=${boot_hex:$((off + 2)):2}
	rsvd=$((16#$hi$lo))
	if (( rsvd == 0 )); then
		return 1
	fi

	off=$((16 * 2))
	nfats=$((16#${boot_hex:$off:2}))
	if (( nfats == 0 )); then
		return 1
	fi

	off=$((22 * 2))
	lo=${boot_hex:$off:2}
	hi=${boot_hex:$((off + 2)):2}
	fatsz16=$((16#$hi$lo))

	off=$((36 * 2))
	lo=${boot_hex:$off:2}
	hi=${boot_hex:$((off + 2)):2}
	fatsz32=$((16#$hi$lo))

	if (( fatsz16 == 0 && fatsz32 == 0 )); then
		return 1
	fi

	return 0
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
	local mint_vers_path

	require_cmd mformat
	require_cmd mcopy
	require_cmd mmd

	dd if=/dev/zero of="$ramdisk" bs=1048576 count="$ramdisk_size_mib" 2>/dev/null

	mformat -i "$ramdisk" -r "$ramdisk_sector_size" -c 1 ::

	mint_vers_path=$(kernel_version_path "$version_h")
	mmd -i "$ramdisk" ::/MINT 2>/dev/null || true
	mmd -i "$ramdisk" "::/MINT/$mint_vers_path" 2>/dev/null || true
	mcopy -o -i "$ramdisk" "$kernel_src" "::/MINT/$mint_vers_path/MINT040.PRG"

	if find "$ramdisk_dir" -type f -print -quit | grep -q .; then
		mcopy -i "$ramdisk" -s "$ramdisk_dir"/* ::
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
	local artifact_dir=$9
	local qemu_cpu=${10}
	local qemu_pipe tee_pid rc
	local -a qemu_cmd trace_args

	qemu_cmd=(
		"$qemu"
		-M "virt,memory-backend=ram"
		-object "memory-backend-file,id=ram,size=$mem_size_bytes,mem-path=$mem_file,share=on"
		-cpu "$qemu_cpu"
		-action panic=exit-failure
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

	if (
		cd "$artifact_dir"
		"${qemu_cmd[@]}"
	) >"$qemu_pipe" 2>&1; then
		rc=0
	else
		rc=$?
	fi

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
	local artifact_dir="${ARTIFACT_DIR:-/tmp/mintboot-virt-run}"
	local qemu="${QEMU:-$qemu_default}"
	local elf="${ELF:-$ROOT/.compile_virt/mintboot-virt.elf}"
	local ramdisk="${RAMDISK:-$ROOT/ramdisk.img}"
	local ramdisk_dir="${RAMDISK_DIR:-$ROOT/ramdisk.d}"
	local mem_file="${MEM_FILE:-$artifact_dir/memory.bin}"
	local cmdline="${CMDLINE:-}"
	local qemu_trace="${QEMU_TRACE:-}"
	local run_log="${RUN_LOG:-$artifact_dir/run.log}"
	local cov_stream="${COV_STREAM:-$artifact_dir/coverage.stream}"
	local qemu_cpu="${QEMU_CPU:-m68040}"
	local disasm_cpu="${DISASM_CPU:-${qemu_cpu#m}}"
	local version_h="${VERSION_H:-$ROOT/../../sys/buildinfo/version.h}"
	local kernel_src="${KERNEL_SRC:-$ROOT/../../sys/compile.040/mint040.prg}"
	local ramdisk_size_mib="${RAMDISK_SIZE_MIB:-32}"
	local ramdisk_sector_size="${RAMDISK_SECTOR_SIZE:-512}"
	local base_mem_size_mib="${BASE_MEM_SIZE_MIB:-64}"
	local mem_size_bytes qemu_rc analysis_tmp

	if [[ ! -f "$kernel_src" ]]; then
		kernel_src="$ROOT/../../sys/.compile_040/mint040.prg"
	fi

	if (($# > 0)); then
		cmdline="$*"
	fi

	qemu=$(to_abs_path "$qemu")
	elf=$(to_abs_path "$elf")
	ramdisk=$(to_abs_path "$ramdisk")
	ramdisk_dir=$(to_abs_path "$ramdisk_dir")
	mem_file=$(to_abs_path "$mem_file")
	run_log=$(to_abs_path "$run_log")
	cov_stream=$(to_abs_path "$cov_stream")
	kernel_src=$(to_abs_path "$kernel_src")
	version_h=$(to_abs_path "$version_h")

	require_file "$elf" "mintboot-virt.elf"
	require_executable "$qemu" "qemu binary"
	require_file "$kernel_src" "kernel"
	require_file "$version_h" "version header"

	prepare_artifact_dir "$artifact_dir"
	mkdir -p "$ramdisk_dir"
	if need_ramdisk_regen "$ramdisk" "$ramdisk_dir" "$kernel_src"; then
		build_ramdisk "$ramdisk" "$ramdisk_dir" "$kernel_src" "$version_h" \
			"$ramdisk_size_mib" "$ramdisk_sector_size"
	fi

	mem_size_bytes=$(ensure_mem_file "$ramdisk" "$mem_file" "$base_mem_size_mib")

	if run_qemu "$qemu" "$mem_size_bytes" "$mem_file" "$elf" "$ramdisk" "$qemu_trace" "$cmdline" "$run_log" "$artifact_dir" "$qemu_cpu"; then
		qemu_rc=0
	else
		qemu_rc=$?
	fi

	process_coverage "$run_log" "$cov_stream" "$ROOT"
	if [[ "$qemu_rc" -ne 0 ]]; then
		if grep -q 'mintboot panic:' "$run_log"; then
			analysis_tmp=$(mktemp /tmp/mintboot-analysis.XXXXXX)
			analyze_qemu_failure "$run_log" "$mem_file" "$elf" "$kernel_src" "$disasm_cpu" >"$analysis_tmp"
			cat "$analysis_tmp"
			{
				echo
				echo "==== mintboot panic analysis ===="
				cat "$analysis_tmp"
				echo "==== end panic analysis ===="
			} >> "$run_log"
			rm -f "$analysis_tmp"
		else
			analyze_qemu_failure "$run_log" "$mem_file" "$elf" "$kernel_src" "$disasm_cpu"
		fi
	fi

	if [[ "$qemu_rc" -eq 0 ]]; then
		echo "mintboot: qemu exited with zero status"
	else
		echo "mintboot: qemu exited with non-zero status ($qemu_rc)"
	fi
	echo "mintboot: run log at $run_log"
	echo "mintboot: preserved guest RAM image at $mem_file"
	exit "$qemu_rc"
}

main "$@"
