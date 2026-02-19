# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

include ../Makefile.destdir

EE_LIBS = -lkernel -ldraw -lgraph -lmath3d -ldma -lpatches

all: rov-test-2-light-blend-zmask.elf \
rov-test-2-heavy-blend-zmask.elf \
rov-test-2-heavy-noblend-zmask.elf \
rov-test-2-heavy-noblend-nozmask.elf

rov-test-2-light-blend-zmask.o: rov-test-2.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DMODE=0 -DBLEND=1 -DZMASK=1 -c $< -o $@

rov-test-2-heavy-blend-zmask.o: rov-test-2.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DMODE=1 -DBLEND=1 -DZMASK=1 -c $< -o $@

rov-test-2-heavy-noblend-zmask.o: rov-test-2.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DMODE=1 -DBLEND=0 -DZMASK=1 -c $< -o $@

rov-test-2-heavy-noblend-nozmask.o: rov-test-2.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DMODE=1 -DBLEND=0 -DZMASK=0 -c $< -o $@

%.elf: %.o
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

clean:
	rm -f *.elf *.o

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
