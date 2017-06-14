/*
 * Simple IPC implementation
 *
 * Description:
 * cpu_0 will fill a 4k section of share_onchip with the same value for each section
 * cpu_1 - cpu_4 will read and write back to the shared memory after performing some operation on the data
 *
 * Implementation details:
 * - ONLY 1 mutex will be used to control access to the shared memory
 * - shared memory will be "implicitly" divided into 4 sections, each available to one CPU.
 * - cpu_0 will initialize each shared memory section with some value EXCEPT for the "flag" (the 1st byte) which is initialized to 0x69
 * - cpu_0 will check the flags in ALL CPU shared_onchip memory sections and will turn on LEDs in the "leds_reds" peripheral if a CPU has finished processing the data
 * - cpu_0 will accept input from Qsys' "Buttons" peripheral to dump the value of shared_onchip memory for a specific CPU
 * - showing the shared_onchip memory can be done in bytes, words or ints depending on user input
 * - cpu_0 will also monitor the "switches" peripheral to detect whether to write to one specific CPU section or to ALL CPU sections at the same time
 * - cpu_1 - cpu_4 will modify the data (with arithmetic and/or logic operations) and then set a flag to indicate data processing finished
 * - after cpu_1 through cpu_4 update the shared memory data, cpu_0 will turn on some LEDs in "leds_red" to indicate when data from each CPU has been processed
 * - cpu_1 - cpu_4 will process the data locally and differently ( one byte|char, two bytes|one word OR four bytes|one dword|one int at a time ) to have different processing speeds
 * - after data has been proccessed, cpu_1 - cpu_4 will only check the "flag" to see if more data is available AND will check if local data has already been updated in the shared_onchip memory
 * - at the very end, wait 5 milliseconds
 */
#include "../inc/common_objs.h"

extern void delay (int millisec);



/*
 * Main
 */
int main()
{
// Variable's declarations
	alt_16			i;	//general loop variable
	alt_u32			x;	//general purpose variables
	alt_u8			s[8];

	alt_mutex_dev	*mutex_0;	// Handle to the mutex hardware

	alt_u8			*shared_mem_char_ptr;	// Use this pointer to read shared_memory ONE byte ( ONE char/byte ) at a time
	alt_u16			*shared_mem_word_ptr;	// Use this pointer to read shared_memory TWO byte ( ONE word/short ) at a time
	alt_u32			*shared_mem_int_ptr;	// Use this pointer to read shared_memory FOUR bytes ( ONE int/long ) at a time
	alt_u64			*shared_mem_qword_ptr;	// Use this pointer to read shared_memory EIGHT bytes ( TWO ints/ONE long long ) at a time

	alt_printf("Hello from %s!\n", ALT_CPU_NAME);



// Variable's initializations
	mutex_0 = altera_avalon_mutex_open(MUTEX_0_NAME);	// Get the handle to the hardware mutex

	shared_mem_char_ptr = (alt_u8 *) (SHARED_ONCHIP_BASE);		//
	shared_mem_word_ptr = (alt_u16 *) (SHARED_ONCHIP_BASE);	//
	shared_mem_int_ptr = (alt_u32 *) (SHARED_ONCHIP_BASE);		//
	shared_mem_qword_ptr = (alt_u64 *) (SHARED_ONCHIP_BASE);		//

	// Performance counter initialization
	// Reset Performance Counter
	PERF_RESET(PERFORMANCE_COUNTER_0_BASE);  

	// Start Measuring
	PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
  
// First, lock the mutex
	while( altera_avalon_mutex_trylock( mutex_0, ALT_CPU_CPU_ID_VALUE ) ){}

	alt_printf( "Initializing shared_onchip @ 0x%x ...\n", shared_mem_int_ptr );
// Write a value to the first 1024 (0x400) ints (4096 = 0x1000 bytes) of shared_onchip memory (0x102000 to 0x103fff)
	// Section 1
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
	shared_mem_char_ptr = (alt_u8 *) ( CPU_1_SHARED_ONCHIP_ADDR );
	for( i = 0; i < DATA_BYTES; i++ ){ *( shared_mem_char_ptr + i ) = 0x10; }
	PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
	
	// Section 2
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_2);
	shared_mem_word_ptr = (alt_u16 *) ( CPU_2_SHARED_ONCHIP_ADDR );
	for( i = 0; i < DATA_WORDS; i++ ){ *( shared_mem_word_ptr + i ) = 0x2020; }
	PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_2);
	
	// Section 3
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_3);
	shared_mem_int_ptr = (alt_u32 *) ( CPU_3_SHARED_ONCHIP_ADDR );
	for( i = 0; i < DATA_DWORDS; i++ ){ *( shared_mem_int_ptr + i ) = 0x30303030; }
	PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_3);
	
	// Section 4
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_4);
	shared_mem_qword_ptr = (alt_u64 *) ( CPU_4_SHARED_ONCHIP_ADDR );
	for( i = 0; i < DATA_QWORDS; i++ ){ *( shared_mem_qword_ptr + i ) = 0x4040404040404040; }
	PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_4);
	
	*( shared_mem_char_ptr + CPU_1_OFFSET ) = DATA_NOT_PROCESSED;	// "Flags" for cpu_1
	*( shared_mem_char_ptr + CPU_2_OFFSET ) = DATA_NOT_PROCESSED;	// "Flags" for cpu_2
	*( shared_mem_char_ptr + CPU_3_OFFSET ) = DATA_NOT_PROCESSED;	// "Flags" for cpu_3
	*( shared_mem_char_ptr + CPU_4_OFFSET ) = DATA_NOT_PROCESSED;	// "Flags" for cpu_4

	// End Measuring
	PERF_STOP_MEASURING(PERFORMANCE_COUNTER_0_BASE);

	// Print measurement report
	perf_print_formatted_report( PERFORMANCE_COUNTER_0_BASE,            
		ALT_CPU_FREQ,                 // defined in "system.h"
		4,                            // How many sections to print
		"bytes", "words", "dwords", "qwords" );    // Display-name of section(s).
		
	alt_printf ( "Initialization done!\n" );
// Unlock the mutex
	altera_avalon_mutex_unlock( mutex_0 );



/*
 * Infinite loop
 */
	while( TRUE )
	{
// First, lock the mutex
		while( altera_avalon_mutex_trylock( mutex_0, ALT_CPU_CPU_ID_VALUE ) ){}

// Read the Qsys' "Buttons" peripheral and act accordingly
		switch( ( IORD_ALTERA_AVALON_PIO_DATA( BUTTONS_BASE ) ) & 0xF )	//Read the push-buttons' state (Altera's DE2 board push buttons are active low, that means, that when the switches are read AND NO button is pressed, the returned value will be 0xF)
		{
			case 1:	s[0] = 1; break;
			case 2: s[0] = 2; break;
			case 4: s[0] = 3; break;
			case 8: s[0] = 4; break;
			default:
// Check cpu_1 "Flag" to see if cpu_1 has already processed the data. Display the value in Qsys' leds_red peripheral
				if( *( shared_mem_char_ptr + CPU_1_OFFSET ) == DATA_PROCESSED )
				{
					IOWR_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE, ( IORD_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE ) | 0x3 ) );	// Put the "Flag"'s first char value in the leds_red
				}
// Check cpu_2 "Flag" to see if cpu_1 has already processed the data. Display the value in Qsys' leds_red peripheral
				if( *( shared_mem_char_ptr + CPU_2_OFFSET ) == DATA_PROCESSED )
				{
					IOWR_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE, ( IORD_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE ) | 0x18 ) );	// Put the "Flag"'s first char value in the leds_red
				}
// Check cpu_3 "Flag" to see if cpu_1 has already processed the data. Display the value in Qsys' leds_red peripheral
				if( *( shared_mem_char_ptr + CPU_3_OFFSET ) == DATA_PROCESSED )
				{
					IOWR_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE, ( IORD_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE ) | 0xC0 ) );	// Put the "Flag"'s first char value in the leds_red
				}
// Check cpu_4 "Flag" to see if cpu_1 has already processed the data. Display the value in Qsys' leds_red peripheral
				if( *( shared_mem_char_ptr + CPU_4_OFFSET ) == DATA_PROCESSED )
				{
					IOWR_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE, ( IORD_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE ) | 0x600 ) );	// Put the "Flag"'s first char value in the leds_red
				}
				s[0] = 0;
				break;
		} // switch( ( IORD_ALTERA_AVALON_PIO_DATA( BUTTONS_BASE ) ) & 0xF )



// Ask whether or not to show the shared memory section corresponding to cpu_x
		if( ( s[0] >= 1 ) && ( s[0] <= 4 ) )
		{
			alt_printf("Show contents of shared memory assigned to cpu_%x? (y/n)\n", s[0] );
			i = alt_getchar();
			if( ( i == 0x79 ) || ( i == 0x59 ) )
			{
				alt_printf( "Show contents in bytes, words or ints? (b|w|i)\n" );
				i = alt_getchar();
				switch(s[0])
				{
					case 1: x = CPU_1_OFFSET; break;
					case 2: x = CPU_2_OFFSET; break;
					case 3: x = CPU_3_OFFSET; break;
					case 4: x = CPU_4_OFFSET; break;
					default: break;
				}
				switch(i)
				{
					case 'b':
						shared_mem_char_ptr = (alt_u8 *) ( SHARED_ONCHIP_BASE + x );
						for( i = 0; i < DATA_BYTES; i++ )
						{
							alt_printf("0x%x @ 0x%x\n", *( shared_mem_char_ptr + i ), ( shared_mem_char_ptr + i ) );
						}
						break;
					case 'w':
						shared_mem_word_ptr = (alt_u16 *) ( SHARED_ONCHIP_BASE + x );
						for( i = 0; i < DATA_WORDS; i++ )
						{
							alt_printf("0x%x @ 0x%x\n", *( shared_mem_word_ptr + i ), ( shared_mem_word_ptr + i ) );
						}
						break;
					case 'i':
						shared_mem_int_ptr = (alt_u32 *) ( SHARED_ONCHIP_BASE + x );
						for( i = 0; i < DATA_DWORDS; i++ )
						{
							alt_printf("0x%x @ 0x%x\n", *( shared_mem_int_ptr + i ), ( shared_mem_int_ptr + i ) );
						}
						break;
					default:
						alt_printf( "Neither 'b', 'w' nor 'i' was pressed!!!\n" );
						break;
				} // switch(i)
				alt_printf("Press enter to continue...\n");
				i = alt_getchar();
				alt_printf("0x%x was pressed\n", i);
			} // if( ( i == 0x79 ) || ( i == 0x59 ) )
			s[0] = 0;
		} // if( ( s[0] >= 1 ) && ( s[0] <= 4 ) )



		switch( IORD_ALTERA_AVALON_PIO_DATA( SWITCHES_BASE ) & 0x3e )
		{
			case 0x02:
// Write data to cpu_1's shared_onchip memory section
				shared_mem_int_ptr = (alt_u32 *) ( shared_mem_char_ptr + CPU_1_OFFSET );
				for( i = 0; i < DATA_DWORDS; i++ )
				{
					*(shared_mem_int_ptr + i) = 0xffffffff;	//
				}

				*( shared_mem_char_ptr + CPU_1_OFFSET ) = DATA_NOT_PROCESSED;	// "Flags" for cpu_1
				IOWR_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE, ( IORD_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE ) & 0xfffc ) );
				break;

			case 0x04:
// Write data to cpu_2's shared_onchip memory section
				shared_mem_int_ptr = (alt_u32 *) ( shared_mem_char_ptr + CPU_2_OFFSET );
				for( i = 0; i < DATA_DWORDS; i++ )
				{
					*(shared_mem_int_ptr + i) = 0xffffffff;	//
				}

				*( shared_mem_char_ptr + CPU_2_OFFSET ) = DATA_NOT_PROCESSED;	// "Flags" for cpu_2
				IOWR_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE, ( IORD_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE ) & 0xffe7 ) );
				break;

			case 0x08:
// Write data to cpu_3's shared_onchip memory section
				shared_mem_int_ptr = (alt_u32 *) ( shared_mem_char_ptr + CPU_3_OFFSET );
				for( i = 0; i < DATA_DWORDS; i++ )
				{
					*(shared_mem_int_ptr + i) = 0xffffffff;	//
				}

				*( shared_mem_char_ptr + CPU_3_OFFSET ) = DATA_NOT_PROCESSED;	// "Flags" for cpu_3
				IOWR_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE, ( IORD_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE ) & 0xff3f ) );
				break;

			case 0x10:
// Write data to cpu_4's shared_onchip memory section
				shared_mem_int_ptr = (alt_u32 *) ( shared_mem_char_ptr + CPU_4_OFFSET );
				for( i = 0; i < DATA_DWORDS; i++ )
				{
					*(shared_mem_int_ptr + i) = 0xffffffff;	//
				}

				*( shared_mem_char_ptr + CPU_4_OFFSET ) = DATA_NOT_PROCESSED;	// "Flags" for cpu_4
				IOWR_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE, ( IORD_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE ) & 0xf9ff ) );
				break;

			case 0x20:
				alt_printf( "Input 8 hex digits to fill the shared memory in Least Signigicant digit to Most Significant digit order!\n" );
				alt_printf( "0x" );
				x = 0;
				for( i = 7; i >= 0; i--)
				{
					s[i] = alt_getchar();
					alt_printf( "%x", s[i] );
					if( ( s[i] >= 0x30 ) && ( s[i] <= 0x39 ) )
					{
						s[i] = s[i] - 0x30;
					}else if( ( s[i] >= 0x61 ) && ( s[i] <= 0x66 ) )
					{

						s[i] = s[i] - 0x57;
					}
					x = x | ( s[i] << ( i * 4 ) );
				}
				alt_printf( "Number was: 0x%x\n", x );

				shared_mem_int_ptr = (alt_u32 *) (SHARED_ONCHIP_BASE + CPU_1_OFFSET );
				for( i = 0; i < DATA_DWORDS; i++ )
				{
					*(shared_mem_int_ptr + i) = x;	//
				}
				shared_mem_int_ptr = (alt_u32 *) ( SHARED_ONCHIP_BASE + CPU_2_OFFSET );
				for( i = 0; i < DATA_DWORDS; i++ )
				{
					*(shared_mem_int_ptr + i) = x;	//
				}
				shared_mem_int_ptr = (alt_u32 *) (SHARED_ONCHIP_BASE + CPU_3_OFFSET );
				for( i = 0; i < DATA_DWORDS; i++ )
				{
					*(shared_mem_int_ptr + i) = x;	//
				}
				shared_mem_int_ptr = (alt_u32 *) (SHARED_ONCHIP_BASE + CPU_4_OFFSET );
				for( i = 0; i < DATA_DWORDS; i++ )
				{
					*(shared_mem_int_ptr + i) = x;	//
				}

				*( shared_mem_char_ptr + CPU_1_OFFSET ) = DATA_NOT_PROCESSED;	// "Flags" for cpu_1
				*( shared_mem_char_ptr + CPU_2_OFFSET ) = DATA_NOT_PROCESSED;	// "Flags" for cpu_2
				*( shared_mem_char_ptr + CPU_3_OFFSET ) = DATA_NOT_PROCESSED;	// "Flags" for cpu_3
				*( shared_mem_char_ptr + CPU_4_OFFSET ) = DATA_NOT_PROCESSED;	// "Flags" for cpu_4
				IOWR_ALTERA_AVALON_PIO_DATA( LEDS_RED_BASE, 0 );
				break;

			default:
				break;
		} // switch ( IORD_ALTERA_AVALON_PIO_DATA( SWITCHES_BASE ) & 0xf )

		altera_avalon_mutex_unlock( mutex_0 );

		x = 5;
		delay (x);		// Delay a "random" number of milliseconds (MAX 0x400)
	} //End of while (TRUE)	//Infinite loop
	
	return 0;
}
