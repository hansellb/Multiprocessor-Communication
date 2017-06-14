/*
 * cpu_1
 * - accesses the shared memory ONE byte at a time
 * - make a local copy of the shared memory data (ONE byte at a time)
 * - process the data (locally) - add 0x11 to the value of each BYTE
 * - after all data is zero, set the flag to DATA_PROCESSED
 * - copy the processed data back to the shared memory (ONE byte at a time)
 * - wait 10 milliseconds
 */
#include "../inc/common_objs.h"

extern void delay (int millisec);



/*
 * Main
 */
int main()
{
// Variable's declarations
	alt_u16			i;	// General loop variable
	alt_u32			x;	// General purpose variables

	alt_u8			data_was_updated;	// Indicates whether or not the last processed data was updated back to the shared memory

	alt_mutex_dev	*mutex_0;	// Handle to the mutex hardware

	alt_u8			char_array[DATA_BYTES];	// Shared memory data's local copy

	alt_u8			*shared_mem_char_ptr;	// Use this pointer to read shared_memory ONE byte ( ONE char ) at a time
	//alt_u16			*shared_mem_word_ptr;	// Use this pointer to read shared_memory TWO byte ( ONE word ) at a time
	//alt_u32			*shared_mem_int_ptr;	// Use this pointer to read shared_memory FOUR bytes ( ONE int ) at a time

	alt_u8			*local_mem_char_ptr;	// Auxiliary pointer to byte

	alt_printf("Hello from %s!\n", ALT_CPU_NAME);

// Variable's initialization
	mutex_0 = altera_avalon_mutex_open(MUTEX_0_NAME);	// Get the handle to the hardware mutex

	shared_mem_char_ptr = (alt_u8 *) ( SHARED_ONCHIP_BASE + CPU_1_OFFSET );		//
	//shared_mem_word_ptr = (alt_u16 *) ( SHARED_ONCHIP_BASE + CPU_1_OFFSET );	//
	//shared_mem_int_ptr = (alt_u32 *) ( SHARED_ONCHIP_BASE + CPU_1_OFFSET );		//

	local_mem_char_ptr = (alt_u8 *) char_array;			// Typecast the "pointer to char" to a "pointer to word"

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
			alt_printf( "Cloning data from 0x%x to 0x%x...\n", ( shared_mem_char_ptr ), char_array );
			alt_printf( "Data @ 0x%x: 0x%x\n", ( shared_mem_char_ptr ), *( shared_mem_char_ptr ) );
			for( i = 0; i < DATA_BYTES; i++ )
			{
				char_array[i] = *( shared_mem_char_ptr + i );
			}
			data_was_updated = 0;
		}
		altera_avalon_mutex_unlock( mutex_0 );



// Process the data LOCALLY
		if ( !data_was_updated )
		{
			alt_printf( "Processing data from 0x%x to 0x%x using 0x%x size blocks...\n", char_array, ( char_array + DATA_BYTES ), sizeof( *char_array ) );
			alt_printf( "Adding 0x11 to ALL data...\n" );
			for( x = 0; x < 100; x++ )
			{
				for( i = 1; i < DATA_BYTES; i++ )
				{
					( *( local_mem_char_ptr + i ) ) += 0x11;
				}	
			}
			char_array[0] = DATA_PROCESSED;

// Update the shared memory with the LOCALLY processed data
			alt_printf( "Updating the shared memory content...\n" );
// First, lock the mutex
			while( altera_avalon_mutex_trylock( mutex_0, ALT_CPU_CPU_ID_VALUE ) ){}
			for( i = 0; i < DATA_BYTES; i++ )
			{
				*( shared_mem_char_ptr + i ) = char_array[i];
			}
			data_was_updated = 1;
			alt_printf( "Shared memory updated successfully!!!\n" );
			altera_avalon_mutex_unlock( mutex_0 );
		}
		x = 10;
		delay (x);		// Delay a "random" number of milliseconds (MAX 0x400)
	}	// while( TRUE )

	return 0;
}
