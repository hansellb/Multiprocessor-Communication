/*
 * This file contains objects that may be used by all CPUs
 */
//#include <stdio.h>
#include "system.h"
//#include "altera_avalon_pio_regs.h"
#include "io.h"
#include "time.h"

/*
 * Parallel IO registers
 */
#include <altera_avalon_pio_regs.h>

/*
 *	Use alt_xxx functions and definitions for portability and code size.
 *	alt_stdio.h has a smaller code footprint but less functionality than stdio.h
 */
#include "sys/alt_stdio.h"
#include "alt_types.h"

/*
 * Mutex HAL and LWHAL (LightWeight)
 */
#include "altera_avalon_mutex.h"
//#include "altera_avalon_mutex_lwhal.h"

/*
 * Timing headers - System Clock & Timestamp
 */
#include "sys/alt_alarm.h"
//#include "sys/alt_timestamp.h"

/*
 * FIFO headers
 */
#include "altera_avalon_fifo_regs.h"
#include "altera_avalon_fifo_util.h"
#include "altera_avalon_fifo.h"

/*
 * Performance counter headers
 */
#include <altera_avalon_performance_counter.h>

#define requests_notepad_area_1	(philosopher *) SHARED_ONCHIP_BASE + 1024

#define TRUE 1

#define USE_ISR_FOR_BUTTONS		0

#define SHOW_PRINTF_LOW_LEVEL	0
#define SHOW_PRINTF				1

#define DATA_NOT_PROCESSED	0x69
#define DATA_PROCESSED		0x96

#define CPU_1_OFFSET		0x000
#define CPU_2_OFFSET		0x400
#define CPU_3_OFFSET		0x800
#define CPU_4_OFFSET		0xc00

#define CPU_1_SHARED_ONCHIP_ADDR	( SHARED_ONCHIP_BASE + CPU_1_OFFSET )
#define CPU_2_SHARED_ONCHIP_ADDR	( SHARED_ONCHIP_BASE + CPU_2_OFFSET )
#define CPU_3_SHARED_ONCHIP_ADDR	( SHARED_ONCHIP_BASE + CPU_3_OFFSET )
#define CPU_4_SHARED_ONCHIP_ADDR	( SHARED_ONCHIP_BASE + CPU_4_OFFSET )

#define DATA_BYTES			0x400				// Amount of BYTES in a data section/block
#define DATA_WORDS			(DATA_BYTES / 2)	// Amount of WORDS in a data section/block
#define DATA_DWORDS			(DATA_BYTES / 4)	// Amount of INTS in a data section/block
#define DATA_QWORDS			(DATA_BYTES / 8)	// Amount of LONGS in a data section/block

#define CPU_1_MESSAGE		0x11111111
#define CPU_2_MESSAGE		0x22222222
#define CPU_3_MESSAGE		0x33333333
#define CPU_4_MESSAGE		0x44444444

#define SECTION_1			1
#define SECTION_2			2
#define SECTION_3			3
#define SECTION_4			4

//Defines properties of a fork/chopstick (mutex)
typedef struct forks
{
	alt_mutex_dev	*handle;	// Handle to the mutex produced by _open
	alt_u8			is_held;	// Allows to know if the CPU has a hold of the mutex

}forks;

//This construct is often used to avoid writing: struct some_struct {...}; and then typedef struct some_struct another_struct;
typedef struct philosopher
{
	//alt_u8		number;
	//alt_u8*		notepad;		// Base address in "shared_onchip" where the philosopher can make "requests"
	alt_u8		has_request;	// Flag variable to let "Dinner host" if the philosopher has a pending request.
	alt_u8		request_type;	//
	alt_u8		angry_level;	//
	alt_u8		food[50];
	alt_u8		philosophical_thought[50];
}philosopher;
