#include <stdio.h>
#include "altera_avalon_performance_counter.h"
#include "includes.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "sys/alt_alarm.h"
#include "system.h"
#include "io.h"
#include "ascii_gray.h"

#include "images.h"

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];
OS_STK    task3_stk[TASK_STACKSIZE];
OS_STK    task4_stk[TASK_STACKSIZE];
OS_STK    task5_stk[TASK_STACKSIZE];
OS_STK    task6_stk[TASK_STACKSIZE];
OS_STK    task7_stk[TASK_STACKSIZE];
OS_STK    StartTask_Stack[TASK_STACKSIZE];

/* Definition of Task Priorities */
#define STARTTASK_PRIO      1
#define TASK1_PRIORITY      5
#define TASK2_PRIORITY      8
#define TASK3_PRIORITY      10
#define TASK4_PRIORITY      12
#define TASK5_PRIORITY      14
#define TASK6_PRIORITY      16

/* Definition of User Inputs */
#define DEBUG             	    1

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
unsigned char sizeX, sizeY, maxcol;
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


// Semaphores
OS_EVENT *Task1Sem;
OS_EVENT *Task2Sem;
OS_EVENT *Task3Sem;
OS_EVENT *Task4Sem;
OS_EVENT *Task5Sem;
OS_EVENT *Task6Sem;


//TASK1
void task1(void* pdata)
{
	INT8U err;
	INT8U value=0;
	INT8U value1=0;
	INT8U current_image=0;
	int count=0;
	unsigned char* base;
	extern unsigned char grayed_matrix[64][64];
	extern unsigned char CoordinateX[4], CoordinateY[4];
	int i = 0;
	int p;
	int x, y, r, g, b;
	extern unsigned char sizeX, sizeY;
	int max_col;


	while (count< (NUMBER_OF_TIMES*sequence_length) )
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

		//Function that makes the image grayed
		base = image_sequence[current_image];

		sizeX = *base++;
		sizeY = *base++;
		max_col = *base++;
		
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

		OSSemPost(Task1Sem);

		OSSemPend(Task6Sem, 0, &err);



		
		if((count== NUMBER_OF_TIMES*(sequence_length)-1))
		{
#if PERFORMANCE		/* End Measuring */
			PERF_END(PERFORMANCE_COUNTER_0_BASE, 1);
			PERF_STOP_MEASURING(PERFORMANCE_COUNTER_0_BASE);

			//Print report
			printf("End of performance measurements\n");
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


		/* Increment the image pointer */
		current_image=(current_image+1) % sequence_length;
		if (current_image == 0) {
			previousX = 15;
			previousY = 15;
		}
		count = count + 1;

	}
}



//TASK2
void task2(void* pdata)
{
	INT8U err;
	INT8U value=0;
	extern int previousX;
	extern int previousY;
	extern unsigned char sizeX, sizeY;
	extern int cropX, cropY;
	extern const int dSPAN;

	while (1)
	{
		OSSemPend(Task1Sem, 0, &err);

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

		OSSemPost(Task2Sem);

	}
}



//TASK3
void task3(void* pdata)
{
	INT8U err;
	INT8U value=0;
	extern unsigned char grayed_matrix[64][64];
	extern unsigned char sizeX, sizeY;
	extern int cropX, cropY, CROPSIZE;
	int i, j;
	extern unsigned int cropMatrix[36][36];


	while (1)
	{
		OSSemPend(Task2Sem, 0, &err);

		for (i=0; i<CROPSIZE; i++)
		{
			for (j=0; j<CROPSIZE; j++)
			{
				cropMatrix[i][j] = (int) grayed_matrix[i+cropY][j+cropX];
			}
		}		

		OSSemPost(Task3Sem);

	}
}



//TASK4
void task4(void* pdata)
{
	INT8U err;
	INT8U value = 0;
	int i, j, l, w;
	extern int CROPSIZE;
	extern int CROPSIZE_4;
	extern unsigned int cropMatrix [36][36];
	extern int sumMatrix [32][32];
	extern const int xPATTERN[5][5];
	int prodMatrix[5][5];
	int sum;


	while (1)
	{
		OSSemPend(Task3Sem, 0, &err);

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

		OSSemPost(Task4Sem);

	}
}



//TASK5
void task5(void* pdata)
{
	INT8U err;
	extern int CROPSIZE_4;
	extern int sumMatrix[32][32];
	extern int offsetX, offsetY;
	int i, j, max;

	while (1)
	{
		OSSemPend(Task4Sem, 0, &err);

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

		OSSemPost(Task5Sem);

	}
}



//TASK6
void task6(void* pdata)
{
	INT8U err;
	extern int cropX, cropY, offsetX, offsetY;
	extern unsigned char sizeX, sizeY;
	int detectX_2, detectY_2;
	extern unsigned char CoordinateX[4], CoordinateY[4];
	extern int previousX, previousY;  	
    	int i = 0;

	while (1)
	{
		OSSemPend(Task5Sem, 0, &err);

		previousX = cropX + offsetX;
		previousY = cropY + offsetY;

		detectX_2 = detectX + 2;
		detectY_2 = detectY + 2;

		CoordinateX[i]= previousX;
		CoordinateY[i]= previousY;
		i++;
		if(i==sequence_length) i=0;

		OSSemPost(Task6Sem);

	}
}



void StartTask(void* pdata)
{
  INT8U err;
  void* context;

   /*
   * Creation of Kernel Objects
   */

  Task1Sem = OSSemCreate(0);
  Task2Sem = OSSemCreate(0);
  Task3Sem = OSSemCreate(0);
  Task4Sem = OSSemCreate(0);
  Task5Sem = OSSemCreate(0);
  Task6Sem = OSSemCreate(0);

  /*
   * Create statistics task
   */

  OSStatInit();

  /*
   * Creating Tasks in the system
   */

  /*---------- Task 1 -----------*/
  err=OSTaskCreateExt(task1,
                  NULL,
                  (void *)&task1_stk[TASK_STACKSIZE-1],
                  TASK1_PRIORITY,
                  TASK1_PRIORITY,
                  task1_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  /*---------- Task 2 -----------*/
  err=OSTaskCreateExt(task2,
                  NULL,
                  (void *)&task2_stk[TASK_STACKSIZE-1],
                  TASK2_PRIORITY,
                  TASK2_PRIORITY,
                  task2_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);


  /*---------- Task 3 -----------*/
  err=OSTaskCreateExt(task3,
                  NULL,
                  (void *)&task3_stk[TASK_STACKSIZE-1],
                  TASK3_PRIORITY,
                  TASK3_PRIORITY,
                  task3_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);


  /*---------- Task 4 -----------*/
  err=OSTaskCreateExt(task4,
                  NULL,
                  (void *)&task4_stk[TASK_STACKSIZE-1],
                  TASK4_PRIORITY,
                  TASK4_PRIORITY,
                  task4_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);


  /*---------- Task 5 -----------*/
  err=OSTaskCreateExt(task5,
                  NULL,
                  (void *)&task5_stk[TASK_STACKSIZE-1],
                  TASK5_PRIORITY,
                  TASK5_PRIORITY,
                  task5_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);


  /*---------- Task 6 -----------*/
  err=OSTaskCreateExt(task6,
                  NULL,
                  (void *)&task6_stk[TASK_STACKSIZE-1],
                  TASK6_PRIORITY,
                  TASK6_PRIORITY,
                  task6_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);


  /* Task deletes itself */

  OSTaskDel(OS_PRIO_SELF);
}


int main(void) {

#if DEBUG
  printf("MicroC/OS-II-Vesion: %1.2f\n", (double) OSVersion()/100.0);
#endif

  //Creation of the 'StartTask' used to create all the tasks
  OSTaskCreateExt(
	 StartTask, // Pointer to task code
	 NULL,      // Pointer to argument that is
				// passed to task
	 (void *)&StartTask_Stack[TASK_STACKSIZE-1], // Pointer to top
						 // of task stack
	 STARTTASK_PRIO,
	 STARTTASK_PRIO,
	 (void *)&StartTask_Stack[0],	//Pointer to bottom of stack
	 TASK_STACKSIZE,
	 (void *) 0,
	 OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

  OSStart();

  return 0;
}
