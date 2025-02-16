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

runner.elf: runner.o pcsx2_dump_data.o 
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	# $(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

runner.o: runner.c common.h pcsx2_dump_vars.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c $< -o $@

pcsx2_dump_data.o: pcsx2_dump_data.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c pcsx2_dump_data.c -o $@

pcsx2_dump_data.c pcsx2_dump_vars.c: preprocess_dump pcsx2_dump.gs
	./preprocess_dump pcsx2_dump.gs pcsx2_dump_data.c pcsx2_dump_vars.c

scratch.elf: scratch.c
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)

clean:
	rm -f runner.elf runner.o pcsx2_dump_data.o preprocess_dump

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
