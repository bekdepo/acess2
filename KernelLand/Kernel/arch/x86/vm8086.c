/*
 * Acess2 Kernel (x86)
 * - By John Hodge (thePowersGang)
 *
 * vm8086.c
 * - Virtual 8086 Mode Monitor
 */
#define DEBUG	0
#include <acess.h>
#include <vm8086.h>
#include <modules.h>
#include <hal_proc.h>
#include <semaphore.h>

// === CONSTANTS ===
#define TRACE_EMU	0

#define VM8086_USER_BASE	0x1000

#define VM8086_MAGIC_CS	0xFFFF
#define VM8086_MAGIC_IP	0x0010
#define VM8086_STACK_SEG	0x9F00
#define VM8086_STACK_OFS	0x0AFE
enum eVM8086_Opcodes
{
	VM8086_OP_PUSHF   = 0x9C,
	VM8086_OP_POPF    = 0x9D,
	VM8086_OP_INT_I   = 0xCD,
	VM8086_OP_IRET    = 0xCF,
	VM8086_OP_IN_AD   = 0xEC,
	VM8086_OP_IN_ADX  = 0xED,
	VM8086_OP_OUT_AD  = 0xEE,
	VM8086_OP_OUT_ADX = 0xEF
};
#define VM8086_PAGES_PER_INST	4

#define VM8086_BLOCKSIZE	128
#define VM8086_BLOCKCOUNT	((0x9F000-0x10000)/VM8086_BLOCKSIZE)

// === TYPES ===
struct sVM8086_InternalPages
{
	Uint32	Bitmap;	// 32 sections = 128 byte blocks
	char	*VirtBase;
	tPAddr	PhysAddr;
};
struct sVM8086_InternalData
{
	struct sVM8086_InternalPages	AllocatedPages[VM8086_PAGES_PER_INST];
};

// === PROTOTYPES ===
 int	VM8086_Install(char **Arguments);
void	VM8086_GPF(tRegs *Regs);
//tVM8086	*VM8086_Init(void);

// === GLOBALS ===
MODULE_DEFINE(0, 0x100, VM8086, VM8086_Install, NULL, NULL);
tMutex	glVM8086_Process;
tSemaphore	gVM8086_TaskComplete;
tSemaphore	gVM8086_TasksToDo;
tPID	gVM8086_WorkerPID;
tTID	gVM8086_CallingThread;
tVM8086	volatile * volatile gpVM8086_State = (void*)-1;	// Set to -1 to avoid race conditions
Uint32	gaVM8086_MemBitmap[VM8086_BLOCKCOUNT/32];
 int	gbVM8086_ShadowIF = 0;

// === FUNCTIONS ===
int VM8086_Install(char **Arguments)
{
	Semaphore_Init(&gVM8086_TasksToDo, 0, 10, "VM8086", "TasksToDo");
	
	// Lock to avoid race conditions
	Mutex_Acquire( &glVM8086_Process );
	
	// Create BIOS Call process
	tPID pid = Proc_Clone(CLONE_VM);
	LOG("pid = %i", pid);
	if(pid == -1)
	{
		Log_Error("VM8086", "Unable to clone kernel into VM8086 worker");
		return MODULE_ERR_MISC;
	}
	if(pid == 0)
	{
		Uint	* volatile stacksetup;	// Initialising Stack
		Uint16	* volatile rmstack;	// Real Mode Stack

		LOG("Initialising worker");
	
		// Set Image Name
		Threads_SetName("VM8086");

		// Map ROM Area
		for(unsigned int i = 0xA0;i<0x100;i++) {
			MM_RefPhys(i * 0x1000);
			MM_Map( (void*)(i * 0x1000), i * 0x1000 );
		}
		MM_RefPhys(0);
		MM_Map( (void*)0, 0 );	// IVT / BDA
		if( MM_GetRefCount(0x00000) > 2 ) {
			Log_Notice("VM8086", "Ok, who's touched the IVT? (%i)",
				MM_GetRefCount(0x00000));
		}
		MM_RefPhys(0x9F000);
		MM_Map( (void*)0x9F000, 0x9F000 );	// Stack / EBDA
		if( MM_GetRefCount(0x9F000) > 2 ) {
			Log_Notice("VM8086", "And who's been playing with my EBDA? (%i)",
				MM_GetRefCount(0x9F000));
		}
		// System Stack / Stub
		if( MM_Allocate( (void*)0x100000 ) == 0 ) {
			Log_Error("VM8086", "Unable to allocate memory for stack/stub");
			gVM8086_WorkerPID = 0;
			Threads_Exit(0, 1);
		}
		
		*(Uint8*)(0x100000) = VM8086_OP_IRET;
		*(Uint8*)(0x100001) = 0x07;	// POP ES
		*(Uint8*)(0x100002) = 0x1F;	// POP DS
		*(Uint8*)(0x100003) = 0xCB;	// RET FAR
		
		rmstack = (Uint16*)(VM8086_STACK_SEG*16 + VM8086_STACK_OFS);
		rmstack--;	*rmstack = 0xFFFF;	//CS
		rmstack--;	*rmstack = 0x0010;	//IP
		
		// Setup Stack
		stacksetup = (Uint*)0x101000;
		stacksetup--;	*stacksetup = VM8086_STACK_SEG;	// GS
		stacksetup--;	*stacksetup = VM8086_STACK_SEG;	// FS
		stacksetup--;	*stacksetup = VM8086_STACK_SEG;	// DS
		stacksetup--;	*stacksetup = VM8086_STACK_SEG;	// ES
		stacksetup--;	*stacksetup = VM8086_STACK_SEG;	// SS
		stacksetup--;	*stacksetup = VM8086_STACK_OFS-2;	// SP
		stacksetup--;	*stacksetup = 0x20202;	// FLAGS
		stacksetup--;	*stacksetup = 0xFFFF;	// CS
		stacksetup--;	*stacksetup = 0x10;	// IP
		stacksetup--;	*stacksetup = 0xAAAA;	// AX
		stacksetup--;	*stacksetup = 0xCCCC;	// CX
		stacksetup--;	*stacksetup = 0xDDDD;	// DX
		stacksetup--;	*stacksetup = 0xBBBB;	// BX
		stacksetup--;	*stacksetup = 0x5454;	// SP
		stacksetup--;	*stacksetup = 0xB4B4;	// BP
		stacksetup--;	*stacksetup = 0x5151;	// SI
		stacksetup--;	*stacksetup = 0xD1D1;	// DI
		stacksetup--;	*stacksetup = 0x20|3;	// DS - Kernel
		stacksetup--;	*stacksetup = 0x20|3;	// ES - Kernel
		stacksetup--;	*stacksetup = 0x20|3;	// FS
		stacksetup--;	*stacksetup = 0x20|3;	// GS
		LOG("stacksetup = %p, entering vm8086");
		__asm__ __volatile__ (
		"mov %%eax,%%esp;\n\t"	// Set stack pointer
		"pop %%gs;\n\t"
		"pop %%fs;\n\t"
		"pop %%es;\n\t"
		"pop %%ds;\n\t"
		"popa;\n\t"
		"iret;\n\t" : : "a" (stacksetup));
		for(;;);	// Shouldn't be reached
	}
	
	gVM8086_WorkerPID = pid;

	// It's released when the GPF fires
	LOG("Waiting for worker %i to start", gVM8086_WorkerPID);
	Mutex_Acquire( &glVM8086_Process );
	Mutex_Release( &glVM8086_Process );
	
	// Worker killed itself
	if( gVM8086_WorkerPID != pid ) {
		return MODULE_ERR_MISC;
	}
	
	return MODULE_ERR_OK;
}

void VM8086_GPF(tRegs *Regs)
{
	Uint8	opcode;
	Uint16	newcs, newip;
	
//	Log_Log("VM8086", "GPF - %04x:%04x", Regs->cs, Regs->eip);

	LOG("VM8086 GPF at %04x:%04x", Regs->cs, Regs->eip);

	if(Regs->eip == VM8086_MAGIC_IP && Regs->cs == VM8086_MAGIC_CS
	&& Threads_GetPID() == gVM8086_WorkerPID)
	{
		if( gpVM8086_State == (void*)-1 ) {
			Log_Log("VM8086", "Worker thread ready and waiting");
			gpVM8086_State = NULL;
			Mutex_Release( &glVM8086_Process );	// Release lock obtained in VM8086_Install
		}
//		Log_Log("VM8086", "gpVM8086_State = %p, gVM8086_CallingThread = %i",
//			gpVM8086_State, gVM8086_CallingThread);
		if( gpVM8086_State )
		{
			gpVM8086_State->AX = Regs->eax;	gpVM8086_State->CX = Regs->ecx;
			gpVM8086_State->DX = Regs->edx;	gpVM8086_State->BX = Regs->ebx;
			gpVM8086_State->BP = Regs->ebp;
			gpVM8086_State->SI = Regs->esi;	gpVM8086_State->DI = Regs->edi;
			gpVM8086_State->DS = Regs->ds;	gpVM8086_State->ES = Regs->es;

			LOG("gpVM8086_State = %p", gpVM8086_State);
			LOG("gpVM8086_State->Internal = %p", gpVM8086_State->Internal);
			for( Uint i = 0; i < VM8086_PAGES_PER_INST; i ++ )
			{
				if( !gpVM8086_State->Internal->AllocatedPages[i].VirtBase )
					continue ;
				MM_Deallocate( (tPage*)VM8086_USER_BASE + i );
			}

			gpVM8086_State = NULL;
				
			// Wake the caller
			Semaphore_Signal(&gVM8086_TaskComplete, 1);
		}
		
		//Log_Log("VM8086", "Waiting for something to do");
		__asm__ __volatile__ ("sti");
		Semaphore_Wait(&gVM8086_TasksToDo, 1);
		
		for( Uint i = 0; i < VM8086_PAGES_PER_INST; i ++ )
		{
			if( !gpVM8086_State->Internal->AllocatedPages[i].VirtBase )
				continue ;
			MM_RefPhys( gpVM8086_State->Internal->AllocatedPages[i].PhysAddr );
			MM_Map( (tPage*)VM8086_USER_BASE + i, gpVM8086_State->Internal->AllocatedPages[i].PhysAddr );
		}

		
		//Log_Log("VM8086", "We have a task (%p)", gpVM8086_State);
		Regs->esp -= 2;	*(Uint16*)( (Regs->ss<<4) + (Regs->esp&0xFFFF) ) = VM8086_MAGIC_CS;
		Regs->esp -= 2;	*(Uint16*)( (Regs->ss<<4) + (Regs->esp&0xFFFF) ) = VM8086_MAGIC_IP;
		Regs->esp -= 2;	*(Uint16*)( (Regs->ss<<4) + (Regs->esp&0xFFFF) ) = gpVM8086_State->CS;
		Regs->esp -= 2;	*(Uint16*)( (Regs->ss<<4) + (Regs->esp&0xFFFF) ) = gpVM8086_State->IP;
		Regs->esp -= 2;	*(Uint16*)( (Regs->ss<<4) + (Regs->esp&0xFFFF) ) = gpVM8086_State->DS;
		Regs->esp -= 2;	*(Uint16*)( (Regs->ss<<4) + (Regs->esp&0xFFFF) ) = gpVM8086_State->ES;
		
		// Set Registers
		Regs->eip = 0x11;	Regs->cs = 0xFFFF;
		Regs->eax = gpVM8086_State->AX;	Regs->ecx = gpVM8086_State->CX;
		Regs->edx = gpVM8086_State->DX;	Regs->ebx = gpVM8086_State->BX;
		Regs->esi = gpVM8086_State->SI;	Regs->edi = gpVM8086_State->DI;
		Regs->ebp = gpVM8086_State->BP;
		Regs->ds = 0x23;	Regs->es = 0x23;
		Regs->fs = 0x23;	Regs->gs = 0x23;
		return ;
	}
	
	opcode = *(Uint8*)( (Regs->cs*16) + (Regs->eip) );
	Regs->eip ++;
	switch(opcode)
	{
	case VM8086_OP_PUSHF:	//PUSHF
		Regs->esp -= 2;
		*(Uint16*)( Regs->ss*16 + (Regs->esp&0xFFFF) ) = Regs->eflags & 0xFFFF;
		if( gbVM8086_ShadowIF )
			*(Uint16*)( Regs->ss*16 + (Regs->esp&0xFFFF) ) |= 0x200;
		else
			*(Uint16*)( Regs->ss*16 + (Regs->esp&0xFFFF) ) &= ~0x200;
		#if TRACE_EMU
		Log_Debug("VM8086", "%04x:%04x Emulated PUSHF (value 0x%x)",
			Regs->cs, Regs->eip-1, Regs->eflags & 0xFFFF);
		#endif
		break;
	case VM8086_OP_POPF:	//POPF
		// Changing IF is not allowed
		Regs->eflags &= 0xFFFF0202;
		Regs->eflags |= *(Uint16*)( Regs->ss*16 + (Regs->esp&0xFFFF) );
		gbVM8086_ShadowIF = !!(*(Uint16*)( Regs->ss*16 + (Regs->esp&0xFFFF) ) & 0x200);
		Regs->esp += 2;
		#if TRACE_EMU
		Log_Debug("VM8086", "%04x:%04x Emulated POPF (new value 0x%x)",
			Regs->cs, Regs->eip-1, Regs->eflags & 0xFFFF);
		#endif
		break;
	
	case VM8086_OP_INT_I:	//INT imm8
		{
		 int	id;
		id = *(Uint8*)( Regs->cs*16 +(Regs->eip&0xFFFF));
		Regs->eip ++;
		
		Regs->esp -= 2;	*(Uint16*)( Regs->ss*16 + (Regs->esp&0xFFFF) ) = Regs->eflags;
		Regs->esp -= 2;	*(Uint16*)( Regs->ss*16 + (Regs->esp&0xFFFF) ) = Regs->cs;
		Regs->esp -= 2;	*(Uint16*)( Regs->ss*16 + (Regs->esp&0xFFFF) ) = Regs->eip;
		
		newcs = *(Uint16*)(4*id + 2);
		newip = *(Uint16*)(4*id);
		#if TRACE_EMU
		Log_Debug("VM8086", "%04x:%04x Emulated INT 0x%x (%04x:%04x) - AX=%04x,BX=%04x",
			Regs->cs, Regs->eip-2, id, newcs, newip, Regs->eax, Regs->ebx);
		#endif
		Regs->cs = newcs;
		Regs->eip = newip;
		}
		break;
	
	case VM8086_OP_IRET:	//IRET
		newip = *(Uint16*)( Regs->ss*16 + (Regs->esp&0xFFFF) );	Regs->esp += 2;
		newcs = *(Uint16*)( Regs->ss*16 + (Regs->esp&0xFFFF) );	Regs->esp += 2;
		#if TRACE_EMU
		Log_Debug("VM8086", "%04x:%04x IRET to %04x:%04x",
			Regs->cs, Regs->eip-1, newcs, newip);
		#endif
		Regs->cs = newcs;
		Regs->eip = newip;
		break;
	
	
	case VM8086_OP_IN_AD:	//IN AL, DX
		Regs->eax &= 0xFFFFFF00;
		Regs->eax |= inb(Regs->edx&0xFFFF);
		#if TRACE_EMU
		Log_Debug("VM8086", "%04x:%04x Emulated IN AL, DX (Port 0x%x [Val 0x%02x])",
			Regs->cs, Regs->eip-1, Regs->edx&0xFFFF, Regs->eax&0xFF);
		#endif
		break;
	case VM8086_OP_IN_ADX:	//IN AX, DX
		Regs->eax &= 0xFFFF0000;
		Regs->eax |= inw(Regs->edx&0xFFFF);
		#if TRACE_EMU
		Log_Debug("VM8086", "%04x:%04x Emulated IN AX, DX (Port 0x%x [Val 0x%04x])",
			Regs->cs, Regs->eip-1, Regs->edx&0xFFFF, Regs->eax&0xFFFF);
		#endif
		break;
		
	case VM8086_OP_OUT_AD:	//OUT DX, AL
		outb(Regs->edx&0xFFFF, Regs->eax&0xFF);
		#if TRACE_EMU
		Log_Debug("VM8086", "%04x:%04x Emulated OUT DX, AL (*0x%04x = 0x%02x)",
			Regs->cs, Regs->eip-1, Regs->edx&0xFFFF, Regs->eax&0xFF);
		#endif
		break;
	case VM8086_OP_OUT_ADX:	//OUT DX, AX
		outw(Regs->edx&0xFFFF, Regs->eax&0xFFFF);
		#if TRACE_EMU
		Log_Debug("VM8086", "%04x:%04x Emulated OUT DX, AX (*0x%04x = 0x%04x)",
			Regs->cs, Regs->eip-1, Regs->edx&0xFFFF, Regs->eax&0xFFFF);
		#endif
		break;
		
	// TODO: Decide on allowing VM8086 Apps to enable/disable interrupts
	case 0xFA:	//CLI
		#if TRACE_EMU
		Log_Debug("VM8086", "%04x:%04x Ignored CLI",
			Regs->cs, Regs->eip);
		#endif
		gbVM8086_ShadowIF = 0;
		break;
	case 0xFB:	//STI
		#if TRACE_EMU
		Log_Debug("VM8086", "%04x:%04x Ignored STI",
			Regs->cs, Regs->eip);
		#endif
		gbVM8086_ShadowIF = 1;
		break;
	
	case 0x66:
		opcode = *(Uint8*)( (Regs->cs*16) + (Regs->eip&0xFFFF));
		Regs->eip ++;
		switch( opcode )
		{
		case VM8086_OP_IN_ADX:	//IN AX, DX
			Regs->eax = ind(Regs->edx&0xFFFF);
			#if TRACE_EMU
			Log_Debug("VM8086", "%04x:%04x Emulated IN EAX, DX (Port 0x%x [Val 0x%08x])",
				Regs->cs, Regs->eip-1, Regs->edx&0xFFFF, Regs->eax);
			#endif
			break;
		case VM8086_OP_OUT_ADX:	//OUT DX, AX
			outd(Regs->edx&0xFFFF, Regs->eax);
			#if TRACE_EMU
			Log_Debug("VM8086", "%04x:%04x Emulated OUT DX, EAX (*0x%04x = 0x%08x)",
				Regs->cs, Regs->eip-1, Regs->edx&0xFFFF, Regs->eax);
			#endif
			break;
		default:
			Log_Error("VM8086", "Error - Unknown opcode 66 %02x caused a GPF at %04x:%04x",
				Regs->cs, Regs->eip-2,
				opcode
				);
			// Force an end to the call
			Regs->cs = VM8086_MAGIC_CS;
			Regs->eip = VM8086_MAGIC_IP;
			break;
		}
		break;
	
	case 0x0F:
		opcode = *(Uint8*)( (Regs->cs*16) + (Regs->eip&0xFFFF));
		Log_Error("VM8086", "Error - Unknown opcode 0F %02x caused a GPF at %04x:%04x",
			opcode, Regs->cs, Regs->eip);
		// Force an end to the call
		Regs->cs = VM8086_MAGIC_CS;
		Regs->eip = VM8086_MAGIC_IP;
		break;

	default:
		Log_Error("VM8086", "Error - Unknown opcode %02x caused a GPF at %04x:%04x",
			opcode, Regs->cs, Regs->eip-1);
		// Force an end to the call
		Regs->cs = VM8086_MAGIC_CS;
		Regs->eip = VM8086_MAGIC_IP;
		break;
	}
}

/**
 * \brief Create an instance of the VM8086 Emulator
 */
tVM8086 *VM8086_Init(void)
{
	tVM8086	*ret;
	ret = calloc( 1, sizeof(tVM8086) + sizeof(struct sVM8086_InternalData) );
	ret->Internal = (void*)((tVAddr)ret + sizeof(tVM8086));
	return ret;
}

void VM8086_Free(tVM8086 *State)
{
	// TODO: Make sure the state isn't in use currently
	for( Uint i = VM8086_PAGES_PER_INST; i --; )
		MM_UnmapHWPages( State->Internal->AllocatedPages[i].VirtBase, 1);
	free(State);
}

void *VM8086_Allocate(tVM8086 *State, int Size, Uint16 *Segment, Uint16 *Offset)
{
	struct sVM8086_InternalPages	*pages = State->Internal->AllocatedPages;
	 int	i, j, base = 0;
	 int	nBlocks, rem;
	
	Size = (Size + 127) & ~127;
	nBlocks = Size / 128;
	
	if(Size > 4096)	return NULL;
	
	for( i = 0; i < VM8086_PAGES_PER_INST; i++ )
	{
		if( pages[i].VirtBase == 0 )	continue;
		
		//Log_Debug("VM8086", "pages[%i].Bitmap = 0b%b", i, pages[i].Bitmap);
		
		rem = nBlocks;
		base = 0;
		// Scan the bitmap for a free block
		// - 32 blocks per page == 128 bytes per block == 8 segments
		for( j = 0; j < 32; j++ )
		{
			if( pages[i].Bitmap & (1 << j) )
			{
				base = j+1;
				rem = nBlocks;
			}
			
			rem --;
			if(rem == 0)	// Goodie, there's a gap
			{
				for( j = 0; j < nBlocks; j++ )
					pages[i].Bitmap |= 1 << (base + j);
				*Segment = (VM8086_USER_BASE + i * 0x1000) / 16 + base * 8;
				*Offset = 0;
				LOG("Allocated at #%i,%04x", i, base*8*16);
				LOG(" - %x:%x", *Segment, *Offset);
				return pages[i].VirtBase + base * 8 * 16;
			}
		}
	}
	
	// No pages with free space?, allocate a new one
	for( i = 0; i < VM8086_PAGES_PER_INST; i++ )
	{
		if( pages[i].VirtBase == 0 )	break;
	}
	// Darn, we can't allocate any more
	if( i == VM8086_PAGES_PER_INST ) {
		Log_Warning("VM8086", "Out of pages in %p", State);
		return NULL;
	}
	
	pages[i].VirtBase = MM_AllocDMA(1, -1, &pages[i].PhysAddr);
	if( pages[i].VirtBase == 0 ) {
		Log_Warning("VM8086", "Unable to allocate data page");
		return NULL;
	}
	pages[i].Bitmap = 0;
	LOG("AllocatedPages[%i].VirtBase = %p", i, pages[i].VirtBase);
	LOG("AllocatedPages[%i].PhysAddr = %P", i, pages[i].PhysAddr);
		
	for( j = 0; j < nBlocks; j++ )
		pages[i].Bitmap |= 1 << j;
	LOG("AllocatedPages[%i].Bitmap = 0b%b", i, pages[i].Bitmap);
	*Segment = (VM8086_USER_BASE + i * 0x1000) / 16;
	*Offset = 0;
	LOG(" - %04x:%04x", *Segment, *Offset);
	return pages[i].VirtBase;
}

void *VM8086_GetPointer(tVM8086 *State, Uint16 Segment, Uint16 Offset)
{
	Uint32	addr = Segment * 16 + Offset;
	
	if( VM8086_USER_BASE <= addr && addr < VM8086_USER_BASE + VM8086_PAGES_PER_INST*0x1000 )
	{
		int pg = (addr - VM8086_USER_BASE) / 0x1000;
		if( State->Internal->AllocatedPages[pg].VirtBase == 0)
			return NULL;
		else
			return State->Internal->AllocatedPages[pg].VirtBase + (addr & 0xFFF);
	}
	else
	{
		return (void*)( KERNEL_BASE + addr );
	}
}

void VM8086_Int(tVM8086 *State, Uint8 Interrupt)
{
	State->IP = *(Uint16*)(KERNEL_BASE+4*Interrupt);
	State->CS = *(Uint16*)(KERNEL_BASE+4*Interrupt+2);

//	Log_Debug("VM8086", "Software interrupt %i to %04x:%04x", Interrupt, State->CS, State->IP);
	
	Mutex_Acquire( &glVM8086_Process );
	
	gpVM8086_State = State;
	gVM8086_CallingThread = Threads_GetTID();
	Semaphore_Signal(&gVM8086_TasksToDo, 1);

	Semaphore_Wait(&gVM8086_TaskComplete, 1);
	
	Mutex_Release( &glVM8086_Process );
}
