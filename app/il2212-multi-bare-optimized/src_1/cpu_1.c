/*---------------------------------------------------------------------------*/
/*			    FILES TO BE INCLUDED		             */
/*---------------------------------------------------------------------------*/
/*---------------- General ----------------*/
#include "alt_types.h"
#include <stdio.h>
#include "system.h"
#include "io.h"

/*--------------- Mutex -------------------*/
#include "altera_avalon_mutex.h"

/*------- System Clock & Timestamp --------*/
#include "sys/alt_alarm.h"
#include "sys/alt_timestamp.h"
#include "sys/alt_irq.h"

/*---------------- Fifo -------------------*/
#include "altera_avalon_fifo_regs.h"
#include "altera_avalon_fifo_util.h"
#include "altera_avalon_fifo.h"

/*--------- Application functions ---------*/
extern void delay (int millisec);

/*----------- Application input -----------*/
#include "../common_objs.h"


/*---------------------------------------------------------------------------*/
/*			       GLOBAL VARIABLES			             */
/*---------------------------------------------------------------------------*/
#define IMG_DONE_MSG		0x69696969
#define CPU_1_MESSAGE		0x11111111
#define FIFO_ALMOST_FULL_LEVEL	FIFO_0_IN_FIFO_DEPTH - 4
#define FIFO_ALMOST_EMPTY_LEVEL	3


/*---------------------------------------------------------------------------*/
/*		                    MAIN                                     */
/*---------------------------------------------------------------------------*/
int main()
{
  //Variables
  static unsigned char current_image=0;
  unsigned char cropX3;
  unsigned char sequence_length = 4;
  int* pointer_max;
  int sum, max;
  unsigned char r, g, b, x, y, i, j, l, w, previousX, previousY, offsetX, offsetY, not_found;
  unsigned char* pointer_shared;
  int image_grayed_crop [17][36];
  int fifo_element, int_read; 
  int* pointer_int;

  //Mutex declaration
  alt_mutex_dev* mutex0 = altera_avalon_mutex_open("/dev/mutex_0"); 	//mutex used for access to shared memory
  alt_mutex_dev* mutex1 = altera_avalon_mutex_open("/dev/mutex_1");

  //FIFOs' initialization
  altera_avalon_fifo_init( FIFO_0_IN_CSR_BASE, 0, FIFO_ALMOST_EMPTY_LEVEL, FIFO_ALMOST_FULL_LEVEL );	//FIFO_0 is used in order to send messages from others to cpu_0	
  altera_avalon_fifo_init( FIFO_0_OUT_CSR_BASE, 0, FIFO_ALMOST_EMPTY_LEVEL, FIFO_ALMOST_FULL_LEVEL );
  altera_avalon_fifo_init( FIFO_1_IN_CSR_BASE, 0, FIFO_ALMOST_EMPTY_LEVEL, FIFO_ALMOST_FULL_LEVEL );	//FIFO_1 is used in order to send messages from cpu_0 to others
  altera_avalon_fifo_init( FIFO_1_OUT_CSR_BASE, 0, FIFO_ALMOST_EMPTY_LEVEL, FIFO_ALMOST_FULL_LEVEL );

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - INTIAL SYNCHRONIZATION - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  // Send message to cpup_0 indicating cpu_1 is ready
  altera_avalon_fifo_write_fifo( FIFO_0_IN_BASE, FIFO_0_IN_CSR_BASE, 0x1 );

	
  while (current_image < sequence_length)
  {
	/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - SYNCHRONIZATION - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
	// Wait until cpu_0 has written to the FIFO: four messages for the four CPUs have been sent
	while ( altera_avalon_fifo_read_status( FIFO_1_OUT_CSR_BASE, ALTERA_AVALON_FIFO_STATUS_AE_MSK ) == 0x08 ){}

	// Check for "new image available" messages from cpu_0. If there are any messages, read one
	fifo_element = 0;
	while ( fifo_element == 0 )
	{
		fifo_element = altera_avalon_fifo_read_fifo( FIFO_1_OUT_BASE, FIFO_1_OUT_CSR_BASE );
	}

	not_found = 1;

	altera_avalon_mutex_lock(mutex1,1);

#if DEBUG_FOR_US
	printf("I can start the computation: from cpu_0 I received the message!\n\n");
#endif


	/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - TASK4 - TASK5 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/	
	//The read operation in the shared memory needs to be protected with the mutex, since in this phase all CPUs can read from it
	//altera_avalon_mutex_lock(mutex0,1);

	
	/*----------------- Copy of the portion of the grayed in its own memory  -----------------*/
	pointer_shared = SHARED_ONCHIP_BASE;
	cropX3 = *pointer_shared;
	pointer_shared = SHARED_ONCHIP_BASE + 4;			//It points to the row 0 of the image 36x64*3
	pointer_shared = pointer_shared + cropX3;
	


	for(y = 0; y < 17; y++)
	{
		 		
		for (x = 0; x < 36; x++)
		{
			r = *pointer_shared++;
			g = *pointer_shared++;
			b = *pointer_shared++;
			//Computation just with shifts and addition				
			image_grayed_crop[y][x] = (r>>2) + (r>>4) + (g>>1) + (g>>4) + (b>>3); //(0.25r+0.0625r) + (0.5g+0.0625g) + (0.125b)
		}
		pointer_shared = pointer_shared + 84;			//new line of the cropped image
					
	}


	//altera_avalon_mutex_unlock(mutex0);
	
	/*-------------------------------- Search of the maximum  --------------------------------*/
	
	for (i=0; i< 13; i++) {
		for (j=0; j< 34; j++) {
			if (image_grayed_crop[i][j]>200) {
				if (image_grayed_crop[i+1][j-1]>200 && image_grayed_crop[i+2][j-2]>200 && image_grayed_crop[i+1][j+1]>200 && image_grayed_crop[i+2][j+2]>200 && image_grayed_crop[i+3][j-1]>200 && image_grayed_crop[i+3][j+1]>200 && image_grayed_crop[i+4][j]>200) {
					offsetX = j-2;
					offsetY = i;
					not_found = 0;
					break;
				}
			}

		}
	}
	

	//The write operation in the shared memory needs to be protected with the mutex, since in this phase all CPUs can write on it
	//altera_avalon_mutex_lock(mutex0,1);

	if (not_found==0) {
		pointer_shared = SHARED_ONCHIP_BASE+7333;
		*pointer_shared++ = offsetX; 
		*pointer_shared = offsetY;
	}

		
	//altera_avalon_mutex_unlock(mutex0);

 	
	current_image = current_image + 1;

	altera_avalon_mutex_unlock(mutex1);

    }

    return 0;
}
