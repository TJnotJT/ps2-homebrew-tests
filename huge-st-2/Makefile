# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

include ../Makefile.destdir

EE_LIBS = -ldraw -lgraph -lmath3d -lpacket -ldma

all: huge-st-2-huge.elf huge-st-2-normal.elf

%.elf: %.o
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $< $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

huge-st-2-normal.o: huge-st-2-template.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DHUGE_ST=0 -c $< -o $@

huge-st-2-huge.o: huge-st-2-template.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DHUGE_ST=1 -c $< -o $@

clean:
	rm -f huge-st-2-huge.elf huge-st-2-normal.elf huge-st-2-huge.o huge-st-2-normal.o

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
