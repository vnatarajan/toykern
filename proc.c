/**
 * @file      proc.c
 * @brief     Process management for toy kernel
 *
 * A simple process management code to illustrate basic process
 * management related kernel APIs and scheduling.
 * Coded for x86-64 64-bit mode.
 *
 * @author    Natarajan Venkataraman, mr.v.natarajan@gmail.com
 * @copyright Copyright (c) 2016, Natarajan Venkataraman
 */

#include <proc.h>
#include <mem.h>
#include <stdint.h>
#include <unistd.h>

#define	STACKSZ	(128 * 1024)		/* Size of process stack */
/* Magic# to recognize a PCB in the memory. */
#define	MAGIC_PROC	0x50524F43	/* 'PROC' */

typedef enum {
	READY = 0,
	RUNNING,
	SLEEPING,
	WAITING
} procState_t;

/* Process control block (PCB) */
typedef struct proc_ {
	struct proc_	*next;
	uint32_t	magic;	/* Magic# for PCB */
	int		pid;	/* Process ID */
	procState_t	state;	/* Process state */
	char	*stackAddr;	/* Address of stack assigned to process */
	void	*resumeAddr;	/* Address to resume process execution from */
	/* Registers */
	char	*stackPtr;	/* Stack Pointer */
	char	*framePtr;	/* Frame Pointer */
} pcb_t;

static void sched(void);

int procId = 0;			/* Counter used to generate process identifer */
/* TODO: We are using a PID generation that is too simplistic and will
 * wrap-over if too many processes are created. Need to replace with a
 * correct implementation.
 */

pcb_t	*readyQ = NULL;		/* Queue of ready to run processes */
pcb_t	*readyQEnd = NULL;	/* End of readyQ */
pcb_t	*runningProc = NULL;	/* Process that is currently running */

/**
 * @brief
 * Initialize the process management subsystem and create the first
 * process of the system. This process must exist forever and spawn
 * other processes.
 *
 * @param[in]
 *       None.
 *
 * @param[out]
 *       None.
 *
 * @return
 *       - None.
 */
void
procInit(void)
{
	pcb_t	*proc;
	char	*stack;

	readyQ = NULL;
	readyQEnd = NULL;
	runningProc = NULL;
	procId = 0;

	/* Make the invoking code as the 'first' or 'init' process. */
	proc = memAlloc(sizeof(pcb_t));
	if (proc == NULL) {
		return;
	}

	__asm__("mov %%rsp, %0"
		: "=r" (stack));

	proc->magic = MAGIC_PROC;
	proc->pid = procId++;
	proc->state = READY;
	proc->stackAddr = NULL;
	proc->stackPtr = stack;

	runningProc = proc;
	return;
}

/**
 * @brief
 * API to create a new process
 *
 * @param[in]
 *       start: Pointer to start address of code for new process.
 *
 * @param[out]
 *       None.
 *
 * @return
 *       - Success : Process ID of new process
 *       - Failure : -1
 */
int
procCreate(procStart_t start)
{
	pcb_t	*proc;
	char	*stack;

	proc = memAlloc(sizeof(pcb_t));
	if (proc == NULL) {
		return (-1);
	}

	stack = memAlloc(STACKSZ);
	if (stack == NULL) {
		memFree(proc);
		return (-1);
	}

	proc->pid = procId++;
	proc->state = READY;
	proc->stackAddr = stack;
	proc->stackPtr = stack + STACKSZ - sizeof(procStart_t);
	* (procStart_t *) proc->stackPtr = start;
	proc->stackPtr -= sizeof(void *);	/* Dummy EBP */

	/* Put process into ready list */
	proc->next = readyQ;
	readyQ = proc;
	if (proc->next == NULL) {
		readyQEnd = proc;
	}

	/* Run the scheduler */
	sched();

	return (proc->pid);
}

/**
 * @brief
 * API to delete a process
 *
 * @param[in]
 *       pid: Process ID of process to delete.
 *
 * @param[out]
 *       None.
 *
 * @return
 *       - Success : 0
 *       - Failure : -1
 */
int
procDelete(int pid)
{
	pcb_t	*proc;
	pcb_t	*prevProc;

	/* Remove proc from readyQ */
	proc = readyQ;
	prevProc = NULL;
	while (proc) {
		if (proc->pid == pid) break;
		prevProc = proc;
		proc = proc->next;
	}
	if (proc) {
		if (proc == readyQ) {
			readyQ = proc->next;
			if (readyQ == NULL) {
				readyQEnd = NULL;
			}
		} else if (proc == readyQEnd) {
			readyQEnd = prevProc;
			prevProc->next = NULL;
		} else {
			prevProc->next = proc->next;
		}
		/* Free the memory allocated for process management */
		memFree(proc->stackAddr);
		memFree(proc);
	} else if (runningProc->pid == pid) {
		runningProc = NULL;
	} else {
		/* Must not happen !! */
		/* When we implement more states, then we need
		 * to deal with removing process from other queues.
		 */
	}
	sched();
	return 0;
}

/**
 * @brief
 * API to yield control so another process can run
 *
 * @param[in]
 *       None.
 *
 * @param[out]
 *       None.
 *
 * @return
 *       - None.
 */
void
procYield(void)
{
	sched();
}

/**
 * @brief
 * The scheduler.
 *
 * @param[in]
 *       iparam: Description of iparam
 *
 * @param[out]
 *       oparam: Description of oparam
 *
 * @return
 *       - Success return value
 *       - Failure return value
 */
static void
sched(void)
{
	pcb_t	*proc, *oldProc;
	void	*oldStackPtr, *newStackPtr;

	proc = readyQ;
	if (proc == NULL) {
		/* Nothing to schedule. Continue with current process. */
		return;
	}

	oldProc = runningProc;

	/* Dequeue process from readyQ */
	readyQ = proc->next;
	if (readyQ == NULL) readyQEnd = NULL;

	/* Put current running proc into readyQ */
	if (readyQ == NULL) {
		readyQ = readyQEnd = runningProc;
	} else {
		readyQEnd->next = runningProc;
		readyQEnd = runningProc;
	}

	newStackPtr = proc->stackPtr;
	runningProc = proc;
	runningProc->next = NULL;

	/* Switch stack-pointer to stack of newly running process */
	// Tricky business. Do the following:
	// Old-Proc:
	// 	stackPtr = Register-RSP; (64-bit)
	//
	__asm__ ("mov %%rsp, %0"
		 : "=r" (oldStackPtr));
	// New-Proc:
	//	Register-RSP = stackPtr; (64-bit)
	//	return;			/* This would do "pop ebp; ret" */
	__asm__ ("mov %0, %%rsp"
		 :
		 : "r" (newStackPtr));
	if (oldProc) {
		oldProc->stackPtr = oldStackPtr;
	}

	return;
}
