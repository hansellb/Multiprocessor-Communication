/*disp3*/
#include <stdio.h>
#include <altera_avalon_mutex.h>
#include <system.h>
#include <altera_avalon_fifo.h>
#include <altera_avalon_fifo_regs.h>
#include <altera_avalon_fifo_util.h>

int main()
{
	int *flag,i=0,j=2,*mem_cpu[8];
	alt_mutex_dev* mutex,*fifo_mutex;
	flag= (int*)(SHARED_ONCHIP_BASE+0x20) ;
	mem_cpu[0] = (int*)(SHARED_ONCHIP_BASE+0x00) ;
	mem_cpu[1] = (int*)(SHARED_ONCHIP_BASE+0x04) ;
	mem_cpu[2] = (int*)(SHARED_ONCHIP_BASE+0x08) ;
	mem_cpu[3] = (int*)(SHARED_ONCHIP_BASE+0x0C) ;
	mem_cpu[4] = (int*)(SHARED_ONCHIP_BASE+0x10) ;
	mem_cpu[5] = (int*)(SHARED_ONCHIP_BASE+0x14) ;
	mem_cpu[6] = (int*)(SHARED_ONCHIP_BASE+0x18) ;
	mem_cpu[7] = (int*)(SHARED_ONCHIP_BASE+0x1C) ;

	/* get the mutex device handle */
	fifo_mutex = altera_avalon_mutex_open( MUTEX_1_NAME );
 	/* acquire the mutex, setting the value to one */
 	altera_avalon_mutex_lock( fifo_mutex, 1 );
 	/* writing to FIFO */
	altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE,FIFO_0_IN_CSR_BASE,0xE00);
	/* Unlocking mutex */
	altera_avalon_mutex_unlock( fifo_mutex );
	while(1)
    	{
  		/* get the mutex device handle */
  	  	mutex = altera_avalon_mutex_open( MUTEX_0_NAME );
  	  	/* acquire the mutex, setting the value to one */
  	  	altera_avalon_mutex_lock( mutex, 1 );
  		if(j>0)						/* To ensure that each processor writes to the shared ram only twice*/
  	  	{
  	  		while (*mem_cpu[i] != 0x00 && i<8 )	/* parses through shared memory to find an empty one */
  	  		{
	  			i++;
  	  		}

  	  		*mem_cpu[i]=  0x03;			/* Writes 3 to memory to indicate location acquired */
  	  		*flag= *flag | (0x01 << i);		/* sets flag indicating position acquired */
  			j--;
    		}
   		/* release the lock */
  	  	altera_avalon_mutex_unlock( mutex );
    	}

  	return 0;
}
