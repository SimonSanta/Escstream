/* ------------------------------------------------------------------------------------------------ */
/* FILE :		dw_uart.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the Designware implementation of the "standard" NS 16550 UART.			*/
/*				IMPORTANT: The interrupt controller MUST be set to level triggering when using		*/
/*				           the interrupts (uart_init() with TXsize != 0 or RXsize != 0)				*/
/*																									*/
/*																									*/
/* Copyright (c) 2016-2019, Code-Time Technologies Inc. All rights reserved.						*/
/*																									*/
/* Redistribution and use in source and binary forms, with or without								*/
/* modification, are permitted provided that the following conditions are met:						*/
/*																									*/
/* 1. Redistributions of source code must retain the above copyright notice, this					*/
/*    list of conditions and the following disclaimer.												*/
/* 2. Redistributions in binary form must reproduce the above copyright notice,						*/
/*    this list of conditions and the following disclaimer in the documentation						*/
/*    and/or other materials provided with the distribution.										*/
/*																									*/
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND					*/
/* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED					*/
/* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE							*/
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR					*/
/* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES					*/
/* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;						*/
/* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND						*/
/* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT						*/
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS					*/
/* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.										*/
/*																									*/
/*																									*/
/*	$Revision: 1.28 $																				*/
/*	$Date: 2019/01/10 18:07:06 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* NOTES:																							*/
/*																									*/
/*		The set-up using uart_init may be a boit confusing and it's due to legacy.  The cricular	*/
/*		buffers were added later on and for backward compatibility it had to be implemented in		*/
/*		a bit convoluted way. So here's what to do for all the way the uart driver can operate.		*/
/*		Each direction set-up is indepenent from the other. e.g., RX can be set to circular buffer	*/
/*		with blocking amd TX can be set for the same device to polling with no blocking.			*/
/*																									*/
/*		- Polling (no blocking) :																	*/
/*			in uart_init(), the arguments RXsize / TXsize must be set to 0							*/
/*			The build options UART_CIRC_BUF_RX / UART_CIRC_BUF_TX should not be defined, but if		*/
/*			defined and	set to a +ve value, the cricular still IS NOT used and memory is allocated	*/
/*			for the circular buffer, That memory is dead data memory as it is never used.			*/
/*																									*/
/*		- No buffering with blocking on semaphore													*/
/*			in uart_init(), the arguments RXsize / TXsize must be set to a -ve value.  The build	*/
/*			options UART_CIRC_BUF_RX / UART_CIRC_BUF_TX	should not be defined or if defined, should	*/
/*			be set to a non +ve value (.i.e. <=0 and the value itself does not matter).				*/
/*																									*/
/*		- Message queue between BG and interrupts:													*/
/*			in uart_init(), the arguments RXsize / TXsize must be set to a +ve value. The value		*/
/*			defines the size of the queue. The build options UART_CIRC_BUF_RX / UART_CIRC_BUF_TX	*/
/*			should not be defined or if defined, should be set to a non +ve value (.i.e. <=0 and	*/
/*			the value itself does not matter).														*/
/*																									*/
/*		- Circular buffer (always with blocking)													*/
/*			The build options UART_CIRC_BUF_RX / UART_CIRC_BUF_TX must be defined and set to a +ve	*/
/*			value. The value is the size of the circular buffer. In uart_init() the arguments		*/
/*			RXsize / TXsize must be set to a non-zero value (the exact value does not matter).		*/
/*																									*/
/* LIMITATIONS:																						*/
/*																									*/
/* NOT YET SUPPORTED:																				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#if defined(_STANDALONE_)
  #include "mAbassi.h"
#elif defined(_UABASSI_)						 	/* It is the same include file in all cases.	*/
  #include "mAbassi.h"								/* There is a substitution during the release 	*/
#elif defined (OS_N_CORE)							/* This file is the same for Abassi, mAbassi,	*/
  #include "mAbassi.h"								/* and uAbassi, so Code Time uses these checks	*/
#else												/* to not have to keep 3 quasi identical		*/
  #include "mAbassi.h"								/* copies of this file							*/
#endif

#include "dw_uart.h"

#ifdef __SAL_H__									/* When used in standalone, some build options	*/
  #undef UART_SINGLE_MUTEX							/* cannot be set, so they are forced to their	*/
  #undef UART_SLEEP									/* default values								*/
#endif

#ifndef UART_CIRC_BUF_RX							/* If the RX direction uses a circular buffer	*/
  #define UART_CIRC_BUF_RX		0					/* for accumulation.  if >0, is the size of		*/
#endif												/* the circular buffer							*/

#ifndef UART_CIRC_BUF_TX							/* If the TX direction uses a circular buffer	*/
  #define UART_CIRC_BUF_TX		0					/* for accumulation.  if >0, is the size of		*/
#endif												/* the circular buffer							*/

#ifndef UART_SLEEP									/* When polling, if the task can go into sleep	*/
  #define UART_SLEEP			0					/* Be aware the FIFOs are 128 deep and baud at	*/
#endif												/* 115200 will fill the RX FIFO in ~11 ms		*/

#ifndef UART_FULL_PROTECT							/* To use ISR on/off + spinlock to protect		*/
  #define UART_FULL_PROTECT		0					/* access instead of mutexes					*/
#endif

#if ((UART_FULL_PROTECT) != 0)						/* When full access protection done with ISRs	*/
  #undef  UART_SINGLE_MUTEX							/* on/off + spinlock, mutexes are redundant		*/
  #define UART_SINGLE_MUTEX		-1
#endif

#ifndef UART_SINGLE_MUTEX							/* If single mutex used for all devices or each	*/
  #define UART_SINGLE_MUTEX		-1					/* device has its own mutex						*/
#endif												/* ==0: individual / >0: single /-ve: none		*/
													/* Set to none as "C" lib should protect		*/
#ifndef UART_BUS_WIDTH
  #define UART_BUS_WIDTH		4					/* Bus width to access the UART.				*/
#endif

#ifndef UART_ARG_CHECK
  #define UART_ARG_CHECK		1					/* If checking validity of function arguments	*/
#endif

#ifndef UART_TOUT
  #if ((OX_TIMER_US) != 0)							/* Timeouts are supported						*/
	#define UART_TOUT		(1*(OS_TICK_PER_SEC))	/* Safety to not remain block forever			*/
  #else
	#define UART_TOUT		(-1)
  #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* These are internal defintions that enable the use of multiple devices type UART drivers			*/
/* Doing this keep the whole core code of the UART the same across all types of UART				*/
/* the UART generic UART_??? and/or specific ???_UART_ tokens are remapped to MY_UART_??? tokens	*/

#ifdef __SAL_H__
  #undef DW_UART_SINGLE_MUTEX
  #undef DW_UART_SLEEP
#endif

#ifdef DW_UART_CIRC_BUF_RX
  #define MY_UART_CIRC_BUF_RX		(DW_UART_CIRC_BUF_RX)
#else
  #define MY_UART_CIRC_BUF_RX		(UART_CIRC_BUF_RX)
#endif

#ifdef DW_UART_CIRC_BUF_TX
  #define MY_UART_CIRC_BUF_TX		(DW_UART_CIRC_BUF_TX)
#else
  #define MY_UART_CIRC_BUF_TX		(UART_CIRC_BUF_TX)
#endif

#ifdef DW_UART_SLEEP
  #define MY_UART_SLEEP				(DW_UART_SLEEP)
#else
  #define MY_UART_SLEEP				(UART_SLEEP)
#endif

#ifdef DW_UART_FULL_PROTECT
  #define MY_UART_FULL_PROTECT		(DW_UART_FULL_PROTECT)
#else
  #define MY_UART_FULL_PROTECT		(UART_FULL_PROTECT)
#endif

#ifdef DW_UART_SINGLE_MUTEX
  #define MY_UART_SINGLE_MUTEX		(DW_UART_SINGLE_MUTEX)
#else
  #define MY_UART_SINGLE_MUTEX		(UART_SINGLE_MUTEX)
#endif

#ifdef DW_UART_BUS_WIDTH
  #define MY_UART_BUS_WIDTH			(DW_UART_BUS_WIDTH)
#else
  #define MY_UART_BUS_WIDTH			(UART_BUS_WIDTH)
#endif

#ifdef DW_UART_ARG_CHECK
  #define MY_UART_ARG_CHECK			(DW_UART_ARG_CHECK)
#else
  #define MY_UART_ARG_CHECK			(UART_ARG_CHECK)
#endif

#ifdef DW_UART_TOUT
  #define MY_UART_TOUT				(DW_UART_TOUT)
#else
  #define MY_UART_TOUT				(UART_TOUT)
#endif

#ifdef DW_UART_CLK
  #define MY_UART_CLK				(DW_UART_CLK)
#elif defined(UART_CLK)
  #define MY_UART_CLK				(UART_CLK)
#endif

#define MY_UART_MAX_DEVICES			(DW_UART_MAX_DEVICES)
#define MY_UART_LIST_DEVICE			(DW_UART_LIST_DEVICE)

#define MY_UART_PREFIX				DW_UART_PREFIX
#define MY_UART_ASCII				DW_UART_ASCII

#undef  UART_ADD_PREFIX
#define UART_ADD_PREFIX				DW_UART_ADD_PREFIX

/* ------------------------------------------------------------------------------------------------ */
/* Base addresses of the HW UART modules and other platform specific information					*/

#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)		/* Arria V / Cyclone V							*/
  static volatile void * const g_HWaddr[2] = { (volatile uint32_t *)0xFFC02000,
                                               (volatile uint32_t *)0xFFC03000
                                             };

  #define UART_RESET_DEV(Dev)	do {																\
									volatile int _i;												\
									*((volatile uint32_t *)0xFFD05014) |=  (1U << ((Dev)+16));		\
									for ( _i=0 ; _i<256 ; _i++ );									\
									*((volatile uint32_t *)0xFFD05014) &= ~(1U << ((Dev)+16));		\
								} while(0)

 #ifndef MY_UART_CLK
  #define MY_UART_CLK			100000000			/* 100MHz used on all supported boards			*/
 #endif

 #if ((MY_UART_MAX_DEVICES) > 2)
  #warning "Too many UART devices for Arria V / Cyclone V:"
  #error   "      Set UART_MAX_DEVICES <= 2"
 #endif

 #if ((MY_UART_BUS_WIDTH) != 4)
  #error   "UART_BUS_WIDTH must be set to 4 on this platform"
 #endif

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)	/* Arria 10										*/
  static volatile void * const g_HWaddr[2] = { (volatile uint32_t *)0xFFC02000,
                                               (volatile uint32_t *)0xFFC02100
                                             };

  #define UART_RESET_DEV(Dev)	do {																\
									volatile int _i;												\
									*((volatile uint32_t *)0xFFD05028) |=  (1U << ((Dev)+16));		\
									for ( _i=0 ; _i<256 ; _i++ );									\
									*((volatile uint32_t *)0xFFD05028) &= ~(1U << ((Dev)+16));		\
								} while(0)

 #ifndef MY_UART_CLK
  #define MY_UART_CLK			 50000000
 #endif

 #if ((MY_UART_NMB_DEVICES) > 2)
  #warning "Too many UART devices for Arria 10:"
  #error   "      Set UART_MAX_DEVICES <= 2"
 #endif

 #if ((MY_UART_BUS_WIDTH) != 4)
  #error   "UART_BUS_WIDTH must be set to 4 on this platform"
 #endif

#else
  	#error "Unsupported platform specified by OS_PLATFORM"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* You shouldn't need to mofify anything below														*/
/* ------------------------------------------------------------------------------------------------ */
/* UART register index definitions																	*/
/* They are accessed as uint32_t, uint16_t or uint8_t depending on the UART_BUS_WIDTH				*/


#define UART_RBR_THR_DLL_REG	(0x00/4)
#define UART_IER_DLH_REG		(0x04/4)
#define UART_FCR_REG			(0x08/4)
#define UART_LCR_REG			(0x0C/4)
#define UART_MCR_REG			(0x10/4)
#define UART_LSR_REG			(0x14/4)
#define UART_MSR_REG			(0x18/4)
#define UART_SCR_REG			(0x1C/4)
#define UART_SRBR_REG			(0x30/4)
#define UART_STHR_REG			(0x34/4)
#define UART_FAR_REG			(0x70/4)
#define UART_TFR_REGNT			(0x74/4)
#define UART_RFW_REG			(0x78/4)
#define UART_USR_REG			(0x7C/4)
#define UART_TFL_REG			(0x80/4)
#define UART_RFL_REG			(0x84/4)
#define UART_SRR_REG			(0x88/4)
#define UART_SRTS_REG			(0x8C/4)
#define UART_SBCR_REG			(0x90/4)
#define UART_SDMAM_REG			(0x94/4)
#define UART_SFE_REG			(0x98/4)
#define UART_SRT_REG			(0x9C/4)
#define UART_STET_REG			(0xA0/4)
#define UART_HTX_REG			(0xA4/4)
#define UART_DMASA_REG			(0xA8/4)
#define UART_CPR_REG			(0xF4/4)
#define UART_UCV_REG			(0xF8/4)
#define UART_CTR_REG			(0xFC/4)

/* ------------------------------------------------------------------------------------------------ */
/* Other than configuration, these are all the UART access operations that have to be performed		*/

													/* Check if room for RX or data in TX			*/
#define UART_RX_EMPTY(Cfg)			(0 == ((Cfg)->HW[UART_USR_REG] & 8U))
#define UART_TX_FULL(Cfg)			(0 == ((Cfg)->HW[UART_USR_REG] & 2U))

													/* Read / write data from / to the controller	*/
#define UART_DATA_RX(Cfg, Var)		do { (Var)=(char)((Cfg)->HW[UART_RBR_THR_DLL_REG]);   } while(0)
#define UART_DATA_TX(Cfg, Var)		do { (Cfg)->HW[UART_RBR_THR_DLL_REG]=(uint32_t)(Var); } while(0)

													/* Enable / disable RX or TX interrupts			*/
#define UART_RX_DISABLE(Cfg)		do {(Cfg)->HW[UART_IER_DLH_REG] &= ~1U;} while(0)
#define UART_RX_ENABLE(Cfg)			do {(Cfg)->HW[UART_IER_DLH_REG] |=  1U;} while(0)
#define UART_TX_DISABLE(Cfg)		do {(Cfg)->HW[UART_IER_DLH_REG] &= ~2U;} while(0)
#define UART_TX_ENABLE(Cfg)			do {(Cfg)->HW[UART_IER_DLH_REG] |=  2U;} while(0)

													/* To acknowledge / clear the raised interrupt	*/
#define UART_CLR_RX_INT(Cfg)		OX_DO_NOTHING();
#define UART_CLR_TX_INT(Cfg)		OX_DO_NOTHING();

/* ------------------------------------------------------------------------------------------------ */
/* UART driver is special: can be a stub when used with System Call Layer where stdio is not used	*/

#if ((MY_UART_LIST_DEVICE) == 0U) 
int UART_ADD_PREFIX(uart_recv)  (int Dev, char *Buff, int Len)
{
	return(0);
}
int UART_ADD_PREFIX(uart_send)  (int Dev, const char *Buff, int Len)
{
	return(0);
}
int UART_ADD_PREFIX(uart_filt)  (int Dev, int Enable, int Filter)
{
	return(0);
}
int UART_ADD_PREFIX(uart_init)  (int Dev, int Baud, int Nbits, int Parity, int Stop,
                                 int RXsize, int TXsize, int Filter)
{
	return(0);
}
#else

/* ------------------------------------------------------------------------------------------------ */
/* This section uses a lots of macros but it's to reduce the size of the data requirements by only	*/
/* creating descriptor / mutexes etc for the devices in use ("1" in MY_UART_LIST_DEVICE)			*/

#ifndef MY_UART_MAX_DEVICES
 #if   (((MY_UART_LIST_DEVICE) & 0x200) != 0U)
  #define MY_UART_MAX_DEVICES	10
 #elif (((MY_UART_LIST_DEVICE) & 0x100) != 0U)
  #define MY_UART_MAX_DEVICES	9
 #elif (((MY_UART_LIST_DEVICE) & 0x080) != 0U)
  #define MY_UART_MAX_DEVICES	8
 #elif (((MY_UART_LIST_DEVICE) & 0x040) != 0U)
  #define MY_UART_MAX_DEVICES	7
 #elif (((MY_UART_LIST_DEVICE) & 0x020) != 0U)
  #define MY_UART_MAX_DEVICES	6
 #elif (((MY_UART_LIST_DEVICE) & 0x010) != 0U)
  #define MY_UART_MAX_DEVICES	5
 #elif (((MY_UART_LIST_DEVICE) & 0x008) != 0U)
  #define MY_UART_MAX_DEVICES	4
 #elif (((MY_UART_LIST_DEVICE) & 0x004) != 0U)
  #define MY_UART_MAX_DEVICES	3
 #elif (((MY_UART_LIST_DEVICE) & 0x002) != 0U)
  #define MY_UART_MAX_DEVICES	2
 #elif (((MY_UART_LIST_DEVICE) & 0x001) != 0U)
  #define MY_UART_MAX_DEVICES	1
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* This section figures out how to remap the individual devices specified by MY_UART_LIST_DEVICE	*/
/* e.g. if MY_UART_LIST_DEVICE == 0x12, the only descriptors/mutexes for 2 devices are required and	*/
/*      device#1 is remapped to [0] and device #4 is remapped to [1]								*/

#if (((MY_UART_LIST_DEVICE) & 0x001) != 0)
  #define UART_CNT_0	0
  #define UART_IDX_0	0
#else
  #define UART_CNT_0	(-1)
  #define UART_IDX_0	(-1)
#endif
#if (((MY_UART_LIST_DEVICE) & 0x002) != 0)
  #define UART_CNT_1	((UART_CNT_0) + 1)
  #define UART_IDX_1	((UART_CNT_0) + 1)
#else
  #define UART_CNT_1	(UART_CNT_0)
  #define UART_IDX_1	-1
#endif
#if (((MY_UART_LIST_DEVICE) & 0x004) != 0)
  #define UART_CNT_2	((UART_CNT_1) + 1)
  #define UART_IDX_2	((UART_CNT_1) + 1)
#else
  #define UART_CNT_2	(UART_CNT_1)
  #define UART_IDX_2	-1
#endif
#if (((MY_UART_LIST_DEVICE) & 0x008) != 0)
  #define UART_CNT_3	((UART_CNT_2) + 1)
  #define UART_IDX_3	((UART_CNT_2) + 1)
#else
  #define UART_CNT_3	(UART_CNT_2)
  #define UART_IDX_3	-1
#endif
#if (((MY_UART_LIST_DEVICE) & 0x010) != 0)
  #define UART_CNT_4	((UART_CNT_3) + 1)
  #define UART_IDX_4	((UART_CNT_3) + 1)
#else
  #define UART_CNT_4	(UART_CNT_3)
  #define UART_IDX_4	-1
#endif
#if (((MY_UART_LIST_DEVICE) & 0x020) != 0)
  #define UART_CNT_5	((UART_CNT_4) + 1)
  #define UART_IDX_5	((UART_CNT_4) + 1)
#else
  #define UART_CNT_5	(UART_CNT_4)
  #define UART_IDX_5	-1
#endif
#if (((MY_UART_LIST_DEVICE) & 0x040) != 0)
  #define UART_CNT_6	((UART_CNT_5) + 1)
  #define UART_IDX_6	((UART_CNT_5) + 1)
#else
  #define UART_CNT_6	(UART_CNT_5)
  #define UART_IDX_6	-1
#endif
#if (((MY_UART_LIST_DEVICE) & 0x080) != 0)
  #define UART_CNT_7	((UART_CNT_6) + 1)
  #define UART_IDX_7	((UART_CNT_6) + 1)
#else
  #define UART_CNT_7	(UART_CNT_6)
  #define UART_IDX_7	-1
#endif
#if (((MY_UART_LIST_DEVICE) & 0x100) != 0)
  #define UART_CNT_8	((UART_CNT_7) + 1)
  #define UART_IDX_8	((UART_CNT_7) + 1)
#else
  #define UART_CNT_8	(UART_CNT_7)
  #define UART_IDX_8	-1
#endif
#if (((MY_UART_LIST_DEVICE) & 0x200) != 0)
  #define UART_CNT_9	((UART_CNT_8) + 1)
  #define UART_IDX_9	((UART_CNT_8) + 1)
#else
  #define UART_CNT_9	(UART_CNT_8)
  #define UART_IDX_9	-1
#endif

#define UART_NMB_DEVICES	((UART_CNT_9)+1)

/* ------------------------------------------------------------------------------------------------ */

static const int g_DevReMap[] = { UART_IDX_0
                               #if ((MY_UART_MAX_DEVICES) > 1)
                                 ,UART_IDX_1
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 2)
                                 ,UART_IDX_2
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 3)
                                 ,UART_IDX_3
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 4)
                                 ,UART_IDX_4
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 5)
                                 ,UART_IDX_5
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 6)
                                 ,UART_IDX_6
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 7)
                                 ,UART_IDX_7
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 8)
                                 ,UART_IDX_8
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 9)
                                 ,UART_IDX_9
                               #endif
                               };

/* ------------------------------------------------------------------------------------------------ */

#if ((MY_UART_SINGLE_MUTEX) == 0)
  static const char g_MtxNames[][16] = {
                                 MY_UART_ASCII "UART-0"
                               #if ((MY_UART_MAX_DEVICES) > 1)
                                ,MY_UART_ASCII "UART-1"
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 2)
                                ,MY_UART_ASCII "UART-2"
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 3)
                                ,MY_UART_ASCII "UART-3"
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 4)
                                ,MY_UART_ASCII "UART-4"
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 5)
                                ,MY_UART_ASCII "UART-5"
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 6)
                                ,MY_UART_ASCII "UART-6"
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 7)
                                ,MY_UART_ASCII "UART-7"
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 8)
                                ,MY_UART_ASCII "UART-8"
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 9)
                                ,MY_UART_ASCII "UART-9"
                               #endif
                               #if ((MY_UART_MAX_DEVICES) > 10)
                                #error "Increase g_MtxNames[] array size and init values"
                               #endif
                               };
#elif ((MY_UART_SINGLE_MUTEX) > 0)
  static const char g_MtxNames[16] = MY_UART_ASCII "UART";
#endif

static const char g_Names[][2][16] = {
                                 {MY_UART_ASCII "UARTRX-0", MY_UART_ASCII "UARTTX-0"}
                               #if ((MY_UART_MAX_DEVICES) > 1)
                                ,{MY_UART_ASCII "UARTRX-1", MY_UART_ASCII "UARTTX-1"}
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 2)
                                ,{MY_UART_ASCII "UARTRX-2", MY_UART_ASCII "UARTTX-2"}
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 3)
                                ,{MY_UART_ASCII "UARTRX-3", MY_UART_ASCII "UARTTX-3"}
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 4)
                                ,{MY_UART_ASCII "UARTRX-4", MY_UART_ASCII "UARTTX-4"}
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 5)
                                ,{MY_UART_ASCII "UARTRX-5", MY_UART_ASCII "UARTTX-5"}
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 6)
                                ,{MY_UART_ASCII "UARTRX-6", MY_UART_ASCII "UARTTX-6"}
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 7)
                                ,{MY_UART_ASCII "UARTRX-7", MY_UART_ASCII "UARTTX-7"}
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 8)
                                ,{MY_UART_ASCII "UARTRX-8", MY_UART_ASCII "UARTTX-8"}
                               #endif 
                               #if ((MY_UART_MAX_DEVICES) > 9)
                                ,{MY_UART_ASCII "UARTRX-9", MY_UART_ASCII "UARTTX-9"}
                               #endif
                               #if ((MY_UART_MAX_DEVICES) > 10)
                                #error "Increase g_Names[] array size and init values"
                               #endif
                               };

/* ------------------------------------------------------------------------------------------------ */

#if ((MY_UART_CIRC_BUF_RX) > 0)
  #define UART_MAILBOX_RX	0
#else
  #define UART_MAILBOX_RX	(OX_MAILBOX)
#endif

#if ((MY_UART_CIRC_BUF_TX) > 0)
  #define UART_MAILBOX_TX	0
#else
  #define UART_MAILBOX_TX	(OX_MAILBOX)
#endif

#if ((OX_MAILBOX) != 0)								/* Abassi must be set-up to have MBXput report	*/
  #if ((OX_MBXPUT_ISR) == 0)						/* valid return value (full or write OK)		*/
	#error "When using mailboxes with dw_uart, OS_MBXPUT_ISR MUST be set to non-zero"
  #endif
#endif


#if (((MY_UART_BUS_WIDTH) != 4) && ((MY_UART_BUS_WIDTH) != 2) && ((MY_UART_BUS_WIDTH) != 1))
  #error "MY_UART_BUS_WIDTH must be set to 4, 2, or 1"
#endif

/* ------------------------------------------------------------------------------------------------ */

#define UART_FILT_IN_CR_MASK   ((UART_FILT_IN_CR_CRLF)|(UART_FILT_IN_CR_LF)|(UART_FILT_IN_CR_DROP))
#define UART_FILT_IN_LF_MASK   ((UART_FILT_IN_LF_CRLF)|(UART_FILT_IN_LF_CR)|(UART_FILT_IN_LF_DROP))
#define UART_FILT_OUT_CR_MASK  ((UART_FILT_OUT_CR_CRLF)|(UART_FILT_OUT_CR_LF)|(UART_FILT_OUT_CR_DROP))
#define UART_FILT_OUT_LF_MASK  ((UART_FILT_OUT_LF_CRLF)|(UART_FILT_OUT_LF_CR)|(UART_FILT_OUT_LF_DROP))

/* ------------------------------------------------------------------------------------------------ */
/* When protection mutex(es) are requested															*/
/* Need to check against NULL in case the RTOS wasn;t started yet									*/

#if ((MY_UART_SINGLE_MUTEX) == 0)
  #define UART_MTX_LOCK(Cfg)	do { 																\
									if ((Cfg)->Mutex != (MTX_t*)NULL) {								\
										if (0 != MTXlock((Cfg)->Mutex, -1)) {						\
											return(-10);											\
										}															\
									}																\
								} while(0)
  #define UART_MTX_UNLOCK(Cfg)	do { 																\
									if ((Cfg)->Mutex != (MTX_t*)NULL) {								\
										MTXunlock((Cfg)->Mutex);									\
									}																\
								} while(0)
#elif ((MY_UART_SINGLE_MUTEX) > 0)
  #define UART_MTX_LOCK(Cfg)	do { 																\
									if (g_MyMutex != (MTX_t*)NULL) {								\
										if (0 != MTXlock(g_MyMutex, -1)) {							\
											return(-10);											\
										}															\
									}																\
								} while(0)
  #define UART_MTX_UNLOCK(Cfg)	do { 																\
									if (g_MyMutex != (MTX_t*)NULL) {								\
										MTXunlock(g_MyMutex);										\
									}																\
								} while(0)

  static MTX_t *g_MyMutex = (MTX_t *)NULL;
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Access protection for back-ground vs ISR.  The ISR handler is sometimes called from the			*/
/* background so mutexes cannot be used to provide exclusive access: mutexes cannot be locked in	*/
/* an ISR.																							*/

#if ((OX_N_CORE) == 1)								/* UART's register exclusive access control		*/
  #define UART_GET_EXCL(Cfg, x)		do {x=OSintOff();} while(0)
  #define UART_RLS_EXCL(Cfg, x)		(OSintBack(x))

#else												/* Multi-core also requires to use a spinlock	*/
  #define UART_GET_EXCL(Cfg, x)		do {x=OSintOff();												\
									    CORElock(UART_SPINLOCK, &(Cfg)->SpinLock, 0, COREgetID()+1);\
									} while(0)
  #define UART_RLS_EXCL(Cfg, x)		do {COREunlock(UART_SPINLOCK, &(Cfg)->SpinLock, 0);				\
									    OSintBack(x);												\
									} while(0)
#endif

/* ------------------------------------------------------------------------------------------------ */
/* The following ???_FA_EXCL are used to replace mutexes in the background to provide exclusive		*/
/* access the UART descriptor.																		*/
/* ???_FR_EXCL to protect register / interrupt handler access and are opposite to ???_R_EXCL		*/

#if ((MY_UART_FULL_PROTECT) == 0)					/* Not requested to perform local full protect	*/
  #define UART_GET_FA_EXCL(Dv, x)	OX_DO_NOTHING()
  #define UART_RLS_FA_EXCL(Dv, x)	OX_DO_NOTHING()
  #define UART_GET_FZ_EXCL(Dv, x)	UART_GET_EXCL(Dv, x)
  #define UART_RLS_FZ_EXCL(Dv, x)	UART_RLS_EXCL(Dv, x)
#else												/* Requested to perform full local protection	*/
  #define UART_GET_FA_EXCL(Dv, x)	UART_GET_EXCL(Dv, x)
  #define UART_RLS_FA_EXCL(Dv, x)	UART_RLS_EXCL(Dv, x)
  #define UART_GET_FZ_EXCL(Dv, x)	OX_DO_NOTHING()
  #define UART_RLS_FZ_EXCL(Dv, x)	OX_DO_NOTHING()
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Register access protection when enabling / disabling the RX and TX interrupts					*/
/* If the register(s) is read-modify-write, it requires protection, if not, then no protections		*/

#if 0												/* Not RMW - 2 reg: set reg and clear reg		*/
  #define UART_GET_R_EXCL(Dv, x)	OX_DO_NOTHING()
  #define UART_RLS_R_EXCL(Dv, x)	OX_DO_NOTHING()

#else												/* Is RMW  - 1 reg: write 0 or 1 to set/clear	*/
  #define UART_GET_R_EXCL(Dv, x)	UART_GET_EXCL(Dv, x)
  #define UART_RLS_R_EXCL(Dv, x)	UART_RLS_EXCL(Dv, x)

#endif

/* ------------------------------------------------------------------------------------------------ */
/* Local variables																					*/

#if ((MY_UART_BUS_WIDTH) == 4)						/* Select the register access size according to	*/
  typedef uint32_t UartReg_t;						/* the bus width								*/
#elif ((MY_UART_BUS_WIDTH) == 2)
  typedef uint16_t UartReg_t;
#lese
  typedef uint8_t UartReg_t;
#endif

typedef struct {									/* Per device: configuration					*/
	volatile UartReg_t *HW;							/* Base address of the UART peripheral			*/
	SEM_t *SemRX;									/* When using semaphore instead of timeout		*/
	SEM_t *SemTX;									/* When using semaphore instead of timeout		*/
	int    RXsize;									/* Size of the RX mailbox (0 when polling)		*/
	int    TXsize;									/* Size of the TX mailbox (0 when polling)		*/
	int    PendRX;									/* Pending char to read							*/
	int    PendTX;									/* Pending char to transmit						*/
	int    PrevRX;									/* Previously received character (unfiltered	*/
	int    IsInit;									/* If the device has been initialized			*/
	unsigned int Filter;							/* Operations about ECHO, CR & LF to perform	*/
  #if ((OX_N_CORE) >= 2)							/* Doing Read-Modify-Write of UART registers	*/
	int    SpinLock;								/* Need also a spinlock in extra of ISR disable	*/
  #endif											/* when multi-core								*/
  #if ((MY_UART_SINGLE_MUTEX) == 0)
	MTX_t *Mutex;									/* When 1 mutex per device						*/
  #endif
  #if ((UART_MAILBOX_RX) != 0)
	MBX_t *MboxRX;									/* RX mailbox									*/
    int    PendINT;									/* Safety used in UART RX interrupt handler		*/
  #endif
  #if ((UART_MAILBOX_TX) != 0)
	MBX_t *MboxTX;									/* TX mailbox									*/
  #endif
  #if ((MY_UART_CIRC_BUF_RX) > 0)
	char CBufRX[(MY_UART_CIRC_BUF_RX)+1];
	volatile int RXrdIdx;
	volatile int RXwrtIdx;
  #endif
  #if ((MY_UART_CIRC_BUF_TX) > 0)
	char CBufTX[(MY_UART_CIRC_BUF_TX)+1];
	volatile int TXrdIdx;
	volatile int TXwrtIdx;
  #endif
} UARTcfg_t;

static UARTcfg_t g_UARTcfg[UART_NMB_DEVICES];		/* Device configuration							*/
static int       g_CfgIsInit = 0;					/* To track first time an init occurs			*/

/* ------------------------------------------------------------------------------------------------ */
/* Local functions prototypes																		*/

static void UARTintRX(UARTcfg_t *Cfg, int IsBG);	/* UART interrupt handler for RX direction		*/
static void UARTintTX(UARTcfg_t *Cfg, int IsBG);	/* UART interrupt handler for TX direction		*/

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: uart_recv																				*/
/*																									*/
/* uart_recv - receive data from a UART device														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int uart_recv(int Dev, char *Buff, int Len)													*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : UART device number																	*/
/*		Buff : pointer to a buffer to hold the data received										*/
/*		Len  : number or max number of bytes (filtered) to read from the UART						*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int : number of characters in Buff															*/
/*            may be different from the number received due to filtering							*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int UART_ADD_PREFIX(uart_recv) (int Dev, char *Buff, int Len)
{
char ccc;											/* Character being handled						*/
int  Drop;											/* If filtering drops the received character	*/
int  GotEOF;										/* If has received the end of file				*/
int  GotEOL;										/* If has received the end of line				*/
int  Idx;											/* Index writing in Buff[]						*/
int  ii;											/* General purpose								*/
int  NoLock;										/* Boolean if can block or lock the mutex		*/
char OriCCC; 										/* Character before filtering					*/
int  OSnotRdy;										/* Boolean if the OS is running or not			*/
int  PollSem;										/* Boolean if doing polling						*/
int  ReMap;											/* Remapped "Dev" to index in g_UARTcfg[]		*/
int  Wait;											/* Wait time (0 or -1:forever)					*/
#if ((UART_MAILBOX_RX) != 0)
  intptr_t Data;									/* To grab the character from the ISR mailbox	*/
  int      Ret;										/* Return value grabbing data from the mailbox	*/
#endif
#if ((MY_UART_CIRC_BUF_RX) > 0)
  int RdIdx;										/* I'm the only one touching this one			*/
#endif

UARTcfg_t *MyCfg;									/* De-reference array for faster access			*/

static const char BSecho[2] ={ ' ', '\b'};			/* BS echo with expansion, add " \b" after '\b'	*/

  #if ((MY_UART_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	MyCfg = &g_UARTcfg[ReMap];

  #else
	if ((Dev < 0)									/* Device # must be within range to be valid	*/
	||  (Dev >= MY_UART_MAX_DEVICES)) {
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by MY_UART_LIST_DEVICE			*/
		return(-2);
	}

	if (Buff == (char*)NULL) {						/* NULL buffer, report error					*/
		return(-3);
	}

	MyCfg = &g_UARTcfg[ReMap];						/* This is my device configuration				*/

    if ((Len == 0) 									/* Exit when no data to read					*/
	||  (g_CfgIsInit == 0)							/* or nothing configured yet					*/
	||  (MyCfg->IsInit == 0)) {						/* or this device hasn't been initialized yet	*/
        return(0);									/* Report nothing read							*/
    }
  #endif
													/* Can't block or use MBX/MTX/SEM if the OS 	*/
	OSnotRdy = (TSKmyID() == (TSK_t *)NULL);		/* is not yet started							*/

   #ifdef __ABASSI_H__								/* Single core internals != multi-core ones		*/
	NoLock = (int)G_OSstate[0]						/* Can't lock/block if within an ISR			*/
	       | (0 == OSisrInfo())						/* Can't lock/block if the ISRs are disabled	*/
	       | OSnotRdy;
   #else											/* mAbassi or uAbassi							*/
	ii = OSintOff();								/* Must disable as Core # can switch in middle	*/
	  NoLock = (int)G_OSisrState[0][COREgetID()];	/* Can't lock/block if within an ISR			*/
	OSintBack(ii);
	NoLock  |= (0 == OSisrInfo())					/* Can't lock/block if the ISRs are disabled	*/
	        |  OSnotRdy;							/* Can't lock/block if OS not started			*/
   #endif

	Wait = -1;										/* Assume wait (forever) for RX data available	*/
	if (Len < 0) {									/* No waiting requested when Len is negative	*/
		Wait = 0;
		Len  = -Len;								/* Positive value is the max # of char to read	*/
	}

  #if ((MY_UART_CIRC_BUF_RX) <= 0)
	PollSem = (MyCfg->RXsize <= 0)					/* If has to do polling / use semaphore			*/
	        | OSnotRdy;								/* Must do polling if the OS not yet started	*/
  #else
	PollSem = OSnotRdy;								/* When circular buffer is used, waiting on the	*/
  #endif											/* semaphore will happen if MyCfg->RXsize <= 0	*/

  #if ((MY_UART_SINGLE_MUTEX) >= 0)					/* When mutexes are used for protection			*/
	if (NoLock == 0) {								/* Lock the mutex only if it can be done		*/
		UART_MTX_LOCK(MyCfg);
	}
  #endif

	GotEOF = 0;
	GotEOL = 0;
	Idx    = 0;										/* Index writing into Buff[]					*/
	do {
		Drop = 0;									/* Assume we are keeping this character			*/
		UART_GET_FA_EXCL(MyCfg, ii);				/* When requested to do local full protection	*/
		if (MyCfg->PendRX >= 0) {					/* If there is a pending RX char, grab it		*/
			ccc = (char)MyCfg->PendRX;				/* instead of reading the UART or the mailbox	*/
			MyCfg->PendRX = -1;						/* Indicate no more pending RX char				*/
			UART_RLS_FA_EXCL(MyCfg, ii);
		}											/* Pending chars exist because of filtering		*/
		else {										/* No pending char to grab, get a new one		*/
			UART_RLS_FA_EXCL(MyCfg, ii);
			if (PollSem != 0) {						/* Set-up for polling or semaphore				*/
				if (Wait == 0) {					/* Request to not wait for avail char			*/
					UART_GET_R_EXCL(MyCfg, ii);		/* When requested to do local full protection	*/
					if (UART_RX_EMPTY(MyCfg)) {		/* No char available							*/
						UART_RLS_R_EXCL(MyCfg, ii);
						break;						/* Abort the processing and exit				*/
					}
					else {
						UART_DATA_RX(MyCfg, ccc);
						UART_RLS_R_EXCL(MyCfg, ii);
					}
				}

				else {								/* Must wait until a char is available			*/
					UART_GET_R_EXCL(MyCfg, ii);		/* When requested to do local full protection	*/
					while (UART_RX_EMPTY(MyCfg)) {	/* Loop as long as RX FIFO empty				*/
						UART_RLS_R_EXCL(MyCfg, ii);
						if (NoLock == 0) { 			/* If can block, block on semaphore or sleep	*/
							if (MyCfg->RXsize < 0) {/* Wait on the RX semaphore						*/
								UART_GET_R_EXCL(MyCfg, ii);
								UART_RX_ENABLE(MyCfg);	/* FIFO is empty, make sure ISRs are enable	*/
								UART_RLS_R_EXCL(MyCfg,ii);
								SEMwaitBin(MyCfg->SemRX, MY_UART_TOUT);/* Posted: FIFO not empty	*/
							}						/* RX ISR disable in ISR when FIFO not empty	*/
						  #if (((MY_UART_SLEEP) > 0) && ((OX_TIMER_US) && ((OX_TIMEOUT) > 0)))
							else {					/* Can only sleep if timeouts are available		*/
								TSKsleep(MY_UART_SLEEP);	/* And authorized by the build option	*/
							}
						  #endif
						}
						UART_GET_R_EXCL(MyCfg, ii);	/* When requested to do local full protection	*/
					}
					UART_DATA_RX(MyCfg, ccc);
					UART_RLS_R_EXCL(MyCfg, ii);
				}
			}
		  #if ((MY_UART_CIRC_BUF_RX) > 0)			/* Circular buffer used							*/
			else {
				UART_GET_FA_EXCL(MyCfg, ii);		/* Need exclusive access to manipulate RXrdIdx	*/

				RdIdx = MyCfg->RXrdIdx;				/* Volatile: read once							*/
				if (Wait == 0) {					/* Not waiting for char to be received			*/
					if (RdIdx == MyCfg->RXwrtIdx) {	/* Nothing in the circ buffer but				*/
						UART_GET_FZ_EXCL(MyCfg, ii);/* manual call as ISR may be turned off			*/
						UARTintRX(MyCfg, 1);
						UART_RLS_FZ_EXCL(MyCfg, ii);
					}
					if (RdIdx == MyCfg->RXwrtIdx) {	/* If still nothing								*/
						UART_RLS_FA_EXCL(MyCfg, ii);
						break;						/* Request to not wait, skip the processing		*/
					}
				}

				while (RdIdx == MyCfg->RXwrtIdx) {	/* Wait for something in circ buffer			*/
					if ((MyCfg->RXsize != 0)		/* When ISRs are used							*/
					&&  (NoLock == 0)) { 			/* If can block, block on semaphore or sleep	*/
						UART_GET_FZ_EXCL(MyCfg, ii);/* Make sure ISRs are enable					*/
						UART_RX_ENABLE(MyCfg);		/* Circbuff is empty, make sure ISRs are enable	*/
						UART_RLS_FZ_EXCL(MyCfg, ii);
						if (RdIdx == MyCfg->RXwrtIdx) {	/* There is a possible race condition here	*/
							UART_RLS_FA_EXCL(MyCfg,ii);
							SEMwaitBin(MyCfg->SemRX, MY_UART_TOUT);
						}
						else {
							UART_RLS_FA_EXCL(MyCfg,ii);
						}
					}				
					else {							/* Can't block, call the ISR handler manually	*/
						UART_RLS_FA_EXCL(MyCfg, ii);
					  #if (((MY_UART_SLEEP) > 0) && ((OX_TIMER_US) && ((OX_TIMEOUT) > 0)))
						if (NoLock == 0) {			/* If can bloxk									*/
							TSKsleep(MY_UART_SLEEP);/* Can only sleep if timeouts are available		*/
						}							/* And authorized by the build option			*/
					  #endif
						UART_GET_EXCL(MyCfg, ii);	/* No ISRs: manual call to the ISR handler		*/
						UARTintRX(MyCfg, 1);
						UART_RLS_EXCL(MyCfg, ii);
					}
				}

				ccc = MyCfg->CBufRX[RdIdx];
				if (--RdIdx < 0) {
					RdIdx = MY_UART_CIRC_BUF_RX;
				}
				MyCfg->RXrdIdx = RdIdx;

				UART_RLS_FA_EXCL(MyCfg, ii);
			}

		  #elif ((UART_MAILBOX_RX) != 0)			/* This code only used when mailbox included	*/
			else {									/* Not polling, therefore use Mailboxes & ISRs	*/
				Ret = MBXget(MyCfg->MboxRX, (intptr_t *)&Data, 0);	/* Quick try first				*/
				if ((Ret != 0)						/* When nothing in mailbox, a char may be held	*/
				&&  (MyCfg->PendINT >= 0)) { 		/* in ->PendINT (the safety mechanism)			*/
					UART_GET_EXCL(MyCfg, ii);		/* To get the char in PendINT, exclusiv access	*/
					if (MyCfg->PendINT >= 0) {		/* Recheck in case pre-empted (should never)	*/
						Data           = (intptr_t)MyCfg->PendINT;
						MyCfg->PendINT = -1;
						Ret            = 0;
					}
					UART_RLS_EXCL(MyCfg, ii);
				}

				if (Ret != 0) {						/* Mailbox & PendINT are empty, wait for char	*/
					if (NoLock != 0) {				/* If can't block, then poll the mailbox		*/
						if (Wait == 0) {			/* Request to not wait, skip the processing		*/
							break;
						}
						do {						/* Either ISRs disabled or inside an interrupt	*/
							Ret = 1;
							UART_GET_EXCL(MyCfg, ii);	/* So call the RX interrupt handler manually*/
							if (MyCfg->PendINT >= 0) {	/* Check if something there					*/
								Data           = (intptr_t)MyCfg->PendINT;
								MyCfg->PendINT = -1;
								Ret            = 0;
							}
							else if ((MBXavail(MyCfg->MboxRX) == 0)
							     &&  (UART_RX_EMPTY(MyCfg) == 0)) {
								UART_DATA_RX(MyCfg, Data);
								Ret = 0;								
							}
							UART_RLS_EXCL(MyCfg, ii);
							if (Ret != 0) {
								Ret = MBXget(MyCfg->MboxRX, (intptr_t *)&Data, 0);
							}
						} while (Ret != 0);			/* If mailbox still empty, try again			*/
					}
					else {							/* We can block, so normal processing			*/
						UART_GET_R_EXCL(MyCfg, ii);	/* Make sure the ISRs are enable as will be		*/
						UART_RX_ENABLE(MyCfg);		/* blocking on RX char received					*/
						UART_RLS_R_EXCL(MyCfg, ii);	/* RX processing (That's another core)			*/
						Ret = MBXget(MyCfg->MboxRX, (intptr_t *)&Data, Wait);
						if (Ret != 0) {				/* Non-zero result is only when timeout = 0		*/
							break;					/* which is when no-wait requested				*/
						}							/* Abort the rest of the processing				*/
					}
				}

				UART_GET_R_EXCL(MyCfg, ii);
				UART_RX_ENABLE(MyCfg);				/* Make sure the RX ISRs are enable as we have	*/
				UART_RLS_R_EXCL(MyCfg, ii);			/* read the mailbox, so there is room for one	*/
				ccc = (char)Data;					/* char in it now for sure						*/
			}						
		  #endif
		}

		OriCCC = ccc;

		if (ccc == '\r') {							/* Input CR filtering / EOL detection			*/
			if (0U != (MyCfg->Filter & UART_FILT_IN_CR_MASK)) {
				if (0 != (MyCfg->Filter & UART_FILT_IN_CR_CRLF)) {
					UART_GET_FA_EXCL(MyCfg, ii);	/* When requested to do local full protection	*/
					MyCfg->PendRX = 0xFF & (int)'\n';
					UART_RLS_FA_EXCL(MyCfg, ii);
				}
				else if (0 != (MyCfg->Filter & UART_FILT_IN_CR_LF)) {
					ccc = '\n';
				}
				else if (0 != (MyCfg->Filter & UART_FILT_IN_CR_DROP)) {
					Drop = 1;
				}
			}
			if (0 != (MyCfg->Filter & UART_FILT_EOL_CR)) {
				GotEOL = 1;
			}
		}
		else if (ccc == '\n') {						/* Input LF filtering / EOL detection			*/
			if (0U != (MyCfg->Filter & UART_FILT_IN_LF_MASK)) {
				if (0 != (MyCfg->Filter & UART_FILT_IN_LF_CRLF)) {
					UART_GET_FA_EXCL(MyCfg, ii);	/* When requested to do local full protection	*/
					MyCfg->PendRX = 0xFF & (int)'\n';
					UART_RLS_FA_EXCL(MyCfg, ii);
					ccc = '\r';
				}
				else if (0 != (MyCfg->Filter & UART_FILT_IN_LF_CR)) {
					ccc = '\r';
				}
				else if (0 != (MyCfg->Filter & UART_FILT_IN_LF_DROP)) {
					Drop = 1;
				}
			}
			if (0 != (MyCfg->Filter & UART_FILT_EOL_LF)) {
				GotEOL = 1;
			}
			else if ((0 != (MyCfg->Filter & UART_FILT_EOL_CRLF))
			     &&  (MyCfg->PrevRX == '\r')) {
				GotEOL = 1;
			}
		}
		else if ((ccc == 4)
		     &&  (0 != (MyCfg->Filter & UART_FILT_EOF_CTRLD))) {
			GotEOF = 1;
		}
		else if ((ccc == 26)
		     &&  (0 != (MyCfg->Filter & UART_FILT_EOF_CTRLZ))) {
			GotEOF = 1;
		}

		MyCfg->PrevRX = OriCCC;

		if (Drop == 0) {							/* We do use this character						*/
			if (0U != (MyCfg->Filter & UART_ECHO)) {/* If set to echo on output, do it				*/
				UART_ADD_PREFIX(uart_send) (Dev, (const char *)&ccc, 1);
				if ((ccc == '\b')					/* If filtering expand BACKSPACE to output		*/
				&&  (0U != (MyCfg->Filter & UART_ECHO_BS_EXPAND))) {
					UART_ADD_PREFIX(uart_send) (Dev, (const char *)&BSecho[0], 2);
				}
			}
			Buff[Idx++] = ccc;						/* Memo the newly received/filtered character	*/
		}											/* Process as long a room in buffer or 			*/

		if ((GotEOL != 0)							/* When we've received the EOL, we are done		*/
		||  (GotEOF != 0)) {
			break;
		}
	} while (Idx < Len);							/* aborted when no-wait requested				*/

  #if ((MY_UART_SINGLE_MUTEX) >= 0)					/* When mutexes are used for protection			*/
	if (NoLock == 0) {								/* Remove lock if was obtained					*/
		UART_MTX_UNLOCK(MyCfg);
	}
  #endif

	return(Idx);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: uart_send																				*/
/*																									*/
/* uart_send - send data to the UART device															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int uart_send(int Dev, const char *Buff, int Len)											*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : UART device number																	*/
/*		Buff : pointer to a buffer that holds the data to send										*/
/*		Len  : number of bytes to send to the slave device											*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int  : number of character used from Buff													*/
/*             may be different from the number transmitted due to filtering						*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int UART_ADD_PREFIX(uart_send) (int Dev, const char *Buff, int Len)
{
char ccc;											/* Character being handled						*/
int  Drop;											/* If filtering drops the received character	*/
int  Idx;											/* Index reading from Buff[]					*/
int  ii;											/* General purpose								*/
int  InKrnl;										/* If called from the kernel (for logging)		*/
int  NoLock;										/* Boolean if can block or lock the mutex		*/
int  OSnotRdy;										/* Boolean if the OS is running or not			*/
int  PendTX;										/* Original contents of UARTcfg_t.PendTX		*/
int  PollSem;										/* Boolean if doing polling						*/
int  ReMap;											/* Remapped "Dev" to index in g_UARTcfg[]		*/
int  Wait;											/* Wait time (0 or -1:forever)					*/
int  WasAbort;										/* Boolean if the writing was aborted (no wait)	*/
#if ((UART_MAILBOX_TX) != 0)
  int  NextRd;										/* Next read index in the mailbox				*/
#endif
#if ((MY_UART_CIRC_BUF_TX) > 0)
  int LastWrt;										/* Next write index in the circ buffer			*/
  int WrtNow;										/* Current write index in the circ buffer		*/
#endif
UARTcfg_t *MyCfg;									/* De-reference array for faster access			*/

  #if ((MY_UART_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	MyCfg = &g_UARTcfg[ReMap];

  #else
	if ((Dev < 0)									/* Device # must be within range to be valid	*/
	||  (Dev >= MY_UART_MAX_DEVICES)) {
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by MY_UART_LIST_DEVICE			*/
		return(-2);
	}

	if (Buff == (char*)NULL) {						/* NULL buffer, report error					*/
		return(-3);
	}

	MyCfg = &g_UARTcfg[ReMap];						/* This is my device configuration				*/

	if ((Len == 0) 									/* No data to send, exit						*/
	||  (g_CfgIsInit == 0)							/* Nothing configured yet						*/
	||  (MyCfg->IsInit == 0)) {						/* This device hasn't been initialized yet		*/
		return(0);									/* Report nothing written						*/
	}
  #endif
													/* Can't block or use MBOX/MTX/SEM if the OS 	*/
	OSnotRdy = (TSKmyID() == (TSK_t *)NULL);		/* is not yet started							*/
	InKrnl   = 0;									/* Assume is not called from the kernel			*/
  #ifdef __ABASSI_H__								/* Single core internals != multi-core ones		*/
   #if ((OX_LOGGING_TYPE) == 1)						/* Only type 1 makes kernel to do direct print	*/
	InKrnl = (int)G_OSstate[2];						/* Memo if called from the kernel (logging)		*/
   #endif
	NoLock   = (int)G_OSstate[0]					/* Can't lock/block if within an ISR			*/
	         | InKrnl								/* can't lock/block if in the kernel (logging)	*/
	         | (0 == OSisrInfo())					/* Can't lock/block if the ISRs are disabled	*/
	         | OSnotRdy;
  #else
	ii = OSintOff();								/* Must disable as Core # can switch in middle	*/
	{
	 int CoreID;
	 CoreID = COREgetID();
	 NoLock = (int)G_OSisrState[0][CoreID];			/* Can't lock/block if within an ISR			*/
   #if ((OX_LOGGING_TYPE) == 1)						/* Only type 1 makes kernel to do direct print	*/
	 InKrnl = (int)G_OSisrState[3][CoreID];			/* Memo if called from the kernel (logging)		*/
   #endif
	}
	OSintBack(ii);
	NoLock  |= InKrnl								/* Can't lock/block if in the kernel			*/
            |  (0 == OSisrInfo())					/* Can't lock/block if the ISRs are disabled	*/
	        |  OSnotRdy;							/* Can't lock/block if OS not started			*/
  #endif

	Wait = -1;										/* Assume wait (forever) for roam in TX FIFO	*/
	if (Len < 0) {									/* No waiting requested when Len is negative	*/
		Wait = 0;
		Len  = -Len;								/* Positive value is the # of char to read		*/
	}

  #if ((MY_UART_CIRC_BUF_TX) <= 0)
	PollSem = (MyCfg->TXsize <= 0)					/* If has to do polling / wait on semaphore		*/
	        | InKrnl								/* Polling must be used when called from kernel	*/
	        | OSnotRdy;								/* MBX use would re-enter the kernel: very bad	*/
  #else
	PollSem = InKrnl								/* Polling must be used when called from kernel	*/
	        | OSnotRdy;								/* When circular buffer is used, waiting on the	*/
  #endif											/* semaphore will happen if MyCfg->TXsize <= 0	*/

  #if ((MY_UART_SINGLE_MUTEX) >= 0)					/* When mutexes are used for protection			*/
	if (NoLock == 0) {								/* Lock the mutex only if it can be done		*/
		UART_MTX_LOCK(MyCfg);
	}
  #endif

	for (Idx=0 ; (Idx<Len) || (MyCfg->PendTX >= 0) ; ) {
		Drop   = 0;									/* Assume we transmit the processed character	*/

		UART_GET_FA_EXCL(MyCfg, ii);				/* When requested to do local full protection	*/
		PendTX = MyCfg->PendTX;						/* Memo original in case no wait and out full	*/
		if (PendTX >= 0) {							/* If there is a pending TX char, send it		*/
			ccc           = MyCfg->PendTX;			/* instead of reading the supplied buffer		*/
			MyCfg->PendTX = -1;						/* Tag these is no more pending TX char			*/
			UART_RLS_FA_EXCL(MyCfg, ii);
		}											/* Pending chars exist because of filtering		*/
		else {										/* No pending TX char, use next from buffer		*/
			UART_RLS_FA_EXCL(MyCfg, ii);
			ccc = Buff[Idx++];
			if ((ccc == '\r')						/* Output CR filtering							*/
			&&  (0U != (MyCfg->Filter & UART_FILT_OUT_CR_MASK))) {
				if (0 != (MyCfg->Filter & UART_FILT_OUT_CR_CRLF)) {
					UART_GET_FA_EXCL(MyCfg, ii);	/* When requested to do local full protection	*/
					MyCfg->PendTX = 0xFF & (int)'\n';
					UART_RLS_FA_EXCL(MyCfg, ii);
				}
				else if (0 != (MyCfg->Filter & UART_FILT_OUT_CR_LF)) {
					ccc = '\n';
				}
				else if (0 != (MyCfg->Filter & UART_FILT_OUT_CR_DROP)) {
					Drop = 1;
				}
			}
			else if ((ccc == '\n')					/* Output LF filtering							*/
			&&       (0U != (MyCfg->Filter & UART_FILT_OUT_LF_MASK))) {
				if (0 != (MyCfg->Filter & UART_FILT_OUT_LF_CRLF)) {
					UART_GET_FA_EXCL(MyCfg, ii);	/* When requested to do local full protection	*/
					MyCfg->PendTX = 0xFF & (int)'\n';
					UART_RLS_FA_EXCL(MyCfg, ii);
					ccc = '\r';
				}
				else if (0 != (MyCfg->Filter & UART_FILT_OUT_LF_CR)) {
					ccc = '\r';
				}
				else if (0 != (MyCfg->Filter & UART_FILT_OUT_LF_DROP)) {
					Drop = 1;
				}
			}
		}

		if (Drop == 0) {							/* Not dropping the character, queue/transmit 	*/
			WasAbort = 0;
			if (PollSem != 0) {						/* Polling/Sema: check for room in the TX FIFO	*/
				UART_GET_R_EXCL(MyCfg, ii);			/* When requested to do local full protection	*/
				while (UART_TX_FULL(MyCfg)) {
					UART_RLS_R_EXCL(MyCfg, ii);
					if (Wait == 0) {				/* No room in the TX FIFO and requested to not	*/
						WasAbort = 1;				/* wait, so abort								*/
						break;
					}
					if (NoLock == 0) { 				/* If can block, semaphore or sleep				*/
						if (MyCfg->TXsize < 0) {	/* Wait on the TX semaphore						*/
							UART_GET_R_EXCL(MyCfg, ii);	/* Get exclusive access (ISR disables TX	*/
							UART_TX_ENABLE(MyCfg);	/* As FIFO is full, make sure ISRs are enable	*/
							UART_RLS_R_EXCL(MyCfg, ii);	/* Release the exclusive access				*/
							SEMwaitBin(MyCfg->SemTX, MY_UART_TOUT);	/* Posted when FIFO <= 1/2		*/
						}							/* The int handler will disable the TX ISRs		*/
					  #if (((MY_UART_SLEEP) > 0) && ((OX_TIMER_US) && ((OX_TIMEOUT) > 0)))
						else {						/* Can only sleep if timeouts are available		*/
							TSKsleep(MY_UART_SLEEP);/* And authorized by the build option			*/
						}
					  #endif
					}
					UART_GET_R_EXCL(MyCfg, ii);		/* When requested to do local full protection	*/
				}									/* When while is done, may room in TX FIFO		*/
				if (WasAbort == 0) {				/* or may be there was an abort					*/
					UART_DATA_TX(MyCfg, ccc);
				}									/* If not access protected, may lose this write	*/
				UART_RLS_R_EXCL(MyCfg, ii);
			}										/* If not mutex protected, may drop, too bad	*/
		  #if ((MY_UART_CIRC_BUF_TX) > 0)
			else {
				UART_GET_FA_EXCL(MyCfg, ii);		/* Need exclusive access to manipulate TXwrtIdx	*/

				WrtNow = MyCfg->TXwrtIdx;
				if (Wait == 0) {					/* No room in the circular buffer and requested	*/
					if (WrtNow == MyCfg->TXrdIdx) {
						UART_GET_FZ_EXCL(MyCfg, ii);
						UARTintTX(MyCfg, 1);		/* Manual xfer in case ISR are turned off		*/
						UART_RLS_FZ_EXCL(MyCfg, ii);
					}
					if (WrtNow == MyCfg->TXrdIdx) {/* If still no room, can't do more				*/
						UART_RLS_FA_EXCL(MyCfg, ii);
						WasAbort = 1;
						break;
					}
				}

				LastWrt = WrtNow;
				if (++LastWrt > MY_UART_CIRC_BUF_TX) {	/* Last write position						*/
					LastWrt = 0;
				}

				if ((LastWrt == MyCfg->TXrdIdx)		/* If the circular buffer is empty				*/
				&&  (0 == UART_TX_FULL(MyCfg))) {	/* And room in FIFO, immediate write			*/
					UART_GET_FZ_EXCL(MyCfg, ii);	
					UART_DATA_TX(MyCfg, ccc);
					UART_RLS_FZ_EXCL(MyCfg, ii);
					UART_RLS_FA_EXCL(MyCfg, ii);
				}
				else {								/* Circ buffer out empty, or no room in FIFO	*/
					while (WrtNow == MyCfg->TXrdIdx) {	/* Wait for room in circ buffer				*/
						if ((MyCfg->TXsize != 0)	/* When ISRs are used							*/
						&&  (NoLock == 0)) { 		/* If can block, block on semaphore				*/
							UART_GET_FZ_EXCL(MyCfg, ii);/* Get exclusive access (ISR disables TX	*/
							UART_TX_ENABLE(MyCfg);	/* As FIFO is full, make sure ISRs are enable	*/
							UART_RLS_FZ_EXCL(MyCfg, ii);
							if (WrtNow == MyCfg->TXrdIdx) {
								UART_RLS_FA_EXCL(MyCfg, ii);
								SEMwaitBin(MyCfg->SemTX, MY_UART_TOUT);
							}
							else {
								UART_RLS_FA_EXCL(MyCfg, ii);	
							}
						}
						else {						/* Can't block, call the ISR handler manually	*/
							UART_RLS_FA_EXCL(MyCfg, ii);
						  #if (((MY_UART_SLEEP) > 0) && ((OX_TIMER_US) && ((OX_TIMEOUT) > 0)))
							if (NoLock == 0) {		/* If can bloxk									*/
								TSKsleep(MY_UART_SLEEP);/* And authorized by the build option		*/
							}
						  #endif
							UART_GET_EXCL(MyCfg, ii);
							UARTintTX(MyCfg, 1);
							UART_RLS_EXCL(MyCfg, ii);
						}
						UART_GET_FA_EXCL(MyCfg, ii);	
					}

					MyCfg->CBufTX[MyCfg->TXwrtIdx] = ccc;
					if (--MyCfg->TXwrtIdx < 0) {
						MyCfg->TXwrtIdx = MY_UART_CIRC_BUF_TX;
					}
					UART_RLS_FA_EXCL(MyCfg, ii);

					if (MyCfg->TXsize == 0) {		/* ISRs are not used, must wait for all char	*/
						LastWrt = MyCfg->TXwrtIdx;
						if (++LastWrt > MY_UART_CIRC_BUF_TX) {
							LastWrt = 0;
						}
						UART_GET_FA_EXCL(MyCfg, ii);/* to be out. Done manually						*/
						do {
							UART_GET_FZ_EXCL(MyCfg, ii);
							UARTintTX(MyCfg, 1);
							UART_RLS_FZ_EXCL(MyCfg, ii);

							UART_RLS_FA_EXCL(MyCfg, ii);/* Make sure ISRs can run when full protect	*/
							UART_GET_FA_EXCL(MyCfg, ii);
						} while(LastWrt != MyCfg->TXrdIdx);

						UART_RLS_FA_EXCL(MyCfg, ii);
					}
				}
			}
		  #elif ((UART_MAILBOX_TX) != 0)			/* This code only used when mailbox included	*/
			else {									/* Mailbox & ISRs								*/
				UART_GET_R_EXCL(MyCfg, ii);
				NextRd = MyCfg->MboxTX->RdIdx-1;
				if (NextRd < 0) {
					NextRd = MyCfg->MboxTX->Size;
				}
				if ((NextRd == MyCfg->MboxTX->WrtIdx)	/* If the mailbox is empty 					*/
				&&  (0 == UART_TX_FULL(MyCfg))) {	/* and room in the TX FIFO, direct write		*/
					UART_DATA_TX(MyCfg, ccc);
					UART_RLS_R_EXCL(MyCfg, ii);
				}
				else {
					UART_RLS_R_EXCL(MyCfg, ii);
					if (0 != MBXput(MyCfg->MboxTX, (intptr_t)ccc, 0)) {/* Trying to fill the mbox	*/
						UART_GET_R_EXCL(MyCfg, ii);	/* We'll fill until full, then will block		*/
						UART_TX_ENABLE(MyCfg);		/* Make sure TX ISRs are enabled				*/
						UART_RLS_R_EXCL(MyCfg, ii);	/* The ISR will empty the mailbox				*/
						if (0 != MBXput(MyCfg->MboxTX, (intptr_t)ccc, (NoLock != 0) ? 0 : Wait)) {
							if (Wait == 0) {		/* No room in the mailbox and no wait / no lock	*/
								WasAbort = 1;		/* can't go into kernel to handle MBX			*/
							}
						}
					}
				}
			}
		  #endif

			if (WasAbort != 0) {					/* If there was an abort, rewind back before	*/
				UART_GET_FA_EXCL(MyCfg, ii);		/* When requested to do local full protection	*/
				MyCfg->PendTX = PendTX;				/* filtering									*/
				UART_RLS_FA_EXCL(MyCfg, ii);
				if (PendTX < 0) {
					Idx--;
				}
				break;								/* Exit the outer for() loop					*/
			}
			else if (MyCfg->TXsize != 0) {			/* Was able to put a character in mailbox		*/
				UART_GET_R_EXCL(MyCfg, ii);			/* so that means the TX ISR can pull it			*/
				UART_TX_ENABLE(MyCfg);				/* Exclusive access as ISR disable TX ISRs		*/
				UART_RLS_R_EXCL(MyCfg, ii);
			}
		}
	}

  #if ((MY_UART_SINGLE_MUTEX) >= 0)					/* When mutexes are used for protection			*/
	if (NoLock == 0) {								/* Remove lock if was obtained					*/
		UART_MTX_UNLOCK(MyCfg);
	}
  #endif

	return(Idx);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: uart_filt																				*/
/*																									*/
/* uart_filt - modify the filter on a UART device													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int uart_filt(int Dev int Enable, int Filter);												*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev     : UART device number																*/
/*		Enable  : == 0 - disable the filter setting specified by Filter								*/
/*                != 0 - ensable the filter setting specified by Filter								*/
/*		Filter  : Filter mask to enable or disable													*/
/*		          if == 0, do nothing and return the current filer setting							*/
/*																									*/
/* RETURN VALUE:																					*/
/*		 0  : previous filter setting (only the original bits in Filter are reported)				*/
/*      -ve : error																					*/
/* ------------------------------------------------------------------------------------------------ */

int UART_ADD_PREFIX(uart_filt)  (int Dev, int Enable, int Filter)
{
UARTcfg_t *MyCfg;									/* De-reference array for faster access			*/
int        Prev;									/* Previous filter setting						*/
int        ReMap;									/* Remapped "Dev" to index in g_UARTcfg[]		*/

  #if ((MY_UART_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	MyCfg = &g_UARTcfg[ReMap];						/* This is this device configuration			*/

  #else
	if ((Dev < 0)									/* Device # must be within range to be valid	*/
	||  (Dev >= MY_UART_MAX_DEVICES)) {
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by MY_UART_LIST_DEVICE			*/
		return(-2);
	}

	MyCfg = &g_UARTcfg[ReMap];						/* This is this device configuration			*/

	if ((g_CfgIsInit == 0)							/* Make sure is accessing a device that has		*/
	||  (MyCfg->IsInit == 0)) {						/* been coofigured								*/
        return(-3);
    }

  #endif

	if (Filter == 0) {
		return(MyCfg->Filter);
	}

	Prev = (int)MyCfg->Filter						/* Returns the previous filter setting			*/
	     & Filter;									/* but only for the ones to set-up				*/

	if (Enable != 0) {								/* Update new filter set-up. recv & send		*/
		MyCfg->Filter |=  (unsigned int)Filter;		/* access Filter as read-only so no need to		*/
	}												/* protect the access							*/
	else {
		MyCfg->Filter &= ~(unsigned int)Filter;
	}

	return(Prev);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: uart_init																				*/
/*																									*/
/* uart_init - initialize a UART controller															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int uart_init(int Dev int Baud, int Nbits, int Parity, int Stop, int RXsize,				*/
/*		              int TXsize, int Filter);														*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev     : UART device number																*/
/*		Baud    : BaudRate to set																	*/
/*		Nbits   : Number of data bits (5->8)														*/
/*		Parity  : Parity																			*/
/*		          == 0 : none																		*/
/*		          +ve value: even parity bit														*/
/*		          -ve value:  odd parity bit														*/
/*		Stop    : Stop bit																			*/
/*		          == 10 : 1   stop bit																*/
/*		          == 15 : 1.5 stop bit																*/
/*						  NOTE: <= 10 is 1 stop bit, else is 1.5 stop bit							*/
/*		RXsize  : RX operating mode: Polling / Semaphore with ISRs / OS queue with ISRs				*/
/*				  -ve  : Blocking on a semaphore, uses ISRs											*/
/*		          == 0 : polling with sleep (when timeout available)								*/
/*		          +ve  : OS queue size, uses ISRs													*/
/*		TXsize  : TX operating mode: Polling / Semaphore with ISRs / OS queue with ISRs				*/
/*				  -ve  : Blocking on a semaphore, uses ISRs											*/
/*		          == 0 : polling with sleep (when timeout available)								*/
/*		          +ve  : OS queue size, uses ISRs													*/
/*		Filter  : Handling of \r, \n and \b (Is a set of bits OR any of the control masks)			*/
/*		          == 0 : do nothing																	*/
/*		          See #define of input & output character filtering control masks in "dw_uart.h"	*/
/*																									*/
/* RETURN VALUE:																					*/
/*		 0 : success																				*/
/*      -1 : error																					*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int UART_ADD_PREFIX(uart_init) (int Dev, int Baud, int Nbits, int Parity, int Stop,
                                int RXsize, int TXsize, int Filter)
{
UARTcfg_t *MyCfg;									/* De-reference array for faster access			*/
int        OSnotRdy;								/* If the OS is running or not					*/
int        ReMap;									/* Remapped "Dev" to index in g_UARTcfg[]		*/
#if ((MY_UART_SINGLE_MUTEX) >= 0)
  int      ii;										/* General purpose								*/
#endif

  #if ((MY_UART_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];

  #else
	if ((Dev < 0)									/* Device # must be within range to be valid	*/
	||  (Dev >= MY_UART_MAX_DEVICES)) {
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by MY_UART_LIST_DEVICE			*/
		return(-2);
	}

	if (Baud <= 0) {
		return(-3);
	}

	if ((Nbits < 5)
	||  (Nbits > 8)) {
		return(-4);
	}
  #endif

	OSnotRdy = (TSKmyID() == (TSK_t *)NULL);		/* Can't block or use MBOX if OS not started	*/

  #if ((MY_UART_SINGLE_MUTEX) >= 0)
	ii = 0x00;
	if ((g_CfgIsInit == 0)							/* Don't go through locking the mutex if has	*/
	&&  (OSnotRdy == 0)) {							/* been initialized								*/
		if (0 != MTXlock(G_OSmutex, -1)) {
			return(-5);								/* Although infinte wait, if deadlock detection	*/
		}											/* is enable, could return !=0 and not lock		*/
		ii = 0x01;
	}
  #endif

	if (g_CfgIsInit == 0) {							/* Check again due to small critical region		*/
		memset(&g_UARTcfg[0], 0, sizeof(g_UARTcfg));/* Safety in case BSS isn't zeroed				*/
	  #if ((MY_UART_SINGLE_MUTEX) > 0)				/* started, so check if mutex exixts			*/
		g_MyMutex = (MTX_t *)NULL;					/* When single mutex for all UARTs				*/
	  #endif
		g_CfgIsInit = 1;
	}

	MyCfg = &g_UARTcfg[ReMap];						/* This is this device configuration			*/

  #if ((MY_UART_SINGLE_MUTEX) >= 0)
	if (OSnotRdy == 0) {							/* May have gone through one init before OS was	*/
	  #if ((MY_UART_SINGLE_MUTEX) > 0)				/* started, so check if mutex exixts			*/
		if (g_MyMutex == (MTX_t *)NULL) {			/* Single mutex for all UARTs					*/
			g_MyMutex = MTXopen(&g_MtxNames[0]);
		}
		if (0 != MTXlock(g_MyMutex, -1)) {			/* Global lock									*/
			ii |= 0x10;								/* Although infinte wait, if deadlock detection	*/
		}											/* is enable, could return !=0 and not lock		*/
	  #else
		if (MyCfg->Mutex == (MTX_t *)NULL) {
			MyCfg->Mutex = MTXopen(&g_MtxNames[Dev][0]);
		}
		if (0 != MTXlock(MyCfg->Mutex, -1)) {		/* Device lock									*/
			ii |= 0x10;								/* Although infinte wait, if deadlock detection	*/
		}											/* is enable, could return !=0 and not lock		*/
	  #endif
	}

	if ((ii & 0x01) != 0) {							/* ii&0x01 != 0 is G_OSmutex has been locked	*/
		MTXunlock(G_OSmutex);						/* Not needed anymore because UART mutex should	*/
	}												/* be locked now								*/
	if ((ii & 0x10) != 0) {							/* ii&0x10 != 0 is UART mutex cannot be locked	*/
		return(-6);
	}
  #endif

	UART_RESET_DEV(Dev);							/* Hardware reset of the module					*/

	if (OSnotRdy != 0) {							/* OS not yet running, can't create mailboxes	*/
		RXsize = 0;									/* or semaphores. Overload size parameters		*/
		TXsize = 0;
	}

  #if ((UART_MAILBOX_RX) != 0)						/* Deal with mailbox only if available			*/
	if (RXsize > 0) {								/* Create them only once in case re-init		*/
		if (MyCfg->MboxRX == (MBX_t *)NULL) {
			MyCfg->MboxRX = MBXopen(&g_Names[Dev][0][0], RXsize);
			MBXputInISR(MyCfg->MboxRX);				/* UART RX mailboxes need full/non-full in ISR	*/
		}
	}
  #else												/* Mailboxes not available, back to polling		*/
	if (RXsize > 0) {								/* If mailbox was requested, use polling or 	*/
		RXsize = -1;								/* circular buffers with semaphore				*/
	}
  #endif

  #if ((UART_MAILBOX_TX) != 0)
	if (TXsize > 0) {
		if (MyCfg->MboxTX == (MBX_t *)NULL) {
			MyCfg->MboxTX = MBXopen(&g_Names[Dev][1][0], TXsize);
		}
	}
  #else												/* Mailboxes not available, back to polling		*/
	if (TXsize > 0) {								/* If mailbox was requested, use polling or 	*/
		TXsize = -1;								/* circular buffer with semaphore				*/
	}
  #endif

  #if ((MY_UART_CIRC_BUF_RX) > 0)
	MyCfg->RXrdIdx  = 0;
	MyCfg->RXwrtIdx = 0;
  #endif

  #if ((MY_UART_CIRC_BUF_TX) > 0)
	MyCfg->TXrdIdx  = 0;
	MyCfg->TXwrtIdx = MY_UART_CIRC_BUF_TX;
  #endif

	if (RXsize < 0) {								/* Request to use a semaphore for RX			*/
		if (MyCfg->SemRX == (SEM_t *)NULL) {
			MyCfg->SemRX = SEMopen(&g_Names[Dev][0][0]);
		}
	}
	if (TXsize < 0) {								/* Request to use a semaphore for TX			*/
		if (MyCfg->SemTX == (SEM_t *)NULL) {
			MyCfg->SemTX = SEMopen(&g_Names[Dev][1][0]);
		}
	}

	MyCfg->RXsize  = RXsize;						/* Memo sizes to know if using mailbox or not	*/
	MyCfg->TXsize  = TXsize;						/* The size of the mailboxes is not use per se	*/
  #if ((UART_MAILBOX_RX) != 0)
	MyCfg->PendINT = -1;							/* No pending char to deal with in the ISR		*/
  #endif
	MyCfg->Filter  = (unsigned int)Filter;			/* Memo the In/Out filtering operations			*/
	MyCfg->PendRX  = -1;							/* No In/Out pending characters					*/
	MyCfg->PendTX  = -1;

	MyCfg->HW = (volatile UartReg_t *) g_HWaddr[Dev];

/* ------------------------------------------------	*/
/* Beginning of target specific code				*/

{
int Divisor;										/* UART baud rate divisor value					*/

	MyCfg->HW[UART_IER_DLH_REG] &= ~0xF;			/* Disable all UART interrupts					*/
	MyCfg->HW[UART_FCR_REG]      = (0x01<<0)		/* Enable both FIFOs							*/
	                             | (0x03<<1)		/* Reset both FIFOs (self clearing)				*/
	                             | ((TXsize > 0)
	                               ? (0x02<<4)		/* TX FIFO 1/4 full trigger level (mailbox)		*/
	                               : (0x03<<4))		/* TX FIFO 1/2 full trigger level (semaphore)	*/
	                             | (0x00<<6);		/* RX FIFO 1 char trigger level					*/
	MyCfg->HW[UART_MCR_REG]     |= 0x03;			/* Turn ON DTR & RTS							*/
	MyCfg->HW[UART_LCR_REG]      = (0x3 & ((Nbits-5)))
	                             | ((Stop<=10)  ? 0x00 : 0x04)
	                             | ((Parity==0) ? 0x00 : (Parity<0) ? 0x08 : 0x18)
	                             | (0x80);			/* Enable divisor latch bit						*/

	Divisor = (((MY_UART_CLK) + (8 * Baud))			/* +(8*Baud) is for rounding					*/
	        / (16 * Baud));

	MyCfg->HW[UART_RBR_THR_DLL_REG] = (uint32_t)(0xFF & (Divisor));
	MyCfg->HW[UART_IER_DLH_REG]     = (uint32_t)(0xFF & (Divisor>>8));
	MyCfg->HW[UART_LCR_REG]        &= ~0x80;		/* Disable divisor latch bit					*/
}

/* ------------------------------------------------	*/
/* End of target specific code						*/

	MyCfg->IsInit = 1;								/* Is now initialized							*/

  #if ((MY_UART_SINGLE_MUTEX) >= 0)
	UART_MTX_UNLOCK(MyCfg);
  #endif

	if (RXsize != 0) {								/* Semaphore is in use							*/
		UART_GET_R_EXCL(MyCfg, ReMap);				/* Re-use ReMap for that						*/
		UART_RX_ENABLE(MyCfg);						/* Enable the RX interrupts for mailbox only	*/
		UART_RLS_R_EXCL(MyCfg, ReMap);				/* TX will be enable when using uart_send()		*/
	}

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* Interrupt handlers for the UART transfers														*/
/*																									*/
/* A common function is used for all the device, the device to handle is specified by the argument	*/
/* Quite straightforward processing, refer to the side comments										*/
/* The interrupt ID field is  not used as simply checking the RS & TX FIFO and put mailbox			*/
/* states is simpler and more efficient as it can deal with both interrupts raised at the same time	*/
/* The interrupt handler is split into RX & TX individual functions as the driver can also call		*/
/* a single direction handler. When the driver call one direction, the interrupts are disable in	*/
/* that direction, so that's why the state of the ISR (enable/disable) is checked when the handler	*/
/* is called from an interrupt to not go into conflict with the driver call							*/
/* ------------------------------------------------------------------------------------------------ */

  #define  UART_INT_HNDL(Prefix, Device) _UART_INT_HNDL(Prefix, Device)
  #define _UART_INT_HNDL(Prefix, Device)															\
	void Prefix##UARTintHndl_##Device(void)															\
	{																								\
	  int ii;																						\
	  int ReMap;																					\
	  UARTcfg_t *MyCfg;																				\
		ReMap =  g_DevReMap[Device];																\
		MyCfg = &g_UARTcfg[ReMap];																	\
		UART_GET_EXCL(MyCfg, ii);																	\
		UARTintRX(MyCfg, 0);						/* Driver will not call the handler directly*/	\
		UARTintTX(MyCfg, 0);						/* So it's safe to call both directions		*/	\
		UART_RLS_EXCL(MyCfg, ii);																	\
		return;																						\
	}

/* ----------------------------------------------------------------------------------------------- */ 

static void UARTintRX(UARTcfg_t *IntCfg, int IsBG)
{
#if ((MY_UART_CIRC_BUF_RX) > 0)
char Data;											/* Interim needed due to volatile warning		*/
int  NextWrt;										/* Next write index								*/
int  RdOri;											/* Read index unpon entry						*/
int  ToPost;										/* If has to post the semaphore					*/
int  WrtOri;										/* Write index upon entry						*/

	RdOri   = IntCfg->RXrdIdx;
	WrtOri  = IntCfg->RXwrtIdx;

	NextWrt = WrtOri-1;
	if (NextWrt < 0) {
		NextWrt = MY_UART_CIRC_BUF_RX;
	}

	ToPost = 0;										/* Assume no semaphore posting					*/
	while ((IntCfg->RXrdIdx != NextWrt)				/* Make sure to not overflow circular buffer	*/
	&&     (!UART_RX_EMPTY(IntCfg))) {				/* Grab the RX data as long as FIFO not empty	*/
		UART_DATA_RX(IntCfg, Data);
		IntCfg->CBufRX[IntCfg->RXwrtIdx] = Data;
		IntCfg->RXwrtIdx = NextWrt;					/* Keep updating for BG to be in-sync			*/
		if (--NextWrt < 0) {						/* Because on multi-core another core could be	*/
			NextWrt = MY_UART_CIRC_BUF_RX;			/* emptying the circular buffer					*/
		}
		ToPost = 1;									/* One char put in buffer, may have to post		*/
	}

	if ((WrtOri == RdOri)							/* If the circ buffer was empty upon entry, the	*/
	&&  (ToPost != 0)								/* background may be waitng for semaphore now	*/
	&&  (IsBG   == 0)
	&&  (IntCfg->RXsize != 0)) {					/* When RXsize == 0, semaphore not used			*/
		SEMpost(IntCfg->SemRX);
	}

	if (IntCfg->RXrdIdx == NextWrt) {				/* The circular buffer is full, disable the		*/
		UART_RX_DISABLE(IntCfg);					/* ISRs, they will be re-enable in uart_recv()	*/
	}

#else
 #if ((UART_MAILBOX_RX) != 0)						/* Mailboxes are available						*/
 intptr_t Data;										/* To read a mailbox with the proper data type	*/
 int      RetVal;									/* When sending to mailbox, result				*/
													/* Make sure we are using the RX mailboxes		*/
	if (IntCfg->RXsize > 0) {						/* First make sure there is room in the mailbox	*/
		RetVal = (IntCfg->MboxRX->Size <= IntCfg->MboxRX->Count);
		if (IntCfg->PendINT >= 0) {					/* Check if previous data read but not put in	*/
			Data   = (intptr_t)IntCfg->PendINT;		/* the mailbox (this is a safety mechanism)		*/
			RetVal = MBXput(IntCfg->MboxRX, Data, 0);	/* Try to put it in the mailbox				*/
			if (RetVal == 0) {						/* If was able to put in mailbox				*/
				IntCfg->PendINT = -1;				/* No more pending on hold						*/
			}
		}
		if (RetVal == 0) {							/* Pending mailbox write may have failed (0:OK)	*/
			while (! UART_RX_EMPTY(IntCfg)) {		/* Process as long as data in RX FIFO			*/
				UART_DATA_RX(IntCfg, Data);
				RetVal = MBXput(IntCfg->MboxRX, Data, 0);	/* Put in the mailbox. If could			*/
				if (RetVal != 0) {					/* not do it, keep char for next time			*/
					IntCfg->PendINT = (int)Data;	
					break;							/* If mailbox is full, we are done				*/
				}
			}
		}
		if (RetVal != 0) {							/* The mailbox is full, can't get anymore ISRs	*/
			UART_RX_DISABLE(IntCfg);				/* So disable the RX interrupts otherwise will	*/
		}											/* will be constantly interrupted				*/
	}												/* They will be re-enable in uart_recv()		*/
	else
#endif

	if (!UART_RX_EMPTY(IntCfg)) {					/* If data in RX FIFO, report to upper level	*/
		if ((IsBG == 0)
		&&  (IntCfg->RXsize < 0)) {					/* Using semaphore for RX direction				*/
			SEMpost(IntCfg->SemRX);
		}
		if (IntCfg->RXsize <= 0) {					/* When semaphore, disable RX interrupts		*/
			UART_RX_DISABLE(IntCfg);				/* Otherwise will be continuously interrupted	*/
		}											/* Also done for polling as polling should		*/
	}												/* never enable the interrupts					*/
#endif

	UART_CLR_RX_INT(IntCfg);						/* Clear the raised interrupts					*/

	return;											/* uart_recv will re-enable when FIFO is empty	*/
}

/* ------------------------------------------------------------------------------------------------ */

static void UARTintTX(UARTcfg_t *IntCfg, int IsBG)
{
#if ((MY_UART_CIRC_BUF_TX) > 0)
int RdNow;											/* Index where the next character to send is	*/
int RdOri;											/* Read index upon entry						*/
int ToPost;											/* If has to post the semaphore					*/
int WrtOri;											/* TXwrtIdx upon entry							*/

	RdNow   = IntCfg->TXrdIdx;
	RdOri   = RdNow;
	WrtOri  = IntCfg->TXwrtIdx;
	if (--RdNow < 0) {
		RdNow = MY_UART_CIRC_BUF_TX;
	}

	ToPost = 0;
	while ((RdNow != WrtOri)						/* Send as long as data in the circular buffer	*/
	&&     (!UART_TX_FULL(IntCfg))){				/* and room in the UART TX FIFO					*/
		UART_DATA_TX(IntCfg, IntCfg->CBufTX[RdNow]);
		IntCfg->TXrdIdx = RdNow;					/* Keep updating to keep BG in-sync				*/
		if (--RdNow < 0) {							/* Because on multi-core another core could be	*/
			RdNow = MY_UART_CIRC_BUF_TX;			/* filling the circular buffer					*/
		}
		ToPost = 1;									/* One char took from buffer, may have to post	*/
	}

	if ((WrtOri == RdOri)							/* The circular buffer was full, post the		*/
	&&  (ToPost != 0)								/* semaphore only if room now in circ buffer	*/
	&&  (IsBG   == 0)
	&&  (IntCfg->TXsize != 0)) {					/* When TXsize == 0, semaphore not used			*/
		SEMpost(IntCfg->SemTX);		
	}

	if (RdNow == WrtOri) {							/* If circular buffer is empty, done with the	*/
		UART_TX_DISABLE(IntCfg);					/* ISRs, they will be re-enable in uart_send()	*/
	}

#else
 #if ((UART_MAILBOX_RX) != 0)						/* Mailboxes available							*/
intptr_t Data;										/* To read a mailbox with the proper data type	*/

	if (IntCfg->TXsize > 0) {						/* Make sure we are using the TX mailboxes		*/
		while (!UART_TX_FULL(IntCfg)) {				/* Process as long as room in the TX FIFO		*/
			if (0 == MBXget(IntCfg->MboxTX, &Data, 0)) {	/* Got something, send to UART			*/
				UART_DATA_TX(IntCfg, Data);
			}
			else {									/* If mailbox is empty: done. Disable TX ISRs	*/
				UART_TX_DISABLE(IntCfg);			/* They will be re-enable in uart_send()		*/
				break;								/* Break out of the loop: nothing more to send	*/
			}
		}
	}
	else
 #endif

	if (!UART_TX_FULL(IntCfg)) {					/* If room in TX FIFO, report to upper level	*/
		if ((IsBG == 0)
		&&  (IntCfg->TXsize < 0)) {					/* Using semaphore for TX direction				*/
			SEMpost(IntCfg->SemTX);
		}
		if (IntCfg->TXsize <= 0) {					/* When semaphore, disable TX interrupts		*/
			UART_TX_DISABLE(IntCfg);				/* Otherwise will be continuously interrupted	*/
		}											/* Also done for polling as polling should		*/
	}												/* never enable the interrupts					*/
#endif

	UART_CLR_TX_INT(IntCfg);						/* Clear the raised interrupts					*/

	return;
}

/* ------------------------------------------------------------------------------------------------ */

#if (((MY_UART_LIST_DEVICE) & 0x001) != 0U)
  UART_INT_HNDL(MY_UART_PREFIX, 0)
#endif
#if (((MY_UART_LIST_DEVICE) & 0x002) != 0U)
  UART_INT_HNDL(MY_UART_PREFIX, 1)
#endif
#if (((MY_UART_LIST_DEVICE) & 0x004) != 0U)
  UART_INT_HNDL(MY_UART_PREFIX, 2)
#endif
#if (((MY_UART_LIST_DEVICE) & 0x008) != 0U)
  UART_INT_HNDL(MY_UART_PREFIX, 3)
#endif
#if (((MY_UART_LIST_DEVICE) & 0x010) != 0U)
  UART_INT_HNDL(MY_UART_PREFIX, 4)
#endif
#if (((MY_UART_LIST_DEVICE) & 0x020) != 0U)
  UART_INT_HNDL(MY_UART_PREFIX, 5)
#endif
#if (((MY_UART_LIST_DEVICE) & 0x040) != 0U)
  UART_INT_HNDL(MY_UART_PREFIX, 6)
#endif
#if (((MY_UART_LIST_DEVICE) & 0x080) != 0U)
  UART_INT_HNDL(MY_UART_PREFIX, 7)
#endif
#if (((MY_UART_LIST_DEVICE) & 0x100) != 0U)
  UART_INT_HNDL(MY_UART_PREFIX, 8)
#endif
#if (((MY_UART_LIST_DEVICE) & 0x200) != 0U)
  UART_INT_HNDL(MY_UART_PREFIX, 9)
#endif

#endif												/* ENDIF of MY_UART_LIST_DEVICE == 0			*/

/* EOF */
