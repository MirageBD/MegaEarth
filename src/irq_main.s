#include "constants.h"

			.rtmodel cpu, "*"

			.extern _Zp

			.extern modplay_play
			.extern program_update

			.extern sine

frame		.byte 0
bump		.byte 0

 ; ------------------------------------------------------------------------------------

			.public irq_main
irq_main:
			php
			pha
			phx
			phy
			phz

			lda #0x0f
			sta 0xd020
			sta 0xd021

			;lda #0x02
			;sta 0xd020

			inc frame

			jsr program_update

			;lda #0x01
			;sta 0xd020

			jsr initmaptexture
			jsr maptexture

			;lda #0x08
			;sta 0xd020

			jsr fillsinetables

			jsr initfillspherepositions
			jsr fillspherepositions

			;lda #0x0a
			;sta 0xd020

			jsr modplay_play

			;lda #0x06
			;sta 0xd020

			jsr initrenderbumps
			;jsr renderbumps
			jsr renderbumpline ; render first bump line
			jsr renderbumpline ; render first bump line

			;lda #0x0f
			;sta 0xd020

			lda #.byte0 (SCREEN + 0*RRBSCREENWIDTH2)
			sta 0xd060
			lda #.byte1 (SCREEN + 0*RRBSCREENWIDTH2)
			sta 0xd061

			lda #.byte0 (SCREEN + 1*RRBSCREENWIDTH2)
			sta scrptrlo+1
			lda #.byte1 (SCREEN + 1*RRBSCREENWIDTH2)
			sta scrptrhi+1

			lda #.byte0 COLOR_RAM_OFFSET
			sta 0xd064
			lda #.byte1 COLOR_RAM_OFFSET
			sta 0xd065

			lda #.byte0 (COLOR_RAM_OFFSET + 1*RRBSCREENWIDTH2)
			sta colptrlo+1
			lda #.byte1 (COLOR_RAM_OFFSET + 1*RRBSCREENWIDTH2)
			sta colptrhi+1

			lda #0x68						; reset textypos for start of screen
			sta 0xd04e
			lda #0x00
			sta 0xd04f

			lda #0x69+4
			sta textyposlo+1
			lda #0x00
			sta textyposhi+1

			lda #0x35
			sta 0xd012
			sta raster+1

			lda #.byte0 irq_main2
			sta 0xfffe
			lda #.byte1 irq_main2
			sta 0xffff

			plz
			ply
			plx
			pla
			plp
			asl 0xd019
			rti

; ------------------------------------------------------------------------------------

irq_main2:
			php
			pha
			phx
			phy
			phz

			;lda #0x01
			;sta 0xd020

			ldx #0xd0
wr1:		dex
			bne wr1

scrptrlo:	lda #.byte0 (SCREEN + 1*RRBSCREENWIDTH2)
			sta 0xd060
scrptrhi:	lda #.byte1 (SCREEN + 1*RRBSCREENWIDTH2)
			sta 0xd061

colptrlo:	lda #.byte0 (COLOR_RAM_OFFSET + 1*RRBSCREENWIDTH2)
			sta 0xd064
colptrhi:	lda #.byte1 (COLOR_RAM_OFFSET + 1*RRBSCREENWIDTH2)
			sta 0xd065

textyposlo:	lda #0x67
			sta 0xd04e
textyposhi:	lda #0x00
			sta 0xd04f

			clc
			lda scrptrlo+1
			adc #.byte0 RRBSCREENWIDTH2
			sta scrptrlo+1
			lda scrptrhi+1
			adc #.byte1 RRBSCREENWIDTH2
			sta scrptrhi+1

			clc
			lda colptrlo+1
			adc #.byte0 RRBSCREENWIDTH2
			sta colptrlo+1
			lda colptrhi+1
			adc #.byte1 RRBSCREENWIDTH2
			sta colptrhi+1

			clc
			lda textyposlo+1
			adc #8
			sta textyposlo+1
			lda textyposhi+1
			adc #0x00
			sta textyposhi+1

			;lda #0x0f
			;sta 0xd020

			clc
			lda raster+1
			adc #4
			sta raster+1
			cmp #0x35+50*4
			beq endloop

			;lda #0x02
			;sta 0xd020
			jsr renderbumpline
			;lda #0x0f
			;sta 0xd020

			;jmp endloop

raster:		lda #0x34
			sta 0xd012

			plz
			ply
			plx
			pla
			plp
			asl 0xd019
			rti

endloop:	lda #0xff
			sta 0xd012

			lda #.byte0 irq_main
			sta 0xfffe
			lda #.byte1 irq_main
			sta 0xffff

			plz
			ply
			plx
			pla
			plp
			asl 0xd019
			rti

; ------------------------------------------------------------------------------------

copypositionline:

			sta 0xd707							; inline DMA copy
			.byte 0x80, 0						; sourcemb
			.byte 0x81, 0						; destmb
			.byte 0x85, 4						; dst skip rate
			.byte 0x00							; end of job options
			.byte 0x00							; copy, no chain
			.word 64							; count 128-1 because I don't want to touch the last gotox320 bytes
cppsrc1:	.word 0								; src
			.byte (SLICESINUSES >> 16)			; src bank
cppdst1:	.word SCREEN+SCREENWIDTH			; dst
			.byte 0x00							; dst bank
			.byte 0x00							; cmd hi
			.word 0x0000						; modulo, ignored

			; see http://c65.lgb.hu/dma.html for explanation of SRC_DEC bit in F018B DMA format

			sta 0xd707							; inline DMA copy
			.byte 0x80, 0						; sourcemb
			.byte 0x81, 0						; destmb
			.byte 0x85, 4						; dst skip rate
			.byte 0x00							; end of job options
			.byte 0b00010000					; copy, decrease source, no chain
			.word 63							; count 128-1 because I don't want to touch the last gotox320 bytes
cppsrc2:	.word 0								; src
			.byte (SLICESINUSES >> 16)			; src bank and flags | 0b01000000 = reverse direction
cppdst2:	.word SCREEN+SCREENWIDTH			; dst
			.byte 0x00							; dst bank and flags
			.byte 0x00							; cmd hi
			.word 0x0000						; modulo, ignored

			rts

; ------------------------------------------------------------------------------------

copytextureline:

			sta 0xd707							; inline DMA copy
			.byte 0x80, 0						; sourcemb
			.byte 0x81, 0						; destmb
			.byte 0x85, 4						; dst skip rate
			.byte 0x00							; end of job options
			.byte 0x00							; copy, no chain
			.word 64							; count 128-1 because I don't want to touch the last gotox320 bytes
cptsrc1:	.word 0								; src (fill value)
			.byte (TEXTUREMEM >> 16)			; src bank
cptdst1:	.word SCREEN+SCREENWIDTH			; dst
			.byte 0x00							; dst bank
			.byte 0x00							; cmd hi
			.word 0x0000						; modulo, ignored

			sta 0xd707							; inline DMA copy
			.byte 0x80, 0						; sourcemb
			.byte 0x81, 0						; destmb
			.byte 0x85, 4						; dst skip rate
			.byte 0x00							; end of job options
			.byte 0b00010000					; copy, decrease source, no chain
			.word 63							; count 128-1 because I don't want to touch the last gotox320 bytes
cptsrc2:	.word 0								; src (fill value)
			.byte (TEXTUREMEM >> 16)			; src bank
cptdst2:	.word SCREEN+SCREENWIDTH			; dst
			.byte 0x00							; dst bank
			.byte 0x00							; cmd hi
			.word 0x0000						; modulo, ignored

			rts

; ------------------------------------------------------------------------------------

initmaptexture:

			lda frame
			sta cptsrc1+0
			lda #0
			sta cptsrc1+1

			lda #.byte0 (SCREEN+SCREENWIDTH2+2)
			sta cptdst1+0
			lda #.byte1 (SCREEN+SCREENWIDTH2+2)
			sta cptdst1+1

			clc
			lda frame
			adc #127
			sta cptsrc2+0
			lda #0
			adc #0
			sta cptsrc2+1

			lda #.byte0 (SCREEN+SCREENWIDTH2+2+4*64)
			sta cptdst2+0
			lda #.byte1 (SCREEN+SCREENWIDTH2+2+4*64)
			sta cptdst2+1

			rts

; ------------------------------------------------------------------------------------

maptexture:

			ldx #47

copytxtlineloop:
			jsr copytextureline

			clc
			lda cptdst1+0
			adc #.byte0 RRBSCREENWIDTH2
			sta cptdst1+0
			lda cptdst1+1
			adc #.byte1 RRBSCREENWIDTH2
			sta cptdst1+1

			inc cptsrc1+1
			inc cptsrc1+1

			clc
			lda cptdst2+0
			adc #.byte0 RRBSCREENWIDTH2
			sta cptdst2+0
			lda cptdst2+1
			adc #.byte1 RRBSCREENWIDTH2
			sta cptdst2+1

			inc cptsrc2+1
			inc cptsrc2+1

			dex
			bpl copytxtlineloop

			rts

; ------------------------------------------------------------------------------------

		.public initfillspherepositions
initfillspherepositions:

			lda #0
			sta cppsrc1+0
			lda #0
			sta cppsrc1+1

			lda #.byte0 (SCREEN+SCREENWIDTH2)
			sta cppdst1+0
			lda #.byte1 (SCREEN+SCREENWIDTH2)
			sta cppdst1+1

			lda #127
			sta cppsrc2+0
			lda #0
			sta cppsrc2+1

			lda #.byte0 (SCREEN+SCREENWIDTH2+4*64)
			sta cppdst2+0
			lda #.byte1 (SCREEN+SCREENWIDTH2+4*64)
			sta cppdst2+1

			rts

; ------------------------------------------------------------------------------------

		.public fillspherepositions
fillspherepositions

			ldx #47

copyposlineloop:
			jsr copypositionline

			clc
			lda cppdst1+0
			adc #.byte0 RRBSCREENWIDTH2
			sta cppdst1+0
			lda cppdst1+1
			adc #.byte1 RRBSCREENWIDTH2
			sta cppdst1+1

			inc cppsrc1+1

			clc
			lda cppdst2+0
			adc #.byte0 RRBSCREENWIDTH2
			sta cppdst2+0
			lda cppdst2+1
			adc #.byte1 RRBSCREENWIDTH2
			sta cppdst2+1

			inc cppsrc2+1

			dex
			bpl copyposlineloop

			rts

; ------------------------------------------------------------------------------------

		.public fillsinetables
fillsinetables:

			lda #0
			sta 0xd770
			sta 0xd771
			sta 0xd772
			sta 0xd773
			sta 0xd774
			sta 0xd775
			sta 0xd776
			sta 0xd777
			sta cpsdst+0
			sta cpsdst+1

			ldx #47
fillsineouterloop:

			lda spherediam,x
			sta 0xd774
			lsr a
			sta diamhalf
			sec
			lda #128; +24
			sbc diamhalf
			sta diamoffset

			ldy #127
fillsineloop:
			lda sine,y
			sta 0xd770
			lda 0xd779
			;clc
			adc diamoffset
			sta SINETEMP+0x0000,y
			dey
			bpl fillsineloop

			sta 0xd707							; inline DMA copy
			.byte 0x80, 0						; sourcemb
			.byte 0x81, 0						; destmb
			.byte 0x00							; end of job options
			.byte 0x00							; copy, no chain
			.word 128							; count
			.word 0xe000						; src (fill value)
			.byte 0x00							; src bank (ignored)
cpsdst:		.word 0								; dst
			.byte (SLICESINUSES >> 16)			; dst bank
			.byte 0x00							; cmd hi
			.word 0x0000						; modulo, ignored

			inc cpsdst+1

			dex
			bpl fillsineouterloop
			rts

; ------------------------------------------------------------------------------------

initrenderbumps:

			lda #0
			sta 0xd770
			sta 0xd771
			sta 0xd772
			sta 0xd773

			sta 0xd774
			sta 0xd775
			sta 0xd776
			sta 0xd777

			lda #0xff
			sta 0xd770

			lda #.byte0 (SCREEN + SCREENWIDTH2) ; + y*RRBSCREENWIDTH2
			sta zp:_Zp+242
			lda #.byte1 (SCREEN + SCREENWIDTH2) ; + y*RRBSCREENWIDTH2
			sta zp:_Zp+243

			lda #.byte0 (SCREEN + SCREENWIDTH2 + 256) ; + y*RRBSCREENWIDTH2
			sta zp:_Zp+244
			lda #.byte1 (SCREEN + SCREENWIDTH2 + 256) ; + y*RRBSCREENWIDTH2
			sta zp:_Zp+245

			lda #.byte0 BUMPMEM
			sta zp:_Zp+234
			lda #.byte1 BUMPMEM
			sta zp:_Zp+235
			lda #.byte2 BUMPMEM
			sta zp:_Zp+236
			lda #.byte3 BUMPMEM
			sta zp:_Zp+237

			rts

; ------------------------------------------------------------------------------------

renderbumpline:

			ldy #0
			ldz frame
bumpleftloop:
			tya
			eor #0xff
			sta 0xd770
			lda [zp:_Zp+234],z
			sta 0xd774
			sec
			lda (zp:_Zp+242),y
			sbc 0xd779
			clc
			adc #1
			sta (zp:_Zp+242),y
			inz
			iny
			iny
			iny
			iny
			bne bumpleftloop

			tza
			clc
			adc #64
			taz
bumprightloop:
			tya
			eor #0xff
			sta 0xd770
			lda [zp:_Zp+234],z
			sta 0xd774
			clc
			lda (zp:_Zp+244),y
			adc 0xd779
			sec
			sbc #1
			sta (zp:_Zp+244),y
			dez
			iny
			iny
			iny
			iny
			bne bumprightloop

			clc
			lda zp:_Zp+242
			adc #.byte0 RRBSCREENWIDTH2
			sta zp:_Zp+242
			lda zp:_Zp+243
			adc #.byte1 RRBSCREENWIDTH2
			sta zp:_Zp+243

			clc
			lda zp:_Zp+244
			adc #.byte0 RRBSCREENWIDTH2
			sta zp:_Zp+244
			lda zp:_Zp+245
			adc #.byte1 RRBSCREENWIDTH2
			sta zp:_Zp+245

			inc zp:_Zp+235
			inc zp:_Zp+235

			rts

; ------------------------------------------------------------------------------------

renderbumps:

			ldx #0
bumpouterloop:
			inc 0xd020
			jsr renderbumpline
			inx
			cpx #30
			bne bumpouterloop

			rts

; ------------------------------------------------------------------------------------

diamhalf	.byte 0
diamoffset	.byte 0

spherediam
			;.byte  47,  86, 111, 131, 148, 161, 174, 185, 194, 203, 210, 217, 224, 229, 234, 238, 242, 245, 248, 250, 252, 254, 254, 255
			;.byte 255, 254, 254, 252, 250, 248, 245, 242, 238, 234, 229, 224, 217, 210, 203, 194, 185, 174, 161, 148, 131, 111,  86,  47

			.byte 41, 70, 89, 104, 117, 127, 137, 145, 153, 159, 165, 171, 176, 180, 184, 187, 190, 193, 195, 196, 198, 199, 200, 200
			.byte 200, 200, 199, 198, 196, 195, 193, 190, 187, 184, 180, 176, 171, 165, 159, 153, 145, 137, 127, 117, 104, 89, 70, 41
