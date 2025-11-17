MEMORY {
	RAM (rwx) : ORIGIN = 0x80020000, LENGTH = 0x10000
}

__stack_size = 1024;
__heap_size = 1024;
__heap_metadata_size = 512;