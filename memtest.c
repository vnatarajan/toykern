/**
 * @file      memtest.c
 * @brief     Unit test for toy kernel memory management.
 *
 * Test out toy kernel memory management.
 *
 * @author    Natarajan Venkataraman, mr.v.natarajan@gmail.com
 * @copyright Copyright (c) 2016, Natarajan Venkataraman
 */

#include <mem.h>
#include <stdlib.h>
#include <unistd.h>

char space[1*1024*1024];

int
main(void)
{
	srandom(getpid());
	{
		void *ptr[4] = {0};

		memInit(space, 610+(32*3)); /* 32 is sizeof(mcb_t) */
		ptr[0] = memAlloc(100);
		ptr[1] = memAlloc(200);
		ptr[2] = memAlloc(300);
		ptr[3] = memAlloc(30); // Alloc must fail.
		memFree(ptr[0]);
		memFree(ptr[2]);
		memFree(ptr[1]);
		memFree(ptr[3]);
	}
	{
		int i, sz;
		void *ptr[10] = {0};

		memInit(space, sizeof(space));
		for(i=0; i<10; i++) {
			do {
				sz = (random() % 100);
			} while(sz == 0);
			ptr[i] = memAlloc(sz);
		}
		for(i=10; i>0; i--) {
			memFree(ptr[i-1]);
		}
	}
	{
		int i, sz, idx;
		void *ptr[1000] = {0};

		memInit(space, sizeof(space));
		for(i=0; i<100000; i++) {
			idx = random() % 1000;
			if (ptr[idx] == 0) {
				sz = random() % 10000;
				ptr[idx] = memAlloc(sz);
			} else {
				memFree(ptr[idx]);
				ptr[idx] = 0;
			}
		}
	}
}
