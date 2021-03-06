/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/vterm_vt100.c
 * - Virtual Terminal - VT100 (Kinda) Emulation
 */
#define DEBUG	0
#include "vterm.h"

#define sTerminal	sVTerm
#include "../../../Usermode/Applications/gui_terminal_src/vt100.c"

void *Display_GetTermState(tTerminal *Term) {
	return Term->VT100Info;
}
void Display_SetTermState(tTerminal *Term, void *State) {
	Term->VT100Info = State;
}

void Display_SendInput(tTerminal *Term, const char *String)
{
	PTY_SendInput(Term->PTY, String, strlen(String));
}

void Display_AddText(tTerminal *Term, size_t Length, const char *UTF8Text)
{
	LOG("'%*C'", Length, UTF8Text);
	VT_int_PutRawString(Term, (const void*)UTF8Text, Length);
}
void Display_Newline(tTerminal *Term, bool bCarriageReturn)
{
	LOG("");
	VT_int_PutChar(Term, '\n');
}
void Display_SetScrollArea(tTerminal *Term, int Start, int Count)
{
	LOG("(%i,+%i)", Start, Count);
	Term->ScrollTop = Start;
	Term->ScrollHeight = Count;
}
void Display_ScrollDown(tTerminal *Term, int CountDown)
{
	LOG("(%i)", CountDown);
	VT_int_UpdateScreen(Term, 0);
	if( Term->Flags & VT_FLAG_ALTBUF )
		VT_int_ScrollText(Term, -CountDown);
	else
	{
		if(Term->ViewTopRow + CountDown < 0)
			return ;
		if(Term->ViewTopRow + CountDown  > Term->TextHeight * (giVT_Scrollback + 1))
			return ;
		
		Term->ViewTopRow += CountDown;
	}
}
void Display_SetCursor(tTerminal *Term, int Row, int Col)
{
	LOG("(R%i,C%i)", Row, Col);
	VT_int_UpdateScreen(Term, 0);
	 int	maxrows = VT_int_GetBufferRows(Term);
	ASSERTCR( Row, >=, 0, );
	ASSERTCR( Row, <, maxrows, );
	ASSERTCR( Col, >=, 0, );
	ASSERTCR( Col, <, Term->TextWidth, );
	VT_int_GetWritePosPtr(Term)->Row = Row;
	VT_int_GetWritePosPtr(Term)->Col = Col;
}
void Display_MoveCursor(tTerminal *Term, int RelRow, int RelCol)
{
	LOG("(R+%i,C+%i)", RelRow, RelCol);
	tVT_Pos	*wrpos = VT_int_GetWritePosPtr(Term);

	// TODO: Support scrolling if cursor goes offscreen
	// if( bScrollIfNeeded )
	//	Display_ScrollDown(extra);
	// else
	//	clip

	if( RelCol != 0 )
	{
		// 
		if( RelCol < 0 )
		{
			 int	max = wrpos->Col;
			if( RelCol < -max )
				RelCol = -max;
		}
		else
		{
			size_t	max = Term->TextWidth - wrpos->Col - 1;
			if(RelCol > max)
				RelCol = max;
		}
		wrpos->Col += RelCol;
	}
	if( RelRow != 0 )
	{
		 int	currow = wrpos->Row;
		 int	maxrows = VT_int_GetBufferRows(Term);
		if( RelRow < 0 )
		{
			if( RelRow < -currow )
				RelRow = -currow;
		}
		else
		{
			if( currow + RelRow > maxrows-1 )
				RelRow = maxrows-1 - currow;
		}
		wrpos->Row += RelRow;
	}
	LOG("=(R%i,C%i)", wrpos->Row, wrpos->Col);
}
void Display_SaveCursor(tTerminal *Term)
{
	Term->SavedWritePos = *VT_int_GetWritePosPtr(Term);
	LOG("Saved = %i", Term->SavedWritePos);
}
void Display_RestoreCursor(tTerminal *Term)
{
	size_t	maxrow = VT_int_GetBufferRows(Term);
	tVT_Pos	*wrpos = VT_int_GetWritePosPtr(Term);
	*wrpos = Term->SavedWritePos;
	if(wrpos->Row >= maxrow)
		wrpos->Row = maxrow-1;
	if(wrpos->Col >= Term->TextWidth )
		wrpos->Col = Term->TextWidth-1;
	LOG("Restored (R%i,C%i)", wrpos->Row, wrpos->Col);
}
// 0: All, 1: Forward, -1: Reverse
void Display_ClearLine(tTerminal *Term, int Dir)
{
	const tVT_Pos	*wrpos = VT_int_GetWritePosPtr(Term);

	LOG("(Dir=%i)", Dir);

	// Erase all
	if( Dir == 0 ) {
		VT_int_ClearLine(Term, wrpos->Row);
		VT_int_UpdateScreen(Term, 0);
	}
	// Erase to right
	else if( Dir == 1 ) {
		VT_int_ClearInLine(Term, wrpos->Row, wrpos->Col, Term->TextWidth);
		VT_int_UpdateScreen(Term, 0);
	}
	// Erase to left
	else if( Dir == -1 ) {
		VT_int_ClearInLine(Term, wrpos->Row, 0, wrpos->Col);
		VT_int_UpdateScreen(Term, 0);
	}
	else {
		// ERROR!
		ASSERTC(Dir, >=, -1);
		ASSERTC(Dir, <=, 1);
	}
}
void Display_ClearLines(tTerminal *Term, int Dir)
{
	LOG("(Dir=%i)", Dir);	
	tVT_Pos	*wrpos = VT_int_GetWritePosPtr(Term);
	size_t	height = VT_int_GetBufferRows(Term);
	
	// All
	if( Dir == 0 ) {
		
		if( !(Term->Flags & VT_FLAG_ALTBUF) ) {
			Term->ViewTopRow = 0;
		}
		int count = height;
		while( count -- )
			VT_int_ClearLine(Term, count);
		wrpos->Col = 0;
		wrpos->Row = 0;
		VT_int_UpdateScreen(Term, 1);
	}
	// Downwards
	else if( Dir == 1 ) {
		for( int row = wrpos->Row; row < height; row ++ )
			VT_int_ClearLine(Term, row);
		VT_int_UpdateScreen(Term, 1);
	}
	// Upwards
	else if( Dir == -1 ) {
		for( int row = 0; row < wrpos->Row; row ++ )
			VT_int_ClearLine(Term, row);
		VT_int_UpdateScreen(Term, 1);
	}
	else {
		// ERROR!
		ASSERTC(Dir, >=, -1);
		ASSERTC(Dir, <=, 1);
	}
}
void Display_ResetAttributes(tTerminal *Term)
{
	LOG("");	
	Term->CurColour = DEFAULT_COLOUR;
}
void Display_SetForeground(tTerminal *Term, uint32_t RGB)
{
	LOG("(%06x)", RGB);
	Term->CurColour &= 0x8000FFFF;
	Term->CurColour |= (Uint32)VT_Colour24to12(RGB) << 16;
	
}
void Display_SetBackground(tTerminal *Term, uint32_t RGB)
{
	LOG("(%06x)", RGB);
	Term->CurColour &= 0xFFFF8000;
	Term->CurColour |= (Uint32)VT_Colour24to12(RGB) <<  0;
}
void Display_Flush(tTerminal *Term)
{
	LOG("");
	VT_int_UpdateScreen(Term, 0);
}
void Display_ShowAltBuffer(tTerminal *Term, bool AltBufEnabled)
{
	LOG("(%B)", AltBufEnabled);
	VT_int_ToggleAltBuffer(Term, AltBufEnabled);
}
void Display_SetTitle(tTerminal *Term, const char *Title)
{
	// ignore
}

