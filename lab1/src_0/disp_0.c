/*disp0*/

#include <stdio.h>
#include <altera_avalon_mutex.h>
#include <altera_avalon_pio_regs.h>
#include <altera_avalon_fifo.h>
#include <altera_avalon_fifo_regs.h>
#include <altera_avalon_fifo_util.h>
#include <altera_avalon_performance_counter.h>

#define SECTION_1 1
#define SECTION_2 2
/*initializing values to be passed to the 7-segment display*/
static int hex_val[] = {
		0x3F ,//0
		0x06 ,//1
		0x5B ,//2
		0x4F ,//3
		0x66 ,//4
		0x6D ,//5
		0x7D ,//6
		0x07 ,//7
		0x7F ,//8
		0x67 ,//9
		0x77 ,//A
		0x7C ,//B
		0x39 ,//C
		0x5E ,//D
		0x79 ,//E
		0x71 //F
		};
int main()
{
	alt_mutex_dev* mutex,*fifo_mutex;
	int out_0,out_4,i,printed=0,read_fifo=0;
	int *mem_cpu[8],read_cpu[8];
	int *flag;

	mem_cpu[0] = (int*)(SHARED_ONCHIP_BASE+0x00) ;
	mem_cpu[1] = (int*)(SHARED_ONCHIP_BASE+0x04) ;
	mem_cpu[2] = (int*)(SHARED_ONCHIP_BASE+0x08) ;
	mem_cpu[3] = (int*)(SHARED_ONCHIP_BASE+0x0C) ;
	mem_cpu[4] = (int*)(SHARED_ONCHIP_BASE+0x10) ;
	mem_cpu[5] = (int*)(SHARED_ONCHIP_BASE+0x14) ;
	mem_cpu[6] = (int*)(SHARED_ONCHIP_BASE+0x18) ;
	mem_cpu[7] = (int*)(SHARED_ONCHIP_BASE+0x1C) ;
	flag= (int*)(SHARED_ONCHIP_BASE+0x20) ;

	/* initialising FIFO*/
	altera_avalon_fifo_init(FIFO_0_IN_CSR_BASE,0,2,10);
	altera_avalon_fifo_init(FIFO_0_OUT_CSR_BASE,0,2,10);

	/* performance measurement*/
	PERF_RESET(PERFORMANCE_COUNTER_0_BASE);

	/* Start Measuring */
	PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);

	/* Section 1 for performance measurement*/
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);

	/*opening and locking mutex*/
	mutex = altera_avalon_mutex_open( MUTEX_0_NAME );
	altera_avalon_mutex_lock( mutex, 1 );

	/*initializing shared memory and flag*/
	*mem_cpu[0] = 0x00;
	*mem_cpu[1] = 0x00;
	*mem_cpu[2] = 0x00;
	*mem_cpu[3] = 0x00;
	*mem_cpu[4] = 0x00;
	*mem_cpu[5] = 0x00;
	*mem_cpu[6] = 0x00;
	*mem_cpu[7] = 0x00;
	*flag=0x00;

	/*unlocking mutex*/
	altera_avalon_mutex_unlock( mutex );

	/*ending section for performance counter*/
	PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);

	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_2);

	while(1)
  	{
	/* reading contents of FIFO */

		fifo_mutex = altera_avalon_mutex_open( MUTEX_1_NAME );
		/* acquire the mutex, setting the value to one */
		altera_avalon_mutex_lock( fifo_mutex, 1 );
		while (altera_avalon_fifo_read_status(FIFO_0_OUT_CSR_BASE,ALTERA_AVALON_FIFO_STATUS_E_MSK)!=2)
		{
			 read_fifo= altera_avalon_fifo_read_fifo(FIFO_0_OUT_BASE,FIFO_0_OUT_CSR_BASE);
			 IOWR_32DIRECT(LEDS_RED_BASE,0x00,read_fifo);
		}
		altera_avalon_mutex_unlock( fifo_mutex );
		/* end of read FIFO*/

	if (printed == 0)
	{
		/* get the mutex device handle */
	  	mutex = altera_avalon_mutex_open( MUTEX_0_NAME );
	  	/* acquire the mutex, setting the value to one */
	  	altera_avalon_mutex_lock( mutex, 1 );
        	/*  To print the results stored by other CPU's once all of them write to their designated addresses*/
	  	if(*flag == 0xFF)		/* to check if all the 8 places have been occupied */
	  	{
			for(i=0;i<8;i++)	/* parses through shared memory to find an empty one */
	  		{
	  			read_cpu[i]= int2seven(*mem_cpu[i]);	/*convert int to seven segment display format */
	  		}
	  		/*combining the values to be displayed on the 7-seg display*/
	  		out_0 = read_cpu[0] << 24 |
				read_cpu[1] << 16 |
			 	read_cpu[2]<< 8  |
			 	read_cpu[3];
	  	 	out_4 = read_cpu[4] << 24 |
				read_cpu[5] << 16 |
			  	read_cpu[6]<< 8  |
			  	read_cpu[7];
	  		/* Writing values to 7-seg display*/
	  	 	IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_BASE,out_0);
	  	 	IOWR_ALTERA_AVALON_PIO_DATA(HEX7_HEX4_BASE,out_4);
	  	 	/* unlocking the mutex*/
	  	 	altera_avalon_mutex_unlock( mutex );
	  	 	/* flag to ensure that each processor occupies only 2 spaces from the memory*/
	  		printed=1;
	  		/* ending section for performance counter*/
	  		PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_2);
			PERF_STOP_MEASURING(PERFORMANCE_COUNTER_0_BASE);
			/* Printing measurement report */
			perf_print_formatted_report(
				PERFORMANCE_COUNTER_0_BASE,
	  			ALT_CPU_FREQ,                 // defined in "system.h"
	  			2,                            // How many sections to print
	  			"shared_mem_init", "rest of the program");    // Display-name of section(s).

	  		printf("\n");
	  	}
		/* unlocking the mutex if control comes here after aquiring 2 chairs*/
		altera_avalon_mutex_unlock( mutex );
	}


  }
  return 0;
}
/* function to return equivalent value to be written to the 7-segment display */
int int2seven(int inval)
{
	return hex_val[inval];
}
