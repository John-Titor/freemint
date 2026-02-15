ENTRY(mb_entry)

OUTPUT_FORMAT(elf32-m68k)
OUTPUT_ARCH(m68k)

MB_VIRT_BASE = 0x03f00000;
MB_VIRT_MAX_FOOTPRINT = 0x00100000;

PHDRS
{
	text PT_LOAD FLAGS(5);
	data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = MB_VIRT_BASE;

	.vectors : {
		*(.vectors)
	} :text

    . = 0x03f00800;

	.text : {
		*(.text*)
	} :text

	.rodata : {
		*(.rodata*)
	} :text

	.gcov_info : {
		__gcov_info_start = .;
		KEEP(*(.gcov_info))
		__gcov_info_end = .;
	} :data

	.data : {
		*(.data*)
	} :data

	.bss (NOLOAD) : {
		*(.bss*)
		*(COMMON)
	} :data

	. = ALIGN(4);
	_mb_image_end = .;
}

ASSERT((_mb_image_end - MB_VIRT_BASE) <= MB_VIRT_MAX_FOOTPRINT,
       "mintboot: virt image footprint exceeds 1MiB");
