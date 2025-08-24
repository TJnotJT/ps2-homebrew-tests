# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

include ../Makefile.destdir

EE_LIBS = -lkernel -ldraw -lgraph -lmath3d -ldma -lpatches

all: fog-test-auto.elf

fog-test-auto.elf: fog-test-auto.o ../lib-read-fb/lib-read-fb.a ../lib-common/lib-common.a
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

fog-test-auto.o: fog-test-auto.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c $< -o $@

clean:
	rm -f fog-test-auto.elf fog-test-auto.o

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal