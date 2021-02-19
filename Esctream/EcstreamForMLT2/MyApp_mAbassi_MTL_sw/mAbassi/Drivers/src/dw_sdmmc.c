/* ------------------------------------------------------------------------------------------------ */
/* FILE :		dw_sdmmc.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the Designware SD/MMC.													*/
/*																									*/
/*																									*/
/* Copyright (c) 2013-2019, Code-Time Technologies Inc. All rights reserved.						*/
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
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* NOTES:																							*/
/*																									*/
/* LIMITATIONS:																						*/
/*																									*/
/* NOT YET SUPPORTED:																				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* How to use this driver:																			*/
/*																									*/
/* A working example for FatFS us located in: ... src/Media_FatFFS_$$$.c							*/
/*																									*/
/* Key defines (in dw_sdmmc.h)																		*/
/*																									*/
/*	SDMMC_NUM_DMA_DESC:	Number of chained DMA descriptors											*/
/*	                    The maximum number of bytes per transfer is 512*SDMMC_NUM_DMA_DESC			*/
/*	                    If not defined, the default is set to 16 (check in dw_sdmmc.h)				*/
/*						If the transfer size requested exceeds this maximum, an error is reported	*/
/*																									*/
/*	SDMMC_BUFFER_TYPE:  Type memory for the DMA buffers												*/
/*	                    SDMMC_BUFFER_UNCACHED (Value == 0) Buffer in uncached memory				*/
/*	                    SDMMC_BUFFER_CACHED   (Value != 0) Buffers in cached memory					*/
/*	                    if not defined, the default is SDMMC_BUFFER_UNCACHED (check in dw_sdmmc.h)	*/
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

#include "dw_sdmmc.h"

#ifndef SDMMC_USE_MUTEX								/* If a mutex is used to protect accesses		*/
  #define SDMMC_USE_MUTEX		0					/* == 0 : No mutex       != 0 : Use mutex		*/
#endif

#ifndef SDMMC_OPERATION								/* Bit #0  : Disable ISRs on polled transfers	*/
  #define SDMMC_OPERATION		0x00003				/* Bit #1  : Interrupts are used				*/
#endif

#ifndef SDMMC_TOUT_ISR_ENB							/* Set to 0 if timeouts not required in polling	*/
  #define SDMMC_TOUT_ISR_ENB	1					/* When timeout used, if ISRs requested to be	*/
#endif												/* disable, enable-disable to update/read tick	*/
													/* to allow update of the RTOS timer tick count	*/
#ifndef SDMMC_REMAP_LOG_ADDR
  #define SDMMC_REMAP_LOG_ADDR	1					/* If remapping logical to physical adresses	*/
#endif

#ifndef SDMMC_MULTICORE_ISR
  #define SDMMC_MULTICORE_ISR	1					/* When operating on a multicore, set to != 0	*/
#endif												/* if more than 1 core process the interrupt	*/

#ifndef SDMMC_ARG_CHECK
  #define SDMMC_ARG_CHECK		0					/* If checking validity of function arguments	*/
#endif

#ifndef SDMMC_DEBUG
  #define SDMMC_DEBUG			1					/* Set to non-zero to print debug info			*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Base addresses of the HW SD/MMC (sdio) modules & related peripherals								*/

#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)		/* Arria V / Cyclone V							*/
  static volatile uint32_t * const g_CLKaddr[1] = { (volatile uint32_t *)0xFFD04000 };
  static volatile uint32_t * const g_HWaddr[1]  = { (volatile uint32_t *)0xFF704000 };
  static volatile uint32_t * const g_SYSaddr[1] = { (volatile uint32_t *)0xFFD08000 };

  #define CLKMGR_PERPLLGRP_EN_REG				(0xA0/4)
  #define CLKMGR_SDMMC_CLK_ENABLE				(1 << 8)
  #define SYSMGR_SDMMCGRP_CTRL_REG				(0x108/4)
  #define SYSMGR_SDMMC_CTRL_GET_DRVSEL(x)		(((x) >> 0) & 0x7)
  #define SYSMGR_SDMMC_CTRL_SET(smplsel,drvsel)	((((drvsel) << 0) & 0x7) | (((smplsel) << 3)&0x38))

 #ifndef SDMMC_CLK
  #define SDMMC_CLK				(50000000)
 #endif

 #if ((SDMMC_MAX_DEVICES) > 1)
  #error "Too many SDMMC devices: set SDMMC_MAX_DEVICES <= 1 and/or SDMMC_LIST_DEVICE <= 0x1"
 #endif

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)	/* Arria 10										*/
  static volatile uint32_t * const g_CLKaddr[1] = { (volatile uint32_t *)0xFFD04000 };
  static volatile uint32_t * const g_HWaddr[1]  = { (volatile uint32_t *)0xFF808000 };
  static volatile uint32_t * const g_SYSaddr[1] = { (volatile uint32_t *)0xFFD06000 };

  #define CLKMGR_PERPLLGRP_EN_REG				(0x08/4)
  #define CLKMGR_SDMMC_CLK_ENABLE				(1 << 5)
  #define SYSMGR_SDMMCGRP_CTRL_REG				(0x28/4)
  #define SYSMGR_SDMMC_CTRL_GET_DRVSEL(x)		(((x) >> 0) & 0x7)
  #define SYSMGR_SDMMC_CTRL_SET(smplsel,drvsel)	((((drvsel) << 0) & 0x7) | (((smplsel) << 4)&0x70))

 #ifndef SDMMC_CLK
  #define SDMMC_CLK				(50000000)
 #endif

 #if ((SDMMC_MAX_DEVICES) > 1)
  #error "Too many SDMMC devices: set SDMMC_MAX_DEVICES <= 1 and/or SDMMC_LIST_DEVICE <= 0x1"
 #endif

#endif

/* ------------------------------------------------------------------------------------------------ */
/* This section uses a lots of macros but it's to reduce the size of the data requirements by only	*/
/* creating descriptor / mutexes etc for the devices in use ("1" in SDMMC_LIST_DEVICE)				*/

#ifndef SDMMC_MAX_DEVICES
 #if   (((SDMMC_LIST_DEVICE) & 0x200) != 0U)
  #define SDMMC_MAX_DEVICES	10
 #elif (((SDMMC_LIST_DEVICE) & 0x100) != 0U)
  #define SDMMC_MAX_DEVICES	9
 #elif (((SDMMC_LIST_DEVICE) & 0x080) != 0U)
  #define SDMMC_MAX_DEVICES	8
 #elif (((SDMMC_LIST_DEVICE) & 0x040) != 0U)
  #define SDMMC_MAX_DEVICES	7
 #elif (((SDMMC_LIST_DEVICE) & 0x020) != 0U)
  #define SDMMC_MAX_DEVICES	6
 #elif (((SDMMC_LIST_DEVICE) & 0x010) != 0U)
  #define SDMMC_MAX_DEVICES	5
 #elif (((SDMMC_LIST_DEVICE) & 0x008) != 0U)
  #define SDMMC_MAX_DEVICES	4
 #elif (((SDMMC_LIST_DEVICE) & 0x004) != 0U)
  #define SDMMC_MAX_DEVICES	3
 #elif (((SDMMC_LIST_DEVICE) & 0x002) != 0U)
  #define SDMMC_MAX_DEVICES	2
 #elif (((SDMMC_LIST_DEVICE) & 0x001) != 0U)
  #define SDMMC_MAX_DEVICES	1
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* This section figures out how to remap the individual devices specified by SDMMC_LIST_DEVICE		*/
/* e.g. if SDMMC_LIST_DEVICE == 0x12, the only descriptors/mutexes for 2 devices are required and	*/
/*      device#1 is remapped to [0] and device #4 is remapped to [1]								*/

#if (((SDMMC_LIST_DEVICE) & 0x001) != 0)
  #define SDMMC_CNT_0	0
  #define SDMMC_IDX_0	0
#else
  #define SDMMC_CNT_0	(-1)
  #define SDMMC_IDX_0	(-1)
#endif
#if (((SDMMC_LIST_DEVICE) & 0x002) != 0)
  #define SDMMC_CNT_1	((SDMMC_CNT_0) + 1)
  #define SDMMC_IDX_1	((SDMMC_CNT_0) + 1)
#else
  #define SDMMC_CNT_1	(SDMMC_CNT_0)
  #define SDMMC_IDX_1	-1
#endif
#if (((SDMMC_LIST_DEVICE) & 0x004) != 0)
  #define SDMMC_CNT_2	((SDMMC_CNT_1) + 1)
  #define SDMMC_IDX_2	((SDMMC_CNT_1) + 1)
#else
  #define SDMMC_CNT_2	(SDMMC_CNT_1)
  #define SDMMC_IDX_2	-1
#endif
#if (((SDMMC_LIST_DEVICE) & 0x008) != 0)
  #define SDMMC_CNT_3	((SDMMC_CNT_2) + 1)
  #define SDMMC_IDX_3	((SDMMC_CNT_2) + 1)
#else
  #define SDMMC_CNT_3	(SDMMC_CNT_2)
  #define SDMMC_IDX_3	-1
#endif
#if (((SDMMC_LIST_DEVICE) & 0x010) != 0)
  #define SDMMC_CNT_4	((SDMMC_CNT_3) + 1)
  #define SDMMC_IDX_4	((SDMMC_CNT_3) + 1)
#else
  #define SDMMC_CNT_4	(SDMMC_CNT_3)
  #define SDMMC_IDX_4	-1
#endif
#if (((SDMMC_LIST_DEVICE) & 0x020) != 0)
  #define SDMMC_CNT_5	((SDMMC_CNT_4) + 1)
  #define SDMMC_IDX_5	((SDMMC_CNT_4) + 1)
#else
  #define SDMMC_CNT_5	(SDMMC_CNT_4)
  #define SDMMC_IDX_5	-1
#endif
#if (((SDMMC_LIST_DEVICE) & 0x040) != 0)
  #define SDMMC_CNT_6	((SDMMC_CNT_5) + 1)
  #define SDMMC_IDX_6	((SDMMC_CNT_5) + 1)
#else
  #define SDMMC_CNT_6	(SDMMC_CNT_5)
  #define SDMMC_IDX_6	-1
#endif
#if (((SDMMC_LIST_DEVICE) & 0x080) != 0)
  #define SDMMC_CNT_7	((SDMMC_CNT_6) + 1)
  #define SDMMC_IDX_7	((SDMMC_CNT_6) + 1)
#else
  #define SDMMC_CNT_7	(SDMMC_CNT_6)
  #define SDMMC_IDX_7	-1
#endif
#if (((SDMMC_LIST_DEVICE) & 0x100) != 0)
  #define SDMMC_CNT_8	((SDMMC_CNT_7) + 1)
  #define SDMMC_IDX_8	((SDMMC_CNT_7) + 1)
#else
  #define SDMMC_CNT_8	(SDMMC_CNT_7)
  #define SDMMC_IDX_8	-1
#endif
#if (((SDMMC_LIST_DEVICE) & 0x200) != 0)
  #define SDMMC_CNT_9	((SDMMC_CNT_8) + 1)
  #define SDMMC_IDX_9	((SDMMC_CNT_8) + 1)
#else
  #define SDMMC_CNT_9	(SDMMC_CNT_8)
  #define SDMMC_IDX_9	-1
#endif

#define SDMMC_NMB_DEVICES	((SDMMC_CNT_9)+1)

/* ------------------------------------------------------------------------------------------------ */

static const int g_DevReMap[] = { SDMMC_IDX_0
                               #if ((SDMMC_MAX_DEVICES) > 1)
                                 ,SDMMC_IDX_1
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 2)
                                 ,SDMMC_IDX_2
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 3)
                                 ,SDMMC_IDX_3
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 4)
                                 ,SDMMC_IDX_4
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 5)
                                 ,SDMMC_IDX_5
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 6)
                                 ,SDMMC_IDX_6
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 7)
                                 ,SDMMC_IDX_7
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 8)
                                 ,SDMMC_IDX_8
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 9)
                                 ,SDMMC_IDX_9
                               #endif
                               };

/* ------------------------------------------------------------------------------------------------ */

static const char g_Names[][8] = {
                                 "SDMMC-0"
                               #if ((SDMMC_MAX_DEVICES) > 1)
                                ,"SDMMC-1"
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 2)
                                ,"SDMMC-2"
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 3)
                                ,"SDMMC-3"
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 4)
                                ,"SDMMC-4"
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 5)
                                ,"SDMMC-5"
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 6)
                                ,"SDMMC-6"
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 7)
                                ,"SDMMC-7"
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 8)
                                ,"SDMMC-8"
                               #endif 
                               #if ((SDMMC_MAX_DEVICES) > 9)
                                ,"SDMMC-9"
                               #endif
                               };

/* ------------------------------------------------------------------------------------------------ */
/* Checks																							*/

#if ((OX_TIMER_US) <= 0)
  #error "The SDMMC driver relies on timeouts."
  #error " You must enable the RTOS timer by setting OS_TIMER_US to > 0"
#endif
#if ((OX_TIMEOUT) <= 0)
  #error "The SDMMC driver relies on timeouts. You must set OS_TIMEOUT to > 0"
#endif

#if (((SDMMC_OPERATION) & ~0x00003) != 0)
  #error "Invalid value assigned to SDMMC_OPERATION"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* SDMMC register index definitions (are accessed as uint32_t)										*/

#define SD_CTRL_REG					(0x000/4)
  #define SD_CTRL_RESET				  (1U << 0)
  #define SD_CTRL_FIFO_RESET		  (1U << 1)
  #define SD_CTRL_INT_ENABLE		  (1U << 4)
  #define SD_CTRL_DMA_RESET			  (1U << 2)
  #define SD_CTRL_SEND_AS_CCSD		  (1U << 10)
  #define SD_IDMAC_EN				  (1U << 25)
  #define SD_RESET_ALL				  (SD_CTRL_RESET | SD_CTRL_FIFO_RESET |	SD_CTRL_DMA_RESET)
#define	SD_PWREN_REG				(0x004/4)
#define SD_CLKDIV_REG				(0x008/4)
#define SD_CLKSRC_REG				(0x00C/4)
#define SD_CLKENA_REG				(0x010/4)
  #define SD_CLKEN_ENABLE			  (1U << 0)
  #define SD_CLKEN_LOW_PWR			  (1U << 16)
#define SD_TMOUT_REG				(0x014/4)
#define SD_CTYPE_REG				(0x018/4)
  #define SD_CTYPE_1BIT				  0U
  #define SD_CTYPE_4BIT				  (1U << 0)
  #define SD_CTYPE_8BIT				  (1U << 16)
#define SD_BLKSIZ_REG				(0x01C/4)
#define SD_BYTCNT_REG				(0x020/4)
#define SD_INTMASK_REG				(0x024/4)
  #define SD_INTMSK_ALL				0x0001FFFFU
  #define SD_INTMSK_CD				  (1U <<  0)	/* Card detect									*/
  #define SD_INTMSK_RE				  (1U <<  1)	/* Response error								*/
  #define SD_INTMSK_CDONE			  (1U <<  2)	/* Command done									*/
  #define SD_INTMSK_DT				  (1U <<  3)	/* Data transfer over							*/
  #define SD_INTMSK_TXDR			  (1U <<  4)	/* Transmit FIFO data request					*/
  #define SD_INTMSK_RXDR			  (1U <<  5)	/* Receive FIFO data request					*/
  #define SD_INTMSK_DCRC			  (1U <<  7)	/* Response CRC error							*/
  #define SD_INTMSK_RTO				  (1U <<  8)	/* Data CRC error								*/
  #define SD_INTMSK_DRTO			  (1U <<  9)	/* Response time out boot ACK received			*/
  #define SD_INTMSK_HTO				  (1U << 10)	/* data starvation host timeout volt switch_int	*/
  #define SD_INTMSK_FRUN			  (1U << 11)	/* FIFO under-run / overrun error				*/
  #define SD_INTMSK_HLE				  (1U << 12)	/* Hardware lock write error					*/
  #define SD_INTMSK_SBE				  (1U << 13)	/* Start bit error								*/
  #define SD_INTMSK_ACD				  (1U << 14)	/* Auto command done							*/
  #define SD_INTMSK_EBE				  (1U << 15)	/* End-bit error								*/
  #define SD_INTMSK_SDIO_INT		  (1U << 16)	/* SDIO interrupt								*/
#define SD_CMDARG_REG				(0x028/4)
#define SD_CMD_REG					(0x02C/4)
  #define SD_CMD_RESP_EXP			  (1U << 6)
  #define SD_CMD_RESP_LENGTH		  (1U << 7)
  #define SD_CMD_CHECK_CRC			  (1U << 8)
  #define SD_CMD_DATA_EXP			  (1U << 9)
  #define SD_CMD_RW					  (1U << 10)
  #define SD_CMD_SEND_STOP			  (1U << 12)
  #define SD_CMD_ABORT_STOP			  (1U << 14)
  #define SD_CMD_PRV_DAT_WAIT		  (1U << 13)
  #define SD_CMD_UPD_CLK			  (1U << 21)
  #define SD_CMD_USE_HOLD_REG		  (1U << 29)
  #define SD_CMD_START				  (1U << 31)
#define SD_RESP0_REG				(0x030/4)
#define SD_RESP1_REG				(0x034/4)
#define SD_RESP2_REG				(0x038/4)
#define SD_RESP3_REG				(0x03C/4)
#define SD_MINTSTS_REG				(0x040/4)
#define SD_RINTSTS_REG				(0x044/4)
  #define SD_DATA_ERR				  (SD_INTMSK_EBE  | SD_INTMSK_SBE | SD_INTMSK_HLE |				\
									   SD_INTMSK_FRUN | SD_INTMSK_DCRC)
  #define SD_DATA_TOUT				(SD_INTMSK_HTO  | SD_INTMSK_DRTO)
#define SD_STATUS_REG				(0x048/4)
  #define SD_BUSY					  (1U << 9)
  #define COMMAND_FSM_STATES		  (0xFU<<4)
  #define STATE_IDLEANDOTHERS		  (0x0U<<4)
#define SD_FIFOTH_REG				(0x04C/4)
  #define MSIZE(x)					  ((x) << 28)
  #define RX_WMARK(x)				  ((x) << 16)
  #define TX_WMARK(x)				  (x)
  #define FIFO_DEPTH(x)				  ((((x) >> 16) & 0x3ff) + 1)
#define SD_CDETECT_REG				(0x050/4)
#define SD_WRTPRT_REG				(0x054/4)
#define SD_GPIO_REG					(0x058/4)
#define SD_TCMCNT_REG				(0x05C/4)
#define SD_TBBCNT_REG				(0x060/4)
#define SD_DEBNCE_REG				(0x064/4)
#define SD_USRID_REG				(0x068/4)
#define SD_VERID_REG				(0x06C/4)
#define SD_HCON_REG					(0x070/4)
#define SD_UHS_REG_REG				(0x074/4)
#define SD_BMOD_REG					(0x080/4)
  #define SD_BMOD_IDMAC_RESET		  (1U << 0)
  #define SD_BMOD_IDMAC_FB			  (1U << 1)
  #define SD_BMOD_IDMAC_EN			  (1U << 7)
#define SD_PLDMND_REG				(0x084/4)
#define SD_DBADDR_REG				(0x088/4)
#define SD_IDSTS_REG				(0x08C/4)
#define SD_IDINTEN_REG				(0x090/4)
#define SD_DSCADDR_REG				(0x094/4)
#define SD_BUFADDR_REG				(0x098/4)
#define SD_DATA_REG					(0x200/4)

#define SD_FIFO_DEPTH				(1024)

/* ------------------------------------------------------------------------------------------------ */
/* SD/MMC command & parameter definitions															*/

#define MMC_STATUS_MASK				(~0x0206BF7F)
#define MMC_STATUS_RDY_FOR_DATA 	(1U   << 8)
#define MMC_STATUS_CURR_STATE		(0xFU << 9)
#define MMC_STATUS_ERROR			(1U   << 19)
#define MMC_STATE_PRG				(7U   << 9)

#define MMC_VDD_165_195				0x00000080		/* VDD voltage 1.65V - 1.95						*/
#define MMC_VDD_20_21				0x00000100		/* VDD voltage 2.0V  ~ 2.1V						*/
#define MMC_VDD_21_22				0x00000200		/* VDD voltage 2.1V  ~ 2.2V						*/
#define MMC_VDD_22_23				0x00000400		/* VDD voltage 2.2V  ~ 2.3V						*/
#define MMC_VDD_23_24				0x00000800		/* VDD voltage 2.3V  ~ 2.4V						*/
#define MMC_VDD_24_25				0x00001000		/* VDD voltage 2.4V  ~ 2.5v						*/
#define MMC_VDD_25_26				0x00002000		/* VDD voltage 2.5V  ~ 2.6V						*/
#define MMC_VDD_26_27				0x00004000		/* VDD voltage 2.6V  ~ 2.7V						*/
#define MMC_VDD_27_28				0x00008000		/* VDD voltage 2.7V  ~ 2.8V						*/
#define MMC_VDD_28_29				0x00010000		/* VDD voltage 2.8V  ~ 2.9V						*/
#define MMC_VDD_29_30				0x00020000		/* VDD voltage 2.9V  ~ 3.0V						*/
#define MMC_VDD_30_31				0x00040000		/* VDD voltage 3.0V  ~ 3.1V						*/
#define MMC_VDD_31_32				0x00080000		/* VDD voltage 3.1V  ~ 3.2V						*/
#define MMC_VDD_32_33				0x00100000		/* VDD voltage 3.2V  ~ 3.3V						*/
#define MMC_VDD_33_34				0x00200000		/* VDD voltage 3.3V  ~ 3.4V						*/
#define MMC_VDD_34_35				0x00400000		/* VDD voltage 3.4V  ~ 3.5V						*/
#define MMC_VDD_35_36				0x00800000		/* VDD voltage 3.5V  ~ 3.6V						*/

#define SUPPORTED_VOLTAGES 			(MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195)

#define SD_DATA_4BIT			    (1<<2)
#define SD_SWITCH_CHECK				0
#define SD_SWITCH_SWITCH			1

#define SD_HIGHSPEED_BUSY			0x00000200		/* Byte swapped									*/
#define SD_HIGHSPEED_SUPPORTED		0x00000200		/* Byte swapped									*/

#define SD_VERSION_SD				0x20000
#define SD_VERSION_2				(SD_VERSION_SD | 0x20)
#define SD_VERSION_1_0				(SD_VERSION_SD | 0x10)
#define SD_VERSION_1_10				(SD_VERSION_SD | 0x1a)
#define MMC_VERSION_MMC				0x10000
#define MMC_VERSION_UNKNOWN			(MMC_VERSION_MMC)
#define MMC_VERSION_1_2				(MMC_VERSION_MMC | 0x12)
#define MMC_VERSION_1_4				(MMC_VERSION_MMC | 0x14)
#define MMC_VERSION_2_2				(MMC_VERSION_MMC | 0x22)
#define MMC_VERSION_3				(MMC_VERSION_MMC | 0x30)
#define MMC_VERSION_4				(MMC_VERSION_MMC | 0x40)

#define MMC_MODE_HS					0x001
#define MMC_MODE_HS_52MHz			0x010
#define MMC_MODE_4BIT				0x100
#define MMC_MODE_8BIT				0x200
#define MMC_MODE_SPI				0x400
#define MMC_MODE_HC					0x800
#define HOST_CAPS 					(MMC_MODE_HS | MMC_MODE_HS_52MHz | MMC_MODE_HC | MMC_MODE_4BIT)

#define SD_IDMAC_OWN				(1U << 31)
#define SD_IDMAC_CH					(1U << 4)
#define SD_IDMAC_FS					(1U << 3)
#define SD_IDMAC_LD					(1U << 2)

typedef struct {
	uint32_t flags;
	uint32_t cnt;
	uint32_t addr;
	uint32_t next;
} SDdma_t;

static const int g_FreqBase[] =  {    10000,		/* Frequency bases divided by 10 to be nice		*/
                                     100000,		/* to platforms without floating point			*/
                                    1000000,
                                   10000000
                                 };

static const int g_SpeedMult[] = {  0,				/* Multiplier values for TRAN_SPEED.			*/
                                   10,				/* Multiplied by 10 to be nice to platforms		*/
                                   12,				/* without floating point						*/
                                   13,
                                   15,
                                   20,
                                   25,
                                   30,
                                   35,
                                   40,
                                   45,
                                   50,
                                   55,
                                   60,
                                   70,
                                   80,
                                 };


/* ------------------------------------------------------------------------------------------------ */
/* Some definitions																					*/

#if ((SDMMC_USE_MUTEX) != 0)						/* Mutex for exclusive access					*/
  #define SDMMC_MTX_LOCK(Cfg)		MTXlock(Cfg->MyMutex, -1)
  #define SDMMC_MTX_UNLOCK(Cfg)		MTXunlock(Cfg->MyMutex)
#else
  #define SDMMC_MTX_LOCK(Cfg)	  	0
  #define SDMMC_MTX_UNLOCK(Cfg)		OX_DO_NOTHING()
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Local variables																					*/

#if ((SDMMC_BUFFER_TYPE) == (SDMMC_BUFFER_UNCACHED))
 #if ((OX_CACHE_LSIZE) <= 8)
  #define DMA_ATTRIB __attribute__ ((aligned (8))) __attribute__ ((section (".uncached")))
 #else
  #define DMA_ATTRIB __attribute__ ((aligned (OX_CACHE_LSIZE))) __attribute__ ((section (".uncached")))
 #endif
#else
 #if ((OX_CACHE_LSIZE) <= 8)
  #define DMA_ATTRIB __attribute__ ((aligned (8)))
 #else
  #define DMA_ATTRIB __attribute__ ((aligned (OX_CACHE_LSIZE)))	/* Align on data cache lines		*/
 #endif
#endif
#if ((OX_CACHE_LSIZE) <= 64)
  static char            g_DMAscratch[SDMMC_MAX_DEVICES][64] DMA_ATTRIB;
#else
  static char            g_DMAscratch[SDMMC_MAX_DEVICES][OX_CACHE_LSIZE] DMA_ATTRIB;
#endif

static SDdma_t           g_DMAdesc[SDMMC_NMB_DEVICES][SDMMC_NUM_DMA_DESC] DMA_ATTRIB;

typedef struct {									/* Per device: configuration					*/
	volatile uint32_t *HW;							/* Base address of the SDMMC peripheral			*/
	volatile uint32_t *ClkReg;						/* Base address of the clock manager			*/
	volatile uint32_t *SysReg;						/* Base address of the system manager			*/
	int      IsInit;								/* If this device has been initialized			*/
	SDdma_t *DMAdesc;								/* DMA descriptor to use with this device		*/
	int      BlkLen;								/* Block length of the card						*/
	int      BlkLenRW;								/* Block length of the card when R/W			*/
	int64_t  Capacity;								/* Capacity (size) of the card					*/
	uint32_t CardCaps;								/* Capabilities of the card						*/
	uint32_t CardVer;								/* Version of the card							*/
	uint8_t  CID[16];
	uint32_t CSD[4];
	uint32_t OCR;
	uint32_t RCA;
	uint8_t  SCR[8];
	volatile uint32_t RawInter;						/* Raw interrupts (beforee masking)				*/
	uint32_t TranSpeed;
	SEM_t   *MySema;
  #if ((SDMMC_USE_MUTEX) != 0)						/* If mutex protection enable					*/
	MTX_t   *MyMutex;								/* One mutex per controller						*/
  #endif
  #if (((SDMMC_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1))
	int      SpinLock;
  #endif
  #if ((SDMMC_DEBUG) > 0)
	int      Dev;
  #endif
} MMCcfg_t;

static MMCcfg_t g_MMCcfg[SDMMC_NMB_DEVICES];		/* Device configuration							*/
static int      g_CfgIsInit = 0;					/* In case BSS is not zeroed					*/

/* ------------------------------------------------------------------------------------------------ */
/* Wait for the controller to terminate its reset sequence											*/
/* Value: type of reset to apply																	*/
/* Internal use																						*/
/* ------------------------------------------------------------------------------------------------ */

static int MMCwaitReset(MMCcfg_t *Cfg, uint32_t value)
{
int ii;												/* General purpose								*/
volatile uint32_t *Reg;
int Tout;											/* RTOS timer tick Time out value				*/

	Reg = Cfg->HW;

	Reg[SD_CTRL_REG] = value
				   #if (((SDMMC_OPERATION) & 2) != 0)/* Usign semaphore posted by interrupt			*/
	                 | SD_CTRL_INT_ENABLE
				   #endif
	                 | SD_IDMAC_EN;

	Tout = OS_MS_TO_MIN_TICK_EXP(10, 2);			/* This is very short, so use polling instead	*/
	do {											/* of TSKsleep()								*/
		ii = OS_HAS_TIMEDOUT(Tout);
	} while ((Reg[SD_CTRL_REG] & value)				/* The SDMMC clears all the bits when the		*/
	  &&     (ii == 0));							/* reset sequence is completed					*/

	if (ii != 0) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Unable to reset (1)\n", Cfg->Dev);
	  #endif
		return(-1);
	}

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* Prepare the data for a transfer																	*/
/* When requested/necessary, the data buffer is split and attached to the chained DMA descriptors	*/
/* Internal use																						*/
/* ------------------------------------------------------------------------------------------------ */

static int SDprepData(int Dev, MMCcfg_t *Cfg, MMCdata_t *Data)
{
unsigned int       BlockSize;						/* Size of the individual DMA blocks			*/
SDdma_t           *DMAdsc;							/* Pointer to DMA descriptor being set-up		*/
unsigned int       Flg;								/* Flags to set in the DMA descriptor			*/
         int       ii;								/* General purpose								*/
volatile uint32_t *Reg;								/* Base address of the SD/MMC device			*/
char              *StartAddr;						/* Start address of the individual DMA blocks	*/
unsigned int       TotalSize;						/* Total size of the data transfer				*/

	Reg       = Cfg->HW;
	TotalSize = Data->Nbytes;						/* Number of bytes left to transfer				*/
	BlockSize = (Data->Nbytes > 512U)				/* Select the correct block size for the xfer	*/
	          ? 512U								/* All SD/MMC can work with 512 bytes transfers	*/
	          : TotalSize;

	if (((TotalSize+511U)>>9U) > (sizeof(g_DMAdesc[0])/sizeof(g_DMAdesc[0][0]))) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - byte count to transfer is too large\n", Dev);
	  #endif
		return(-1);
	}

	if (0 != MMCwaitReset(Cfg, SD_CTRL_FIFO_RESET)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Unable to reset (2)\n", Dev);
	  #endif
		return(-2);
	}

	DMAdsc    = Cfg->DMAdesc;
	StartAddr = (char *)Data->Buffer;

  #if ((SDMMC_REMAP_LOG_ADDR) == 0)
	Reg[SD_DBADDR_REG] = (uint32_t)DMAdsc;
  #else
	Reg[SD_DBADDR_REG] = (uint32_t)MMUlog2Phy(DMAdsc);
  #endif

	ii = 0;
	while(TotalSize > 0) {							/* Distribute the data amongst DMA descriptors	*/
		Flg = (ii == 0)
		    ? (SD_IDMAC_OWN | SD_IDMAC_CH | SD_IDMAC_FS)
		    : (SD_IDMAC_OWN | SD_IDMAC_CH);

		if (TotalSize > BlockSize) {				/* Transfer of a block that is not the last one	*/
			DMAdsc[ii].cnt = (uint32_t)BlockSize;
			TotalSize     -= BlockSize;
		}
		else {										/* Transfer of the last block					*/
			DMAdsc[ii].cnt = (uint32_t)TotalSize;
			Flg           |= SD_IDMAC_LD;
			TotalSize      = 0;
		}
		DMAdsc[ii].flags   = (uint32_t)Flg;			/* Put DMA flags for this block in the DMA desc	*/
		DMAdsc[ii++].addr  = (uint32_t)StartAddr;
		StartAddr         += BlockSize;				/* Be ready for the next iteration				*/
	}

  #if ((SDMMC_BUFFER_TYPE) != (SDMMC_BUFFER_UNCACHED))/* If buffers are cached, clean & invalidate	*/
	DCacheFlushRange(Cfg->DMAdesc, ii * sizeof(g_DMAdesc[0]));
  #endif

	Reg[SD_CTRL_REG]   = Reg[SD_CTRL_REG]
	                   | SD_IDMAC_EN;
	Reg[SD_BMOD_REG]   = Reg[SD_BMOD_REG]
	                   | SD_BMOD_IDMAC_FB
	                   | SD_BMOD_IDMAC_EN;
	Reg[SD_BLKSIZ_REG] = (uint32_t)BlockSize;
	Reg[SD_BYTCNT_REG] = (uint32_t)Data->Nbytes;

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: mmc_sendcmd																			*/
/*																									*/
/* mmc_sendcmd - send a command to the SD/MMC card													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int mmc_sendcmd((int Dev, unsigned int Cmd, uint32_t Arg, uint32_t Exp, uint32_t *Resp,		*/
/*		                  MMCdata_t *Data)															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : SD/MMC device number to send the request												*/
/*		Cmd  : command to send to the SD/MMC														*/
/*		       They are defined in dw_sdmmc.h (defined as MMC_CMD_$$$)								*/
/*		Arg  : argument to the command																*/
/*		Exp  : expected type of response															*/
/*		       They are defined in dw_sdmmc.h (defined as MMC_RSP_$$$)								*/
/*		Resp : response received from the SD/MMC													*/
/*		       Can be NULL is not needed by caller													*/
/*		Data : data exchanged with the SD/MMC														*/
/*		       can be NULL																			*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*      != 0 : error																				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int mmc_sendcmd(int Dev, unsigned int Cmd, uint32_t Arg, uint32_t Exp, uint32_t *Resp,MMCdata_t *Data)
{
MMCcfg_t          *Cfg;
int                ii;								/* General purpose								*/
uint32_t           Flags;							/* Flags set-up for the command register		*/
volatile uint32_t *MyRawInt;						/* Pointer to g_RawInter[Dev]					*/
volatile uint32_t *Reg;								/* Base address of the SD/MMC device			*/
int                ReMap;							/* Device remap index							*/
int                Retry;							/* Retry count									*/
int                RetVal;							/* Propagated return value from called function	*/
int                Tout;							/* RTOS timer tick Time out value				*/
#if ((SDMMC_REMAP_LOG_ADDR) != 0)
  void *BufBack;
#endif
#if ((SDMMC_OPERATION) == 1)
  int ISRstate;
#endif

  #if ((SDMMC_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_MMCcfg[ReMap];

  #else
	if ((Dev < 0)									/* Device # must be within range to be valid	*/
	||  (Dev >= SDMMC_MAX_DEVICES)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Invalid device number\n", Dev);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by MY_UART_LIST_DEVICE			*/
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - cannot remap device\n", Dev);
	  #endif
		return(-2);
	}

	Cfg = &g_MMCcfg[ReMap];
	if ((g_CfgIsInit == 0)							/* Make sure SDMMC device has been initialized	*/
	||  (Cfg->IsInit == 0)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Uninitialized device\n", Dev);
	  #endif
		return(-3);
	}
  #endif

	Reg       = Cfg->HW;
	MyRawInt = &Cfg->RawInter;

  #if ((SDMMC_REMAP_LOG_ADDR) != 0)
	BufBack = NULL;									/* To remove compilker warning					*/
	if (Data != (MMCdata_t *)NULL) {
		BufBack = (void *)Data->Buffer;
	}
  #endif

	if (0 != SDMMC_MTX_LOCK(Cfg)) {					/* Now we need to have exclusive access			*/
		return(-4);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	Retry = 3;
	do {
		RetVal = 0;
		Tout   = OS_MS_TO_MIN_TICK_EXP(1000, 5);	/* This is normally fairly short, use smallest	*/
		while(((Reg[SD_STATUS_REG] & SD_BUSY) != 0)
		&&    (0 == OS_HAS_TIMEDOUT(Tout)));

		if (0 != OS_HAS_TIMEDOUT(Tout)) {
		  #if ((SDMMC_DEBUG) > 0)
			printf("SDMMC [Dev:%d] - Error - Timed out waiting to respond", Dev);
		  #endif
			SDMMC_MTX_UNLOCK(Cfg);
			return(-5);								/* Exit because if after 1 sec it fails there	*/
		}											/* is little chance retrying will succeed		*/

		Flags = ((Cmd == MMC_CMD_STOP_TRANSMISSION)
		        ? SD_CMD_ABORT_STOP
		        : SD_CMD_PRV_DAT_WAIT)
		      | Cmd
		      | SD_CMD_START
		      | SD_CMD_USE_HOLD_REG;

		if (Data != NULL) {
		  #if ((SDMMC_REMAP_LOG_ADDR) != 0)
			Data->Buffer = BufBack;					/* Needed in case retrying						*/
		  #endif

			if (0 != SDprepData(Dev, Cfg, Data)) {
			  #if ((SDMMC_DEBUG) > 0)
				printf("SDMMC [Dev:%d] - Error - Failed to prepare data\n", Dev);
			  #endif
				SDMMC_MTX_UNLOCK(Cfg);
				return(-6);							/* Exit as there are no possible recovery from	*/
			}										/* this one										*/
													/* If buffers are cached, clean & invalidate	*/
		  #if ((SDMMC_BUFFER_TYPE) != (SDMMC_BUFFER_UNCACHED))
			DCacheFlushRange((void *)Data->Buffer, (int)Data->Nbytes);
		  #endif									/* Works for both DMA read and write			*/
		  #if ((SDMMC_REMAP_LOG_ADDR) != 0)
			Data->Buffer = MMUlog2Phy(Data->Buffer);
		  #endif

			Flags |= (Data->Flags & MMC_DATA_WRITE)
			       ? (SD_CMD_DATA_EXP | SD_CMD_RW)
			       :  SD_CMD_DATA_EXP;
		}

		if ((Exp & SD_CMD_RSP_PRESENT) != 0) {
			Flags |= SD_CMD_RESP_EXP;
			if (Exp & SD_CMD_RSP_136) {
				Flags |= SD_CMD_RESP_LENGTH;
			}
		}

		if ((Exp & SD_CMD_RSP_CRC) != 0) {
			Flags |= SD_CMD_CHECK_CRC;
		}

		ii = 1000;
		if (Data != NULL) {							/* Select time out according to # bits to xfer	*/
			ii = Data->Nbytes << 3;
			ii = 4 * ((ii+1251)/(1250));			/* Slowest data rate is 12.5 Mbps				*/
		}											/* +1251 is a safety headroom					*/

		Tout = ii									/* Timeout in SD/MMC controller register		*/
		     * Cfg->TranSpeed;
		if (((uint32_t)Tout) >= 0x00FFFFFF) {
			Tout =  0xFFFFFFFF;
		}
		Reg[SD_TMOUT_REG]   = (Tout << 8)
		                    | 0xFF;
		Reg[SD_RINTSTS_REG] = SD_INTMSK_ALL;		/* Clear all pending interrupts					*/
		*MyRawInt           = 0;					/* Clear previous raw interrupt results			*/

	  #if (((SDMMC_OPERATION) & 2) == 0)			/* Polling										*/
		Reg[SD_CMDARG_REG]  = Arg;
		Reg[SD_CMD_REG]     = Flags;				/* Send command/ trigger the data exchange		*/

	   #if ((SDMMC_OPERATION) & 1)					/* Disable interrupts during transfers			*/
		ISRstate = OSintOff();
	   #endif

		Tout = OS_MS_TO_MIN_TICK_EXP(5000, 2);
		do {
			do {
				*MyRawInt |= Reg[SD_RINTSTS_REG];
				if (*MyRawInt & SD_INTMSK_CD) {		/* Card removed, can't use it anymore			*/
					Reg[SD_INTMASK_REG] = 0;		/* Disable all interrupts						*/
					Cfg->IsInit         = 0;		/* Declare the card as un-initialized			*/
				}
				Reg[SD_RINTSTS_REG] = *MyRawInt;
			  #if (((SDMMC_OPERATION) & 1) && (SDMMC_TOUT_ISR_ENB))
				OSintBack(ISRstate);
				ii = OS_HAS_TIMEDOUT(Tout);
				ISRstate = OSintOff();
			  #else
				ii = OS_HAS_TIMEDOUT(Tout);
			  #endif
			} while (0U != (*MyRawInt & Reg[SD_MINTSTS_REG]));
		} while (((*MyRawInt & SD_INTMSK_CDONE) == 0)
		  &&     (ii == 0));

		Tout = ii;

	   #if ((SDMMC_OPERATION) & 1)					/*Disable interrupts during transfers			*/
		OSintBack(ii);
	   #endif

			if ((Tout != 0)							/* Semaphore timeout							*/
			||  ((*MyRawInt & ( SD_DATA_ERR			/* Transfer error								*/
			                  | SD_DATA_TOUT
			                  | SD_INTMSK_RE
			                  | SD_INTMSK_RTO)) != 0)) {
				RetVal = -6;
			}
		}
	  #else											/* Semaphore posted by interrupts				*/

		SEMreset(Cfg->MySema);						/* Clear any pending postings in the semaphore	*/

		Reg[SD_CMDARG_REG]  = Arg;
		Reg[SD_CMD_REG]     = Flags;				/* Send command/ trigger the data exchange		*/

		if ((*MyRawInt & SD_INTMSK_CDONE) == 0) {	/* Could have been preempted: command done		*/
			Tout = SEMwait(Cfg->MySema, OS_MS_TO_MIN_TICK(5000,2));	/* Wait posting with timeout*/
			if ((Tout != 0)							/* Semaphore timeout							*/
			||  ((*MyRawInt & ( SD_DATA_ERR			/* Transfer error								*/
			                  | SD_DATA_TOUT
			                  | SD_INTMSK_RE
			                  | SD_INTMSK_RTO)) != 0)) {
				RetVal = -6;
			}
		}
	  #endif

		if ((RetVal == 0)
		&&  ((*MyRawInt & SD_INTMSK_CDONE) == 0)) {
		  #if ((SDMMC_DEBUG) > 0)
			printf("SDMMC [Dev:%d] - Error - Command not completed\n", Dev);
		  #endif
			RetVal = -7;
		}

		if (Resp != (uint32_t *)NULL) {				/* Valid transfer here							*/
			if ((Exp & SD_CMD_RSP_PRESENT) != 0) {
				if (Exp & SD_CMD_RSP_136) {
					Resp[0] = Reg[SD_RESP3_REG];
					Resp[1] = Reg[SD_RESP2_REG];
					Resp[2] = Reg[SD_RESP1_REG];
					Resp[3] = Reg[SD_RESP0_REG];
				}
				else {
					Resp[0] = Reg[SD_RESP0_REG];
				}
			}
		}

		if ((Data != NULL)
		&&  (RetVal == 0)
		&&  ((*MyRawInt & SD_INTMSK_DT) == 0)) {
		  #if (((SDMMC_OPERATION) & 2) == 0)		/* Polling										*/

		   #if ((SDMMC_OPERATION) & 1)				/*Disable interrupts during transfers			*/
			ISRstate = OSintOff();
		   #endif

			Tout = OS_MS_TO_MIN_TICK_EXP(1000, 5);
			do {
				do {
					*MyRawInt |= Reg[SD_RINTSTS_REG];
					if (*MyRawInt & SD_INTMSK_CD) { {/* Card removed, can't use it anymore			*/
						Reg[SD_INTMASK_REG] = 0;	/* Disable all interrupts						*/
						Cfg->IsInit         = 0;	/* Declare the card as un-initialized			*/
					}
					Reg[SD_RINTSTS_REG] = *MyRawInt;
				  #if (((SDMMC_OPERATION) & 1) && (SDMMC_TOUT_ISR_ENB))
					OSintBack(ISRstate);
					ii = OS_HAS_TIMEDOUT(Tout);
					ISRstate=OSintOff();
				  #else
					ii = OS_HAS_TIMEDOUT(Tout);
				  #endif
				} while (0U != (*MyRawInt & Reg[SD_MINTSTS_REG]));
			} while ((0 == (*MyRawInt & SD_INTMSK_DT))
			  &&     (ii == 0));

			Tout = ii;

		   #if ((SDMMC_OPERATION) & 1)				/*Disable interrupts during transfers			*/
			OSintBack(ISRstate);
		   #endif

		  #else										/* Semaphore posted by interrupts				*/
			Tout = SEMwait(Cfg->MySema, OS_MS_TO_MIN_TICK(1000,2));
		  #endif
			if ((Tout != 0)							/* Xfer timeout									*/
			||  ((*MyRawInt & ( SD_DATA_ERR			/* Transfer error								*/
			                  | SD_DATA_TOUT)) != 0)) {
				RetVal = -8;
			}
			if ((RetVal == 0)
			&&  ((*MyRawInt & SD_INTMSK_DT) == 0)) {
			  #if ((SDMMC_DEBUG) > 0)
				printf("SDMMC [Dev:%d] - Error - Data transfer not completed\n", Dev);
				if (Retry > 1) {
					printf("SDMMC [Dev:%d]           Retrying\n", Dev);
				}
			  #endif
				RetVal = -9;
			}
		}

	  #if ((SDMMC_DEBUG) > 0)
		if ((RetVal == -6)
		||  (RetVal == -8)) {
			if ((*MyRawInt & SD_DATA_ERR) != 0) {
				printf("SDMMC [Dev:%d] - Error - Data error (%d)\n", Dev, RetVal);
			}
			if ((*MyRawInt & SD_DATA_TOUT) != 0) {
				printf("SDMMC [Dev:%d] - Error - Data timeout (%d)\n", Dev, RetVal);
			}
			if ((*MyRawInt & SD_INTMSK_RE) != 0) {
				printf("SDMMC [Dev:%d] - Error - Response Error (%d)\n", Dev, RetVal);
			}
			if ((*MyRawInt & SD_INTMSK_RTO) != 0) {
				printf("SDMMC [Dev:%d] - Error - Response Timeout (%d)\n", Dev, RetVal);
			}
			if (Tout != 0) {
				printf("SDMMC [Dev:%d] - Error - Semaphore posting timeout (%d)\n", Dev, RetVal);
			}
		}
	  #endif
	}
	while ((--Retry > 0)
	&&     (RetVal != 0));

  #if ((SDMMC_REMAP_LOG_ADDR) != 0)
	if (Data != (MMCdata_t *)NULL) {				/* Put back logical address as the caller will	*/
		Data->Buffer = BufBack;						/* most likely use it							*/
	}
  #endif

  #if ((SDMMC_BUFFER_TYPE) != (SDMMC_BUFFER_UNCACHED))
	if (Data != NULL) {								/* If cache branch prediction used, needed when	*/
		DCacheInvalRange((void *)Data->Buffer, (int)Data->Nbytes);	/* DMA is writing to memory		*/
	}												/* Works for both DMA read and write			*/
  #endif

	SDMMC_MTX_UNLOCK(Cfg);

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: mmc_sendstatus																			*/
/*																									*/
/* mmc_sendstatus - send a status request to the SD/MMC card										*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int mmc_sendstatus(int Dev);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : SD/MMC device number to send the request												*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*      != 0 : error																				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int mmc_sendstatus(int Dev)
{
MMCcfg_t *Cfg;
uint32_t CmdResp[4];								/* Response to a command sent to SD/MMC			*/
int      ii;										/* General purpose								*/
int      ReMap;										/* Device remap index							*/

  #if ((SDMMC_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_MMCcfg[ReMap];

  #else
	if ((Dev < 0)									/* Device # must be within range to be valid	*/
	||  (Dev >= SDMMC_MAX_DEVICES)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Invalid device number\n", Dev);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by MY_UART_LIST_DEVICE			*/
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - cannot remap device\n", Dev);
	  #endif
		return(-2);
	}

	Cfg = &g_MMCcfg[ReMap];
	if ((g_CfgIsInit == 0)							/* Make sure SDMMC device has been initialized	*/
	||  (Cfg->IsInit == 0)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Uninitialized device\n", Dev);
	  #endif
		return(-3);
	}
  #endif

	if (0 != SDMMC_MTX_LOCK(Cfg)) {					/* Now we need to have exclusive access			*/
		return(-4);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	for (ii=OS_MS_TO_MIN_TICK(10,3) ; ii>=0; ii--) {
		if (0 == mmc_sendcmd(Dev, MMC_CMD_SEND_STATUS, Cfg->RCA<<16, MMC_RSP_R1, &CmdResp[0],
		                     NULL)) {
			if (( CmdResp[0] & MMC_STATUS_RDY_FOR_DATA)
			&&  ((CmdResp[0] & MMC_STATUS_CURR_STATE) != MMC_STATE_PRG)) {
				break;
			}
			if (CmdResp[0] & MMC_STATUS_MASK) {
			  #if ((SDMMC_DEBUG) > 0)
				printf("SDMMC [Dev:%d] - Error - Status Error: 0x%08X\n", Dev, (int)CmdResp[0]);
			  #endif
				SDMMC_MTX_UNLOCK(Cfg);
				return(-3);
			}
		}
		TSKsleep(OS_MS_TO_MIN_TICK(1,1));
	}

	if (ii < 0) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Timed out waiting for status\n", Dev);
	  #endif
		SDMMC_MTX_UNLOCK(Cfg);
		return(-4);
	}

	SDMMC_MTX_UNLOCK(Cfg);

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* Switch speed																						*/
/* Internal use																						*/
/* ------------------------------------------------------------------------------------------------ */

static int SDswitch(int Dev, int Mode, int Group, int Value, char *Resp)
{
MMCdata_t    Data;
unsigned int Val;

	Val	 = (Mode << 31)								/* Switch the frequency							*/
	     | 0x00FFFFFF;
	Val &= ~(0xF << (Group * 4));
	Val |= Value << (Group * 4);

	Data.Buffer = Resp;
	Data.Nbytes = 64U;
	Data.Flags  = MMC_DATA_READ;

	return(mmc_sendcmd(Dev, MMC_CMD_SWITCH, Val, MMC_RSP_R1b, NULL, &Data));
}

/* ------------------------------------------------------------------------------------------------ */
/* Change the bus speed of the SD/MMC card															*/
/* Internal use																						*/
/* ------------------------------------------------------------------------------------------------ */

static int SDchangeFreq(int Dev)
{
MMCcfg_t    *Cfg;
int       ii;										/* General purpose								*/
MMCdata_t Data;
int          ReMap;
char     *SCR;
uint32_t *SwitchStatus;

	ReMap = g_DevReMap[Dev];
	Cfg   = &g_MMCcfg[ReMap];

	Cfg->CardCaps = 0;
	SCR           = &g_DMAscratch[ReMap][0];

	if (0 != mmc_sendcmd(Dev, MMC_CMD_APP_CMD, Cfg->RCA<<16, MMC_RSP_R1, NULL, NULL)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Command prefix send error (1)\n", Dev);
	  #endif
		return(-1);									/* Read the SCR to find out if this card		*/
	}												/* supports higher speeds						*/

	Data.Buffer = SCR;
	Data.Nbytes = 8U;
	Data.Flags  = (unsigned int)MMC_DATA_READ;

	for (ii=3; ii>0; ii--) {						/* Try 3 times for reliability					*/
		if (0 == mmc_sendcmd(Dev, MMC_CMD_APP_SEND_SCR, 0, MMC_RSP_R1, NULL, &Data)) {
			break;
		}
	}
	if (ii <= 0) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Timed out waiting for SCR\n", Dev);
	  #endif
		return(-2);
	}

	memmove(&Cfg->SCR[0], SCR, 8);

	switch (SCR[0] & 0xf) {							/* Extract the card version						*/
		case 0:
			Cfg->CardVer = SD_VERSION_1_0;
			break;
		case 1:
			Cfg->CardVer = SD_VERSION_1_10;
			break;
		case 2:
			Cfg->CardVer = SD_VERSION_2;
			break;
		default:
			Cfg->CardVer = SD_VERSION_1_0;
			break;
	}

	if (SCR[1] & SD_DATA_4BIT) {					/* Merge if 1 or 4 bit data bus					*/
		Cfg->CardCaps |= MMC_MODE_4BIT;
	}

	if (Cfg->CardVer == SD_VERSION_1_0) {			/* Version 1.0 doesn't support switching		*/
		return(0);									/* So we are done								*/
	}

	SwitchStatus = (uint32_t *)&g_DMAscratch[ReMap][0];
	for (ii=4; ii>0; ii--) {
		if (0 != SDswitch(Dev, SD_SWITCH_CHECK, 0, 1, (char *)SwitchStatus)) {
		  #if ((SDMMC_DEBUG) > 0)
			printf("SDMMC [Dev:%d] - Error - Cannot check switch status\n", Dev);
		  #endif
			return(-3);
		}

		if ((SwitchStatus[7] & SD_HIGHSPEED_BUSY) == 0) {	/* SD_HIGHSPEED_SUPPORTED already swap	*/
			break;									/* The high-speed function is busy.	Try again	*/
		}
	}

	if (ii <= 0) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Timed out waiting for Switch\n", Dev);
	  #endif
		return(-4);
	}

	if ((SwitchStatus[3] & SD_HIGHSPEED_SUPPORTED) == 0) {	/* SD_HIGHSPEED_SUPPORTED already swap	*/
		return(0);									/* If high-speed isn't supported, we return		*/
	}

	if (0 != SDswitch(Dev, SD_SWITCH_SWITCH, 0, 1, (char *)SwitchStatus)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Cannot switch to high speed\n", Dev);
	  #endif
		return(-5);
	}

	if ((SwitchStatus[4] & 0x0000000F) == 0x00000001) {
		Cfg->CardCaps |= MMC_MODE_HS;
	}

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* Perform the start-up to get the SD card on-line													*/
/* Internal use																						*/
/* ------------------------------------------------------------------------------------------------ */

static int MMCstartup(int Dev)
{
MMCcfg_t    *Cfg;
uint32_t     CmdResp[4];							/* Response to a command sent to SD/MMC			*/
uint64_t     CMult;									/* Card size parameters							*/
uint64_t     CSize;									/* Card size parameters							*/
unsigned int Freq;
         int ii;
unsigned int Mult;
volatile uint32_t *Reg;								/* Base address of the SD/MMC device			*/
int          ReMap;
unsigned int Val;

	ReMap = g_DevReMap[Dev];
	Cfg   = &g_MMCcfg[ReMap];
	Reg   = Cfg->HW;
													/* Put the Card in Identify Mode 				*/
	if (0 != mmc_sendcmd(Dev, MMC_CMD_ALL_SEND_CID, 0, MMC_RSP_R2, &CmdResp[0], NULL)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Could not put card in Identify Mode\n", Dev);
	  #endif
		return(-1);
	}

	memmove(Cfg->CID, &CmdResp[0], sizeof(Cfg->CID));	/* For MMC cards, set the Relative Address	*/
													/* For SD cards, get the Relative Address		*/
													/* This also PUTS the card into Standby State	*/
	if (0 != mmc_sendcmd(Dev, MMC_CMD_SET_RELATIVE_ADDR, Cfg->RCA<<16, MMC_RSP_R6, &CmdResp[0],
	                     NULL)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Could not get card Relative Address\n", Dev);
	  #endif
		return(-2);
	}

	Cfg->RCA = (CmdResp[0]>>16)						/* Get the Card Specific Data					*/
	         & 0xffff;

	if (0 != mmc_sendcmd(Dev, MMC_CMD_SEND_CSD, Cfg->RCA<<16, MMC_RSP_R2, &CmdResp[0], NULL)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Could not get CSD\n", Dev);
	  #endif
		return(-3);
	}

	Cfg->CSD[0] = CmdResp[0];
	Cfg->CSD[1] = CmdResp[1];
	Cfg->CSD[2] = CmdResp[2];
	Cfg->CSD[3] = CmdResp[3];

  #if ((SDMMC_DEBUG) > 1)
	printf("SDMMC [Dev:%d] init: CSD 0x", Dev);
	for (ii=0 ; ii<4 ; ii++) {
		printf("%08X", (unsigned)CmdResp[ii]);
	}
	printf("\nSDMMC [Dev:%d] init: waiting for ready status\n", Dev);
  #endif

	if (0 != mmc_sendstatus(0)) {					/* Waiting for the ready status					*/
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Could not get Status\n", Dev);
	  #endif
		return(-4);
	}

	Freq            = g_FreqBase[(Cfg->CSD[0] & 0x7)];	/* Freq/10: multipliers 10x bigger			*/
	Mult            = g_SpeedMult[((Cfg->CSD[0] >> 3) & 0xf)];
	Cfg->TranSpeed  = (Freq
	                * Mult) / 1000;
	Cfg->BlkLen     = 1 << ((Cfg->CSD[1] >> 16) & 0xf);

	if ((Cfg->OCR & OCR_HCS) == OCR_HCS) {
		CSize = (Cfg->CSD[1] & 0x0000003f) << 16
	          | (Cfg->CSD[2] & 0xffff0000) >> 16;
		CMult = 8;
	}
	else {
		CSize = (Cfg->CSD[1] & 0x000003ff) <<  2
		      | (Cfg->CSD[2] & 0xc0000000) >> 30;
		CMult = (Cfg->CSD[2] & 0x00038000) >> 15;
	}
	Cfg->Capacity = ((CSize+1) << (CMult+2))
	              * (uint64_t)Cfg->BlkLen;
													/* Select the card, and put it into Xfer Mode	*/
	if (0 != mmc_sendcmd(Dev, MMC_CMD_SELECT_CARD, Cfg->RCA<<16, MMC_RSP_R1b,&CmdResp[0],NULL)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Could not put card into Transfer mode\n", Dev);
	  #endif
		return(-5);
	}

	Cfg->BlkLenRW = 512;							/* Block length can only be set in Xfer mode	*/
	if ((Cfg->OCR & OCR_HCS) != OCR_HCS) {			/* Setting block length only needed for SDSC	*/
		if (0 != mmc_sendcmd(Dev, MMC_CMD_SET_BLOCKLEN, Cfg->BlkLen, MMC_RSP_R1, &CmdResp[0], NULL)) {
		  #if ((SDMMC_DEBUG) > 0)
			printf("SDMMC [Dev:%d] - Error - Could not set card block length\n", Dev);
		  #endif
			return(-6);
		}
	}

	if (0 != SDchangeFreq(Dev)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Could not change card frequency\n", Dev);
	  #endif
		return(-7);
	}

	Cfg->CardCaps &= HOST_CAPS;					/* Restrict CardCaps to host capabilities		*/

	if (Cfg->CardCaps & MMC_MODE_4BIT) {
		if (0 != mmc_sendcmd(Dev, MMC_CMD_APP_CMD, Cfg->RCA<<16, MMC_RSP_R1, &CmdResp[0],NULL)) {
		  #if ((SDMMC_DEBUG) > 0)
			printf("SDMMC [Dev:%d] - Error - Could not change to 4-bit mode (1)\n", Dev);
		  #endif
			return(-8);
		}
		if (0 != mmc_sendcmd(Dev, MMC_CMD_SWITCH, 2, MMC_RSP_R1, &CmdResp[0], NULL)) {
		  #if ((SDMMC_DEBUG) > 0)
			printf("SDMMC [Dev:%d] - Error - Could not change to 4-bit mode (2)\n", Dev);
		  #endif
			return(-9);
		}
		Reg[SD_CTYPE_REG] = SD_CTYPE_4BIT;
	}

	Reg[SD_CLKENA_REG] = 0U;						/* Set the bus speed							*/
	Reg[SD_CLKSRC_REG] = 0U;						/* Must always be 0								*/
	Cfg->TranSpeed     = (Cfg->CardCaps & MMC_MODE_HS)
	                   ? (50000000/1000)
	                   : (25000000/1000);
	Reg[SD_CLKDIV_REG] = (Cfg->CardCaps & MMC_MODE_HS)
	                   ? (((SDMMC_CLK) + 25000000)
		                      / 50000000)			/* Bus speed 50M, +25M for rounding				*/
		                   / 2						/* Resulting division is 2 * value in Register	*/
	                   : (((SDMMC_CLK) + 12500000)
		                      / 25000000)			/* Bus speed 25M, +12.5M for rounding			*/
	 	                   / 2;						/* Resulting division is 2 * value in Register	*/
	Reg[SD_CMD_REG]    = SD_CMD_PRV_DAT_WAIT
	                   | SD_CMD_UPD_CLK
	                   | SD_CMD_START;
	Val                = Cfg->ClkReg[CLKMGR_PERPLLGRP_EN_REG];	/* Disable SDMMC clocks				*/
	Val               &= ~CLKMGR_SDMMC_CLK_ENABLE;
	Cfg->ClkReg[CLKMGR_PERPLLGRP_EN_REG]  = Val;
	Cfg->SysReg[SYSMGR_SDMMCGRP_CTRL_REG] = SYSMGR_SDMMC_CTRL_SET(Dev, 3);/*sample=0 drive=3		*/
	Val                = Cfg->ClkReg[CLKMGR_PERPLLGRP_EN_REG];		/* Enable SDMMC clocks			*/
	Val               |= CLKMGR_SDMMC_CLK_ENABLE;
	Cfg->ClkReg[CLKMGR_PERPLLGRP_EN_REG]  = Val;

	ii = OS_MS_TO_MIN_TICK(1000, 2);
	while(((Reg[SD_CMD_REG] & SD_CMD_START) != 0)
	&&    (ii >= 0)) {
		TSKsleep(OS_MS_TO_MIN_TICK(10, 1));
		ii -= OS_MS_TO_MIN_TICK(10, 1);
	}

	if (ii < 0) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - SD/MMC did not respond (1)\n", Dev);
	  #endif
		return(-10);
	}

	Reg[SD_CLKENA_REG] = SD_CLKEN_ENABLE;
	Reg[SD_CMD_REG]    = SD_CMD_PRV_DAT_WAIT
	                      | SD_CMD_UPD_CLK
	                      | SD_CMD_START;

	ii = OS_MS_TO_MIN_TICK(1000,2);
	while(((Reg[SD_CMD_REG] & SD_CMD_START) != 0)
	&&    (ii >= 0)) {
		TSKsleep(OS_MS_TO_MIN_TICK(10, 1));
		ii -= OS_MS_TO_MIN_TICK(10, 1);
	}

	if (ii < 0) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - SD/MMC did not respond (2)\n", Dev);
	  #endif
		return(-11);
	}

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: mmc_read																				*/
/*																									*/
/* mmc_read - read data from the sd/mmc card														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int mmc_read(int Dev, uint32_t Blk, void *Buf, uint32_t Nblk);								*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : SD/MMC device number																	*/
/*      Blk  : index of the first block to read														*/
/*      Buf  : where the data read lands															*/
/*		Nblk : number of 512 byte (BlkLenRW) blocks to read											*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>= 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int mmc_read(int Dev, uint32_t Blk, void *Buf, uint32_t Nblk)
{
MMCcfg_t    *Cfg;
int          ii;									/* General purpose								*/
uint32_t     Left;									/* Number of block left to be read				*/
MMCdata_t    MMCdata;								/* MMC DMA descriptor							*/
int          ReMap;
unsigned int Size;									/* Single tranfer size							*/
uint32_t     Snow;

  #if ((SDMMC_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_MMCcfg[ReMap];

  #else
	if ((Dev < 0)									/* Device # must be within range to be valid	*/
	||  (Dev >= SDMMC_MAX_DEVICES)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Invalid device number\n", Dev);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by MY_UART_LIST_DEVICE			*/
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - cannot remap device\n", Dev);
	  #endif
		return(-2);
	}

	Cfg = &g_MMCcfg[ReMap];
	if ((g_CfgIsInit == 0)							/* Make sure SDMMC device has been initialized	*/
	||  (Cfg->IsInit == 0)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Uninitialized device\n", Dev);
	  #endif
		return(-3);
	}

	if (Nblk == 0) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - read  number of block == 0\n", Dev);
	  #endif
		return(-4);
	}
  #endif

  #if ((SDMMC_DEBUG) > 1)
	printf("SDMMC [Dev:%d] - Reading %u block of %d bytes starting at block %u\n", Dev,
	      (unsigned int)Nblk, Cfg->BlkLenRW, (unsigned int)Blk);
  #endif

	Left = Nblk;
	do {											/* May have to do multiple read if the request	*/
		Size = Left;
		if (Size > (sizeof(g_DMAdesc[0])/sizeof(g_DMAdesc[0][0]))) {/* Max transfer size limited by	*/
			Size = (sizeof(g_DMAdesc[0])/sizeof(g_DMAdesc[0][0]));	/* the # number of DMA descrip	*/
		}
		MMCdata.Nbytes = Size
		               * Cfg->BlkLenRW;
		MMCdata.Buffer = (char *)Buf;
		MMCdata.Flags  = MMC_DATA_READ;

		Snow = Blk;									/* Current sector to start reading from			*/
		if ((Cfg->OCR & OCR_HCS) != OCR_HCS) {
			Snow *= Cfg->BlkLenRW;
		}

		ii = mmc_sendcmd(Dev, (Size > 1)
		                     ? MMC_CMD_READ_MULTIPLE_BLOCK
		                     : MMC_CMD_READ_SINGLE_BLOCK, Snow, MMC_RSP_R1, NULL, &MMCdata);
	  #if 1											/* This does not work on MMC (full size) cards	*/
		if ((ii == 0)
		&&  (Size > 1)) {							/* Stop transmission when multi-block			*/
			ii = mmc_sendcmd(0, MMC_CMD_STOP_TRANSMISSION, 0, MMC_RSP_R1b, NULL, NULL);
		}
	  #endif

		Left -= Size;								/* This less to transfer the next time			*/
		Blk  += Size;								/* Next sector to start reading from			*/
		Buf   = (Size * Cfg->BlkLenRW)
		      + (uint8_t *)Buf;
	} while ((ii == 0)
	  &&     (Left > 0));

	return((ii!=0) ? -2 : 0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: mmc_write																				*/
/*																									*/
/* mmc_write - write data to the sd/mmc card														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int mmc_write(int Dev, Blk, Buf, Nblk);														*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : SD/MMC device number																	*/
/*      Blk  : index of the first block to write													*/
/*      Buf  : data to write																		*/
/*		Nblk : number of 512 byte (BlkLenRW) blocks to write										*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int mmc_write(int Dev, uint32_t Blk, const void *Buf, uint32_t Nblk)
{
MMCcfg_t    *Cfg;
int          ii;									/* General purpose								*/
uint32_t     Left;									/* Number of block left to be read				*/
MMCdata_t    MMCdata;								/* MMC DMA descriptor							*/
int          ReMap;
unsigned int Size;									/* Single tranfer size							*/
uint32_t     Snow;

  #if ((SDMMC_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_MMCcfg[ReMap];

  #else
	if ((Dev < 0)									/* Device # must be within range to be valid	*/
	||  (Dev >= SDMMC_MAX_DEVICES)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Invalid device number\n", Dev);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by MY_UART_LIST_DEVICE			*/
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - cannot remap device\n", Dev);
	  #endif
		return(-2);
	}

	Cfg = &g_MMCcfg[ReMap];
	if ((g_CfgIsInit == 0)							/* Make sure SDMMC device has been initialized	*/
	||  (Cfg->IsInit == 0)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Uninitialized device\n", Dev);
	  #endif
		return(-3);
	}

	if (Nblk == 0) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - write number of block == 0\n", Dev);
	  #endif
		return(-4);
	}
  #endif


  #if ((SDMMC_DEBUG) > 1)
	printf("SDMMC [Dev:%d] - Writing %u block of %d bytes starting at block %u\n", Dev,
	      (unsigned int)Nblk, Cfg->BlkLenRW, (unsigned int)Blk);
  #endif

	Left = Nblk;
	do {											/* May have to do multiple read if the request	*/
		Size = Left;
		if (Size > (sizeof(g_DMAdesc[0])/sizeof(g_DMAdesc[0][0]))) {/* Max transfer size limited by	*/
			Size = (sizeof(g_DMAdesc[0])/sizeof(g_DMAdesc[0][0]));	/* the # number of DMA descrip	*/
		}
		MMCdata.Nbytes = Size
		               * Cfg->BlkLenRW;
		MMCdata.Buffer = (char *)Buf;
		MMCdata.Flags  = MMC_DATA_WRITE;

		Snow = Blk;									/* Current sector to start writing to			*/
		if ((Cfg->OCR & OCR_HCS) != OCR_HCS) {
			Snow *= Cfg->BlkLenRW;
		}

		ii = mmc_sendcmd(Dev, (Size > 1)
		                     ? MMC_CMD_WRITE_MULTIPLE_BLOCK
		                     : MMC_CMD_WRITE_SINGLE_BLOCK, Snow, MMC_RSP_R1, NULL, &MMCdata);

		if ((ii == 0)
		&&  (Size > 1)) {							/* Stop transmission when multi-block			*/
			ii = mmc_sendcmd(0, MMC_CMD_STOP_TRANSMISSION, 0, MMC_RSP_R1b, NULL, NULL);
		}

		if (ii == 0) {
			ii = mmc_sendstatus(0);					/* Waiting for the ready status					*/
		}

		Left -= Size;								/* This less to transfer the next time			*/
		Blk  += Size;								/* Next sector to start writing to				*/
		Buf   = (Size * Cfg->BlkLenRW)
		      + (uint8_t *)Buf;
	} while ((ii == 0)
	  &&     (Left > 0));

	return((ii!=0) ? -2 : 0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: mmc_init																				*/
/*																									*/
/* mmc_init - initialize to access a SD/MMC card													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int mmc_init(int Dev);																		*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : SD/MMC device number																	*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int mmc_init(int Dev)
{
MMCcfg_t *Cfg;										/* De-reference array for faster access			*/
uint32_t  CmdResp[4];								/* Response to a command sent to SD/MMC			*/
SDdma_t  *DMAdsc;									/* DMA descriptors for this device				*/
volatile  uint32_t *Reg;							/* Base address of the SD/MMC device			*/
int       ii;										/* General purpose								*/
int       ReMap;									/* Device remap index							*/
SDdma_t  *StartDMAdsc;								/* First DMA descriptor for this device			*/
uint32_t  Val;										/* Value to send with the command to SD/MMC		*/

  #if ((SDMMC_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];

  #else
	if ((Dev < 0)									/* Device # must be within range to be valid	*/
	||  (Dev >= SDMMC_MAX_DEVICES)) {
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by MY_UART_LIST_DEVICE			*/
		return(-2);
	}
  #endif

  #if ((SDMMC_USE_MUTEX) != 0)	
	ii = 0;											/* Safety in case BSS isn't zeroed				*/
	if (g_CfgIsInit == 0) {							/* Don't go through locking the mutex if has	*/
		if (0 != MTXlock(G_OSmutex, -1)) {			/* been initialized								*/
			return(-3);								/* Although infinte wait, if deadlock detection	*/
		}											/* is enable, could return !=0 and not lock		*/
		ii = 1;
	}
  #endif

	if (g_CfgIsInit == 0) {							/* Here, is not init, got exclisive access		*/
		memset(&g_MMCcfg[0], 0, sizeof(g_MMCcfg));	/* check again due to small critical region		*/
		g_CfgIsInit = 1;
	}

	Cfg = &g_MMCcfg[ReMap];							/* This is this device configuration			*/

  #if ((SDMMC_USE_MUTEX) != 0)	
	if (Cfg->MyMutex == (MTX_t *)NULL) {
		Cfg->MyMutex = MTXopen(&g_Names[Dev][0]);
	}
	if (ii != 0) {
		MTXunlock(G_OSmutex);
	}
  #endif

	if (0 != SDMMC_MTX_LOCK(Cfg)) {					/* Now we need to have exclusive access			*/
		return(-5);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	if (Cfg->IsInit == 0) {
		Cfg->MySema = SEMopen(&g_Names[Dev][0]);	/* Open the semaphore used by the ISRs		*/
		Cfg->HW      = g_HWaddr[Dev];
		Cfg->ClkReg  = g_CLKaddr[Dev];
		Cfg->SysReg  = g_SYSaddr[Dev];
		Cfg->DMAdesc = &g_DMAdesc[ReMap][0];
	  #if ((SDMMC_DEBUG) > 0)
		Cfg->Dev     = Dev;
	  #endif
		Cfg->IsInit  = 1;
	}

	Reg = Cfg->HW;									/* Base address of the SD/MMC registers			*/

	Cfg->RCA     = 0;								/* To remove compiler warning					*/
	Cfg->CardVer = 0;								/* To remove compiler warning					*/
	Cfg->OCR     = 0;

	StartDMAdsc  = &g_DMAdesc[ReMap][0];			/* Create linked list of DMA descriptors		*/
	DMAdsc       = StartDMAdsc;
	for (ii=0; ii<((sizeof(g_DMAdesc[0])/sizeof(g_DMAdesc[0][0]))-1) ; ii++) {
		DMAdsc->next = ((uint32_t)(DMAdsc))
		             + sizeof(*DMAdsc);
		DMAdsc++;
	}
	DMAdsc->next = (uint32_t)StartDMAdsc;			/* Link back to beginning						*/

	Reg[SD_PWREN_REG] = 0x1;

	if (0 != MMCwaitReset(Cfg, SD_RESET_ALL)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Unable to reset (3)\n", Dev);
	  #endif
		SDMMC_MTX_UNLOCK(Cfg);
		return(-6);
	}

	Reg[SD_RINTSTS_REG] = SD_INTMSK_ALL;
	Reg[SD_INTMASK_REG] = SD_INTMSK_CD
	                    | SD_INTMSK_CDONE
	                    | SD_DATA_ERR
	                    | SD_DATA_TOUT
	                    | SD_INTMSK_RE
	                    | SD_INTMSK_RTO
	                    | SD_INTMSK_DT
	                    | SD_INTMSK_CD;
	Reg[SD_TMOUT_REG]   = 0xFFFFFFFF;
	Reg[SD_IDINTEN_REG] = 0;
	Reg[SD_BMOD_REG]    = 1;
	Reg[SD_FIFOTH_REG]  = MSIZE(0x2)
	                    | RX_WMARK(SD_FIFO_DEPTH / 2 - 1)
	                    | TX_WMARK(SD_FIFO_DEPTH / 2);
	Reg[SD_CLKENA_REG]  = 0;
	Reg[SD_CLKSRC_REG]  = 0;
	Reg[SD_CTYPE_REG]   = SD_CTYPE_1BIT; 		/* Set bus width to 1 bit and bus speed to		*/
	Reg[SD_CLKENA_REG]  = 0;						/* 400KHz for discovery							*/
	Reg[SD_CLKSRC_REG]  = 0;
	Val                 = (2 * 400000);
	Reg[SD_CLKDIV_REG]  = (((SDMMC_CLK) + 400000)/* +400000 is for rounding						*/
	                    / Val);
	Reg[SD_CMD_REG]     = SD_CMD_PRV_DAT_WAIT
	                    | SD_CMD_UPD_CLK
	                    | SD_CMD_START;
	Val                 = Cfg->ClkReg[CLKMGR_PERPLLGRP_EN_REG]; 		/* Disable the SDMMC clocks	*/
	Val                &= ~CLKMGR_SDMMC_CLK_ENABLE;
	Cfg->ClkReg[CLKMGR_PERPLLGRP_EN_REG]  = Val;
	Cfg->SysReg[SYSMGR_SDMMCGRP_CTRL_REG] = SYSMGR_SDMMC_CTRL_SET(Dev, 3);	/*sample=0 drive=3		*/
	Val                 = Cfg->ClkReg[CLKMGR_PERPLLGRP_EN_REG];			/* Enable the SDMMC clocks	*/
	Val                |= CLKMGR_SDMMC_CLK_ENABLE;
	Cfg->ClkReg[CLKMGR_PERPLLGRP_EN_REG] = Val;

	ii = OS_MS_TO_MIN_TICK(1000, 2);				/* Try for 1 sec, checking every ms				*/
	while(((Reg[SD_CMD_REG] & SD_CMD_START) != 0)
	&&    (ii >= 0)) {
		TSKsleep(OS_MS_TO_MIN_TICK(1,1));
		ii -= OS_MS_TO_MIN_TICK(1,1);
	}

	if (ii < 0) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - SD/MMC did not respond (3)\n", Dev);
	  #endif
		SDMMC_MTX_UNLOCK(Cfg);
		return(-7);
	}

	Reg[SD_CLKENA_REG] = SD_CLKEN_ENABLE
	                   | SD_CLKEN_LOW_PWR;
	Reg[SD_CMD_REG]    = SD_CMD_PRV_DAT_WAIT
	                   | SD_CMD_UPD_CLK
	                   | SD_CMD_START;

	ii = OS_MS_TO_MIN_TICK(1000, 5);
	while(((Reg[SD_CMD_REG] & SD_CMD_START) != 0)
	&&    (ii >= 0)) {								/* Start the SD/MMC and wait for it to clear	*/
		TSKsleep(OS_MS_TO_MIN_TICK(10, 1));			/* the request									*/
		ii -= OS_MS_TO_MIN_TICK(10, 1);				/* Wait for a maximum of 1 second, checking		*/
	}												/* every 10 ms									*/

	if (ii < 0) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - SD/MMC did not respond (5)\n", Dev);
	  #endif
		SDMMC_MTX_UNLOCK(Cfg);
		return(-8);
	}

	TSKsleep(OS_MS_TO_MIN_TICK(2,2));				/* Test for SD version 2. First Go Idle			*/
	ii = mmc_sendcmd(Dev, MMC_CMD_GO_IDLE_STATE, 0, MMC_RSP_NONE, NULL, NULL);
	if (ii != 0) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - SD/MMC does not reach idle\n", Dev);
	  #endif
		SDMMC_MTX_UNLOCK(Cfg);
		return(-9);
	}

	TSKsleep(OS_MS_TO_MIN_TICK(2,2));				/* Test for SD version 2, Set the bit if  Host	*/
													/* supports voltages between 2.7V and 3.6V		*/
	Val = (((SUPPORTED_VOLTAGES & 0x00FF8000) != 0) << 8) | 0xAA;
	if (0 != mmc_sendcmd(Dev, MMC_CMD_SEND_EXT_CSD, Val, MMC_RSP_R7, &CmdResp[0], NULL)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - SD/MMC did not respond (6)\n", Dev);
	  #endif
		SDMMC_MTX_UNLOCK(Cfg);
		return(-10);
	}

	if ((CmdResp[0] & 0xFF) == 0xAA) {
		Cfg->CardVer = SD_VERSION_2;
	}

	ii = OS_MS_TO_MIN_TICK(1000, 5);
 	do {
		TSKsleep(OS_MS_TO_MIN_TICK(10, 1));
		if (0 != mmc_sendcmd(Dev, MMC_CMD_APP_CMD, 0, MMC_RSP_R1, &CmdResp[0], NULL)) {
		  #if ((SDMMC_DEBUG) > 0)
			printf("SDMMC [Dev:%d] - Error - Command prefix send error (2)\n", Dev);
		  #endif
			SDMMC_MTX_UNLOCK(Cfg);
			return(-11);
		}

		TSKsleep(OS_MS_TO_MIN_TICK(10, 1));
		Val  = (SUPPORTED_VOLTAGES & 0x00FF8000)
		     | ((Cfg->CardVer == SD_VERSION_2)
		        ? OCR_HCS
		        : 0);
		if (0 != mmc_sendcmd(Dev, MMC_CMD_APP_SEND_OP_COND, Val, MMC_RSP_R3, &CmdResp[0], NULL)) {
		  #if ((SDMMC_DEBUG) > 0)
			printf("SDMMC [Dev:%d] - Error - OCR busy timeout\n", Dev);
		  #endif
			SDMMC_MTX_UNLOCK(Cfg);
			return(-12);
		}

	} while (((CmdResp[0] & OCR_BUSY) == 0)
	  &&      ii--);

	if (ii <= 0) {									/* The card cannot be used						*/
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - the card cannot be used\n", Dev);
	  #endif
		SDMMC_MTX_UNLOCK(Cfg);
		return(-13);
	}

	if (Cfg->CardVer != SD_VERSION_2) {
		Cfg->CardVer = SD_VERSION_1_0;
	}

	Cfg->OCR = CmdResp[0];

	ii = MMCstartup(Dev);

	if (ii != 0) {
		Cfg->IsInit = 0;
	}

	SDMMC_MTX_UNLOCK(Cfg);

	return(ii);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: mmc_present																			*/
/*																									*/
/* mmc_present - report if a card is inserted														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int mmc_present(int Dev);																	*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : SD/MMC device number																	*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : there are no cards																	*/
/*		>  0 : a card is present																	*/
/*		<  0 : "Dev" is invalid or not initialized													*/
/*																									*/
/* NOTE:																							*/
/*      card detect does not work with mini SD & micro SD (pin not available)						*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int mmc_present(int Dev)
{
MMCcfg_t *Cfg;
int       ReMap;

  #if ((SDMMC_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_MMCcfg[ReMap];

  #else
	if ((Dev < 0)									/* Device # must be within range to be valid	*/
	||  (Dev >= SDMMC_MAX_DEVICES)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Invalid device number\n", Dev);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by MY_UART_LIST_DEVICE			*/
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - cannot remap device\n", Dev);
	  #endif
		return(-2);
	}

	Cfg = &g_MMCcfg[ReMap];
	if ((g_CfgIsInit == 0)							/* Make sure SDMMC device has been initialized	*/
	||  (Cfg->IsInit == 0)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Uninitialized device\n", Dev);
	  #endif
		return(-3);
	}
  #endif

	return(!(Cfg->HW[SD_CDETECT_REG] & 1));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: mmc_nowrt																				*/
/*																									*/
/* mmc_nowrt - report if an inserted card is write-protected										*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int mmc_nowrt(int Dev);																		*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : SD/MMC device number																	*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : the card is read / write																*/
/*		>  0 : the card is write-only																*/
/*		<  0 : "Dev" is invalid or not initialized													*/
/*																									*/
/* NOTE:																							*/
/*      mini SD & micro SD always report NOT write protected (return value == 0)					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int mmc_nowrt(int Dev)
{
MMCcfg_t *Cfg;
int       ReMap;

  #if ((SDMMC_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_MMCcfg[ReMap];

  #else
	if ((Dev < 0)									/* Device # must be within range to be valid	*/
	||  (Dev >= SDMMC_MAX_DEVICES)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Invalid device number\n", Dev);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by MY_UART_LIST_DEVICE			*/
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - cannot remap device\n", Dev);
	  #endif
		return(-2);
	}

	Cfg = &g_MMCcfg[ReMap];
	if ((g_CfgIsInit == 0)							/* Make sure SDMMC device has been initialized	*/
	||  (Cfg->IsInit == 0)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Uninitialized device\n", Dev);
	  #endif
		return(-3);
	}
  #endif

	return(Cfg->HW[SD_WRTPRT_REG] & 1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: mmc_blklen																				*/
/*																									*/
/* mmc_blklen - report the card block length														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int mmc_blklen(int Dev, int Len);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev : SD/MMC device number to change the block size											*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>  0 : success																				*/
/*      <= 0 : error																				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int mmc_blklen(int Dev)
{
MMCcfg_t *Cfg;
int       ReMap;									/* Device remap index							*/

  #if ((SDMMC_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_MMCcfg[ReMap];

  #else
	if ((Dev < 0)									/* Device # must be within range to be valid	*/
	||  (Dev >= SDMMC_MAX_DEVICES)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Invalid device number\n", Dev);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by MY_UART_LIST_DEVICE			*/
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - cannot remap device\n", Dev);
	  #endif
		return(-2);
	}

	Cfg = &g_MMCcfg[ReMap];
	if ((g_CfgIsInit == 0)							/* Make sure SDMMC device has been initialized	*/
	||  (Cfg->IsInit == 0)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Uninitialized device\n", Dev);
	  #endif
		return(-3);
	}
  #endif

	return(Cfg->BlkLen);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: mmc_capacity																			*/
/*																									*/
/* mmc_capacity - report the SD/MMC card capacity													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int64_t mmc_capacity(int Dev);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev : SD/MMC device number to change the block size											*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>  0 : capacity in bytes																	*/
/*      <= 0 : error																				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int64_t mmc_capacity(int Dev)
{
MMCcfg_t *Cfg;
int ReMap;											/* Device remap index							*/

  #if ((SDMMC_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_MMCcfg[ReMap];
  #else
	if ((Dev < 0)									/* Device # must be within range to be valid	*/
	||  (Dev >= SDMMC_MAX_DEVICES)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Invalid device number\n", Dev);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];						/* The device must have a valid remapping		*/
	if (ReMap < 0) {								/* This is set by MY_UART_LIST_DEVICE			*/
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - cannot remap device\n", Dev);
	  #endif
		return(-2);
	}

	Cfg = &g_MMCcfg[ReMap];
	if ((g_CfgIsInit == 0)							/* Make sure SDMMC device has been initialized	*/
	||  (Cfg->IsInit == 0)) {
	  #if ((SDMMC_DEBUG) > 0)
		printf("SDMMC [Dev:%d] - Error - Uninitialized device\n", Dev);
	  #endif
		return(-3);
	}
  #endif

	return(Cfg->Capacity);
}

/* ------------------------------------------------------------------------------------------------ */
/* Interrupt handler for the SD/MMC transfers														*/
/* NOTE:																							*/
/*																									*/
/* The following code was tested to try to optimize the overall response by posting the semaphore	*/
/* only once during a data transfer. The read throughput was minimally improved (~0.1%) and the		*/
/* write throughput got a minimal degradation (~0.25%). As the write throughput is always lower		*/
/* than the read throughput, this code was not used.												*/
/*																									*/
/*	if ((g_XferData == 0)							/ * When doing a data transfer, save time by	*/
/*	||  ((Interim & SD_INTMSK_DT) != 0)) {			/ * posting the semaphore only when all done	*/
/*		SEMpost(Cfg->MySema);						/ * Post the SDMMC semaphore					*/
/*	}																								*/
/*																									*/
/* The looping over SD_INT_STAT_REG is done to eliminate possible spurious interrupts				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void MMCintHndl(int Dev)
{
MMCcfg_t *Cfg;
volatile uint32_t *Reg;
uint32_t Interim;
int      ReMap;

	ReMap   = g_DevReMap[Dev];
	Reg     = g_HWaddr[ReMap];
	Cfg     = &g_MMCcfg[ReMap];
	Interim = (uint32_t)0;

  #if (((SDMMC_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1))	/* If multiple cores handle the ISR		*/
   #if ((OX_NESTED_INTS) != 0)						/* Get exclusive access first					*/
	ReMap = OSintOff();								/* and then set the OnGoing flag to inform the	*/
   #endif											/* other cores to skip as I'm handlking the ISR	*/
	CORElock(MMC_SPINLOCK, &Cfg->SpinLock, 0, COREgetID()+1);
  #endif

	do {
		Interim |= Reg[SD_RINTSTS_REG];
		if (Interim & SD_INTMSK_CD) {				/* Card removed, can't use it anymore			*/
			Reg[SD_INTMASK_REG] = 0;				/* Disable all interrupts						*/
			Cfg->IsInit         = 0;				/* Declare the card as un-initialized			*/
		}
		Reg[SD_RINTSTS_REG] = Interim;
	} while (0U != (Interim & Reg[SD_MINTSTS_REG]));

	Cfg->RawInter |= Interim;

	if (Cfg->MySema != NULL) {
		SEMpost(Cfg->MySema);
	}

  #if (((SDMMC_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1))	/* If multiple cores handle the ISR		*/
	COREunlock(MMC_SPINLOCK, &Cfg->SpinLock, 0);
   #if ((OX_NESTED_INTS) != 0)
	OSintBack(ReMap);
   #endif
  #endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Individual interrupt handlers macro. It calls the common handler with the device #				*/

#define MMC_INT_HNDL(Dev)																			\
																									\
	void MMCintHndl_##Dev(void)																		\
	{																								\
		MMCintHndl(Dev);																			\
		return;																						\
	}

/* ------------------------------------------------------------------------------------------------ */

#if (((SDMMC_LIST_DEVICE) & 0x001) != 0U)
  MMC_INT_HNDL(0)
#endif
#if (((SDMMC_LIST_DEVICE) & 0x002) != 0U)
  MMC_INT_HNDL(1)
#endif
#if (((SDMMC_LIST_DEVICE) & 0x004) != 0U)
  MMC_INT_HNDL(2)
#endif
#if (((SDMMC_LIST_DEVICE) & 0x008) != 0U)
  MMC_INT_HNDL(3)
#endif
#if (((SDMMC_LIST_DEVICE) & 0x010) != 0U)
  MMC_INT_HNDL(4)
#endif
#if (((SDMMC_LIST_DEVICE) & 0x020) != 0U)
  MMC_INT_HNDL(5)
#endif
#if (((SDMMC_LIST_DEVICE) & 0x040) != 0U)
  MMC_INT_HNDL(6)
#endif
#if (((SDMMC_LIST_DEVICE) & 0x080) != 0U)
  MMC_INT_HNDL(7)
#endif
#if (((SDMMC_LIST_DEVICE) & 0x100) != 0U)
  MMC_INT_HNDL(8)
#endif
#if (((SDMMC_LIST_DEVICE) & 0x200) != 0U)
  MMC_INT_HNDL(9)
#endif

/* EOF */
