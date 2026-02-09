# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

include ../Makefile.destdir

EE_LIBS = -lkernel -ldraw -lgraph -lmath3d -ldma -lpatches

all: rov-test.elf rov-test-tex.elf rov-test-z.elf rov-test-tex-z.elf \
rov-test-date.elf rov-test-tex-date.elf rov-test-z-date.elf rov-test-tex-z-date.elf

rov-test.o: rov-test.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_TEX=0 -DUSE_Z=0 -DUSE_DATE=0 -c $< -o $@

rov-test-tex.o: rov-test.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_TEX=1 -DUSE_Z=0 -DUSE_DATE=0 -c $< -o $@

rov-test-z.o: rov-test.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_TEX=0 -DUSE_Z=1 -DUSE_DATE=0 -c $< -o $@

rov-test-tex-z.o: rov-test.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_TEX=1 -DUSE_Z=1 -DUSE_DATE=0 -c $< -o $@

rov-test-date.o: rov-test.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_TEX=0 -DUSE_Z=0 -DUSE_DATE=1 -c $< -o $@

rov-test-tex-date.o: rov-test.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_TEX=1 -DUSE_Z=0 -DUSE_DATE=1 -c $< -o $@

rov-test-z-date.o: rov-test.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_TEX=0 -DUSE_Z=1 -DUSE_DATE=1 -c $< -o $@

rov-test-tex-z-date.o: rov-test.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_TEX=1 -DUSE_Z=1 -DUSE_DATE=1 -c $< -o $@

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
