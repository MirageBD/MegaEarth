# -----------------------------------------------------------------------------

megabuild		= 1
attachdebugger	= 0

# -----------------------------------------------------------------------------

MAKE			= make
CP				= cp
MV				= mv
RM				= rm -f
CAT				= cat

SRC_DIR			= ./src
EXE_DIR			= ./exe
BIN_DIR			= ./bin

CC1541			= cc1541
MC				= MegaConvert
MEGAADDRESS		= megatool -a
MEGACRUNCH		= megatool -c
MEGAIFFL		= megatool -i
EL				= etherload
XMEGA65			= D:\PCTOOLS\xemu\xmega65.exe
MEGAFTP			= mega65_ftp -e

.SUFFIXES: .o .s .out .bin .pu .b2 .a

default: all

VPATH = src

# Common source files
ASM_SRCS = decruncher.s iffl.s irqload.s irq_fastload.s irq_main.s startup.s program_asm.s
C_SRCS = main.c dma.c modplay.c dmajobs.c program.c

OBJS = $(ASM_SRCS:%.s=$(EXE_DIR)/%.o) $(C_SRCS:%.c=$(EXE_DIR)/%.o)
OBJS_DEBUG = $(ASM_SRCS:%.s=$(EXE_DIR)/%-debug.o) $(C_SRCS:%.c=$(EXE_DIR)/%-debug.o)

BINFILES  = $(BIN_DIR)/gfx_pal0.bin
BINFILES += $(BIN_DIR)/earth_chars0.bin
BINFILES += $(BIN_DIR)/spheregrad_chars0.bin
BINFILES += $(BIN_DIR)/gfx_chars0.bin
BINFILES += $(BIN_DIR)/bump_chars0.bin
BINFILES += $(BIN_DIR)/song.mod

BINFILESMC  = $(BIN_DIR)/gfx_pal0.bin.addr.mc
BINFILESMC += $(BIN_DIR)/earth_chars0.bin.addr.mc
BINFILESMC += $(BIN_DIR)/spheregrad_chars0.bin.addr.mc
BINFILESMC += $(BIN_DIR)/gfx_chars0.bin.addr.mc
BINFILESMC += $(BIN_DIR)/bump_chars0.bin.addr.mc
BINFILESMC += $(BIN_DIR)/song.mod.addr.mc

# -----------------------------------------------------------------------------

# charmode  = 1 = SuperExtendedAttributeMode
# direction = 2 = PixelLeftRightTopBottom
$(BIN_DIR)/earth_chars0.bin: $(BIN_DIR)/earth.bin
	$(MC) $< cm1:1 d1:2 cl1:10000 rc1:0

# charmode  = 1 = SuperExtendedAttributeMode
# direction = 2 = PixelLeftRightTopBottom
$(BIN_DIR)/spheregrad_chars0.bin: $(BIN_DIR)/spheregrad.bin
	$(MC) $< cm1:1 d1:2 cl1:16000 rc1:0

# charmode  = 1 = SuperExtendedAttributeMode
# direction = 2 = PixelLeftRightTopBottom
$(BIN_DIR)/bump_chars0.bin: $(BIN_DIR)/bump.bin
	$(MC) $< cm1:1 d1:2 cl1:18000 rc1:0

# charmode  = 2 = NibbleColour
# direction = 0 = CharLeftRightTopBottom
$(BIN_DIR)/gfx_pal0.bin: $(BIN_DIR)/gfx.bin
	$(MC) $< cm1:2 d1:0 cl1:1c000 rc1:0

$(BIN_DIR)/alldata.bin: $(BINFILES)
	$(MEGAADDRESS) $(BIN_DIR)/gfx_pal0.bin           0000cc00
	$(MEGAADDRESS) $(BIN_DIR)/earth_chars0.bin       00010000
	$(MEGAADDRESS) $(BIN_DIR)/spheregrad_chars0.bin  00016000
	$(MEGAADDRESS) $(BIN_DIR)/gfx_chars0.bin         00018000
	$(MEGAADDRESS) $(BIN_DIR)/bump_chars0.bin        00019000
	$(MEGAADDRESS) $(BIN_DIR)/song.mod               08000000
	$(MEGACRUNCH) $(BIN_DIR)/gfx_pal0.bin.addr
	$(MEGACRUNCH) $(BIN_DIR)/earth_chars0.bin.addr
	$(MEGACRUNCH) $(BIN_DIR)/spheregrad_chars0.bin.addr
	$(MEGACRUNCH) $(BIN_DIR)/gfx_chars0.bin.addr
	$(MEGACRUNCH) $(BIN_DIR)/bump_chars0.bin.addr
	$(MEGACRUNCH) $(BIN_DIR)/song.mod.addr
	$(MEGAIFFL) $(BINFILESMC) $(BIN_DIR)/alldata.bin

$(EXE_DIR)/%.o: %.s
	as6502 --target=mega65 --list-file=$(@:%.o=%.lst) -o $@ $<

$(EXE_DIR)/%.o: %.c
	cc6502 --target=mega65 -O2 --list-file=$(@:%.o=%.lst) -o $@ $<

$(EXE_DIR)/%-debug.o: %.s
	as6502 --target=mega65 --debug --list-file=$(@:%.o=%.lst) -o $@ $<

$(EXE_DIR)/%-debug.o: %.c
	cc6502 --target=mega65 --debug --list-file=$(@:%.o=%.lst) -o $@ $<

# there are multiple places that need to be changed for the start address:
# ln6502 command line option --load-address 0x1001
# megacrunch start address -f 100e
# scm file   address (#x1001) section (programStart #x1001)

$(EXE_DIR)/megavoxl.prg: $(OBJS)
	ln6502 --target=mega65 mega65-custom.scm -o $@ $^ --load-address 0x1200 --raw-multiple-memories --cstartup=mystartup --rtattr printf=nofloat --rtattr exit=simplified --output-format=prg --list-file=$(EXE_DIR)/cprog.lst

$(EXE_DIR)/megavoxl.prg.mc: $(EXE_DIR)/megavoxl.prg
	$(MEGACRUNCH) -f 1200 $(EXE_DIR)/megavoxl.prg

# -----------------------------------------------------------------------------

$(EXE_DIR)/megavoxl.d81: $(EXE_DIR)/megavoxl.prg.mc  $(BIN_DIR)/alldata.bin
	$(RM) $@
	$(CC1541) -n "megavoxl" -i " 2025" -d 19 -v\
	 \
	 -f "megavoxl"      -w $(EXE_DIR)/megavoxl.prg.mc \
	 -f "megavoxl.dat"  -w $(BIN_DIR)/alldata.bin \
	$@

# -----------------------------------------------------------------------------

run: $(EXE_DIR)/megavoxl.d81

ifeq ($(megabuild), 1)
	$(MEGAFTP) -c "put .\exe\megavoxl.d81 megavoxl.d81" -c "quit"
	$(EL) -m MEGAVOXL.D81 -r $(EXE_DIR)/megavoxl.prg.mc
ifeq ($(attachdebugger), 1)
	m65dbg --device /dev/ttyS2
endif
else
ifeq ($(attachdebugger), 1)
	cmd.exe /c "$(XMEGA65) -uartmon :4510 -autoload -8 $(EXE_DIR)/megavoxl.d81" & m65dbg -l tcp 4510
else
	cmd.exe /c "$(XMEGA65) -autoload -8 $(EXE_DIR)/megavoxl.d81"
endif
endif

clean:
	-rm -f $(OBJS) $(OBJS:%.o=%.clst) $(OBJS_DEBUG) $(OBJS_DEBUG:%.o=%.clst) $(BIN_DIR)/*_*.bin
	-rm -f $(EXE_DIR)/megavoxl.d81 $(EXE_DIR)/megavoxl.elf $(EXE_DIR)/megavoxl.prg $(EXE_DIR)/megavoxl.prg.mc $(EXE_DIR)/megavoxl.lst $(EXE_DIR)/megavoxl-debug.lst
