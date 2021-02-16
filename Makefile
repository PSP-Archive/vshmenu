release: all
	psp-packer $(TARGET).prx
	bin2c $(TARGET).prx $(TARGET)_bin.h $(TARGET)
	rm *.elf *.o


TARGET = satelite
OBJS = main.o blit.o ui.o menu.o clock.o stubkk.o cache.o exports.o 
#cache.o  pluginmgr.o

BUILD_PRX=1

INCDIR = 
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

PRX_EXPORTS = exports.exp

LIBDIR =
LDFLAGS =

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
