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

/*---------- Performance counter ----------*/
#include "altera_avalon_performance_counter.h"

/*--------- Application functions ---------*/
#include "ascii_gray.h"
extern void delay (int millisec);

/*----------- Application input -----------*/
#include "images.h"
#include "../common_objs.h"



/*---------------------------------------------------------------------------*/
/*			       GLOBAL VARIABLES			             */
/*---------------------------------------------------------------------------*/
#define IMG_DONE_MSG		0x69696969
#define CPU_1_MESSAGE		0x11111111
#define CPU_2_MESSAGE		0x22222222
#define CPU_3_MESSAGE		0x33333333
#define CPU_4_MESSAGE		0x44444444
#define FIFO_ALMOST_FULL_LEVEL	FIFO_0_IN_FIFO_DEPTH - 4
#define FIFO_ALMOST_EMPTY_LEVEL	3
unsigned char CoordinateX[4], CoordinateY[4];
unsigned char i, p;
unsigned char sizeX, sizeY, cropX, cropY;


/*---------------------------------------------------------------------------*/
/*		  		FUNCTIONS			             */
/*---------------------------------------------------------------------------*/

// Functions that saves in the on-chip memory the grayed 31x31 image
 void read_image(unsigned char* base){
	
	unsigned char x, y;
	int r, g, b;
	extern unsigned char sizeX, sizeY;	
	unsigned char* shared;
	extern unsigned char cropX, cropY;

	/*----------------- Read from SRAM information about size of the image  -----------------*/
	sizeX = *base++;						//read sizeX of image
	sizeY = *base++;						//read sizeY of image
	*base++;							//maxcol
	base = base + cropY*sizeX*3 + cropX*3;				//point to the rgb image to crop
	
	
	/*---------------- Write on ON-CHIP MEMORY the grayed and cropped image ---------------- */
	shared = SHARED_ONCHIP_BASE + 4;
	
	for(y = 0; y < 36; y++)
	{
		for (x = 0; x < 36; x++)
		{
			r = *base++;
			g = *base++;
			b = *base++;
			//Computation just with shifts and addition				
			*shared++= (r*3125)/10000 + (g*5625)/10000 + (b*125)/1000; //(0.3125r) + (0.5625g) + (0.125b)
		}
		base = base + sizeX*3 - 108;				//new line of the cropped image
	}
}
	


/*---------------------------------------------------------------------------*/
/*		                    MAIN                                     */
/*---------------------------------------------------------------------------*/
int main()
{
  // Variables
  static unsigned char current_image=0;
  extern unsigned char sizeX, sizeY, cropX, cropY;
  int sum, max, max_read;
  unsigned char i, previousX, previousY, offsetX, offsetY, coordX, coordY;
  unsigned char* pointer_shared;
  int* pointer_max;
  const unsigned char xPATTERN[5][5] = {{0,0,1,0,0}, {0,1,0,1,0}, {1,0,0,0,1}, {0,1,0,1,0}, {0,0,1,0,0}};
  int fifo_element;  
  unsigned char count=0;
  extern unsigned char CoordinateX[4], CoordinateY[4];


  // Mutex declaration
  alt_mutex_dev* mutex0 = altera_avalon_mutex_open("/dev/mutex_0"); 	//mutex used for access to shared memory

  // FIFOs' initialization
  altera_avalon_fifo_init( FIFO_0_IN_CSR_BASE, 0, FIFO_ALMOST_EMPTY_LEVEL, FIFO_ALMOST_FULL_LEVEL );	//FIFO_0 is used in order to send messages from others to cpu_0	
  altera_avalon_fifo_init( FIFO_0_OUT_CSR_BASE, 0, FIFO_ALMOST_EMPTY_LEVEL, FIFO_ALMOST_FULL_LEVEL );
  altera_avalon_fifo_init( FIFO_1_IN_CSR_BASE, 0, FIFO_ALMOST_EMPTY_LEVEL, FIFO_ALMOST_FULL_LEVEL ); 	//FIFO_1 is used in order to send messages from cpu_0 to others
  altera_avalon_fifo_init( FIFO_1_OUT_CSR_BASE, 0, FIFO_ALMOST_EMPTY_LEVEL, FIFO_ALMOST_FULL_LEVEL );


  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - INTIAL SYNCHRONIZATION - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#if DEBUG_FOR_US
  alt_printf("Waiting for all CPUs to be ready...\n");
#endif

  // Empty FIFOs from previously stored elements
  while ( altera_avalon_fifo_read_status( FIFO_0_OUT_CSR_BASE, ALTERA_AVALON_FIFO_STATUS_E_MSK )!= 0x02 ) // Read FIFO_0 until it becomes empty
  {
  	  fifo_element = altera_avalon_fifo_read_fifo( FIFO_0_OUT_BASE, FIFO_0_OUT_CSR_BASE );
  }
  while ( altera_avalon_fifo_read_status( FIFO_1_OUT_CSR_BASE, ALTERA_AVALON_FIFO_STATUS_E_MSK )!= 0x02 ) // Read FIFO_1 until it becomes empty
  {
  	  fifo_element = altera_avalon_fifo_read_fifo( FIFO_1_OUT_BASE, FIFO_1_OUT_CSR_BASE );
  }

  // Wait until ALL four CPUs have written to the FIFO. WARNING: This depends on the "almostempty" initialization value which is set to 3. With this value, when 4 elements have been written to the FIFO, the   "almost empty" flag will be cleared
  while ( altera_avalon_fifo_read_status( FIFO_0_OUT_CSR_BASE, ALTERA_AVALON_FIFO_STATUS_AE_MSK ) == 0x08 ){}
  
  // Read all messages to double check they are from cpu_1 through cpu_4
  for( i = 0; i < 4; i++ ) 
  {	// Read ONLY four messages. With the while we can read ALL messages until FIFO buffer is empty
  	fifo_element = altera_avalon_fifo_read_fifo( FIFO_0_OUT_BASE, FIFO_0_OUT_CSR_BASE );
#if DEBUG_FOR_US
  	switch( fifo_element )
  	{
		case 0x1:	alt_printf( "cpu_1 is ready!!!\n" ); break;
		case 0x2:	alt_printf( "cpu_2 is ready!!!\n" ); break;
		case 0x3:	alt_printf( "cpu_3 is ready!!!\n" ); break;
		case 0x4:	alt_printf( "cpu_4 is ready!!!\n" ); break;
		default:	alt_printf( "Something weird happened!!! Message received was: %x\n", fifo_element ); break;

	}
#endif
  }

#if DEBUG_FOR_US
  alt_printf("ALL CPUs are ready!!!\n");
#endif

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - START PERFORMANCE MEASUREMENTS - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#if PERFORMANCE
  PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
  PERF_START_MEASURING(PERFORMANCE_COUNTER_0_BASE);
  PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, 1);
#endif

	
  while (count<(NUMBER_OF_TIMES*sequence_length))
  {
	/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - TASK2 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

	if (current_image==0)
	{
		previousX = 15;
		previousY = 15;
		cropX = 0;
		cropY = 0;
	}
	else {
		if (previousX <= 15)
			cropX = 0;
		else if (previousX > sizeX - 15)
			cropX = sizeX - 32;
		else
			cropX = previousX - 16;


		if (previousY <= 15)
			cropY = 0;
		else if (previousY > sizeY - 15)
			cropY = sizeY - 32;
		else
			cropY = previousY - 16;
	}


	/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - TASK1- TASK3 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

	altera_avalon_mutex_lock(mutex0,1);			      	//it wants to write for a while the shared resource
	read_image(image_sequence[current_image]);			//Function that reads a portion of image from SRAM and grayes it	
	altera_avalon_mutex_unlock(mutex0);				//it finishes to write for a while the shared resource


	/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - SYNCHRONIZATION - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
	// Send message to FIFO_0 for the other CPUs: they can start to read the image and working on it 
	altera_avalon_fifo_write_fifo( FIFO_1_IN_BASE, FIFO_1_IN_CSR_BASE, IMG_DONE_MSG );
	altera_avalon_fifo_write_fifo( FIFO_1_IN_BASE, FIFO_1_IN_CSR_BASE, IMG_DONE_MSG );
	altera_avalon_fifo_write_fifo( FIFO_1_IN_BASE, FIFO_1_IN_CSR_BASE, IMG_DONE_MSG );
	altera_avalon_fifo_write_fifo( FIFO_1_IN_BASE, FIFO_1_IN_CSR_BASE, IMG_DONE_MSG );


	// Wait until ALL four CPUs have written to the FIFO: the image has been processed, than cpu_0 can change image
	while ( altera_avalon_fifo_read_status( FIFO_0_OUT_CSR_BASE, ALTERA_AVALON_FIFO_STATUS_AE_MSK ) == 0x08 ){}


 	// Read all messages to double check they are from cpu_1 through cpu_4 and for get empty the FIF0_0
  	for( i = 0; i < 4; i++ ) 
 	{	// Read ONLY four messages. With the while we can read ALL messages until FIFO buffer is empty
  		fifo_element = altera_avalon_fifo_read_fifo( FIFO_0_OUT_BASE, FIFO_0_OUT_CSR_BASE );
#if DEBUG_FOR_US
  		switch( fifo_element )
  		{
			case CPU_1_MESSAGE:	alt_printf( "cpu_1 finished!!!\n" ); break;
			case CPU_2_MESSAGE:	alt_printf( "cpu_2 finished!!!\n" ); break;
			case CPU_3_MESSAGE:	alt_printf( "cpu_3 finished!!!\n" ); break;
			case CPU_4_MESSAGE:	alt_printf( "cpu_4 finished!!!\n" ); break;
			default:		alt_printf( "Something weird happened!!! Message received was: %x\n", fifo_element ); break;

		}
#endif
  	}

#if DEBUG_FOR_US
  	alt_printf("ALL CPUs have finished to compute the local maximum!!!\n");
#endif

	/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - TASK6 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

	/*--------- Search for the absolute maximum ---------*/
	// The read operation in the shared memory needs to be protected with the mutex
	altera_avalon_mutex_lock(mutex0,1);

	// Read maximum from cpu1
	pointer_max = SHARED_ONCHIP_BASE + 1301;
	pointer_shared = SHARED_ONCHIP_BASE + 1317;
	max = *pointer_max;
	offsetX = *pointer_shared++;
	offsetY = *pointer_shared++;

	// Comparison with max found by cpu_2
	pointer_max++;
	max_read = *pointer_max;
	if (max_read > max) {
		max = max_read;
		offsetX = *pointer_shared++;
		offsetY = *pointer_shared++;
	}
	else
		pointer_shared = pointer_shared + 2;

	// Comparison with max found by cpu_3
	pointer_max++;
	max_read = *pointer_max;
	if (max_read > max) {
		max = max_read;
		offsetX = *pointer_shared++;
		offsetY = *pointer_shared++;
	}
	else
		pointer_shared = pointer_shared + 2;


	// Comparison with max found by cpu_4
	pointer_max++;
	max_read = *pointer_max;
	if (max_read > max) {
		max = max_read;
		offsetX = *pointer_shared++;
		offsetY = *pointer_shared++;
	}
	else
		pointer_shared = pointer_shared + 2;

	altera_avalon_mutex_unlock(mutex0);


	/*--------- Update coordinates ---------*/
	//Update coordinates to use for next image
	previousX = cropX + offsetX;
	previousY = cropY + offsetY;
	
	// The write operation in the shared memory needs to be protected with the mutex
	CoordinateX[current_image] = previousX;
	CoordinateY[current_image] = previousY;
	
	current_image++;
	if(current_image == sequence_length) current_image=0;
	count++;

   }


   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - END PERFORMANCE MEASUREMENTS - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#if PERFORMANCE	
	/* End Measuring */
	PERF_END(PERFORMANCE_COUNTER_0_BASE, 1);
	PERF_STOP_MEASURING(PERFORMANCE_COUNTER_0_BASE);

	//Print report
	perf_print_formatted_report(PERFORMANCE_COUNTER_0_BASE, ALT_CPU_FREQ, 1, "Total time");
#endif
	
	printf("----------------------------------\n\n");
	printf("------ LIST OF COORDINATES  ------\n\n");
	printf("----------------------------------\n\n");
	for (i=0; i< sequence_length ; i++)
	{
		printf("X(%d)= %d ", i, CoordinateX[i]);
		printf("Y(%d)= %d\n\n\n", i, CoordinateY[i]);
	}


#if DEBUG 		
	printf("----------------------------------\n\n");
	printf("------------- IMAGES -------------\n\n");
	printf("----------------------------------\n\n");
	// Print of the images with and without pattern
	for (i=0 ; i< sequence_length; i++)
	{ 	
		printf("------------- Image %d -------------\n\n",i);
		//Image with pattern
		gray_displayAscii(image_sequence[i]);
		
		printf("------------- Image %d without pattern -------------\n\n",i);
		//Image without pattern
		gray_displayAsciiHidden(image_sequence[i],CoordinateX[i],CoordinateY[i]);
	}
#endif

   return 0;
}
