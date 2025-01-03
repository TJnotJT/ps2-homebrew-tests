# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_LIBS = -ldraw -lgraph -lmath3d -lpacket -ldma
BUILD_DIR = build
C_FILE = $(BUILD_DIR)/$(FILE).c
O_FILE = $(BUILD_DIR)/$(FILE).o
ELF_FILE = $(BUILD_DIR)/$(FILE).elf

include ../Makefile.destdir

$(C_FILE): uv-bounds.c
	$(EE_C_COMPILE) $(DEFINES) -E -x c uv-bounds.c > $@

$(O_FILE): $(C_FILE)
	$(EE_CC) $(EE_CFLAGS) -c $< -o $@

$(ELF_FILE): $(O_FILE)
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $< $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

all: $(ELF_FILE)

clean:
	rm -f $(ELF_FILE) $(O_FILE) $(C_FILE)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
