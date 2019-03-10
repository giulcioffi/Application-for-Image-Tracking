// Skeleton for lab 2
//
// Task 1 writes periodically RGB-images to the shared memory
//
// No guarantees provided - if bugs are detected, report them in the Issue tracker of the github repository!

#include <stdio.h>
#include "altera_avalon_performance_counter.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "sys/alt_alarm.h"
#include "system.h"
#include "io.h"
#include "ascii_gray.h"

#include "images.h"


/* Definition of User Inputs */
#define DEBUG       		    0

#if DEBUG
	#define NUMBER_OF_TIMES     1
	#define PERFORMANCE	    0
#else
	#define NUMBER_OF_TIMES     10
	#define PERFORMANCE	    1
#endif


/*
 * Global variables
 */
int delay; // Delay of HW-timer
int sizeX, sizeY;
unsigned char* shared_grayed_first;
int previousX = 15;
int previousY = 15;
int cropX, cropY;
int offsetX, offsetY;
int detectX, detectY;
const int xPATTERN[5][5] = {{0,0,1,0,0}, {0,1,0,1,0}, {1,0,0,0,1}, {0,1,0,1,0}, {0,0,1,0,0}};
const int dSPAN = 15;
int CROPSIZE = 36;
int CROPSIZE_4 = 32;
unsigned int cropMatrix[36][36];
int sumMatrix[32][32];
unsigned char grayed_matrix[64][64];
unsigned char CoordinateX[4], CoordinateY[4];



//TASK1
void task1(void)
{
	static int current_image=0;
	unsigned char* base;
	extern unsigned char grayed_matrix[64][64];
	int x, y, r, g, b;
	extern int sizeX, sizeY;


	//Function that makes the image grayed
	base = image_sequence[current_image];

	sizeX = *base++;
	sizeY = *base++;
	base++;
	
	for(y = 0; y < 64; y++)
	{
		for(x = 0; x < 64; x++)
		{
			r = *base++; 	// R
			g = *base++;	// G
			b = *base++;	// B
			grayed_matrix[y][x] = (r*3125)/10000 + (g*5625)/10000 + (b*125)/1000;
		}
	}


	current_image++;
	if(current_image==sequence_length) current_image=0;
}



//TASK2
void task2(void)
{
	extern int previousX;
	extern int previousY;
	extern int sizeX, sizeY;
	extern int cropX, cropY;
	extern const int dSPAN;
	static int current_image=0;


		if (previousX <= dSPAN)
			cropX = 0;
		else if (previousX > sizeX - dSPAN)
			cropX = sizeX - CROPSIZE - 1;
		else
			cropX = previousX - dSPAN -1;


		if (previousY <= dSPAN)
			cropY = 0;
		else if (previousY > sizeY - dSPAN)
			cropY = sizeY - CROPSIZE - 1;
		else
			cropY = previousY - dSPAN -1;

		
		if(current_image==sequence_length) 
		{	current_image=0;
			cropX = 0; cropY = 0;
		}
		current_image++;
}



//TASK3
void task3(void)
{
	extern unsigned char* shared_grayed_first;
	unsigned char* shared_crop_read;
	extern int sizeX, sixeY, cropX, cropY, CROPSIZE;
	int i, j;
	extern unsigned char grayed_matrix[64][64];
	extern unsigned int cropMatrix[36][36];


	for (i=0; i<CROPSIZE; i++)
	{
		for (j=0; j<CROPSIZE; j++)
		{
			cropMatrix[i][j] = (int) grayed_matrix[i+cropY][j+cropX];
		}
	}		

}



//TASK4
void task4(void)
{
	int i, j, l, w;
	extern int CROPSIZE;
	extern int CROPSIZE_4;
	extern unsigned int cropMatrix [36][36];
	extern int sumMatrix [32][32];
	extern const int xPATTERN[5][5];
	int prodMatrix[5][5];
	int sum;


	for (i=0; i< CROPSIZE_4; i++) {
		for (j=0; j< CROPSIZE_4; j++) {
			for (l=0; l<5; l++) {
				for (w=0; w<5; w++) {
					prodMatrix[l][w] = cropMatrix[i+l][j+w] * xPATTERN[l][w];
				}
			}

			sum = 0;
			for (l=0; l<5; l++) {
				for (w=0; w<5; w++) {
					sum = sum + prodMatrix[l][w];
				}
			}

			sumMatrix[i][j] = sum;

		}
	}
}



//TASK5
void task5(void)
{
	extern int CROPSIZE_4;
	extern int sumMatrix[32][32];
	extern int offsetX, offsetY;
	int i, j, max;


	max = 0;

	for (i=0; i<CROPSIZE_4; i++) {
		for (j=0; j<CROPSIZE_4; j++) {
			if (sumMatrix[i][j] > max) {
				offsetX = j;
				offsetY = i;
				max = sumMatrix[i][j];
			}
		}
	}

}



//TASK6
void task6(void)
{
	extern int cropX, cropY, offsetX, offsetY;
	extern int previousX, previousY, sizeX, sizeY;
	int detectX_2, detectY_2;
	static int i=0;
	extern unsigned char CoordinateX[4], CoordinateY[4];


	previousX = cropX + offsetX;
	previousY = cropY + offsetY;

	detectX_2 = detectX + 2;
	detectY_2 = detectY + 2;

	CoordinateX[i]= previousX; 
	CoordinateY[i]= previousY;
	i++;
	if(i==sequence_length) i=0;
}



int main(void)
{

  int count=0;
  int p;
  int i = 0;
  extern unsigned char CoordinateX[4], CoordinateY[4];


    while (count<NUMBER_OF_TIMES*(sequence_length))
    {
#if PERFORMANCE	
    	/* Start Measurement here */
	if(count==0)
	{
		PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
		PERF_START_MEASURING(PERFORMANCE_COUNTER_0_BASE);
		PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, 1);
	}
#endif

	/*---------- Task 1 -----------*/
	task1();
	/*---------- Task 2 -----------*/
	task2();
	/*---------- Task 3 -----------*/
	task3();
	/*---------- Task 4 -----------*/
	task4();
	/*---------- Task 5 -----------*/
	task5();
	/*---------- Task 6 -----------*/
	task6();


	if(count== NUMBER_OF_TIMES*(sequence_length)-1)
	{
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
		for (p=0; p< sequence_length ; p++)
		{
			printf("X(%d)= %d ", p, CoordinateX[p]);
			printf("Y(%d)= %d\n\n\n", p, CoordinateY[p]);
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

	}

	count = count + 1;
     }

     return 0;
}
