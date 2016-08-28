/**
 * @file      proc.h
 * @brief     Include file for toy kernel process management
 *
 * A simple process management code to illustrate basic process
 * management related kernel APIs.
 *
 * @author    Natarajan Venkataraman, mr.v.natarajan@gmail.com
 * @copyright Copyright (c) 2016, Natarajan Venkataraman
 */

#ifndef _PROC_H_
#define _PROC_H_

/* Process start function template */
typedef int (*procStart_t) (void);

extern void procInit(void);
extern int procCreate(procStart_t start);
extern int procDelete(int pid);
extern void procYield(void);

#endif /* _PROC_H_ */
