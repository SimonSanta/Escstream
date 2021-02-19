#
# File: BareMetal_GCC.make
#
# Copyright (c) 2017-2018, Code-Time Technologies Inc.
# All rights reserved.
#

# NOTE:
# This Makefile includes "Common_GCC.make" which contains the common part of all makefiles
# It is done to simplify the work needed when something is changed for a target or for a demo #
#
# The symbol _STANDALONE_ is defined to build bare-metal apps although uAbassi is used and _UABASSI_
# is also defined.  _STANDALONE_ is only used in the drivers to select the RTOS/SAL include file and
# because it is the first conditional in a sequence of preprocessor ifs, it take precedence
# This configures the drivers to use the SAL RTO Sspoofing code instead fi uAbassi
# Refer to BareMetalApps.c for a more detailled dsescription
# ----------------------------------------------------------------------------------------------------

MAKE_NAME := MyApp_MTL1_GCC.make            # For full re-build if I'm modified

FATFS        := ../../mAbassi/FatFS-0.13a

# The path order of in VPATH is important
VPATH   := ../src                            # This is due to multiple file with same names
VPATH   += :../hwlib
VPATH   += :../painter
VPATH   += :../painter/fonts
VPATH   += :../painter/graphic_lib
VPATH   += :../painter/terasic_lib
VPATH   += :../../mAbassi/Abassi
VPATH   += :../../mAbassi/Platform/src
VPATH   += :../../mAbassi/Drivers/src
VPATH   += :../../mAbassi/Share/src
VPATH   += :$(FATFS)/src
VPATH   += :$(FATFS)/src/option

C_SRC   :=                                    # C sources with/without thumb (see THUMB above)
C_SRC   += Main_mAbassi.c
C_SRC   += MyApp_MTL.c

C_SRC   += TIMERinit.c
C_SRC   += alt_gpio.c
C_SRC   += arm_acp.c
C_SRC   += arm_pl330.c
C_SRC   += dw_i2c.c
C_SRC   += dw_uart.c
C_SRC   += MediaIF.c
C_SRC   += SysCall_CY5.c
C_SRC   += dw_sdmmc.c

C_SRC   += ff.c
C_SRC   += ffunicode.c
C_SRC   += Abassi_FatFS.c
C_SRC   += Media_FatFS.c
C_SRC   += SysCall_FatFS.c

C_SRC   += tahomabold_20.c
C_SRC   += tahomabold_32.c
C_SRC   += geometry.c
C_SRC   += gesture.c
C_SRC   += gui_vpg.c
C_SRC   += gui.c
C_SRC   += vip_fr.c
C_SRC   += simple_graphics.c
C_SRC   += simple_text.c
C_SRC   += debug.c
C_SRC   += multi_touch.c
C_SRC   += queue.c

C_SRC   += alt_generalpurpose_io.c
C_SRC   += alt_globaltmr.c
C_SRC   += alt_clock_manager.c
# Assembly files
S_SRC   :=
# Object files
O_SRC   :=

AFLAGS   :=
CFLAGS   :=
LFLAGS   :=
LIBS     :=

# ----------------------------------------------------------------------------------------------------

C_INC   :=                                     # All "C" include files for dependencies
C_INC   += ../../mAbassi/Abassi/mAbassi.h
C_INC   += ../../mAbassi/Abassi/SysCall.h
C_INC   += ../../mAbassi/Platform/inc/Platform.h
C_INC   += ../../mAbassi/Platform/inc/HWinfo.h
C_INC   += ../../mAbassi/Platform/inc/AbassiLib.h
C_INC   += ../../mAbassi/Drivers/inc/alt_gpio.h
C_INC   += ../../mAbassi/Drivers/inc/arm_acp.h
C_INC   += ../../mAbassi/Drivers/inc/arm_pl330.h
C_INC   += ../../mAbassi/Drivers/inc/dw_i2c.h
C_INC   += ../../mAbassi/Drivers/inc/dw_uart.h
C_INC   += ../../mAbassi/Drivers/inc/MediaIF.h

C_INC   += $(FATFS)/inc/diskio.h
C_INC   += $(FATFS)/inc/ff.h
C_INC   += $(FATFS)/inc/integer.h
C_INC   += ../../mAbassi/Share/inc/ffconf.h

C_INC   += ../../mAbassi/Drivers/inc/dw_sdmmc.h

C_INC   += ../inc/MyApp_MTL.h
C_INC   += ../inc/hps_0.h

C_INC   += ../painter/geometry.h
C_INC   += ../painter/gesture.h
C_INC   += ../painter/gui.h
C_INC   += ../painter/vip_fr.h
C_INC   += ../painter/fonts/fonts.h
C_INC   += ../painter/graphic_lib/simple_graphics.h
C_INC   += ../painter/graphic_lib/simple_text.h
C_INC   += ../painter/terasic_lib/debug.h
C_INC   += ../painter/terasic_lib/multi_touch.h
C_INC   += ../painter/terasic_lib/queue.h
C_INC   += ../painter/terasic_lib/terasic_includes.h


# Compiler command line options. The -I order is important
CFLAGS  += -g -O3 -Wall
CFLAGS  += -I ../inc
CFLAGS  += -I ../painter
CFLAGS  += -I ../painter/fonts
CFLAGS  += -I ../painter/graphic_lib
CFLAGS  += -I ../painter/terasic_lib

CFLAGS  += -I../../mAbassi/Abassi
CFLAGS  += -I../../mAbassi/Platform/inc
CFLAGS  += -I../../mAbassi/Drivers/inc
CFLAGS  += -I../../mAbassi/Share/inc

CFLAGS  += -I$(FATFS)/inc
CFLAGS  += -I../../Drivers/inc

CFLAGS  += -I ${SOCEDS_DEST_ROOT}/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av
CFLAGS  += -I ${SOCEDS_DEST_ROOT}/ip/altera/hps/altera_hps/hwlib/include

CFLAGS  += -DOS_START_STACK=8192
CFLAGS  += -DOS_SYS_CALL=1
CFLAGS  += -DSYS_CALL_DEV_I2C=1
CFLAGS  += -DSYS_CALL_DEV_TTY=1
CFLAGS  += -DSYS_CALL_TTY_EOF=4
CFLAGS  += -DI2C_DEBUG=0
CFLAGS  += -DI2C_OPERATION=0x00707
CFLAGS  += -DI2C_USE_MUTEX=1
CFLAGS  += -DMEDIA_DEBUG=1
CFLAGS  += -DSDMMC_BUFFER_TYPE=SDMMC_BUFFER_CACHED
CFLAGS  += -DSDMMC_NUM_DMA_DESC=64
CFLAGS  += -DSDMMC_USE_MUTEX=1
CFLAGS  += -DUART_FULL_PROTECT=1
CFLAGS  += -DMEDIA_MDRV_SIZE=-1
CFLAGS  += -D_VOLUMES=2
CFLAGS  += -DSYS_CALL_N_DRV=2
CFLAGS  += -DMEDIA_AUTO_SELECT=0
CFLAGS  += -DMEDIA_SDMMC0_IDX=0
CFLAGS  += -DMEDIA_SDMMC0_DEV=SDMMC_DEV
CFLAGS  += -DMEDIA_MDRV_IDX=1
CFLAGS  += -DDEMO_USE_SDMMC=1

CFLAGS  += -D soc_cv_av
CFLAGS  += -D MYAPP_MTL

# Assembler command line options
AFLAGS  += -g

NMB_DRV := 2

# ----------------------------------------------------------------------------------------------------

include ../../mAbassi/Common_GCC.make

# EOF


