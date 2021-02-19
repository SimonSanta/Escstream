/* ------------------------------------------------------------------------------------------------ */
/* File :		dw_spi.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the Designware SPI.														*/
/*				IMPORTANT: The interrupt controller MUST be set to level triggering when using		*/
/*				           the interrupts															*/
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
/*	$Revision: 1.12 $																				*/
/*	$Date: 2019/01/10 18:07:03 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* See dw_spi.c for NOTE / LIMITATIONS / NOT YET SUPPORTED											*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Supported value for the build options															*/
/* (OS_PLATFORM & 0x00FFFFFF):																		*/
/*					0xAA10 : Arria 10 																*/
/*					0xAAC5 : Cyclone V & Arria V													*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Default values that depend on (OS_PLATFORM & 0x00FFFFFF):										*/
/* I2C_MAX_DEVICES:																					*/
/*					0xAA10 : 4																		*/
/*					0xAAC5 : 4																		*/
/* I2C_LIST_DEVICE:																					*/
/*					0xAA10 : 0x0F																	*/
/*					0xAAC5 : 0x0F																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __DW_SPI_H
#define __DW_SPI_H

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef SPI_MULTI_DRIVER							/* To select if multiple SPI drivers are used	*/
  #define SPI_MULTI_DRIVER		0					/* in an application. Examples are SocFPGA with */
#endif												/* SPI added in the FPGA in extra to processor	*/

#if ((SPI_MULTI_DRIVER) == 0)						/* When not mutliple driver, don't prefix the	*/
  #define DW_SPI_PREFIX								/* API names									*/
  #define DW_SPI_ASCII			""
#else
 #ifndef DW_SPI_PREFIX								/* When multiple drivers, use a prefix			*/
  #define DW_SPI_PREFIX			 dw_				/* Default prefix, always the same as the file	*/
  #define DW_SPI_ASCII			"dw "				/* name prefix to spi.c							*/
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */

#if ((SPI_MULTI_DRIVER) == 0)
 #if !defined(SPI_MAX_DEVICES)
  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0xAAC5)
	#define SPI_MAX_DEVICES	4					/* Arria V / Cyclone V has a total of 2 SPIs	*/
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0xAA10)
	#define SPI_MAX_DEVICES	4					/* Arria 10 has a total of 2 SPIs				*/
  #else
	#define SPI_MAX_DEVICES	1					/* Undefined platform							*/
  #endif
 #endif

 #ifndef SPI_LIST_DEVICE
  #define SPI_LIST_DEVICE		((1U<<(SPI_MAX_DEVICES))-1U)
 #endif

 #if (((1U<<(SPI_MAX_DEVICES))-1U) < (SPI_LIST_DEVICE))
  #error "too many devices in SPI_LIST_DEVICE vs the max # device defined by SPI_MAX_DEVICES"
 #endif

 #undef  DW_SPI_MAX_DEVICES
 #define DW_SPI_MAX_DEVICES (SPI_MAX_DEVICES)

 #undef  DW_SPI_LIST_DEVICE
 #define DW_SPI_LIST_DEVICE (SPI_LIST_DEVICE)

#else
 #if (!defined(DW_SPI_LIST_DEVICE) && !defined(DW_SPI_MAX_DEVICES))
	#if defined(SPI_LIST_DEVICE)
	  #define DW_SPI_LIST_DEVICE	(SPI_LIST_DEVICE)
	#endif
	#if defined(SPI_MAX_DEVICES)
	  #define DW_SPI_MAX_DEVICES	(SPI_MAX_DEVICES)
	#endif
 #endif

 #if !defined(DW_SPI_MAX_DEVICES)
  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0xAAC5)
	#define DW_SPI_MAX_DEVICES	4					/* Arria V / Cyclone V has a total of 2 SPIs	*/
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0xAA10)
	#define DW_SPI_MAX_DEVICES	4					/* Arria 10 has a total of 2 SPIs				*/
  #else
	#define DW_SPI_MAX_DEVICES	1					/* Undefined platform							*/
  #endif
 #endif

 #ifndef DW_SPI_LIST_DEVICE
  #define DW_SPI_LIST_DEVICE		((1U<<(DW_SPI_MAX_DEVICES))-1U)
 #endif

 #if (((1U<<(DW_SPI_MAX_DEVICES))-1U) < (DW_SPI_LIST_DEVICE))
  #error "too many devices in DW_SPI_LIST_DEVICE vs the max # device defined by DW_SPI_MAX_DEVICES"
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Mode definitions used by spi_init()																*/
/*																									*/
/* Bit #1-0  : Protocol																				*/
/*             0 - Motorola SPI																		*/
/*             1 - TI SSP																			*/
/*             2 - National Semiconductor MicroWire													*/
/* Bit #2    : CPHA																					*/
/*             0 - CPHA0 : Data captured on the inactive->active clock transition					*/
/*           : 1 - CPH1 : Data captured on the active->inactive clock transition					*/
/* Bit #3    : CPOL																					*/
/*             0 - CPOL0 : clock is inactive on low value (on a 0)									*/
/*             1 - CPOL1 : clock is inactive on a high value (on a 1)								*/
/* Bit #4    : Frame alignment																		*/
/*             0 - Frames are right-aligned in the buffer data										*/
/*             1 - Frames are left-aligned in the buffer data										*/
/* Bit #5    : Data packing																			*/
/*             0 - 1 frame per byte/16 bit word														*/
/*             1 - packed frames (continuous bit stream)											*/
/* Bit #6    : TX-RX protocol 																		*/
/*             0 - TX is going out at the same time as RX is captured								*/
/*             1 - EEPROM protocol: data is first TXed then RX is captured							*/
/* Bit #7    : Controller is a master or a slave													*/
/*             0 - Master																			*/
/*             1 - Slave																			*/
/* Bit #8    : Transfer mode																		*/
/*             0 - Transfers are done through polling												*/
/*             1 - Transfers are done in interrupts													*/
/*             2 - Transfers are done through DMA													*/
/* Bit #10   : End of transmission																	*/
/*             0 - EOT is detected by polling														*/
/*             1 - EOT is detected through an interrupt												*/
/* Bit #11   : MicroWire handshake																	*/
/*             0 - MicroWire does not use handshake													*/
/*             1 - MicroWire uses handshake															*/
/* Bit #12   : Type of chip select																	*/
/*             0 - 1 to N																			*/
/*             1 - N to 2^N (external decoder)														*/
/* Bits #27+ : Cache maintenance configuration for the DMA											*/
/*             SPI_CFG_CACHE_nX(0,0) : Cache maintenance is not used								*/
/*             SPI_CFG_CACHE_nX(1,n) : ACP is used with page #n mapped to 0x80000000->0xBFFFFFFF	*/
/*																									*/
/* DO NOT CHANGE THESE DEFINES AS THE CODE SOMETIME RELIES ON SPECIFIC VALUES & BIT POSITIONS		*/
/* ALWAYS USE THE DEFINES AND NOT THE BITS IN YOUR APPLICATION FOR PORTABILITY AND FUTURE-PROOFING	*/
/* IT'S EVEN MORE IMPORTANT WHEN USING THE DRIVER IN A MULTI_DRIVER SET-UP							*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#define SPI_PROTO_SPI				(0U<<0)
#define SPI_PROTO_SSP				(1U<<0)
#define SPI_PROTO_UWIRE				(2U<<0)

#define SPI_CLK_CPHA0				(0U<<2)
#define SPI_CLK_CPHA1				(1U<<2)

#define SPI_CLK_CPOL0				(0U<<3)
#define SPI_CLK_CPOL1				(1U<<3)

#define SPI_ALIGN_RIGHT				(0U<<4)
#define SPI_ALIGN_LEFT				(1U<<4)

#define SPI_DATA_NONPACK			(0U<<5)
#define SPI_DATA_PACK				(1U<<5)

#define SPI_TX_RX					(0U<<6)
#define SPI_TX_RX_EEPROM			(1U<<6)

#define SPI_MASTER					(0U<<7)
#define SPI_SLAVE					(1U<<7)

#define SPI_XFER_POLLING			(0U<<8)
#define SPI_XFER_ISR				(1U<<8)
#define SPI_XFER_DMA				(1U<<9)

#define SPI_EOT_POLLING				(0U<<10)
#define SPI_EOT_ISR					(1U<<10)

#define SPI_UWIRE_NO_HS				(0U<<11)
#define SPI_UWIRE_HS				(1U<<11)

#define SPI_CFG_CS_N				(0U<<12)
#define SPI_CFG_CS_NN				(1U<<12)

#define SPI_CFG_CACHE_RX(Acp,Page)	(((Page)<<30)|(((Acp)!=0)<<29)|(1U<<28))
#define SPI_CFG_CACHE_TX(Acp,Page)	(((Page)<<30)|(((Acp)!=0)<<29)|(1U<<27))

													/* Argument Mode in spi_set()					*/
#define SPI_RX_WATER_ISR	0						/* RX FIFO watermark (specified in %) with ISRs	*/
#define SPI_TX_WATER_ISR	1						/* TX FIFO watermark (specified in %) with ISRs	*/
#define SPI_RX_MIN_ISR		2						/* Minimum # of frames in RX FIFO to use ISRs	*/
#define SPI_TX_MIN_ISR		3						/* Minimum # of frames in TX FIFO to use DMA	*/
#define SPI_RX_MIN_DMA		4						/* Minimum # of frames in RX FIFO to use DMA	*/
#define SPI_TX_MIN_DMA		5						/* Minimum # of frames in TX FIFO to use DMA	*/
#define SPI_TIMEOUT			6						/* TimeOut in # tick (-ve:OK, 0:use default)	*/

/* ------------------------------------------------------------------------------------------------ */
/* Function prototypes																				*/

#define   DW_SPI_ADD_PREFIX(Token)				 _DW_SPI_ADD_PREFIX(DW_SPI_PREFIX, Token)
#define  _DW_SPI_ADD_PREFIX(Prefix, Token)		__DW_SPI_ADD_PREFIX(Prefix, Token)
#define __DW_SPI_ADD_PREFIX(Prefix, Token)		Prefix##Token


int     DW_SPI_ADD_PREFIX(spi_init)      (int Dev, int Slv, int Freq, int Bits, uint32_t Mode);
int     DW_SPI_ADD_PREFIX(spi_set)       (int Dev, int Slv, int Type, int Val);
int     DW_SPI_ADD_PREFIX(spi_recv)      (int Dev, int Slv, void *Buf, uint32_t Len);
int     DW_SPI_ADD_PREFIX(spi_send)      (int Dev, int Slv, const void *Buf, uint32_t Len);
int     DW_SPI_ADD_PREFIX(spi_send_recv) (int Dev, int Slv, const void *BufTX, uint32_t LenTX,
                                          void *BufRX, uint32_t LenRX);

/* ------------------------------------------------------------------------------------------------ */
#define  DW_EXT_SPI_INT_HNDL(Prefix, Device) _DW_EXT_SPI_INT_HNDL(Prefix, Device)
#define _DW_EXT_SPI_INT_HNDL(Prefix, Device) extern void Prefix##SPIintHndl_##Device(void)

#if (((DW_SPI_LIST_DEVICE) & 0x001) != 0)
  DW_EXT_SPI_INT_HNDL(DW_SPI_PREFIX, 0);
#endif
#if (((DW_SPI_LIST_DEVICE) & 0x002) != 0)
  DW_EXT_SPI_INT_HNDL(DW_SPI_PREFIX, 1);
#endif
#if (((DW_SPI_LIST_DEVICE) & 0x004) != 0)
  DW_EXT_SPI_INT_HNDL(DW_SPI_PREFIX, 2);
#endif
#if (((DW_SPI_LIST_DEVICE) & 0x008) != 0)
  DW_EXT_SPI_INT_HNDL(DW_SPI_PREFIX, 3);
#endif
#if (((DW_SPI_LIST_DEVICE) & 0x010) != 0)
  DW_EXT_SPI_INT_HNDL(DW_SPI_PREFIX, 4);
  void SPIintHndl_4(void);
#endif
#if (((DW_SPI_LIST_DEVICE) & 0x020) != 0)
  DW_EXT_SPI_INT_HNDL(DW_SPI_PREFIX, 5);
#endif
#if (((DW_SPI_LIST_DEVICE) & 0x040) != 0)
  DW_EXT_SPI_INT_HNDL(DW_SPI_PREFIX, 6);
  void SPIintHndl_6(void);
#endif
#if (((DW_SPI_LIST_DEVICE) & 0x080) != 0)
  DW_EXT_SPI_INT_HNDL(DW_SPI_PREFIX, 7);
  void SPIintHndl_7(void);
#endif
#if (((DW_SPI_LIST_DEVICE) & 0x100) != 0)
  DW_EXT_SPI_INT_HNDL(DW_SPI_PREFIX, 8);
  void SPIintHndl_8(void);
#endif
#if (((DW_SPI_LIST_DEVICE) & 0x200) != 0)
  DW_EXT_SPI_INT_HNDL(DW_SPI_PREFIX, 9);
  void SPIintHndl_9(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
