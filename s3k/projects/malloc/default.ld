/* See LICENSE file for copyright and license details. */
OUTPUT_ARCH(riscv)
ENTRY(_start)

__global_pointer$ = MIN(_sdata + 0x800, MAX(_data + 0x800, _end - 0x800));

SECTIONS {
	.text : {
		*( .init )
		*( .text .text.* )
	} > RAM

	.heap_metadata : {
		__heap_metadata_pointer = .;
		. += __heap_metadata_size;
        _end = .;
    } > RAM
	/*
		8 byte for pointer
		2 bytes for unique value of canary (4Kib heap (1 byte heap object) = 2^12 potential heap objects. 2 byte lets us define that)
		(Make canary metadata 4KiB for now)
	*/
	.canary_metadata : ALIGN(4096) {
		__canary_metadata_pointer = .;
		. += __canaryTable_size;
	}

	.data : {
		_data = . ;
		*( .data )
		*( .data.* )
		_sdata = . ;
		*( .sdata )
		*( .sdata.* )
	} > RAM

	.bss : {
		_bss = .;
		_sbss = .;
		*(.sbss .sbss.*)
		*(.bss .bss.*)
		_end = .;
	} > RAM

	.heap : ALIGN(4096){
		__heap_pointer = .;
		. += __heap_size;
		__canary_pointer = .;
        _end = .;
    }

	.stack : ALIGN(8) {
		. += __stack_size;
		__stack_pointer = .;
		_end = .;
	}
}
