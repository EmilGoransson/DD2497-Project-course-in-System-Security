#pragma once
#include "altc/altio.h"
#include "s3k/s3k.h"

#define UART0_BASE_ADDR (0x10000000ull)

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

#define TAG_BLOCK_TO_ADDR(tag, block) ( \
					(((uint64_t) tag) << S3K_MAX_BLOCK_SIZE) + \
					(((uint64_t) block) << S3K_MIN_BLOCK_SIZE) \
					)

void setup_uart_app0();

uint32_t find_free_cap();

void setup_trap(void (*trap_handler)(void), void * trap_stack_base, uint64_t trap_stack_size);

void s3k_print_cap(s3k_cap_t *cap);

void debug_capability_from_idx(uint32_t cap_idx);