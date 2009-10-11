/*
 * AcessOS Microkernel Version
 * lib.c
 */
#include <common.h>

// === CODE ===
void Spinlock(int *lock)
{
	 int	v = 1;
	while(v)	__asm__ __volatile__ ("lock xchgl %%eax, (%%edi)":"=a"(v):"a"(1),"D"(lock));
}

void Release(int *lock)
{
	__asm__ __volatile__ ("lock andl $0, (%0)"::"r"(lock));
}

// === IO Commands ===
void outb(Uint16 Port, Uint8 Data)
{
	__asm__ __volatile__ ("outb %%al, %%dx"::"d"(Port),"a"(Data));
}
void outw(Uint16 Port, Uint16 Data)
{
	__asm__ __volatile__ ("outw %%ax, %%dx"::"d"(Port),"a"(Data));
}
void outd(Uint16 Port, Uint32 Data)
{
	__asm__ __volatile__ ("outl %%eax, %%dx"::"d"(Port),"a"(Data));
}
Uint8 inb(Uint16 Port)
{
	Uint8	ret;
	__asm__ __volatile__ ("inb %%dx, %%al":"=a"(ret):"d"(Port));
	return ret;
}
Uint16 inw(Uint16 Port)
{
	Uint16	ret;
	__asm__ __volatile__ ("inw %%dx, %%ax":"=a"(ret):"d"(Port));
	return ret;
}
Uint32 ind(Uint16 Port)
{
	Uint32	ret;
	__asm__ __volatile__ ("inl %%dx, %%eax":"=a"(ret):"d"(Port));
	return ret;
}

/**
 * \fn void *memset(void *Dest, int Val, Uint Num)
 * \brief Do a byte granuality set of Dest
 */
void *memset(void *Dest, int Val, Uint Num)
{
	__asm__ __volatile__ (
		"rep stosl;\n\t"
		"mov %3, %%ecx;\n\t"
		"rep stosb"
		:: "D" (Dest), "a" (Val), "c" (Num/4), "r" (Num&3));
	return Dest;
}
/**
 * \fn void *memsetd(void *Dest, Uint Val, Uint Num)
 */
void *memsetd(void *Dest, Uint Val, Uint Num)
{
	__asm__ __volatile__ ("rep stosl" :: "D" (Dest), "a" (Val), "c" (Num));
	return Dest;
}


/**
 * \fn void *memcpy(void *Dest, void *Src, Uint Num)
 */
void *memcpy(void *Dest, void *Src, Uint Num)
{
	if((Uint)Dest & 3 || (Uint)Src & 3)
		__asm__ __volatile__ ("rep movsb" :: "D" (Dest), "S" (Src), "c" (Num));
	else {
		__asm__ __volatile__ (
			"rep movsl;\n\t"
			"mov %3, %%ecx;\n\t"
			"rep movsb"
			:: "D" (Dest), "S" (Src), "c" (Num/4), "r" (Num&3));
	}
	return Dest;
}
/**
 * \fn void *memcpyd(void *Dest, void *Src, Uint Num)
 */
void *memcpyd(void *Dest, void *Src, Uint Num)
{
	__asm__ __volatile__ ("rep movsl" :: "D" (Dest), "S" (Src), "c" (Num));
	return Dest;
}

/**
 * \fn Uint64 __udivdi3(Uint64 Num, Uint64 Den)
 * \brief Divide two 64-bit integers
 */
Uint64 __udivdi3(Uint64 Num, Uint64 Den)
{
	Uint64	ret = 0;
	
	if(Den == 0)	__asm__ __volatile__ ("int $0x0");	// Call Div by Zero Error
	if(Den == 1)	return Num;	// Speed Hacks
	if(Den == 2)	return Num >> 1;	// Speed Hacks
	if(Den == 4)	return Num >> 2;	// Speed Hacks
	if(Den == 8)	return Num >> 3;	// Speed Hacks
	if(Den == 16)	return Num >> 4;	// Speed Hacks
	if(Den == 32)	return Num >> 5;	// Speed Hacks
	if(Den == 1024)	return Num >> 10;	// Speed Hacks
	if(Den == 2048)	return Num >> 11;	// Speed Hacks
	if(Den == 4096)	return Num >> 12;
	
	if(Num >> 32 == 0 && Den >> 32 == 0)
		return (Uint32)Num / (Uint32)Den;
	
	//Log("__udivdi3: (Num={0x%x:%x}, Den={0x%x:%x})",
	//	Num>>32, Num&0xFFFFFFFF,
	//	Den>>32, Den&0xFFFFFFFF);
	
	while(Num > Den) {
		ret ++;
		Num -= Den;
	}
	return ret;
}

/**
 * \fn Uint64 __umoddi3(Uint64 Num, Uint64 Den)
 * \brief Get the modulus of two 64-bit integers
 */
Uint64 __umoddi3(Uint64 Num, Uint64 Den)
{
	if(Den == 0)	__asm__ __volatile__ ("int $0x0");	// Call Div by Zero Error
	if(Den == 1)	return 0;	// Speed Hacks
	if(Den == 2)	return Num & 1;	// Speed Hacks
	if(Den == 4)	return Num & 3;	// Speed Hacks
	if(Den == 8)	return Num & 7;	// Speed Hacks
	if(Den == 16)	return Num & 15;	// Speed Hacks
	if(Den == 32)	return Num & 31;	// Speed Hacks
	if(Den == 1024)	return Num & 1023;	// Speed Hacks
	if(Den == 2048)	return Num & 2047;	// Speed Hacks
	if(Den == 4096)	return Num & 4095;	// Speed Hacks
	
	if(Num >> 32 == 0 && Den >> 32 == 0)
		return (Uint32)Num % (Uint32)Den;
	
	while(Num > Den)
		Num -= Den;
	return Num;
}

// --- EXPORTS ---
EXPORT(memcpy);	EXPORT(memset);
//EXPORT(memcpyw);	EXPORT(memsetw);
EXPORT(memcpyd);	EXPORT(memsetd);
EXPORT(inb);	EXPORT(inw);	EXPORT(ind);
EXPORT(outb);	EXPORT(outw);	EXPORT(outd);
EXPORT(__udivdi3);	EXPORT(__umoddi3);
