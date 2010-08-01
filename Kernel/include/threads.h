/*
 */
#ifndef _THREADS_H_
#define _THREADS_H_

#include <arch.h>
#include <signal.h>
#include <proc.h>

typedef struct sMessage
{
	struct sMessage	*Next;
	Uint	Source;
	Uint	Length;
	Uint8	Data[];
} tMsg;	// sizeof = 12+

typedef struct sThread
{
	// --- threads.c's
	struct sThread	*Next;	//!< Next thread in list
	tSpinlock	IsLocked;	//!< Thread's spinlock
	volatile int	Status;		//!< Thread Status
	 int	RetStatus;	//!< Return Status
	
	Uint	TID;	//!< Thread ID
	Uint	TGID;	//!< Thread Group (Process)
	Uint	PTID;	//!< Parent Thread ID
	Uint	UID, GID;	//!< User and Group
	char	*ThreadName;	//!< Name of thread
	
	// --- arch/proc.c's responsibility
	//! Kernel Stack Base
	tVAddr	KernelStack;
	
	//! Memory Manager State
	tMemoryState	MemState;
	
	//! State on task switch
	tTaskState	SavedState;
	
	// --- threads.c's
	 int	CurFaultNum;	//!< Current fault number, 0: none
	tVAddr	FaultHandler;	//!< Fault Handler
	
	tMsg * volatile	Messages;	//!< Message Queue
	tMsg	*LastMessage;	//!< Last Message (speeds up insertion)
	
	 int	Quantum, Remaining;	//!< Quantum Size and remaining timesteps
	 int	NumTickets;	//!< Priority - Chance of gaining CPU
	
	Uint	Config[NUM_CFG_ENTRIES];	//!< Per-process configuration
	
	volatile int	CurCPU;
} tThread;


enum {
	THREAD_STAT_NULL,
	THREAD_STAT_ACTIVE,
	THREAD_STAT_SLEEPING,
	THREAD_STAT_WAITING,
	THREAD_STAT_ZOMBIE,
	THREAD_STAT_DEAD
};

enum eFaultNumbers
{
	FAULT_MISC,
	FAULT_PAGE,
	FAULT_ACCESS,
	FAULT_DIV0,
	FAULT_OPCODE,
	FAULT_FLOAT
};

#define GETMSG_IGNORE	((void*)-1)

// === FUNCTIONS ===
extern tThread	*Proc_GetCurThread(void);
extern tThread	*Threads_GetThread(Uint TID);
extern void	Threads_SetTickets(tThread *Thread, int Num);
extern int	Threads_Wake(tThread *Thread);
extern void	Threads_AddActive(tThread *Thread);
extern tThread	*Threads_GetNextToRun(int CPU, tThread *Last);

#endif
