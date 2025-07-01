# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

include ../Makefile.destdir

EE_LIBS = -lkernel -ldraw -lpatches -lgraph -lmath3d -ldma

all: z-prec.elf

irx/usbd_irx.c: irx/usbd.irx
	bin2c $< irx/usbd_irx.c usbd_irx

irx/usbmass_bd_irx.c: irx/usbmass_bd.irx
	bin2c $< irx/usbmass_bd_irx.c usbmass_bd_irx

irx/bdm_irx.c: irx/bdm.irx
	bin2c $< irx/bdm_irx.c bdm_irx

irx/bdmfs_fatfs_irx.c: irx/bdmfs_fatfs.irx
	bin2c $< irx/bdmfs_fatfs_irx.c bdmfs_fatfs_irx

irx/usbd_irx.o: irx/usbd_irx.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c $< -o $@

irx/usbmass_bd_irx.o: irx/usbmass_bd_irx.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c $< -o $@

irx/bdm_irx.o: irx/bdm_irx.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c $< -o $@

irx/bdmfs_fatfs_irx.o: irx/bdmfs_fatfs_irx.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c $< -o $@

z-prec.elf: z-prec.o usb.o bmp.o irx/usbd_irx.o irx/bdm_irx.o irx/bdmfs_fatfs_irx.o irx/usbmass_bd_irx.o
	$(DIR_GUARD)
	$(EE_CC) -T$(EE_LINKFILE) $(EE_OPTFLAGS) -o $@ $^ $(EE_LDFLAGS) $(EXTRA_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@
ifdef DEST_DIR
	cp $@ $(DEST_DIR)
endif

usb.o: usb.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c $< -o $@

bmp.o: bmp.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c $< -o $@

z-prec.o: z-prec.c
	$(EE_C_COMPILE) $(EE_CFLAGS) -c $< -o $@

clean:
	rm -f z-prec.elf z-prec.o

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
