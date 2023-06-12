ENTRY(_reset);
MEMORY {
	flash(rx) : ORIGIN = 0x08000000, LENGTH = 32k
	ram(rwx) : ORIGIN = 0x20000000, LENGTH = 12k
}
_estack = ORIGIN(ram) + LENGTH(ram);

SECTIONS {
	.vectors 	: { KEEP(*(.vectors)) }											> flash
	.text		: { *(.text.reset) *(.text.default_handler) *(.text*) } 		> flash
	.rodata		: { *(.rodata*) }												> flash

	.data : {
		_sdata = .;
		*(.first_data)
		*(.data SORT(.data.*))
		_edata = .;
	} > ram AT > flash
	_sidata = LOADADDR(.data);

	.bss : {
		_sbss = .;
		*(.bss SORT(.bss.*) COMMON)
		_ebss = .;
	} > ram

	.heap : {
		_sheap = .;
		. = . + 0x200;	
		_eheap = .;
	} > ram

	. = ALIGN(8);
	_end = .;
}
