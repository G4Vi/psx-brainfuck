#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "common/syscalls/syscalls.h"


const char *SQUARES = 
"++++[>+++++<-]>[<+++++>-]+<+["
"    >[>+>+<<-]++>>[<<+>>-]>>>[-]++>[-]+"
"    >>>+[[-]++++++>>>]<<<[[<++++++++<++>>-]+<.<[>----<-]<]"
"    <<[>>>>>[>>>[-]+++++++++<[>-<-]+++++++++>[-[<->-]+[<<<]]<[>+<-]>]<<-]<<-"
"]";


const char *HELLOWORLD = 
">++++++++[<+++++++++>-]<.>++++[<+++++++>-]<+.+++++++..+++.>>++++++[<+++++++>-]<+"
"+.------------.>++++++[<+++++++++>-]<+.<.+++.------.--------.>>>++++[<++++++++>-"
"]<+.";

const char *HEAD = "+>>>>>>>>>>-[,+[-.----------[[-]>]<->]<]";
const char *HEADINPUT = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n";


uint32_t MEMORY[1048576/sizeof(uint32_t)];

bool interpret(const char *program, const char *input)
{
	const char *pc = program;
	memset(&MEMORY, 0, sizeof(MEMORY));
	int32_t *sp = &MEMORY[0];
	int nestlevel = 0;
	int tmpnestlevel;

    // loop through the opcodes
	while(1)
	{
		switch(*pc)
		{
			case '\0':
			return (nestlevel == 0);
			break;
			case '>':
			++sp;
			break;
			case '<':
			--sp;
			break;
			case '+':
			++*sp;
			break;
			case '-':
			--*sp;
			break;
			case '.':
			ramsyscall_printf("%c", *sp);			
			break;
			case ',':
			if((input == NULL) || (*input == '\0'))
			{
				return false;
			}
            *sp = *input;
			++input;
			break;
			case '[':
			if(*sp)
			{
				++nestlevel;
			}
			else
			{
				// advance the pc to the closing bracket
				tmpnestlevel = 0;
                while(1)
				{
					if(*pc == '\0')
					{
						return false;
					}
					else if(*pc == ']')
					{
						--tmpnestlevel;
						if(tmpnestlevel == 0)
						{
							break;
						}
					}
					else if(*pc == '[')
					{
						++tmpnestlevel;
					}
					++pc;
				}				
			}
			break;
			case ']':
            if(*sp)
			{
				// reverse the pc to opening bracket
				tmpnestlevel = 0;
				while(1)
				{
					if(*pc == '[')
					{
						--tmpnestlevel;
						if(tmpnestlevel == 0)
						{
							break;
						}
					}
					else if(*pc == ']')
					{
						++tmpnestlevel;
					}

					if(pc == program)
					{
						return false;
					}
					--pc;
				}				
			}
			else
			{
				--nestlevel;
			}
			break;
		}
		++pc;
	}
	return true;
}

#define R_00 (0x00)
#define R_A0 (0x04)
#define R_A1 (0x05)
#define R_T0 (0x08)
#define R_S0 (0x10)
#define R_SP (0x1D)
#define R_RA (0x1F)


#define OPCODE(code) (code << 26)
#define RS(REG) (REG << 21)
#define RT(REG) (REG << 16)

#define LUI(REG, IMM16) (OPCODE(0xF) | RS(0) | RT((REG)) | (IMM16))
#define JAL(ADDRESS) (OPCODE(0x3) | (((ADDRESS)>>2)& 0x03FFFFFF))
#define ADDIU(SRC, DEST, IMM16) (OPCODE(0x9) | RS((SRC)) | RT((DEST)) | ((IMM16) & 0xFFFF))
#define JR(REG) (OPCODE(0x0) | RS(REG) | 0x8)
#define NOP 0x0
#define SW(SRC, DEST, OFFSET16) (OPCODE(0x2B) | RS(DEST) | RT(SRC) | OFFSET16)
#define LW(SRC, DEST, OFFSET16) (OPCODE(0x23) | RS(SRC) | RT(DEST) | OFFSET16)
#define BEQ(FIRST, SECOND, LINES16) (OPCODE(0x04) | RS((FIRST)) | RT((SECOND)) | ((LINES16)&0xFFFF))
#define BNE(FIRST, SECOND, LINES16) (OPCODE(0x05) | RS((FIRST)) | RT((SECOND)) | ((LINES16)&0xFFFF))

typedef void (*voidfunc)(void);
bool compile(const char *program, const char *input)
{
	memset(&MEMORY, 0, sizeof(MEMORY));
	ramsyscall_printf("memaddr 0x%X\n", &MEMORY);
	uint32_t printfaddr = (uint32_t)&ramsyscall_printf;
    const char *asmstring = "hello asm\n";
	uint32_t asmstringaddr = (uint32_t)asmstring;

    /*MEMORY[0] = ADDIU(R_SP, R_SP, -0x18);
	MEMORY[1] = SW(R_RA, R_SP, 0x14);
	MEMORY[2] = LUI(R_A0, (asmstringaddr >> 16));
	MEMORY[3] = JAL(printfaddr);
	MEMORY[4] = ADDIU(R_A0, R_A0, (asmstringaddr & 0xFFFF));
	MEMORY[5] = LW(R_SP, R_RA, 0x14);
	MEMORY[6] = ADDIU(R_SP, R_SP, 0x18);	
	MEMORY[7] = JR(R_RA);
	MEMORY[8] = NOP;*/

    
    uint32_t *dins = &MEMORY[0];
	// prologue
	*dins = NOP;
	*++dins = ADDIU(R_SP, R_SP, -0x18);
	*++dins = SW(R_RA, R_SP, 0x14);
	*++dins = SW(R_S0, R_SP, 0x10);
	// leave space to initialize s0 to memory after the program
    uint32_t *inits0hi = ++dins;
	uint32_t *inits0lo = ++dins;    
	
    const int MAX_BRACES = 100;
	int bracecnt = 0;
	uint32_t braces[MAX_BRACES];
	int lines;
	//const char *sins = program;
	//const char *sins = "+[-]";
	//const char *sins = HELLOWORLD;
	const char *sins = SQUARES;

    

	// s0 stack ptr
	const char *fmt = "%c";	

	//if(0)
	while(*sins != '\0')
	{
		switch(*sins)
		{
			case '>':
            // inc s0
			*++dins = ADDIU(R_S0, R_S0, 0x4);
			break;
			case '<':
			// dec s0
			*++dins = ADDIU(R_S0, R_S0, -0x4);            
			break;
			case '+':
			// load value at s0. increment, and, store
			*++dins = LW(R_S0, R_T0,0x0);
			*++dins = ADDIU(R_T0, R_T0, 0x1);
			*++dins = SW(R_T0, R_S0, 0x0);
			break;
			case '-':
			// load value at s0. decrement, and, store
			*++dins = LW(R_S0, R_T0,0x0);
			*++dins = ADDIU(R_T0, R_T0, -0x1);
			*++dins = SW(R_T0, R_S0, 0x0);
			break;
			case '.':
			// load value at s0. load fmt string and call printf
			*++dins = LW(R_S0, R_A1, 0x0);
			*++dins = LUI(R_A0, ((uint32_t)fmt) >> 16);
			*++dins = JAL(printfaddr);
			*++dins = ADDIU(R_A0, R_A0, ((uint32_t)fmt)&0xFFFF);
			break;
			case ',':
			// copy from buffer
			// inc buffer ptr
			break;
			case '[':            
			if(bracecnt >= 100)
			{
				return false;
			}
			// load value at s0 BEQZ
			*++dins = LW(R_S0, R_T0, 0x0);			
			// save where we need a BEQ
			braces[bracecnt++] = (uint32_t)(++dins);
			*++dins = NOP; // branch delay slot
			break;
			case ']':
			if(bracecnt == 0)
			{
				return false;
			}
			// load value at s0 BEQZ
			*++dins = LW(R_S0, R_T0, 0x0);
			lines = (((uint32_t)(++dins)) -  braces[bracecnt-1])/ 4;			
			*(uint32_t*)braces[--bracecnt] = BEQ(R_T0, R_00, lines);
			*dins = BNE(R_T0, R_00, -lines);
			*++dins = NOP; // branch delay slot
			break;
		}	

		++sins;
	}
	// epilogue
	// todo restore s register usage
	*++dins = LW(R_SP, R_S0, 0x10);
	*++dins = LW(R_SP, R_RA, 0x14);
	*++dins = ADDIU(R_SP, R_SP, 0x18);	
	*++dins = JR(R_RA);
	*++dins = NOP;

	// intialize the start ptr to just after the instructions
	uint32_t startptr =  (uint32_t)++dins;
	*inits0hi = LUI(R_S0, startptr >> 16);
    *inits0lo = ADDIU(R_S0, R_S0, startptr & 0xFFFF);
	syscall_flushCache();
    return (bracecnt == 0);		
}

void interpret_program(const char *program, const char *input)
{
	ramsyscall_printf("\n---BEGIN_PROGRAM---\n%s\n---END_PROGRAM---\n", program);
    ramsyscall_printf("Interpreting\n");
	if(!interpret(program, input))
	{
		ramsyscall_printf("Interpreting Failed\n");
	}
	else
	{
		ramsyscall_printf("Interpreting Done\n");
	}
}


int main(void) {
	ramsyscall_printf("psx-brainfuck\n");
	interpret_program(HELLOWORLD, NULL);
    interpret_program(SQUARES, NULL);
	interpret_program(HEAD, HEADINPUT);
	compile(NULL, NULL);
	voidfunc memcall = (voidfunc)&MEMORY;
	memcall();		
	ramsyscall_printf("compiled ran\n");
	while(1);
}
