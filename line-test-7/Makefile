# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

include ../Makefile.destdir

EE_LIBS = -lkernel -ldraw -lgraph -lmath3d -ldma -lpatches

all: line-test-7-h.elf line-test-7-h-aa.elf line-test-7-v.elf line-test-7-v-aa.elf \
		 line-test-7-h-tex.elf line-test-7-h-aa-tex.elf line-test-7-v-tex.elf line-test-7-v-aa-tex.elf

line-test-7-h.o: line-test-7.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=0 -DVERTICAL=0 -DUSE_TEX=0 -c $< -o $@

line-test-7-h-aa.o: line-test-7.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=1 -DVERTICAL=0 -DUSE_TEX=0 -c $< -o $@

line-test-7-v.o: line-test-7.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=0 -DVERTICAL=1 -DUSE_TEX=0 -c $< -o $@

line-test-7-v-aa.o: line-test-7.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=1 -DVERTICAL=1 -DUSE_TEX=0 -c $< -o $@

line-test-7-h-tex.o: line-test-7.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=0 -DVERTICAL=0 -DUSE_TEX=1 -c $< -o $@

line-test-7-h-aa-tex.o: line-test-7.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=1 -DVERTICAL=0 -DUSE_TEX=1 -c $< -o $@

line-test-7-v-tex.o: line-test-7.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=0 -DVERTICAL=1 -DUSE_TEX=1 -c $< -o $@

line-test-7-v-aa-tex.o: line-test-7.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=1 -DVERTICAL=1 -DUSE_TEX=1 -c $< -o $@


%.elf: %.o ../lib-bmp/lib-bmp.a ../lib-read-fb/lib-read-fb.a ../lib-usb/lib-usb.a ../lib-common/lib-common.a
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
