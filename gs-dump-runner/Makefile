# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

include ../Makefile.destdir

EE_CFLAGS += -fno-builtin-printf
EE_LIBS = -ldraw -lgraph -lmath3d -lpacket -ldma

all: runner.elf

runner.elf: my_read.o pcsx2_dump.o main.o 
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

pcsx2_dump.c: pcsx2_dump.gs
	bin2c pcsx2_dump.gs pcsx2_dump.c pcsx2_dump

pcsx2_dump.o: pcsx2_dump.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c $< -o $@

my_read.o: my_read.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c $< -o $@

main.o: main.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c $< -o $@

clean:
	rm -f runner.elf main.o my_read.o pcsx2_dump.o

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
