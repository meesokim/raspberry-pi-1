
all: kernel.img

include ../kernel/common.mk

include ../libvorbis/pkgconfig.mk
include ../libpng/pkgconfig.mk
include ../SDL2/pkgconfig.mk
include ../SDL2_image/pkgconfig.mk
include ../SDL2_mixer/pkgconfig.mk

OBJS = \
	data/enemies.o data/map.o \
	graphics/blueparch.o graphics/fonts.o graphics/history.o graphics/intro.o graphics/parchment2.o \
	graphics/parchment4.o graphics/parchment6.o graphics/redparch.o graphics/ending.o graphics/gameover.o \
	graphics/intromd.o graphics/parchment1.o graphics/parchment3.o graphics/parchment5.o graphics/parchment.o \
	graphics/tiles.o \
	sounds/AreaIChurchN.o sounds/doorfx.o sounds/HangmansTree.o sounds/MainTitleN.o sounds/mechanismn.o sounds/shoot.o \
	sounds/AreaIICavesV2N.o sounds/EvilFightN.o sounds/Item.o sounds/ManhuntN.o sounds/onedeathn.o sounds/slash.o \
	sounds/AreaIIIHellN.o sounds/GameOverV2N.o sounds/jump.o sounds/ManhuntwoodN.o sounds/PrayerofHopeN.o \
	src/drawing.o src/ending.o src/enemies.o src/game.o src/gameover.o src/history.o src/jean.o src/loading.o src/startscreen.o \
	src/main.o src/dummy.o \
	src/spc1000.o src/mc6847.o src/keyboard.o src/cpu.o src/cassette.o src/spcall.o src/emu2149.o src/ugui.o src/stub.o \
	src/miniz/miniz_zip.o \
	src/bzip2/blocksort.o src/bzip2/bzlib.o src/bzip2/compress.o src/bzip2/crctable.o src/bzip2/decompress.o src/bzip2/huffman.o src/bzip2/randtable.o

CFLAGS += -finline-functions -funswitch-loops -fpredictive-commoning -fgcse-after-reload -ftree-vectorize -Isrc -Isrc/miniz -Isrc/bzip2 -ffunction-sections -fdata-sections -mno-unaligned-access -D__circle__
CPPFLAGS = $(filter-out -ffreestanding, $(CFLAGS)) -fno-exceptions -fno-unwind-tables -fno-rtti -std=c++17 -fpermissive
LDFLAGS += -Wl,--gc-sections

-include $(DEPDIR)/*.Po

kernel.img: $(OBJS) Makefile ../kernel/raspberry.ld $(LIBS_DEP)
	$(CXX) $(LDFLAGS) -o kernel.elf $(OBJS) $(LIBS)
	$(OBJDUMP) -d kernel.elf | $(TOOLCHAIN)c++filt > kernel.lst
	$(OBJCOPY) kernel.elf -O binary kernel.img

%.o: %.png
	$(LD) -r -b binary -o $@ $<

%.o: %.ogg
	$(LD) -r -b binary -o $@ $<

%.o: %.txt
	$(LD) -r -b binary -o $@ $<

clean:
	rm -f $(OBJS) kernel.elf kernel.img kernel.map
	rm -rf $(DEPDIR)
