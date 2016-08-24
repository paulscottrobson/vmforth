// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Name:		sys_processor.c
//		Purpose:	Processor Emulation.
//		Created:	25th July 2016
//		Author:		Paul Robson (paul@robsons.org.uk)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include "sys_processor.h"
#include "sys_debug_system.h"
#include "hardware.h"
#include "__primitives.h"

static void _CPUExecutePrimitive(BYTE8 opcode);

// *******************************************************************************************************************************
//														   Timing
// *******************************************************************************************************************************

#define FRAME_RATE		(60)														// Frames per second (60)
#define CYCLES_PER_FRAME (1000*1000)												// Cycles per frame (100k)

// *******************************************************************************************************************************
//														CPU / Memory
// *******************************************************************************************************************************

static LONG32 memory[MSIZE] = { 0 }; 												// 256k words of memory. (1M Bytes) 00000-FFFFF
static LONG32 pctr;
static LONG32 rsp;
static LONG32 dsp;
static LONG32 cycles;
static BYTE8* bMemory = (BYTE8 *)memory;											// Access memory byte wise.

// *******************************************************************************************************************************
//															Stack
// *******************************************************************************************************************************

#define PUSHD(v) { dsp -= 4;memory[dsp >> 2] = (v); }
#define PUSHR(v) { rsp -= 4;memory[rsp >> 2] = (v); }
#define PUSHT(n) { n1 = ((n) != 0) ? 0xFFFFFFFF : 0; PUSHD(n1); }

#define PULLD(tgt) { tgt = memory[dsp >> 2];dsp += 4; }
#define PULLR(tgt) { tgt = memory[rsp >> 2];rsp += 4; }

#define TOSD() 	memory[dsp >> 2]
#define TOSD2() memory[(dsp >> 2)+1]
#define TOSD3() memory[(dsp >> 2)+2]

#define CHECK(a) if (((a) & 3) != 0) { exit(fprintf(stderr,"Address failure %08x",a)); }

// *******************************************************************************************************************************
//														Reset the CPU
// *******************************************************************************************************************************

#define LITERAL(x)  memory[pctr++] = ((((x) & 0x3FFFFFFF) << 2) | 3)				// Assembler Macros
#define CORE(x) memory[pctr++] = ((x) << 2) | 1
#define CALL(a) memory[pctr++] = (a) & 0xFFFFFFFC
#define COMPOSE(t,c1,c2,c3,c4) (t << 30) | (c4 << 2) | (c3 << 9) | (c2 << 16) | (c1 << 23) | 2

void CPUReset(void) {
	HWIReset();
	pctr = 0x00000;
	rsp = RST_RSP;
	dsp = RST_DSP;
	cycles = 0;
	LITERAL(2);
	CALL(0x20);
	LITERAL(3);

	pctr = 0x20 >> 2;
	memory[pctr-3] = COMPOSE(3,'d','e','m','o');
	memory[pctr-2] = COMPOSE(2,'w','d',0,0);
	memory[pctr-1] = 0x20;
	LITERAL(-1);
	CORE(COP_RETURN);
	pctr = 0x00000;
}

// *******************************************************************************************************************************
//												Execute a single instruction
// *******************************************************************************************************************************

BYTE8 CPUExecuteInstruction(void) {

	LONG32 instruction = memory[pctr >> 2];											// Fetch instruction
	pctr = (pctr + 4) & MMASK;														// Bump PC

	switch (instruction & 3) {
		case 0:																		// 00 call
			PUSHR(pctr);pctr = instruction & MMASK;
			break;
		case 1:																		// 01 core command
			_CPUExecutePrimitive(instruction >> 2);
			break;
		case 2:																		// 02 definition/string
			break;
		case 3:																		// 03 literal.
			instruction = instruction >> 2;											// 30 bit unsigned.
			if (instruction & 0x20000000) instruction |= 0xC0000000;				// Sign extend
			PUSHD(instruction)														// Push onto data stack.
			break;
	}

	cycles++;
	if (cycles < CYCLES_PER_FRAME) return 0;										// Not completed a frame.
	cycles = cycles - CYCLES_PER_FRAME;												// Adjust this frame rate.
	return FRAME_RATE;																// Return frame rate.
}

// *******************************************************************************************************************************
//												Execute primitives
// *******************************************************************************************************************************

static void _CPUExecutePrimitive(BYTE8 primitive) {
	LONG32 addr,data,n1,n2,n3;
	BYTE8 found;

	switch (primitive) {

		case COP_STORE:																// store word in memory
			PULLD(addr);addr &= MMASK;PULLD(data);memory[addr >> 2] = data;
			break;

		case COP_MUL:
			PULLD(n1);PULLD(n2);n1 = (n1 * n2) & 0xFFFFFFFF;PUSHD(n1);
			break;

		case COP_ADD:
			PULLD(n1);PULLD(n2);n1 = (n1 + n2) & 0xFFFFFFFF;PUSHD(n1);
			break;

		case COP_ADD_STORE:
			PULLD(addr);addr &= MMASK;PULLD(data);memory[addr >> 2] += data;
			break;

		case COP_SUB:
			PULLD(n1);PULLD(n2);n1 = (n2 - n1) & 0xFFFFFFFF;PUSHD(n1);
			break;

		case COP_DIV:
			PULLD(n1);PULLD(n2);n1 = (n1 == 0) ? 0 : (n2 / n1);PUSHD(n1);
			break;

		case COP_0_SUB:
			PULLD(n1);PUSHD((-n1) & 0xFFFFFFFF);
			break;

		case COP_0_LESS:
			PULLD(n1);PUSHT(n1 & 0x80000000);
			break;

		case COP_0_EQUAL:
			PULLD(n1);PUSHT(n1 == 0);
			break;

		case COP_0_GREATER:
			PULLD(n1);PUSHT((n1 != 0) && ((n1 & 0x80000000) == 0))
			break;

		case COP_1_ADD:
			PULLD(n1);PUSHD((n1+1) & 0xFFFFFFFF);
			break;

		case COP_1_SUB:
			PULLD(n1);PUSHD((n1-1) & 0xFFFFFFFF);
			break;

		case COP_2_MUL:
			PULLD(n1);PUSHD((n1 << 1) & 0xFFFFFFFF);
			break;

		case COP_2_DIV:
			PULLD(n1);PUSHD((n1 >> 1) & 0x7FFFFFFF);
			break;

		case COP_RETURN:
			PULLR(pctr);pctr = (pctr & MMASK) & 0xFFFFFFFC;
			break;

		case COP_GREATER_R:
			PULLD(n1);PUSHR(n1);
			break;

		case COP_READ:
			PULLD(addr);addr &= MMASK;data = memory[addr >> 2];PUSHD(data);
			break;

		case COP_AND:
			PULLD(n1);PULLD(n2);n1 = n1 & n2;PUSHD(n1);
			break;

		case COP_C_STORE:
			PULLD(addr);addr &= MMASK;PULLD(data);bMemory[addr] = data;
			break;

		case COP_C_READ:
			PULLD(addr);addr &= MMASK;data = bMemory[addr];PUSHD(data);
			break;

		case COP_DROP:
			PULLD(n1);
			break;

		case COP_DSP_STORE:
			PULLD(n1);dsp = n1 & MMASK & 0xFFFFFFFC;
			break;

		case COP_DSP_READ:
			n1 = dsp;PUSHD(n1);
			break;

		case COP_DUP:
			PULLD(n1);PUSHD(n1);PUSHD(n1);
			break;

		case COP_FOR:															// FOR should not be nested even though it works.
			PUSHR(pctr);														// Push loop address
			PULLD(n1);PUSHR(n1-1);												// Push counter
			break;

		case COP_IF:															// IF is not nestable as in ColorFORTH.
			PULLD(n1);
			if (n1 == 0) {														// Test failed
				found = 0;
				while (found == 0) {											// Keep skipping
					n2 = memory[pctr >> 2];										// Fetch
					pctr = (pctr + 4) & MMASK;
					if (n2 == ((COP_THEN << 2) | 1)) found = 1;					// Exit if THEN
					if (n2 == ((COP_RETURN << 2) | 1)) found = 1;				// Exit if ; [RETURN]
					if ((n2 & 0xC0000003) == 0x80000002) found = 1; 			// Run into another definition ????
				} 
			}
			break;

		case COP_NEXT:
			PULLR(n1);PULLR(addr);												// Restore count, loop address.
			n1 = (n1 - 1) & 0xFFFFFFFF;
			if ((n1 & 0x80000000) == 0) {										// Loop back
				pctr = addr;
				PUSHR(addr);PUSHR(n1);
			}
			break;

		case COP_NOT:
			PULLD(n1);n1 = n1 & 0xFFFFFFFF;PUSHD(n1);
			break;

		case COP_OR:
			PULLD(n1);PULLD(n2);n1 = n1 | n2;PUSHD(n1);
			break;

		case COP_OVER:
			PULLD(n1);PULLD(n2);PUSHD(n2);PUSHD(n1);PUSHD(n2);
			break;

		case COP_R_GREATER:
			PULLR(n1);PUSHD(n1);
			break;

		case COP_RDROP:
			PULLR(n1);
			break;

		case COP_ROT:
			PULLD(n1);PULLD(n2);PULLD(n3);
			PUSHD(n2);PUSHD(n1);PUSHD(n3);
			break;

		case COP_RSP_STORE:
			PULLD(n1);rsp = n1 & MMASK & 0xFFFFFFFC;
			break;

		case COP_RSP_READ:
			n1 = rsp;PUSHD(n1);
			break;

		case COP_SWAP:
			PULLD(n1);PULLD(n2);PUSHD(n1);PUSHD(n2);
			break;

		case COP_THEN:
			// Does nothing, marker for IF.
			break;

		case COP_XOR:
			PULLD(n1);PULLD(n2);n1 = n1 ^ n2;PUSHD(n1);
			break;

	}
}

#ifdef INCLUDE_DEBUGGING_SUPPORT

// *******************************************************************************************************************************
//		Execute chunk of code, to either of two break points or frame-out, return non-zero frame rate on frame, breakpoint 0
// *******************************************************************************************************************************

BYTE8 CPUExecute(LONG32 breakPoint1,LONG32 breakPoint2) { 
	do {
		BYTE8 r = CPUExecuteInstruction();											// Execute an instruction
		if (r != 0) return r; 														// Frame out.
	} while (pctr != breakPoint1 && pctr != breakPoint2);							// Stop on breakpoint.
	return 0; 
}

// *******************************************************************************************************************************
//									Return address of breakpoint for step-over, or 0 if N/A
// *******************************************************************************************************************************

LONG32 CPUGetStepOverBreakpoint(void) {
	BYTE8 opcode = memory[pctr >> 2] & 3;											// Get instruction type.
	if (opcode != 0) return 0;														// Not a call.
	return (pctr+4) & 0xFFFFF;														// Instruction after next.	
}

// *******************************************************************************************************************************
//												Read/Write Memory
// *******************************************************************************************************************************

LONG32 CPUReadMemory(LONG32 address) {
	return memory[(address / 4) & 0x3FFFF] & 0xFFFFFFFF;
}

void CPUWriteMemory(WORD16 address,LONG32 data) {
	memory[(address/4) & 0x3FFFF] = data & 0xFFFFFFFF;
}

// *******************************************************************************************************************************
//												Load a binary file into RAM
// *******************************************************************************************************************************

#include <stdio.h>

void CPULoadBinary(const char *fileName) {
	FILE *f = fopen(fileName,"rb");
	BYTE8 *ram = (BYTE8 *)memory;
	ram += 0;
	while (!feof(f)) {
		fread(ram,1,16384,f);
		ram += 16384;
	}
	fclose(f);
}

// *******************************************************************************************************************************
//											Retrieve a snapshot of the processor
// *******************************************************************************************************************************

static CPUSTATUS s;																	// Status area

CPUSTATUS *CPUGetStatus(void) {
	s.pc = pctr;s.dsp = dsp;s.rsp = rsp;s.cycles = cycles;
	return &s;
}

#endif
