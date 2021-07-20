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
#define R_V0 (0x02)
#define R_A0 (0x04)
#define R_A1 (0x05)
#define R_T0 (0x08)
#define R_T1 (0x09)
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

// not sure if right
#define JALR(REG) ( OPCODE(0x00) | RS((REG)) | RT(0x00) | 0xF809)



uint32_t *o_encode_adjust_ptr(uint32_t *before, const char ** pProgram)
{
	int adjust = 0;
	while(1)
	{
		if(**pProgram == '>')
		{
			adjust += 0x4;
		}
		else if(**pProgram == '<')
		{
			adjust -= 0x4;
		}
		else if((**pProgram == '+') || (**pProgram == '-') || (**pProgram == '[') || (**pProgram == ']') || (**pProgram == '.') || (**pProgram == ',') || (**pProgram == '\0'))
		{
			if(adjust == 0) return before;
			break;
		}
		++*pProgram;
	}
	*++before = ADDIU(R_S0, R_S0, adjust);
	return before;
}

uint32_t *o_encode_inc_dec(uint32_t *before, const char ** pProgram)
{
	int adjust = 0;
	while(1)
	{
		if(**pProgram == '+')
		{
			++adjust;
		}
		else if(**pProgram == '-')
		{
			--adjust;
		}
		else if((**pProgram == '>') || (**pProgram == '<') || (**pProgram == '[') || (**pProgram == ']') || (**pProgram == '.') || (**pProgram == ',') || (**pProgram == '\0'))
		{
			if(adjust == 0) return before;
			break;
		}
		++*pProgram;
	}
	*++before = LW(R_S0, R_T0,0x0);
	*++before = NOP;
	*++before = ADDIU(R_T0, R_T0, adjust);
	*++before = SW(R_T0, R_S0, 0x0);
	return before;
}

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
    //const char *sins = "[]";
    

	// s0 stack ptr
	const char *fmt = "%c";

	bool optimize = true;	

	//if(0)
	while(*sins != '\0')
	{
		switch(*sins)
		{
			case '>':
			if(!optimize)
			{
				// inc s0
			    *++dins = ADDIU(R_S0, R_S0, 0x4);
			}
			else
			{
				
				dins = o_encode_adjust_ptr(dins, &sins);
				continue;
			}
			break;
			case '<':
			if(!optimize)
			{
				// dec s0
			    *++dins = ADDIU(R_S0, R_S0, -0x4);  
			}
			else
			{
				dins = o_encode_adjust_ptr(dins, &sins);
				continue;
			}
			          
			break;
			case '+':
			if(!optimize)
			{
				// load value at s0. increment, and, store
			    *++dins = LW(R_S0, R_T0,0x0);
			    *++dins = NOP;
			    *++dins = ADDIU(R_T0, R_T0, 0x1);
			    *++dins = SW(R_T0, R_S0, 0x0);
			}
			else
			{
				dins = o_encode_inc_dec(dins, &sins);
				continue;
			}
			
			break;
			case '-':
			if(!optimize)
			{
				// load value at s0. decrement, and, store
			    *++dins = LW(R_S0, R_T0,0x0);
			    *++dins = NOP;
			    *++dins = ADDIU(R_T0, R_T0, -0x1);
			    *++dins = SW(R_T0, R_S0, 0x0);
			}
			else
			{
				dins = o_encode_inc_dec(dins, &sins);
				continue;
			}
			
			break;
			case '.':
			// printf
			/**++dins = LW(R_S0, R_A1, 0x0);
			*++dins = LUI(R_A0, ((uint32_t)fmt) >> 16);
			*++dins = JAL(printfaddr);
			*++dins = ADDIU(R_A0, R_A0, ((uint32_t)fmt)&0xFFFF);*/
		    // syscall_putchar            
			*++dins = LW(R_S0, R_A0, 0x0);
			*++dins = ADDIU(R_00, R_V0, 0xB0);
			*++dins = JALR(R_V0);
			*++dins = ADDIU(R_00, R_T1, 0x3D);
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
			// only load value if necessary
			if(!optimize || (*dins != SW(R_T0, R_S0, 0x0)))
			{
				// load value at s0
			    *++dins = LW(R_S0, R_T0, 0x0);
			    *++dins = NOP;	
			}
					
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
			*++dins = NOP;
			lines = (((uint32_t)(++dins)) -  braces[bracecnt-1])/ 4;			
			*(uint32_t*)braces[--bracecnt] = BEQ(R_T0, R_00, lines+1);
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
	ramsyscall_printf("inst end 0x%X\n", ++dins);
	const uint32_t delta = (uint32_t)dins-(uint32_t)&MEMORY;
	ramsyscall_printf("ins len %u\n", delta);

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

// Thanks Shendo http://www.psxdev.net/forum/viewtopic.php?t=384

/*Bitrate reload factor*/
#define SIO_REL_STOP	0
#define SIO_REL_MUL1	1
#define SIO_REL_MUL16	2
#define SIO_REL_MUL64	3

/*Bit lenght*/
#define SIO_DATA_LEN_5	0
#define SIO_DATA_LEN_6	1
#define SIO_DATA_LEN_7	2
#define SIO_DATA_LEN_8	3

/*Stop bit*/
#define SIO_STOP_BIT_1	1
#define SIO_STOP_BIT_15	2	/*1.5*/
#define SIO_STOP_BIT_2	3

/*Parity*/
#define SIO_PARITY_NONE	0
#define SIO_PARITY_ODD	1	/*ODD/EVEN are set to vice-versa according to Martin's docs*/
#define SIO_PARITY_EVEN	3

/*Pointers to SIO registers*/
volatile static unsigned char *SIO_TX_RX = (unsigned char*)0x1F801050;	/*Read/Write*/
volatile static unsigned short *SIO_STAT = (unsigned short*)0x1F801054;	/*Read only*/
volatile static unsigned short *SIO_MODE = (unsigned short*)0x1F801058;	/*Read/Write*/
volatile static unsigned short *SIO_CTRL = (unsigned short*)0x1F80105A;	/*Read/Write*/
volatile static unsigned short *SIO_BPSV = (unsigned short*)0x1F80105E;	/*Read/Write*/

/*
 * Initialize SIO communication at the specified bitrate.
 * Mode is 8N1.
 */
void StartSIO(int bitrate);

/*
 * Same as StartSIO but with more control.
 * IMPORTANT: Must use defined macros.
 * For example setting datalenght to 5 should be done with "SIO_DATA_LEN_5"
 * and not simply passing 5 as an argument.
 */
void StartSIOEx(int bitrate, int datalenght, int parity, int stopbit);

/*
 * Shuts down SIO communication.
 */
void StopSIO();

/*
 * Read a single byte from the input buffer.
 * Direct access to register, doesn't use BIOS routines.
 */
unsigned char ReadByteSIO();

/*
 * Send a single byte to the output buffer.
 * Direct access to register, doesn't use BIOS routines.
 */
void SendByteSIO(unsigned char data);

/*
 * Check if any data is waiting in the input buffer.
 * Must be used when fetching data otherwise incorrect data could be read (usually 0x00).
 * 0 - No, 1 - Yes
 */
int CheckSIOInBuffer();

/*
 * Check if register is ready to send data (previos operation finished).
 * Must be used when sending data as output buffer is only 2 bytes long.
 * 0 - No, 1 - Yes.
 */
int CheckSIOOutBuffer();

void StartSIO(int bitrate)
{
	/*Set to 8N1 mode with desired bitrate*/
	StartSIOEx(bitrate, SIO_DATA_LEN_8, SIO_PARITY_NONE, SIO_STOP_BIT_1);
}

void StartSIOEx(int bitrate, int datalenght, int parity, int stopbit)
{
	/*Set SIO_MODE register, bitrate reload factor set to MUL16 by default*/
	*SIO_MODE = SIO_REL_MUL16 | (datalenght << 2) | (parity << 4) | (stopbit << 6);

	/*Reset SIO_CTRL register.*/
	*SIO_CTRL = 0;

	/*Set TX and RT to enabled, no handshaking signals.*/
	*SIO_CTRL = 1 | (1 << 2);

	/*Calculate bitrate reload value based on the given bitrate
	 * Reload = SystemClock (33 Mhz) / (Factor (MULI16) * bitrate)*/
	*SIO_BPSV = 0x204CC00 / (16 * bitrate);
}

void StopSIO()
{
	/*Set all SIO related registers to zero*/
	*SIO_MODE = 0;
	*SIO_CTRL = 0;
	*SIO_BPSV = 0;
}

unsigned char ReadByteSIO()
{
	return *(unsigned char*)SIO_TX_RX;
}

void SendByteSIO(unsigned char data)
{
	*SIO_TX_RX = data;
}

int CheckSIOInBuffer()
{
	/*Return status of RX FIFO*/
	return (*SIO_STAT & 0x2)>>1;
}

int CheckSIOOutBuffer()
{
	/*Return status of TX Ready flag*/
	return (*SIO_STAT & 0x4)>>1;
}


static __attribute__((always_inline)) int syscall_removeDevice(const char *device_name) {
    register int n asm("t1") = 0x48;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int (*)(const char *))0xb0)(device_name);
}

#include "common/psxlibc/device.h"

int KTTYAction(struct File * file, enum FileAction inMode)
{
    
    if( inMode != PSXWRITE )
	{
		return -1;
	}
        
    
    uint32_t transferLeft = file->count;
    uint8_t *readAddr = file->buffer;
	while(transferLeft > 0)
	{
		uint32_t bailout = 0;
        
        // SR_TXU | SR_TXRDY
        while( (*SIO_STAT & 0x05) == 0 ){
            if ( bailout++ > 8000 )
                break;
        }

        *SIO_TX_RX = *readAddr;
		--transferLeft;
		++readAddr;
	}
	return file->count;
}

void KTTYNull(){
    
}

int KTTYReturn0(){
    return 0;    
}

int KTTYReadWrite(struct File * file, void *buffer, int size)
{
	return 0;
}

struct DirEntry *KTTYfirstFile(struct File *file, const char *filename, struct DirEntry *entry)
{
	return NULL;
}
struct DirEntry *KTTYnextFile(struct File *file, struct DirEntry *entry)
{
	return NULL;
}

int KTTYformat(struct File *file)
{
	return 0;
}

const char devname[] = "tty";
const char devdesc[] = "SIO TTY";

static struct Device newtty  = {
	.name = (const char*)&devname,
	.flags = (PSXDTTYPE_CHAR | PSXDTTYPE_CONS),
	.blockSize = 0,
	.desc = (const char*)&devdesc,
	.init = &KTTYNull,
	.open = &KTTYReturn0,
	.action = &KTTYAction,
	.close = &KTTYReturn0,
	.ioctl = &KTTYReturn0,   
    .read  = &KTTYReadWrite,      
    .write = &KTTYReadWrite,      
    .erase = &KTTYNull,      
    .undelete = &KTTYNull,   
    .firstFile = &KTTYfirstFile,  
    .nextFile = &KTTYnextFile,   
    .format = &KTTYformat,     
    .chdir = &KTTYNull,      
    .rename = &KTTYNull,     
    .deinit = &KTTYNull,     
    .check = &KTTYNull
};

int main(void) {
	ramsyscall_printf("psx-brainfuck\n");
	interpret_program(HELLOWORLD, NULL);
    interpret_program(SQUARES, NULL);
	interpret_program(HEAD, HEADINPUT);
	compile(NULL, NULL);
	voidfunc memcall = (voidfunc)&MEMORY;
	memcall();		
	ramsyscall_printf("compiled ran\n");    
   
    /*char buf[] = "Got char X\r\n";
	while(1)
	{
		if(CheckSIOInBuffer())
		{
			char c = ReadByteSIO();
			//ramsyscall_printf("RECV char %c\n", c);
			buf[sizeof(buf)-4] = c;
			for(char *str = buf; *str != '\0'; ++str)
			{
				while(!CheckSIOOutBuffer());
				SendByteSIO(*str);
			}
		}	
	}*/

    // close STDIN and STDOUT
    syscall_close(0);
	syscall_close(1);

	// replace the TTY
	enterCriticalSection();		
	syscall_removeDevice(devname);
    syscall_addDevice(&newtty);    
	leaveCriticalSection();

    // reopen STDIN and STDOUT
	syscall_open(devname, PSXREAD);
	syscall_open(devname, PSXWRITE);

	ramsyscall_printf("tty installed\n");   
	
	while(1);
}
