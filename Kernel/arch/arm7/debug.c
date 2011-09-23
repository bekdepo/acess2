/**
 */
#include <acess.h>

// === CONSTANTS ===
//#define UART0_BASE	0x10009000
#define UART0_BASE	0xF0000000

// === PROTOTYPES ===
void	KernelPanic_SetMode(void);
void	KernelPanic_PutChar(char Ch);
void	StartupPrint(const char *str);

// === GLOBALS ===
 int	giDebug_SerialInitialised = 0;

// === CODE ===
void Debug_PutCharDebug(char ch)
{
//	while( *(volatile Uint32*)(SERIAL_BASE + SERIAL_REG_FLAG) & SERIAL_FLAG_FULL )
		;
	
//	*(volatile Uint32*)(SERIAL_BASE + SERIAL_REG_DATA) = ch;
	*(volatile Uint32*)(UART0_BASE) = ch;
}

void Debug_PutStringDebug(const char *str)
{
	for( ; *str; str++ )
		Debug_PutCharDebug( *str );
}

void KernelPanic_SetMode(void)
{
}

void KernelPanic_PutChar(char ch)
{
	Debug_PutCharDebug(ch);
}

void StartupPrint(const char *str)
{
}

