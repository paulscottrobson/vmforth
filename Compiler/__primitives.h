/* Automatically generated */

#ifndef __PRIMITIVES
#define __PRIMITIVES

#define COP_COUNT (39)

#define COP_STORE (0)
#define COP_SYS_SYS_BRANCH (1)
#define COP_SYS_SYS_HWIO (2)
#define COP_MUL (3)
#define COP_ADD (4)
#define COP_ADD_STORE (5)
#define COP_SUB (6)
#define COP_DIV (7)
#define COP_0_SUB (8)
#define COP_0_LESS (9)
#define COP_0_EQUAL (10)
#define COP_0_GREATER (11)
#define COP_1_ADD (12)
#define COP_1_SUB (13)
#define COP_2_MUL (14)
#define COP_2_DIV (15)
#define COP_RETURN (16)
#define COP_GREATER_R (17)
#define COP_READ (18)
#define COP_AND (19)
#define COP_C_STORE (20)
#define COP_C_READ (21)
#define COP_DROP (22)
#define COP_DSP_STORE (23)
#define COP_DSP_READ (24)
#define COP_DUP (25)
#define COP_FOR (26)
#define COP_IF (27)
#define COP_NEXT (28)
#define COP_NOT (29)
#define COP_OR (30)
#define COP_OVER (31)
#define COP_R_GREATER (32)
#define COP_ROT (33)
#define COP_RSP_STORE (34)
#define COP_RSP_READ (35)
#define COP_SWAP (36)
#define COP_THEN (37)
#define COP_XOR (38)

#ifdef STATIC_WORD_NAMES

static const char *__primitives[] = {"!","$$branch","$$hwio","*","+","+!","-","/","0-","0<","0=","0>","1+","1-","2*","2/",";",">r","@","and","c!","c@","drop","dsp!","dsp@","dup","for","if","next","not","or","over","r>","rot","rsp!","rsp@","swap","then","xor"};
#endif

#endif

