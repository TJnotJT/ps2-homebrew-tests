# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

include ../Makefile.destdir

EE_LIBS = -lkernel -ldraw -lgraph -lmath3d -ldma -lpatches

all: triangle-test-6-h.elf triangle-test-6-aa-h.elf

triangle-test-6-h.o: triangle-test-6.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=0 -DREFLECT=0 -DVERTICAL=0 -c $< -o $@

triangle-test-6-reflect-h.o: triangle-test-6.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=0 -DREFLECT=1 -DVERTICAL=0 -c $< -o $@

triangle-test-6-v.o: triangle-test-6.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=0 -DREFLECT=0 -DVERTICAL=1 -c $< -o $@

triangle-test-6-reflect-v.o: triangle-test-6.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=0 -DREFLECT=1 -DVERTICAL=1 -c $< -o $@

triangle-test-6-aa-h.o: triangle-test-6.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=1 -DREFLECT=0 -DVERTICAL=0 -c $< -o $@

triangle-test-6-aa-v.o: triangle-test-6.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=1 -DREFLECT=0 -DVERTICAL=1 -c $< -o $@

triangle-test-6-aa-reflect-h.o: triangle-test-6.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=1 -DREFLECT=1 -DVERTICAL=0 -c $< -o $@

triangle-test-6-aa-reflect-v.o: triangle-test-6.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DUSE_AA=1 -DREFLECT=1 -DVERTICAL=1 -c $< -o $@

triangle-test-6-h.elf: triangle-test-6-h.o ../lib-bmp/lib-bmp.a ../lib-read-fb/lib-read-fb.a ../lib-usb/lib-usb.a ../lib-common/lib-common.a
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

triangle-test-6-reflect-h.elf: triangle-test-6-reflect-h.o ../lib-bmp/lib-bmp.a ../lib-read-fb/lib-read-fb.a ../lib-usb/lib-usb.a ../lib-common/lib-common.a
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

triangle-test-6-v.elf: triangle-test-6-v.o ../lib-bmp/lib-bmp.a ../lib-read-fb/lib-read-fb.a ../lib-usb/lib-usb.a ../lib-common/lib-common.a
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

triangle-test-6-reflect-v.elf: triangle-test-6-reflect-v.o ../lib-bmp/lib-bmp.a ../lib-read-fb/lib-read-fb.a ../lib-usb/lib-usb.a ../lib-common/lib-common.a
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif


triangle-test-6-aa-h.elf: triangle-test-6-aa-h.o ../lib-bmp/lib-bmp.a ../lib-read-fb/lib-read-fb.a ../lib-usb/lib-usb.a ../lib-common/lib-common.a
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

triangle-test-6-aa-reflect-h.elf: triangle-test-6-aa-reflect-h.o ../lib-bmp/lib-bmp.a ../lib-read-fb/lib-read-fb.a ../lib-usb/lib-usb.a ../lib-common/lib-common.a
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

triangle-test-6-aa-v.elf: triangle-test-6-aa-v.o ../lib-bmp/lib-bmp.a ../lib-read-fb/lib-read-fb.a ../lib-usb/lib-usb.a ../lib-common/lib-common.a
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

triangle-test-6-aa-reflect-v.elf: triangle-test-6-aa-reflect-v.o ../lib-bmp/lib-bmp.a ../lib-read-fb/lib-read-fb.a ../lib-usb/lib-usb.a ../lib-common/lib-common.a
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
