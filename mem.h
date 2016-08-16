/**
 * @file      mem.h
 * @brief     Include file for toy kernel memory management
 *
 * A simple memory management code to illustrate basic memory
 * management related kernel APIs.
 *
 * @author    Natarajan Venkataraman, mr.v.natarajan@gmail.com
 * @copyright Copyright (c) 2016, Natarajan Venkataraman
 */

#ifndef _MEM_H_
#define _MEM_H_

void memInit(void *addr, int size);
void *memAlloc(int size);
void memFree(void *addr);

#endif /* _MEM_H_ */
