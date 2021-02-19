#
# File: Common_GCC.make
#
# Copyright (c) 2017-2018, Code-Time Technologies Inc.
# All rights reserved.		
#

# NOTE:
# This Makefile is included by all GCC projects
# It is done to simplify the work needed when something is changed for a target or for a demo #

# ----------------------------------------------------------------------------------------------------
# Common "C" and "ASM files

ifeq ($(BUILD_TYPE), TYPE_SRC)
  C_SRC   += mAbassi.c
  S_SRC   += ARMv7_SMP_L1_L2_GCC.s
  S_SRC   += mAbassi_SMP_CORTEXA9_GCC.s
endif
ifneq ($(IS_SANITY),YES)
  C_SRC   += mAbassiCfgA9.c
endif
ifeq ($(DBG_SHELL), YES)
  C_SRC   += Shell.c
  C_SRC   += SubShell.c
endif
ifeq  ($(PERF_MON), YES)
  C_SRC   += PerfMon.c
endif

CFLAGS += -DOS_DEMO=-$(DEMO_NMB)

ifeq ($(UART_TYPE),POLL)
  CFLAGS  += -DUART_Q_SIZE_RX=0
  CFLAGS  += -DUART_Q_SIZE_TX=0
else ifeq ($(UART_TYPE),MBX)
  CFLAGS  += -DDUART_Q_SIZE_RX=((OX_MAX_PEND_RQST)/4)
  CFLAGS  += -DDUART_Q_SIZE_TX=((OX_MAX_PEND_RQST)/4)
else
  CFLAGS  += -DUART_Q_SIZE_RX=1
  CFLAGS  += -DUART_Q_SIZE_TX=1
  CFLAGS  += -DUART_CIRC_BUF_RX=$(UART_TYPE)
  CFLAGS  += -DUART_CIRC_BUF_TX=$(UART_TYPE)
endif

ifeq ($(BUILD_TYPE),TYPE_SRC)
 ifeq ($(IS_SANITY), YES)
  CFLAGS  += -DOS_MP_TYPE=5U
 else
  CFLAGS  += -DOS_MP_TYPE=3U
 endif
  CFLAGS  += -DOS_MBXPUT_ISR=1
endif

# ----------------------------------------------------------------------------------------------------
# Common "C" and "ASM flags (target processor options)

CFLAGS  += -std=gnu99
CFLAGS  += -mfpu=neon
CFLAGS  += -march=armv7-a
CFLAGS  += -mtune=cortex-a9
CFLAGS  += -mcpu=cortex-a9
CFLAGS  += -mfloat-abi=softfp
CFLAGS  += -mno-unaligned-access
CFLAGS  += -ffunction-sections
CFLAGS  += -fdata-sections
CFLAGS  += -Wstrict-prototypes

CFLAGS  += -DOS_PLATFORM=$(PLATFORM)
CFLAGS  += -DOS_N_CORE=$(N_CORE)
CFLAGS  += -DOS_NEWLIB_REENT=$(LIB_REENT)

ifeq ($(DBG_SHELL), YES)
  C_INC   += ../../mAbassi/Abassi/Shell.h
  CFLAGS  += -DUSE_SHELL=1
  CFLAGS  += -DSHELL_LOGIN=0
  CFLAGS  += -DSHELL_HISTORY=64
endif

AFLAGS  += -mcpu=cortex-a9
AFLAGS  += -mfpu=neon
AFLAGS  += --defsym OS_PLATFORM=$(PLATFORM)
ifeq ($(LINARO),YES)
  AFLAGS += --defsym OS_CODE_SOURCERY=0
endif

ifeq ($(BUILD_TYPE),TYPE_SRC)
  AFLAGS  += --defsym OS_N_CORE=$(N_CORE)
  AFLAGS  += --defsym OS_NEWLIB_REENT=$(LIB_REENT)
  AFLAGS  += --defsym OS_RUN_PRIVILEGE=1
  AFLAGS  += --defsym OS_HANDLE_PSR_Q=0
  AFLAGS  += --defsym OS_VFP_TYPE=32
  AFLAGS  += --defsym OS_SPINLOCK=1
  AFLAGS  += --defsym OS_USE_CACHE=1
  AFLAGS  += --defsym OS_L2_BASE_ADDR=-1
  AFLAGS  += --defsym OS_SAME_L1_PAGE_TBL=0
  AFLAGS  += --defsym OS_ARM_ERRATA_ALL=1
  AFLAGS  += --defsym OS_MMU_ALL_INVALID=0
  AFLAGS  += --defsym OS_L1_CACHE_BP=1
  AFLAGS  += --defsym OS_L1_CACHE_PF=1
  AFLAGS  += --defsym OS_L2_CACHE_PF=1
  AFLAGS  += --defsym OS_CACHE_WRITE_ZERO=1
  AFLAGS  += --defsym OS_USE_NON_SHARED=1
  AFLAGS  += --defsym OS_ABORT_STACK_SIZE=1024
  AFLAGS  += --defsym OS_FIQ_STACK_SIZE=1024
  AFLAGS  += --defsym OS_IRQ_STACK_SIZE=8192
  AFLAGS  += --defsym OS_SUPER_STACK_SIZE=1024
  AFLAGS  += --defsym OS_UNDEF_STACK_SIZE=1024
  AFLAGS  += --defsym OS_MMU_EXTERN_DEF=0
endif

ifeq ($(PERF_MON), YES)
 ifeq ($(PLATFORM), 0x0000AAC5)				# Cyclone V SocFGPA @ 925 MHZ & /4
  CFLAGS  += -DOS_PERF_MON=-1
  AFLAGS  += --defsym OS_PERF_TIMER_BASE=1
  AFLAGS  += --defsym OS_PERF_TIMER_DIV=231
 else ifeq ($(PLATFORM), 0x1000AAC5)		# DE0 Nano/ DE10 nano  @ 925 MHz & /4
  CFLAGS  += -DOS_PERF_MON=-1
  AFLAGS  += --defsym OS_PERF_TIMER_BASE=1
  AFLAGS  += --defsym OS_PERF_TIMER_DIV=231
 else ifeq ($(PLATFORM), 0x2000AAC5)		# EBV Socrates @ 800 MHz & /4
  CFLAGS  += -DOS_PERF_MON=-1
  AFLAGS  += --defsym OS_PERF_TIMER_BASE=1
  AFLAGS  += --defsym OS_PERF_TIMER_DIV=200
 else ifeq ($(PLATFORM), 0x3000AAC5)		# SocKIT @ 800 MHz & /4
  CFLAGS  += -DOS_PERF_MON=-1
  AFLAGS  += --defsym OS_PERF_TIMER_BASE=1
  AFLAGS  += --defsym OS_PERF_TIMER_DIV=200
 else ifeq ($(PLATFORM), 0x4000AAC5)		# Arria V socFPGA @ 925 MHZ & /4
  CFLAGS  += -DOS_PERF_MON=-1
  AFLAGS  += --defsym OS_PERF_TIMER_BASE=1
  AFLAGS  += --defsym OS_PERF_TIMER_DIV=231
 else ifeq ($(PLATFORM), 0x0000AA10)		# Arria 10 socFPGA @ 1200 MHZ & /4 (OS_PERF_MON == -2)
  CFLAGS  += -DOS_PERF_MON=-2
  AFLAGS  += --defsym OS_PERF_TIMER_BASE=1
  AFLAGS  += --defsym OS_PERF_TIMER_DIV=150
 else										# Unknown, assume around 1000 MHz
  CFLAGS  += -DOS_PERF_MON=-1
  AFLAGS  += --defsym OS_PERF_TIMER_BASE=1
  AFLAGS  += --defsym OS_PERF_TIMER_DIV=250
 endif
endif

# ----------------------------------------------------------------------------------------------------
# Platform specific files & build options
# ----------------------------------------------------------------------------------------------------

ifeq ($(PLATFORM), 0x0100AAC5)
  BOARD_ID   := DE0NANOSOC
  LIB_ID     := AR5_CY5
  LNK_SCRIPT := ../mAbassi/AR5_CY5.ld

else
  $(error Error : Unsupported value for the defintion of PLATFORM)
endif

# ----------------------------------------------------------------------------------------------------
# Package source selection
# ----------------------------------------------------------------------------------------------------

ifneq ($(BUILD_TYPE),TYPE_SRC)
 ifeq ($(LIB_REENT), 0)
  LIBRR := 
 else
  LIBRR := R_
 endif
 ifeq ($(PERF_MON), YES)
  ifeq ($(BUILD_TYPE),TYPE_EVAL)
   $(error Error : performance monitoring is not available in the eval package)
  else
   LIBREENT := $(LIBRR)PM_
  endif
 else
  LIBREENT := $(LIBRR)
 endif
 ifeq ($(LINARO),YES)
  CC_TYPE := li
 else
  CC_TYPE := 
 endif
 ifeq ($(BUILD_TYPE), TYPE_EVAL)
  LIB_TYPE := EVAL_
 else ifeq ($(BUILD_TYPE), TYPE_FREE)
  LIB_TYPE := FREE_
 else
  LIB_TYPE := TRIAL_
 endif
  LIBS   += -Wl,-l_mAbassi_$(LIB_TYPE)$(LIBREENT)GCC$(CC_TYPE)_$(LIB_ID)
  LDEP   += ../../mAbassi/Platform/lib/lib_mAbassi_$(LIB_TYPE)$(LIBREENT)GCC$(CC_TYPE)_$(LIB_ID).a
  LFLAGS += -Wl,-L../../mAbassi/Platform/lib
endif

# ----------------------------------------------------------------------------------------------------
# Compilation, assembly, linking
# ----------------------------------------------------------------------------------------------------

ifeq ($(LINARO), YES)
  CROSS_COMPILE := /Program Files (x86)/GCC ARM embedded/4.9/bin/arm-none-eabi-
else
  CROSS_COMPILE := arm-altera-eabi-
  LIBS          += -Wl,-lcs3 -Wl,-lcs3unhosted -Wl,-lcs3arm
endif

AS := "$(CROSS_COMPILE)as"
CC := "$(CROSS_COMPILE)gcc"
LD := "$(CROSS_COMPILE)g++"
NM := "$(CROSS_COMPILE)nm"
OC := "$(CROSS_COMPILE)objcopy"

RM := rm -f
CP := cp -f

ifeq ($(DEMO_NMB), 10)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), -10)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), 11)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), -11)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), 12)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), -12)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), 13)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), -13)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), 14)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), -14)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), 15)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), -15)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), 16)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), -16)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), 17)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), -17)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), 18)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), -18)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), 19)
  ELF_NAME := 10-19
else ifeq ($(DEMO_NMB), -19)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), 20)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), -20)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), 21)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), -21)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), 22)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), -22)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), 23)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), -23)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), 24)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), -24)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), 25)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), -25)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), 26)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), -26)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), 27)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), -27)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), 28)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), -28)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), 29)
  ELF_NAME := 20-29
else ifeq ($(DEMO_NMB), -29)
  ELF_NAME := 20-29
else
  ELF_NAME := $(DEMO_NMB)
endif

ifeq ($(IS_SANITY),YES)
  BIN  := Sanity_SMP_$(BOARD_ID)_A9_GCC.bin
  ELF  := Sanity_SMP_$(BOARD_ID)_A9_GCC.axf
else
  BIN  := Demo_$(ELF_NAME)_SMP_$(BOARD_ID)_A9_GCC.bin
  ELF  := Demo_$(ELF_NAME)_SMP_$(BOARD_ID)_A9_GCC.axf
endif

OBJ    := $(patsubst %.c,%.o,$(C_SRC))
SOBJ   := $(patsubst %.s,%.o,$(S_SRC))

.PHONY: all
all: $(ELF) $(BIN)
	@echo "Done"

.PHONY: clean
clean:
	-$(RM) *.d *.o *.axf *.map *.bin *.lst

$(OBJ): %.o: %.c Makefile ../Common_GCC.make ../$(MAKE_NAME) $(C_INC)
	$(CC) $(CFLAGS) -c $< -o $(notdir $(basename $<)).o

$(SOBJ): %.o: %.s Makefile ../Common_GCC.make ../$(MAKE_NAME)
	$(AS) $(AFLAGS) $< -o $(notdir $(basename $<)).o

$(ELF): $(SOBJ) $(OBJ) $(O_SRC) Makefile ../Common_GCC.make ../$(MAKE_NAME) $(LDEP) ../$(LNK_SCRIPT)
	$(LD) -Wl,--gc-sections $(LFLAGS) -Xlinker -Map -Xlinker $(notdir $(basename $@)).map 			\
	      -T ../$(LNK_SCRIPT) $(notdir $(SOBJ)) $(notdir $(OBJ)) $(O_SRC) $(LIBS)					\
          -Wl,-lgcc -Wl,-lc -o $(notdir $@)
	$(NM) $(notdir $@) > $(notdir $@).map

$(BIN): $(ELF)
	$(OC) $(ELF) -j vectors -j .rodata -j .text -j .data -j .bss --output-target=binary $(BIN)

#EOF
