# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

include ../Makefile.destdir

all: lib-common.a

lib-common.a: common.o
	$(DIR_GUARD)
	$(EE_AR) rcs $@ $^

common.o: common.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c $< -o $@

clean:
	rm -f lib-common.a

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
