/* ------------------------------------------------------------------------------------------------ */
/* FILE :		dw_sdmmc.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the DesignWare SD/MMC.													*/
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
/*	$Revision: 1.12 $																				*/
/*	$Date: 2019/01/10 18:07:03 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* See dw_sdmmc.c for NOTE / LIMITATIONS / NOT YET SUPPORTED										*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Supported value for the build options															*/
/* (OS_PLATFORM & 0x00FFFFFF):																		*/
/*					0xAA10 : Arria 10 																*/
/*					0xAAC5 : Cyclone V & Arria V													*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Default values that depend on (OS_PLATFORM & 0x00FFFFFF):										*/
/* SDMMC_MAX_DEVICES:																				*/
/*					0xAA10 : 1																		*/
/*					0xAAC5 : 1																		*/
/* SDMMX_LIST_DEVICE:																				*/
/*					0xAA10 : 0x01																	*/
/*					0xAAC5 : 0x01																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __DW_SDMMC_H
#define __DW_SDMMC_H

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------ */

#ifndef SDMMC_NUM_DMA_DESC
  #define SDMMC_NUM_DMA_DESC	(16)				/* The maximum transfer size of the driver is:	*/
#endif												/*           SDMMC_NUM_DMA_DESC * 512			*/
													/* If exceeded, an error is reported			*/
#define SDMMC_BUFFER_UNCACHED	(0)					/* When using buffers in non-cached memory		*/
#define SDMMC_BUFFER_CACHED		(1)					/* When using buffers in cached memory			*/

#ifndef SDMMC_BUFFER_TYPE							/* By default, use buffers in non-cached memory	*/
  #define SDMMC_BUFFER_TYPE		SDMMC_BUFFER_UNCACHED
#endif

/* ------------------------------------------------------------------------------------------------ */

#if defined(SDMMC_MAX_DEVICES) && defined(SDMMC_LIST_DEVICE)
  #error "Define one of the two: SDMMC_MAX_DEVICES or SDMMC_LIST_DEVICE"
  #error "Do not define both as there could be conflicting info"
#endif

#if !defined(SDMMC_MAX_DEVICES)
  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0xAAC5)
	#define SDMMC_MAX_DEVICES	1
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0xAA10)
	#define SDMMC_MAX_DEVICES	1
  #else
	#define SDMMC_MAX_DEVICES	1
  #endif
#endif

#ifndef SDMMC_LIST_DEVICE
  #define SDMMC_LIST_DEVICE		((1U<<(SDMMC_MAX_DEVICES))-1U)
#endif

#if ((SDMMC_MAX_DEVICES) <= 0)
  #error "SDMMC_MAX_DEVICES must be 1 or greater"
#endif

#if (((1U<<(SDMMC_MAX_DEVICES))-1U) < (SDMMC_LIST_DEVICE))
  #error "too many devices in SDMMC_LIST_DEVICE vs the max # device defined by SDMMC_MAX_DEVICES"
#endif

/* ------------------------------------------------------------------------------------------------ */

#define MMC_DATA_READ					1
#define MMC_DATA_WRITE					2

#define MMC_CMD_GO_IDLE_STATE			0
#define MMC_CMD_SEND_OP_COND			1
#define MMC_CMD_ALL_SEND_CID			2
#define MMC_CMD_SET_RELATIVE_ADDR		3
#define MMC_CMD_SET_DSR					4
#define MMC_CMD_SWITCH					6
#define MMC_CMD_SELECT_CARD				7
#define MMC_CMD_SEND_EXT_CSD			8
#define MMC_CMD_SEND_CSD				9
#define MMC_CMD_SEND_CID				10
#define MMC_CMD_STOP_TRANSMISSION		12
#define MMC_CMD_SEND_STATUS				13
#define MMC_CMD_SET_BLOCKLEN			16
#define MMC_CMD_READ_SINGLE_BLOCK		17
#define MMC_CMD_READ_MULTIPLE_BLOCK		18
#define MMC_CMD_WRITE_SINGLE_BLOCK		24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK	25
#define MMC_CMD_ERASE_WR_BLK_START		32
#define MMC_CMD_ERASE_WR_BLK_END		33
#define MMC_CMD_ERASE_GROUP_START		35
#define MMC_CMD_ERASE_GROUP_END			36
#define MMC_CMD_ERASE					38
#define MMC_CMD_APP_SEND_OP_COND		41
#define MMC_CMD_APP_SEND_SCR			51
#define MMC_CMD_APP_CMD					55
#define MMC_CMD_SPI_READ_OCR			58
#define MMC_CMD_SPI_CRC_ON_OFF			59

#define SD_CMD_RSP_NONE					(0 << 0)
#define SD_CMD_RSP_PRESENT 				(1 << 0)
#define SD_CMD_RSP_136					(1 << 1)	/* 136 bit response								*/
#define SD_CMD_RSP_CRC					(1 << 2)	/* Expect a valid CRC							*/
#define SD_CMD_RSP_BUSY					(1 << 3)	/* Card may send busy							*/
#define SD_CMD_RSP_OPCODE				(1 << 4)	/* Response contains op-code					*/
#define MMC_RSP_NONE					(SD_CMD_RSP_NONE)
#define MMC_RSP_R1						(SD_CMD_RSP_PRESENT|SD_CMD_RSP_CRC|SD_CMD_RSP_OPCODE)
#define MMC_RSP_R1b						(SD_CMD_RSP_PRESENT|SD_CMD_RSP_CRC|SD_CMD_RSP_OPCODE		\
										|SD_CMD_RSP_BUSY)
#define MMC_RSP_R2						(SD_CMD_RSP_PRESENT|SD_CMD_RSP_136|SD_CMD_RSP_CRC)
#define MMC_RSP_R3						(SD_CMD_RSP_PRESENT)
#define MMC_RSP_R4						(SD_CMD_RSP_PRESENT)
#define MMC_RSP_R5						(SD_CMD_RSP_PRESENT|SD_CMD_RSP_CRC|SD_CMD_RSP_OPCODE)
#define MMC_RSP_R6						(SD_CMD_RSP_PRESENT|SD_CMD_RSP_CRC|SD_CMD_RSP_OPCODE)
#define MMC_RSP_R7						(SD_CMD_RSP_PRESENT|SD_CMD_RSP_CRC|SD_CMD_RSP_OPCODE)

#define OCR_BUSY						(0x80000000)
#define OCR_HCS							(0x40000000)
#define OCR_VOLTAGE_MASK				(0x007FFF80)
#define OCR_ACCESS_MODE					(0x60000000)

/* ------------------------------------------------------------------------------------------------ */

typedef struct {
	char        *Buffer;							/* Data buffer for the exchange with SD/MMC		*/
	unsigned int Flags;								/* Xfer type: MMC_DATA_READ or MMC_DATA_WRITE	*/
	unsigned int Nbytes;							/* Number of bytes in the data buffer			*/
} MMCdata_t;

/* ------------------------------------------------------------------------------------------------ */

int     mmc_blklen    (int Dev);
int64_t mmc_capacity  (int Dev);
int     mmc_init      (int Dev);
int     mmc_nowrt     (int Dev);
int     mmc_present   (int Dev);
int     mmc_read      (int Dev, uint32_t Blk, void *Buf, uint32_t Nblk);
int     mmc_sendcmd   (int Dev, unsigned int Cmd, uint32_t Arg, uint32_t Exp, uint32_t *Resp,
                       MMCdata_t *Data);
int     mmc_sendstatus(int Dev);
int     mmc_write     (int Dev, uint32_t Blk, const void *Buf, uint32_t Nblk);

/* ------------------------------------------------------------------------------------------------ */

#if (((SDMMC_LIST_DEVICE) & 0x001) != 0)
  extern void MMCintHndl_0(void);
#endif
#if (((SDMMC_LIST_DEVICE) & 0x002) != 0)
  extern void MMCintHndl_1(void);
#endif
#if (((SDMMC_LIST_DEVICE) & 0x004) != 0)
  extern void MMCintHndl_2(void);
#endif
#if (((SDMMC_LIST_DEVICE) & 0x008) != 0)
  extern void MMCintHndl_3(void);
#endif
#if (((SDMMC_LIST_DEVICE) & 0x010) != 0)
  extern void MMCintHndl_4(void);
#endif
#if (((SDMMC_LIST_DEVICE) & 0x020) != 0)
  extern void MMCintHndl_5(void);
#endif
#if (((SDMMC_LIST_DEVICE) & 0x040) != 0)
  extern void MMCintHndl_6(void);
#endif
#if (((SDMMC_LIST_DEVICE) & 0x080) != 0)
  extern void MMCintHndl_7(void);
#endif
#if (((SDMMC_LIST_DEVICE) & 0x100) != 0)
  extern void MMCintHndl_8(void);
#endif
#if (((SDMMC_LIST_DEVICE) & 0x200) != 0)
  extern void MMCintHndl_9(void);
#endif

/* ------------------------------------------------------------------------------------------------ */

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
