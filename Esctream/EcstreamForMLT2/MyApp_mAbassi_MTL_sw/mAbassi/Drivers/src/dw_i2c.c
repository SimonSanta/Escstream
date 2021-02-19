/* ------------------------------------------------------------------------------------------------ */
/* FILE :		dw_i2c.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the Designware I2C peripheral.											*/
/*																									*/
/*																									*/
/* Copyright (c) 2014-2019, Code-Time Technologies Inc. All rights reserved.						*/
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
/*	$Revision: 1.42 $																				*/
/*	$Date: 2019/01/10 18:07:06 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* NOTES:																							*/
/*			ACP (when the DMA is used) is not supported becauae the I2C exchanges are quite slow	*/
/*			and adding ACP doesn't provide real transfer speed gain									*/
/*																									*/
/* LIMITATIONS:																						*/
/*																									*/
/* NOT YET SUPPORTED:																				*/
/*			Slave mode																				*/
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

#include "dw_i2c.h"

#ifndef I2C_N_TRIES									/* Number of time to retry when transfer error	*/
  #define I2C_N_TRIES			1					/* 0 is the minimum. When >0, it will retry		*/
#endif												/* I2C_N_TRIES times							*/

#ifndef I2C_USE_MUTEX								/* If a mutex is used to protect accesses		*/
  #define I2C_USE_MUTEX			1					/* == 0 : No mutex       != 0 : Use mutex		*/
#endif
													/* Bit #0->7 are for RX / Bit #8->15 are for TX	*/
#ifndef I2C_OPERATION								/* Bit #0/#8  : Disable ISRs on burst			*/
  #define I2C_OPERATION			0x00101				/* Bit #1/#9  : Interrupts are used				*/
#endif												/* Bit #2/#10 : DMA is used						*/
													/* Bit #16    : manual bus clear enable			*/

#ifndef I2C_ISR_RX_THRS								/* When using RX ISRs, percentage (0->100) of	*/
  #define I2C_ISR_RX_THRS		50					/* the RX FIFO when the ISR is triggered		*/
#endif

#ifndef I2C_ISR_TX_THRS								/* When using TX ISRs, percentage (0->100) of	*/
  #define I2C_ISR_TX_THRS		50					/* the TX FIFO when the ISR is triggered		*/
#endif

#ifndef I2C_MIN_4_RX_DMA
  #define I2C_MIN_4_RX_DMA		4					/* Minimum number of Bytes to use the RX DMA	*/
#endif

#ifndef I2C_MIN_4_TX_DMA
  #define I2C_MIN_4_TX_DMA		4					/* Minimum number of Bytes to use the TX DMA	*/
#endif

#ifndef I2C_MIN_4_RX_ISR
  #define I2C_MIN_4_RX_ISR		2					/* Minimum number of Bytes to use the RX ISR	*/
#endif

#ifndef I2C_MIN_4_TX_ISR
  #define I2C_MIN_4_TX_ISR		2					/* Minimum number of Bytes to use the TX ISR	*/
#endif

#ifndef I2C_MULTICORE_ISR							/* When operating on a multicore, set to != 0	*/
  #define I2C_MULTICORE_ISR		1					/* if more than 1 core process the interrupt	*/
#endif												/* Enable by default as it's safer				*/

#ifndef I2C_TOUT_ISR_ENB							/* Set to 0 if timeouts not required in polling	*/
  #define I2C_TOUT_ISR_ENB		1					/* When timeout used, if ISRs requested to be	*/
#endif												/* disable, enable-disable to update/read tick	*/

#ifndef I2C_REMAP_LOG_ADDR
  #define I2C_REMAP_LOG_ADDR	1					/* If remapping logical to physical adresses	*/
#endif												/* Only used/needed with DMA transfers			*/

#ifndef I2C_ARG_CHECK								/* If checking validity of function arguments	*/
  #define I2C_ARG_CHECK			1
#endif

#ifndef I2C_DEBUG									/* == 0 no debug information is printed			*/
  #define I2C_DEBUG				0					/* != 0 print init information and errors		*/
#endif												/* >= 2 print all information					*/

/* ------------------------------------------------------------------------------------------------ */
/* These are internal defintions that enable the use of multiple devices type I2C drivers			*/
/* Doing this keep the whole core code of the I2C the same across all types of I2C					*/
/* the I2C generic I2C_??? and/or specific ???_I2C_ tokens are remapped to MY_I2C_??? tokens		*/

#ifdef DW_I2C_N_TRIES
  #define MY_I2C_N_TRIES		(DW_I2C_N_TRIES)
#else
  #define MY_I2C_N_TRIES		(I2C_N_TRIES)
#endif

#ifdef DW_I2C_USE_MUTEX
  #define MY_I2C_USE_MUTEX		(DW_I2C_USE_MUTEX)
#else
  #define MY_I2C_USE_MUTEX		(I2C_USE_MUTEX)
#endif

#ifdef DW_I2C_OPERATION
  #define MY_I2C_OPERATION		(DW_I2C_OPERATION)
#else
  #define MY_I2C_OPERATION		(I2C_OPERATION)
#endif

#ifdef DW_I2C_ISR_RX_THRS
  #define MY_I2C_ISR_RX_THRS	(DW_I2C_ISR_RX_THRS)
#else
  #define MY_I2C_ISR_RX_THRS	(I2C_ISR_RX_THRS)
#endif

#ifdef DW_I2C_ISR_TX_THRS
  #define MY_I2C_ISR_TX_THRS	(DW_I2C_ISR_TX_THRS)
#else
  #define MY_I2C_ISR_TX_THRS	(I2C_ISR_TX_THRS)
#endif

#ifdef DW_I2C_MIN_4_RX_DMA
  #define MY_I2C_MIN_4_RX_DMA	(DW_I2C_MIN_4_RX_DMA)
#else
  #define MY_I2C_MIN_4_RX_DMA	(I2C_MIN_4_RX_DMA)
#endif

#ifdef DW_I2C_MIN_4_TX_DMA
  #define MY_I2C_MIN_4_TX_DMA	(DW_I2C_MIN_4_TX_DMA)
#else
  #define MY_I2C_MIN_4_TX_DMA	(I2C_MIN_4_TX_DMA)
#endif

#ifdef DW_I2C_MIN_4_RX_ISR
  #define MY_I2C_MIN_4_RX_ISR	(DW_I2C_MIN_4_RX_ISR)
#else
  #define MY_I2C_MIN_4_RX_ISR	(I2C_MIN_4_RX_ISR)
#endif

#ifdef DW_I2C_MIN_4_TX_ISR
  #define MY_I2C_MIN_4_TX_ISR	(DW_I2C_MIN_4_TX_ISR)
#else
  #define MY_I2C_MIN_4_TX_ISR	(I2C_MIN_4_TX_ISR)
#endif

#if ((OX_N_CORE) > 1)
 #ifdef DW_I2C_MULTICORE_ISR
  #define MY_I2C_MULTICORE_ISR	(DW_I2C_MULTICORE_ISR)
 #else
  #define MY_I2C_MULTICORE_ISR	(I2C_MULTICORE_ISR)
 #endif
#else
  #define MY_I2C_MULTICORE_ISR	0
#endif

#ifdef DW_I2C_TOUT_ISR_ENB
  #define MY_I2C_TOUT_ISR_ENB	(DW_I2C_TOUT_ISR_ENB)
#else
  #define MY_I2C_TOUT_ISR_ENB	(I2C_TOUT_ISR_ENB)
#endif

#ifdef DW_I2C_REMAP_LOG_ADDR
  #define MY_I2C_REMAP_LOG_ADDR	(DW_I2C_REMAP_LOG_ADDR)
#else
  #define MY_I2C_REMAP_LOG_ADDR	(I2C_REMAP_LOG_ADDR)
#endif

#ifdef DW_I2C_ARG_CHECK
  #define MY_I2C_ARG_CHECK		(DW_I2C_ARG_CHECK)
#else
  #define MY_I2C_ARG_CHECK		(I2C_ARG_CHECK)
#endif

#ifdef DW_I2C_DEBUG
  #define MY_I2C_DEBUG			(DW_I2C_DEBUG)
#else
  #define MY_I2C_DEBUG			(I2C_DEBUG)
#endif

#ifdef DW_I2C_CLK
  #define MY_I2C_CLK			(DW_I2C_CLK)
#elif defined(I2C_CLK)
  #define MY_I2C_CLK			(I2C_CLK)
#endif

#define MY_I2C_MAX_DEVICES		(DW_I2C_MAX_DEVICES)
#define MY_I2C_LIST_DEVICE		(DW_I2C_LIST_DEVICE)

#define MY_I2C_PREFIX			DW_I2C_PREFIX
#define MY_I2C_ASCII			DW_I2C_ASCII

#undef  I2C_ADD_PREFIX
#define I2C_ADD_PREFIX			DW_I2C_ADD_PREFIX

/* ------------------------------------------------------------------------------------------------ */
/* Base addresses of the HW I2C modules and other platform specific information						*/

#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)		/* Arria V / Cyclone V							*/
  #include "alt_gpio.h"

  static volatile uint32_t * const g_HWaddr[4] = { (volatile uint32_t *)0xFFC04000,
                                                   (volatile uint32_t *)0xFFC05000,
                                                   (volatile uint32_t *)0xFFC06000,
                                                   (volatile uint32_t *)0xFFC07000
                                                 };

  #define I2C_RESET_DEV(Dev)	do {																\
									*((volatile uint32_t *)0xFFD05014) |=  (1<<(12+(Dev)));			\
									*((volatile uint32_t *)0xFFD05014) &= ~(1<<(12+(Dev)));			\
								} while(0);

 #if (((MY_I2C_OPERATION) & 0x00404) != 0)
  #include "arm_pl330.h"

  #define DMA_DEV				0					/* Only one DMA on Arria V & Cyclone V			*/
  static int g_DMAchan[][2] = {{ 8,  9},			/* I2C #0 DMA: TX chan, RX chan					*/
                               {10, 11},			/* I2C #1 DMA: TX chan, RX chan					*/
                               {12, 13},			/* I2C #2 DMA: TX chan, RX chan					*/
                               {14, 15}				/* I2C #3 DMA: TX chan, RX chan					*/
                              };
 #endif

  #define MY_I2C_RX_FIFO_SIZE	(64)				/* Size of RX FIFO in bytes						*/
  #define MY_I2C_TX_FIFO_SIZE	(64)				/* Size of TX FIFO in bytes						*/

 #ifndef MY_I2C_CLK
  #define MY_I2C_CLK			100000000
 #endif

  #define UDELAY_MULT			(1000/6)			/* Cached: Multiplier for loop counter for 1 us	*/

  #if ((MY_I2C_MAX_DEVICES) > 4)
    #error "Too many I2C devices: set I2C_MAX_DEVICES <= 4 and/or I2C_LIST_DEVICE <= 0xF"
  #endif

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)	/* Arria 10										*/
  #include "alt_gpio.h"

  static volatile uint32_t * const g_HWaddr[5] = { (volatile uint32_t *)0xFFC02200,
                                                   (volatile uint32_t *)0xFFC02300,
                                                   (volatile uint32_t *)0xFFC02400,
                                                   (volatile uint32_t *)0xFFC02500,
                                                   (volatile uint32_t *)0xFFC02600
                                                 };
  #define I2C_RESET_DEV(Dev)	do {																\
									*((volatile uint32_t *)0xFFD05028) |=  (1<<(8+(Dev)));			\
									*((volatile uint32_t *)0xFFD05028) &= ~(1<<(8+(Dev)));			\
								} while(0);

 #if (((MY_I2C_OPERATION) & 0x00404) != 0)
  #include "arm_pl330.h"

  #define DMA_DEV				0					/* Only one DMA on Arria 10						*/
  static int g_DMAchan[][2] = {{ 8,  9},			/* I2C #0 DMA: TX chan, RX chan					*/
                               {10, 11},			/* I2C #1 DMA: TX chan, RX chan					*/
                               {12, 13},			/* I2C #2 DMA: TX chan, RX chan					*/
                               {14, 15},			/* I2C #3 DMA: TX chan, RX chan					*/
                               { 6,  7}				/* I2C #4 DMA: TX chan, RX chan					*/
                              };
 #endif

  #define MY_I2C_RX_FIFO_SIZE	(64)				/* Size of RX FIFO in bytes						*/
  #define MY_I2C_TX_FIFO_SIZE	(64)				/* Size of TX FIFO in bytes						*/

 #ifndef MY_I2C_CLK
  #define MY_I2C_CLK			100000000
 #endif

  #define UDELAY_MULT			(500/6)				/* Cached: Multiplier for loop counter for 1 us	*/

  #if ((MY_I2C_MAX_DEVICES) > 5)
    #error "Too many I2C devices: set I2C_MAX_DEVICES <= 5 and/or I2C_LIST_DEVICE <= 0x1F"
  #endif

#else
  	#error "Unsupported platform specified by OS_PLATFORM"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Pre-compute the FIFO thresholds																	*/

													/* RX, 0 is 1 and MY_I2C_RX_FIFO_SIZE-1 is 100%	*/
#define MY_I2C_RX_WATERMARK		((((MY_I2C_RX_FIFO_SIZE)-1) * MY_I2C_ISR_RX_THRS)/100)
													/* RX, 0 is 0 and MY_I2C_RX_FIFO_SIZE-1 is 100%	*/
#define MY_I2C_TX_WATERMARK		(((MY_I2C_TX_FIFO_SIZE) * MY_I2C_ISR_TX_THRS)/100)

/* ------------------------------------------------------------------------------------------------ */
/* This section uses a lots of macros but it's to reduce the size of the data requirements by only	*/
/* creating descriptor / mutexes etc for the devices in use ("1" in I2C_LIST_DEVICE)				*/

#ifndef MY_I2C_MAX_DEVICES
 #if   (((MY_I2C_LIST_DEVICE) & 0x200) != 0U)
  #define MY_I2C_MAX_DEVICES	10
 #elif (((MY_I2C_LIST_DEVICE) & 0x100) != 0U)
  #define MY_I2C_MAX_DEVICES	9
 #elif (((MY_I2C_LIST_DEVICE) & 0x080) != 0U)
  #define MY_I2C_MAX_DEVICES	8
 #elif (((MY_I2C_LIST_DEVICE) & 0x040) != 0U)
  #define MY_I2C_MAX_DEVICES	7
 #elif (((MY_I2C_LIST_DEVICE) & 0x020) != 0U)
  #define MY_I2C_MAX_DEVICES	6
 #elif (((MY_I2C_LIST_DEVICE) & 0x010) != 0U)
  #define MY_I2C_MAX_DEVICES	5
 #elif (((MY_I2C_LIST_DEVICE) & 0x008) != 0U)
  #define MY_I2C_MAX_DEVICES	4
 #elif (((MY_I2C_LIST_DEVICE) & 0x004) != 0U)
  #define MY_I2C_MAX_DEVICES	3
 #elif (((MY_I2C_LIST_DEVICE) & 0x002) != 0U)
  #define MY_I2C_MAX_DEVICES	2
 #elif (((MY_I2C_LIST_DEVICE) & 0x001) != 0U)
  #define MY_I2C_MAX_DEVICES	1
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* This section figures out how to remap the individual devices specified by I2C_LIST_DEVICE		*/
/* e.g. if I2C_LIST_DEVICE == 0x12, the only descriptors/mutexes for 2 devices are required and		*/
/*      device#1 is remapped to [0] and device #4 is remapped to [1]								*/

#if (((MY_I2C_LIST_DEVICE) & 0x001) != 0)
  #define I2C_CNT_0	0
  #define I2C_IDX_0	0
#else
  #define I2C_CNT_0	(-1)
  #define I2C_IDX_0	(-1)
#endif
#if (((MY_I2C_LIST_DEVICE) & 0x002) != 0)
  #define I2C_CNT_1	((I2C_CNT_0) + 1)
  #define I2C_IDX_1	((I2C_CNT_0) + 1)
#else
  #define I2C_CNT_1	(I2C_CNT_0)
  #define I2C_IDX_1	-1
#endif
#if (((MY_I2C_LIST_DEVICE) & 0x004) != 0)
  #define I2C_CNT_2	((I2C_CNT_1) + 1)
  #define I2C_IDX_2	((I2C_CNT_1) + 1)
#else
  #define I2C_CNT_2	(I2C_CNT_1)
  #define I2C_IDX_2	-1
#endif
#if (((MY_I2C_LIST_DEVICE) & 0x008) != 0)
  #define I2C_CNT_3	((I2C_CNT_2) + 1)
  #define I2C_IDX_3	((I2C_CNT_2) + 1)
#else
  #define I2C_CNT_3	(I2C_CNT_2)
  #define I2C_IDX_3	-1
#endif
#if (((MY_I2C_LIST_DEVICE) & 0x010) != 0)
  #define I2C_CNT_4	((I2C_CNT_3) + 1)
  #define I2C_IDX_4	((I2C_CNT_3) + 1)
#else
  #define I2C_CNT_4	(I2C_CNT_3)
  #define I2C_IDX_4	-1
#endif
#if (((MY_I2C_LIST_DEVICE) & 0x020) != 0)
  #define I2C_CNT_5	((I2C_CNT_4) + 1)
  #define I2C_IDX_5	((I2C_CNT_4) + 1)
#else
  #define I2C_CNT_5	(I2C_CNT_4)
  #define I2C_IDX_5	-1
#endif
#if (((MY_I2C_LIST_DEVICE) & 0x040) != 0)
  #define I2C_CNT_6	((I2C_CNT_5) + 1)
  #define I2C_IDX_6	((I2C_CNT_5) + 1)
#else
  #define I2C_CNT_6	(I2C_CNT_5)
  #define I2C_IDX_6	-1
#endif
#if (((MY_I2C_LIST_DEVICE) & 0x080) != 0)
  #define I2C_CNT_7	((I2C_CNT_6) + 1)
  #define I2C_IDX_7	((I2C_CNT_6) + 1)
#else
  #define I2C_CNT_7	(I2C_CNT_6)
  #define I2C_IDX_7	-1
#endif
#if (((MY_I2C_LIST_DEVICE) & 0x100) != 0)
  #define I2C_CNT_8	((I2C_CNT_7) + 1)
  #define I2C_IDX_8	((I2C_CNT_7) + 1)
#else
  #define I2C_CNT_8	(I2C_CNT_7)
  #define I2C_IDX_8	-1
#endif
#if (((MY_I2C_LIST_DEVICE) & 0x200) != 0)
  #define I2C_CNT_9	((I2C_CNT_8) + 1)
  #define I2C_IDX_9	((I2C_CNT_8) + 1)
#else
  #define I2C_CNT_9	(I2C_CNT_8)
  #define I2C_IDX_9	-1
#endif

#define I2C_NMB_DEVICES	((I2C_CNT_9)+1)

/* ------------------------------------------------------------------------------------------------ */
/* Remapping table:																					*/

static const int g_DevReMap[] = { I2C_IDX_0
                               #if ((MY_I2C_MAX_DEVICES) > 1)
                                 ,I2C_IDX_1
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 2)
                                 ,I2C_IDX_2
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 3)
                                 ,I2C_IDX_3
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 4)
                                 ,I2C_IDX_4
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 5)
                                 ,I2C_IDX_5
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 6)
                                 ,I2C_IDX_6
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 7)
                                 ,I2C_IDX_7
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 8)
                                 ,I2C_IDX_8
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 9)
                                 ,I2C_IDX_9
                               #endif
                               };

/* ------------------------------------------------------------------------------------------------ */

#if (((MY_I2C_USE_MUTEX) != 0) || (((MY_I2C_OPERATION) & 0x00202) != 0))

  static const char g_Names[][16] = {
                                 MY_I2C_ASCII "I2C-0"
                               #if ((MY_I2C_MAX_DEVICES) > 1)
                                ,MY_I2C_ASCII "I2C-1"
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 2)
                                ,MY_I2C_ASCII "I2C-2"
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 3)
                                ,MY_I2C_ASCII "I2C-3"
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 4)
                                ,MY_I2C_ASCII "I2C-4"
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 5)
                                ,MY_I2C_ASCII "I2C-5"
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 6)
                                ,MY_I2C_ASCII "I2C-6"
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 7)
                                ,MY_I2C_ASCII "I2C-7"
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 8)
                                ,MY_I2C_ASCII "I2C-8"
                               #endif 
                               #if ((MY_I2C_MAX_DEVICES) > 9)
                                ,MY_I2C_ASCII "I2C-9"
                               #endif
                               };
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Cyclone V / Arria V / Arria 10 specific register access / configuration and settings				*/
/* These are for the bus clear manual operation														*/

typedef struct {
	volatile uint32_t *Addr;					/* Mux register address								*/
	int GPIOn;									/* GPIO# on that mux (DTA), CLK is GPIO#+1			*/
	int ValGPIO;								/* Mux value for route GPIO							*/
	int ValI2C;									/* Mux value to route I2C signal					*/
} MuxI2C_t;

#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)		/* Cyclone V & Arraa 5							*/
static const MuxI2C_t g_MuxIO[4][3] = {{			/* Device #0									*/
                                       { (uint32_t *)0xFFD0849C, 55, 0, 1 },
                                       { (uint32_t *)0xFFD084BC, 63, 0, 3 },
                                       { (uint32_t *)0x00000000,  0, 0, 0 }
                                       },
                                       {			/* Device #1									*/
                                       { (uint32_t *)0xFFD0848C, 51, 0, 1 },
                                       { (uint32_t *)0xFFD084A4, 57, 0, 2 },
                                       { (uint32_t *)0xFFD084E4, 64, 0, 3 }
                                       },
                                       {			/* Device #2									*/
                                       { (uint32_t *)0xFFD08418,  6, 0, 1 },
                                       { (uint32_t *)0x00000000,  0, 0, 0 },
                                       { (uint32_t *)0x00000000,  0, 0, 0 }
                                       },
                                       {			/* Device #3									*/
                                       { (uint32_t *)0xFFD08518, 20, 0, 1 },
                                       { (uint32_t *)0x00000000,  0, 0, 0 },
                                       { (uint32_t *)0x00000000,  0, 0, 0 }
                                       }
                                     };

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)	/* Arria 10 									*/

static const MuxI2C_t g_MuxIO[5][4] = {{			/* Device #0									*/
                                       { (uint32_t *)0xFFD07010,  4, 15, 0 },
                                       { (uint32_t *)0xFFD07058, 22, 15, 0 },
                                       { (uint32_t *)0xFFD07068, 26, 15, 0 },
                                       { (uint32_t *)0x00000000,  0,  0, 0 }
                                       },
                                       {			/* Device #1									*/
                                       { (uint32_t *)0xFFD07008,  2, 15, 0 },
                                       { (uint32_t *)0xFFD07050, 20, 15, 0 },
                                       { (uint32_t *)0xFFD07078, 30, 15, 0 },
                                       { (uint32_t *)0xFFD07090, 36, 15, 0 }
                                       },
                                       {			/* Device #2									*/
                                       { (uint32_t *)0xFFD07028, 10, 15, 0 },
                                       { (uint32_t *)0xFFD07088, 34, 15, 0 },
                                       { (uint32_t *)0xFFD070B8, 46, 15, 0 },
                                       { (uint32_t *)0x00000000,  0,  0, 0 }
                                       },
                                       {			/* Device #3									*/
                                       { (uint32_t *)0xFFD07020,  8, 15, 0 },
                                       { (uint32_t *)0xFFD070A8, 42, 15, 0 },
                                       { (uint32_t *)0x00000000,  0,  0, 0 },
                                       { (uint32_t *)0x00000000,  0,  0, 0 }
                                       },
                                       {			/* Device #4									*/
                                       { (uint32_t *)0xFFD07018,  6, 15, 0 },
                                       { (uint32_t *)0xFFD07080, 32, 15, 0 },
                                       { (uint32_t *)0xFFD070B0, 44, 15, 0 },
                                       { (uint32_t *)0x00000000,  0,  0, 0 }
                                       }
                                     };

#endif

#define PIN_DTA(Cfg)		((Cfg)->MyMux->GPIOn)
#define PIN_CLK(Cfg)		((Cfg)->MyMux->GPIOn+1)

#define SEL_IO_DTA(Cfg)		do{ *((Cfg)->MyMux->Addr)   = (Cfg)->MyMux->ValGPIO; }while(0)
#define SEL_IO_CLK(Cfg)		do{ *((Cfg)->MyMux->Addr+1) = (Cfg)->MyMux->ValGPIO; }while(0)
#define SEL_I2C_DTA(Cfg)	do{ *((Cfg)->MyMux->Addr)   = (Cfg)->MyMux->ValI2C;  }while(0)
#define SEL_I2C_CLK(Cfg)	do{ *((Cfg)->MyMux->Addr+1) = (Cfg)->MyMux->ValI2C;  }while(0)

/* ------------------------------------------------------------------------------------------------ */
/* GPIO access are done with the GPIO driver														*/

#define IO_OUT(Pin)		gpio_dir(Pin, 0)
#define IO_IN(Pin)		gpio_dir(Pin, 1)
#define IO_HI(Pin)		gpio_set(Pin, 1)
#define IO_LO(Pin)		gpio_set(Pin, 0)
#define IO_READ(Pin)	gpio_get(Pin)

/* ------------------------------------------------------------------------------------------------ */
/* Checks 																							*/

#if (((MY_I2C_OPERATION) & ~0x10707) != 0)
  #error "Invalid value assigned to I2C_OPERATION"
#endif

#if ((MY_I2C_N_TRIES) < 0)
  #error "I2C_N_TRIES must be >= 0"
#endif

#if (((MY_I2C_ISR_RX_THRS) < 0) || ((MY_I2C_ISR_RX_THRS) > 100))
  #error "I2C_ISR_RX_THRS must be between 0 and 100"
#endif

#if (((MY_I2C_ISR_TX_THRS) < 0) || ((MY_I2C_ISR_TX_THRS) > 100))
  #error "I2C_ISR_TX_THRS must be between 0 and 100"
#endif

#if ((MY_I2C_MIN_4_RX_DMA) < 0)
  #error "I2C_MIN_4_RX_DMA must be >= 1"
#endif

#if ((MY_I2C_MIN_4_TX_DMA) < 0)
  #error "I2C_MIN_4_TX_DMA must be >= 1"
#endif

#if ((MY_I2C_MIN_4_RX_ISR) < 0)
  #error "I2C_MIN_4_RX_ISR must be >= 1"
#endif

#if ((MY_I2C_MIN_4_TX_ISR) < 0)
  #error "I2C_MIN_4_TX_ISR must be >= 1"
#endif

#if ((OX_TIMER_US) <= 0)
  #error "The I2C driver relies on timeouts."
  #error " You must enable the RTOS timer by setting OS_TIMER_US to > 0"
#endif
#if ((OX_TIMEOUT) <= 0)
  #error "The I2C driver relies on timeouts. You must set OS_TIMEOUT to > 0"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Some definitions																					*/

#if ((MY_I2C_USE_MUTEX) != 0)						/* Mutex for exclusive access					*/
  #define I2C_MTX_LOCK(Cfg)			MTXlock(Cfg->MyMutex, -1)
  #define I2C_MTX_UNLOCK(Cfg)		MTXunlock(Cfg->MyMutex)
#else
  #define I2C_MTX_LOCK(Cfg)	  		0
  #define I2C_MTX_UNLOCK(Cfg)		OX_DO_NOTHING()
#endif

#if ((((MY_I2C_OPERATION) & 0x00001) == 0) || ((MY_I2C_TOUT_ISR_ENB) == 0))
  #define I2C_EXPIRY_RX(_IsExp, _IsrState, _Expiry)		do {										\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
														} while(0)
#else
  #define I2C_EXPIRY_RX(_IsExp, _IsrState, _Expiry)		do {										\
															OSintBack(_IsrState);					\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
															_IsrState = OSintOff();					\
														} while(0)
#endif
#if ((((MY_I2C_OPERATION) & 0x00100) == 0) || ((MY_I2C_TOUT_ISR_ENB) == 0))
  #define I2C_EXPIRY_TX(_IsExp, _IsrState, _Expiry)		do {										\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
														} while(0)
#else
  #define I2C_EXPIRY_TX(_IsExp, _IsrState, _Expiry)	do {											\
															OSintBack(_IsrState);					\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
															_IsrState = OSintOff();					\
														} while(0)
#endif
#if ((((MY_I2C_OPERATION) & 0x00101) == 0) || ((MY_I2C_TOUT_ISR_ENB) == 0))
  #define I2C_EXPIRY_RXTX(_IsExp, _IsrState, _Expiry)	do {										\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
														} while(0)
#else
  #define I2C_EXPIRY_RXTX(_IsExp, _IsrState, _Expiry)	do {										\
															OSintBack(_IsrState);					\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
															_IsrState = OSintOff();					\
														} while(0)
#endif

/* ------------------------------------------------------------------------------------------------ */
/* I2C register index definitions (/4 as are accessed as uint32_t)									*/

#define I2C_CON_REG					(0x00/4)
#define I2C_TAR_REG					(0x04/4)
#define I2C_SAR_REG					(0x08/4)
#define I2C_DATA_CMD_REG			(0x10/4)
#define I2C_SS_SCL_HCNT_REG			(0x14/4)
#define I2C_SS_SCL_LCNT_REG			(0x18/4)
#define I2C_FS_SCL_HCNT_REG			(0x1C/4)
#define I2C_FS_SCL_LCNT_REG			(0x20/4)
#define I2C_INTR_STAT_REG			(0x2C/4)
#define I2C_INTR_MASK_REG			(0x30/4)
#define I2C_RAW_INTR_STAT_REG		(0x34/4)
#define I2C_RX_TL_REG				(0x38/4)
#define I2C_TX_TL_REG				(0x3C/4)
#define I2C_CLR_INTR_REG			(0x40/4)
#define I2C_CLR_RX_UNDER_REG		(0x44/4)
#define I2C_CLR_RX_OVER_REG			(0x48/4)
#define I2C_CLR_TX_OVER_REG			(0x4C/4)
#define I2C_CLR_RD_REQ_REG			(0x50/4)
#define I2C_CLR_TX_ABRT_REG			(0x54/4)
#define I2C_CLR_RX_DONE_REG			(0x58/4)
#define I2C_CLR_ACTIVITY_REG		(0x5C/4)
#define I2C_CLR_STOP_DET_REG		(0x60/4)
#define I2C_CLR_START_DET_REG		(0x64/4)
#define I2C_CLR_GEN_CALL_REG		(0x68/4)
#define I2C_ENABLE_REG				(0x6C/4)
#define I2C_STATUS_REG				(0x70/4)
#define I2C_TXFLR_REG				(0x74/4)
#define I2C_RXFLR_REG				(0x78/4)
#define I2C_SDA_HOLD_REG			(0x7C/4)
#define I2C_TX_ABRT_SOURCE_REG 		(0x80/4)
#define I2C_DMA_CR_REG				(0x88/4)
#define I2C_DMA_TDLR_REG			(0x8C/4)
#define I2C_DMA_RDLR_REG			(0x90/4)
#define I2C_ENABLE_STATUS_REG		(0x9C/4)
#define I2C_FS_SPKLEN_REG			(0xA0/4)
#define I2C_COMP_PARAM_1_REG		(0xF4/4)

/* ------------------------------------------------------------------------------------------------ */
/* Local variables																					*/

typedef struct {									/* Per device: configuration					*/
	volatile uint32_t *HW;							/* Base address of the I2C peripheral			*/
    int AddBits;									/* Number of address bits: 7 or 10				*/
	int BusFreq;									/* Bus frequency								*/
	int Dev;										/* Needed to perform bus recovery				*/
	int IsInit;										/* If the device has been initialized			*/
	int NsecBit;									/* Nano-second per bit transfered				*/
	const MuxI2C_t *MyMux;							/* GPIO/I2C Mux for test and bus clear			*/
  #if ((MY_I2C_USE_MUTEX) != 0)						/* If mutex protection enable					*/
	MTX_t *MyMutex;									/* One mutex per controller						*/
  #endif
  #if (((MY_I2C_OPERATION) & 0x00202) != 0)			/* If interrupts are used (Read and/or write)	*/
	SEM_t   *MySema;								/* Semaphore posted in the ISR					*/
	volatile char *ISRbufRX;						/* Current buffer address to write to			*/
	volatile char *ISRbufTX;						/* Current buffer address to read from			*/
	volatile int   ISRleftDum;						/* # of dummy byte to send left					*/
	volatile int   ISRleftRX;						/* # byte left in the transfer					*/
	volatile int   ISRleftTX;						/* # byte left in the transfer					*/
   #if ((MY_I2C_MULTICORE_ISR) != 0)
	volatile int   ISRonGoing;						/* Inform other cores if one handling the ISR	*/
	int            SpinLock;						/* Spinlock for exclusive access to ISRonGoing	*/
   #endif
  #endif
} I2Ccfg_t;

static I2Ccfg_t  g_I2Ccfg[I2C_NMB_DEVICES];			/* Device configuration							*/
static int       g_CfgIsInit = 0;					/* To track first time an init occurs			*/

#if (((MY_I2C_OPERATION) & 0x00400) != 0)			/* When PL330 reads burst of memory with no inc	*/
 static uint16_t g_DumVal[DMA_MAX_BURST];			/* it looks like the AMBA/AXI bus returns the	*/
#endif												/* consecutive memory data and not multiple		*/
													/* copies of the memory location to read		*/
static volatile unsigned int g_Dummy;				/* Usd for dummy reads and SW delay				*/

/* ------------------------------------------------------------------------------------------------ */
/* Local functions																					*/

static void i2c_bus_clear    (I2Ccfg_t *Cfg);
static void i2c_disable      (I2Ccfg_t *Cfg);
static void i2c_enable       (I2Ccfg_t *Cfg);
static void	i2c_set_bus_speed(I2Ccfg_t *Cfg, int MyFreq);
static int  i2c_wait_for_bb  (I2Ccfg_t *Cfg);
static void uDelay           (int Time);

/* ------------------------------------------------------------------------------------------------	*/
/* FUNCTION: i2c_recv																				*/
/*																									*/
/* i2c_recv - receive data from a slave I2C device													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int i2c_recv(int Dev, int Target, char *Buf, int Len);										*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev    : controller number																	*/
/*		Target : I2C address of the slave device to receive data from								*/
/*		Buf    : pointer to a buffer to hold the data received										*/
/*		Len    : number of bytes to receive from the slave device									*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int : == 0 success																			*/
/*		      != 0 error																			*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int I2C_ADD_PREFIX(i2c_recv) (int Dev, int Target, char *Buf, int Len)
{
char     *BufOri;									/* In case has to retry							*/
I2Ccfg_t *Cfg;										/* De-reference array for faster access			*/
uint32_t  Expiry;									/* With polling, exiry time						*/
int       FirstLen;									/* At start, # to fill the TX FIFO				*/
int       ii;										/* General purpose								*/
int       IsExp;									/* If it has expired							*/
int       LeftDum;									/* Number of dummy write left to do				*/
int       LeftRX;									/* Number of byte left to be received			*/
volatile uint32_t *Reg;								/* Base pointer to I2C controller registers		*/
int       ReMap;									/* Remapped "Dev" to index in g_I2Ccfg[]		*/
int       ReTry;									/* To limit the numbers of retries				*/
int       TimeOut;									/* Timeout in # timer ticks on reception		*/
#if (((MY_I2C_OPERATION) & 0x00001) != 0)
  int IsrState;
#endif
#if (((MY_I2C_OPERATION) & 0x00202) != 0)
  unsigned int UseISR;
#endif
#if (((MY_I2C_OPERATION) & 0x00404) != 0)
  uint32_t CfgOp[8];								/* DMA configuration commands					*/
  int      DMAidRX;									/* ID of the DMA transfer for RX				*/
  int      DMAidTX;									/* ID of the DMA transfer for TX				*/
 #if (((MY_I2C_OPERATION) & 0x00400) != 0)
  int32_t DmaLenTX;									/* Number of bytes to send using the DMA		*/
 #endif
#endif
#if ((MY_I2C_DEBUG) > 0)
  int DbgDump;
#endif
#if ((MY_I2C_DEBUG) > 1)
  unsigned int XferType;
#endif

  #if ((MY_I2C_DEBUG) > 1)
	printf("I2C   [Dev:%d - Targ:%d] - Receiving %d bytes\n", Dev, Target, Len);
	XferType = 0U;
  #endif

  #if ((MY_I2C_ARG_CHECK) == 0)						/* Arguments validity not checked				*/
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_I2Ccfg[ReMap];						/* This is this device configuration			*/

  #else												/* Arguments validity checked					*/
	if ((Dev < 0)									/* Check the validity of "Dev"					*/
	||  (Dev >= (MY_I2C_MAX_DEVICES))) {
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - Out of range device\n", Dev, Target);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by I2C_LIST_DEVICE				*/
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - Cannot remap device\n", Dev, Target);
	  #endif
		return(-2);
	}

	if (Buf == (char *)NULL) {
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - NULL buffer\n", Dev, Target);
	  #endif
		return(-3);
	}

	Cfg = &g_I2Ccfg[ReMap];							/* This is this device configuration			*/
	if ((g_CfgIsInit == 0)
	||  (Cfg->IsInit == 0)) {						/* The controller is not initialized			*/
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - Device not initialized\n", Dev, Target);
	  #endif
		return(-4);
	}

	if (Len < 0) {									/* -ve number of bytes to transfer				*/
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - Negative number of bytes to receive\n",
		       Dev, Target);
	  #endif
		return(-5);
	}
  #endif

	if (Len == 0) {									/* Receiving nothing is not considered an error	*/
		return(0);
	}

	BufOri  = Buf;
	Reg     = Cfg->HW;								/* Timeout: use *8 for margin of error			*/
	TimeOut = (9 * (Len+1) * Cfg->NsecBit)			/* Timeout waiting for the the transfer done	*/
	        >> 17;		         					/* Doing /(128*1024) instead of *8/(1000*1000)	*/

	TimeOut = OS_MS_TO_MIN_TICK(TimeOut, 3);		/* Convert ms to RTOS timer tick count			*/

	if (0 != I2C_MTX_LOCK(Cfg)) {					/* Now we need to have exclusive access			*/
		return(-6);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	ReTry = MY_I2C_N_TRIES;							/* Upon failure, try again this number of times	*/
	do {
		if (0 != i2c_wait_for_bb(Cfg)) {			/* Wait for a non-busy bus						*/
			I2C_MTX_UNLOCK(Cfg);
		  #if ((MY_I2C_DEBUG) > 0)
			printf("I2C   [Dev:%d - Targ:%d] - Error - Bus stalled in busy mode\n", Dev, Target);
		  #endif
			return(-6);								/* Did not reached a non-busy bus, error		*/
	    }

		i2c_disable(Cfg);
		Reg[I2C_TAR_REG] = Target;					/* Init the transfer sequence					*/
		i2c_enable(Cfg);

		Buf      = BufOri;							/* Set-up data transfer parameters				*/
		IsExp    = 0;								/* != 0 indicates a timeout						*/
		LeftDum  = Len;								/* Number of dummy write left to do				*/
		LeftRX   = Len;								/* Number of data byte left to read				*/
		FirstLen = Len;								/* First thing done is filling the TX FIFO		*/
		if (FirstLen > (MY_I2C_TX_FIFO_SIZE)) {
			FirstLen = MY_I2C_TX_FIFO_SIZE;
		}

		Reg[I2C_RX_TL_REG]  = MY_I2C_RX_WATERMARK;
		Reg[I2C_TX_TL_REG]  = MY_I2C_TX_WATERMARK;
		Reg[I2C_DMA_CR_REG] = 0;					/* No DMA (yet)									*/

	  #if ((MY_I2C_DEBUG) > 0)
		DbgDump = 0;
	  #endif

	  #if (((MY_I2C_OPERATION) & 0x00202) != 0)		/* ISRs can be used. Use both RX & TX OPERATION	*/
		UseISR = 0U;								/* because there is one TX for each RX			*/

	   #if (((MY_I2C_OPERATION) & 0x00002) != 0)
		if ((Len >= (MY_I2C_MIN_4_RX_ISR))			/* If enough data to RX to use ISRs				*/
		&&  (Len >  (MY_I2C_RX_WATERMARK))) {		/* And more than RX threshold					*/
			UseISR |= 1U;							/* Use ISR for RX and EOT						*/
		}
	   #endif

	   #if (((MY_I2C_OPERATION) & 0x00200) != 0)
		if ((Len >= (MY_I2C_MIN_4_TX_ISR))			/* If enough data to TX to use ISRs				*/
		&&  (Len >  (MY_I2C_TX_WATERMARK))			/* And more than TX threshold					*/
		&&  (Len >  FirstLen)) {					/* And with leftover after initial TX FIFO fill	*/
			UseISR |= 2U;							/* Use ISR for TX and EOT						*/
		}
	   #endif
	  #endif

	  #if (((MY_I2C_OPERATION) & 0x00404) != 0)		/* DMA can be used. Use both RX & TX OPERATION	*/
		DMAidRX = 0;								/* because there is one TX for each RX			*/
		DMAidTX = 0;								/* If DMA is not used, these will remain == 0	*/

	   #if (((MY_I2C_OPERATION) & 0x00004) != 0)	/* DMA RX can be used							*/
		if  (Len >= (MY_I2C_MIN_4_RX_DMA))	{		/* Enough data to use the DMA & > RX FIFO size	*/

		  #if ((MY_I2C_DEBUG) > 1)
			XferType |= 1U << 2;					/* Use the same bits as in I2C_OPERATION		*/
		  #endif

			ii  = (MY_I2C_RX_FIFO_SIZE)/2;			/* Figure out the largest useable burst length	*/
			if (ii > (DMA_MAX_BURST)) {				/* Maximizing the burst length maximises the	*/
				ii = DMA_MAX_BURST;					/* transfer rate								*/
			}
			if (ii > Len) {
				ii = Len;
			}
			Reg[I2C_DMA_RDLR_REG] = ii-1;			/* Watermark: trigger when #RX FIFO > watermark	*/
			Reg[I2C_DMA_CR_REG]  |= 0x1;			/* Enable DMA RX operations						*/

			CfgOp[0] = DMA_CFG_NOWAIT;				/* RX DMA started but won't wait here			*/
			CfgOp[1] = (((MY_I2C_OPERATION) & 0x00002) != 0)
			         ? DMA_CFG_EOT_ISR				/* When using dma_wait(), type of waiting		*/
			         : DMA_CFG_EOT_POLLING;
		  #if ((MY_I2C_REMAP_LOG_ADDR) == 0)
			CfgOp[2] = 0;
		  #else
			CfgOp[2] = DMA_CFG_LOGICAL_DST;
			CfgOp[3] = DMA_CFG_LOGICAL_SRC;
			CfgOp[4] = 0;
		  #endif

			IsExp = dma_xfer(DMA_DEV,
				             Buf, 1, -1,
			                 (void *)&Reg[I2C_DATA_CMD_REG], 0, g_DMAchan[Dev][1],
			                 1, Reg[I2C_DMA_RDLR_REG]+1, Len,
				             DMA_OPEND_NONE, NULL, 0,
				             &CfgOp[0], &DMAidRX, TimeOut);

			LeftRX = 0;								/* DMA reads all received bytes					*/

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {						/* Something went wrong setting up the DMA		*/
				printf("I2C   [Dev:%d - Targ:%d] - Error -  RX DMA reported error #%d\n",
				        Dev, Target, IsExp);
			}
		  #endif

		  #if (((MY_I2C_OPERATION) & 0x00002) != 0)
			UseISR &= ~1U;							/* ISRs are not needed for receiving			*/
		  #endif
		}
	  #endif

	   #if (((MY_I2C_OPERATION) & 0x00400) != 0)	/* DMA TX can be used							*/
		DmaLenTX = (Len-FirstLen)					/* Number of frames to send with the DMA		*/
		         - 1;								/* Manually send last for stop condition		*/
		if ((DmaLenTX > 0)
		&&  (IsExp == 0)) {

		  #if ((MY_I2C_DEBUG) > 1)
			XferType |=  1U << 10;					/* Use the same bits as in I2C_OPERATION		*/
		  #endif

			ii = (MY_I2C_TX_FIFO_SIZE)/2;			/* Try to keep the TX FIFO at 1/2 full			*/
			if (ii > (DMA_MAX_BURST)) {				/* Can't do burst larger than what the DMA does	*/
				ii = DMA_MAX_BURST;
			}
			if (ii > DmaLenTX) {					/* Can have a burst larger than what to send	*/
				ii = DmaLenTX;
			}

			Reg[I2C_DMA_TDLR_REG] = (MY_I2C_TX_FIFO_SIZE)	/* Optimal DMA watermark				*/
			                      - ii;				/* FIFO is all filled before TX DMA				*/
			Reg[I2C_DMA_CR_REG]  |= 0x2;			/* RX and TX DMA are used						*/

			CfgOp[0] = DMA_CFG_NOSTART;				/* Is manually started after TX FIFO filled		*/
			CfgOp[1] = (DMAidRX == 0)				/* When RX DMA is used, we can block on TX		*/
			         ? DMA_CFG_NOWAIT				/* until it's time to send the stop condition	*/
			         : DMA_CFG_NOP;					/* else, RX is done with polling or ISRs		*/
			CfgOp[2] = (((MY_I2C_OPERATION) & 0x00200) != 0)
			         ? DMA_CFG_EOT_ISR				/* If waiting, type of wait						*/
			         : DMA_CFG_EOT_POLLING;
			CfgOp[3] = DMA_CFG_NOCACHE_SRC;			/* g_DumVal is static and was flushed at init	*/
		  #if ((MY_I2C_REMAP_LOG_ADDR) == 0)
			CfgOp[4] = 0;
		  #else
			CfgOp[4] = DMA_CFG_LOGICAL_DST;
			CfgOp[5] = DMA_CFG_LOGICAL_SRC;
			CfgOp[6] = 0;
		  #endif

			IsExp = dma_xfer(DMA_DEV,
			                (void *)&Reg[I2C_DATA_CMD_REG], 0, g_DMAchan[Dev][0],
				             &g_DumVal[0], 0, -1,
			                 2, (MY_I2C_TX_FIFO_SIZE)-Reg[I2C_DMA_TDLR_REG], DmaLenTX,
				             DMA_OPEND_NONE, NULL, 0,
				             &CfgOp[0], &DMAidTX, TimeOut);

		  #if (((MY_I2C_OPERATION) & 0x00002) != 0)
			UseISR &= ~2U;							/* ISRs are not needed for TXing				*/
		  #endif

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {						/* Something went wrong setting up the DMA		*/
				printf("I2C   [Dev:%d - Targ:%d] - Error -  TX DMA reported error #%d\n",
				        Dev, Target, IsExp);
			}
		  #endif
		}
	   #endif

	  #endif


	  #if (((MY_I2C_OPERATION) & 0x00001) != 0)		/* Set-up to disable ISRs on read burst			*/
		IsrState = OSintOff();
	  #endif

		if (IsExp == 0) {							/* First thing is filling the TX FIFO			*/
			ii = 1U << 10;							/* Restart command								*/
			for ( ; FirstLen>0 ; FirstLen--) {		/* Fill as much as possible the TX FIFO			*/
				Reg[I2C_DATA_CMD_REG] = (1U << 8)	/* Data command read							*/
				                      | ((--LeftDum == 0) ? (1U << 9) : 0U)
				                      | ii;			/* 1<<9 is for the stop condition for last byte	*/
				ii = 0;								/* Not a restart anymore						*/
			}
		}

		Expiry = OS_TICK_EXP(TimeOut);				/* This is where the transfer is started		*/

	  #if (((MY_I2C_OPERATION) & 0x00400) != 0)		/* DMA TX can be used							*/
		if ((DMAidTX != 0)							/* DMA is used for TXing						*/
		&&  (IsExp == 0)
		&&  ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)) {

		  #if (((MY_I2C_OPERATION) & 0x00001) != 0)
			OSintBack(IsrState);
		  #endif

			IsExp = dma_start(DMAidTX);				/* Start it and in Polling/ISR send stop cond	*/

			LeftDum = 1;							/* Stop condition is the only one left to TX	*/
													/* Done in Polling below						*/
		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {						/* Only time could be wrong is timeout			*/
				printf("I2C   [Dev:%d - Targ:%d] - Error -  Timeout waiting for TX DMA\n",
				        Dev, Target);
				DbgDump = 1;
				IsExp   = 1;						/* Make sure i2c_recv reports a timeout			*/
			}
		  #endif
		}
	  #endif		


	  #if (((MY_I2C_OPERATION) & 0x00002) != 0)		/* Using ISRs									*/
		if ((UseISR != 0U)							/* Enough RX to receive to use ISRs?			*/
		&&  (IsExp  == 0)
		&&  ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)) {

			Cfg->ISRleftDum = 0;
			Cfg->ISRleftRX  = 0;
			Cfg->ISRleftTX  = 0;
			Cfg->ISRbufRX   = Buf;

			if ((UseISR & 2U) != 0U) {				/* Using TX ISRs								*/
				Cfg->ISRleftDum = LeftDum;
				LeftDum         = 0;
			  #if ((MY_I2C_DEBUG) > 1)
				XferType |=  1U << 1;				/* Use the same bits as in I2C_OPERATION		*/
			  #endif

			}

			if ((UseISR & 1U) != 0U) {				/* Using RX ISRs								*/
				Cfg->ISRleftRX = LeftRX;
				LeftRX         = 0;
			  #if ((MY_I2C_DEBUG) > 1)
				XferType |=  1U << 9;				/* Use the same bits as in I2C_OPERATION		*/
			  #endif

			}

			SEMreset(Cfg->MySema);					/* Make sure there are no pending posting		*/

			Reg[I2C_INTR_MASK_REG] = (((UseISR&4U)>>2) << 9)	/* Enable the stop condition int	*/
			                       | (((UseISR&2U)>>1) << 4)	/* Enable <= TX watermark int		*/
			                       | (((UseISR&1U)>>0) << 2);	/* Enable above RX watermark		*/

		  #if (((MY_I2C_OPERATION) & 0x00001) != 0)	/* Must be enable for the ISRs to transfer		*/
			OSintBack(IsrState);
		  #endif
		}
	  #endif

													/* Transfer through Polling						*/
		while (((LeftRX+LeftDum) != 0)				/* Read everything left to receive				*/
		&&     (IsExp == 0)							/* and/or send all left over dummies			*/
		&&     ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)) {

			I2C_EXPIRY_RX(IsExp, IsrState, Expiry);	/* Check if has timed out						*/

			while ((LeftRX != 0)					/* Non-zero number of bytes in RX FIFO			*/
			&&     (Reg[I2C_RXFLR_REG] != 0U)) {
				*Buf++ = (char)Reg[I2C_DATA_CMD_REG];
				LeftRX--;
			}

		  #if (((MY_I2C_OPERATION) & 0x00400) != 0)	/* DMA TX may be be used						*/
			if (DMAidTX != 0) {						/* If so, need to send the stop condition		*/
				if (LeftDum != 0) {					/* If dummy hasn't been sent					*/
					if ((dma_done(DMAidTX) != 0)	/* TX DMA done and room in FIFO, put stop cond	*/
					&&  (Reg[I2C_TXFLR_REG] <= ((MY_I2C_TX_FIFO_SIZE))/2)) {
						LeftDum = 0;				/* Use SIZE/2 to make sure DMA is really done	*/
						Reg[I2C_DATA_CMD_REG] = (1U << 8)	/* Data command read					*/
						                      | (1U << 9);	/* Stop on last byte					*/
					}
				}
			}
			else
		  #endif

			while ((LeftDum != 0)					/* If dummy left to write and room in the FIFO	*/
			&&     (Reg[I2C_TXFLR_REG] != (MY_I2C_TX_FIFO_SIZE))) {
				Reg[I2C_DATA_CMD_REG] = (1U << 8)	/* Data command read							*/
				                      | ((--LeftDum == 0) ? (1U << 9) : 0U);/* Stop on last byte	*/
			}

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {
				printf("I2C   [Dev:%d - Targ:%d] - Error -  Timeout sending after filling FIFO\n",
				        Dev, Target);
				DbgDump = 1;
			}
		  #endif
		}

	  #if (((MY_I2C_OPERATION) & 0x00001) != 0)		/* We're done with ISR possibly disable			*/
		OSintBack(IsrState);
	  #endif

	  #if (((MY_I2C_OPERATION) & 0x00002) != 0)		/* Using ISRs, wait for end of them				*/
		if ((UseISR != 0U)
		&&  (IsExp  == 0)
		&&  ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)) {

			IsExp = SEMwait(Cfg->MySema, TimeOut);

			Reg[I2C_INTR_MASK_REG] = 0;				/* Disable all I2C interrupts source			*/

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {
				printf("I2C   [Dev:%d - Targ:%d] - Error -  Timeout on semaphore\n", Dev, Target);
				DbgDump = 1;
			}
		  #endif
		}

	   #if ((MY_I2C_DEBUG) > 0)
	    #if (((MY_I2C_OPERATION) & 0x00002) != 0)
		if ((UseISR & 1U) != 0U) {					/* Recover these for error report				*/
			LeftRX = Cfg->ISRleftRX;
		}
	 	#endif
		#if (((MY_I2C_OPERATION) & 0x00200) != 0)
		if ((UseISR & 2U) != 0U) {
			LeftDum = Cfg->ISRleftDum;
		}
	    #endif
	   #endif

	  #endif

	  #if (((MY_I2C_OPERATION) & 0x00004) != 0)
		if ((DMAidRX != 0)							/* If the RX DMA is still going on, wait for it	*/
		&&  (IsExp == 0)							/* to be done									*/
		&&  ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)) {

			IsExp = dma_wait(DMAidRX);

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {
				printf("I2C   [Dev:%d - Targ:%d] - Error -  RX DMA timeout (2)\n", Dev, Target);
			}				
		  #endif
		}
	  #endif

		while ((IsExp == 0)							/* Wait for the stop condition					*/
		&&     ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)
		&&     ((Reg[I2C_RAW_INTR_STAT_REG] & (1U << 9)) == 0)) {	/* Stop condition is bit 9		*/

			IsExp = OS_HAS_TIMEDOUT(Expiry);		/* Check if has timed out						*/

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {
				printf("I2C   [Dev:%d - Targ:%d] - Error -  Timeout on stop condition\n",
				       Dev, Target);
			}
		  #endif
		}

	    g_Dummy = Reg[I2C_CLR_STOP_DET_REG];		/* To clear the stop condition: read register	*/

	  #if ((MY_I2C_DEBUG) > 1)						/* Print after the Xfer to not impact polling	*/
		if ((XferType & (1U<<10)) != 0U) {
			printf("I2C   [Dev:%d - Targ:%d] - TX transfer done with DMA\n", Dev, Target);
		}
		else if ((XferType & (1U<<9)) != 0U) {
			printf("I2C   [Dev:%d - Targ:%d] - TX transfer done through ISRs\n", Dev, Target);
		}
		else {
			printf("I2C   [Dev:%d - Targ:%d] - TX transfer done with polling\n", Dev, Target);
		}
		if ((XferType & (1U<<2)) != 0U) {
			printf("I2C   [Dev:%d - Targ:%d] - RX transfer done with DMA\n", Dev, Target);
		}
		else if ((XferType & (1U<<1)) != 0U) {
			printf("I2C   [Dev:%d - Targ:%d] - RX transfer done through ISRs\n", Dev, Target);
		}
		else {
			printf("I2C   [Dev:%d - Targ:%d] - RX transfer done with polling\n", Dev, Target);
		}
	  #endif

	  #if (((MY_I2C_OPERATION) & 0x00404) != 0) 
		if (DMAidRX != 0) {
			dma_kill(DMAidRX);						/* Make sure the channel is released			*/
		}
		if (DMAidTX != 0) {
			dma_kill(DMAidTX);						/* Make sure the channel is released			*/
		}
	  #endif

		if (IsExp != 0) {							/* There was an error, try to recover from the	*/
			I2C_ADD_PREFIX(i2c_init) (Dev, 0, 0);	/* error to finish the transfer and re-init the	*/
			IsExp = -7;								/* module										*/
		}
		else {
			if ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) != 0) {
			  #if ((MY_I2C_DEBUG) > 0)
				printf("I2C   [Dev:%d - Targ:%d] - Error -  Transfer error [stat reg: 0x%03X]\n",
				       Dev, Target, (unsigned int)(0xFFF & Reg[I2C_RAW_INTR_STAT_REG]));
				DbgDump = 1;
			  #endif
				IsExp  = -8;
			}
		}

	  #if ((MY_I2C_DEBUG) > 0)
		if (DbgDump != 0) {
			printf("               TX dummies remaining : %u\n", (unsigned)LeftDum);
			printf("               RX data remaining    : %u\n", (unsigned)LeftRX);
		}
	  #endif

	} while ((IsExp != 0)
	  &&     (ReTry-- > 0));

	i2c_disable(Cfg);								/* Disable the controller						*/

	I2C_MTX_UNLOCK(Cfg);

	return(IsExp);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: i2c_send																				*/
/*																									*/
/* i2c_send - send data to a slave I2C device														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int i2c_send(int Dev, int Target, const char *Buf, int Len);								*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev    : controller number																	*/
/*		Target : I2C address of the slave device to send data to									*/
/*		Buf    : pointer to a buffer that holds the data to send									*/
/*		Len    : number of bytes to send to the slave device										*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int : == 0 success																			*/
/*		      != 0 error																			*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int I2C_ADD_PREFIX(i2c_send) (int Dev, int Target, const char *Buf, int Len)
{
const char *BufOri;									/* In case has to retry							*/
I2Ccfg_t *Cfg;										/* De-reference array for faster access			*/
uint32_t  Expiry;									/* With polling, exiry time						*/
int       FirstLen;									/* When starting, always fil the TX FIFO		*/
int       ii;										/* General purpose								*/
int       IsExp;									/* If it has expired							*/
int       LeftTX;									/* Number of dummy write left to do				*/
volatile uint32_t *Reg;								/* Base pointer to I2C controller registers		*/
int       ReMap;									/* Remapped "Dev" to index in g_I2Ccfg[]		*/
int       ReTry;									/* To limit the numbers of retries				*/
int       TimeOut;									/* Timeout in # timer ticks on reception		*/
#if (((MY_I2C_OPERATION) & 0x00100) != 0)
 int      IsrState;
#endif
#if (((MY_I2C_OPERATION) & 0x00200) != 0)
 unsigned int UseISR;
#endif
#if (((MY_I2C_OPERATION) & 0x00400) != 0)
 uint32_t CfgOp[8];									/* DMA configuration commands					*/
 int      DMAid;									/* ID of the DMA transfer						*/
 int32_t  DmaLen;									/* Number of bytes to send using the DMA		*/
#endif
#if ((MY_I2C_DEBUG) > 0)
 int DbgDump;
#endif
#if ((MY_I2C_DEBUG) > 1)
 unsigned int XferType;
#endif

  #if ((MY_I2C_DEBUG) > 1)
	printf("I2C   [Dev:%d - Targ:%d] - Sending %d bytes\n", Dev, Target, Len);
	XferType = 0U;
  #endif

  #if ((MY_I2C_ARG_CHECK) == 0)						/* Arguments validity not checked				*/
	ReMap = g_DevReMap[Dev];
	Cfg = &g_I2Ccfg[ReMap];

  #else
	if ((Dev < 0)									/* Dev # must be within range to be valid		*/
	||  (Dev >= (MY_I2C_MAX_DEVICES))) {
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - Out of range device\n", Dev, Target);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by I2C_LIST_DEVICE				*/
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - Cannot remap device\n", Dev, Target);
	  #endif
		return(-2);
	}

	if (Buf == (char *)NULL) {
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - NULL buffer\n", Dev, Target);
	  #endif
		return(-3);
	}

	Cfg = &g_I2Ccfg[ReMap];							/* This is this device configuration			*/
	if ((g_CfgIsInit == 0)
	||  (Cfg->IsInit == 0)) {						/* The controller is not initialized			*/
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - Device not initialized\n", Dev, Target);
	  #endif
		return(-4);
	}

	if (Len < 0) {
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - Negative number of bytes to send\n", Dev, Target);
	  #endif
		return(-5);
	}
  #endif

	if (Len == 0) {									/* Sending nothing is not considered an error	*/
		return(0);
	}

	BufOri  = Buf;
	Reg     = Cfg->HW;								/* Timeout: use *8 for margin of error			*/
	TimeOut = (9 * (Len+1) * Cfg->NsecBit)			/* Timeout waiting for the the transfer done	*/
	        >> 17;		         					/* Doing /(128*1024) instead of *8/(1000*1000)	*/
	TimeOut = OS_MS_TO_MIN_TICK(TimeOut, 3);		/* Convert ms to RTOS timer tick count			*/

	if (0 != I2C_MTX_LOCK(Cfg)) {					/* Now we need to have exclusive access			*/
		return(-6);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	ReTry = MY_I2C_N_TRIES;							/* Upon failure, try again this number of times	*/
	do {
		if (i2c_wait_for_bb(Cfg)) {					/* Wait for a non-busy bus						*/
			I2C_MTX_UNLOCK(Cfg);
		  #if ((MY_I2C_DEBUG) > 0)
			printf("I2C   [Dev:%d - Targ:%d] - Error - Bus stalled in busy mode\n", Dev, Target);
		  #endif
			return(-7);								/* Did not reached a non-busy bus, error		*/
	    }

		i2c_disable(Cfg);
		Reg[I2C_TAR_REG] = Target;					/* Init the transfer sequence					*/
		i2c_enable(Cfg);

		Buf      = BufOri;
		IsExp    = 0;								/* Is Exp is also the error indicator			*/
		LeftTX   = Len;
		FirstLen = Len;								/* First thing done is filling the TX FIFO		*/
		if (FirstLen > (MY_I2C_TX_FIFO_SIZE)) {
			FirstLen = MY_I2C_TX_FIFO_SIZE;
		}

		Reg[I2C_RX_TL_REG]  = MY_I2C_RX_WATERMARK;
		Reg[I2C_TX_TL_REG]  = MY_I2C_TX_WATERMARK;
		Reg[I2C_DMA_CR_REG] = 0;					/* No DMA (yet)									*/

	  #if ((MY_I2C_DEBUG) > 0)
		DbgDump = 0;
	  #endif

	  #if (((MY_I2C_OPERATION) & 0x00200) != 0)		/* ISRs can be used								*/
		UseISR = 0U;
		if ((Len >= (MY_I2C_MIN_4_TX_ISR))			/* If enough data to TX to use ISRs				*/
		&&  (Len > FirstLen)) {						/* And more than TX FIFO size, filled at start	*/
			UseISR |= 2U;							/* Use ISR for both sending & EOT				*/
			Reg[I2C_TX_TL_REG] = MY_I2C_TX_WATERMARK;	/* TX FIFO watermark for the ISRs			*/
		}
	  #endif

	  #if (((MY_I2C_OPERATION) & 0x00400) != 0)		/* DMA can be used								*/
		DMAid      = 0;								/* If DMA is not used, it will remain == 0		*/
		DmaLen     = 0;								/* To remove compiler warning					*/

		if  ((Len    >= (MY_I2C_MIN_4_TX_DMA))		/* Enough data to use the DMA & > TX FIFO size	*/
		&&   ((Len-FirstLen) >  1)) {				/* >1 : Need to manually add the stop condition	*/

		  #if ((MY_I2C_DEBUG) > 1)
			XferType |= 1U << 10;					/* Use the same bits as in I2C_OPERATION		*/
		  #endif

			ii = (MY_I2C_TX_FIFO_SIZE)/2;			/* Try to transfer the largest busrt length		*/
			if (ii > (DMA_MAX_BURST)) {				/* and at the same time keep the FIFO as full	*/
				ii = DMA_MAX_BURST;					/* as possible to maximize the tranfer rate		*/
			}
			if (ii > (Len-FirstLen)) {
				ii = Len-FirstLen;
			}
			Reg[I2C_DMA_TDLR_REG] = (MY_I2C_TX_FIFO_SIZE)	/* Optimal DMA watermark				*/
			                      - ii;
			Reg[I2C_DMA_CR_REG]   = 0x2;			/* Enable DMA TX operations						*/
			Reg[I2C_TX_TL_REG]    = 0;				/* When using ISR for EOT: when TX FIFO empty	*/

			DmaLen = (Len-FirstLen)					/* Number of frames to send with the DMA		*/
			       - 1;								/* Manually send last to set stop condition		*/

		  #if (((MY_I2C_OPERATION) & 0x00200) != 0)
			UseISR &= 4U;							/* Only keep stop EOT interrupt					*/
		  #endif

			CfgOp[0] = DMA_CFG_NOSTART;				/* No start: must write start in command reg	*/
		  #if (((MY_I2C_OPERATION) & 0x00200) != 0)
		 	CfgOp[1] = (UseISR != 0U)				/* And it's done when filling TX FIFO at start	*/
			         ? DMA_CFG_EOT_ISR				/* Select the type of waiting					*/
			         : DMA_CFG_EOT_POLLING;
		  #else
			CfgOp[1] = DMA_CFG_EOT_POLLING;
		  #endif
		  #if ((MY_I2C_REMAP_LOG_ADDR) == 0)
			CfgOp[2] = 0;
		  #else
			CfgOp[2] = DMA_CFG_LOGICAL_DST;
			CfgOp[3] = DMA_CFG_LOGICAL_SRC;
			CfgOp[4] = 0;
		  #endif

			IsExp = dma_xfer(DMA_DEV,
			                 (void *)&Reg[I2C_DATA_CMD_REG], 0, g_DMAchan[Dev][0],
				             (void *)(Buf+FirstLen), 1, -1,
			                 1, (MY_I2C_TX_FIFO_SIZE)-Reg[I2C_DMA_TDLR_REG], DmaLen,
				             DMA_OPEND_NONE, NULL, 0,
				             &CfgOp[0], &DMAid, TimeOut);

			if (IsExp != 0) {						/* Something went wrong setting up the DMA		*/
			  #if ((MY_I2C_DEBUG) > 0)
				printf("I2C   [Dev:%d - Targ:%d] - Error -  TX DMA reported error #%d\n",
				        Dev, Target, IsExp);
			  #endif
				IsExp -= 10;						/* Re-adjust error value for I2C context report	*/
			}
		}
	  #endif

	  #if (((MY_I2C_OPERATION) & 0x00100) != 0)
		IsrState = OSintOff();
	  #endif
													/* First thing is filling the TX FIFO			*/
		ii = 1U << 10;								/* Restart command								*/
		for ( ; FirstLen>0 ; FirstLen--) {			/* Fill as much as possible the TX FIFO			*/
			Reg[I2C_DATA_CMD_REG] = (0xFF & (unsigned int)(*Buf++))
			                      | ((--LeftTX == 0) ? (1U << 9) : 0U)
			                      | ii;				/* 1<<9 is for the stop condition for last byte	*/
			ii = 0;									/* Not a restart anymore						*/
		}

		Expiry = OS_TICK_EXP(TimeOut);				/* When polling has to expire					*/

	  #if (((MY_I2C_OPERATION) & 0x00400) != 0)		/* DMA can be used								*/
		if ((DMAid != 0)							/* The DMA has to send data						*/
		&&  (IsExp == 0)
		&&  ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)) {

		  #if (((MY_I2C_OPERATION) & 0x00100) != 0)
			OSintBack(IsrState);
		  #endif
													/* Wait for DMA done before sending last + stop	*/
			dma_start(DMAid);						/* DMA done is not EOT: is all in TX FIFO		*/

			while ((IsExp == 0)						/* Wait for room in TX FIFO to write stop cond	*/
			&&     (Reg[I2C_TXFLR_REG] > ((MY_I2C_TX_FIFO_SIZE)-2))) {
				IsExp = OS_HAS_TIMEDOUT(Expiry);	/* Check if has timed out						*/
			}

			if (IsExp == 0) {						/* No timeout, write last byte + stop condition	*/
				Reg[I2C_DATA_CMD_REG] = (0xFF & (unsigned int)(Buf[DmaLen]))
				                      | (1U << 9);	/* 1<<9 is for the stop condition for last byte	*/
			}
			else {									/* Has timeout, do a clean-up					*/
			  #if ((MY_I2C_DEBUG) > 0)
				printf("I2C   [Dev:%d - Targ:%d] - Error - TX DMA timeout\n", Dev, Target);
			  #endif
			}

			LeftTX = 0;								/* DMA write all but one, last one was written	*/
		}											/* in polling for sending stop condition		*/
	  #endif		

	  #if (((MY_I2C_OPERATION) & 0x00200) != 0)		/* Using ISRs									*/
		if ((UseISR != 0U)							/* Had enough data for ISR transfers			*/
		&&  ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)) {

		  #if ((MY_I2C_DEBUG) > 1)
			XferType |= 1U << 9;					/* Use the same bits as in I2C_OPERATION		*/
		  #endif

			Cfg->ISRleftDum    = 0;
			Cfg->ISRleftRX     = 0;
			Cfg->ISRleftTX     = LeftTX;			/* When used with DMA for EOT, LeftTX == 0		*/
			Cfg->ISRbufTX      = (char *)Buf;
			SEMreset(Cfg->MySema);					/* Make sure there are no pending posting		*/

			Reg[I2C_INTR_MASK_REG] = (((UseISR&4U)>>2) << 9)	/* Enable the stop condition int	*/
			                       | (((UseISR&2U)>>1) << 4);	/* Enable <= TX watermark int		*/

		  #if (((MY_I2C_OPERATION) & 0x00100) != 0)	/* Can't be disable waiting for ISR posting		*/
			OSintBack(IsrState);
		  #endif

			IsExp = SEMwait(Cfg->MySema, TimeOut);

			Reg[I2C_INTR_MASK_REG] = 0;				/* Disable all I2C interrupts source			*/

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {
				printf("I2C   [Dev:%d - Targ:%d] - Error - Timeout on semaphore\n", Dev, Target);
				DbgDump = 1;
			}
		  #endif

			LeftTX = Cfg->ISRleftTX;				/* ISR xfer all, but needed in case of error	*/
		}
	  #endif										/* Flow through with polling					*/

		while ((LeftTX != 0)						/* Send everything left to send					*/
		&&     (IsExp == 0)
		&&     ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)) {

			I2C_EXPIRY_TX(IsExp, IsrState, Expiry);	/* Check if has timed out						*/

			if (Reg[I2C_TXFLR_REG] != (MY_I2C_TX_FIFO_SIZE)) {	/* When room in the FIFO, do a write*/
				Reg[I2C_DATA_CMD_REG] = (0xFF & (unsigned int)(*Buf++))
				                      | ((--LeftTX == 0) ? (1U << 9) : 0U);	/* Stop on last byte	*/
			}

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {
				printf("I2C   [Dev:%d - Targ:%d] - Error - Timeout sending after filling FIFO\n",
				       Dev, Target);
				DbgDump = 1;
			}
		  #endif
		}

	  #if (((MY_I2C_OPERATION) & 0x00100) != 0)
		OSintBack(IsrState);
	  #endif

		while ((IsExp == 0)							/* Wait for the stop condition					*/
		&&     ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)
		&&     ((Reg[I2C_RAW_INTR_STAT_REG] & (1U << 9)) == 0)) {	/* Stop condition is bit 9		*/

			IsExp = OS_HAS_TIMEDOUT(Expiry);		/* Check if has timed out						*/

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {
				printf("I2C   [Dev:%d - Targ:%d] - Error - Timeout on stop condition\n",
				       Dev, Target);
			}
		  #endif
		}

	    g_Dummy = Reg[I2C_CLR_STOP_DET_REG];		/* To clear the stop condition, read register	*/

	  #if (((MY_I2C_OPERATION) & 0x00400) != 0) 
		if (DMAid != 0) {
			dma_kill(DMAid);						/* Make sure the channel is released			*/
		}
	  #endif

	  #if ((MY_I2C_DEBUG) > 1)						/* Print after the Xfer to not impact polling	*/
		if ((XferType & (1U<<10)) != 0U) {
			printf("I2C   [Dev:%d - Targ:%d] - TX transfer done with DMA\n", Dev, Target);
		}
		else if ((XferType & (1U<<9)) != 0U) {
			printf("I2C   [Dev:%d - Targ:%d] - TX transfer done through ISRs\n", Dev, Target);
		}
		else {
			printf("I2C   [Dev:%d - Targ:%d] - TX transfer done with polling\n", Dev, Target);
		}
	  #endif

		if (IsExp != 0) {							/* There was an error, try to recover from the	*/
			I2C_ADD_PREFIX(i2c_init) (Dev, 0, 0);	/* error to finish the transfer and re-init the	*/
			IsExp = -8;							/* module										*/
		}
		else {
			if ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) != 0) {
			  #if ((MY_I2C_DEBUG) > 0)
				printf("I2C   [Dev:%d - Targ:%d] - Error - Transfer error [stat reg: 0x%03X]\n",
				       Dev, Target, (unsigned int)(0xFFF & Reg[I2C_RAW_INTR_STAT_REG]));
			  #endif
				IsExp = -9;
			}
		}

	  #if ((MY_I2C_DEBUG) > 0)
		if (DbgDump != 0) {
			printf("               TX data remaining    : %u\n", (unsigned)LeftTX);
		}
	  #endif

	} while ((IsExp != 0)
	  &&     (ReTry-- > 0));

	i2c_disable(Cfg);								/* Disable the controller						*/

	I2C_MTX_UNLOCK(Cfg);

	return(IsExp);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: i2c_send_recv																			*/
/*																									*/
/* i2c_send_recv - send data to then receive data from a slave I2C device							*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int i2c_send_recv(int Dev, int Target, const char *BufTX, int LenTX, char *BufRX,int LenRX);*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev    : controller number																	*/
/*		Target : I2C address of the slave device to send data to									*/
/*		BufTX  : pointer to a buffer that holds the data to send									*/
/*		LenTX  : number of bytes to send to the slave device										*/
/*		BufRX  : pointer to a buffer to hold the data received										*/
/*		LenRX  : number of bytes to receive from the slave device									*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int : == 0 success																			*/
/*		      != 0 error																			*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int I2C_ADD_PREFIX(i2c_send_recv) (int Dev, int Target, const char *BufTX, int LenTX, char *BufRX,
                                   int LenRX)
{
      char *BufRxOri;								/* In case has to retry							*/
const char *BufTxOri;								/* In case has to retry							*/
I2Ccfg_t *Cfg;										/* De-reference array for faster access			*/
uint32_t  Expiry;									/* With polling, exiry time						*/
int       FirstLen;									/* At start, # to fill the TX FIFO				*/
int       ii;										/* General purpose								*/
int       IsExp;									/* If it has expired							*/
int       LeftDum;
int       LeftRX;
int       LeftTX;
volatile uint32_t *Reg;								/* Base pointer to I2C controller registers		*/
int       ReMap;									/* Remapped "Dev" to index in g_I2Ccfg[]		*/
int       ReTry;									/* To limit the numbers of retries				*/
int       TimeOut;									/* Timeout in # timer ticks on reception		*/
#if (((MY_I2C_OPERATION) & 0x00101) != 0)
 int      IsrState;
#endif
#if (((MY_I2C_OPERATION) & 0x00202) != 0)
 unsigned int UseISR;
#endif
#if (((MY_I2C_OPERATION) & 0x00404) != 0)
 uint32_t CfgOp[8];									/* DMA configuration commands					*/
 int      DMAidRX;									/* ID of the RX DMA transfer					*/
 int      DMAidTX1;									/* ID of the Data DMA transfer					*/
 int      DMAidTX2;									/* ID of the Dummy DMA transfer					*/
 #if (((MY_I2C_OPERATION) & 0x00400) != 0)
  int32_t DmaLenTX1;								/* Number of bytes to send using the DMA		*/
  int32_t DmaLenTX2;								/* Number of dummies to send using the DMA		*/
 #endif
#endif
#if ((MY_I2C_DEBUG) > 0)
 int DbgDump;
#endif
#if ((MY_I2C_DEBUG) > 1)
 unsigned int XferType;
#endif

  #if ((MY_I2C_DEBUG) > 1)
	printf("I2C   [Dev:%d - Targ:%d] - Writing %d byte and reading %d bytes\n", Dev, Target, LenTX,
	       LenRX);
	XferType = 0U;
  #endif

  #if ((MY_I2C_ARG_CHECK) == 0)						/* Arguments validity not checked				*/
	ReMap = g_DevReMap[Dev];
	Cfg = &g_I2Ccfg[ReMap];

  #else
	if ((Dev < 0)									/* Dev # must be within range to be valid		*/
	||  (Dev >= (MY_I2C_MAX_DEVICES))) {
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - Out of range device\n", Dev, Target);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by I2C_LIST_DEVICE				*/
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - Cannot remap device\n", Dev, Target);
	  #endif
		return(-2);
	}

	if ((BufRX == (char *)NULL)
	||  (BufTX == (char *)NULL)) {
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - NULL buffer\n", Dev, Target);
	  #endif
		return(-3);
	}

	Cfg = &g_I2Ccfg[ReMap];							/* This is this device configuration			*/

	if (Cfg->IsInit == 0) {
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - Device not initialized\n", Dev, Target);
	  #endif
		return(-4);
	}

	if ((LenTX < 0)
	||  (LenRX < 0)) {
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d - Targ:%d] - Error - Negative number of bytes to send or receive\n",
		       Dev, Target);
	  #endif
		return(-5);
	}

  #endif

	if ((LenTX <= 0)								/* No data to send / recveive, exit				*/
	&&  (LenRX <= 0)) {								/* Not considered an error						*/
		return(0);
	}

	BufRxOri = BufRX;
	BufTxOri = BufTX;
	Reg      = Cfg->HW;								/* Timeout: use *8 for margin of error			*/
	TimeOut  = (9 * (LenRX+LenTX+2) * Cfg->NsecBit)	/* Timeout waiting for the the transfer done	*/
	         >> 17;		         					/* Doing /(128*1024) instead of *8/(1000*1000)	*/
	TimeOut  = OS_MS_TO_MIN_TICK(TimeOut, 3);		/* Convert ms to RTOS timer tick count			*/

	if (0 != I2C_MTX_LOCK(Cfg)) {					/* Now we need to have exclusive access			*/
		return(-6);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	ReTry = MY_I2C_N_TRIES;							/* Upon failure, try again this number of times	*/
	do {
		if (0 != i2c_wait_for_bb(Cfg)) {			/* Wait for a non-busy bus						*/
			I2C_MTX_UNLOCK(Cfg);
		  #if ((MY_I2C_DEBUG) > 0)
			printf("I2C   [Dev:%d - Targ:%d] - Error - Bus stalled in busy mode\n", Dev, Target);
		  #endif
			return(-7);								/* Did not reached a non-busy bus, error		*/
	    }

		i2c_disable(Cfg);
		Reg[I2C_TAR_REG] = Target;					/* Init the transfer sequence					*/
		i2c_enable(Cfg);

		BufRX    = BufRxOri;						/* When retrying, need to re-start at beginning	*/
		BufTX    = BufTxOri;
		IsExp    = 0;								/* != 0 indicates a timeout						*/
		LeftDum  = LenRX;							/* To read, need to send dummy with bit #8 set	*/
		LeftRX   = LenRX;
		LeftTX   = LenTX;							/* Number of data byte left to send				*/
		FirstLen = LeftTX + LeftDum;				/* First thing done is filling the TX FIFO		*/
		if (FirstLen > (MY_I2C_TX_FIFO_SIZE)) {
			FirstLen = MY_I2C_TX_FIFO_SIZE;
		}

		Reg[I2C_RX_TL_REG]  = MY_I2C_RX_WATERMARK;
		Reg[I2C_TX_TL_REG]  = MY_I2C_TX_WATERMARK;
		Reg[I2C_DMA_CR_REG] = 0;					/* No DMA (yet)									*/

	  #if ((MY_I2C_DEBUG) > 0)
		DbgDump = 0;
	  #endif

	  #if (((MY_I2C_OPERATION) & 0x00202) != 0)		/* ISRs can be used								*/
		UseISR = 0U;

	   #if (((MY_I2C_OPERATION) & 0x00002) != 0)
		if ((LeftRX >= (MY_I2C_MIN_4_RX_ISR))		/* If enough data to RX to use ISRs				*/
		&&  (LeftRX >  (MY_I2C_RX_WATERMARK))) {	/* And more than RX threshold					*/
			UseISR |= 1U;							/* Use ISR for RX and EOT						*/
		}
	   #endif

	   #if (((MY_I2C_OPERATION) & 0x00200) != 0)
		ii = LeftTX									/* Total number of bytes to send. Dummies are	*/
		   + LeftDum;								/* needed to RX the data from the bus			*/
		if ((ii >= (MY_I2C_MIN_4_TX_ISR))			/* If enough data to TX to use ISRs				*/
		&&  (ii >  (MY_I2C_TX_WATERMARK))			/* And more than TX threshold					*/
		&&  (ii >   FirstLen)) {					/* And with leftover after initial TX FIFO fill	*/
			UseISR |= 2U;							/* Use ISR for TX and EOT						*/
		}
	   #endif

	  #endif

	  #if (((MY_I2C_OPERATION) & 0x00404) != 0)		/* DMA can be used								*/
		DMAidRX    = 0;								/* If DMA is not used, it will remain == 0		*/
		DMAidTX1   = 0;								/* If DMA is not used, it will remain == 0		*/
		DMAidTX2   = 0;								/* If DMA is not used, it will remain == 0		*/

	   #if (((MY_I2C_OPERATION) & 0x00004) != 0)	/* RX DMA can be used							*/
		if  (LenRX >= (MY_I2C_MIN_4_RX_DMA))	{	/* Enough data to use the DA & > RX FIFO size	*/

		  #if ((MY_I2C_DEBUG) > 1)
			XferType |= 1U << 2;					/* Use the same bits as in I2C_OPERATION		*/
		  #endif

			ii  = (MY_I2C_RX_FIFO_SIZE)/2;			/* Figure out the largest useable burst length	*/
			if (ii > (DMA_MAX_BURST)) {				/* Maximizing the burst length maximises the	*/
				ii = DMA_MAX_BURST;					/* transfer rate								*/
			}
			if (ii > LenRX) {
				ii = LenRX;
			}
			Reg[I2C_DMA_RDLR_REG] = ii-1;			/* Watermark: trigger when #RX FIFO > watermark	*/
			Reg[I2C_DMA_CR_REG]  |= 0x1;			/* Enable DMA RX operations						*/

			CfgOp[0] = DMA_CFG_NOWAIT;				/* RX DMA started but won't wait here			*/
			CfgOp[1] = (((MY_I2C_OPERATION) & 0x00002) != 0)
			         ? DMA_CFG_EOT_ISR				/* When using dma_wait(), type of waiting		*/
			         : DMA_CFG_EOT_POLLING;
		  #if ((MY_I2C_REMAP_LOG_ADDR) == 0)
			CfgOp[2] = 0;
		  #else
			CfgOp[2] = DMA_CFG_LOGICAL_DST;
			CfgOp[3] = DMA_CFG_LOGICAL_SRC;
			CfgOp[4] = 0;
		  #endif

			IsExp = dma_xfer(DMA_DEV,
				             BufRX, 1, -1,
			                 (void *)&Reg[I2C_DATA_CMD_REG], 0, g_DMAchan[Dev][1],
			                 1, Reg[I2C_DMA_RDLR_REG]+1, LenRX,
				             DMA_OPEND_NONE, NULL, 0,
				             &CfgOp[0], &DMAidRX, TimeOut);

			LeftRX = 0;								/* DMA reads all received bytes					*/

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {						/* Something went wrong setting up the DMA		*/
				printf("I2C   [Dev:%d - Targ:%d] - Error - RX DMA reported error #%d\n",
				        Dev, Target, IsExp);
			}
		  #endif

		  #if (((MY_I2C_OPERATION) & 0x00002) != 0)
			UseISR &= ~1U;							/* ISRs are not needed for receiving			*/
		  #endif
		}
	   #endif

	   #if (((MY_I2C_OPERATION) & 0x00400) != 0)	/* DMA TX can be used							*/

		if ((LeftTX+LeftDum) <= (FirstLen+1)) {
			DmaLenTX1 = 0;							/* Data and dummies all sent when filling FIFO	*/
			DmaLenTX2 = 0;							/* +1 as even with DMA, need to manually send 	*/
		}											/* the stop condition							*/
		else if (LeftTX <= FirstLen) {				/* All data to send done when filling TX FIFO	*/
			DmaLenTX1 = 0;
			DmaLenTX2 = (LeftTX+LeftDum)
			          - FirstLen
		              - 1;
		}
		else {
			DmaLenTX1 = LeftTX						/* DMA is left to xfer this						*/
			          - FirstLen;
			DmaLenTX2 = LeftDum
			          - 1;							/* -1 as need to manually send stop cond		*/
		}

		if (DmaLenTX2 < 0) {
			DmaLenTX1--;
			DmaLenTX2 = 0;
		}

		ii = (MY_I2C_TX_FIFO_SIZE)/2;				/* Try to transfer the largest busrt length		*/
		if (ii > (DMA_MAX_BURST)) {					/* and at the same time keep the FIFO as full	*/
			ii = DMA_MAX_BURST;						/* as possible to maximize the tranfer rate		*/
		}
		if ((ii > DmaLenTX1)
		&&  (DmaLenTX1 > 0)) {
			ii = DmaLenTX1;
		}

		if ((ii > DmaLenTX2)
		&&  (DmaLenTX2 > 0)) {
			ii = DmaLenTX2;
		}

		if (((DmaLenTX1+DmaLenTX2) != 0)
		&&  (IsExp == 0)) {
			Reg[I2C_DMA_TDLR_REG] = (MY_I2C_TX_FIFO_SIZE)	/* Optimal DMA watermark				*/
			                      - ii;
			Reg[I2C_DMA_CR_REG]  |= 0x2;			/* Enable DMA TX operations						*/

		  #if ((MY_I2C_DEBUG) > 1)
			XferType |= 1U << 10;					/* Use the same bits as in I2C_OPERATION		*/
		  #endif

			if  (DmaLenTX2 != 0) {					/* Dummies send with the DMA					*/
				CfgOp[0] = (DmaLenTX1 > 0)
				         ? DMA_CFG_WAIT_TRG			/* Wait for the trigger from after data sent	*/
				         : DMA_CFG_NOSTART;			/* Is manually started after TX FIFO filled		*/
				CfgOp[1] = (DMAidRX == 0)			/* When RX DMA is used, we can block on TX		*/
				         ? DMA_CFG_NOWAIT			/* until it's time to send the stop condition	*/
				         : DMA_CFG_NOP;				/* else, RX is done with polling or ISRs		*/
				CfgOp[2] = (((MY_I2C_OPERATION) & 0x00200) != 0)
				         ? DMA_CFG_EOT_ISR			/* If waiting, type of wait						*/
				         : DMA_CFG_EOT_POLLING;
				CfgOp[3] = DMA_CFG_NOCACHE_SRC;		/* g_DumVal is static and was flushed at init	*/
			  #if ((MY_I2C_REMAP_LOG_ADDR) == 0)
				CfgOp[4] = 0;
			  #else
				CfgOp[4] = DMA_CFG_LOGICAL_DST;
				CfgOp[5] = DMA_CFG_LOGICAL_SRC;
				CfgOp[6] = 0;
			  #endif

				IsExp = dma_xfer(DMA_DEV,
				                (void *)&Reg[I2C_DATA_CMD_REG], 0, g_DMAchan[Dev][0],
					             &g_DumVal[0], 0, -1,
				                 2, (MY_I2C_TX_FIFO_SIZE)-Reg[I2C_DMA_TDLR_REG], DmaLenTX2,
					             DMA_OPEND_NONE, NULL, 0,
					             &CfgOp[0], &DMAidTX2, TimeOut);

			}

			if ((DmaLenTX1 != 0)					/* Data to send with the DMA					*/
			&&  (IsExp == 0)) {
				CfgOp[0] = DMA_CFG_NOSTART;			/* No start: must write start in command reg	*/
				CfgOp[1] = ((DMAidRX != 0)			/* When RX DMA is used, we can block on TX		*/
				        &&  (DmaLenTX2 == 0))		/* only if DMA not used with dummies(none or 1)	*/
				         ? DMA_CFG_NOP				/* until it's time to send the stop condition	*/
				         : DMA_CFG_NOWAIT;			/* else, RX is done with polling or ISRs		*/
				CfgOp[2] = (((MY_I2C_OPERATION) & 0x00200) != 0)
				         ? DMA_CFG_EOT_ISR			/* If waiting, type of wait						*/
				         : DMA_CFG_EOT_POLLING;
			  #if ((MY_I2C_REMAP_LOG_ADDR) == 0)
				CfgOp[3] = 0;
			  #else
				CfgOp[3] = DMA_CFG_LOGICAL_DST;
				CfgOp[4] = DMA_CFG_LOGICAL_SRC;
				CfgOp[5] = 0;
			  #endif

				IsExp = dma_xfer(DMA_DEV,
				                 (void *)&Reg[I2C_DATA_CMD_REG], 0, g_DMAchan[Dev][0],
					             (void *)(BufTX+FirstLen), 1, -1,
				                 1, (MY_I2C_TX_FIFO_SIZE)-Reg[I2C_DMA_TDLR_REG], DmaLenTX1,
					             DMA_OPEND_NONE, NULL, 0,
					             &CfgOp[0], &DMAidTX1, TimeOut);
			}

		  #if (((MY_I2C_OPERATION) & 0x00002) != 0)
			UseISR &= ~2U;							/* ISRs are not needed for TXing				*/
		  #endif

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {						/* Something went wrong setting up the DMA		*/
				printf("I2C   [Dev:%d - Targ:%d] - Error - TX DMA reported error #%d\n",
				        Dev, Target, IsExp);
			}
		  #endif
		}
	   #endif
	  #endif

	  #if (((MY_I2C_OPERATION) & 0x00101) != 0)
		IsrState = OSintOff();
	  #endif

		if (IsExp == 0) {							/* First thing is filling the TX FIFO			*/
			ii = 1U << 10;							/* Restart command								*/
			for ( ; (FirstLen>0) && (LeftTX>0) ; FirstLen--) {/* Fill as much as possible TX FIFO	*/
				Reg[I2C_DATA_CMD_REG] = (0xFF & (unsigned int)(*BufTX++))
				                      | ((((--LeftTX)+LeftDum) == 0) ? (1U << 9) : 0U)
				                      | ii;			/* 1<<9 is for the stop condition for last byte	*/
				ii = 0;								/* Not a restart anymore						*/
			}
											
			for ( ; FirstLen>0 ; FirstLen--) {
				Reg[I2C_DATA_CMD_REG] = (1U << 8)	/* Data command read							*/
				                      | ((--LeftDum == 0) ? (1U << 9) : 0U)
				                      | ii;
				ii = 0;
			}
		}

		Expiry = OS_TICK_EXP(TimeOut);				/* This is where the transfer is started		*/

	  #if (((MY_I2C_OPERATION) & 0x00400) != 0)		/* DMA TX can be used							*/
		if (((DmaLenTX1+DmaLenTX2) != 0)			/* DMA is used for TXing						*/
		&&  (IsExp == 0)
		&&  ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)) {

		  #if (((MY_I2C_OPERATION) & 0x00101) != 0)
			OSintBack(IsrState);
		  #endif
													/* Start the DMA TX: depends if sending data	*/
			IsExp = dma_start((DMAidTX1 != 0) ? DMAidTX1 : DMAidTX2);	/* and/or dummies			*/
			if (DMAidTX2 != 0) {
				LeftDum = 1;
				LeftTX  = 0;
			}
			else {
				LeftDum = 0;
				LeftTX  = 1;
			}

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {						/* Only time could be wrong is timeout			*/
				printf("I2C   [Dev:%d - Targ:%d] - Error - Timeout waiting for TX DMA\n",
				        Dev, Target);
				DbgDump = 1;
			}
		  #endif
		}
	  #endif


	  #if (((MY_I2C_OPERATION) & 0x00202) != 0)		/* Using ISRs									*/
		if ((UseISR != 0U)							/* Enough RX to receive to use ISRs?			*/
		&&  ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)) {

			if (UseISR == 6U) {						/* If both are Xfered through ISR, can block	*/
				UseISR = 7U;
			}
			Cfg->ISRleftDum = 0;
			Cfg->ISRleftRX  = 0;
			Cfg->ISRleftTX  = 0;
			Cfg->ISRbufRX   = (char *)BufRX;
			Cfg->ISRbufTX   = (char *)BufTX;

			if ((UseISR & 2U) != 0U) {				/* Using TX ISRs								*/
				Cfg->ISRleftDum = LeftDum;
				Cfg->ISRleftTX  = LeftTX;
				LeftDum         = 0;
				LeftTX          = 0;
			  #if ((MY_I2C_DEBUG) > 1)
				XferType |= 1U << 9;				/* Use the same bits as in I2C_OPERATION		*/
			  #endif

			}

			if ((UseISR & 1U) != 0U) {				/* Using RX ISRs								*/
				Cfg->ISRleftRX = LeftRX;
				LeftRX         = 0;
			  #if ((MY_I2C_DEBUG) > 1)
				XferType |= 1U << 1;				/* Use the same bits as in I2C_OPERATION		*/
			  #endif

			}

			SEMreset(Cfg->MySema);					/* Make sure there are no pending posting		*/

			Reg[I2C_INTR_MASK_REG] = (((UseISR&4U)>>2) << 9)	/* Enable the stop condition int	*/
			                       | (((UseISR&2U)>>1) << 4)	/* Enable <= TX watermark int		*/
			                       | (((UseISR&1U)>>0) << 2);	/* Enable above RX watermark		*/

		  #if (((MY_I2C_OPERATION) & 0x00101) != 0)	/* Can't be disable as Xfer done in ISR			*/
			OSintBack(IsrState);
		  #endif
		}
	  #endif
													/* Transfer through Polling						*/
		while (((LeftRX+LeftTX+LeftDum) != 0)		/* Read / write everything left					*/
		&&     (IsExp == 0)
		&&     ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)) {

			I2C_EXPIRY_RXTX(IsExp, IsrState, Expiry);	/* Check if has timed out					*/

			while ((LeftRX != 0)					/* Non-zero number of bytes in RX FIFO			*/
			&&     (Reg[I2C_RXFLR_REG] != 0U)) {
				*BufRX++ = (char)Reg[I2C_DATA_CMD_REG];
				LeftRX--;
			}

		  #if (((MY_I2C_OPERATION) & 0x00400) != 0)	/* DMA TX may be be used						*/
			if (DMAidTX2 != 0) {					/* If so, need to send the stop condition		*/
				if (LeftDum != 0) {					/* If dummy hasn't been sent					*/
					if ((dma_done(DMAidTX2) != 0)	/* TX DMA done and room in FIFO, put stop cond	*/
					&&  (Reg[I2C_TXFLR_REG] <= ((MY_I2C_TX_FIFO_SIZE))/2)) {
						LeftDum = 0;				/* Use SIZE/2 to make sure DMA is really done	*/
						Reg[I2C_DATA_CMD_REG] = (1U << 8)	/* Data command read					*/
						                      | (1U << 9);	/* Stop on last byte					*/
					}
				}
			}
			else if (DMAidTX1 != 0) {
				if (LeftTX != 0) {
					if ((dma_done(DMAidTX1) != 0)	/* TX DMA done and room in FIFO, put stop cond	*/
					&&  (Reg[I2C_TXFLR_REG] <= ((MY_I2C_TX_FIFO_SIZE))/2)) {
						LeftTX = 0;					/* Use SIZE/2 to make sure DMA is really done	*/
						Reg[I2C_DATA_CMD_REG] = (0xFF & (unsigned int)(*BufTX++))
						                      | (((LeftDum) == 0) ? (1U << 9) : 0U)
						                      | (1U << 9);	/* Stop on last byte					*/
					}
				}
			}
			else
		  #endif
			if (LeftTX != 0) {
				while ((LeftTX > 0)					/* If dummy left to write and room in the FIFO	*/
				&&     (Reg[I2C_TXFLR_REG] != (MY_I2C_TX_FIFO_SIZE))) {
					Reg[I2C_DATA_CMD_REG] = (0xFF & (unsigned int)(*BufTX++))
					                      | ((((--LeftTX)+LeftDum) == 0) ? (1U << 9) : 0U);
				}
			}
			else if (LeftDum > 0) {
				while ((LeftDum > 0)				/* If dummy left to write and room in the FIFO	*/
				&&     (Reg[I2C_TXFLR_REG] != (MY_I2C_TX_FIFO_SIZE))) {
					Reg[I2C_DATA_CMD_REG] = (1U << 8)	/* Data command read						*/
					                      | ((--LeftDum == 0) ? (1U << 9) : 0U);/* Stop on last byte*/
				}
			}

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {
				printf("I2C   [Dev:%d - Targ:%d] - Error - Timeout sending after filling FIFO\n",
				        Dev, Target);
				DbgDump = 1;
			}
		  #endif
		}

	  #if (((MY_I2C_OPERATION) & 0x00101) != 0)		/* If timeout nor required, done here			*/
		OSintBack(IsrState);
	  #endif

	  #if ((MY_I2C_DEBUG) > 1)						/* Print after the Xfer to not impact polling	*/
		if ((XferType & (1U<<10)) != 0U) {
			printf("I2C   [Dev:%d - Targ:%d] - TX transfer done with DMA\n", Dev, Target);
		}
		else if ((XferType & (1U<<9)) != 0U) {
			printf("I2C   [Dev:%d - Targ:%d] - TX transfer done through ISRs\n", Dev, Target);
		}
		else {
			printf("I2C   [Dev:%d - Targ:%d] - TX transfer done with polling\n", Dev, Target);
		}
		if ((XferType & (1U<<2)) != 0U) {
			printf("I2C   [Dev:%d - Targ:%d] - RX transfer done with DMA\n", Dev, Target);
		}
		else if ((XferType & (1U<<2)) != 0U) {
			printf("I2C   [Dev:%d - Targ:%d] - RX transfer done through ISRs\n", Dev, Target);
		}
		else {
			printf("I2C   [Dev:%d - Targ:%d] - RX transfer done with polling\n", Dev, Target);
		}
	  #endif

	  #if (((MY_I2C_OPERATION) & 0x00002) != 0)		/* Using ISRs, wait for end of them				*/
		if ((UseISR != 0U)
		&&  (IsExp  == 0)
		&&  ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)) {

			IsExp = SEMwait(Cfg->MySema, TimeOut);

			Reg[I2C_INTR_MASK_REG] = 0;				/* Disable all I2C interrupts source			*/

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {
				printf("I2C   [Dev:%d - Targ:%d] - Error - Timeout on semaphore\n", Dev, Target);
				DbgDump = 1;
			}
		  #endif
		}

	   #if ((MY_I2C_DEBUG) > 0)
	    #if (((MY_I2C_OPERATION) & 0x00002) != 0)
		if ((UseISR & 1U) != 0U) {					/* Recover these for error report				*/
			LeftRX = Cfg->ISRleftRX;
		}
		#endif
		#if (((MY_I2C_OPERATION) & 0x00200) != 0)
		if ((UseISR & 2U) != 0U) {
			LeftDum = Cfg->ISRleftDum;
			LeftTX  = Cfg->ISRleftTX;
		}
	    #endif
	   #endif

	  #endif										/* Flow through with polling					*/

	  #if (((MY_I2C_OPERATION) & 0x00004) != 0)
		if ((DMAidRX != 0)							/* If the RX DMA is still going on, wait for it	*/
		&&  (IsExp == 0)							/* to be done									*/
		&&     ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)) {

			IsExp = dma_wait(DMAidRX);

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {
				printf("I2C   [Dev:%d - Targ:%d] - Error - RX DMA timeout (2)\n", Dev, Target);
			}				
		  #endif
			if (IsExp == 0) {
				DMAidRX = 0;
			}
		}
	  #endif

		while ((IsExp == 0)							/* Wait for the stop condition					*/
		&&     ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) == 0)
		&&     ((Reg[I2C_RAW_INTR_STAT_REG] & (1U << 9)) == 0)) {	/* Stop condition is bit 9		*/

			IsExp = OS_HAS_TIMEDOUT(Expiry);		/* Check if has timed out						*/

		  #if ((MY_I2C_DEBUG) > 0)
			if (IsExp != 0) {
				printf("I2C   [Dev:%d - Targ:%d] - Error - Timeout on stop condition\n",
				       Dev, Target);
			}
		  #endif
		}

	    g_Dummy = Reg[I2C_CLR_STOP_DET_REG];		/* To clear the stop condition, read register	*/

	  #if (((MY_I2C_OPERATION) & 0x00404) != 0) 
		if (DMAidRX != 0) {
			dma_kill(DMAidRX);						/* Make sure the channel is released			*/
		}
		if (DMAidTX1 != 0) {
			dma_kill(DMAidTX1);						/* Make sure the channel is released			*/
		}
		if (DMAidTX2 != 0) {
			dma_kill(DMAidTX2);						/* Make sure the channel is released			*/
		}
	  #endif

		if (IsExp != 0) {							/* There was an error, try to recover from the	*/
			I2C_ADD_PREFIX(i2c_init) (Dev, 0, 0);	/* error to finish the transfer and re-init the	*/
			IsExp = -10;							/* module										*/
		}
		else {
			if ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) != 0) {
			  #if ((MY_I2C_DEBUG) > 0)
				printf("I2C   [Dev:%d - Targ:%d] - Error - Transfer error [stat reg: 0x%03X]\n",
				       Dev, Target, (unsigned int)(0xFFF & Reg[I2C_RAW_INTR_STAT_REG]));
			  #endif
				IsExp = -11;
			}
		}

	  #if ((MY_I2C_DEBUG) > 0)
		if (DbgDump != 0) {
			printf("               TX data remaining    : %u\n", (unsigned)LeftTX);
			printf("               TX dummies remaining : %u\n", (unsigned)LeftDum);
			printf("               RX data remaining    : %u\n", (unsigned)LeftRX);
		}
	  #endif

	} while ((IsExp != 0)
	  &&     (ReTry-- > 0));

	i2c_disable(Cfg);								/* Disable the controller						*/

	I2C_MTX_UNLOCK(Cfg);

	return(IsExp);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: i2c_init																				*/
/*																									*/
/* i2c_init - initialize a I2C controller in master mode											*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int i2c_init(int Dev, int AddBits, int Freq);												*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev     : controller number																	*/
/*		AddBits : number of bits for the addressing (7 or 10)										*/
/*		          NOTE : == 10 is 10 bits, anything else is 7 bits									*/
/*		Freq    : desired bus speed specified in Hz													*/
/*		          when ==0, bus stalled recovery using the original Freq & AddBits					*/
/*																									*/
/* RETURN VALUE:																					*/
/*		 0 : success																				*/
/*      -1 : error																					*/
/*																									*/
/* DESCRIPTION:																						*/
/* The I2C standard describes a bus clear operation to recover from a stalled bus. The procedure is	*/
/* to toggle the clock line 9 times and that should make the slave release the bus.					*/
/* The procedure employed here to recover from a stalled bus is not the one indicated in the		*/
/* standard. Instead, what is done is to make sure a stop condition is see when trying to un-stall	*/
/* the bus. This is done by force the data line to 0 when toggling the clock. The when the clock	*/
/* is high, after a 1/2 period, the data line is then left un-driven. If it is at a high level the	*/
/* slave has released the bus and a stop condition was created.										*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int I2C_ADD_PREFIX(i2c_init) (int Dev, int AddBits, int Freq)
{
int       AddBitsBack;								/* Back-up of AddBits							*/
int       BusFreqBack;								/* BackUp of BusFreq							*/
I2Ccfg_t *Cfg;										/* De-reference array for faster access			*/
int       ii;										/* General purpose								*/
int       NsecBitBack;								/* Back-up of NsecBit							*/
volatile uint32_t *Reg;								/* Base pointer to I2C controller registers		*/
int       ReMap;									/* Remapped "Dev" to index in g_I2Ccfg[]		*/
#if ((MY_I2C_USE_MUTEX) != 0)
  int     jj;										/* General purpose								*/
#endif

  #if ((MY_I2C_ARG_CHECK) == 0)						/* Arguments validity not checked				*/
	ReMap = g_DevReMap[Dev];

  #else
	if ((Dev < 0)									/* Dev # must be within range to be valid		*/
	||  (Dev >= (MY_I2C_MAX_DEVICES))) {
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d] - Error - Out of range device\n", Dev);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by I2C_LIST_DEVICE				*/
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d] - Error - Cannot remap device\n", Dev);
	  #endif
		return(-2);
	}

	if (Freq < 0) {									/* Freq == 0 is a re-init request				*/
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d] - Error - Non-positive bus speed\n", Dev);
	  #endif
		return(-3);
	}
  #endif

  #if ((MY_I2C_USE_MUTEX) != 0)
	jj = 0;
	if (g_CfgIsInit == 0) {							/* Use the RTOS global mutex until the I2C		*/
		if (0 != MTXlock(G_OSmutex, -1)) {			/* mutex has been created						*/
			return(-4);								/* Although infinte wait, if deadlock detection	*/
		}											/* is enable, could return !=0 and not lock		*/
		jj = 1;
	}
  #endif

	if (g_CfgIsInit == 0) {							/* First time an init occurs, init globals		*/
		gpio_init(0);								/* Not a forced init							*/
		memset(&g_I2Ccfg[0], 0, sizeof(g_I2Ccfg));
	  #if (((MY_I2C_OPERATION) & 0x00400) != 0)
		for (ii=0 ; ii<(sizeof(g_DumVal)/sizeof(g_DumVal[0])) ; ii++) {
			g_DumVal[ii] = 1U << 8;
		}
		DCacheFlushRange(&g_DumVal[0], sizeof(g_DumVal));
	  #endif
		g_CfgIsInit = 1;
	}

	Cfg = &g_I2Ccfg[ReMap];							/* This is this device configuration			*/

	NsecBitBack = Cfg->NsecBit;
	BusFreqBack = Cfg->BusFreq;
	AddBitsBack = Cfg->AddBits;

  #if (((MY_I2C_OPERATION) & 0x00202) != 0)
	if (Cfg->MySema == (SEM_t *)NULL) {				/* Restore my device semaphore. If was NULL		*/
		Cfg->MySema = SEMopen(&g_Names[Dev][0]);	/* then it doesn't exists, so open it			*/
	}
  #endif

  #if ((MY_I2C_USE_MUTEX) != 0)
	if (Cfg->MyMutex == (MTX_t *)NULL) {			/* Restore my device mutex. If was NULL			*/
		Cfg->MyMutex = MTXopen(&g_Names[Dev][0]);	/* then it doesn't exist, so open it			*/
	}
	if (jj != 0) {									/* Lock it and release the RTOS global if it	*/
		MTXunlock(G_OSmutex);						/* was used for the first init					*/
	}
  #endif

	if (0 != I2C_MTX_LOCK(Cfg)) {					/* Now we need to have exclusive access			*/
		return(-4);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	if (Freq == 0) {								/* Bus clear request							*/
		i2c_bus_clear(Cfg);							/* i2c_bus_clear() checks if is device is init	*/
	}

	Cfg->IsInit = 0;								/* In cass re-init								*/
													/* Find which GPIO mux to use for clear bus		*/
	for (ii=0 ; ii<(sizeof(g_MuxIO[0])/sizeof(g_MuxIO[0][0])) ; ii++) {
		if (g_MuxIO[Dev][ii].Addr== NULL) {			/* When mux register address NULL, there is no	*/
			break;									/* more mux to look at, done with bus clear		*/
		}
		if (g_MuxIO[Dev][ii].ValI2C == *g_MuxIO[Dev][ii].Addr) {
			Cfg->MyMux = &g_MuxIO[Dev][ii];
			break;
		}
	}

	if (Freq != 0) {
		NsecBitBack = 1000000000					/* Nano-second per bit transfered				*/
		             / Freq;
		BusFreqBack = Freq;							/* When using auto-re-init due to timeout		*/
		AddBitsBack = AddBits;						/* 7 or 10 bit addresses						*/
	}
	Cfg->NsecBit = NsecBitBack;
	Cfg->BusFreq = BusFreqBack;
	Cfg->AddBits = AddBitsBack;
	Cfg->Dev     = Dev;								/* Controller number							*/
	Cfg->HW      = g_HWaddr[ReMap];					/* I2C peripheral base address					*/
	Reg          = Cfg->HW;

  #if ((MY_I2C_DEBUG) > 1)
	printf("I2C   [Dev:%d] init with %d Address bits at %dHz\n", Dev,
	        (Cfg->AddBits == 10)?10:7, Cfg->BusFreq);
  #endif

	I2C_RESET_DEV(Dev);								/* HW reset of the module						*/

	i2c_disable(Cfg);								/* First, disable the controller				*/

	Reg[I2C_CON_REG] = (1U << 0)					/* Controller is always the Master				*/
	                 | (2U << 1)					/* FAST speed									*/
	                 | (1U << 4)					/* 10 bit address master						*/
	                 | (1U << 5)					/* Restart enable								*/
	                 | (1U << 6)					/* Slave disable								*/
	                 | ((Cfg->AddBits == 10)
	                   ? (1U << 3)					/* 10 bit address slave							*/
	                   : 0);

	Reg[I2C_INTR_MASK_REG] = 0;						/* Interrupts enable when exchanges triggered	*/

	i2c_set_bus_speed(Cfg, Cfg->BusFreq);
													/* Set the FIFO thresholds						*/
	Reg[I2C_RX_TL_REG] = (3*(MY_I2C_RX_FIFO_SIZE))>>2;	/* RX triggers interrupts when 3/4 full		*/
	Reg[I2C_TX_TL_REG] = (  (MY_I2C_TX_FIFO_SIZE))>>2;	/* TX triggers interrupts when 1/4 full		*/

	ii = ((MY_I2C_CLK)/8)							/* ~25% 1/2 period spike removal				*/
	   / Cfg->BusFreq;

	if (ii < 2) {									/* Minimum value for both registers				*/
		ii = 2;
	}

	if (ii > 64000) {								/* The register can only hold 16 bits			*/
		ii = 64000;
	}
	Reg[I2C_SDA_HOLD_REG] = (unsigned int)ii;

	if (ii > 255) {									/* The register can only hold 8 bits			*/
		ii = 255;
	}
	Reg[I2C_FS_SPKLEN_REG] = (unsigned int)ii;

	g_Dummy = Reg[I2C_CLR_ACTIVITY_REG];			/* Clear out all pending stuff					*/
	g_Dummy = Reg[I2C_CLR_GEN_CALL_REG];
	g_Dummy = Reg[I2C_CLR_INTR_REG];
	g_Dummy = Reg[I2C_CLR_RD_REQ_REG];
	g_Dummy = Reg[I2C_CLR_RX_DONE_REG];
	g_Dummy = Reg[I2C_CLR_RX_OVER_REG];
	g_Dummy = Reg[I2C_CLR_RX_UNDER_REG];
	g_Dummy = Reg[I2C_CLR_START_DET_REG];
	g_Dummy = Reg[I2C_CLR_STOP_DET_REG];
	g_Dummy = Reg[I2C_CLR_TX_ABRT_REG];
	g_Dummy = Reg[I2C_CLR_TX_OVER_REG];

	i2c_enable(Cfg);								/* Enable the controller						*/

  #if ((MY_I2C_DEBUG) > 0)
	if (Freq != 0) {
		putchar('\n');
		printf("I2C  - Device         : #%d\n", Dev);
		printf("I2C  - RX FIFO size   : %2d bytes\n",  MY_I2C_RX_FIFO_SIZE);
		printf("I2C  - RX FIFO thres  : %2d bytes\n", (MY_I2C_RX_WATERMARK)+1);
		printf("I2C  - Min for RX DMA : %2d bytes\n",  MY_I2C_MIN_4_RX_DMA);
		printf("I2C  - Min for RX ISR : %2d bytes\n",  MY_I2C_MIN_4_RX_ISR);
		printf("I2C  - TX FIFO size   : %2d bytes\n",  MY_I2C_TX_FIFO_SIZE);
		printf("I2C  - TX FIFO thres  : %2d bytes\n",  MY_I2C_TX_WATERMARK);
		printf("I2C  - Min for TX DMA : %2d bytes\n",  MY_I2C_MIN_4_TX_DMA);
		printf("I2C  - Min for TX ISR : %2d bytes\n",  MY_I2C_MIN_4_TX_ISR);
		if ((Reg[I2C_SS_SCL_HCNT_REG] == 65525)
		||  (Reg[I2C_SS_SCL_LCNT_REG] == 65525)) {
			printf("I2C  - I2C clk        : Requested Hz is lower than controller capabilities\n");
			printf("                       Using lowest possible\n");
		}
		printf("I2C  - CTRL clock     : %9d Hz\n", MY_I2C_CLK);
		if ((Reg[I2C_ENABLE_REG] & (3U<<1)) == 1) {
			printf("I2C  - I2C clk        : %9d Hz [/%d /%d]\n",
			           (int)((MY_I2C_CLK)/(Reg[I2C_SS_SCL_HCNT_REG]+Reg[I2C_SS_SCL_LCNT_REG])),
			               (int)(Reg[I2C_SS_SCL_HCNT_REG]), (int)(Reg[I2C_SS_SCL_LCNT_REG]));
		}
		else {
			printf("I2C  - I2C clk        : %9d Hz [/%d /%d]\n",
			           (int)((MY_I2C_CLK)/(Reg[I2C_FS_SCL_HCNT_REG]+Reg[I2C_FS_SCL_LCNT_REG])),
			               (int)(Reg[I2C_FS_SCL_HCNT_REG]), (int)(Reg[I2C_FS_SCL_LCNT_REG]));
		}
	}
  #endif

	Cfg->IsInit = 1;

	I2C_MTX_UNLOCK(Cfg);

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: uDelay																					*/
/*																									*/
/* uDelay - software delay																			*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void uDelay(int Time);																		*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Time  : requested delay specified in us (will be very approximated)							*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static void uDelay(int Time)
{
int ii;												/* Loop counter									*/

	if (Time == 0) {
		Time = 1;
	}
	Time *= UDELAY_MULT;
	for (ii=0 ; ii<Time ; ii++) {
		g_Dummy++;
	}
	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: i2c_wait_for_bb																		*/
/*																									*/
/* i2c_wait_for_bb - wait until the bus is not busy													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int i2c_wait_for_bb(I2Ccfg_t *Cfg);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Cfg : Device configuration descriptor														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int i2c_wait_for_bb(I2Ccfg_t *Cfg)
{
volatile uint32_t *Reg;								/* Base pointer to I2C controller registers		*/
int ReInit;											/* To re-initialize the controller				*/
int ReTry;											/* To limit the numbers of retries				*/
unsigned int status;								/* Controller status							*/
int TimeOut;										/* Timeout expiry timer tick value				*/
int WasErr;											/* If there was a bus error during the exchange	*/

	ReInit = 0;
	ReTry  = 5;										/* For the busy timeout, we use the time needed	*/
												 	/* to clear the TX FIFO							*/
													/* Timeout: use *8 for margin of error			*/
	TimeOut = ((MY_I2C_RX_FIFO_SIZE) * 9 * Cfg->NsecBit)	/* Timeout waiting for the ISR done		*/
	        >> 17;		         					/* Doing /(128*1024) instead of *8/(1000*1000)	*/
	TimeOut = OS_MS_TO_MIN_TICK(TimeOut, 3);		/* Convert ms to RTOS timer tick count			*/

	Reg  = Cfg->HW;
	WasErr = 0;
	do {											/* Wait for empty TX FIFO and Idle state 		*/
		status = Reg[I2C_STATUS_REG];				/* Grab current status of the controller		*/
		if ((status & ((1U<<2)|(1U<<5))) == (1U<<2)) {
			break;
		}
		if (++ReInit > 3) {							/* We don't want to re-init over and over as	*/
			ReInit = 0;								/* we are waiting for the controller to not be	*/
			I2C_ADD_PREFIX(i2c_init) (Cfg->Dev, 0, 0);	/* busy										*/
		}
		TSKsleep(TimeOut);							/* Still busy, wait for a while					*/
	} while(ReTry-- > 0);							/* Stop when too many retries					*/

	if (ReTry == 0) {								/* Check if tries too many times				*/
		I2C_ADD_PREFIX(i2c_init) (Cfg->Dev, 0, 0);	/* Re-init the controller, hoping for the best	*/
		WasErr = -1;								/* next time									*/
	  #if ((MY_I2C_DEBUG) > 0)
		puts("I2C  - Error - Timeout on bus busy");
	  #endif
	}

	return(WasErr);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: i2c_bus_clear																			*/
/*																									*/
/* i2c_bus_clear - perform the bus_clear CLK & DTA signal toggling									*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void i2c_bus_clear(I2Ccfg_t *Cfg);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Cfg : Device configuration descriptor														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static void i2c_bus_clear(I2Ccfg_t *Cfg)
{
int Dev;
int ii;

	if (Cfg->IsInit != 0) {							/* Can't do bus clear if device not initialized	*/
	  if ((Cfg->MyMux->Addr != NULL)				/* Bus error: Designware does not support the	*/
	  &&  (((MY_I2C_OPERATION) & 0x10000) != 0)) {	/* the bus clear in standard. Do it manually	*/

		Dev = Cfg->Dev;

		IO_IN(PIN_CLK(Cfg));
		IO_IN(PIN_DTA(Cfg));
		SEL_IO_CLK(Cfg);							/* Bring GPIO in input to I2C CLK and DTA		*/
		SEL_IO_DTA(Cfg);							/* This will allow us to see the state of the	*/
		uDelay(10);									/* I2C bus										*/

		ii = IO_READ(PIN_CLK(Cfg));					/* Can only recover of the CLK line is high		*/
		if (ii != 0) {								/* Clock line is HI, so try to recover			*/
			IO_HI(PIN_CLK(Cfg));					/* Set the CLK level to output high				*/
			IO_OUT(PIN_CLK(Cfg));
			uDelay(10);
			for(ii=0 ; ii<128 ; ii++) {
				IO_LO(PIN_DTA(Cfg));				/* Set the CLK level to output low				*/
				IO_OUT(PIN_DTA(Cfg));
				IO_LO(PIN_CLK(Cfg));				/* Lower the CLK line							*/
				uDelay(Cfg->NsecBit>>11);			/* Wait 1/2 a period (/1000*2)					*/
				IO_HI(PIN_CLK(Cfg));				/* Raise the CLK line							*/
				uDelay(Cfg->NsecBit>>11);			/* Wait 1/2 a period (/1000*2)					*/
				IO_IN(PIN_DTA(Cfg));				/* Put the DTA line in input					*/
				uDelay(100);						/* If the DTA line is high, then the staller	*/
				if (IO_READ(PIN_DTA(Cfg)) != 0) {	/* has released the line and this sequence		*/
					uDelay(Cfg->NsecBit>>11);		/* has created a stop condition. The device 	*/
					break;							/* should now abort all I2C operations			*/
				}
			}
		}
		I2C_RESET_DEV(Dev);							/* Reset the I2C module							*/

		SEL_I2C_DTA(Cfg);							/* Make sure the I/O pins are I2C				*/
		SEL_I2C_CLK(Cfg);							/* OS, bring the I2C lines to the I2C module	*/
		uDelay(10000);
	  }
	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: i2c_disable																			*/
/*																									*/
/* i2c_disable - disable a I2C controller															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void i2c_disable(I2Ccfg_t *Cfg);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Cfg : Device configuration descriptor														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static void i2c_disable(I2Ccfg_t *Cfg)
{
unsigned int enbl;
int ii;												/* Loop counter									*/
volatile uint32_t *Reg;								/* Base pointer to I2C controller registers		*/

	Reg  = Cfg->HW;
	enbl = Reg[I2C_ENABLE_REG]
	     & ~(1U << 0);
	for (ii=0 ; ii<10 ; ii++) {
		Reg[I2C_ENABLE_REG] = enbl;
		if ((Reg[I2C_ENABLE_STATUS_REG] & (1U << 0)) == 0U) {
			break;
		}
		uDelay(100);
	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: i2c_enable																				*/
/*																									*/
/* i2c_enable - enable a I2C controller																*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void i2c_enable(I2Ccfg_t *Cfg);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Cfg : Device configuration descriptor														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static void i2c_enable(I2Ccfg_t *Cfg)
{
unsigned int enbl;
int ii;												/* Loop counter									*/
volatile uint32_t *Reg;								/* Base pointer to I2C controller registers		*/

	Reg  = Cfg->HW;
	enbl = Reg[I2C_ENABLE_REG]
	     | (1U << 0);
	for (ii=0 ; ii<10 ; ii++) {
		Reg[I2C_ENABLE_REG] = enbl;
		if ((Reg[I2C_ENABLE_STATUS_REG] & (1U << 0)) != 0U) {
			break;
		}
		uDelay(100);
	}


	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: i2c_set_bus_speed																		*/
/*																									*/
/* i2c_set_bus_speed - set the I2C bus speed														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void i2c_set_bus_speed(I2Ccfg_t *Cfg, int speed);											*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Cfg : Device configuration descriptor														*/
/*		Freq  : desired bus speed specified in Hz													*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#define CNT_DEN	1000000
#define CNT_NUM ((MY_I2C_CLK)/1000)

void i2c_set_bus_speed(I2Ccfg_t *Cfg, int speed)
{
unsigned int cntl;									/* Controller configuration						*/
unsigned int enbl;									/* Current controller enable/disable			*/
unsigned int orig;									/* Original state of the controller				*/
int Period;
volatile uint32_t *Reg;								/* Base pointer to I2C controller registers		*/
int Thi;
int Tlo;
int Tf;

	Reg  = Cfg->HW;

	enbl                      = Reg[I2C_ENABLE_REG];	/* 1st, disable the I2C controller			*/
	orig                      = (enbl & (1U << 0));
	enbl                     &= ~(1U << 0);
	Reg[I2C_ENABLE_REG] = enbl;
    cntl                      = Reg[I2C_CON_REG]
                              & (~(((1U<<2)-1U) << 1));

	Period  = 1000000000/speed;						/* Requested speed period in ns					*/
	Tf      = 300;									/* Fast & standard mode fall time (very safe)	*/

	Tlo     = Period/2;								/* -------------------------------------------- */
	if (Tlo < 1300) { 								/* Fast mode minimum low duration: 1300 ns		*/
		Tlo = 1300;
	}
	Thi     = Period-Tlo;
	if (Thi < 600){ 								/* Fast mode minimum high duration: 600 ns		*/
		Thi = 600;
	}
	Reg[I2C_FS_SCL_HCNT_REG] = (unsigned int)(((CNT_NUM*(Thi + Tf))+(CNT_DEN/2)) / CNT_DEN) - 3;
	Reg[I2C_FS_SCL_LCNT_REG] = (unsigned int)(((CNT_NUM*(Tlo + Tf))+(CNT_DEN/2)) / CNT_DEN) - 1;

	Tlo     = Period/2;								/* -------------------------------------------- */
	if (Tlo < 4700){ 								/* Standard mode minimum low duration: 4700 ns	*/
		Tlo = 4700;
	}
	Thi     = Period-Tlo;
	if (Thi < 4000) { 								/* Standard mode minimum high duration: 4000 ns	*/
		Thi = 4000;
	}
	Reg[I2C_SS_SCL_HCNT_REG] = (unsigned int)(((CNT_NUM*(Thi + Tf))+(CNT_DEN/2)) / CNT_DEN) - 3;
	Reg[I2C_SS_SCL_LCNT_REG] = (unsigned int)(((CNT_NUM*(Tlo + Tf))+(CNT_DEN/2)) / CNT_DEN) - 1;

													/* -------------------------------------------- */
	cntl |= (speed > I2C_STANDARD_SPEED)			/* Select the speed range						*/
	      ? (2U << 1)								/* FAST speed									*/
	      : (1U << 1);								/* Standard speed								*/

    Reg[I2C_CON_REG]    = cntl;						/* Controller is always the Master				*/
	enbl               |= orig;						/* Put the controller back to original state	*/
	Reg[I2C_ENABLE_REG] = enbl;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: i2c_test																				*/
/*																									*/
/* i2c_test -test the I2C CLK & DTA lines															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void i2c_test(int Dev);																		*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : controller number																	*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*																									*/
/* DESCRIPTION:																						*/
/* The I2C pins (CLK & DTA) are switched from the I2C controller to the GPIO and these are driven	*/
/* with an out of phase square waves for 1 minute.													*/
/* Once the test is over, the I2C controller is connected back to the pins.							*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void i2c_test(int Dev)
{
int ReMap;											/* Remapped "Dev" to index in g_I2Ccfg[]		*/
int Time;
I2Ccfg_t *Cfg;										/* De-reference array for faster access			*/

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	Cfg   = &g_I2Ccfg[ReMap];

	if (Cfg->MyMux->Addr != NULL) {					/* Make sure GPIO has been associated			*/
		Time = G_OStimCnt
		     + (OS_TICK_PER_SEC*60);

		IO_OUT(PIN_CLK(Cfg));
		IO_OUT(PIN_DTA(Cfg));
		SEL_IO_CLK(Cfg);
		SEL_IO_DTA(Cfg);
		uDelay(10);

		while ((G_OStimCnt-Time) < 0) {
			IO_HI(PIN_CLK(Cfg));					/* Raise the CLK line							*/
			IO_LO(PIN_DTA(Cfg));					/* Lower the DTA line							*/
			uDelay(Cfg->NsecBit>>11);				/* Wait 1/2 a period (/1000*2)					*/

			IO_LO(PIN_CLK(Cfg));					/* Lower the CLK line							*/
			IO_HI(PIN_DTA(Cfg));					/* Raise the DTA line							*/
			uDelay(Cfg->NsecBit>>11);				/* Wait 1/2 a period (/1000*2)					*/
		}

		IO_IN(PIN_DTA(Cfg));
		IO_IN(PIN_CLK(Cfg));

		SEL_I2C_DTA(Cfg);							/* Make sure the I/O pins are I2C				*/
		SEL_I2C_CLK(Cfg);							/* OS, bring the I2C lines to the I2C module	*/

		uDelay(10000);
	}
	else {											/* No GPIOs associated with the I2C				*/
	  #if ((MY_I2C_DEBUG) > 0)
		printf("I2C   [Dev:%d] - Error - Device has no associated GPIOs\n", Dev);
		printf("                         Skipping the test\n");
	  #endif
	}
	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Interrupt handlers for the I2C transfers															*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static void I2CintHndl(int Dev)
{
#if (((MY_I2C_OPERATION) & 0x00202) != 0)
 char     *BufRX;
 char     *BufTX;
 I2Ccfg_t *Cfg;
 int       ii;
 int       jj;
 int       LeftDum;
 int       LeftRX;
 int       LeftTX;
 volatile uint32_t *Reg;
 int       ReMap;

	ReMap   =  g_DevReMap[Dev];
	Cfg     = &g_I2Ccfg[ReMap];
	Reg     = Cfg->HW;

  #if ((MY_I2C_MULTICORE_ISR) != 0)					/* If multiple cores handle the ISR				*/

   #if ((OX_NESTED_INTS) != 0)						/* Get exclusive access first					*/
	ReMap = OSintOff();								/* and then set the OnGoing flag to inform the	*/
   #endif											/* other cores to skip as I'm handlking the ISR	*/
	CORElock(I2C_SPINLOCK, &Cfg->SpinLock, 0, COREgetID()+1);

	ii = Cfg->ISRonGoing;							/* Memo if is current being handled				*/
	if (ii == 0) {
		Cfg->ISRonGoing = 1;						/* Declare as being handle / will be handle		*/
	}

	COREunlock(I2C_SPINLOCK, &Cfg->SpinLock, 0);
  #if ((OX_NESTED_INTS) != 0)
	OSintBack(ReMap);
  #endif

	if (ii != 0) {									/* Another core was handling it					*/
		return;
	}

  #endif

	BufRX   = (char *)Cfg->ISRbufRX;
	BufTX   = (char *)Cfg->ISRbufTX;
	LeftDum = Cfg->ISRleftDum;
	LeftRX  = Cfg->ISRleftRX;
	LeftTX  = Cfg->ISRleftTX;

	while ((LeftTX > 0)								/* If data left to write and room in the FIFO	*/
	&&     (Reg[I2C_TXFLR_REG] != (MY_I2C_TX_FIFO_SIZE))) {
		if ((--LeftTX+LeftDum) == 0) {				/* If all data sent (case with no dummies		*/
			Reg[I2C_DATA_CMD_REG]   = (0xFF & (unsigned int)(*BufTX++))
		                            | (1U << 9);	/* Stop on last byte							*/
			Reg[I2C_INTR_MASK_REG] &= ~(1U << 4);	/* All sent, disable the TX ISR					*/
		}											/* Keep the stop condition ISR					*/
		else {
			Reg[I2C_DATA_CMD_REG] = 0xFF & (unsigned int)(*BufTX++);
		}
	}

	while ((LeftDum > 0)							/* If dummy left to write and room in the FIFO	*/
	&&     (LeftTX <= 0)
	&&     (Reg[I2C_TXFLR_REG] != (MY_I2C_TX_FIFO_SIZE))) {
		if (--LeftDum == 0) {						/* If wrote all dummies							*/
			Reg[I2C_DATA_CMD_REG]   = (1U << 8)		/* Data command read							*/
		                            | (1U << 9);	/* Stop on last byte							*/
			Reg[I2C_INTR_MASK_REG] &= ~(1U << 4);	/* All sent, disable the TX ISR					*/
		}											/* Keep the stop condition ISR					*/
		else {
			Reg[I2C_DATA_CMD_REG] = (1U << 8);		/* Data command read							*/
		}
	}

	if ((LeftRX > 0)								/* Bytes still needed to be received			*/
	&&  (Reg[I2C_RXFLR_REG] != 0U)) {				/* Non-zero number of bytes in RX FIFO			*/
	
		ii = Reg[I2C_RXFLR_REG];					/* Number of frames in the RX FIFO				*/
		if (ii > LeftRX) {							/* This can happen in i2c_send_recv() when		*/
			ii = LeftRX;							/* not operating in EEPROM mode & RXlen < TXlen	*/
		}

		jj = Reg[I2C_RX_TL_REG]+1;					/* Number of frames that trigger the ISR		*/
		if (((LeftRX-ii) > 0)						/* Need to make sure the last transfer length	*/
		&&  ((LeftRX-ii) < jj)) {					/* is > Watermark as no interrupt issued as		*/
			ii = LeftRX - jj;						/* as RX FIFO contents <= Watermark				*/
		}

		LeftRX -= ii;
		for ( ; ii>0 ; ii--) {
			*BufRX++ = (char)Reg[I2C_DATA_CMD_REG];	/* Grab the new byte							*/
		}
		if (LeftRX == 0) {							/* If got all the RX data, disable RX ISR		*/
			Reg[I2C_INTR_MASK_REG] &= ~(1U << 2);
		}
	}


	if (((LeftRX+LeftTX+LeftDum) == 0)				/* If everything has been transferred			*/
	||  ((Reg[I2C_RAW_INTR_STAT_REG] & 0x4B) != 0)) {	/* or there was an transfer error			*/
		Reg[I2C_INTR_MASK_REG] = 0;
		SEMpost(Cfg->MySema);
	}

	Cfg->ISRbufRX   = BufRX;
	Cfg->ISRbufTX   = BufTX;
	Cfg->ISRleftDum = LeftDum;
	Cfg->ISRleftRX  = LeftRX;
	Cfg->ISRleftTX  = LeftTX;

  #if ((MY_I2C_MULTICORE_ISR) != 0)
	Cfg->ISRonGoing = 0;
  #endif

#else
													/* ISRs not used, disable them					*/
	g_I2Ccfg[g_DevReMap[Dev]].HW[I2C_INTR_MASK_REG] = 0;

#endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Individual interrupt handlers macro. It calls the common handler with the device #				*/

  #define  I2C_INT_HNDL(Prefix, Device) _I2C_INT_HNDL(Prefix, Device)
  #define _I2C_INT_HNDL(Prefix, Device)																\
	void Prefix##I2CintHndl_##Device(void)															\
	{																								\
		I2CintHndl(Device);																			\
		return;																						\
	}

/* ------------------------------------------------------------------------------------------------ */

#if (((MY_I2C_LIST_DEVICE) & 1U) != 0U)
  I2C_INT_HNDL(MY_I2C_PREFIX, 0)
#endif
#if (((MY_I2C_LIST_DEVICE) & 2U) != 0U)
  I2C_INT_HNDL(MY_I2C_PREFIX, 1)
#endif
#if (((MY_I2C_LIST_DEVICE) & 4U) != 0U)
  I2C_INT_HNDL(MY_I2C_PREFIX, 2)
#endif
#if (((MY_I2C_LIST_DEVICE) & 8U) != 0U)
  I2C_INT_HNDL(MY_I2C_PREFIX, 3)
#endif
#if (((MY_I2C_LIST_DEVICE) & 16U) != 0U)
  I2C_INT_HNDL(MY_I2C_PREFIX, 4)
#endif
#if (((MY_I2C_LIST_DEVICE) & 32U) != 0U)
  I2C_INT_HNDL(MY_I2C_PREFIX, 5)
#endif
#if (((MY_I2C_LIST_DEVICE) & 64U) != 0U)
  I2C_INT_HNDL(MY_I2C_PREFIX, 6)
#endif
#if (((MY_I2C_LIST_DEVICE) & 128U) != 0U)
  I2C_INT_HNDL(MY_I2C_PREFIX, 7)
#endif
#if (((MY_I2C_LIST_DEVICE) & 256U) != 0U)
  I2C_INT_HNDL(MY_I2C_PREFIX, 8)
#endif
#if (((MY_I2C_LIST_DEVICE) & 512U) != 0U)
  I2C_INT_HNDL(MY_I2C_PREFIX, 9)
#endif

/* EOF */
