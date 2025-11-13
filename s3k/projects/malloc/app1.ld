MEMORY {
	RAM (rwx) : ORIGIN = 0x80020000, LENGTH = 0x10000
}

__stack_size = 1024;
__canaryTable_size = 4096;
