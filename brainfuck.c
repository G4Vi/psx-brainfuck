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

#define R_A0 (0x4)
#define R_SP (0x1D)
#define R_RA (0x1F)


#define OPCODE(code) (code << 26)
#define RS(REG) (REG << 21)
#define RT(REG) (REG << 16)

#define LUI(REG, IMM16) (OPCODE(0xF) | RS(0) | RT(REG) | IMM16)
#define JAL(ADDRESS) (OPCODE(0x3) | ((ADDRESS>>2)& 0x03FFFFFF))
#define ADDIU(SRC, DEST, IMM16) (OPCODE(0x9) | RS(SRC) | RT(DEST) | (IMM16 & 0xFFFF))
#define JR(REG) (OPCODE(0x0) | RS(REG) | 0x8)
#define NOP 0x0
#define SW(SRC, DEST, OFFSET16) (OPCODE(0x2B) | RS(DEST) | RT(SRC) | OFFSET16)
#define LW(SRC, DEST, OFFSET16) (OPCODE(0x23) | RS(SRC) | RT(DEST) | OFFSET16) 

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
	// todo save s0 through s7

    // leave space to initialize s0 to after the program
	*++dins = LUI(R_A0, (asmstringaddr >> 16));
	*++dins = JAL(printfaddr);
	*++dins = ADDIU(R_A0, R_A0, (asmstringaddr & 0xFFFF));

	int braceind = 0;
	uint32_t braces[100];
	const char *sins = program;

    

	// s0 stack ptr	

	if(0)
	//while(*sins != '\0')
	{
		switch(*sins)
		{
			case '>':
            // inc s0
			break;
			case '<':
            // dec s0
			break;
			case '+':
			// inc value at s0
			break;
			case '-':
			// dec value at s0
			break;
			case '.':
			// mov string to a0
	        // mov s0 to a1
			// call printf
			break;
			case ',':
			// copy from buffer
			// inc buffer ptr
			break;
			case '[':
            // conditional jump
			break;
			case ']':
			// conditional jump
			break;
		}	

		++sins;
	}
	// epilogue
	// todo restore s0 through s7
	*++dins = LW(R_SP, R_RA, 0x14);
	*++dins = ADDIU(R_SP, R_SP, 0x18);	
	*++dins = JR(R_RA);
	*++dins = NOP;
	syscall_flushCache();
    return (braceind == 0);		
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
