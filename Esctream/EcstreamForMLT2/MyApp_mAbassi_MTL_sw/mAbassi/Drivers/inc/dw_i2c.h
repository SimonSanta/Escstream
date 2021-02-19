/* ------------------------------------------------------------------------------------------------ */
/* FILE :		dw_i2c.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the DesignWare I2C peripheral.											*/
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
/*	$Revision: 1.13 $																				*/
/*	$Date: 2019/01/10 18:07:03 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* See dw_i2c.c for NOTE / LIMITATIONS / NOT YET SUPPORTED											*/
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
/*					0xAA10 : 5																		*/
/*					0xAAC5 : 4																		*/
/* I2C_LIST_DEVICE:																					*/
/*					0xAA10 : 0x1F																	*/
/*					0xAAC5 : 0x0F																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __DW_I2C_H
#define __DW_I2C_H

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef I2C_MULTI_DRIVER							/* To select if multiple I2C drivers are used	*/
  #define I2C_MULTI_DRIVER		0					/* in an application. Examples are SocFPGA with */
#endif												/* I2C added in the FPGA in extra to processor	*/

#if ((I2C_MULTI_DRIVER) == 0)						/* When not mutliple driver, don't prefix the	*/
  #define DW_I2C_PREFIX								/* API names									*/
  #define DW_I2C_ASCII			""
#else
 #ifndef DW_I2C_PREFIX								/* When multiple drivers, use a prefix			*/
  #define DW_I2C_PREFIX			 dw_				/* Default prefix, always the same as the file	*/
  #define DW_I2C_ASCII			"dw "				/* name prefix to spi.c							*/
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */

#if ((I2C_MULTI_DRIVER) == 0)
 #if !defined(I2C_MAX_DEVICES)
  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0xAAC5)
	#define I2C_MAX_DEVICES	4						/* Arria V / Cyclone V has a total of 4 I2C		*/
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0xAA10)
	#define I2C_MAX_DEVICES	5						/* Arria 10 has a total of 5 I2Cs				*/
  #else
	#define I2C_MAX_DEVICES	1						/* Undefined platform							*/
  #endif
 #endif

 #ifndef I2C_LIST_DEVICE
  #define I2C_LIST_DEVICE		((1U<<(I2C_MAX_DEVICES))-1U)
 #endif

 #if (((1U<<(I2C_MAX_DEVICES))-1U) < (I2C_LIST_DEVICE))
  #error "too many devices in I2C_LIST_DEVICE vs the max # device defined by I2C_MAX_DEVICES"
 #endif

 #undef  DW_I2C_MAX_DEVICES
 #define DW_I2C_MAX_DEVICES (I2C_MAX_DEVICES)

 #undef  DW_I2C_LIST_DEVICE
 #define DW_I2C_LIST_DEVICE (I2C_LIST_DEVICE)

#else
 #if (!defined(DW_I2C_LIST_DEVICE) && !defined(DW_I2C_MAX_DEVICES))
	#if defined(I2C_LIST_DEVICE)
	  #define DW_I2C_LIST_DEVICE	(I2C_LIST_DEVICE)
	#endif
	#if defined(I2C_MAX_DEVICES)
	  #define DW_I2C_MAX_DEVICES	(I2C_MAX_DEVICES)
	#endif
 #endif

 #if !defined(DW_I2C_MAX_DEVICES)
  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0xAAC5)
	#define DW_I2C_MAX_DEVICES	4					/* Arria V / Cyclone V has a total of 4 I2Cs	*/
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0xAA10)
	#define DW_I2C_MAX_DEVICES	5					/* Arria 10 has a total of 5 I2Cs				*/
  #else
	#define DW_I2C_MAX_DEVICES	1					/* Undefined platform							*/
  #endif
 #endif

 #ifndef DW_I2C_LIST_DEVICE
  #define DW_I2C_LIST_DEVICE		((1U<<(DW_I2C_MAX_DEVICES))-1U)
 #endif

 #if (((1U<<(DW_I2C_MAX_DEVICES))-1U) < (DW_I2C_LIST_DEVICE))
  #error "too many devices in DW_I2C_LIST_DEVICE vs the max # device defined by DW_I2C_MAX_DEVICES"
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */

#define I2C_MINIMUM_SPEED	 (10000)
#define I2C_STANDARD_SPEED	(100000)
#define I2C_FAST_SPEED		(400000)

/* ------------------------------------------------------------------------------------------------ */
/* Function prototypes																				*/

#define   DW_I2C_ADD_PREFIX(Token)				 _DW_I2C_ADD_PREFIX(DW_I2C_PREFIX, Token)
#define  _DW_I2C_ADD_PREFIX(Prefix, Token)		__DW_I2C_ADD_PREFIX(Prefix, Token)
#define __DW_I2C_ADD_PREFIX(Prefix, Token)		Prefix##Token

int  DW_I2C_ADD_PREFIX(i2c_init)         (int Device, int AddBits, int Freq);
int  DW_I2C_ADD_PREFIX(i2c_recv)         (int Device, int target, char *Buf, int Len);
int  DW_I2C_ADD_PREFIX(i2c_send)         (int Device, int target, const char *Buf, int Len);
int  DW_I2C_ADD_PREFIX(i2c_send_recv)    (int Device, int target, const char *BufTX, int LenTX,
                                          char *BufRX, int LenRX);
void DW_I2C_ADD_PREFIX(i2c_test)         (int Device);

/* ------------------------------------------------------------------------------------------------ */
#define  DW_EXT_I2C_INT_HNDL(Prefix, Device) _DW_EXT_I2C_INT_HNDL(Prefix, Device)
#define _DW_EXT_I2C_INT_HNDL(Prefix, Device) extern void Prefix##I2CintHndl_##Device(void)

#if (((DW_I2C_LIST_DEVICE) & 0x001) != 0)
  DW_EXT_I2C_INT_HNDL(DW_I2C_PREFIX, 0);
#endif
#if (((DW_I2C_LIST_DEVICE) & 0x002) != 0)
  DW_EXT_I2C_INT_HNDL(DW_I2C_PREFIX, 1);
#endif
#if (((DW_I2C_LIST_DEVICE) & 0x004) != 0)
  DW_EXT_I2C_INT_HNDL(DW_I2C_PREFIX, 2);
#endif
#if (((DW_I2C_LIST_DEVICE) & 0x008) != 0)
  DW_EXT_I2C_INT_HNDL(DW_I2C_PREFIX, 3);
#endif
#if (((DW_I2C_LIST_DEVICE) & 0x010) != 0)
  DW_EXT_I2C_INT_HNDL(DW_I2C_PREFIX, 4);
  void I2CintHndl_4(void);
#endif
#if (((DW_I2C_LIST_DEVICE) & 0x020) != 0)
  DW_EXT_I2C_INT_HNDL(DW_I2C_PREFIX, 5);
#endif
#if (((DW_I2C_LIST_DEVICE) & 0x040) != 0)
  DW_EXT_I2C_INT_HNDL(DW_I2C_PREFIX, 6);
  void I2CintHndl_6(void);
#endif
#if (((DW_I2C_LIST_DEVICE) & 0x080) != 0)
  DW_EXT_I2C_INT_HNDL(DW_I2C_PREFIX, 7);
  void I2CintHndl_7(void);
#endif
#if (((DW_I2C_LIST_DEVICE) & 0x100) != 0)
  DW_EXT_I2C_INT_HNDL(DW_I2C_PREFIX, 8);
  void I2CintHndl_8(void);
#endif
#if (((DW_I2C_LIST_DEVICE) & 0x200) != 0)
  DW_EXT_I2C_INT_HNDL(DW_I2C_PREFIX, 9);
  void I2CintHndl_9(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
