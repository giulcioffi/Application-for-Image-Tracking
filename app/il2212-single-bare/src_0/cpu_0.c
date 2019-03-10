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


#define SECTION_1 1

/*
 * Example function for copying a p3 image from sram to the shared on-chip mempry
 */
void sram2sm_p3(unsigned char* base)
{
	int x, y;
	unsigned char* shared = SHARED_ONCHIP_BASE;


	int size_x = *base++;
	int size_y = *base++;
	int max_col= *base++;
	*shared++  = size_x;
	*shared++  = size_y;
	*shared++  = max_col;
	for(y = 0; y < size_y; y++)
	for(x = 0; x < size_x; x++)
	{
		*shared++ = *base++; 	// R
		*shared++ = *base++;	// G
		*shared++ = *base++;	// B
	}
}


//Function that reduces the (3*x)*y image (r,g,b) into a x*y image
void grayed ()
{
	unsigned char* shared = SHARED_ONCHIP_BASE;
	int x,y,r,g,b;
	extern int sizeX;
	extern int sizeY;
	extern unsigned char* shared_grayed_first;
	unsigned char* shared_grayed;
	extern int maxcol;
	extern int DEBUG;

	sizeX = *shared++;
	sizeY = *shared++;
	maxcol = *shared++;


	if(DEBUG) printf("------ GRAYED IMAGE READ  ------\n\n");
	shared_grayed_first = shared;
	shared_grayed = shared_grayed_first;

	for (y=0; y<sizeY; y++) {
		for (x=0; x<sizeX; x++) {
			r=*shared++;
			g=*shared++;
			b=*shared++;
			*shared_grayed=(r*3125)/10000 + (g*5625)/10000 + (b*125)/1000;
			shared_grayed++;
		}
	}
	if(DEBUG) printAscii(shared_grayed_first, sizeX, sizeY);
}




/*
 * Global variables
 */
int delay; // Delay of HW-timer
int sizeX, sizeY, maxcol;
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
int DEBUG;
int* Coordinates;


//TASK1
void task1(void)
{
	static int current_image=0;
	unsigned long long time;


		//Function that reads an image from SRAM
		sram2sm_p3(image_sequence[current_image]);

		//Function that makes the image grayed
		grayed();

		/* Increment the image pointer */
		if (current_image == 0) {
			previousX = 15;
			previousY = 15;
		}
		current_image=(current_image+1) % sequence_length;
}



//TASK2
void task2(void)
{
	extern int previousX;
	extern int previousY;
	extern int sizeX, sizeY;
	extern int cropX, cropY;
	extern const int dSPAN;


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

}



//TASK3
void task3(void)
{
	extern unsigned char* shared_grayed_first;
	unsigned char* shared_crop_read;
	extern int sizeX, sixeY, cropX, cropY, CROPSIZE;
	int i, j;
	extern unsigned int cropMatrix[36][36];


		shared_crop_read = shared_grayed_first + cropY*sizeX + cropX;

		for (i=0; i<CROPSIZE; i++) {
			for (j=0; j<CROPSIZE; j++) {
				cropMatrix[i][j] = *shared_crop_read++;
			}
			shared_crop_read = shared_crop_read + sizeX - CROPSIZE;
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
	extern int detectX, detectY, sizeX, sizeY;
	int detectX_2, detectY_2;
	extern unsigned char* shared_grayed_first;
	extern int DEBUG;


		detectX = cropX + offsetX;
		detectY = cropY + offsetY;

		detectX_2 = detectX + 2;
		detectY_2 = detectY + 2;

		//printAscii(shared_grayed_first, sizeX, sizeY);
		if(DEBUG)
		{
			printf("------ GRAYED IMAGE PATCHED  ------\n\n");
			printAsciiHidden(shared_grayed_first, sizeX, sizeY,detectX_2, detectY_2, 2, 2);
		}
}



//TASK7
void task7(void)
{
	extern int detectX, detectY;
	extern int previousX, previousY;
	extern int* Coordinates;
	static int i = 0;

		previousX = detectX;
		previousY = detectY;
		Coordinates[i]= detectX;
		i++;
		Coordinates[i]= detectY;
		i++;
		if(i==2*sequence_length) i=0;

		if(DEBUG)
		{
			printf("------ESTIMATED COORDINATES  ------\n\n");
			printf("previousX = %d, previousY = %d.\n\n", previousX, previousY);
		}
}


int main(void) {

  int count=0;
  extern int DEBUG;
  int p;
  int i = 0;


    //Choice of the execution mode
    printf("Insert the execution mode:\n [0] Performance Mode\n [1] Debug Mode\n ");
    scanf("%d", &DEBUG);
    printf("\n\n");

    while ((count < sequence_length)&&(DEBUG) || (count<10*(sequence_length))&&(DEBUG==0))
    {
    	/* Start Measurement here */
		if((count==0)&&(DEBUG==0))
		{
			printf("Starting performance measurements\n\n");
			PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
			PERF_START_MEASURING(PERFORMANCE_COUNTER_0_BASE);
			PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
		}

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
		/*---------- Task 7 -----------*/
		task7();

		/* End Measuring */
		if((count== 10*(sequence_length) - 1)&&(DEBUG==0))
		{
			PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
			PERF_STOP_MEASURING(PERFORMANCE_COUNTER_0_BASE);
			//Print report
			printf("End of performance measurements\n");
			perf_print_formatted_report(PERFORMANCE_COUNTER_0_BASE, ALT_CPU_FREQ, 1, "Section 1");
		}

		if((count== sequence_length - 1)&&(DEBUG))
		{
			printf("------ LIST OF COORDINATES  ------\n\n");
			for (p=0; p< sequence_length ; p++)
			{
				printf("X(%d)= %d ", i, Coordinates[i]);
				i++;
				printf("Y(%d)= %d\n", i, Coordinates[i]);
				i++;
			}
			i = 0;
		}

		count = count + 1;

    }

    return 0;
}
