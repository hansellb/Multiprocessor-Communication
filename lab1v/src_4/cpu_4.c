/*
 * cpu_4
 * - accesses the shared memory FOUR bytes at a time
 * - make a local copy of the shared memory data (FOUR bytes at a time)
 * - process the data (locally) - decreases the value by a "random" value and add 0x44444444 to the value of each INT
 * - after all data is zero, set the flag to DATA_PROCESSED
 * - copy the processed data back to the shared memory (FOUR bytes at a time)
 * - wait 40 milliseconds
 */
#include "../inc/common_objs.h"

extern void delay (int millisec);



/*
 * Main
 */
int main()
{
// Variable's declarations
	alt_u16			i, j;	//general loop variables
	alt_u32			x, y;	//general purpose variables

	alt_u8			data_was_updated;	// Indicates whether or not the last processed data was updated back to the shared memory

	alt_mutex_dev	*mutex_0;	// Handle to the mutex hardware

	alt_u8			char_array[DATA_BYTES];	// Shared memory data's local copy

	alt_u8			*shared_mem_char_ptr;	// Use this pointer to read shared_memory ONE byte ( ONE char ) at a time
	//alt_u16			*shared_mem_word_ptr;	// Use this pointer to read shared_memory TWO byte ( ONE word ) at a time
	alt_u32			*shared_mem_int_ptr;	// Use this pointer to read shared_memory FOUR bytes ( ONE int ) at a time

	alt_u32			*local_mem_int_ptr;	// Auxiliary pointer to word

	alt_printf("Hello from %s!\n", ALT_CPU_NAME);

// Variable's initialization
	mutex_0 = altera_avalon_mutex_open(MUTEX_0_NAME);	// Get the handle to the hardware mutex

	shared_mem_char_ptr = (alt_u8 *) ( SHARED_ONCHIP_BASE + CPU_4_OFFSET );		//
	//shared_mem_word_ptr = (alt_u16 *) ( SHARED_ONCHIP_BASE + CPU_4_OFFSET );	//
	shared_mem_int_ptr = (alt_u32 *) ( SHARED_ONCHIP_BASE + CPU_4_OFFSET );		//

	local_mem_int_ptr = (alt_u32 *) char_array;			// Typecast the "pointer to char" to a "pointer to int"

	data_was_updated = 0;



/*
 * Infinite Loop
 */
	while( TRUE )
	{
// First, lock the mutex
		while( altera_avalon_mutex_trylock( mutex_0, ALT_CPU_CPU_ID_VALUE ) ){}

// Check the flag to see if data has already been processed and updated in shared memory
		if( ( *( shared_mem_char_ptr ) == DATA_NOT_PROCESSED ) )
		{
// Copy the shared data to the local variable.
			alt_printf( "Cloning data from 0x%x to 0x%x...\n", ( shared_mem_int_ptr ), char_array );
			alt_printf( "Data @ 0x%x: 0x%x\n", ( shared_mem_int_ptr ), *( shared_mem_int_ptr ) );
			for( i = 0; i < DATA_DWORDS; i++ )
			{
				*( local_mem_int_ptr + i ) = *( shared_mem_int_ptr + i );
			}
			data_was_updated = 0;
		}
		altera_avalon_mutex_unlock( mutex_0 );




// Process the data LOCALLY
		if ( !data_was_updated )
		{
			alt_printf( "Processing data from 0x%x to 0x%x using 0x%x size blocks...\n", char_array, ( char_array + ( DATA_DWORDS * sizeof(*local_mem_int_ptr) )  ), sizeof( *local_mem_int_ptr ) );
			alt_printf( "Substracting 0x160b47a0 and then Adding 0x44444444 to ALL data...\n" );
			*( local_mem_int_ptr ) &= 0x000000FF;		// Processes the three BYTES after the flag
			for( i = 1; i < DATA_DWORDS; i++ )
			{
				//alt_printf ( "i: 0x%x\n", i );
				//x = time(NULL);
				x = 0x12345678;
				for( j = 0; j < 0xff; j++ )
				{
					x = ((x * 7621) + 1) % 32768;			// Generate a "random" number. Linear Congruential Generator
				}
				x %= 0xffff;						//
				x <<= 16;

				//y = ( time(NULL) / ( i + 1 ) );
				y = 0x87654321;
				for( j = 0; j < 0xff; j++ )
				{
					y = ((y * 7621) + 1) % 32768;			// Generate a "random" number. Linear Congruential Generator
				}
				x = x | y;

				*( local_mem_int_ptr + i ) -= x;
				( *( local_mem_int_ptr + i ) ) += 0x44444444;
			}
			char_array[0] = DATA_PROCESSED;

// Update the shared memory with the LOCALLY processed data
			alt_printf( "Updating the shared memory content...\n" );
// First, lock the mutex
			while( altera_avalon_mutex_trylock( mutex_0, ALT_CPU_CPU_ID_VALUE ) ){}
			for( i = 0; i < DATA_DWORDS; i++ )
			{
				*( shared_mem_int_ptr + i ) = *( local_mem_int_ptr + i );
			}
			data_was_updated = 1;
			alt_printf( "Shared memory updated successfully!!!\n" );
			altera_avalon_mutex_unlock( mutex_0 );
		}
		x = 40;
		delay (x);		// Delay a "random" number of milliseconds (MAX 0x400)
	}	// while( TRUE )

	return 0;
}
