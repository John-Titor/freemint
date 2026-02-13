ENTRY(mb_entry)

OUTPUT_FORMAT(elf32-m68k)
OUTPUT_ARCH(m68k)

PHDRS
{
	text PT_LOAD FLAGS(5);
	data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0x03f00000;

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
