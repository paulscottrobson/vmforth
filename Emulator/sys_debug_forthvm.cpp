// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Name:		sys_debug_forthvm.c
//		Purpose:	Debugger Code (System Dependent)
//		Created:	25th July 2016
//		Author:		Paul Robson (paul@robsons->org.uk)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"
#include "sys_processor.h"
#include "debugger.h"
#include "hardware.h"
#define STATIC_WORD_NAMES
#include "__primitives.h"

#define DBGC_ADDRESS 	(0x0F0)														// Colour scheme.
#define DBGC_DATA 		(0x0FF)														// (Background is in main.c)
#define DBGC_HIGHLIGHT 	(0xFF0)

static BYTE8 __fontData[] = {
	#include "__font7x9_mcmfont.h"
};

static WORD16 __colours[8] = { 0x222,0xF00,0x0F0,0xFF0,0x00F,0xF0F,0x0FF,0xFFF };

// *******************************************************************************************************************************
//											This renders the debug screen
// *******************************************************************************************************************************

static const char *labels[] = { "PCTR","DSP","RSP","BRK", NULL };

void DBGXRender(int *address,int runMode) {
	CPUSTATUS *s = CPUGetStatus();
	GFXSetCharacterSize(42,25);
	DBGVerticalLabel(28,0,labels,DBGC_ADDRESS,-1);									// Draw the labels for the register

	GFXNumber(GRID(37,0),s->pc,16,5,GRIDSIZE,DBGC_DATA,-1);
	GFXNumber(GRID(37,1),s->dsp,16,5,GRIDSIZE,DBGC_DATA,-1);
	GFXNumber(GRID(37,2),s->rsp,16,5,GRIDSIZE,DBGC_DATA,-1);
	GFXNumber(GRID(37,3),address[3],16,5,GRIDSIZE,DBGC_DATA,-1);

	for (int i = 0;i < 2;i++) {
		int x = 26 + i*9;
		int y = 5;
		const char *st = (i == 0) ? "DATA":"RETURN";
		GFXString(GRID(x+4-strlen(st)/2,y),st,GRIDSIZE,DBGC_ADDRESS,-1);
		y++;
		int topOfStack = (i == 0) ? RST_DSP:RST_RSP;
		int stack = (i == 0) ? s->dsp:s->rsp;
		int colour = DBGC_HIGHLIGHT;
		while (stack < topOfStack && y <= 15) {
			long n = CPUReadMemory(stack) & 0xFFFFFFFF;
			GFXNumber(GRID(x,y),n,16,8,GRIDSIZE,colour,-1);
			colour = DBGC_DATA;
			y++;
			stack += 4;
		}
	}
	for (int y = 17;y < 25;y++) {
		int addr = (address[1] + (y - 17) * 16) & 0xFFFFC;
		GFXNumber(GRID(2,y),addr,16,5,GRIDSIZE,DBGC_ADDRESS,-1);
		for (int x = 0;x < 4;x++) {
			LONG32 n = CPUReadMemory(addr);
			GFXNumber(GRID(8+x*9,y),n,16,8,GRIDSIZE,DBGC_DATA,-1);
			addr = (addr + 4) & 0xFFFFC;
		}
	}

	for (int y = 0;y < 16;y++) {
		int addr = (address[0]+y*4) & 0xFFFFC;
		int isBrk = (addr == address[3]);
		int code = CPUReadMemory(addr) & 0xFFFFFFFF;
		GFXNumber(GRID(0,y),addr,16,5,GRIDSIZE,isBrk ? DBGC_HIGHLIGHT:DBGC_ADDRESS,-1);
		//GFXNumber(GRID(6,y),code,16,8,GRIDSIZE,isBrk ? DBGC_HIGHLIGHT:DBGC_DATA,-1);

		char szBuffer[64];
		int colour = DBGC_DATA;
		strcpy(szBuffer,"?");
		switch(code & 3) {
			case 0:		break; // CALL			
			case 1:		code = code >> 2;
						sprintf(szBuffer,"(core %x)",code);
						if (code >= 0 && code < COP_COUNT) strcpy(szBuffer,__primitives[code]);
						colour = 0x0FF;
						break;
			case 2: 	break; // STRING
			case 3:		code = (code >> 2);
						if (code & 0x20000000) code |= 0xC0000000;
						sprintf(szBuffer,"%x",code);
						colour = 0xFF0;
						break; 
		}
		szBuffer[32] = '\0';
		GFXString(GRID(6,y),szBuffer,GRIDSIZE,colour,-1);
	}
	if (runMode) {
		int szx = 4,szy = 3;
		SDL_Rect rcDisplay,rcCharacter,rcPixel,rcFrame;		
		rcDisplay.w = szx * 8 * 32;rcDisplay.h = szy * 14 * 16;
		rcDisplay.x = WIN_WIDTH/2-rcDisplay.w/2;
		rcDisplay.y = WIN_HEIGHT-64-rcDisplay.h;
		rcFrame = rcDisplay;
		rcFrame.w += 16;rcFrame.h +=16;rcFrame.x -= 8;rcFrame.y -= 8;

		rcCharacter.w = 8 * szx;rcCharacter.h = 8 * szy;
		rcPixel.w = szx;rcPixel.h = szy;
		GFXRectangle(&rcFrame,0x0);
		for (int y = 0;y < 16;y++) {
			rcCharacter.x = rcDisplay.x;
			for (int x = 0;x < 32;x++) {
				int ch = HWIReadScreenMemory(x,y);
				int col = __colours[(ch >> 8) & 7];
				ch = ch & 0x7F;
				rcCharacter.y = rcDisplay.y + y * 14 * szy;
				if (ch == 0x70 || ch == 0x71 || ch == 0x79 || ch == 0x67 || ch == 0x01 || ch == 0x02 ||
					ch == 0x06 || ch == 0x0B || ch == 0x2C || ch == 0x3B || ch == 0x6A)  
					rcCharacter.y += 3 * szy;
				BYTE8 *fontInfo = __fontData+ch*9;
				if (ch != ' ') {
					for (int y1 = 0;y1 < 9;y1++) {
						if (*fontInfo != 0x00)
						{
							rcPixel.x = rcCharacter.x;
							rcPixel.y = rcCharacter.y + szy * y1;
							for (int x1 = 0;x1 < 8;x1++) {
								if (*fontInfo & (0x80 >> x1)) GFXRectangle(&rcPixel,col);
								rcPixel.x += rcPixel.w;
							}
						}
					fontInfo++;
					}
				}
				rcCharacter.x += rcCharacter.w;
			}
		}
	}
}	
