/* ------------------------------------------------------------------------------------------------ */
/* FILE :		cd_qspi.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the Cadence QSPI.														*/
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
/*	$Date: 2019/01/10 18:07:03 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* See cd_qspi.c for NOTE / LIMITATIONS / NOT YET SUPPORTED											*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Supported value for the build options															*/
/* (OS_PLATFORM & 0x00FFFFFF):																		*/
/*					0xAA10 : Arria 10 																*/
/*					0xAAC5 : Cyclone V & Arria V													*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Default values that depend on (OS_PLATFORM & 0x00FFFFFF):										*/
/* QSPI_MAX_DEVICES:																				*/
/*					0xAA10 : 1																		*/
/*					0xAAC5 : 1																		*/
/* QSPI_LIST_DEVICE:																				*/
/*					0xAA10 : 0x01																	*/
/*					0xAAC5 : 0x01																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __CD_QSPI_H
#define __CD_QSPI_H

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------ */

#if defined(QSPI_MAX_DEVICES) && defined(QSPI_LIST_DEVICE)
  #error "Define one of the two: QSPI_MAX_DEVICES or QSPI_LIST_DEVICE"
  #error "Do not define both as there could be conflicting info"
#endif

#if !defined(QSPI_MAX_DEVICES)
  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0xAAC5)
	#define QSPI_MAX_DEVICES	1
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0xAA10)
	#define QSPI_MAX_DEVICES	1
  #else
	#define QSPI_MAX_DEVICES	1
  #endif
#endif

#ifndef QSPI_LIST_DEVICE
  #define QSPI_LIST_DEVICE		((1U<<(QSPI_MAX_DEVICES))-1U)
#endif

#if ((QSPI_MAX_DEVICES) <= 0)
  #error "QSPI_MAX_DEVICES must be 1 or greater"
#endif

#if (((1U<<(QSPI_MAX_DEVICES))-1U) < (QSPI_LIST_DEVICE))
  #error "too many devices in QSPI_LIST_DEVICE vs the max # device defined by QSPI_MAX_DEVICES"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Mode definitions used by qspi_init()																*/
/*																									*/
/* Bit #0    : Type of chip select																	*/
/*             0 - 1 to N																			*/
/*             1 - N to 2^N (external decoder)														*/
/* Bits #28+ : Cache maintenance configuration for the DMA											*/
/*             QSPI_DMA_CACHE_RX(0,0) : Cache maintenance is not used								*/
/*             QSPI_DMA_CACHE_RX(1,n) : ACP is used with page #n mapped to 0x80000000->0xBFFFFFFF	*/
/*																									*/
/* DO NOT CHANGE THESE DEFINES AS THE CODE SOMETIME RELIES ON SPECIFIC VALUES & BIT POSITIONS		*/
/* ALWAYS USE THE DEFINES AND NOT THE BITS IN YOUR APPLICATION FOR PORTABILITY AND FUTURE-PROOFING	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#define QSPI_CFG_CS_N				(0U<<0)
#define QSPI_CFG_CS_NN				(1U<<0)

#define QSPI_CFG_CACHE_RX(Acp,Page)	(((Page)<<30)|(((Acp)!=0)<<29)|(1U<<28))

/* ------------------------------------------------------------------------------------------------ */
/* Function prototypes																				*/

int32_t qspi_blksize  (int Dev, int Slv);
int     qspi_cmd_read (int Dev, int Slv, int Cmd, uint8_t *Buf, int Nbytes);
int     qspi_cmd_write(int Dev, int Slv, int Cmd, uint8_t *Buf, int Nbytes);
int     qspi_erase    (int Dev, int Slv, uint32_t Addr, uint32_t Len);
int     qspi_init     (int Dev, int Slv, uint32_t Mode);
int     qspi_read     (int Dev, int Slv, uint32_t Addr, void *Buf, uint32_t Len);
int64_t qspi_size     (int Dev, int Slv);
int     qspi_write    (int Dev, int Slv, uint32_t Addr, const void *Buf, uint32_t Len);

/* ------------------------------------------------------------------------------------------------ */

#if (((QSPI_LIST_DEVICE) & 0x001) != 0)
  extern void QSPIintHndl_0(void);
#endif
#if (((QSPI_LIST_DEVICE) & 0x002) != 0)
  extern void QSPIintHndl_1(void);
#endif
#if (((QSPI_LIST_DEVICE) & 0x004) != 0)
  extern void QSPIintHndl_2(void);
#endif
#if (((QSPI_LIST_DEVICE) & 0x008) != 0)
  extern void QSPIintHndl_3(void);
#endif
#if (((QSPI_LIST_DEVICE) & 0x010) != 0)
  extern void QSPIintHndl_4(void);
#endif
#if (((QSPI_LIST_DEVICE) & 0x020) != 0)
  extern void QSPIintHndl_5(void);
#endif
#if (((QSPI_LIST_DEVICE) & 0x040) != 0)
  extern void QSPIintHndl_6(void);
#endif
#if (((QSPI_LIST_DEVICE) & 0x080) != 0)
  extern void QSPIintHndl_7(void);
#endif
#if (((QSPI_LIST_DEVICE) & 0x100) != 0)
  extern void QSPIintHndl_8(void);
#endif
#if (((QSPI_LIST_DEVICE) & 0x200) != 0)
  extern void QSPIintHndl_9(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
