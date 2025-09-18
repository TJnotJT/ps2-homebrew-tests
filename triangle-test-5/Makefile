# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

include ../Makefile.destdir

EE_LIBS = -lkernel -ldraw -lgraph -lmath3d -ldma -lpatches

all: triangle-test-5.elf triangle-test-5-aa.elf

triangle-test-5.o: triangle-test-5.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=0 -c $< -o $@

triangle-test-5-aa.o: triangle-test-5.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=1 -c $< -o $@

triangle-test-5.elf: triangle-test-5.o ../lib-bmp/lib-bmp.a ../lib-read-fb/lib-read-fb.a ../lib-usb/lib-usb.a ../lib-common/lib-common.a
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

triangle-test-5-aa.elf: triangle-test-5-aa.o ../lib-bmp/lib-bmp.a ../lib-read-fb/lib-read-fb.a ../lib-usb/lib-usb.a ../lib-common/lib-common.a
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

clean:
	rm -f triangle-test-5.elf triangle-test-5.o triangle-test-5-aa.o

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
