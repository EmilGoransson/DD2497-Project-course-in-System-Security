// Application process IDs and their memory layout
#define APP0_PID 0
#define APP0_BASE_ADDR 0x80010000
#define APP0_LENGHT 0x10000

#define APP1_PID 1
#define APP1_BASE_ADDR 0x80020000
#define APP1_LENGHT 0x10000

#define APP2_PID 2
#define APP2_BASE_ADDR 0x80030000
#define APP2_LENGHT 0x10000

// See plat_conf.h
#define BOOT_PMP 0
#define RAM_MEM 1
#define UART_MEM 2
#define TIME_MEM 3
#define HART0_TIME 4
#define HART1_TIME 5
#define HART2_TIME 6
#define HART3_TIME 7
#define MONITOR 8
#define CHANNEL 9

// normal caps for setting up UART in app0
#define UART_CAP 10
#define UART_PMP 1

// CAPS for creating process in app0
#define FREE_CAP_BEGIN 12
#define FREE_CAP_END 20


static inline void setup_uart_app0()
{
	uint64_t uart_addr = s3k_napot_encode(UART0_BASE_ADDR, 0x8);
	// Derive a PMP capability for accessing UART
	s3k_cap_derive(UART_MEM, UART_CAP, s3k_mk_pmp(uart_addr, S3K_MEM_RW));
	// Load the derive PMP capability to PMP configuration
	s3k_pmp_load(UART_CAP, UART_PMP);
	// Synchronize PMP unit (hardware) with PMP configuration
	// false => not full synchronization.
	s3k_sync_mem();
}

static inline uint32_t find_free_cap() {
	s3k_cap_t cap;
    for (uint32_t i = FREE_CAP_BEGIN; i <= FREE_CAP_END; i++) {
    	if (s3k_cap_read(i, &cap))
            return i;
    }
    return 0;
}

static inline void setup_trap(void (*trap_handler)(void), void * trap_stack_base, uint64_t trap_stack_size)
{
	// Sets the trap handler
	s3k_reg_write(S3K_REG_TPC, (uint64_t)trap_handler);
	// Set the trap stack
	s3k_reg_write(S3K_REG_TSP, ((uint64_t)trap_stack_base) + trap_stack_size);
}

#define TAG_BLOCK_TO_ADDR(tag, block) ( \
					(((uint64_t) tag) << S3K_MAX_BLOCK_SIZE) + \
					(((uint64_t) block) << S3K_MIN_BLOCK_SIZE) \
					)

static inline void s3k_print_cap(s3k_cap_t *cap) {
	if (!cap)
		alt_printf("Capability is NULL\n");
	switch ((*cap).type) {
	case S3K_CAPTY_NONE:
		alt_printf("No Capability\n");
		break;
	case S3K_CAPTY_TIME:
		alt_printf("Time hart:%X bgn:%X mrk:%X end:%d\n",
				   (*cap).time.hart, (*cap).time.bgn, (*cap).time.mrk, (*cap).time.end);
		break;
	case S3K_CAPTY_MEMORY:
		alt_printf("Memory rwx:%X lock:%X bgn:%X mrk:%X end:%X\n",
				   (*cap).mem.rwx, (*cap).mem.lck,
				   TAG_BLOCK_TO_ADDR((*cap).mem.tag, (*cap).mem.bgn),
				   TAG_BLOCK_TO_ADDR((*cap).mem.tag, (*cap).mem.mrk),
				   TAG_BLOCK_TO_ADDR((*cap).mem.tag, (*cap).mem.end)
				   );
		break;
	case S3K_CAPTY_PMP:
		uint64_t pmp_start_pos;
		uint64_t pmp_size;
		s3k_napot_decode((*cap).pmp.addr, &pmp_start_pos, &pmp_size);
		alt_printf("PMP rwx:%X used:%X index:%X address: 0x%X size: 0x%X\n",
				   (*cap).pmp.rwx, (*cap).pmp.used, (*cap).pmp.slot, pmp_start_pos, pmp_size);
		break;
	case S3K_CAPTY_MONITOR:
		alt_printf("Monitor  bgn:%X mrk:%X end:%X\n",
				    (*cap).mon.bgn, (*cap).mon.mrk, (*cap).mon.end);
		break;
	case S3K_CAPTY_CHANNEL:
		alt_printf("Channel  bgn:%X mrk:%X end:%X\n",
				    (*cap).chan.bgn, (*cap).chan.mrk, (*cap).chan.end);
		break;
	case S3K_CAPTY_SOCKET:
		alt_printf("Socket  mode:%X perm:%X channel:%X tag:%X\n",
				    (*cap).sock.mode, (*cap).sock.perm, (*cap).sock.chan, (*cap).sock.tag);
		break;
	}
}

static inline void debug_capability_from_idx(uint32_t cap_idx) {
	s3k_cap_t cap;
	while (s3k_cap_read(cap_idx, &cap));
	s3k_print_cap(&cap);
}