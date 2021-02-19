/* ------------------------------------------------------------------------------------------------ */
/* FILE :		arm_pl330.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the ARM PL330 DMA controller												*/
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
/*	$Revision: 1.9 $																				*/
/*	$Date: 2019/01/10 18:07:02 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* See arm_pl330.c for NOTE / LIMITATIONS / NOT YET SUPPORTED										*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Supported value for the build options															*/
/* (OS_PLATFORM & 0x00FFFFFF):																		*/
/*					0x7020 : Zynq 7000 family														*/
/*					0xAA10 : Arria 10 																*/
/*					0xAAC5 : Cyclone V & Arria V													*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Default values that depend on (OS_PLATFORM & 0x00FFFFFF):										*/
/* DMA_MAX_DEVICES:																					*/
/*					0x7020 : 1																		*/
/*					0xAA10 : 1																		*/
/*					0xAAC5 : 1																		*/
/* DMA_LIST_DEVICE:																					*/
/*					0x7020 : 0x01																	*/
/*					0xAA10 : 0x01																	*/
/*					0xAAC5 : 0x01																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __ARM_PL330_H__							/* To protect against re-include					*/
#define __ARM_PL330_H__

#include <stdint.h>

#ifdef __cplusplus
  extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------ */

#if defined(DMA_MAX_DEVICES) && defined(DMA_LIST_DEVICE)
  #error "Define one of the two: DMA_MAX_DEVICES or DMA_LIST_DEVICE"
  #error "Do not define both as there could be conflicting info"
#endif

#if !defined(DMA_MAX_DEVICES) && !defined(DMA_LIST_DEVICE)
  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0xAAC5)
	#define DMA_MAX_DEVICES	1
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0xAA10)
	#define DMA_MAX_DEVICES	1
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x7020)
	#define DMA_MAX_DEVICES	1
  #else
	#define DMA_MAX_DEVICES	1
  #endif
#endif

#ifndef DMA_LIST_DEVICE
  #define DMA_LIST_DEVICE		((1U<<(DMA_MAX_DEVICES))-1U)
#endif

/* ------------------------------------------------------------------------------------------------ */

#define DMA_CFG_NOP					(0x00000001)		/* for: (cond) ? DMA_CFG_XXX : DMA_CFG_NOP	*/
#define DMA_CFG_EOT_ISR				(0x00000002)		/* Wait for EOT using interrupts			*/
#define DMA_CFG_EOT_POLLING			(0x00000003)		/* Wait for EOT using polling (default)		*/
#define DMA_CFG_NOSTART				(0x00000004)		/* Do not start the transfer (use dma_start)*/
#define DMA_CFG_NOWAIT				(0x00000005)		/* Do not wait for EOT						*/
#define DMA_CFG_WAIT_TRG			(0x00000006)		/* Wait for another channel trigger			*/
#define DMA_CFG_TRG_PEND_START		(0x00000007)		/* Send trigger: # set by dma_pend_trig()	*/
#define DMA_CFG_TRG_PEND_END		(0x00000008)		/* Send trigger: # set by dma_pend_trig()	*/
#define DMA_CFG_NOCACHE_DST			(0x00000009)		/* No cache maintenance on destin memory	*/
#define DMA_CFG_NOCACHE_SRC			(0x0000000A)		/* No cache maintenance on source memory	*/
#define DMA_CFG_LOGICAL_DST			(0x0000000B)		/* Need to remap dst logical to physical	*/
#define DMA_CFG_LOGICAL_SRC			(0x0000000C)		/* Need to remap srec logical to physical	*/
#define DMA_CFG_REUSE				(0x0000000D)		/* When done, keep config for another xfer	*/
#define DMA_CFG_REPEAT				(0x0000000E)		/* Repeat the whole sequence				*/
#define DMA_CFG_NO_MBARRIER			(0x0000000F)		/* If using memory barrier with I/O			*/
#define DMA_CFG_ARCACHE(cm)			(0x30000000 | ((cm)&7))	/* ARCACHE[] setting (see arm_pl330.c)	*/
#define DMA_CFG_AWCACHE(cm)			(0x40000000 | ((cm)&7))	/* AWCACHE[] setting (see arm_pl330.c)	*/
#define DMA_CFG_TRG_ON_START(ID)	(0x50000000 | (ID))	/* When starting, send trigger to XferID	*/
#define DMA_CFG_TRG_ON_END(ID)		(0x60000000 | (ID))	/* When done, send trigger to XferID		*/
#define DMA_CFG_WAIT_ON_END(ID)		(0x70000000 | (ID))	/* Waiting on a xfer done to declare EOT	*/
#define DMA_CFG_WAIT_AVAIL(x)		(0x80000000 | (x))	/* Wait for an available chan if all used	*/
														/* Is set to 1<<31 to allow max timeout		*/
#define DMA_CFG_MASK_ARG			(0xFF000000)		/* **** DO NOT USE!!! *** FOR INTERNAL USE	*/


#define DMA_MAX_BURST				16					/* Maximum busrt length of the PL330		*/


#define DMA_OPEND_NONE				0					/* At end of Xfer: no operation				*/
#define DMA_OPEND_FCT				1					/* At end of Xfer: call a function			*/
#define DMA_OPEND_VAR				2					/* At end of Xfer: write to a variable		*/
#define DMA_OPEND_SEM				3					/* At end of Xfer: post a semaphore			*/
#define DMA_OPEND_MTX				4					/* At end of Xfer: unlock a mutex			*/
#if ((OX_MAILBOX) != 0)
  #define DMA_OPEND_MBX				5					/* At end of Xfer: write to a mailbox		*/
#endif
#if ((OX_EVENTS) != 0)
  #define DMA_OPEND_EVT				6					/* At end of Xfer: set an event to a task	*/
#endif

int dma_done     (int XferID);
int dma_init     (int Dev);
int dma_kill     (int XferID);
int dma_release  (int XferID);
int dma_start    (int XferID);
int dma_state    (int XferID, int Disp);
int dma_wait     (int XferID);
int dma_xfer(int Dev, void *AddDst, int IncDst, int TypeDst,
                const void *AddSrc, int IncSrc, int TypeSrc,
                int DataSize, int BrstLen, int Nxfer,
                int OpEnd, void *PtrEnd, intptr_t ValEnd,
                const uint32_t *OpMode, int *XferID, int TimeOut);

/* ------------------------------------------------------------------------------------------------ */

extern void DMAintHndl_0(void);
extern void DMAintHndl_1(void);
extern void DMAintHndl_2(void);
extern void DMAintHndl_3(void);
extern void DMAintHndl_4(void);
extern void DMAintHndl_5(void);
extern void DMAintHndl_6(void);
extern void DMAintHndl_7(void);


#ifdef __cplusplus
  }
#endif

#endif											/* The whole file is conditional					*/

/* EOF */
