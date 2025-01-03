# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_LIBS = -ldraw -lgraph -lmath3d -lpacket -ldma
BUILD_DIR = build

include ../Makefile.destdir

all: huge-st-normal.elf huge-st-huge.elf

clean:
	rm -f huge-st-normal.elf huge-st-huge.elf huge-st-normal.o huge-st-huge.o

huge-st-normal.o: huge-st.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DHUGE_ST=0 -c $< -o $@

huge-st-huge.o: huge-st.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DHUGE_ST=1 -c $< -o $@

%.elf: %.o
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $< $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
