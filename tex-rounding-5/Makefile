# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

TW ?= 9
TBW ?= 8
OFF ?= 0
FRAC ?= 0
REV ?= 0

OUT := tex-rounding-5-tw$(TW)-tbw$(TBW)-off$(OFF)-frac$(FRAC)-rev$(REV)

include ../Makefile.destdir

EE_LIBS = -lkernel -ldraw -lgraph -lmath3d -ldma -lpatches

all: $(OUT).elf

$(OUT).o: tex-rounding-5.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DTW=$(TW) -DTBW=$(TBW) -DOFFSET_U=$(OFF) -DOFFSET_FRAC=$(FRAC) -DREV=$(REV) -c $< -o $@

%.elf: %.o ../lib-bmp/lib-bmp.a ../lib-usb/lib-usb.a ../lib-read-fb/lib-read-fb.a
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