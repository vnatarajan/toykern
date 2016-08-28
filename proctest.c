/**
 * @file      proctest.c
 * @brief     Unit test for toy kernel process management.
 *
 * Test out toy kernel process management.
 *
 * @author    Natarajan Venkataraman, mr.v.natarajan@gmail.com
 * @copyright Copyright (c) 2016, Natarajan Venkataraman
 */

#include <mem.h>
#include <proc.h>
#include <stdio.h>

char space[1*1024*1024];

int p1Pid, p2Pid;

extern int process2 (void);

int
process1 (void)
{
	int i;

	p2Pid = procCreate(process2);

	for (i=0; i<10; i+=2) {
		printf("Process-1: %d\n", i);
		procYield();
		printf("Process-1: %d\n", i+1);
		procYield();
	}
	procDelete(p1Pid);
	return 0;
}

int
process2 (void)
{
	printf("Process-2: %d\n", 10);
	procYield();
	printf("Process-2: %d\n", 9);
	procYield();
	printf("Process-2: %d\n", 8);
	procYield();
	printf("Process-2: %d\n", 7);
	procYield();
	printf("Process-2: %d\n", 6);
	procYield();
	printf("Process-2: %d\n", 5);
	procYield();
	printf("Process-2: %d\n", 4);
	procYield();
	printf("Process-2: %d\n", 3);
	procYield();
	printf("Process-2: %d\n", 2);
	procYield();
	printf("Process-2: %d\n", 1);
	procYield();

	procDelete(p2Pid);
	return 0;
}

int
main(void)
{
	memInit(space, sizeof(space));

	procInit();
	p1Pid = procCreate(process1);
	for(;;) {
		/* TODO: We need to implement waiting for process
		 * exit, based on which we can exit instead of being
		 * in an infinite loop.
		 */
		//printf("Init proc\n");
		procYield();
	}
	return 0;
}
