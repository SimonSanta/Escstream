#
# Copyright (c) 2014-2018, Code-Time Technologies Inc.
# All rights reserved.		
#

# NOTE:
# This Makefile includes another one: ../BareMetal_GCC.make
# It is done to simplify the work needed when something is changed for a target or for a demo #
# 
# This makefile must be tailored to the uAbassi package used
# Depending on the availability of the drivers in source form or library
# You need to select the mAbassi type of code
#
# TYPE_SRC  : Source code
# TYPE_EVAL : Evaluation library (with time expiry)
# TYPE_LIB  : Release library (no time expiry)
# TYPE_FREE : Freeware library (no time expiry - resource limited)
#
BUILD_TYPE   := TYPE_EVAL

# If using GCC ARM (Linaro), set LINARO to YES, otherwise set to NO
LINARO       := NO

# How to set-up the UART:
#      UART_TYPE := POLL	- Polling
#      UART_TYPE := MBX		- RTOS mailboxes with ISR
#      UART_TYPE := # 		- Circular buffer with ISR (# is size of the RX & TX circular buffers)
UART_TYPE     := 2048

# ----------------------------------------------------------------------------------------------------
#
# Defines to keep "C" / "ASM" / linker in sync
# For Booleans, the value must be either YES or NO
# LIB_REENT is either 0, +ve or -ve

DEMO_NMB     := 100
PLATFORM     := 0x0100AAC5
N_CORE       := 2
LIB_REENT    := 0
PERF_MON     := NO
DBG_SHELL    := YES

# ----------------------------------------------------------------------------------------------------

include ../MyApp_MTL1_GCC.make

#EOF
