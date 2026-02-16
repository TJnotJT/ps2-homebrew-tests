# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

include ../Makefile.destdir

EE_LIBS = -lkernel -ldraw -lgraph -lmath3d -ldma -lpatches

all: \
tex-rounding-3-tw9-tbw8-off0-frac0-for.elf \
tex-rounding-3-tw8-tbw8-off0-frac0-for.elf \
tex-rounding-3-tw8-tbw4-off0-frac0-for.elf \
tex-rounding-3-tw9-tbw8-off0-frac1-for.elf \
tex-rounding-3-tw9-tbw8-off0-frac0-rev.elf

tex-rounding-3-tw9-tbw8-off0-frac0-for.o: tex-rounding-3.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DTW=9 -DTBW=8 -DOFFSET_U=0 -DOFFSET_FRAC=0 -DREV=0 -c $< -o $@

tex-rounding-3-tw9-tbw8-off0-frac0-rev.o: tex-rounding-3.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DTW=9 -DTBW=8 -DOFFSET_U=0 -DOFFSET_FRAC=0 -DREV=1 -c $< -o $@

tex-rounding-3-tw9-tbw8-off0-frac1-for.o: tex-rounding-3.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DTW=9 -DTBW=8 -DOFFSET_U=0 -DOFFSET_FRAC=1 -DREV=0 -c $< -o $@

tex-rounding-3-tw9-tbw8-off0-frac1-for.o: tex-rounding-3.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DTW=9 -DTBW=8 -DOFFSET_U=0 -DOFFSET_FRAC=1 -DREV=0 -c $< -o $@

tex-rounding-3-tw8-tbw8-off0-frac0-for.o: tex-rounding-3.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DTW=8 -DTBW=8 -DOFFSET_U=0 -DOFFSET_FRAC=0 -DREV=0 -c $< -o $@

tex-rounding-3-tw8-tbw4-off0-frac0-for.o: tex-rounding-3.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -DTW=8 -DTBW=4 -DOFFSET_U=0 -DOFFSET_FRAC=0 -DREV=0 -c $< -o $@

%.elf: %.o ../lib-bmp/lib-bmp.a ../lib-usb/lib-usb.a ../lib-read-fb/lib-read-fb.a
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

clean:
	rm -f tex-rounding-3.elf tex-rounding-3.o

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal