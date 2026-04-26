#include "macros.h"
#include "registers.h"
#include "constants.h"
#include "dmajobs.h"
#include "program.h"

extern void irq_fastload();
extern void irq_main();

void main()
{
	SEI

	CPU.PORT = 0b00110101;										// 0x35 = I/O area visible at $D000-$DFFF, RAM visible at $A000-$BFFF and $E000-$FFFF.
	VIC4.HOTREG = 0;											// disable hot registers
	UNMAP_ALL													// unmap any mappings
	CPU.PORTDDR = 65;											// enable 40Hz
	VIC3.KEY = 0x47;											// Enable the VIC4
	VIC3.KEY = 0x53;
	EOM

	dma_init();

	CIA1.ICR = 0b01111111;										// disable interrupts
	CIA2.ICR = 0b01111111;
	CIA1.ICR;
	CIA2.ICR;
	poke(0xd01a,0x00);											// disable IRQ raster interrupts because C65 uses raster interrupts in the ROM
	VIC2.RC = 0x08;												// d012 = 0
	VIC2.RC8 = 0X00;											// d011
	IRQ_VECTORS.IRQ = (volatile uint16_t)&irq_fastload;			// set irq vector
	poke(0xd01a,0x01);											// ACK!

	CLI

	program_loaddata();

	SEI

	CPU.PORT			= 0b00110101;							// 0x35 = I/O area visible at $D000-$DFFF, RAM visible at $A000-$BFFF and $E000-$FFFF.
	
	VIC3.ROM8			= 0;									// map I/O (Mega65 memory mapping)
	VIC3.ROMA			= 0;
	VIC3.ROMC			= 0;
	VIC3.CROM9			= 0;
	VIC3.ROME			= 0;

	VIC4.FNRST			= 0;									// disable raster interrupts
	VIC4.FNRSTCMP		= 0;
	VIC4.CHR16			= 1;									// use wide character lookup (i.e. character data anywhere in memory)

	VIC4.TEXTXPOSLSB	= 0x88;									// offset TEXTXPOS to center sphere
	VIC4.SDBDRWDLSB		= 0x58;									// set border to $58 instead of $50 to hide RRB bugs at right side of screen
	
	VIC2.MCM			= 1;									// set multicolor mode
	VIC4.FCLRLO			= 1;									// lower block, i.e. 0-255		// use NCM and FCM for all characters
	VIC4.FCLRHI			= 1;									// everything above 255
	
	VIC3.H640			= 0;									// enable 640 horizontal width
	VIC4.CHRXSCL		= 0x78;

	VIC4.TBDRPOSLSB		= 20;

	VIC4.BBDRPOSLSB		= 231; // was 240. set to 231 to hide lower bugs for now
	VIC4.BBDRPOSMSB		= 1;

	// to maximize RRB:
	// DISABLE RRB double buffering		NORRDEL	= 0
	// ENABLE RRB double height			DBLRR	= 1
	// ENABLE V400						V400	= 1
	// SET YSCALE to 0					CHRYSCL	= 0

	VIC4.NORRDEL		= 0;									// enable rrb double buffering
	VIC4.DBLRR			= 1;									// disable double-height rrb
	VIC3.V400			= 1;									// enable 400 vertical height
	VIC4.CHRYSCL		= 0;

	VIC4.DISPROWS		= 25;									// display 50 rows of text

	VIC4.SCRNPTR		= (SCREEN & 0xffff);					// set screen pointer
	VIC4.SCRNPTRBNK		= (SCREEN & 0xf0000) >> 16;
	VIC4.SCRNPTRMB		= 0;

	VIC4.CHRCOUNTLSB	= (RRBSCREENWIDTH >> 0) & 0xff;				// RRBSCREENWIDTH; // 64 works, 65 does not
	VIC4.CHRCOUNTMSB	= (RRBSCREENWIDTH >> 8) & 0xff;					// set RRB screenwidth and linestep
	VIC4.LINESTEP		= RRBSCREENWIDTH2;

	VIC4.COLPTR			= COLOR_RAM_OFFSET;						// set offset to colour ram, so we can use continuous memory

	program_init();

	CIA1.ICR = 0b01111111;										// disable interrupts
	CIA2.ICR = 0b01111111;
	CIA1.ICR;
	CIA2.ICR;
	poke(0xd01a,0x00);											// disable IRQ raster interrupts because C65 uses raster interrupts in the ROM
	VIC2.RC = 0x20;												// d012 = 8
	VIC2.RC8 = 0X00;											// d011
	IRQ_VECTORS.IRQ = (volatile uint16_t)&irq_main;
	poke(0xd01a,0x01);											// ACK!

	CLI

	program_mainloop();
}
