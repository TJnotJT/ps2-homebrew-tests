# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_LIBS = -ldraw -lgraph -lmath3d -lpacket -ldma
PRIM_TRI = 0
PRIM_SPRITE = 1
ST_MODE = 0
UV_MODE = 1
DEFINES = -DST_MODE=$(ST_MODE) -DUV_MODE=$(UV_MODE) -DTRI=$(PRIM_TRI) -DSPRITE=$(PRIM_SPRITE)

include ../Makefile.destdir

all: tri_st.elf tri_uv.elf sprite_st.elf sprite_uv.elf
	$(EE_STRIP) --strip-all $^
ifdef DEST_DIR
	cp $^ $(DEST_DIR)
endif

tri_st.c: main.c
	$(EE_C_COMPILE) $(DEFINES) -DPRIM=$(PRIM_TRI) -DTEX_MODE=$(ST_MODE) -E main.c > $@

tri_uv.c: main.c
	$(EE_C_COMPILE) $(DEFINES) -DPRIM=$(PRIM_TRI) -DTEX_MODE=$(UV_MODE) -E main.c > $@

sprite_st.c: main.c
	$(EE_C_COMPILE) $(DEFINES) -DPRIM=$(PRIM_SPRITE) -DTEX_MODE=$(ST_MODE) -E main.c > $@

sprite_uv.c: main.c
	$(EE_C_COMPILE) $(DEFINES) -DPRIM=$(PRIM_SPRITE) -DTEX_MODE=$(UV_MODE) -E main.c > $@

%.elf: %.o
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $< $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)

clean:
	rm -f *.elf *.o

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
