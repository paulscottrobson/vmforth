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

static LONG32 memory[0x40000]; 														// 256k words of memory. (1M Bytes) 00000-FFFFF
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

void CPUReset(void) {
	HWIReset();
	pctr = 0x00000;
	rsp = RST_RSP;
	dsp = RST_DSP;
	cycles = 0;
	LITERAL(42);
	LITERAL(16);
	CORE(COP_STORE);
	pctr = 0x00000;
}

// *******************************************************************************************************************************
//												Execute a single instruction
// *******************************************************************************************************************************

BYTE8 CPUExecuteInstruction(void) {

	LONG32 instruction = memory[pctr >> 2];											// Fetch instruction
	pctr = (pctr + 4) & 0xFFFFC;													// Bump PC

	switch (instruction & 3) {
		case 0:																		// 00 call
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
	long addr,data;

	switch (primitive) {

		case COP_STORE:																// store word in memory
			PULLD(addr);PULLD(data);memory[addr >> 2] = data;
			break;

		case COP_MUL:
			break;

		case COP_ADD:
			break;

		case COP_ADD_STORE:
			break;

		case COP_SUB:
			break;

		case COP_DIV:
			break;

		case COP_0_SUB:
			break;

		case COP_0_LESS:
			break;

		case COP_0_EQUAL:
			break;

		case COP_0_GREATER:
			break;

		case COP_1_ADD:
			break;

		case COP_1_SUB:
			break;

		case COP_2_MUL:
			break;

		case COP_2_DIV:
			break;

		case COP_RETURN:
			break;

		case COP_GREATER_R:
			break;

		case COP_READ:
			break;

		case COP_AND:
			break;

		case COP_C_STORE:
			break;

		case COP_C_READ:
			break;

		case COP_DROP:
			break;

		case COP_DSP_STORE:
			break;

		case COP_DSP_READ:
			break;

		case COP_DUP:
			break;

		case COP_FOR:
			break;

		case COP_IF:
			break;

		case COP_NEXT:
			break;

		case COP_NOT:
			break;

		case COP_OR:
			break;

		case COP_OVER:
			break;

		case COP_R_GREATER:
			break;

		case COP_RDROP:
			break;

		case COP_ROT:
			break;

		case COP_RSP_STORE:
			break;

		case COP_RSP_READ:
			break;

		case COP_SWAP:
			break;

		case COP_THEN:
			break;

		case COP_XOR:
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
	BYTE8 opcode = memory[pctr >> 2] >> 28;
	if (opcode != 0xC) return 0;													// Not a call.
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
