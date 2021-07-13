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
#define R_RA (0x1F)

#define OPCODE(code) (code << 26)
#define RS(REG) (REG << 21)
#define RT(REG) (REG << 16)

#define LUI(REG, IMM16) (OPCODE(0xF) | RS(0) | RT(REG) | IMM16)
#define JAL(ADDRESS) (OPCODE(0x3) | ((ADDRESS>>2)& 0x03FFFFFF))
#define ADDIU(SRC, DEST, IMM16) (OPCODE(0x9) | RS(SRC) | RT(DEST) | IMM16)
#define JR(REG) (OPCODE(0x0) | RS(REG) | 0x8)
#define NOP 0x0

typedef void (*voidfunc)(void);
bool compile(const char *program, const char *input)
{
	uint32_t printfaddr = (uint32_t)&ramsyscall_printf;
    const char *asmstring = "hello asm\n";
	uint32_t asmstringaddr = (uint32_t)asmstring;

	MEMORY[0] = LUI(R_A0, (asmstringaddr >> 16));
	MEMORY[1] = JAL(printfaddr);
	MEMORY[2] = ADDIU(R_A0, R_A0, (asmstringaddr & 0xFFFF));
	MEMORY[3] = JR(R_RA);
	MEMORY[4] = NOP;

	voidfunc memcall = (voidfunc)&MEMORY;
	memcall();
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
	ramsyscall_printf("compiled ran\n");
	while(1);
}
