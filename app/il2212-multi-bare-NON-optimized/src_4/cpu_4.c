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
#define CPU_4_MESSAGE		0x44444444
#define FIFO_ALMOST_FULL_LEVEL	FIFO_0_IN_FIFO_DEPTH - 4
#define FIFO_ALMOST_EMPTY_LEVEL	3

/*---------------------------------------------------------------------------*/
/*		                    MAIN                                     */
/*---------------------------------------------------------------------------*/
int main()
{
  //Variables
  static unsigned char current_image=0;
  unsigned char cropX, cropY;
  int* pointer_max;
  unsigned char sequence_length = 4;
  int sum, max, max_read;
  unsigned char x, y, i, j, l, w, previousX, previousY, offsetX, offsetY;
  unsigned char* pointer_shared;
  const unsigned char xPATTERN[5][5] = {{0,0,1,0,0}, {0,1,0,1,0}, {1,0,0,0,1}, {0,1,0,1,0}, {0,0,1,0,0}};
  unsigned char image_grayed_crop [14][36];
  int fifo_element;


  //Mutex declaration
  alt_mutex_dev* mutex0 = altera_avalon_mutex_open("/dev/mutex_0"); 	//mutex used for access to shared memory

  //FIFOs' initialization
  altera_avalon_fifo_init( FIFO_0_IN_CSR_BASE, 0, FIFO_ALMOST_EMPTY_LEVEL, FIFO_ALMOST_FULL_LEVEL );	//FIFO_0 is used in order to send messages from others to cpu_0	
  altera_avalon_fifo_init( FIFO_0_OUT_CSR_BASE, 0, FIFO_ALMOST_EMPTY_LEVEL, FIFO_ALMOST_FULL_LEVEL );
  altera_avalon_fifo_init( FIFO_1_IN_CSR_BASE, 0, FIFO_ALMOST_EMPTY_LEVEL, FIFO_ALMOST_FULL_LEVEL );	//FIFO_1 is used in order to send messages from cpu_0 to others
  altera_avalon_fifo_init( FIFO_1_OUT_CSR_BASE, 0, FIFO_ALMOST_EMPTY_LEVEL, FIFO_ALMOST_FULL_LEVEL );

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - INTIAL SYNCHRONIZATION - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  // Send message to cpup_0 indicating cpu_1 is ready
  altera_avalon_fifo_write_fifo( FIFO_0_IN_BASE, FIFO_0_IN_CSR_BASE, 0x4 );

	
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
#if DEBUG_FOR_US
	printf("I can start the computation: from cpu_0 I received the message!\n\n");
#endif

	/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - TASK4 - TASK5 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
	//The read operation in the shared memory needs to be protected with the mutex, since in this phase all CPUs can read from it
	altera_avalon_mutex_lock(mutex0,1);

	pointer_shared = SHARED_ONCHIP_BASE;

	
	/*----------------- Copy of the portion of the grayed in its own memory  -----------------*/
	if (current_image==0)
	{
		previousX = 15;
		previousY = 15;
		cropX = 0;
		cropY = 0;
	}

	else {
		pointer_shared = pointer_shared + 2;
		cropX = *pointer_shared++;
		cropY = *pointer_shared++;
		
	}
	
	pointer_shared = SHARED_ONCHIP_BASE + 796;				//It points to the row 22 of the image 36x36

	for(y = 0; y < 14; y++)
	{
		for (x = 0; x < 36; x++)
		{
			image_grayed_crop[y][x]=*pointer_shared++;
		}
	}

	altera_avalon_mutex_unlock(mutex0);	

	/*-------------------------------- Search of the maximum  --------------------------------*/
	max = 0;

	for (i=0; i< 10; i++) {
		for (j=0; j< 32; j++) {
			sum = 0;
			for (l=0; l<5; l++) {
				for (w=0; w<5; w++) {
					sum = sum + (image_grayed_crop[i+l][j+w] * xPATTERN[l][w]);
				}
			}

			if (sum > max) {
				offsetX = j;
				offsetY = i+22;
				max = sum;
			}

		}
	}
	
	pointer_shared = SHARED_ONCHIP_BASE+1323;
	pointer_max = SHARED_ONCHIP_BASE+1313;

	//The write operation in the shared memory needs to be protected with the mutex, since in this phase all CPUs can write on it
	altera_avalon_mutex_lock(mutex0,1);

	*pointer_max = max;
	
	*pointer_shared = offsetX; 
	pointer_shared++;
	*pointer_shared = offsetY;

	altera_avalon_mutex_unlock(mutex0);	

	current_image = current_image + 1;

	/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - SYNCHRONIZATION - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
	altera_avalon_fifo_write_fifo( FIFO_0_IN_BASE, FIFO_0_IN_CSR_BASE, CPU_4_MESSAGE );
    }

    return 0;
}




