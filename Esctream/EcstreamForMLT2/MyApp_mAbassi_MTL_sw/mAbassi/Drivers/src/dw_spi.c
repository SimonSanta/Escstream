/* ------------------------------------------------------------------------------------------------ */
/* FILE :		dw_spi.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the Designware SPI														*/
/*				IMPORTANT: The interrupt controller MUST be set to level triggering when using		*/
/*				           the interrupts.															*/
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
/*	$Revision: 1.25 $																				*/
/*	$Date: 2019/01/10 18:07:06 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* NOTES:																							*/
/*																									*/
/* On Cyclone V, Arria V & Arria 10 the following device numbers remap to:							*/
/*			Device == 0: SPI master controller #0													*/
/*			Device == 1: SPI master controller #1													*/
/*			Device == 2: SPI slave  controller #0													*/
/*			Device == 3: SPI slave  controller #1													*/
/*																									*/
/* Some info on how things are done:																*/
/*																									*/
/*		spi_recv()																					*/
/*			EOT is declared when the last frame has been read from the RX FIFO.						*/
/*			- Polling xfers: if SPI_EOT_ISR has been specified and the number of frames to read		*/
/*			  is less or equal to the size of the RX FIFO, driver will then block on a semaphore	*/
/*			  that will be posted in the ISR when all the frames have been received. The RX FIFO	*/
/*			  is not read in the ISR: it is emptied after the semaphore has been posted and the		*/
/*			  task has resumed. Otherwise if SPI_EOT_POLLING has been specified or if neither		*/
/*			  SPI_EOT_POLLING or SPI_EOT_ISR have been specified, the end of transfer will be		*/
/*			  declared once the last frame has been read; the driver will not block in this case as	*/
/*			  it continuously polled to read all frames.											*/
/*			  The case when polling xfers are used and the number of frames to read is greater than	*/
/*			  the RX FIFO size could have been handle with SPI_EOT_ISR by reading enough frames		*/
/*			  to be left with RX FIFO size of them and then proceed as described above. This wasn't	*/
/*			  implemented so the sake of keeping the code simple and if there 						*/
/*			- ISR xfers EOT will happen when the ISR has read the last frame from RX FIFO, so it	*/
/*			  doesn't matter if SPI_EOT_POLLING or SPI_EOT_ISR has been specified. The waiting is 	*/
/*			  done by blocking the task on a semaphore which is posted in the SPI ISR.				*/
/*			- DMA xfers EOT will happen when the DMA has read the last frame.  The EOT is waited	*/
/*			  by blocking on the DMA internal semaphore posted in the DMA ISR if SPI_EOT_ISR has	*/
/*			  been specified and if the DMA_OPERATION enables the interrupts.  Otherwise the 		*/
/*			  waiting is done in the DMA driver by polling the state of the DMA until it is 		*/
/*			  declared idle.																		*/
/*																									*/
/*		spi_send()																					*/
/*			EOT cannot be declared when the last frame has been written in the TX FIFO because the	*/
/*			contents of the TX FIFO still needs to be sent on the SPI bus.							*/
/*			For all types of transfers EOT is officially declared when the controller has becomes	*/
/*			inactive.																				*/
/*			For non-DMA transfers, the TX FIFO is always filled as a first step (or all frames		*/
/*			written into if the	number of frames is less than the TX FIFO size). Then polling or	*/
/*			ISR is handle.														 					*/
/*			- Polling Xfers blocks,  when SPI_EOT_ISR is specified, on a semaphore posted by		*/
/*			  the ISR set to activate when the TC FIFO is empty								  		*/
/*			  trigger when the TX FIFO is empty.													*/
/*			- ISR Xfers will block on a semaphore until the the ISR post the semaphore when			*/
/*			  the last frame has been written in the TX FIFO.  Then polling will be used to monitor	*/
/*			  the controller activity.  															*/
/*			- DMA Xfers EOT does not happen when the DMA has written the last frame. If SPI_EOT_ISR	*/
/*			  is specified, the task blocks on a semaphore posted in the ISR when the TX FIFO is	*/
/*			  empty; the driver does not wait for the DMA to finish.  If SPI_EOT_ISR hasn't been	*/
/*			  specified, the DMA will be set to wait to reach the idle state by blocking on its		*/
/*			  internal semaphore. Then once the DMA has gone idle, the driver will wait until the	*/
/*			  SPI controller declares it has reached the Idle state.								*/
/*																									*/
/*		spi_send_recv()																				*/
/*			It kind of combines the way spi_recv() and spi_send() operate.							*/
/*																									*/
/* LIMITATIONS:																						*/
/*			  These are the SPI controller's limitations											*/
/*			  - spi_recv() & spi_send_recv() in non-EEPROM mode can only receive up to 65536 frames	*/
/*			  - The number of bits per frame is restricted between 4 to 16 bits						*/
/*			  - The number of bits for the uWire controller word is restricted between 1 and 16		*/
/*			  - If the DMA is used, packed data cannot be supported if enable.  There are no errors	*/
/*			    or warnings issued when this is set-up.												*/
/*																									*/
/* NOT YET SUPPORTED:																				*/
/*			- Controller operating as a slave (in spi_init(), Mode set with SPI_SLAVE)				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#if defined(_STANDALONE_)
  #include "mAbassi.h"
#elif defined(_UABASSI_)							/* It is the same include file in all cases.	*/
  #include "mAbassi.h"								/* There is a substitution during the release 	*/
#elif defined (OS_N_CORE)							/* This file is the same for Abassi, mAbassi,	*/
  #include "mAbassi.h"								/* and uAbassi, so Code Time uses these checks	*/
#else												/* to not have to keep 3 quasi identical		*/
  #include "mAbassi.h"								/* copies of this file							*/
#endif

#include "dw_spi.h"

#ifndef SPI_USE_MUTEX								/* If a mutex is used to protect accesses		*/
  #define SPI_USE_MUTEX			1					/* == 0 : No mutex       != 0 : Use mutex		*/
#endif
													/* Bit #0->7 are for RX / Bit #8->15 are for TX	*/
#ifndef SPI_OPERATION								/* Bit #0/#8  : Disable ISRs on burst			*/
  #define SPI_OPERATION			0x00101				/* Bit #1/#9  : Interrupts are used				*/
#endif												/* Bit #2/#10 : DMA is used						*/
													/* Bit #16    : Support packed data				*/
#ifndef SPI_ISR_RX_THRS								/* When using RX ISRs, percentage (0->100) of	*/
  #define SPI_ISR_RX_THRS		50					/* the RX FIFO when the ISR is triggered		*/
#endif												/* This is when RX FIFO contents > threshold	*/

#ifndef SPI_ISR_TX_THRS								/* When using TX ISRs, percentage (0->100) of	*/
  #define SPI_ISR_TX_THRS		50					/* the TX FIFO when the ISR is triggered		*/
#endif												/* This is when TX FIFO contents <= threshold	*/

#ifndef SPI_MIN_4_RX_DMA
  #define SPI_MIN_4_RX_DMA		32					/* Minimum number of frames to use the DMA		*/
#endif

#ifndef SPI_MIN_4_TX_DMA
  #define SPI_MIN_4_TX_DMA		32					/* Minimum number of frames to use the DMA		*/
#endif

#ifndef SPI_MIN_4_RX_ISR
  #define SPI_MIN_4_RX_ISR		16					/* Minimum number of frames to use interrupts	*/
#endif												/* It is also the minimum to use the DMA		*/

#ifndef SPI_MIN_4_TX_ISR
  #define SPI_MIN_4_TX_ISR		16					/* Minimum number of frames to use interrupts	*/
#endif												/* It is also the minimum to use the DMA		*/

#ifndef SPI_MULTICORE_ISR
  #define SPI_MULTICORE_ISR		1					/* When operating on a multicore, set to != 0	*/
#endif												/* if more than 1 core process the interrupt	*/

#ifndef SPI_TOUT_ISR_ENB							/* Set to 0 if timeouts not required in polling	*/
  #define SPI_TOUT_ISR_ENB		1					/* When timeout used, if ISRs requested to be	*/
#endif												/* disable, enable-disable to update/read tick	*/

#ifndef SPI_REMAP_LOG_ADDR
  #define SPI_REMAP_LOG_ADDR	1					/* If remapping logical to physical adresses	*/
#endif												/* Only used/needed with DMA transfers			*/

#ifndef SPI_ARG_CHECK								/* If checking validity of function arguments	*/
  #define SPI_ARG_CHECK			1
#endif

#ifndef SPI_DEBUG									/* == 0 no debug information is printed			*/
  #define SPI_DEBUG				0					/* == 1 print only initialization information	*/
#endif												/* >= 2 print all information					*/

/* ------------------------------------------------------------------------------------------------ */
/* These are internal defintions that enable the use of multiple devices type SPI drivers			*/
/* Doing this keep the whole core code of the SPI the same across all types of SPI					*/
/* the SPI generic SPI_??? and/or specific ???_SPI_ tokens are remapped to MY_SPI_??? tokens		*/

#ifdef DW_SPI_USE_MUTEX
  #define MY_SPI_USE_MUTEX		(DW_SPI_USE_MUTEX)
#else
  #define MY_SPI_USE_MUTEX		(SPI_USE_MUTEX)
#endif

#ifdef DW_SPI_OPERATION
  #define MY_SPI_OPERATION		(DW_SPI_OPERATION)
#else
  #define MY_SPI_OPERATION		(SPI_OPERATION)
#endif

#ifdef DW_SPI_ISR_RX_THRS
  #define MY_SPI_ISR_RX_THRS	(DW_SPI_ISR_RX_THRS)
#else
  #define MY_SPI_ISR_RX_THRS	(SPI_ISR_RX_THRS)
#endif

#ifdef DW_SPI_ISR_TX_THRS
  #define MY_SPI_ISR_TX_THRS	(DW_SPI_ISR_TX_THRS)
#else
  #define MY_SPI_ISR_TX_THRS	(SPI_ISR_TX_THRS)
#endif

#ifdef DW_SPI_MIN_4_RX_DMA
  #define MY_SPI_MIN_4_RX_DMA	(DW_SPI_MIN_4_RX_DMA)
#else
  #define MY_SPI_MIN_4_RX_DMA	(SPI_MIN_4_RX_DMA)
#endif

#ifdef DW_SPI_MIN_4_TX_DMA
  #define MY_SPI_MIN_4_TX_DMA	(DW_SPI_MIN_4_TX_DMA)
#else
  #define MY_SPI_MIN_4_TX_DMA	(SPI_MIN_4_TX_DMA)
#endif

#ifdef DW_SPI_MIN_4_RX_ISR
  #define MY_SPI_MIN_4_RX_ISR	(DW_SPI_MIN_4_RX_ISR)
#else
  #define MY_SPI_MIN_4_RX_ISR	(SPI_MIN_4_RX_ISR)
#endif

#ifdef DW_SPI_MIN_4_TX_ISR
  #define MY_SPI_MIN_4_TX_ISR	(DW_SPI_MIN_4_TX_ISR)
#else
  #define MY_SPI_MIN_4_TX_ISR	(SPI_MIN_4_TX_ISR)
#endif

#if ((OX_N_CORE) > 1)
 #ifdef DW_SPI_MULTICORE_ISR
  #define MY_SPI_MULTICORE_ISR	(DW_SPI_MULTICORE_ISR)
 #else
  #define MY_SPI_MULTICORE_ISR	(SPI_MULTICORE_ISR)
 #endif
#else
  #define MY_SPI_MULTICORE_ISR	0
#endif
#ifdef DW_SPI_TOUT_ISR_ENB
  #define MY_SPI_TOUT_ISR_ENB	(DW_SPI_TOUT_ISR_ENB)
#else
  #define MY_SPI_TOUT_ISR_ENB	(SPI_TOUT_ISR_ENB)
#endif

#ifdef DW_SPI_REMAP_LOG_ADDR
  #define MY_SPI_REMAP_LOG_ADDR	(DW_SPI_REMAP_LOG_ADDR)
#else
  #define MY_SPI_REMAP_LOG_ADDR	(SPI_REMAP_LOG_ADDR)
#endif

#ifdef DW_SPI_ARG_CHECK
  #define MY_SPI_ARG_CHECK		(DW_SPI_ARG_CHECK)
#else
  #define MY_SPI_ARG_CHECK		(SPI_ARG_CHECK)
#endif

#ifdef DW_SPI_DEBUG
  #define MY_SPI_DEBUG			(DW_SPI_DEBUG)
#else
  #define MY_SPI_DEBUG			(SPI_DEBUG)
#endif

#ifdef DW_SPI_MAX_SLAVES
  #define MY_SPI_MAX_SLAVES		(DW_SPI_MAX_SLAVES)
#elif defined(SPI_MAX_SLAVES)
  #define MY_SPI_MAX_SLAVES		(SPI_MAX_SLAVES)
#endif

#ifdef DW_SPI_CLK
  #define MY_SPI_CLK			(DW_SPI_CLK)
#elif defined(SPI_CLK)
  #define MY_SPI_CLK			(SPI_CLK)
#endif

#define MY_SPI_MAX_DEVICES		(DW_SPI_MAX_DEVICES)
#define MY_SPI_LIST_DEVICE		(DW_SPI_LIST_DEVICE)

#define MY_SPI_PREFIX			DW_SPI_PREFIX
#define MY_SPI_ASCII			DW_SPI_ASCII

#undef  SPI_ADD_PREFIX
#define SPI_ADD_PREFIX			DW_SPI_ADD_PREFIX

/* ------------------------------------------------------------------------------------------------ */
/* Base addresses of the HW SPI modules																*/
/*																									*/
/* NOTE:																							*/
/*		Arria V, Arria10,  Cyclone V:																*/
/*		The FIFO sizes are set to 255 instead of 256 because the registers reporting the number		*/
/*		of frames in the FIFOs (TXFTLR & RXFTLR) are 8 bits, therefore can only report up to 255	*/
/*		By using 255 at least the number of FIFO entry used can be correctly evaluated and			*/

#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)		/* Arria V / Cyclone V							*/
  static volatile uint32_t * const g_HWaddr[4]  =  {(volatile uint32_t *)0xFFF00000,
                                                    (volatile uint32_t *)0xFFF01000,
                                                    (volatile uint32_t *)0xFFF02000,
                                                    (volatile uint32_t *)0xFFF03000
                                                  };

  #define SPI_RESET_DEV(Dev)	do {																\
									*((volatile uint32_t *)0xFFD05014) |=  (1U << ((Dev)+18));		\
									*((volatile uint32_t *)0xFFD05014) &= ~(1U << ((Dev)+18));		\
								} while(0)

 #if ((MY_SPI_ARG_CHECK) != 0)
  static const uint8_t g_IsMaster[4] = { 1, 1, 0, 0};
 #endif

 #if (((MY_SPI_OPERATION) & 0x00404) != 0)
  #include "arm_pl330.h"

  #define DMA_DEV				0					/* Only one DMA on Arria V & Cyclone V			*/
  static int g_DMAchan[][2] = {{16, 17},			/* SPI #0 (master #0) DMA: TX chan, RX chan		*/
                               {20, 21},			/* SPI #1 (master #1) DMA: TX chan, RX chan		*/
                               {18, 19},			/* SPI #2 (slave #0)  DMA: TX chan, RX chan		*/
                               {22, 23}				/* SPI #3 (slave #1)  DMA: TX chan, RX chan		*/
                              };
 #endif

 #ifndef MY_SPI_CLK
  #define MY_SPI_CLK			200000000
 #endif

  #define MY_SPI_MASTER_IDX		0					/* Index of the first master controller			*/
  #define MY_SPI_SLAVE_IDX		2					/* Index of the first slave controller			*/
  #define MY_SPI_RX_FIFO_SIZE	255					/* Size of the RX FIFO							*/
  #define MY_SPI_TX_FIFO_SIZE	255					/* Size of the TX FIFO							*/

  #ifndef MY_SPI_MAX_SLAVES
	#define MY_SPI_MAX_SLAVES	4
  #endif

  #if ((MY_SPI_MAX_DEVICES) > 4)
    #error "Too many SPI devices: set MY_SPI_MAX_DEVICES <= 4 and/or SPI_LIST_DEVICE <= 0xF"
  #endif

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)	/* Arria 10										*/
  static volatile uint32_t * const g_HWaddr[4]  =  {(volatile uint32_t *)0xFFDA4000,
                                                    (volatile uint32_t *)0xFFDA5000,
                                                    (volatile uint32_t *)0xFFDA2000,
                                                    (volatile uint32_t *)0xFFDA3000
                                                   };

  #define SPI_RESET_DEV(Dev)	do {																\
									*((volatile uint32_t *)0xFFD05024) |=  (1U << ((Dev)+17));		\
									*((volatile uint32_t *)0xFFD05024) &= ~(1U << ((Dev)+17));		\
								} while(0)

 #if ((MY_SPI_ARG_CHECK) != 0)
  static const uint8_t g_IsMaster[4] = { 1, 1, 0, 0};
 #endif

 #if (((MY_SPI_OPERATION) & 0x00404) != 0)
  #include "arm_pl330.h"

  #define DMA_DEV				0					/* Only one DMA on Arria 10						*/
  static int g_DMAchan[][2] = {{16, 17},			/* SPI #0 (master #0) DMA: TX chan, RX chan		*/
                               {20, 21},			/* SPI #1 (master #1) DMA: TX chan, RX chan		*/
                               {18, 19},			/* SPI #2 (slave #0)  DMA: TX chan, RX chan		*/
                               {22, 23}				/* SPI #3 (slave #1)  DMA: TX chan, RX chan		*/
                              };
 #endif

 #ifndef MY_SPI_CLK
  #define MY_SPI_CLK			200000000
 #endif

  #define MY_SPI_MASTER_IDX		0					/* Index of the first master controller			*/
  #define MY_SPI_SLAVE_IDX		2					/* Index of the first slave controller			*/
  #define MY_SPI_RX_FIFO_SIZE	255					/* Size of the RX FIFO							*/
  #define MY_SPI_TX_FIFO_SIZE	255					/* Size of the TX FIFO							*/

  #ifndef MY_SPI_MAX_SLAVES
	#define MY_SPI_MAX_SLAVES	4
  #endif

  #if ((MY_SPI_MAX_DEVICES) > 4)
    #error "Too many SPI devices: set MY_SPI_MAX_DEVICES <= 4 and/or SPI_LIST_DEVICE <= 0xF"
  #endif

#else
  	#error "Unsupported platform specified by OS_PLATFORM"

#endif

/* ------------------------------------------------------------------------------------------------ */
/* Pre-compute the FIFO thresholds																	*/

#define SPI_RX_WATERMARK		((((MY_SPI_RX_FIFO_SIZE)) * MY_SPI_ISR_RX_THRS)/100)
#define SPI_TX_WATERMARK		((((MY_SPI_TX_FIFO_SIZE)) * MY_SPI_ISR_TX_THRS)/100)

/* ------------------------------------------------------------------------------------------------ */
/* This section uses a lots of macros but it's to reduce the size of the data requirements by only	*/
/* creating descriptor / mutexes etc for the devices in use ("1" in SPI_LIST_DEVICE)				*/

#ifndef MY_SPI_MAX_DEVICES
 #if   (((MY_SPI_LIST_DEVICE) & 0x200) != 0U)
  #define MY_SPI_MAX_DEVICES	10
 #elif (((MY_SPI_LIST_DEVICE) & 0x100) != 0U)
  #define MY_SPI_MAX_DEVICES	9
 #elif (((MY_SPI_LIST_DEVICE) & 0x080) != 0U)
  #define MY_SPI_MAX_DEVICES	8
 #elif (((MY_SPI_LIST_DEVICE) & 0x040) != 0U)
  #define MY_SPI_MAX_DEVICES	7
 #elif (((MY_SPI_LIST_DEVICE) & 0x020) != 0U)
  #define MY_SPI_MAX_DEVICES	6
 #elif (((MY_SPI_LIST_DEVICE) & 0x010) != 0U)
  #define MY_SPI_MAX_DEVICES	5
 #elif (((MY_SPI_LIST_DEVICE) & 0x008) != 0U)
  #define MY_SPI_MAX_DEVICES	4
 #elif (((MY_SPI_LIST_DEVICE) & 0x004) != 0U)
  #define MY_SPI_MAX_DEVICES	3
 #elif (((MY_SPI_LIST_DEVICE) & 0x002) != 0U)
  #define MY_SPI_MAX_DEVICES	2
 #elif (((MY_SPI_LIST_DEVICE) & 0x001) != 0U)
  #define MY_SPI_MAX_DEVICES	1
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* This section figures out how to remap the individual devices specified by SPI_LIST_DEVICE		*/
/* e.g. if SPI_LIST_DEVICE == 0x12, the only descriptors/mutexes for 2 devices are required and		*/
/*      device#1 is remapped to [0] and device #4 is remapped to [1]								*/

#if (((MY_SPI_LIST_DEVICE) & 0x001) != 0)
  #define SPI_CNT_0	0
  #define SPI_IDX_0	0
#else
  #define SPI_CNT_0	(-1)
  #define SPI_IDX_0	(-1)
#endif
#if (((MY_SPI_LIST_DEVICE) & 0x002) != 0)
  #define SPI_CNT_1	((SPI_CNT_0) + 1)
  #define SPI_IDX_1	((SPI_CNT_0) + 1)
#else
  #define SPI_CNT_1	(SPI_CNT_0)
  #define SPI_IDX_1	-1
#endif
#if (((MY_SPI_LIST_DEVICE) & 0x004) != 0)
  #define SPI_CNT_2	((SPI_CNT_1) + 1)
  #define SPI_IDX_2	((SPI_CNT_1) + 1)
#else
  #define SPI_CNT_2	(SPI_CNT_1)
  #define SPI_IDX_2	-1
#endif
#if (((MY_SPI_LIST_DEVICE) & 0x008) != 0)
  #define SPI_CNT_3	((SPI_CNT_2) + 1)
  #define SPI_IDX_3	((SPI_CNT_2) + 1)
#else
  #define SPI_CNT_3	(SPI_CNT_2)
  #define SPI_IDX_3	-1
#endif
#if (((MY_SPI_LIST_DEVICE) & 0x010) != 0)
  #define SPI_CNT_4	((SPI_CNT_3) + 1)
  #define SPI_IDX_4	((SPI_CNT_3) + 1)
#else
  #define SPI_CNT_4	(SPI_CNT_3)
  #define SPI_IDX_4	-1
#endif
#if (((MY_SPI_LIST_DEVICE) & 0x020) != 0)
  #define SPI_CNT_5	((SPI_CNT_4) + 1)
  #define SPI_IDX_5	((SPI_CNT_4) + 1)
#else
  #define SPI_CNT_5	(SPI_CNT_4)
  #define SPI_IDX_5	-1
#endif
#if (((MY_SPI_LIST_DEVICE) & 0x040) != 0)
  #define SPI_CNT_6	((SPI_CNT_5) + 1)
  #define SPI_IDX_6	((SPI_CNT_5) + 1)
#else
  #define SPI_CNT_6	(SPI_CNT_5)
  #define SPI_IDX_6	-1
#endif
#if (((MY_SPI_LIST_DEVICE) & 0x080) != 0)
  #define SPI_CNT_7	((SPI_CNT_6) + 1)
  #define SPI_IDX_7	((SPI_CNT_6) + 1)
#else
  #define SPI_CNT_7	(SPI_CNT_6)
  #define SPI_IDX_7	-1
#endif
#if (((MY_SPI_LIST_DEVICE) & 0x100) != 0)
  #define SPI_CNT_8	((SPI_CNT_7) + 1)
  #define SPI_IDX_8	((SPI_CNT_7) + 1)
#else
  #define SPI_CNT_8	(SPI_CNT_7)
  #define SPI_IDX_8	-1
#endif
#if (((MY_SPI_LIST_DEVICE) & 0x200) != 0)
  #define SPI_CNT_9	((SPI_CNT_8) + 1)
  #define SPI_IDX_9	((SPI_CNT_8) + 1)
#else
  #define SPI_CNT_9	(SPI_CNT_8)
  #define SPI_IDX_9	-1
#endif

#define SPI_NMB_DEVICES	((SPI_CNT_9)+1)

/* ------------------------------------------------------------------------------------------------ */
/* Remapping table:																					*/

static const int g_DevReMap[] = { SPI_IDX_0
                               #if ((MY_SPI_MAX_DEVICES) > 1)
                                 ,SPI_IDX_1
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 2)
                                 ,SPI_IDX_2
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 3)
                                 ,SPI_IDX_3
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 4)
                                 ,SPI_IDX_4
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 5)
                                 ,SPI_IDX_5
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 6)
                                 ,SPI_IDX_6
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 7)
                                 ,SPI_IDX_7
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 8)
                                 ,SPI_IDX_8
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 9)
                                 ,SPI_IDX_9
                               #endif
                               };

/* ------------------------------------------------------------------------------------------------ */

#if (((MY_SPI_USE_MUTEX) != 0) || (((MY_SPI_OPERATION) & 0x00202) != 0))

  static const char g_Names[][16] = {
                                 MY_SPI_ASCII "SPI-0"
                               #if ((MY_SPI_MAX_DEVICES) > 1)
                                ,MY_SPI_ASCII "SPI-1"
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 2)
                                ,MY_SPI_ASCII "SPI-2"
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 3)
                                ,MY_SPI_ASCII "SPI-3"
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 4)
                                ,MY_SPI_ASCII "SPI-4"
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 5)
                                ,MY_SPI_ASCII "SPI-5"
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 6)
                                ,MY_SPI_ASCII "SPI-6"
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 7)
                                ,MY_SPI_ASCII "SPI-7"
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 8)
                                ,MY_SPI_ASCII "SPI-8"
                               #endif 
                               #if ((MY_SPI_MAX_DEVICES) > 9)
                                ,MY_SPI_ASCII "SPI-9"
                               #endif
                               };
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Checks																							*/

#if (((MY_SPI_ISR_RX_THRS) < 0) || ((MY_SPI_ISR_RX_THRS) > 100))
  #error "SPI_ISR_RX_THRS must be set between 0 and 100"
#endif

#if (((_MY_SPI_ISR_TX_THRS) < 0) || ((_MY_SPI_ISR_TX_THRS) > 100))
  #error "_MY_SPI_ISR_TX_THRS must be set between 0 and 100"
#endif

#if ((MY_SPI_MIN_4_RX_DMA) <= 0)
  #error "SPI_MIN_4_RX_DMA must be >= 1"
#endif

#if ((MY_SPI_MIN_4_TX_DMA) <= 0)
  #error "SPI_MIN_4_TX_DMA must be >= 1"
#endif

#if ((MY_SPI_MIN_4_RX_ISR) <= 0)
  #error "SPI_MIN_4_RX_ISR must be >= 1"
#endif

#if ((MY_SPI_MIN_4_TX_ISR) <= 0)
  #error "SPI_MIN_4_TX_ISR must be >= 1"
#endif


#if ((MY_SPI_USE_MUTEX) != 0)						/* Mutex for exclusive access					*/
  #define SPI_MTX_LOCK(Cfg)			MTXlock(Cfg->MyMutex, -1)
  #define SPI_MTX_UNLOCK(Cfg)		MTXunlock(Cfg->MyMutex)
#else
  #define SPI_MTX_LOCK(Cfg)	  		0
  #define SPI_MTX_UNLOCK(Cfg)		OX_DO_NOTHING()
#endif


#if ((((MY_SPI_OPERATION) & 0x00001) == 0) || ((MY_SPI_TOUT_ISR_ENB) == 0))
  #define SPI_EXPIRY_RX(_IsExp, _IsrState, _Expiry)		do {										\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
														} while(0)
#else
  #define SPI_EXPIRY_RX(_IsExp, _IsrState, _Expiry)		do {										\
															OSintBack(_IsrState);					\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
															_IsrState = OSintOff();					\
														} while(0)
#endif
#if ((((MY_SPI_OPERATION) & 0x00100) == 0) || ((MY_SPI_TOUT_ISR_ENB) == 0))
  #define SPI_EXPIRY_TX(_IsExp, _IsrState, _Expiry)		do {										\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
														} while(0)
#else
  #define SPI_EXPIRY_TX(_IsExp, _IsrState, _Expiry)	do {											\
															OSintBack(_IsrState);					\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
															_IsrState = OSintOff();					\
														} while(0)
#endif
#if ((((MY_SPI_OPERATION) & 0x00101) == 0) || ((MY_SPI_TOUT_ISR_ENB) == 0))
  #define SPI_EXPIRY_RXTX(_IsExp, _IsrState, _Expiry)	do {										\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
														} while(0)
#else
  #define SPI_EXPIRY_RXTX(_IsExp, _IsrState, _Expiry)	do {										\
															OSintBack(_IsrState);					\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
															_IsrState = OSintOff();					\
														} while(0)
#endif

/* ------------------------------------------------------------------------------------------------ */
/* SPI register index definitions (are accessed as uint32_t)										*/

#define SPI_CTRLR0_REG				(0x00/4)
#define SPI_CTRLR1_REG				(0x04/4)
#define SPI_SPIENR_REG				(0x08/4)
#define SPI_MWCR_REG				(0x0C/4)
#define SPI_SER_REG					(0x10/4)
#define SPI_BAUDR_REG				(0x14/4)
#define SPI_TXFTLR_REG				(0x18/4)
#define SPI_RXFTLR_REG				(0x1C/4)
#define SPI_TXFLR_REG				(0x20/4)
#define SPI_RXFLR_REG				(0x24/4)
#define SPI_SR_REG					(0x28/4)
#define SPI_IMR_REG					(0x2C/4)
#define SPI_ISR_REG					(0x30/4)
#define SPI_RISR_REG				(0x34/4)
#define SPI_TXOICR_REG				(0x38/4)
#define SPI_RXOICR_REG				(0x3C/4)
#define SPI_RXUICR_REG				(0x40/4)
#define SPI_ICR_REG					(0x48/4)
#define SPI_DMACR_REG				(0x4C/4)
#define SPI_DMATDLR_REG				(0x50/4)
#define SPI_DMARDLR_REG				(0x54/4)
#define SPI_IDR_REG					(0x58/4)
#define SPI_ID_REG					(0x5C/4)
#define SPI_DR_REG					(0x60/4)
#define SPI_RXSMPLDLY_REG			(0xFC/4)

/* ------------------------------------------------------------------------------------------------ */
/* Local variables																					*/

typedef struct {									/* Per device: configuration					*/
	int      IsSlave;								/* If this is a slave SPI 						*/
	volatile uint32_t *HW;							/* Base address of the SPI peripheral			*/
	int IsInit;										/* If this device has been initialized			*/
	int SlvInit;									/* Slave that have been initialized (bit field)	*/
	uint32_t Crtl0[MY_SPI_MAX_SLAVES];				/* CRTLR #0 register							*/
	uint32_t BaudR[MY_SPI_MAX_SLAVES];				/* Baud rate register (used in master only)		*/
    int      Nbits[MY_SPI_MAX_SLAVES];				/* Number of bits per frame						*/
	uint32_t Mode[MY_SPI_MAX_SLAVES];				/* How the transfers are done					*/
	int      NsecBit[MY_SPI_MAX_SLAVES];			/* Nano-second per bit tranferred				*/
	int      Shift[MY_SPI_MAX_SLAVES];				/* If data frame are left aligned				*/
	uint32_t SndRcv[MY_SPI_MAX_SLAVES];				/* TMOD for spi_send_recv() EEPROM or TX-RX		*/
	int      DMAminRX;								/* Minimum # frames to use DMA on RX			*/
	int      DMAminTX;								/* Minimum # frames to use DMA on TX			*/
	int      ISRminRX;								/* Minimum # frames to use ISRs on RX			*/
	int      ISRminTX;								/* Minimum # frames to use ISRs on TX			*/
	int      ISRwaterRX;							/* RX FIFO threshold							*/
	int      ISRwaterTX;							/* TX FIFO threshold							*/
	int      TimeOut;								/* Timeout to use unless is 0 (0 set optimal)	*/
  #if ((MY_SPI_USE_MUTEX) != 0)						/* If mutex protection enable					*/
	MTX_t *MyMutex;									/* One mutex per controller						*/
  #endif
  #if (((MY_SPI_OPERATION) & 0x00202) != 0)			/* If interrupts are used						*/
	volatile int      Error;						/* If there was an error the transfer			*/
	SEM_t            *MySema;						/* Semaphore posted in the ISR					*/
	volatile int      ISRnbits;
	volatile int      ISRshift;
   #if ((MY_SPI_MULTICORE_ISR) != 0)
	volatile int      ISRonGoing;					/* Inform other cores if one handling the ISR	*/
	int               SpinLock;						/* Spinlock for exclusive access to ISRonGoing	*/
   #endif
   #if (((MY_SPI_OPERATION) & 0x10000) != 0)		/* If packed data is supported					*/
	volatile int      ISRalignLeft;					/* We give the ISR all pre-computed stuff to	*/
	volatile int      ISRisPack;					/* reduce the overhead as much as possible		*/
   #endif
  #endif
  #if (((MY_SPI_OPERATION) & 0x00002) != 0)			/* If RX interrupts are used					*/
	volatile void *ISRbufRX;						/* Current buffer address to fill				*/
	volatile int   ISRleftRX;						/* Number of frames left to receive				*/
   #if (((MY_SPI_OPERATION) & 0x10000) != 0)
	volatile uint32_t ISRmaskRX;
	volatile uint32_t ISRdataRX;
	volatile int      ISRshiftRX;
   #endif
  #endif
  #if (((MY_SPI_OPERATION) & 0x00200) != 0)			/* If RX interrupts are used					*/
	volatile void *ISRbufTX;						/* Current buffer address to fill				*/
	volatile int   ISRdumTX;						/* Number of dummy frames left to send			*/
	volatile int   ISRleftTX;						/* Number of frames left to send				*/
   #if (((MY_SPI_OPERATION) & 0x10000) != 0)
	volatile uint32_t ISRdataTX;
	volatile int      ISRshiftTX;
   #endif
  #endif
} SPIcfg_t;

static SPIcfg_t g_SPIcfg[SPI_NMB_DEVICES];			/* Device configuration							*/
static int      g_CfgIsInit = 0;					/* To track first time an init occurs			*/

static volatile uint32_t g_Dummy; 					/* General purpose for forced read or write		*/

/* ------------------------------------------------------------------------------------------------ */
/* Macros to perform the packing & unpacking														*/
/* Macros are used as the code, altough simple is quite big and not using macros makes the code		*/
/* a bit difficult to follow.  So they are solely for code readability.								*/

#define SPI_RX_PACK(_AlignLeft, _Nbits, _ii, _Data, _Shift, _Mask, _Buf8, _Buf16, _Tmp16, _Reg)		\
	do {																							\
		if (_AlignLeft != 0) {					/* Left aligned packed data						*/	\
			if (_Nbits <= 8) {					/* Select byte or 16 bit transfers				*/	\
				for ( ; _ii>0 ; _ii--) {															\
					_Data <<= _Nbits;			/* Make room to insert on the right				*/	\
					_Data  |= _Reg[SPI_DR_REG]	/* Insert only the # bits in a frame			*/	\
					        & _Mask;																\
					_Shift += _Nbits;			/* This more bits are held in Data				*/	\
					if (_Shift >= 8) {			/* More bits than a byte						*/	\
						_Shift  -= 8;			/* Adjust the shift value for next time			*/	\
						*_Buf8++ = _Data >> _Shift;		/* Memo leftmost 8 bits					*/	\
					}																				\
				}																					\
			}																						\
			else {								/* 16 bit buffer								*/	\
				if ((0x1 & (uintptr_t)_Buf16) == 0) {												\
					for ( ; _ii>0 ; _ii--) {														\
						_Data <<= _Nbits;		/* Make room to insert on the right				*/	\
						_Data  |= _Reg[SPI_DR_REG]	/* Insert only the # bits in a frame		*/	\
						        & _Mask;															\
						_Shift += _Nbits;		/* This more bits are held in Data				*/	\
						if (_Shift >= 16) {		/* More bits than a 16 bit word					*/	\
							_Shift   -= 16;		/* Adjust the shift value for next time			*/	\
							*_Buf16++ = _Data >> _Shift;	/* Memo leftmost 16 bits			*/	\
						}																			\
					}																				\
				}																					\
				else {																				\
					for ( ; _ii>0 ; _ii--) {														\
						_Data <<= _Nbits;		/* Make room to insert on the right				*/	\
						_Data  |= _Reg[SPI_DR_REG]	/* Insert only the # bits in a frame		*/	\
						        & _Mask;															\
						_Shift += _Nbits;		/* This more bits are held in Data				*/	\
						if (_Shift >= 16) {		/* More bits than a 16 bit word					*/	\
							_Shift   -= 16;		/* Adjust the shift value for next time			*/	\
							_Tmp16[0] = _Data >> _Shift;	/* Memo leftmost 16 bits			*/	\
							*_Buf8++  = ((uint8_t *)_Tmp16)[0];										\
							*_Buf8++  = ((uint8_t *)_Tmp16)[1];										\
						}																			\
					}																				\
					_Buf16 = (uint16_t *)_Buf8;														\
				}																					\
			}																						\
		}																							\
		else {									/* Right aligned packing						*/	\
			if (_Nbits <= 8) {					/* Select byte or 16 bit transfers				*/	\
				for ( ; _ii>0 ; _ii--) {															\
					_Data  |= (_Mask & _Reg[SPI_DR_REG])											\
					       << _Shift;			/* Insert on the left the new frame				*/	\
					_Shift += _Nbits;			/* This more bits are held in Data				*/	\
					if (_Shift >= 8) {			/* More bits than a byte						*/	\
						_Shift  -= 8;			/* Adjust the shift value for next time			*/	\
						*_Buf8++ = _Data;		/* Memo the rightmost 8 bits					*/	\
						_Data  >>= 8;			/* Get rid of the bits that were memoed			*/	\
					}																				\
				}																					\
			}																						\
			else {								/* 16 bit buffer								*/	\
				if ((0x1 & (uintptr_t)_Buf16) == 0) {												\
					for ( ; _ii>0 ; _ii--) {														\
						_Data  |= (_Mask & _Reg[SPI_DR_REG])										\
						        << _Shift;		/* Insert on the left the new frame				*/	\
						_Shift += _Nbits;		/* This more bits are held in Data				*/	\
						if (_Shift >= 16) {		/* More bits than a byte						*/	\
							_Shift   -= 16;		/* Adjust the shift value for next time			*/	\
							*_Buf16++ = _Data;	/* Memo the rightmost 16 bits					*/	\
							_Data   >>= 16;		/* Get rid of the bits that were memoed			*/	\
						}																			\
					}																				\
				}																					\
				else {																				\
					for ( ; _ii>0 ; _ii--) {														\
						_Data  |= (_Mask & _Reg[SPI_DR_REG])										\
						        << _Shift;		/* Insert on the left the new frame				*/	\
						_Shift += _Nbits;		/* This more bits are held in Data				*/	\
						if (_Shift >= 16) {		/* More bits than a byte						*/	\
							_Shift   -= 16;		/* Adjust the shift value for next time			*/	\
							_Tmp16[0] = _Data;	/* Memo the rightmost 16 bits					*/	\
							*_Buf8++  = ((uint8_t *)_Tmp16)[0];										\
							*_Buf8++  = ((uint8_t *)_Tmp16)[1];										\
							_Data   >>= 16;		/* Get rid of the bits that were memoed			*/	\
						}																			\
					}																				\
					_Buf16 = (uint16_t *)_Buf8;														\
				}																					\
			}																						\
		}																							\
	} while(0)

#define SPI_RX_NOTPACK(_Nbits, _ii, _Shift,  _Buf8, _Buf16, _Tmp16, _Reg)							\
	do {																							\
		if (_Nbits <= 8) {																			\
			for ( ; _ii>0 ; _ii--) {																\
				*_Buf8++ = _Reg[SPI_DR_REG] << _Shift;												\
			}																						\
		}																							\
		else {																						\
			if ((0x1 & (uintptr_t)_Buf16) == 0) {													\
				for ( ; _ii>0 ; _ii--) {															\
					*_Buf16++ = _Reg[SPI_DR_REG] << _Shift;											\
				}																					\
			}																						\
			else {																					\
				for ( ; _ii>0 ; _ii--) {															\
					_Tmp16[0] = _Reg[SPI_DR_REG] << _Shift;											\
					*_Buf8++  = ((uint8_t *)_Tmp16)[0];												\
					*_Buf8++  = ((uint8_t *)_Tmp16)[1];												\
				}																					\
				_Buf16 = (uint16_t *)_Buf8;															\
			}																						\
		}																							\
	} while (0)

#if (((MY_SPI_OPERATION) & 0x10000) != 0)
 #define SPI_RX_DATA(_Pack,_AlgLeft,_Nbits,_ii,_Data,_ShfP,_ShfN,_Mask,_Buf8,_Buf16,_Buf16Tmp,_Reg)	\
	do {																							\
		if (_Pack != 0) {																			\
			SPI_RX_PACK(_AlgLeft, _Nbits, _ii, _Data, _ShfP, _Mask,_Buf8,_Buf16,_Buf16Tmp,_Reg);	\
		}																							\
		else {																						\
			SPI_RX_NOTPACK(_Nbits, _ii, _ShfN,  _Buf8, _Buf16, _Buf16Tmp, _Reg);					\
		}																							\
	} while(0)

#else
 #define SPI_RX_DATA(_Pack,_AlgLeft,_Nbits,_ii,_Data,_ShfP,_ShfN,_Mask,_Buf8,_Buf16,_Buf16Tmp,_Reg)	\
	do {																							\
		SPI_RX_NOTPACK(_Nbits, _ii, _ShfN,  _Buf8, _Buf16, _Buf16Tmp, _Reg);						\
	} while(0)

#endif

#define SPI_TX_PACK(_AlignLeft, _Nbits, _ii, _Data, _Shift, _Buf8, _Buf16, _Tmp16, _Reg)			\
	do {																							\
		if (_AlignLeft != 0) {					/* Left aligned packed data						*/	\
			if (_Nbits <= 8) {					/* Select byte or 16 bit transfers				*/	\
				for ( ; _ii>0 ; _ii--) {															\
					if (_Shift < _Nbits) {															\
						_Data <<= 8;			/* Make room to insert new byte					*/	\
						_Data  |= ((uint32_t)(*_Buf8++));											\
						_Shift += 8;			/* This more bits in DataTX						*/	\
					}																				\
					_Shift          -= _Nbits;														\
					_Reg[SPI_DR_REG] = _Data >> _Shift;												\
				}																					\
			}																						\
			else {								/* 16 bit buffer								*/	\
				if ((0x1 & (uintptr_t)_Buf16) == 0) {												\
					for ( ; _ii>0 ; _ii--) {														\
						if (_Shift < _Nbits) {														\
							_Data <<= 16;			/* Make room to insert new byte				*/	\
							_Data  |= ((uint32_t)(*_Buf16++));										\
							_Shift += 16;			/* This more bits in DataTX					*/	\
						}																			\
						_Shift          -= _Nbits;													\
						_Reg[SPI_DR_REG] = _Data >> _Shift;											\
					}																				\
				}																					\
				else {																				\
					for ( ; _ii>0 ; _ii--) {														\
						if (_Shift < _Nbits) {														\
							_Data <<= 16;			/* Make room to insert new byte				*/	\
							((uint8_t *)_Tmp16)[0] = *_Buf8++;										\
							((uint8_t *)_Tmp16)[1] = *_Buf8++;										\
							_Data                 |= (uint32_t)Buf16Tmp[0];							\
							_Shift                += 16;											\
						}																			\
						_Shift          -= _Nbits;													\
						_Reg[SPI_DR_REG] = _Data >> _Shift;											\
					}																				\
					_Buf16 = (uint16_t *)_Buf8;														\
				}																					\
			}																						\
		}																							\
		else {									/* Right Aligned packed frames					*/	\
			if (_Nbits <= 8) {					/* Select byte or 16 bit transfers				*/	\
				for ( ; _ii>0 ; _ii--) {															\
					if (_Shift < 8) {			/* Do we need to grab another data byte?		*/	\
						_Data  |= ((uint32_t)(*_Buf8++)) << _Shift;									\
						_Shift += 8;			/* This more bits in DataTX						*/	\
					}																				\
					_Shift          -= _Nbits;			/* Will send Nbits, remove from count	*/	\
					_Reg[SPI_DR_REG] = _Data >> _Shift;												\
					_Data          >>= _Nbits;			/* Position for the next transfer		*/	\
				}																					\
			}																						\
			else {								/* 16 bit buffer								*/	\
				if ((0x1 & (uintptr_t)_Buf16) == 0) {												\
					for ( ; _ii>0 ; _ii--) {														\
						if (_Shift < 16) {		/* Do we need to grab another data byte?		*/	\
							_Data  |= ((uint32_t)(*_Buf16++)) << _Shift;							\
							_Shift += 16;		/* This more bits in DataTX						*/	\
						}																			\
						_Shift          -= _Nbits;		/* Will send Nbits, remove from count	*/	\
						_Reg[SPI_DR_REG] = _Data >> _Shift;											\
						_Data          >>= _Nbits;		/* Position for the next transfer		*/	\
					}																				\
				}																					\
				else {																				\
					for ( ; _ii>0 ; _ii--) {														\
						if (_Shift < 16) {		/* Do we need to grab another data byte?		*/	\
							((uint8_t *)_Tmp16)[0] = *_Buf8++;										\
							((uint8_t *)_Tmp16)[1] = *_Buf8++;										\
							_Data  |= ((uint32_t)Buf16Tmp[0]) << _Shift;							\
							_Shift += 16;		/* This more bits in DataTX						*/	\
						}																			\
						_Shift          -= _Nbits;		/* Will send Nbits, remove from count	*/	\
						_Reg[SPI_DR_REG] = _Data >> _Shift;											\
						_Data          >>= _Nbits;		/* Position for the next transfer		*/	\
					}																				\
					_Buf16 = (uint16_t *)_Buf8;														\
				}																					\
			}																						\
		}																							\
	} while(0)

#define SPI_TX_NOTPACK(_Nbits, _ii, _Shift,  _Buf8, _Buf16, _Tmp16, _Reg)							\
	do {																							\
		if (_Nbits <= 8) {																			\
			for ( ; _ii>0 ; _ii--) {																\
				_Reg[SPI_DR_REG] = (*_Buf8++) >> _Shift;											\
			}																						\
		}																							\
		else {																						\
			if ((0x1 & (uintptr_t)_Buf16) == 0) {													\
				for ( ; _ii>0 ; _ii--) {															\
					_Reg[SPI_DR_REG] = (*_Buf16++) >> _Shift;										\
				}																					\
			}																						\
			else {																					\
				for ( ; _ii>0 ; _ii--) {															\
					((uint8_t *)_Tmp16)[0] = *_Buf8++;												\
					((uint8_t *)_Tmp16)[1] = *_Buf8++;												\
					 _Reg[SPI_DR_REG]      = _Tmp16[0] >> _Shift;									\
				}																					\
				_Buf16 = (uint16_t *)_Buf8;															\
			}																						\
		}																							\
	} while(0)

#if (((MY_SPI_OPERATION) & 0x10000) != 0)
 #define SPI_TX_DATA(_Pack,_AlignLeft,_Nbits,_ii,_Data,_ShfP,_ShfN,_Buf8,_Buf16,_Buf16Tmp,_Reg)		\
	do {																							\
		if (_Pack != 0) {																			\
			SPI_TX_PACK(_AlignLeft, _Nbits, _ii, _Data, _ShfP, _Buf8, _Buf16, _Buf16Tmp, _Reg);		\
		}																							\
		else {																						\
			SPI_TX_NOTPACK(_Nbits, _ii, _ShfN,  _Buf8, _Buf16, _Buf16Tmp, _Reg);					\
		}																							\
	} while(0)
#else
 #define SPI_TX_DATA(_Pack,_AlignLeft,_Nbits,_ii,_Data,_ShfP,_ShfN,_Buf8,_Buf16,_Buf16Tmp,_Reg)		\
	do {																							\
		SPI_TX_NOTPACK(_Nbits, _ii, _ShfN,  _Buf8, _Buf16, _Buf16Tmp, _Reg);						\
	} while(0)

#endif

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: spi_recv																				*/
/*																									*/
/* spi_recv - receive data over the spi bus															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int spi_recv(int Dev, int Slv, void *Buf, uint32_t Len);									*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : SPI controller number																*/
/*		Slv  : SPI slave index																		*/
/*		Buf  : pointer to the buffer where the received frames will be deposited					*/
/*		Len  : number of frames to receive															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		MicroWire: the 4 first bytes in Buf holds the control word information:						*/
/*		               Byte #0 : number of bits in the control word									*/
/*		               Byte #1 : un-used															*/
/*		               Byte #2 : MSByte of the control word (ignored if # bits <= 8)				*/
/*		               Byte #3 : LSByte of the control word											*/
/*				   The data captured from the SPI bus will overwrite these							*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SPI_ADD_PREFIX(spi_recv) (int Dev, int Slv, void *Buf, uint32_t Len)
{
uint8_t  *Buf8;										/* Same as Buf, for 8 bit write					*/
uint16_t *Buf16;									/* Same as Buf, for 16 bit write				*/
uint16_t  Buf16Tmp[1];								/* When writing to 16 bit unaligned memory		*/
SPIcfg_t *Cfg;										/* De-reference array for faster access			*/
uint32_t  Data;										/* General purpose								*/
int       Error;									/* To track RX FIFO overflow					*/
int       Expiry;									/* Expiry value of the RTOS timer tick counter	*/
int       ii;										/* Multi-purpose								*/
int       IsExp;									/* If the processing has timed-out				*/
uint32_t  LeftRX;									/* Number of frames left ro receive				*/
int       Nbits;									/* Number of bits per frame						*/
int       ReMap;									/* Device # remapped							*/
volatile uint32_t *Reg;								/* Base pointer to SPI controller registers		*/
int       Shift;									/* Shift to apply on the data read				*/
int       TimeOut;									/* Timeout value in # RTOS timer ticks			*/
unsigned int UseISR;								/* If using ISRs for transfer and/or EOT		*/
#if (((MY_SPI_OPERATION) & 0x00001) != 0)
 int      IsrState;									/* ISR state before disabling them				*/
#endif
#if (((MY_SPI_OPERATION) & 0x00004) != 0)
 uint32_t  CfgOp[8];								/* DMA configuration commands					*/
 int       DMAid;									/* ID of the DMA transfer						*/
 uint32_t  DmaLen;									/* Number of frames read using the DMA			*/
#endif
#if (((MY_SPI_OPERATION) & 0x10000) != 0)
 int      AlignLeft;								/* If the packed data is left aligned			*/
 int      IsPack;									/* If has to pack the received data				*/
 uint32_t Mask;										/* Mask to isolate the valid bits recived		*/
#endif
#if ((MY_SPI_DEBUG) > 0)
 int DbgDump;
#endif
#if ((MY_SPI_DEBUG) > 1)
 unsigned int XferType;
#endif

  #if ((MY_SPI_DEBUG) > 1)
	printf("SPI   [Dev:%d - Slv:%d] Receiving %d frames\n", Dev, Slv, (int)Len);
	XferType = 0U;
  #endif

  #if ((MY_SPI_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_SPIcfg[ReMap];						/* This is this device configuration			*/

  #else
	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (MY_SPI_MAX_DEVICES))
	||  (Slv < 0)
	||  (Slv >= (MY_SPI_MAX_SLAVES))) {
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Out of range device / slave\n", Dev, Slv);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by SPI_LIST_DEVICE				*/
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Cannot remap device\n", Dev, Slv);
	  #endif
		return(-2);
	}

	if (Buf == (void *)NULL) {
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - NULL buffer\n", Dev, Slv);
	  #endif
		return(-3);
	}

	Cfg = &g_SPIcfg[ReMap];							/* This is this device configuration			*/
	if ((g_CfgIsInit == 0)
	||  (0 == (Cfg->SlvInit & (1U << Slv)))) {		/* The controller is not initialized			*/
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Device / slave not initialized\n", Dev, Slv);
	  #endif
		return(-4);
	}

	if (Len > 65536) {								/* Maximum # frames the controller can receive	*/
	  #if ((MY_SPI_DEBUG) > 0)						/* Not possible to extend with multiple recv	*/
		printf("SPI   [Dev:%d - Slv:%d] - Error - # of frames exceed controller capabilities\n",
		       Dev, Slv);
	  #endif
		return(-5);
	}
  #endif

	if (Len == 0) {									/* Receiving nothing is not considered an error	*/
		return(0);
	}
													/* Check if ACP remapping is requested			*/
	if (((Cfg->Mode[Slv] & (SPI_CFG_CACHE_RX(1,0))) == SPI_CFG_CACHE_RX(1,0))
	&&  ((0xC0000000 & (uintptr_t)Buf) == 0x80000000)) {	/* Check the address 2 MSBits			*/
		Buf8 = (uint8_t *)((0x3FFFFFFF & (uintptr_t)Buf)	/* When is ACP, remap to ACP address	*/
		                 | (0xC0000000 & (Cfg->Mode[Slv])));
	}
	else {
		Buf8 = (uint8_t *)Buf;
	}

	Buf16   = (uint16_t *)Buf8;
	Data    = 0;									/* When not uWire: to remove compiler warning	*/
	Error   = 0;
	IsExp   = 0;
	LeftRX  = Len;
	Nbits   = Cfg->Nbits[Slv];
	Reg     = Cfg->HW;
	Shift   = Cfg->Shift[Slv];						/* Timeout: use *8 for margin of error			*/
	TimeOut = (Len * Nbits *Cfg->NsecBit[Slv])		/* Timeout waiting for the transfer done		*/
	        >> 17;		         					/* Doing /(128*1024) instead of *8/(1000*1000)	*/
	TimeOut = OS_MS_TO_MIN_TICK(TimeOut, 3);		/* Convert ms to RTOS timer tick count			*/
	UseISR  = 0U;

  #if (((MY_SPI_OPERATION) & 0x10000) != 0)			/* When packed data supported, pull the info	*/
	IsPack    = Cfg->Mode[Slv]
	          & (SPI_DATA_PACK);
	AlignLeft = Cfg->Mode[Slv]
	          & (SPI_ALIGN_LEFT);
	Mask      = (1U << Nbits)
	          - 1; 
  #endif
  #if ((MY_SPI_DEBUG) > 0)
	DbgDump = 0;
  #endif

	if ((Cfg->Mode[Slv] & 3) == (SPI_PROTO_UWIRE)) {/* uWire: prepare the control word				*/

	  #if ((MY_SPI_ARG_CHECK) == 0)
		if ((Buf8[0] == 0)							/* Controller can only send from 1 to 16 bits	*/
		||  (Buf8[0]) > 16)) {
		  #if ((MY_SPI_DEBUG) > 0)
		   printf("SPI   [Dev:%d - Slv:%d] - Error - # of bits for uWire control word out of range\n",
			       Dev, Slv);
		  #endif
			return(-6);
		}		
	  #endif
		Data   = Buf8[2];							/* MSByte of the control word					*/
		Data <<= 8;									/* No ANDing nneded as all data here is uint	*/
		Data  |= Buf8[3];							/* LSByte of the control word					*/
		if ((Cfg->Mode[Slv] & (SPI_ALIGN_LEFT)) != 0) {	/* If is left aligned, right aligned it as	*/
			ii = Buf8[0];							/* it's what the controller needs				*/
			if (ii <= 8) {							/* Get # bits in control word					*/
				Data >>= 8-ii;
			}
			else {
				Data >>= 16-ii;
			}
		}
	}

	if (0 != SPI_MTX_LOCK(Cfg)) {					/* Now we need to have exclusive access			*/
		return(-7);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	Reg[SPI_SPIENR_REG] = 0;						/* Must be disable to be re-programmed			*/
	Reg[SPI_CTRLR0_REG] = Cfg->Crtl0[Slv]
	                    | (2U << 8)					/* Set in reception mode only					*/
	                    | (((uint32_t)(Buf8[0]-1)) << 12);		/* uWire: # bits in ctrl word		*/
	Reg[SPI_MWCR_REG]   = 1							/* Always perform sequential read transfer		*/
	                    | ((Cfg->Mode[Slv] & (SPI_UWIRE_HS))
	                       ? 4						/* If handshake was requested at initialization	*/
	                       : 0);
	Reg[SPI_CTRLR1_REG] = Len-1;					/* Number of frames to read						*/
	Reg[SPI_BAUDR_REG]  = Cfg->BaudR[Slv];			/* Set baud rate for this slave					*/
	Reg[SPI_RXFTLR_REG] = 0;						/* Use default threshold of 0					*/
	Reg[SPI_DMACR_REG]  = 0;						/* Assume DMA not used							*/
	Reg[SPI_SER_REG]    = 1U << Slv;				/* Set the target slave							*/
	Reg[SPI_IMR_REG]    = 0;						/* Default, no ISRs								*/

  #if (((MY_SPI_OPERATION) & 0x00002) != 0)			/* ISRs can be used								*/
	Cfg->ISRnbits       = Nbits;					/* In case pack data is used, give info to ISR	*/
	Cfg->ISRshift       = Shift;
	Reg[SPI_RXFTLR_REG] = (Len <= (MY_SPI_RX_FIFO_SIZE))
	                    ? (Len-1)					/* When all fits, watermark is full length		*/
	                    : Cfg->ISRwaterRX;			/* Not all fit, use specified watermark			*/

	if ((Cfg->Mode[Slv] & (SPI_EOT_ISR)) != 0) {	/* If using ISR for EOT							*/
		UseISR = 4U;								/* Use EOT from ISR 							*/
	}
	if ((Len >= Cfg->ISRminRX) 						/* If enough for transfer						*/
	&&  ((Cfg->Mode[Slv] & (SPI_XFER_ISR)) != 0)) {	/* And can use ISR for the  transfers			*/
		UseISR = 1U;								/* Is always blocking on EOT from ISR			*/
	}

   #if (((MY_SPI_OPERATION) & 0x00200) != 0)		/* ISRs are used but we don't send anything out	*/
	Cfg->ISRleftTX = 0;								/* Make sure the ISRs does nothing with TX		*/
	Cfg->ISRdumTX  = 0;
   #endif
  #endif

  #if (((MY_SPI_OPERATION) & 0x00004) != 0)			/* DMA can be used								*/
	DMAid      = 0;									/* If DMA is not used, it will remain == 0		*/
	DmaLen     = 0;									/* To remove compiler warning					*/

	if (((Cfg->Mode[Slv] & (SPI_XFER_DMA)) != 0)	/* Is this slave set-up to use the DMA?			*/
	&&  (Len >= Cfg->DMAminRX)) {					/* Enough data to use the DMA?					*/
		ii  = (MY_SPI_RX_FIFO_SIZE)-1;				/* Figure out the largest useable burst length	*/
		if (ii > (DMA_MAX_BURST)) {					/* By maximizing the burst length it maximises	*/
			ii = DMA_MAX_BURST;						/* the transfer rate							*/
		}
		if (ii > Len) {
			ii = Len;
		}
		DmaLen               = Len;
		Reg[SPI_DMARDLR_REG] = ii-1;				/* Watermark: trigger when #RX FIFO > watermark	*/
		Reg[SPI_DMACR_REG]   = 0x1;					/* Enable DMA RX operations						*/
		UseISR               = 0U;					/* No ISR tranfers nor EOT (DMA does it all)	*/
	}
  #endif

	Reg[SPI_SPIENR_REG] = 1;						/* Enable the controller						*/
	for (ii=0 ; ii<(MY_SPI_RX_FIFO_SIZE) ; ii++) {	/* Make sure the RX FIFO is empty				*/
		g_Dummy = Reg[SPI_DR_REG];
	}
	g_Dummy = Reg[SPI_ICR_REG];						/* Clear all overflow/underflow interrupts		*/

													/* How to start the DMA: 0: RX/TX no blocking	*/
  #if (((MY_SPI_OPERATION) & 0x00004) != 0)			/* DMA is used									*/
	if (DmaLen != 0) {								/* Program and start the DMA but don't wait for	*/

	  #if ((MY_SPI_DEBUG) > 1)
		XferType = 1U << 2;							/* Use the same bits as in MY_SPI_OPERATION		*/
	  #endif

		CfgOp[0] = DMA_CFG_NOWAIT;					/* EOT because need DR_REG write to start xfer	*/
		CfgOp[1] = (Cfg->Mode[Slv] & (SPI_EOT_ISR))
		         ? DMA_CFG_EOT_ISR
		         : DMA_CFG_EOT_POLLING;
		ii = 2;
	  #if ((MY_SPI_REMAP_LOG_ADDR) != 0)
		CfgOp[ii++] = DMA_CFG_LOGICAL_SRC;
		CfgOp[ii++] = DMA_CFG_LOGICAL_DST;
	  #endif

		if (((Cfg->Mode[Slv] & (SPI_CFG_CACHE_RX(1,0))) == SPI_CFG_CACHE_RX(1,0))
		&& ((0xC0000000 & (uintptr_t)Buf) == 0x80000000)) {		/* If ACP remapping is used			*/
		  #if ((MY_SPI_REMAP_LOG_ADDR) != 0)
			ii--;									/* Do this to overwrite DMA_CFG_LOGICAL_DST		*/
		  #endif
			CfgOp[ii++] = DMA_CFG_NOCACHE_DST;
		}
		else if (Cfg->Mode[Slv] == SPI_CFG_CACHE_RX(0,0)) {	/* No cache maint requested				*/
			CfgOp[ii++] = DMA_CFG_NOCACHE_DST;
		}
		CfgOp[ii] = 0;

		ii = (Nbits <= 8)							/* Number of byte for each transfer				*/
		   ? 1
		   : 2;

		Error = dma_xfer(DMA_DEV,
		                 Buf, ii, -1,
			             (void *)&Reg[SPI_DR_REG], 0, g_DMAchan[Dev][1],
		                 ii, Reg[SPI_DMARDLR_REG]+1, DmaLen, 
			             DMA_OPEND_NONE, NULL, 0,
			             &CfgOp[0], &DMAid, TimeOut);

		LeftRX = 0;									/* The DMA transfers everything					*/
		UseISR = 0;									/* And non-DMA ISR are not used					*/

	  #if ((MY_SPI_DEBUG) > 0)
		if (Error != 0) {							/* Something went wrong setting up the DMA		*/
			printf("SPI   [Dev:%d - Slv:%d] - Error -  RX DMA reported error #%d\n",
			        Dev, Slv, Error);
		}
	  #endif

		if (Error == 0) {
			Reg[SPI_DR_REG] = Data;					/* Dummy write (or uWire ctrl word) to start RX	*/

			Error = dma_wait(DMAid);				/* Wait for the DMA transfer to be completed	*/

		  #if ((MY_SPI_DEBUG) > 0)
			if (Error != 0) {						/* Something went wrong setting up the DMA		*/
				printf("SPI   [Dev:%d - Slv:%d] - Error -  Timeout waiting for RX DMA\n",
				        Dev, Slv);
				IsExp = 1;
			}
		  #endif
		}

		if (Error != 0) {							/* Something went wrong setting up the DMA		*/
			dma_kill(DMAid);						/* Make sure the channel is released			*/
			DMAid  = 0;								/* Invalidate the ID							*/
			Error -= 12;							/* Re-adjust error value for SPI context report	*/
			Len    = 0;								/* This will skip all further processing		*/
		}
	}
  #endif

													/* -------------------------------------------- */
  #if (((MY_SPI_OPERATION) & 0x00002) != 0)			/* Start the ISRs, if they are used				*/
	if (UseISR != 0U) {

		Cfg->ISRleftRX = 0;							/* Asume only EOT								*/

		if ((UseISR & 1U) != 0U) { 

		  #if ((MY_SPI_DEBUG) > 1)
			XferType = 1U << 1;						/* Use the same bits as in MY_SPI_OPERATION		*/
		  #endif

			Cfg->ISRleftRX  = LeftRX;
			Cfg->ISRbufRX   = Buf8;

		  #if (((MY_SPI_OPERATION) & 0x10000) != 0)
			Cfg->ISRmaskRX  = Mask;
			Cfg->ISRdataRX  = Data;
			Cfg->ISRshiftRX = Shift;
		  #endif

			LeftRX  =  0;							/* All data to RX is done in the ISR			*/
		}

		Cfg->Error = 0;

		SEMreset(Cfg->MySema);						/* Make sure there are no pending posting		*/

		Reg[SPI_IMR_REG] = 0x10;					/* RX FIFO above watermark interrupt on			*/

		Reg[SPI_DR_REG]  = Data;					/* Dummy write (or uWire ctrl word) to start RX	*/
	  #if (((MY_SPI_OPERATION) & 0x10000) != 0)
		Data = 0;									/* Need it set to 0 for packed data				*/
	  #endif

		IsExp = SEMwait(Cfg->MySema, TimeOut);		/* And wait for the job to be done				*/

		Reg[SPI_IMR_REG] = 0;						/* Safety in case of timeout					*/

		Error = Cfg->Error;

	  #if ((MY_SPI_DEBUG) > 0)
		if (Error == -11) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - RX FIFO overflow in ISR\n", Dev, Slv);
			DbgDump = 1;
			LeftRX = Cfg->ISRleftRX;
		}
		if (IsExp != 0) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - Timeout waiting for ISR done\n", Dev, Slv);
			LeftRX = Cfg->ISRleftRX;
		}
	  #endif

		UseISR = 1;									/* This informs to not start the transfer		*/
	}
  #endif

  #if (((MY_SPI_OPERATION) & 0x00001) != 0)			/* If requested to keep ISR disabled during the	*/
	IsrState = OSintOff();							/* transfer										*/
  #endif

	Expiry =  OS_TICK_EXP(TimeOut);					/* This is where the Polled Xfer is started		*/

	if ((LeftRX != 0)
	&&  (UseISR == 0)) {
		Reg[SPI_DR_REG] = Data;						/* Dummy write (or uWire ctrl word) to start RX	*/
	  #if (((MY_SPI_OPERATION) & 0x10000) != 0)
		Data = 0;									/* Need it set to 0 for packed data				*/
	  #endif
	}

	while((LeftRX != 0)								/* Do all the polling required					*/
	&&    (IsExp == 0)) {							/* Error can be !=0 if DMA: DMA sets LeftRX==0	*/
		ii =  Reg[SPI_RXFLR_REG];					/* Get how many frames are in the RX FIFO		*/
		if (ii > LeftRX) {
			ii = LeftRX;
		}
		LeftRX -= ii;

		SPI_RX_DATA(IsPack, AlignLeft, Nbits, ii, Data, Shift, Shift, Mask, Buf8, Buf16,Buf16Tmp,Reg);

		if (0 != (Reg[SPI_RISR_REG] & 0x08)) {		/* If RX FIFO has overflown, we are toasted		*/
		  #if ((MY_SPI_DEBUG) > 0)
			printf("SPI   [Dev:%d - Slv:%d] - Error - RX FIFO overflow when doing polling\n",
				   Dev, Slv);
			DbgDump = 1;
		  #endif
			Error = -11;
		}

		SPI_EXPIRY_RX(IsExp, IsrState, Expiry);		/* Check if has timed out						*/

	  #if ((MY_SPI_DEBUG) > 0)
		if (IsExp != 0) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - Timeout when doing polling\n", Dev, Slv);
			DbgDump = 1;
		}
	  #endif
	}

	Reg[SPI_SPIENR_REG] = 0;						/* Done, so disable the controller				*/

  #if (((MY_SPI_OPERATION) & 0x00001) != 0)
	OSintBack(IsrState);
  #endif

  #if ((MY_SPI_DEBUG) > 1)							/* Print after the Xfer to not impact polling	*/
	if (XferType == (1U<<2)) {
		printf("SPI   [Dev:%d - Slv:%d] - RX transfer done with DMA\n", Dev, Slv);
	}
	else if (XferType == (1U<<1)) {
		printf("SPI   [Dev:%d - Slv:%d] - RX transfer done through ISRs\n", Dev, Slv);
	}
	else {
		printf("SPI   [Dev:%d - Slv:%d] - RX transfer done with polling\n", Dev, Slv);
	}
  #endif

	SPI_MTX_UNLOCK(Cfg);

  #if ((MY_SPI_DEBUG) > 0)
	if (DbgDump != 0) {
		printf("                          RX data remaining : %u\n", (unsigned)LeftRX);
	}
  #endif

	ii = 0;											/* Set the return value							*/
	if (Error != 0) {								/* -10 : TX FIFO underrun						*/
		ii = Error;									/* -11 : RX FIFO overflow						*/
	}												/* -12 : timeout expiry							*/
	else if (IsExp != 0) {							/* -13 : DMA error #-1							*/
		ii = -12;									/* -14 : DMA error #-2							*/
	}												/*       etc									*/

	return(ii);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: spi_send																				*/
/*																									*/
/* spi_send - send data over the SPI bus															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int spi_send(int Dev, int Slv, const void *Buf, uint32_t Len);								*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : SPI controller number																*/
/*		Slv  : SPI slave index																		*/
/*		Buf  : pointer to the buffer where the frames to send are located							*/
/*		Len  : number of frames to send																*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		MicroWire: the 4 first bytes in Buf holds the control word information:						*/
/*		               Byte #0 : number of bits in the control word									*/
/*		               Byte #1 : un-used															*/
/*		               Byte #2 : MSByte of the control word (ignored if # bits <= 8)				*/
/*		               Byte #3 : LSByte of the control word											*/
/*				   The real payload starts at the fifth byte in Buf									*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SPI_ADD_PREFIX(spi_send) (int Dev, int Slv, const void *Buf, uint32_t Len)
{
const uint8_t  *Buf8;								/* Same as Buf, for 8 bit read					*/
const uint16_t *Buf16;								/* Same as Buf, for 16 bit read					*/
uint16_t        Buf16Tmp[1];						/* When reading to 16 bit unaligned memory		*/
SPIcfg_t *Cfg;										/* De-reference array for faster access			*/
uint32_t  Data;										/* uWire control word							*/
int       Error;									/* To track TX FIFO under-run					*/
int       Expiry;									/* Expiry value of the RTOS timer tick counter	*/
int       FirstLen;									/* Number of frames to send here (not ISR)		*/
int       ii;										/* Multi-purpose								*/
int       IsExp;									/* If the processing has timed-out				*/
int       LeftTX;									/* Number of frames left to send				*/
int       Nbits;									/* Number of bits per frame						*/
int       ReMap;									/* Device # remapped							*/
volatile uint32_t *Reg;								/* Base pointer to SPI controller registers		*/
int       Shift;									/* Shift to apply if data is left aligned		*/
int       TimeOut;									/* Timeout value in # RTOS timer ticks			*/
int       uWire;									/* If the operation is a MicroWire transfer		*/
int       uWireNbit;								/* With uWire: the number of ctrl bits			*/
#if (((MY_SPI_OPERATION) & 0x00100) != 0)
 int      IsrState;									/* ISR state before disabling them				*/
#endif
#if (((MY_SPI_OPERATION) & 0x00600) != 0)
  unsigned int UseISR;								/* If using ISRs for transfer and/or EOT		*/
#endif
#if (((MY_SPI_OPERATION) & 0x00400) != 0)
 uint32_t CfgOp[8];									/* DMA configuration commands					*/
 int      DMAid;									/* ID of the DMA transfer						*/
 uint32_t DmaLen;									/* Number of frames to send using the DMA		*/
#endif
#if (((MY_SPI_OPERATION) & 0x10000) != 0)
 int      AlignLeft;								/* If the packed data is left aligned			*/
 int      IsPack;									/* If has to pack the received data				*/
#endif
#if ((MY_SPI_DEBUG) > 0)
 int DbgDump;
#endif
#if ((MY_SPI_DEBUG) > 1)
 unsigned int XferType;
#endif

  #if ((MY_SPI_DEBUG) > 1)
	printf("SPI   [Dev:%d - Slv:%d] Sending %d frames\n", Dev, Slv, (int)Len);
	XferType = 0U;
  #endif

  #if ((MY_SPI_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_SPIcfg[ReMap];						/* This is this device configuration			*/

  #else
	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (MY_SPI_MAX_DEVICES))
	||  (Slv < 0)
	||  (Slv >= (MY_SPI_MAX_SLAVES))) {
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Out of range device / slave\n", Dev, Slv);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by SPI_LIST_DEVICE				*/
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Cannot remap device\n", Dev, Slv);
	  #endif
		return(-2);
	}

	if (Buf == (void *)NULL) {
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - NULL buffer\n", Dev, Slv);
	  #endif
		return(-3);
	}

	Cfg = &g_SPIcfg[ReMap];							/* This is this device configuration			*/
	if ((g_CfgIsInit == 0)
	||  (0 == (Cfg->SlvInit & (1U << Slv)))) {		/* The controller is not initialized			*/
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Device / slave not initialized\n", Dev, Slv);
	  #endif
		return(-4);
	}
  #endif

	uWire = ((Cfg->Mode[Slv] & 3) == (SPI_PROTO_UWIRE));

  #if ((MY_SPI_ARG_CHECK) == 0)
	if (uWire != 0) {
		if ((Buf8[0] == 0)							/* Controller can only send from 1 to 16 bits	*/
		||  (Buf8[0]) > 16)) {
		  #if ((MY_SPI_DEBUG) > 0)
		   printf("SPI   [Dev:%d - Slv:%d] - Error - # of bits for uWire control word out of range\n",
			       Dev, Slv);
		  #endif
			return(-5);
		}		
	}
  #endif

	if ((Len == 0)									/* Sending nothing is not considered an error	*/
	&&  (uWire == 0)) {								/* MicroWire can send control word and no data	*/
		return(0);
	}

													/* Check if ACP remapping is requested			*/
	if (((Cfg->Mode[Slv] & (SPI_CFG_CACHE_TX(1,0))) == SPI_CFG_CACHE_TX(1,0))
	&&  ((0xC0000000 & (uintptr_t)Buf) == 0x80000000)) {	/* Check the address 2 MSBits			*/
		Buf8 = (uint8_t *)((0x3FFFFFFF & (uintptr_t)Buf)	/* When is ACP, remap to ACP address	*/
		                 | (0xC0000000 & (Cfg->Mode[Slv])));
	}
	else {
		Buf8 = (uint8_t *)Buf;
	}

	Data    = 0;
	Error   = 0;
	IsExp   = 0;
	LeftTX  = Len;
	Nbits   = Cfg->Nbits[Slv];
	Reg     = Cfg->HW;
	Shift   = Cfg->Shift[Slv];						/* Timeout: use *8 for margin of error			*/
	TimeOut = (Len * Nbits *Cfg->NsecBit[Slv])		/* Timeout waiting for the transfer done		*/
	        >> 17;		         					/* Doing /(128*1024) instead of *8/(1000*1000)	*/
	TimeOut = OS_MS_TO_MIN_TICK(TimeOut, 3);		/* Convert ms to RTOS timer tick count			*/
  #if (((MY_SPI_OPERATION) & 0x00600) != 0)
	UseISR  = 0U;
  #endif

	ii = (uWire != 0)
	   ? 1
	   : 0;
	FirstLen = Len;
	if (FirstLen > ((MY_SPI_TX_FIFO_SIZE)-ii)) {	/* uWire needs to write the control word		*/
		FirstLen = (MY_SPI_TX_FIFO_SIZE)-ii;		/* When > TX FIFO, it is the size of TX FIFO	*/
	}

  #if (((MY_SPI_OPERATION) & 0x10000) != 0)			/* When packed data supported, pull the info	*/
	IsPack    = Cfg->Mode[Slv]
	          & (SPI_DATA_PACK);
	AlignLeft = Cfg->Mode[Slv]
	          & (SPI_ALIGN_LEFT);
  #endif
  #if ((MY_SPI_DEBUG) > 0)
	DbgDump = 0;
  #endif

	uWireNbit = 0;
	if (uWire != 0)  							{	/* uWire: prepare the control word				*/
		uWireNbit = Buf8[0];
		Data      = Buf8[2];						/* MSByte of the control word					*/
		Data   <<= 8;								/* No ANDing needed as all data here is uint	*/
		Data    |= Buf8[3];							/* LSByte of the control word					*/
		if ((Cfg->Mode[Slv] & (SPI_ALIGN_LEFT)) != 0) {	/* If is left aligned, right aligned it as	*/
			ii = Buf8[0];							/* it's what the controller needs				*/
			if (ii <= 8) {							/* Get # bits in control word					*/
				Data >>= 8-ii;
			}
			else {
				Data >>= 16-ii;
			}
		}
		Buf8 = Buf8 + 4;							/* Skip control word information				*/
	  #if (((MY_SPI_OPERATION) & 0x00400) != 0)
		Buf = (void *)(4 + (uintptr_t)Buf);			/* DMA use Buf									*/
	  #endif
	}

	Buf16 = (const uint16_t *)Buf8;					/* Use updated Buf in case is uWire				*/

	if (0 != SPI_MTX_LOCK(Cfg)) {					/* Now we need to have exclusive access			*/
		return(-6);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	Reg[SPI_SPIENR_REG] = 0;						/* Must be disable to be re-programmed			*/
	Reg[SPI_CTRLR0_REG] = Cfg->Crtl0[Slv]
	                    | (1U << 8)					/* Set in transmission mode only				*/
	                    | (((uint32_t)(uWireNbit-1)) << 12);	/* uWire: # bits in ctrl word		*/
	Reg[SPI_MWCR_REG]   = 3							/* Always perform sequential write transfer		*/
	                    | ((Cfg->Mode[Slv] & (SPI_UWIRE_HS))
	                       ? 4						/* If handshake was requested at initialization	*/
	                       : 0);
	Reg[SPI_BAUDR_REG]  = Cfg->BaudR[Slv];			/* Set baud rate for this slave					*/
	Reg[SPI_TXFTLR_REG] = 0;						/* Polling uses TX FIFO count and not the SR bit*/
	Reg[SPI_DMACR_REG]  = 0;						/* Assume DMA not used							*/
	Reg[SPI_SER_REG]    = 1U << Slv;				/* Set the target slave							*/
	Reg[SPI_IMR_REG]    = 0;						/* Default, no ISRs								*/


  #if (((MY_SPI_OPERATION) & 0x00200) != 0)
	Cfg->ISRnbits       = Nbits;					/* In case pack data is used, give info to ISR	*/
	Cfg->ISRshift       = Shift;
	Cfg->ISRdumTX       = 0;
	Reg[SPI_TXFTLR_REG] = (Len <= (MY_SPI_TX_FIFO_SIZE))
	                    ? 0							/* When all fits, watermark is when empty		*/
	                    : Cfg->ISRwaterRX;			/* Not all fit, use specified watermark			*/

	if ((Cfg->Mode[Slv] & (SPI_EOT_ISR)) != 0) {	/* If using ISR for EOT							*/
		UseISR = 4U;
	}
	if ((Len >= Cfg->ISRminTX)						/* If enough for transfer						*/
	&&  (Len > FirstLen)							/* And more than initial TX FIFO filling		*/
	&&  ((Cfg->Mode[Slv] & (SPI_XFER_ISR)) != 0)) {	/* And can use ISR for the  transfers			*/
		UseISR |= 2U;								/* Xfering with ISR will block but when "done"	*/
	}												/* it isn't real EOT. Keep the EOT info in case	*/
													/* case the DMA is used							*/
   #if (((MY_SPI_OPERATION) & 0x00002) != 0)		/* ISRs are used but we don't receive anything	*/
	Cfg->ISRleftRX = 0;								/* Make sure the ISRs does nothing with RX		*/
   #endif
  #endif

  #if (((MY_SPI_OPERATION) & 0x00400) != 0)			/* DMA can be used								*/
	DMAid      = 0;									/* If DMA is not used, it will remain == 0		*/
	DmaLen     = 0;									/* To remove compiler warning					*/
	if (((Cfg->Mode[Slv] & (SPI_XFER_DMA)) != 0)	/* Is this slave set-up to use the DMA?			*/
	&&  (Len >= Cfg->DMAminTX)						/* Enough data to use the ISRs?					*/
	&&  (Len >  FirstLen)) {						/* No need for DMA if all fit in TX FIFO		*/
		ii = (3*(MY_SPI_TX_FIFO_SIZE))/4;			/* Try to transfer the largest busrt length		*/
		if (ii > (DMA_MAX_BURST)) {					/* and at the same time keep the FIFO as full	*/
			ii = DMA_MAX_BURST;						/* as possible to maximize the tranfer rate		*/
		}
		if (((MY_SPI_TX_FIFO_SIZE)-ii) > Len) {
			ii = (MY_SPI_TX_FIFO_SIZE)
			   - Len;
		}
		DmaLen               = Len;					/* Number of frames to send with the DMA		*/
		Reg[SPI_DMATDLR_REG] = (MY_SPI_TX_FIFO_SIZE)/* Optimal DMA watermark						*/
		                     - ii;
		Reg[SPI_DMACR_REG]   = 0x2;					/* Enable DMA TX operations						*/
		FirstLen             = 0;					/* This skips data initial sending with polling	*/
		UseISR              &= 4;					/* No ISR tranfers but keep EOT (TX empty)		*/
	}												/* Threshold is set in EOT section				*/
  #endif

	Reg[SPI_SPIENR_REG] = 1;						/* Enable the controller						*/
	g_Dummy             = Reg[SPI_ICR_REG];			/* Clear all overflow/underflow interrupts		*/

  #if (((MY_SPI_OPERATION) & 0x00400) != 0)			/* DMA is used									*/
	if (DmaLen != 0) {								/* Using no wait as when DMA done, the frames	*/
													/* have all landed in TX FIFO but the transfer	*/
	  #if ((MY_SPI_DEBUG) > 1)						/* is not yet done								*/
		XferType = 1U << 10;						/* Use the same bits as in MY_SPI_OPERATION		*/
	  #endif

		ii = 0;
		CfgOp[ii++] = DMA_CFG_NOWAIT;

	  #if ((MY_SPI_REMAP_LOG_ADDR) != 0)
		CfgOp[ii++] = DMA_CFG_LOGICAL_DST;
		CfgOp[ii++] = DMA_CFG_LOGICAL_SRC;
	  #endif

		if (uWire != 0) {							/* When not using uWire, we can let the DMA		*/
			CfgOp[ii++] = DMA_CFG_NOSTART;			/* start immediately.  With uWire need to write	*/
		}											/* The control word first						*/

		if (((Cfg->Mode[Slv] & (SPI_CFG_CACHE_TX(1,0))) == SPI_CFG_CACHE_TX(1,0))
		&& ((0xC0000000 & (uintptr_t)Buf) == 0x80000000)) {		/* If ACP remapping is used			*/
		  #if ((MY_SPI_REMAP_LOG_ADDR) != 0)
			ii--;									/* Do this to overwrite DMA_CFG_LOGICAL_SRC		*/
		  #endif
			CfgOp[ii++] = DMA_CFG_NOCACHE_SRC;
		}
		else if (Cfg->Mode[Slv] == SPI_CFG_CACHE_TX(0,0)) {		/* No cache maint requested			*/
			CfgOp[ii++] = DMA_CFG_NOCACHE_SRC;
		}
		CfgOp[ii] = 0;								/* NO start in case of uWire					*/
													
		ii = (Nbits <= 8)							/* Number of byte for each transfer				*/
		   ? 1
		   : 2;

		Error = dma_xfer(DMA_DEV,
		                 (void *)&Reg[SPI_DR_REG], 0, g_DMAchan[Dev][0],
			             (void *)Buf, ii, -1,
		                 ii, (MY_SPI_TX_FIFO_SIZE)-Reg[SPI_DMATDLR_REG], DmaLen,
			             DMA_OPEND_NONE, NULL, 0,
			             &CfgOp[0], &DMAid, TimeOut);

		if (uWire != 0) {							/* uWire needs to have the control word written	*/
			uWire = 0;								/* first then the DMA can start					*/
			Reg[SPI_DR_REG] = Data;
			dma_start(DMAid);
		}

		LeftTX = 0;									/* The DMA transfers everything					*/

		if (Error != 0) {							/* Something went wrong setting up the DMA		*/
		  #if ((MY_SPI_DEBUG) > 0)
			printf("SPI   [Dev:%d - Slv:%d] - Error -  TX DMA reported error #%d\n",
			        Dev, Slv, Error);
		  #endif
			Error -= 12;							/* Re-adjust error value for SPI context report	*/
		}
		if (UseISR == 0) {							/* If is 0 here, then ISR EOT is used and it is	*/
			dma_wait(DMAid);						/* much better as it unblock when all TXed		*/
		}											/* Instead of when all frames have been written	*/
	}												/* in the TX FIFO								*/
  #endif

	ii = FirstLen;									/* First, fill the TX FIFO						*/
	if (ii > LeftTX) {								/* ii is used for the number of data frames to	*/
		ii = LeftTX;								/* send											*/
	}
	LeftTX   -= ii;									/* This less # of data to TX with polling/ISR	*/
	FirstLen -= ii;

	if (uWire != 0) {
		Reg[SPI_DR_REG]  = Data;
		Data             = 0;
	}

  #if (((MY_SPI_OPERATION) & 0x00100) != 0)			/* If requested to keep ISR disabled during the	*/
	IsrState = OSintOff();							/* polled transfers								*/
  #endif

	Expiry =  OS_TICK_EXP(TimeOut);					/* This is where the Xfer is considerd stared	*/

	SPI_TX_DATA(IsPack, AlignLeft, Nbits, ii, Data, Shift, Shift, Buf8, Buf16, Buf16Tmp, Reg);

  #if (((MY_SPI_OPERATION) & 0x00600) == 0)
	if (uWire != 0) {								/* Without this, the transfer complete indicator*/
		for (ii=0 ; ii<100 ; ii++) {				/* does not get enough time to get updated		*/
			if ((Reg[SPI_SR_REG] & 1) != 0) {		/* before reaching the end when low number of	*/
				break;								/* frames to send								*/
			}
			g_Dummy++;
		}
	}
  #endif
													/* -------------------------------------------- */
  #if (((MY_SPI_OPERATION) & 0x00200) != 0)			/* Start the ISRs xfers, if they are used		*/
	SEMreset(Cfg->MySema);							/* Make sure there are no pending posting		*/
													/* Done here to cover Xfer and EOT ISRs			*/
	if ((UseISR & 2U)!= 0)  {
	  #if ((MY_SPI_DEBUG) > 1)
		XferType =  1U << 9;						/* Use the same bits as in MY_SPI_OPERATION		*/
	  #endif

		Cfg->ISRleftTX = LeftTX;
		Cfg->ISRbufTX  = (Nbits <= 8)
		               ? (void *)Buf8
   	    			   : (void *)Buf16;

	  #if (((MY_SPI_OPERATION) & 0x10000) != 0)		/* If RX ISR enable, no RX transfers to do		*/
		Cfg->ISRdataTX  = Data;
		Cfg->ISRshiftTX = Shift;
	  #endif

		Cfg->Error         = 0;
		Reg[SPI_IMR_REG]   = 0x01;					/* TX FIFO below watermark interrupt on			*/

		LeftTX  = 0;								/* All data left to TX is done in the ISR		*/
		UseISR &= 4U;								/* Keep info about waiting EOT in ISR			*/

	  #if (((MY_SPI_OPERATION) & 0x00100) != 0)		/* Make sure the ISRs are on					*/
		OSintBack(IsrState);
	  #endif

		IsExp = SEMwait(Cfg->MySema, TimeOut);		/* And wait for the job to be done				*/

		Reg[SPI_IMR_REG]    = 0;					/* Safety in case of timeout					*/

		Error = Cfg->Error;
		LeftTX = Cfg->ISRleftTX;

	  #if ((MY_SPI_DEBUG) > 0)
		if (Error == -10) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - TX FIFO under-run in ISR\n", Dev, Slv);
			DbgDump = 1;
		}
		if (IsExp != 0) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - Timeout waiting for ISR done\n", Dev, Slv);
			DbgDump = 1;
		}
	  #endif
	}
  #endif

	while((LeftTX != 0)								/* Do all the polling required					*/
	&&    (IsExp == 0)) {							/* Error can be !=0 if DMA: DMA sets LeftTX==0	*/
		ii = (MY_SPI_TX_FIFO_SIZE)					/* Get the room left in TX FIFO					*/
		   - Reg[SPI_TXFLR_REG];
		if (ii > LeftTX) {
			ii = LeftTX;
		}
		LeftTX -= ii;

		SPI_TX_DATA(IsPack, AlignLeft, Nbits, ii, Data, Shift, Shift, Buf8, Buf16, Buf16Tmp, Reg);

		SPI_EXPIRY_TX(IsExp, IsrState, Expiry);		/* Check if has timed out						*/

	  #if ((MY_SPI_DEBUG) > 0)
		if (IsExp != 0) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - Timeout when doing polling\n", Dev, Slv);
			DbgDump = 1;
		}
	  #endif
	}

  #if (((MY_SPI_OPERATION) & 0x00200) != 0)			/* If waiting for ISRs to be done				*/
	if ((UseISR != 0U)
	&&  (Error  == 0)
	&&  (IsExp  == 0)
	&&  (Reg[SPI_TXFLR_REG] != 0)) {				/* Do nothing is alread empty					*/

		Reg[SPI_TXFTLR_REG] = 0;					/* EOT when TX FIFO completely empty			*/
		Cfg->ISRleftTX = 0;							/* Nothing left to TX, but ISR on TX empty		*/

	  #if (((MY_SPI_OPERATION) & 0x00100) != 0)
		OSintBack(IsrState);
	  #endif

		Reg[SPI_IMR_REG]    = 0x01;					/* We use the TX FIFO empty interrupt for EOT	*/
 
		IsExp = SEMwait(Cfg->MySema, TimeOut);		/* And wait for the job to be done				*/

		Reg[SPI_IMR_REG] = 0;						/* Safety in case of timeout					*/
		Error = Cfg->Error;

	  #if ((MY_SPI_DEBUG) > 0)
		if (Error == -10) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - TX FIFO under-run in ISR\n", Dev, Slv);
			DbgDump = 1;
		}
		if (IsExp != 0) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - Timeout waiting for ISR done\n", Dev, Slv);
			DbgDump = 1;
		}
	  #endif
	}
  #endif

	while ((Error == 0)								/* Wait for the end of transmission				*/
	&&     (IsExp == 0)								/* Having written all in TX FIFO is not EOT		*/
	&&     ((Reg[SPI_SR_REG] & 1) != 0)) {			/* Done when controller is declared idle		*/

		SPI_EXPIRY_TX(IsExp, IsrState, Expiry);		/* Check if has timed out						*/

	  #if ((MY_SPI_DEBUG) > 0)
		if (IsExp != 0) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - Timeout waiting for end of transfer\n",Dev,Slv);
			DbgDump = 1;
		}
	  #endif
	}

	Reg[SPI_SPIENR_REG] = 0;						/* We're done, so disable the controller		*/

  #if ((MY_SPI_DEBUG) > 1)							/* Print after the Xfer to not impact polling	*/
	if (XferType == (1U<<10)) {
		printf("SPI   [Dev:%d - Slv:%d] - TX transfer done with DMA\n", Dev, Slv);
	}
	else if (XferType == (1U<<9)) {
		printf("SPI   [Dev:%d - Slv:%d] - TX transfer done through ISRs\n", Dev, Slv);
	}
	else {
		printf("SPI   [Dev:%d - Slv:%d] - TX transfer done with polling\n", Dev, Slv);
	}
  #endif

	SPI_MTX_UNLOCK(Cfg);

  #if (((MY_SPI_OPERATION) & 0x00100) != 0)			/* No problems here if was already restored		*/
	OSintBack(IsrState);							/* when RX or TX ISRs were enabled				*/
  #endif

  #if (((MY_SPI_OPERATION) & 0x00400) != 0)
	if (DMAid != 0) {								/* In case dma_wait() was not used when EOT		*/
		dma_kill(DMAid);							/* waited on in ISR								*/
	}
  #endif

  #if ((MY_SPI_DEBUG) > 0)
	if (DbgDump != 0) {
		printf("                          TX data remaining : %u\n", (unsigned)LeftTX);
	}
  #endif

	ii = 0;											/* Set the return value							*/
	if (Error != 0) {								/* -10 : TX FIFO underrun						*/
		ii = Error;									/* -11 : RX FIFO overflow						*/
	}												/* -12 : timeout expiry							*/
	else if (IsExp != 0) {							/* -13 : DMA error #-1							*/
		ii = -12;									/* -14 : DMA error #-2							*/
	}												/*       etc									*/


	return(ii);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: spi_send_recv																			*/
/*																									*/
/* spi_send_recv - send and receive data over the SPI bus											*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int spi_send_recv(int Dev, int Slv, const void *BufTX, uint32_t LenTX, void *BufRX,			*/
/*		                  uint32_t LenRX);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev   : SPI controller number																*/
/*		Slv   : SPI slave index																		*/
/*		BufTX : pointer to the buffer where the frames to send are located							*/
/*		LenTX : number of frames to send															*/
/*		BufRX : pointer to the buffer where the received frames will be deposited					*/
/*		LenRX : number of frames to receive															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SPI_ADD_PREFIX(spi_send_recv) (int Dev, int Slv, const void *BufTX, uint32_t LenTX, void *BufRX,
                                   uint32_t LenRX)
{
uint8_t        *Buf8RX;								/* Same as BufRX, for 8 bit memory write		*/
const uint8_t  *Buf8TX;								/* Same as BufTX, for 8 bit menory write		*/
uint16_t       *Buf16RX;							/* Same as BufRX, for 16 bit memory write		*/
const uint16_t *Buf16TX;							/* Same as BufTX, for 16 bit memory read		*/
uint16_t        Buf16Tmp[1];						/* When accessing to 16 bit unaligned memory	*/
SPIcfg_t *Cfg;										/* De-reference array for faster access			*/
int       DumTX;									/* When non-EEPROM LenTX < LenRX				*/
int       Error;									/* To track FIFO: RX overflow & TX underflow	*/
int       Expiry;									/* Expiry value of the RTOS timer tick counter	*/
int       FirstLen;									/* # of write to fill the TX FIFO (DMA is 0)	*/
int       ii;										/* Multi-purpose								*/
int       IsExp;									/* If the processing has timed-out				*/
uint32_t  LeftRX;									/* Number of frames left to be received			*/
uint32_t  LeftTX;									/* Number of frames left to be transmitted		*/
int       Nbits;									/* Number of bits per frame						*/
int       ReMap;									/* Device # remapped							*/
volatile uint32_t *Reg;								/* Base pointer to SPI controller registers		*/
int       Shift;									/* Shift to apply to merge packed data			*/
int       TimeOut;									/* Timeout value in # RTOS timer ticks			*/
#if (((MY_SPI_OPERATION) & 0x10000) != 0)
 int      AlignLeft;								/* If the packed data is left aligned			*/
 uint32_t DataRX;									/* Shift register fro RX						*/
 uint32_t DataTX;									/* Shift register fro TX						*/
 int      IsPack;									/* If has to pack the data						*/
 int      MaskRX;									/* Mask to isolate the valid bits recived		*/
 int      ShiftRX;									/* Shift to apply on RX data					*/
 int      ShiftTX;									/* Shift to apply on TX data					*/
#endif
#if (((MY_SPI_OPERATION) & 0x00202) != 0)
 unsigned int UseISR;								/* If the RX and/or TX ISR is used, eitehr for	*/
#endif												/* transfers and/or DMA EOT						*/
#if (((MY_SPI_OPERATION) & 0x00101) != 0)
 int      IsrState;									/* ISR state before disabling them				*/
#endif
#if (((MY_SPI_OPERATION) & 0x00404) != 0)
 uint32_t CfgOp[8];									/* DMA configuration commands					*/
 uint32_t DmaLenRX;									/* When != 0, length xfered by DMA RX			*/
 int      DMArxID;									/* ID of the DMA transfer						*/
 int      DMAstart;									/* How the DMAs are started						*/
 uint32_t DmaLenTX;									/* When != 0, length xfered by DMA TX			*/
 int      DMAtxID1;									/* ID of the 1st DMA transfer					*/
 int      DMAtxID2;									/* ID of the 2nd DMA transfer					*/
#endif
#if ((MY_SPI_DEBUG) > 0)
 int DbgDump;
#endif
#if ((MY_SPI_DEBUG) > 1)
 unsigned int XferType;
#endif

  #if ((MY_SPI_DEBUG) > 1)
	printf("SPI   [Dev:%d - Slv:%d] Sending %d and receiving %d frames\n", Dev, Slv, (int)LenTX,
	                                                                                (int)LenRX);
	XferType = 0U;
  #endif

  #if ((MY_SPI_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_SPIcfg[ReMap];						/* This is this device configuration			*/

  #else
	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (MY_SPI_MAX_DEVICES))
	||  (Slv < 0)
	||  (Slv >= (MY_SPI_MAX_SLAVES))) {
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Out of range device / slave\n", Dev, Slv);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by SPI_LIST_DEVICE				*/
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Cannot remap device\n", Dev, Slv);
	  #endif
		return(-2);
	}

	if ((BufRX == (void *)NULL)
	||  (BufTX == (void *)NULL)) {
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - NULL buffer\n", Dev, Slv);
	  #endif
		return(-3);
	}

	Cfg = &g_SPIcfg[ReMap];							/* This is this device configuration			*/
	if ((g_CfgIsInit == 0)
	||  (0 == (Cfg->SlvInit & (1U << Slv)))) {		/* The controller is not initialized			*/
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Device / slave not initialized\n", Dev, Slv);
	  #endif
		return(-4);
	}

	if (((Cfg->Mode[Slv] & (SPI_TX_RX_EEPROM)) != 0)/* When using EEPROM MODE						*/
	&&  (LenRX > 65536)) {							/* Maximum frames the controller can receive	*/
	  #if ((MY_SPI_DEBUG) > 0)						/* Not possible to extend with multiple recv	*/
		printf("SPI   [Dev:%d - Slv:%d] - Error - # of frames exceed controller capabilities\n",
		       Dev, Slv);
	  #endif
		return(-5);
	}

	if ((Cfg->Mode[Slv] & 3) == (SPI_PROTO_UWIRE)) {	/* Cant's do send & receive with MicroWire	*/
	  #if ((MY_SPI_DEBUG) > 0)						/* Not possible to extend with multiple recv	*/
			printf("SPI   [Dev:%d - Slv:%d] - Error - Cannot perform send + receive with uWire\n",
			       Dev, Slv);
	  #endif
		return(-6);
	}

  #endif

	if ((LenTX == 0)								/* Sending and receiving nothing is not			*/
	&&  (LenRX == 0)) {								/* considered an error							*/
		return(0);
	}

	if (((Cfg->Mode[Slv] & (SPI_CFG_CACHE_RX(1,0))) != 0)	/* != 0 when ACP is to be used			*/
	&&  ((0xC0000000 & (uintptr_t)BufRX) == 0x80000000)) {	/* Check the address 2 MSBits			*/
		Buf8RX = (uint8_t *)((0x3FFFFFFF & (uintptr_t)BufRX)/* When is ACP, remap to ACP address	*/
		                   | (0xC0000000 & (Cfg->Mode[Slv])));
	}
	else {
		Buf8RX = (uint8_t *)BufRX;
	}

	if (((Cfg->Mode[Slv] & (SPI_CFG_CACHE_TX(1,0))) != 0)	/* != 0 when ACP is to be used			*/
	&&  ((0xC0000000 & (uintptr_t)BufTX) == 0x80000000)) {	/* Check the address 2 MSBits			*/
		Buf8TX = (uint8_t *)((0x3FFFFFFF & (uintptr_t)BufTX)/* When is ACP, remap to ACP address	*/
		                   | (0xC0000000 & (Cfg->Mode[Slv])));
	}
	else {
		Buf8TX = (const uint8_t *)BufTX;
	}

	Buf16RX   = (uint16_t *)Buf8RX;
	Buf16TX   = (const uint16_t *)Buf8TX;
	Error     = 0;
	IsExp     = 0;
	LeftRX    = LenRX;
	LeftTX    = LenTX;
	Nbits     = Cfg->Nbits[Slv];
	Reg       = Cfg->HW;
	Shift     = Cfg->Shift[Slv];
  #if (((MY_SPI_OPERATION) & 0x00202) != 0)
	UseISR        = 0U;
	Cfg->ISRnbits = Nbits;
	Cfg->ISRshift = Shift;
  #endif
  #if (((MY_SPI_OPERATION) & 0x00404) != 0)
	DmaLenRX = 0;									/* Assume DMA not used for RX					*/
	DmaLenTX = 0;									/* Assume DMA not used for TX					*/
	DMArxID  = 0;
	DMAstart = 0;
	DMAtxID1 = 0;
	DMAtxID2 = 0;
  #endif

	DumTX   = 0;									/* When not EEPROM mode, must TX dummies if		*/
	if ((Cfg->Mode[Slv] & (SPI_TX_RX_EEPROM)) == 0) {	/* more frames to receive than to transmit	*/
		if (LenRX > LenTX) {
			DumTX = LenRX
			      - LenTX;
		}
	}

	FirstLen = LenTX								/* FirstLen is the number of frames to send		*/
	         + DumTX;								/* before letting the ISRs to transfer			*/
	TimeOut = (FirstLen * Nbits *Cfg->NsecBit[Slv])	/* Timeout waiting for the transfer done		*/
	        >> 17;		         					/* Doing /(128*1024) instead of *8/(1000*1000)	*/
	TimeOut = OS_MS_TO_MIN_TICK(TimeOut, 3);		/* Convert ms to RTOS timer tick count			*/
													/* Timeout: use *8 for margin of error			*/
	if (FirstLen > (MY_SPI_TX_FIFO_SIZE)) {
		FirstLen = MY_SPI_TX_FIFO_SIZE;				/* When > TX FIFO, it is the size of TX FIFO	*/
	}

  #if (((MY_SPI_OPERATION) & 0x10000) != 0)			/* When packed data is supported				*/
	IsPack    = Cfg->Mode[Slv]
	          & (SPI_DATA_PACK);
	AlignLeft = Cfg->Mode[Slv]
	          & (SPI_ALIGN_LEFT);
	DataRX    = 0;
	DataTX    = 0;
	MaskRX    = (1U << Nbits)
	          - 1;
	ShiftRX   = 0;
	ShiftTX   = 0;
   #if (((MY_SPI_OPERATION) & 0x00202) != 0)		/* If ISRs are supported						*/
	Cfg->ISRalignLeft = AlignLeft;
	Cfg->ISRisPack    = IsPack;
   #endif
  #endif
  #if ((MY_SPI_DEBUG) > 0)
	DbgDump = 0;
  #endif

	if (0 != SPI_MTX_LOCK(Cfg)) {					/* Now we need to have exclusive access			*/
		return(-7);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	Reg[SPI_SPIENR_REG] = 0;						/* Must be disable to be re-programmed			*/
	Reg[SPI_CTRLR0_REG] = Cfg->Crtl0[Slv]
	                    | Cfg->SndRcv[Slv];			/* Set regular (TX&RX) or EEPROM mode			*/
	Reg[SPI_CTRLR1_REG] = LenRX-1;					/* Number of frames to read (needed by EEPROM)	*/
	Reg[SPI_BAUDR_REG]  = Cfg->BaudR[Slv];			/* Set baud rate for this slave					*/
	Reg[SPI_RXFTLR_REG] = 0;						/* Use default threshold of 0					*/
	Reg[SPI_TXFTLR_REG] = 0;						/* Use default threshold of 0					*/
	Reg[SPI_DMACR_REG]  = 0;						/* Assume DMA not used							*/
	Reg[SPI_SER_REG]    = 1U << Slv;				/* Set the target slave							*/
	Reg[SPI_IMR_REG]    = 0;						/* Default, no ISRs								*/

  #if (((MY_SPI_OPERATION) & 0x00002) != 0)			/* Set the watermark if using ISRs for RX		*/
	if (((Cfg->Mode[Slv] & (SPI_XFER_ISR)) != 0)	/* Is this slave set-up to use the ISRs?		*/
	&&  (LenRX >  Cfg->ISRwaterRX)					/* ISR only raised when # RX FIFO > Watermark	*/
	&&  (LenRX >= Cfg->ISRminRX)) {					/* If enough data to use IRSs					*/
		Reg[SPI_RXFTLR_REG] = Cfg->ISRwaterRX;		/* Using ISR for EOT is useless for RX			*/
		UseISR |= 1U;
	}
  #endif

  #if (((MY_SPI_OPERATION) & 0x00200) != 0)			/* Set the watermark if using ISRs for TX		*/
	ii = (LenTX+DumTX);								/* The first sending is an initial write busrt	*/
	if (((Cfg->Mode[Slv] & (SPI_XFER_ISR)) != 0)	/* Is this slave set-up to use the ISRs?		*/
	&&  (ii >= Cfg->ISRminTX)						/* Enough data to use the ISRs?					*/
	&&  (ii >  (MY_SPI_TX_FIFO_SIZE))) {			/* No need for ISR if all fit in TX FIFO		*/
		Reg[SPI_TXFTLR_REG] = Cfg->ISRwaterTX;		/* Set TX FIFO watermark for interrupts			*/
		UseISR |= 2U;
	}
  #endif

  #if (((MY_SPI_OPERATION) & 0x00202) != 0)			/* Can only use ISR for EOT if ISRs are used	*/
	if (((Cfg->Mode[Slv] & (SPI_EOT_ISR)) != 0)
	&&  (UseISR != 0)) {
		UseISR |= 4U;
	}
  #endif

  #if (((MY_SPI_OPERATION) & 0x00004) != 0)			/* DMA can be used								*/
	if (((Cfg->Mode[Slv] & (SPI_XFER_DMA)) != 0)	/* Is this slave set-up to use the DMA?			*/
	&&  (LenRX >= Cfg->DMAminRX)					/* Enough data to use the DMA?					*/
	&&  (LenRX >= (MY_SPI_RX_FIFO_SIZE))) {			/* No need for DMA if all fit in RX FIFO		*/
		ii  = (MY_SPI_RX_FIFO_SIZE)-1;				/* Figure out the largest useable burst length	*/
		if (ii > (DMA_MAX_BURST)) {					/* Maximizing the burst length maximises the	*/
			ii = DMA_MAX_BURST;						/* transfer rate								*/
		}
		if (ii > LenRX) {
			ii = LenRX;
		}
		DmaLenRX             = LenRX;
		Reg[SPI_DMARDLR_REG] = ii-1;				/* Watermark: trigger when #RX FIFO > watermark	*/
		Reg[SPI_DMACR_REG]  |= 0x1;					/* Enable DMA RX operations						*/

	  #if (((MY_SPI_OPERATION) & 0x00002) != 0)
		UseISR &= ~5U;								/* Transfer done with DMA, can't use ISR for RX	*/
	  #endif										/* EOT will be done in RX DMA handling			*/
	}
  #endif

  #if (((MY_SPI_OPERATION) & 0x00400) != 0)			/* DMA can be used								*/
	ii = (LenTX+DumTX);
	if (((Cfg->Mode[Slv] & (SPI_XFER_DMA)) != 0)	/* Is this slave set-up to use the DMA?			*/
	&&  (ii >= Cfg->DMAminTX)						/* Enough data to use the ISRs?					*/
	&&  (ii >  (MY_SPI_TX_FIFO_SIZE))) {			/* No need for ISR if all fit in TX FIFO		*/
		ii = (3*(MY_SPI_TX_FIFO_SIZE))/4;			/* Try to transfer the largest busrt length		*/
		if (ii > (DMA_MAX_BURST)) {					/* and at the same time keep the FIFO as full	*/
			ii = DMA_MAX_BURST;						/* as possible to maximize the tranfer rate		*/
		}
		if (((MY_SPI_TX_FIFO_SIZE)-ii) > (LenTX+DumTX)) {
			ii = (MY_SPI_TX_FIFO_SIZE)
			   - (LenTX+DumTX);
		}

		DmaLenTX             = LenTX + DumTX;		/* Number of frames to send with the DMA		*/
		Reg[SPI_DMATDLR_REG] = (MY_SPI_TX_FIFO_SIZE)/* Optimal DMA watermark						*/
		                     - ii;
		Reg[SPI_DMACR_REG]  |= 0x2;					/* Enable DMA TX operations						*/
		FirstLen             = 0;					/* This skips data initial sending with polling	*/

	  #if (((MY_SPI_OPERATION) & 0x00200) != 0)
		UseISR &= ~2U;								/* Transfer done with DMA, can't use ISR for TX	*/
	  #endif										/* Need to use the DMA EOT						*/
	}
  #endif

	Reg[SPI_SPIENR_REG] = 1;						/* Enable the controller						*/
	for (ii=0 ; ii<(MY_SPI_RX_FIFO_SIZE) ; ii++) {	/* Make sure the RX FIFO is empty				*/
		g_Dummy = Reg[SPI_DR_REG];
	}
	g_Dummy = Reg[SPI_ICR_REG];						/* Clear all overflow/underflow interrupts		*/

  #if (((MY_SPI_OPERATION) & 0x00404) != 0)			/*                       1: RX wait/nostart		*/
	if ((Cfg->Mode[Slv] & (SPI_TX_RX_EEPROM)) != 0) { 	/*                   2: TX wait				*/
		if (DmaLenTX != 0) {						/* EEPROM mode, RX always ends after TX			*/
			DMAstart = 1;							/* Can't block if TX done with ISR/Polling		*/
		}
	}
	else {											/* Not EEPROM mode								*/
		if ((DmaLenRX != 0)							/* Both DMA used, we can block on one of them	*/
		&&  (DmaLenTX != 0)) {						/* Is non-EEPROM here, #TX always >= #RX		*/
			DMAstart = (DmaLenRX == DmaLenTX)
			         ? 1							/* Wait on DMA RX as both are same length		*/
			         : 2;							/* Wait on DMA TX (here, TX always >= RX)		*/
		}
	}
  #endif

  #if (((MY_SPI_OPERATION) & 0x00101) != 0)			/* If requested to keep ISR disabled during the	*/
	IsrState = OSintOff();							/* transfer										*/
  #endif

	Expiry =  OS_TICK_EXP(TimeOut);					/* This is where the Xfer is considerd stared	*/
													/* How to start the DMA: 0: RX/TX no blocking	*/
  #if (((MY_SPI_OPERATION) & 0x00004) != 0)			/* RX DMA is used								*/
	if (DmaLenRX != 0) {							/* Is the RX DMA used?							*/
	  #if ((MY_SPI_DEBUG) > 1)
		XferType |= 1U << 2;						/* Use the same bits as in MY_SPI_OPERATION		*/
	  #endif

		ii = 0;
		CfgOp[ii++] = DMA_CFG_NOWAIT;				/* Waiting done way below, not in dma_xfer()	*/

	  #if ((MY_SPI_REMAP_LOG_ADDR) != 0)
		CfgOp[ii++] = DMA_CFG_LOGICAL_SRC;
		CfgOp[ii++] = DMA_CFG_LOGICAL_DST;
	  #endif

		if (DMAstart == 1) {						/* RX & TX used, TX is blocking on RX EOT		*/
			CfgOp[ii++] = DMA_CFG_WAIT_TRG;			/* Start RX only when TX is OK and started		*/
		}											/* TX DMA sends the start trigger				*/

		if (((Cfg->Mode[Slv] & (SPI_CFG_CACHE_RX(1,0))) == SPI_CFG_CACHE_RX(1,0))
		&& ((0xC0000000 & (uintptr_t)BufRX) == 0x80000000)) {	/* If ACP remapping is used			*/
		  #if ((MY_SPI_REMAP_LOG_ADDR) != 0)
			ii--;									/* Do this to overwrite DMA_CFG_LOGICAL_DST		*/
		  #endif
			CfgOp[ii++] = DMA_CFG_NOCACHE_DST;
		}
		else if (Cfg->Mode[Slv] == SPI_CFG_CACHE_RX(0,0)) {		/* No cache maint requested			*/
			CfgOp[ii++] = DMA_CFG_NOCACHE_DST;
		}
		CfgOp[ii] = 0;								/* End of DMA configuration						*/

		ii = (Nbits <= 8)							/* Number of byte for each transfer				*/
		   ? 1
		   : 2;

		Error = dma_xfer(DMA_DEV,
		                 BufRX, ii, -1,
			             (void *)&Reg[SPI_DR_REG], 0, g_DMAchan[Dev][1],
		                 ii, Reg[SPI_DMARDLR_REG]+1, LenRX,
			             DMA_OPEND_NONE, NULL, 0,
			             &CfgOp[0], &DMArxID, TimeOut);

		LeftRX = 0;									/* This will skip everything about RX			*/

		if (Error != 0) {							/* Something went wrong setting up the DMA		*/
		  #if ((MY_SPI_DEBUG) > 0)
			printf("SPI   [Dev:%d - Slv:%d] - Error -  RX DMA reported error #%d\n",
			        Dev, Slv, Error);
		  #endif
			dma_kill(DMArxID);						/* Make sure the channel is released			*/
			DMArxID = 0;							/* Invalidate the ID							*/
			Error  -= 12;							/* Re-adjust error value for SPI context report	*/
		}
	}
  #endif

  #if (((MY_SPI_OPERATION) & 0x00400) != 0)			/* TX DMA is used								*/
	if ((DmaLenTX != 0)								/* Is the TX DMA used?							*/
	&&  (Error == 0)) {

	  #if ((MY_SPI_DEBUG) > 1)
		XferType |= 1U << 10;						/* Use the same bits as in MY_SPI_OPERATION		*/
	  #endif

		if (DumTX != 0) {							/* Sending dummies (always the same operations)	*/
			ii = 0;
			CfgOp[ii++] = DMA_CFG_WAIT_TRG;			/* Wait for trigger from first TX transfer		*/
			CfgOp[ii++] = DMA_CFG_NOWAIT;			/* Waiting done way below, not in dma_xfer()	*/
			CfgOp[ii++] = DMA_CFG_NOSTART;			/* Real TX will start us						*/
		  #if ((MY_SPI_REMAP_LOG_ADDR) != 0)
			CfgOp[ii++] = DMA_CFG_LOGICAL_SRC;
			CfgOp[ii++] = DMA_CFG_LOGICAL_DST;
		  #endif
			CfgOp[ii++] = 0;						/* End of DMA configuration						*/
			
			Error  = dma_xfer(DMA_DEV,
			                  (void *)&Reg[SPI_DR_REG], 0, g_DMAchan[Dev][0],
				              NULL, 0, -1,			/* Src address set to NULL to read zeros		*/
			                  (Nbits <= 8) ? 1 : 2, (MY_SPI_TX_FIFO_SIZE)-Reg[SPI_DMATDLR_REG], DumTX,
			                  DMA_OPEND_NONE, NULL, (intptr_t)0,
                              &CfgOp[0], &DMAtxID2, TimeOut);
		}

		if (Error == 0) {							/* This is the real data to send				*/
			ii = 0;									/* Index to use after if {} else if {} else {}	*/
			CfgOp[ii++] = DMA_CFG_NOWAIT;			/* Waiting done way below, not in dma_xfer()	*/
			CfgOp[ii++] = ((Cfg->Mode[Slv] & (SPI_EOT_ISR)) != 0)
		                ? DMA_CFG_EOT_ISR			/* Use the requested type of blocking			*/
			            : DMA_CFG_EOT_POLLING;

			if (DMAstart == 1) {					/* RX & TX used, TX blocks on RX EOT			*/
				CfgOp[ii++] = DMA_CFG_TRG_ON_START(DMArxID);/* When TX starts, RX started too		*/
				CfgOp[ii++] = DMA_CFG_WAIT_ON_END(DMArxID);	/* Block on RX EOT						*/
			}

			if (DumTX != 0) {						/* Trigger the DumTX start when I'm done		*/
				CfgOp[ii++] = DMA_CFG_TRG_ON_END(DMAtxID2);
			}

		  #if ((MY_SPI_REMAP_LOG_ADDR) != 0)
			CfgOp[ii++] = DMA_CFG_LOGICAL_DST;
			CfgOp[ii++] = DMA_CFG_LOGICAL_SRC;
		  #endif

			if (((Cfg->Mode[Slv] & (SPI_CFG_CACHE_TX(1,0))) == SPI_CFG_CACHE_TX(1,0))
			&& ((0xC0000000 & (uintptr_t)BufTX) == 0x80000000)) {	/* If ACP remapping is used		*/
			  #if ((MY_SPI_REMAP_LOG_ADDR) != 0)
				ii--;								/* Do this to overwrite DMA_CFG_LOGICAL_SRC		*/
			  #endif
				CfgOp[ii++] = DMA_CFG_NOCACHE_SRC;
			}
			else if (Cfg->Mode[Slv] == SPI_CFG_CACHE_TX(0,0)) {	/* No cache maint requested			*/
				CfgOp[ii++] = DMA_CFG_NOCACHE_SRC;
			}
			CfgOp[ii] = 0;							/* End of DMA configuration						*/

			ii = (Nbits <= 8)						/* Number of byte for each transfer				*/
			   ? 1
			   : 2;

		  #if (((MY_SPI_OPERATION) & 0x00101) != 0)
			if (DMAstart != 0) {					/* DMAstart 1 & 2 are blocking, so get ISR back	*/
				OSintBack(IsrState);
			}
		  #endif

			Error = dma_xfer(DMA_DEV,
			                 (void *)&Reg[SPI_DR_REG], 0, g_DMAchan[Dev][0],
				             BufTX, ii, -1,
			                 ii, (MY_SPI_TX_FIFO_SIZE)-Reg[SPI_DMATDLR_REG], LeftTX,
				             DMA_OPEND_NONE, NULL, 0,
				             &CfgOp[0], &DMAtxID1, TimeOut);
		}

		LeftTX   = 0;								/* This will skip everything about TX			*/
		DumTX    = 0;								/* This will skip everything about Dummies		*/

		if (Error != 0) {							/* Something went wrong setting up the DMA		*/
		  #if ((MY_SPI_DEBUG) > 0)
			printf("SPI   [Dev:%d - Slv:%d] - Error -  TX DMA reported error #%d\n",
			        Dev, Slv, Error);
		  #endif
			Error   -= 12;							/* Re-adjust error value for SPI context report	*/
		}
	}
  #endif

	ii = FirstLen;									/* First, fill the TX FIFO						*/
	if (ii > LeftTX) {								/* ii is used for the number of data frames to	*/
		ii = LeftTX;								/* send											*/
	}
	LeftTX   -= ii;									/* This less # of data to TX with polling/ISR	*/
	FirstLen -= ii;
	DumTX    -= FirstLen;							/* FirstLen is never > LeftTX+DumTX				*/

	SPI_TX_DATA(IsPack, AlignLeft, Nbits, ii, DataTX, ShiftTX, Shift, Buf8TX, Buf16TX, Buf16Tmp, Reg);

	for ( ; FirstLen>0 ; FirstLen--) {				/* Send dummies after TX data if any			*/
		Reg[SPI_DR_REG] = 0;
	}
													/* -------------------------------------------- */
  #if (((MY_SPI_OPERATION) & 0x00202) != 0)			/* Start the ISRs, if they are used				*/
	if ((UseISR != 0U)								/* If either RX or TX ISR is to be used			*/
	&&  (Error  == 0)) {
		ii  = 0;									/* ii is used later on to finish RX reading		*/

	  #if (((MY_SPI_OPERATION) & 0x00002) != 0)
		if ((LeftRX != 0)
		&&  ((UseISR & 1U) != 0U)) { 

		  #if ((MY_SPI_DEBUG) > 1)
			XferType |= 1U << 1;					/* Use the same bits as in MY_SPI_OPERATION		*/
		  #endif

			ii             |= 0x10;					/* RX FIFO above watermark interrupt on			*/
			Cfg->ISRleftRX  = LeftRX;
			Cfg->ISRbufRX   = (Nbits <= 8)
			                ? (void *)Buf8RX
			                : (void *)Buf16RX;
			LeftRX          = 0;					/* Will get real left over when ISR is done		*/

		   #if (((MY_SPI_OPERATION) & 0x10000) != 0)
			Cfg->ISRmaskRX  = MaskRX;
			Cfg->ISRdataRX  = DataRX;
			Cfg->ISRshiftRX = ShiftRX;
		   #endif
		}
	  #endif

	  #if (((MY_SPI_OPERATION) & 0x00200) != 0)
		if (((LeftTX+DumTX) != 0)
		&&  ((UseISR & 2U) != 0U)) { 

		  #if ((MY_SPI_DEBUG) > 1)
			XferType |=  1U << 9;					/* Use the same bits as in MY_SPI_OPERATION		*/
		  #endif

			ii            |= 0x01;					/* TX FIFO below watermark interrupt on			*/
			Cfg->ISRdumTX  = DumTX;
			Cfg->ISRleftTX = LeftTX;
			Cfg->ISRbufTX  = (Nbits <= 8)
			               ? (void *)Buf8TX
       	    			   : (void *)Buf16TX;
			LeftTX         = 0;						/* All data left to TX is done in the ISR		*/
			DumTX          = 0;

		   #if (((MY_SPI_OPERATION) & 0x10000) != 0)
			Cfg->ISRdataTX  = DataTX;
			Cfg->ISRshiftTX = ShiftTX;
		   #endif
		}
	  #endif

		if (ii != 0) {								/* This inform the section for EOT the IMR		*/
			UseISR &= ~4U;							/* register has been programmed					*/
		}

		Cfg->Error         = 0;
		SEMreset(Cfg->MySema);						/* Make sure there are no pending posting		*/
		Reg[SPI_IMR_REG]   = ii;					/* Enable the ISRs								*/

	  #if (((MY_SPI_OPERATION) & 0x00101) != 0)		/* Make sure the ISRs are on					*/
		OSintBack(IsrState);
	  #endif
	}
  #endif

	while(((LeftTX+DumTX+LeftRX) != 0)				/* Do all the polling required					*/
	&&    (Error == 0)
	&&    (IsExp == 0)) {
		if (LeftRX != 0) {							/* RX done through polling						*/
			ii = Reg[SPI_RXFLR_REG];				/* Get how many frames are in the RX FIFO		*/
			if (ii > LeftRX) {
				ii = LeftRX;
			}
			LeftRX -= ii;

			SPI_RX_DATA(IsPack, AlignLeft, Nbits, ii, DataRX, ShiftRX, Shift, MaskRX, Buf8RX,
			            Buf16RX, Buf16Tmp, Reg);

			if (0 != (Reg[SPI_RISR_REG] & 0x08)) {	/* If RX FIFO has overflown, we are toasted		*/
			  #if ((MY_SPI_DEBUG) > 0)
				printf("SPI   [Dev:%d - Slv:%d] - Error - RX FIFO overflow when doing polling\n",
					   Dev, Slv);
				DbgDump = 1;
			  #endif
				Error = -11;
			}
		}

		if (LeftTX != 0) {							/* Real data left to send?						*/
			ii = (MY_SPI_TX_FIFO_SIZE)				/* Get room in TX FIFO							*/
			   - Reg[SPI_TXFLR_REG];
			if (ii > LeftTX) {
				ii = LeftTX;
			}
			LeftTX -= ii;

			SPI_TX_DATA(IsPack, AlignLeft, Nbits, ii, DataTX, ShiftTX, Shift, Buf8TX, Buf16TX,
			            Buf16Tmp, Reg);
		}

		if ((DumTX != 0)							/* Dummies left to send?						*/
		&&  (LeftTX == 0)) {						/* Sent after no more real data to send			*/
			ii = (MY_SPI_TX_FIFO_SIZE)				/* Get room in TX FIFO							*/
			   - Reg[SPI_TXFLR_REG];
			if (ii > DumTX) {
				ii = DumTX;
			}
			DumTX -= ii;

			for ( ; ii>0 ; ii--) {					/* Send the dummies								*/
				Reg[SPI_DR_REG] = 0;
			}
		}

		SPI_EXPIRY_RXTX(IsExp, IsrState, Expiry);	/* Check if has timed out						*/

	  #if ((MY_SPI_DEBUG) > 0)
		if (IsExp != 0) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - Timeout when doing polling\n", Dev, Slv);
			DbgDump = 1;
		}
	  #endif
	}

  #if (((MY_SPI_OPERATION) & 0x00202) != 0)			/* If waiting for ISRs to be done				*/
	if ((UseISR != 0U)
	&&  (Error  == 0)
	&&  (IsExp  == 0)) {
		if ((UseISR & 4U) != 0U) {					/* No ISR transfers had to be done, so IMR reg	*/
		  #if (((MY_SPI_OPERATION) & 0x00200) != 0)	/* hasn't been set-up. Need to do it			*/
			Cfg->ISRleftTX   = 0;
			Cfg->ISRdumTX    = 0;
		  #endif
		  #if (((MY_SPI_OPERATION) & 0x00101) != 0)
			OSintBack(IsrState);
		  #endif
			if ((Cfg->Mode[Slv] & (SPI_TX_RX_EEPROM)) != 0) {
				Reg[SPI_IMR_REG] = 0x10;			/* We use the RX FIFO empty interrupt for EOT	*/
			}
			else {
				Reg[SPI_IMR_REG] = 0x01;			/* We use the TX FIFO empty interrupt for EOT	*/
			}
		}
 
		IsExp = SEMwait(Cfg->MySema, TimeOut);		/* And wait for the job to be done				*/

		Reg[SPI_IMR_REG] = 0;						/* Safety in case of timeout					*/
		Error = Cfg->Error;

	  #if ((MY_SPI_DEBUG) > 0)
		if (Error == -10) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - TX FIFO under-run in ISR\n", Dev, Slv);
			DbgDump = 1;
		}
		if (Error == -11) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - RX FIFO overflow in ISR\n", Dev, Slv);
			DbgDump = 1;
		}
		if (IsExp != 0) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - Timeout waiting for ISR done\n", Dev, Slv);
			DbgDump = 1;
		}
	  #endif

	  #if (((MY_SPI_OPERATION) & 0x00002) != 0)
		if (UseISR != 0U) {							/* TX sends everything. There may be leftover	*/
			Buf8RX  = (void *)Cfg->ISRbufRX;		/* with RX										*/
			Buf16RX = (uint16_t *)Buf8RX;
			LeftRX  = Cfg->ISRleftRX;
		  #if (((MY_SPI_OPERATION) & 0x10000) != 0)
			DataRX  = Cfg->ISRdataRX;				/* Get back Data & Shift updated in the ISR		*/
			ShiftRX = Cfg->ISRshiftRX;
		  #endif
		}
	  #endif
	}
  #endif

   #if (((MY_SPI_OPERATION) & 0x00404) != 0)		/* Make sure the DMA transfers are done			*/
	if ((Error == 0)
	&&  (IsExp == 0)) {

		if (DMAtxID1 != 0) {						/* DMA TX is used, we wait for it to be done	*/
			if (DMAstart == 2) {					/* ==2 when TXlen >= RXlen but TX fills FIFO	*/
				dma_wait(DMArxID);					/* SO RX may not be done when TX DONE.			*/
			}										/* Need to check RX is also done				*/
			IsExp = dma_wait(DMAtxID1);
		}
		else if (DMArxID != 0) {
			 dma_wait(DMArxID);
		}

	  #if ((MY_SPI_DEBUG) > 0)
		if (IsExp != 0) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - Timeout waiting for DMA done\n", Dev, Slv);
		}
	  #endif
	}
  #endif

	while ((Error == 0)								/* Wait for the end of transfer					*/
	&&     (IsExp == 0)								/* Having written all in TX FIFO is not EOT		*/
	&&     ((Reg[SPI_SR_REG] & 1) != 0)) {			/* Done when controller is declared idle		*/

		SPI_EXPIRY_TX(IsExp, IsrState, Expiry);		/* Check if has timed out						*/

	  #if ((MY_SPI_DEBUG) > 0)
		if (IsExp != 0) {
			printf("SPI   [Dev:%d - Slv:%d] - Error - Timeout waiting for end of transfer\n",Dev,Slv);
			DbgDump = 1;
		}
	  #endif
	}
													/* The exchanges over the SPI bus are over		*/
	Reg[SPI_SPIENR_REG] = 0;						/* Done, so disable the controller				*/

  #if ((MY_SPI_DEBUG) > 1)
	if ((XferType & (1U<<10)) != 0U) {
		printf("SPI   [Dev:%d - Slv:%d] - TX transfer done with DMA\n", Dev, Slv);
	}
	else if ((XferType & (1U<<9)) != 0U) {
		printf("SPI   [Dev:%d - Slv:%d] - TX transfer done through ISRs\n", Dev, Slv);
	}
	else {
		printf("SPI   [Dev:%d - Slv:%d] - TX transfer done with polling\n", Dev, Slv);
	}
	if ((XferType & (1U<<2)) != 0U) {
		printf("SPI   [Dev:%d - Slv:%d] - RX transfer done with DMA\n", Dev, Slv);
	}
	else if ((XferType & (1U<<1)) != 0U) {
		printf("SPI   [Dev:%d - Slv:%d] - RX transfer done through ISRs\n", Dev, Slv);
	}
	else {
		printf("SPI   [Dev:%d - Slv:%d] - RX transfer done with polling\n", Dev, Slv);
	}
  #endif

	SPI_MTX_UNLOCK(Cfg);

  #if (((MY_SPI_OPERATION) & 0x00101) != 0)			/* No problems here if was already restored		*/
	OSintBack(IsrState);							/* when RX or TX ISRs were enabled				*/
  #endif

  #if (((MY_SPI_OPERATION) & 0x10000) != 0)			/* Memo whet's left if not multiple of 8 or 16	*/
	if ((IsPack != 0)
	&&  (ShiftRX != 0)) {
		if (Nbits <= 8) {
			if (AlignLeft != 0) {
				DataRX <<= 8-ShiftRX;
			}
			*Buf8RX = DataRX;
		}
		else {
			if (AlignLeft != 0) {
				DataRX <<= 16-ShiftRX;
			}
			*Buf16RX = DataRX;
		}
	}
  #endif

  #if (((MY_SPI_OPERATION) & 0x00404) != 0)			/* In case of error, cancel the DMAs if they	*/
	if (DMArxID != 0) {								/* were in use.  If the transfer was OK then	*/
		dma_kill(DMArxID);							/* dma_wait() has already done the clean-up		*/
	}												/* and dma_kill() becomes a do-nothing			*/
	if (DMAtxID1 != 0) {
		dma_kill(DMAtxID1);
	}
	if (DMAtxID2 != 0) {
		dma_kill(DMAtxID2);
	}
  #endif

  #if ((MY_SPI_DEBUG) > 0)
	if (DbgDump != 0) {
		printf("                          RX data  remaining : %u\n", (unsigned)LeftRX);
		printf("                          TX data  remaining : %u\n", (unsigned)LeftTX);
		printf("                          TX dummy remaining : %u\n", (unsigned)DumTX);
	}
  #endif

	ii = 0;											/* Set the return value							*/
	if (Error != 0) {								/* -10 : TX FIFO underrun						*/
		ii = Error;									/* -11 : RX FIFO overflow						*/
	}												/* -12 : timeout expiry							*/
	else if (IsExp != 0) {							/* -13 : DMA error #-1							*/
		ii = -12;									/* -14 : DMA error #-2							*/
	}												/*       etc									*/

  #if ((MY_SPI_DEBUG) > 0)
	if (ii != 0) {
		printf("SPI   [Dev:%d - Slv:%d] - Error #%d", Dev, Slv, ii);
	}
  #endif

	return(ii);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: spi_set																				*/
/*																									*/
/* spi_set - change the operating parameters														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int spi_set (int Dev, int Slv, int Type, int Val);											*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev   : SPI controller number																*/
/*		Slv   : SPI part index (Slave index)														*/
/*		Type  : type of operation, one of these														*/
/*						SPI_RX_WATER_ISR															*/
/*						SPI_TX_WATER_ISR															*/
/*						SPI_RX_MIN_DMA																*/
/*						SPI_TX_MIN_DMA																*/
/*						SPI_RX_MIN_ISR																*/
/*						SPI_TX_MIN_ISR																*/
/*						SPI_TIMEOUT																	*/
/*		Val   : value to set																		*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SPI_ADD_PREFIX(spi_set) (int Dev, int Slv, int Type, int Val)
{
SPIcfg_t *Cfg;										/* De-reference array for faster access			*/
int       ReMap;									/* Device # remapped							*/


  #if ((MY_SPI_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_SPIcfg[ReMap];						/* This is this device configuration			*/

  #else
	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (MY_SPI_MAX_DEVICES))
	||  (Slv < 0)
	||  (Slv >= (MY_SPI_MAX_SLAVES))) {
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Out of range device / slave\n", Dev, Slv);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by SPI_LIST_DEVICE				*/
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Cannot remap device\n", Dev, Slv);
	  #endif
		return(-2);
	}

	Cfg = &g_SPIcfg[ReMap];							/* This is this device configuration			*/

	switch(Type) {
	case SPI_RX_WATER_ISR:
	case SPI_TX_WATER_ISR:
		if ((Val < 0)
		||  (Val > 100)) {
		  #if ((MY_SPI_DEBUG) > 0)
			printf("SPI   [Dev:%d - Slv:%d] - Error - Watermark value out of range\n", Dev, Slv);
		  #endif
			return(-3);
		}
		break;
	case SPI_RX_MIN_DMA:
	case SPI_TX_MIN_DMA:
		if (Val <= 0) {
		  #if ((MY_SPI_DEBUG) > 0)
			printf("SPI   [Dev:%d - Slv:%d] - Error - min for DMA value out of range\n", Dev, Slv);
		  #endif
			return(-4);
		}
		break;
	case SPI_RX_MIN_ISR:
	case SPI_TX_MIN_ISR:
		if (Val <= 0) {
		  #if ((MY_SPI_DEBUG) > 0)
			printf("SPI   [Dev:%d - Slv:%d] - Error - min for ISR value out of range\n", Dev, Slv);
		  #endif
			return(-4);
		}
		break;
	case SPI_TIMEOUT:
		break;
	default:
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Invalid Type argument\n", Dev, Slv);
	  #endif
		return(-5);
	}
  #endif

	switch(Type) {
	case SPI_RX_WATER_ISR:
		Cfg->ISRwaterRX = 1
		                + ((((MY_SPI_RX_FIFO_SIZE)-4) * Val)/100);
		break;
	case SPI_TX_WATER_ISR:
		Cfg->ISRwaterTX = ((((MY_SPI_TX_FIFO_SIZE)-4) * Val)/100);
		break;
	case SPI_RX_MIN_DMA:
		Cfg->DMAminRX = Val;
		break;
	case SPI_TX_MIN_DMA:
		Cfg->DMAminTX = Val;
		break;
	case SPI_RX_MIN_ISR:
		Cfg->ISRminRX = Val;
		break;
	case SPI_TX_MIN_ISR:
		Cfg->ISRminTX = Val;
		break;
	case SPI_TIMEOUT:
		Cfg->TimeOut = Val;
		break;
	default:
		break;
	}

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: spi_init																				*/
/*																									*/
/* spi_init - initialization to access a SPI bus													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int spi_init(int Dev, int Slv, int Freq, int Nbits, uint32_t Mode);							*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev   : SPI controller number																*/
/*		Slv   : SPI part index (Slave index)														*/
/*		Freq  : SPI bus speed in Hz																	*/
/*		Nbits : number of bits per frame															*/
/*		Mode  : operating configuration																*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SPI_ADD_PREFIX(spi_init) (int Dev, int Slv, int Freq, int Nbits, uint32_t Mode)
{
SPIcfg_t  *Cfg;										/* De-reference array for faster access			*/
int        Div;										/* Clock divisor for the desired baud rate		*/
int        ii;										/* Multi-purpose								*/
int        ReMap;									/* Device # remapped							*/


  #if ((MY_SPI_DEBUG) > 1)
	printf("SPI   [Dev:%d - Slv:%d] Initialization\n", Dev, Slv);
  #endif

  #if ((MY_SPI_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];

  #else
	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (MY_SPI_MAX_DEVICES))
	||  (Slv < 0)
	||  (Slv >= (MY_SPI_MAX_SLAVES))) {
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Out of range device / slave\n", Dev, Slv);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by SPI_LIST_DEVICE				*/
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Unsupported device according to SPI_LIST_DEVICE\n",
		       Dev, Slv);
	  #endif
		return(-2);
	}

	if ((Nbits < 4)									/* The controller handles 4 to 16 bits			*/
	||  (Nbits > 16)) {
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Number of bits per frame out of range\n", Dev, Slv);
	  #endif
		return(-3);
	}

	if (Freq <= 0) {
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - Non-positive bus speed\n", Dev, Slv);
	  #endif
		return(-4);
	}

	ii = ((Mode & (SPI_MASTER|SPI_SLAVE)) == SPI_MASTER);
	if (ii && (g_IsMaster[Dev] == 0)) {				/* Must have correct Device # for master/slave	*/
	  #if ((MY_SPI_DEBUG) > 0)
		printf("SPI   [Dev:%d - Slv:%d] - Error - device # doesn't match master/slave association\n",
		Dev, Slv);
	  #endif
		return(-5);
	}
  #endif

  #if ((MY_SPI_USE_MUTEX) != 0)
	ii = 0;											/* Use the RTOS global mutex until the SPI		*/
	if (g_CfgIsInit == 0) {							/* mutex has been created						*/
		if (0 != MTXlock(G_OSmutex, -1)) {			/* Now we need to have exclusive access			*/
			return(-6);								/* Although infinte wait, if deadlock detection	*/
		}											/* is enable, could return !=0 and not lock		*/
		ii = 1;
	}
  #endif

	if (g_CfgIsInit == 0) {							/* Safety in case BSS isn't zeroed				*/
		memset(&g_SPIcfg[0], 0, sizeof(g_SPIcfg));
		g_CfgIsInit = 1;
	}

	Cfg = &g_SPIcfg[ReMap];							/* This is this device configuration			*/

  #if ((MY_SPI_USE_MUTEX) != 0)
	if (Cfg->MyMutex == (MTX_t *)NULL) {
		Cfg->MyMutex = MTXopen(&g_Names[Dev][0]);
	}
	if (ii != 0) {									/* Lock it and release the RTOS global			*/
		MTXunlock(G_OSmutex);
	}
  #endif

	if (0 != SPI_MTX_LOCK(Cfg)) {					/* Now we need to have exclusive access			*/
		return(-8);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

  #if (((MY_SPI_OPERATION) & 0x00202) != 0)
	if (Cfg->MySema == (SEM_t *)NULL) {				/* If interrupts are used, then semaphores are	*/
		Cfg->MySema = SEMopen(&g_Names[Dev][0]);	/* the blocking mechanism						*/
	}
  #endif

	if (Cfg->IsInit == 0) {							/* If initialized, another slave can be in use	*/
		SPI_RESET_DEV(Dev);							/* Hardware reset of the module					*/
	}

	Cfg->SlvInit   &= ~(1U << Slv);					/* In case re-initializing this slave			*/
	Cfg->HW         = g_HWaddr[Dev];				/* Memo the controller register base address	*/
	Cfg->Nbits[Slv] = Nbits;						/* Assume is packed data						*/
	if ((Nbits & 0x7) == 0) {						/* When 8 bit or 16 bits, packing is irrelevent	*/
		Mode &= ~(SPI_DATA_PACK);					/* Internally remove that information from Mode	*/
	}
	else {											/* DMA can only transfer 8 bits or 16 bits		*/
		Mode &= ~(SPI_XFER_DMA);					/* Internally remove that information from Mode	*/
	}
	Cfg->Mode[Slv]  = Mode;
													/* -------------------------------------------- */
	Cfg->Crtl0[Slv] = (Nbits-1)						/* CRTLR #0 register set-up						*/
	                | ((Mode & 0x3) << 4)			/* Insert SPI / SSP / MicroWire					*/
	                | ((Mode & 0x4) << 4)			/* Insert CPHA									*/
	                | ((Mode & 0x8) << 4);			/* Insert CPOL									*/
													/* -------------------------------------------- */
	Div = ((MY_SPI_CLK)+Freq-1) / Freq;				/* Determine the divisor for desired SPI clock	*/

	if ((Div & 1) != 0) {							/* The controller ignore LSB, so make sure to	*/
		Div++;										/* not exceed the requested bus clk frequency	*/
	}
	if (Freq >= ((MY_SPI_CLK)/2)) {					/* /2 is the minimum value that can be used		*/
		Div = 2;
	}
	if (Div > 65534) {								/* Maximum divider value is 65534				*/
		Div = 65534;
	}

	Cfg->BaudR[Slv] = Div;

	if ((Div == 0)									/* Register LSB is always ignored on write		*/
	||  (Div > 65534)) {							/* 65534 because bit #0 is always 0				*/
		SPI_MTX_UNLOCK(Cfg);
	  #if ((MY_SPI_DEBUG) > 0)
		puts("SPI  - Error - SPI bus clock frequency too low (can't divide main clock that much)");
	  #endif
		return(-9);
	}

	Cfg->BaudR[Slv] = Div;

	Cfg->Shift[Slv] = 0;
	if (((Mode & (SPI_ALIGN_LEFT)) != 0)			/* Pre-compute the right /left shift to apply	*/
	&&  ((Mode & (SPI_DATA_PACK)) == 0)) {			/* When packed data, Shift must remain at 0		*/
		Cfg->Shift[Slv] = (Nbits <= 8)
		                ? ( 8 - Nbits)
	                    : (16 - Nbits);
	}

	Cfg->SndRcv[Slv]  = ((Mode & (SPI_TX_RX_EEPROM)) != 0)
	                  ? (3 << 8)					/* EEPROM transfer mode							*/
	                  : 0;							/* Regular TX-RX transfer mode					*/
	Cfg->SlvInit     |= (1U << Slv);				/* The slave device is now initialized			*/
	Cfg->NsecBit[Slv] = 1000000000
	                  / (((MY_SPI_CLK))/Div);

	Cfg->ISRwaterRX = SPI_RX_WATERMARK;
	Cfg->ISRwaterTX = SPI_TX_WATERMARK;
	Cfg->DMAminRX   = MY_SPI_MIN_4_RX_DMA;
	Cfg->DMAminTX   = MY_SPI_MIN_4_TX_DMA;
	Cfg->ISRminRX   = MY_SPI_MIN_4_RX_ISR;
	Cfg->ISRminTX   = MY_SPI_MIN_4_TX_ISR;

  #if ((MY_SPI_DEBUG) > 0)
	putchar('\n');
	printf("SPI  - Device         : #%d\n", Dev);
	printf("SPI  - Slave          : #%d\n", Slv);
	printf("SPI  - RX FIFO size   : %3d frames\n", MY_SPI_RX_FIFO_SIZE);
	printf("SPI  - RX FIFO thres  : %3d frames\n", Cfg->ISRwaterRX);
	printf("SPI  - RX min DMA     : %3d frames\n", Cfg->DMAminRX);
	printf("SPI  - RX min INT     : %3d frames\n", Cfg->ISRminRX);
	printf("SPI  - TX FIFO size   : %3d frames\n", MY_SPI_TX_FIFO_SIZE);
	printf("SPI  - TX FIFO thres  : %3d frames\n", Cfg->ISRwaterTX);
	printf("SPI  - TX min DMA     : %3d frames\n", Cfg->DMAminTX);
	printf("SPI  - TX min INT     : %3d frames\n", Cfg->ISRminTX);
	printf("SPI  - CTRL clock     : %9d Hz\n", MY_SPI_CLK);
	printf("SPI  - SPI bus clk    : %9d Hz [/%d]\n", (MY_SPI_CLK)/Div, Div);
	printf("SPI  - Clock Phase    : CPHA%d\n", (0 != (Cfg->Mode[Slv] & SPI_CLK_CPHA1)));
	printf("SPI  - Clock Inactive : CPOL%d\n", (0 != (Cfg->Mode[Slv] & SPI_CLK_CPOL1)));
	printf("SPI  - Bits per frame : %d\n", Nbits);
  #endif

	Cfg->IsInit = 1;								/* And the controller is now initialized		*/

	SPI_MTX_UNLOCK(Cfg);

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* Interrupt handlers for the SPI transfers															*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static void SPIintHndl(int Dev)
{
#if (((MY_SPI_OPERATION) & 0x00202) != 0)
uint8_t  *Buf8;
uint16_t *Buf16;
uint16_t  Buf16Tmp[1];								/* When accessing 16 bit unaligned memory		*/
SPIcfg_t *Cfg;										/* De-reference array for faster access			*/
int       ii;										/* General purpose								*/
int       Nbits;									/* Number of bits per frame						*/
int       ReMap;									/* Device remapped index						*/
volatile uint32_t *Reg;								/* To access the SPI controller registers		*/
int       Shift;									/* Shift to apply if buffer left aligned		*/
#if (((MY_SPI_OPERATION) & 0x00002) != 0)
 int      jj;
 int      LeftRX;									/* Number of frames left to receive				*/
#endif
#if (((MY_SPI_OPERATION) & 0x00200) != 0)
 int      DumTX;									/* Number of dummy frames left to transmit		*/
 int      LeftTX;									/* Number of frames left to transmit			*/
#endif
#if  (((MY_SPI_OPERATION) & 0x10000) != 0)			/* If packed data is supported					*/
 int      AlignLeft;								/* We give the ISR all pre-computed stuff to	*/
 int      IsPack;
 #if  (((MY_SPI_OPERATION) & 0x00002) != 0)
 uint32_t DataRX;									/* reduce the overhead as much as possible		*/
 uint32_t MaskRX;
 int      ShiftRX;
 #endif
 #if  (((MY_SPI_OPERATION) & 0x00200) != 0)
 uint32_t DataTX;
 int      ShiftTX;
 #endif
#endif

	ReMap = g_DevReMap[Dev];
	Cfg   = &g_SPIcfg[ReMap];						/* This is this device configuration			*/
	Reg   = Cfg->HW;

  #if ((MY_SPI_MULTICORE_ISR) != 0)

   #if ((OX_NESTED_INTS) != 0)						/* Get exclusive access first					*/
	ReMap = OSintOff();								/* and then set the OnGoing flag to inform the	*/
   #endif											/* other cores to skip as I'm handlking the ISR	*/
	CORElock(SPI_SPINLOCK, &Cfg->SpinLock, 0, COREgetID()+1);

	ii = Cfg->ISRonGoing;							/* Memo if is current being handled				*/
	if (ii == 0) {
		Cfg->ISRonGoing = 1;						/* Declare as being handle / will be handle		*/
	}

	COREunlock(SPI_SPINLOCK, &Cfg->SpinLock, 0);
  #if ((OX_NESTED_INTS) != 0)
	OSintBack(ReMap);
  #endif

	if (ii != 0) {									/* Another core was handling it					*/
		return;
	}

  #endif

	Nbits   = Cfg->ISRnbits;
	Shift   = Cfg->ISRshift;

  #if  (((MY_SPI_OPERATION) & 0x10000) != 0)		/* If packed data is supported					*/
	AlignLeft = Cfg->ISRalignLeft;
	IsPack    = Cfg->ISRisPack;
   #if (((MY_SPI_OPERATION) & 0x00002) != 0)
	DataRX    = Cfg->ISRdataRX;
	MaskRX    = Cfg->ISRmaskRX;
	ShiftRX   = Cfg->ISRshiftRX;
   #endif
   #if (((MY_SPI_OPERATION) & 0x00200) != 0)
	DataTX    = Cfg->ISRdataTX;
	ShiftTX   = Cfg->ISRshiftTX;
   #endif
  #endif

  #if (((MY_SPI_OPERATION) & 0x00200) != 0)
	if (0 != (Reg[SPI_IMR_REG] & 0x01)) {			/* Check if TX ISRs are enable					*/
		DumTX  = Cfg->ISRdumTX;						/* If they are, TX transfers are done with ISRS	*/
		LeftTX = Cfg->ISRleftTX;					/* For multi-core multi ISR, LeftTX & DumTX are	*/
													/* sufficient for proper operation				*/
		if (LeftTX != 0) {							/* Real frames left to transmit					*/
			Buf8  = (void *)Cfg->ISRbufTX;
			Buf16 = (void *)Cfg->ISRbufTX;

			ii = Reg[SPI_TXFLR_REG];				/* Number of frames in the TX FIFO				*/
			if (ii == 0) {							/* If the TX FIFO has under-run, we are toasted	*/
				ii = MY_SPI_TX_FIFO_SIZE;			/* This will skip further frame sending			*/
				if ((Cfg->Error) == 0) {
					Cfg->Error = -10;
				}
			}

			ii = (MY_SPI_TX_FIFO_SIZE)
			   - ii;								/* Room available in the TX FIFO				*/
			if (ii > LeftTX) {						/* Can't copy more than left to transmit		*/
				ii = LeftTX;
			}
			LeftTX -= ii;

			SPI_TX_DATA(IsPack, AlignLeft, Nbits, ii, DataTX, ShiftTX, Shift, Buf8, Buf16,
			            Buf16Tmp, Reg);

			Cfg->ISRbufTX  = (Nbits <= 8)
			               ? (void *)Buf8
			               : (void *)Buf16;
		}

		if ((LeftTX == 0)							/* Deal with dummy frames left to transmit		*/
		&&  (DumTX != 0)) {
			ii = Reg[SPI_TXFLR_REG];				/* Number of frames in the TX FIFO				*/
			if (ii == 0) {							/* If the TX FIFO has under-run, we are toasted	*/
				ii = MY_SPI_TX_FIFO_SIZE;			/* This will skip further frame sending			*/
				if ((Cfg->Error) == 0) {
					Cfg->Error = -10;
				}
			}

			ii = (MY_SPI_TX_FIFO_SIZE)
			   - ii;								/* Room available in the TX FIFO				*/
			if (ii > DumTX) {						/* Can't copy more than left to transmit		*/
				ii = DumTX;
			}

			DumTX -= ii;

			for ( ; ii>0 ; ii--) {
				Reg[SPI_DR_REG] = 0;
			}
		}

		if (Cfg->Error != 0) {						/* The TX FIFO has underflow					*/
			LeftTX           = 0;					/* Abort the transfer and report the error		*/
			DumTX            = 0;
			Reg[SPI_IMR_REG] = 0;					/* Disable all interrupts						*/
		}

		if ((LeftTX+DumTX) == 0) {					/* When all transferred. No watermark check as	*/
		  #if (((MY_SPI_OPERATION) & 0x00002) != 0)	/* done in RX: TX watermark is always reached	*/
			if (0 == (Reg[SPI_IMR_REG] & 0x10))		/* If RX still going, don't post semaphore yet	*/
		  #endif									/* as the RX will post it						*/
			{
				SEMpost(Cfg->MySema);
			}
			Reg[SPI_IMR_REG] &= ~0x01;				/* Disable the TX interrupt						*/
		}

		Cfg->ISRleftTX = LeftTX;
		Cfg->ISRdumTX  = DumTX;
	}

  #endif
													/* -------------------------------------------- */
  #if (((MY_SPI_OPERATION) & 0x00002) != 0)
	if (0 != (Reg[SPI_IMR_REG] & 0x10)) {			/* Check if RX enable for spi_send_recv()		*/
		Buf8   = (void *)Cfg->ISRbufRX;				/* If they are, RX transfers are done with ISRS	*/
		Buf16  = (void *)Cfg->ISRbufRX;
		LeftRX = Cfg->ISRleftRX;

		if (LeftRX != 0) {
			ii = Reg[SPI_RXFLR_REG];				/* Number of frames in the RX FIFO				*/
			if (ii > LeftRX) {						/* This can happen in spi_send_recv() when		*/
				ii = LeftRX;						/* not operating in EEPROM mode & RXlen < TXlen	*/
			}

			jj = Reg[SPI_RXFTLR_REG]+1;				/* Number of frames that trigger the ISR		*/
			if (((LeftRX-ii) > 0)					/* Need to make sure the last transfer length	*/
			&&  ((LeftRX-ii) < jj)) {				/* is > Watermark as no interrupt issued as		*/
				ii = LeftRX - jj;					/* as RX FIFO contents <= Watermark				*/
			}
			LeftRX -= ii;

			SPI_RX_DATA(IsPack, AlignLeft, Nbits, ii, DataRX, ShiftRX, Shift, MaskRX, Buf8,
			            Buf16, Buf16Tmp, Reg);
		}

		if (Reg[SPI_RISR_REG] & 0x08) {				/* If the RX FIFO has overflown, we are toasted	*/
			LeftRX           = 0;					/* Abort the transfer and report the error		*/
			Reg[SPI_IMR_REG] = 0;					/* Disable all TX interrupt						*/
			if ((Cfg->Error) == 0) {
				Cfg->Error   = -11;
			}
		}

		Cfg->ISRbufRX  = (Nbits <= 8)
		               ? (void *)Buf8
		               : (void *)Buf16;
		Cfg->ISRleftRX = LeftRX;

		if (LeftRX == 0) {							/* Is the transfer done?						*/
		  #if (((MY_SPI_OPERATION) & 0x00200) != 0)
			if (0 == (Reg[SPI_IMR_REG] & 0x01))		/* If TX still going, don't post semaphore yet	*/
		  #endif									/* and TX left to send as the TX will post it	*/
			{
				SEMpost(Cfg->MySema);
			}
			Reg[SPI_IMR_REG] &= ~0x10;				/* Disable the RX interrupt						*/
		}
	}
  #endif

  #if  (((MY_SPI_OPERATION) & 0x10000) != 0)		/* If packed data is supported					*/
   #if  (((MY_SPI_OPERATION) & 0x00002) != 0)
	Cfg->ISRdataRX  = DataRX;
	Cfg->ISRshiftRX = ShiftRX;
   #endif
   #if  (((MY_SPI_OPERATION) & 0x00200) != 0)
	Cfg->ISRdataTX  = DataTX;
	Cfg->ISRshiftTX = ShiftTX;
   #endif
  #endif

  #if ((MY_SPI_MULTICORE_ISR) != 0)
	Cfg->ISRonGoing = 0;							/* Report I'm not handling the interrupt 		*/
  #endif

#else
	g_SPIcfg[g_DevReMap[Dev]].HW[SPI_IMR_REG] = 0;

#endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Individual interrupt handlers macro. It calls the common handler with the device #				*/

  #define  SPI_INT_HNDL(Prefix, Device) _SPI_INT_HNDL(Prefix, Device)
  #define _SPI_INT_HNDL(Prefix, Device)																\
	void Prefix##SPIintHndl_##Device(void)															\
	{																								\
		SPIintHndl(Device);																			\
		return;																						\
	}

/* ------------------------------------------------------------------------------------------------ */

#if (((MY_SPI_LIST_DEVICE) & 1U) != 0U)
  SPI_INT_HNDL(MY_SPI_PREFIX, 0)
#endif
#if (((MY_SPI_LIST_DEVICE) & 2U) != 0U)
  SPI_INT_HNDL(MY_SPI_PREFIX, 1)
#endif
#if (((MY_SPI_LIST_DEVICE) & 4U) != 0U)
  SPI_INT_HNDL(MY_SPI_PREFIX, 2)
#endif
#if (((MY_SPI_LIST_DEVICE) & 8U) != 0U)
  SPI_INT_HNDL(MY_SPI_PREFIX, 3)
#endif
#if (((MY_SPI_LIST_DEVICE) & 16U) != 0U)
  SPI_INT_HNDL(MY_SPI_PREFIX, 4)
#endif
#if (((MY_SPI_LIST_DEVICE) & 32U) != 0U)
  SPI_INT_HNDL(MY_SPI_PREFIX, 5)
#endif
#if (((MY_SPI_LIST_DEVICE) & 64U) != 0U)
  SPI_INT_HNDL(MY_SPI_PREFIX, 6)
#endif
#if (((MY_SPI_LIST_DEVICE) & 128U) != 0U)
  SPI_INT_HNDL(MY_SPI_PREFIX, 7)
#endif
#if (((MY_SPI_LIST_DEVICE) & 256U) != 0U)
  SPI_INT_HNDL(MY_SPI_PREFIX, 8)
#endif
#if (((MY_SPI_LIST_DEVICE) & 512U) != 0U)
  SPI_INT_HNDL(MY_SPI_PREFIX, 9)
#endif

/* EOF */
