#include <stdint.h>
#include "macros.h"
#include "registers.h"
#include "constants.h"
#include "modplay.h"
#include "iffl.h"
#include "irqload.h"
#include "dma.h"
#include "dmajobs.h"
#include "program.h"

extern void irq_main();
extern void fillsinetables();
extern void initfillspherepositions();
extern void fillspherepositions();

void program_mapcolourmem()
{
	__asm(  // MAP_MEMORY_IN1MB %0000, %1111
		" lda #0xff\n"
		" ldx #0b00000000\n"                // %0000
		" ldy #0xff\n"
		" ldz #0b00001111\n"
		" map"
	);

	// SAFE_COLOR_RAM       = 0xff80000 + 0x0800 = 0xff80800
	// SAFE_COLOR_RAM_IN1MB                      = 0x0080800
	// 80800 - $8000 = 0x78800
	// 0x78800 / 256 = 0x788    -> Y = 0x88, Z = 0x07

	__asm(  //	MAP_MEMORY $00000, %0000, SAFE_COLOR_RAM_IN1MB, %0001
		" lda #0x00\n"          // lda #<((offsetlower32kb) / 256)
		" ldx #0b00000000\n"    // ldx #>((offsetlower32kb) / 256) | (enablemasklower32kb << 4)
		" ldy #0x88\n"          // #<((offsetupper32kb - $8000) / 256)
		" ldz #0x17\n"          // #>((offsetupper32kb - $8000) / 256) | (enablemaskupper32kb << 4)
		" map\n"
		" eom\n"
	);
}

void program_unmapcolourmem()
{
	UNMAP_ALL
}
void program_loaddata()
{
	fl_init();
	fl_waiting();
	floppy_iffl_fast_load_init("MEGAVOXL.DAT");
	floppy_iffl_fast_load(); // pal
	floppy_iffl_fast_load(); // earth chars
	floppy_iffl_fast_load(); // spheregrad values
	floppy_iffl_fast_load(); // NCM voxel chars
	floppy_iffl_fast_load(); // bump values
	floppy_iffl_fast_load(); // song
	fl_exit();
}

void program_init()
{
	VIC2.BORDERCOL = 0x00;
	VIC2.SCREENCOL = 0x00;

	VIC2.DEN = 0; // disable display

	modplay_init();
	modplay_initmod(ATTICADDRESS, SAMPLEADRESS);
	modplay_enable();
	
	dma_runjob((__far char *)&dma_clearcolorram1);
	dma_runjob((__far char *)&dma_clearcolorram2);
	dma_runjob((__far char *)&dma_copypalette);

	// render the first char for the background layer
	uint16_t i = (GFXMEM / 64) + 16;
	for(uint16_t y=0; y<50; y++)
	{
		for(uint16_t x=0; x<SCREENWIDTH; x++)
		{
			lpoke(SCREEN + y*RRBSCREENWIDTH2 + 2*x + 0, (i >> 0) & 0xff);
			lpoke(SCREEN + y*RRBSCREENWIDTH2 + 2*x + 1, (i >> 8) & 0xff);
		}
	}

	// render the sprites
	i = (GFXMEM / 64);
	for(uint16_t y=0; y<50; y++)
	{
		uint32_t colptr = SAFE_COLOR_RAM + SCREENWIDTH2 + y*RRBSCREENWIDTH2;
		uint16_t scrptr = SCREEN         + SCREENWIDTH2 + y*RRBSCREENWIDTH2;
		for(uint16_t x=0; x<RRBSPRITES; x++)
		{
			uint8_t g = lpeek(GRADIENTMEM+y*128+x);

			lpoke(colptr + 4*x + 0, 0b10010000); // set gotox and transparency
			lpoke(colptr + 4*x + 1, 0); // pixel row mask flags
			poke(scrptr + 4*x + 0, 0);
			poke(scrptr + 4*x + 1, 0);

			lpoke(colptr + 4*x + 2, 0b00001100); // set to NCM mode and trim pixels
			lpoke(colptr + 4*x + 3, g<<4); // set palette
			poke(scrptr + 4*x + 2, 0);
			poke(scrptr + 4*x + 3, ((i >> 8) & 0xff));
		}
	}

	// set last gotox to 320
	for(uint16_t y=0; y<50; y++)
	{
		lpoke(SCREEN + SCREENWIDTH2 + y*RRBSCREENWIDTH2 + 4*(RRBSPRITES-1) + 0, (320 >> 0) & 0xff);
		lpoke(SCREEN + SCREENWIDTH2 + y*RRBSCREENWIDTH2 + 4*(RRBSPRITES-1) + 1, (320 >> 8) & 0xff);
	}

	fillsinetables();
	initfillspherepositions();
	fillspherepositions();

	VIC2.DEN = 1; // enable display
}

void program_update()
{
}