/* ------------------------------------------------------------------------------------------------ */
/* File :		alt_gpio.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for Altera's SocFPGA GPIOs on the HPS.										*/
/*																									*/
/*																									*/
/* Copyright(c) 2017-2019, Code-Time Technologies Inc. All rights reserved.							*/
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
/*	$Revision: 1.6 $																				*/
/*	$Date: 2019/01/10 18:07:02 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* See alt_gpio.c for NOTE / LIMITATIONS / NOT YET SUPPORTED / pin mapping							*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Supported value for the build options															*/
/* (OS_PLATFORM & 0x00FFFFFF):																		*/
/*					0xAA10 : Arria 10 																*/
/*					0xAAC5 : Cyclone V & Arria V													*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* # of pins on (OS_PLATFORM & 0x00FFFFFF) / exluding FPGA GPIOs:									*/
/*					0xAA10 : 71		 																*/
/*					0xAAC5 : 85																		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Default values that depend on (OS_PLATFORM & 0x00FFFFFF):										*/
/* GPIO_MAX_DEVICES (Numver of interrupt handlers requited)											*/
/*					0xAA10 : 3		 																*/
/*					0xAAC5 : 3																		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __ALT_GPIO_H
#define __ALT_GPIO_H

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef GPIO_MULTI_DRIVER							/* To select if multiple GPIO drivers are used	*/
  #define GPIO_MULTI_DRIVER		0					/* in an application. Examples are SocFPGA with */
#endif												/* GPIO added in the FPGA in extra to processor	*/

#if ((GPIO_MULTI_DRIVER) == 0)						/* When not mutliple driver, don't prefix the	*/
  #define ALT_GPIO_PREFIX							/* API names									*/
  #define ALT_GPIO_ASCII		""
#else
 #ifndef ALT_GPIO_PREFIX							/* When multiple drivers, use a prefix			*/
  #define ALT_GPIO_PREFIX		 alt_				/* Default prefix, always the same as the file	*/
  #define ALT_GPIO_ASCII		"alt "				/* name prefix to gpio.c						*/
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Number of interrupt handler to attach															*/

#ifndef ALT_GPIO_NMB_INTS
  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)
    #define ALT_GPIO_NMB_INTS	3
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)
    #define ALT_GPIO_NMB_INTS	3
  #else
    #define ALT_GPIO_NMB_INTS	0
  #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* GPIO direction setting for gpio_dir()															*/
/*																									*/
/* *** The values assigned are kept the same across all GPIOs in case multiple drivers are used		*/
/*     in an application.																			*/

#define GPIO_DIR_IN					1
#define GPIO_DIR_OUT				0

/* ------------------------------------------------------------------------------------------------ */
/* Configuration defintions to use with gpio_cfg()													*/
/*																									*/
/* *** The values assigned are kept the same across all GPIOs in case multiple drivers are used		*/
/*     in an application.																			*/

#define GPIO_CFG_ISR_LEVEL_0		(1U<<1)
#define GPIO_CFG_ISR_LEVEL_1		(1U<<2)
#define GPIO_CFG_ISR_EDGE_0_1		(1U<<3)
#define GPIO_CFG_ISR_EDGE_1_0		(1U<<4)
#define GPIO_CFG_ISR_EDGE_ANY		(1U<<5)			/* Not supported, kept for compatibility		*/
#define GPIO_CFG_ISR_OFF			(1U<<6)
#define GPIO_CFG_DEB_OFF			(1U<<7)
#define GPIO_CFG_DEB_ON				(1U<<8)

/* ------------------------------------------------------------------------------------------------ */
/* Function prototypes																				*/

#define   ALT_GPIO_ADD_PREFIX(Token)				 _ALT_GPIO_ADD_PREFIX(ALT_GPIO_PREFIX, Token)
#define  _ALT_GPIO_ADD_PREFIX(Prefix, Token)		__ALT_GPIO_ADD_PREFIX(Prefix, Token)
#define __ALT_GPIO_ADD_PREFIX(Prefix, Token)		Prefix##Token

int ALT_GPIO_ADD_PREFIX(gpio_cfg)  (int IOpin, int Cfg);
int ALT_GPIO_ADD_PREFIX(gpio_dir)  (int IOpin, int Dir);
int ALT_GPIO_ADD_PREFIX(gpio_get)  (int IO);
int ALT_GPIO_ADD_PREFIX(gpio_init) (int Forced);
int ALT_GPIO_ADD_PREFIX(gpio_set)  (int IOpin, int Value);

/* ------------------------------------------------------------------------------------------------ */
/* Interrupt stuff																					*/

#define  ALT_EXT_GPIO_INT_HNDL(Prefix, Device) _ALT_EXT_GPIO_INT_HNDL(Prefix, Device)
#define _ALT_EXT_GPIO_INT_HNDL(Prefix, Device) extern void Prefix##GPIOintHndl_##Device(void)

void ALT_GPIO_ADD_PREFIX(GPIOintHndl) (int IO);		/* Application provided call back function		*/

#if ((ALT_GPIO_NMB_INTS) > 0)
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX, 0);
#endif
#if ((ALT_GPIO_NMB_INTS) > 1)
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX, 1);
#endif
#if ((ALT_GPIO_NMB_INTS) > 2)
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX, 2);
#endif
#if ((ALT_GPIO_NMB_INTS) > 3)
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX, 3);
#endif
#if ((ALT_GPIO_NMB_INTS) > 4)
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX, 4);
#endif
#if ((ALT_GPIO_NMB_INTS) > 5)
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX, 5);
#endif
#if ((ALT_GPIO_NMB_INTS) > 6)
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX, 6);
#endif
#if ((ALT_GPIO_NMB_INTS) > 7)
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX, 7);
#endif
#if ((ALT_GPIO_NMB_INTS) > 8)
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX, 8);
#endif
#if ((ALT_GPIO_NMB_INTS) > 9)
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX, 9);
#endif
#if (defined(GPIO_FPGA_ADDR_1) || defined(ALT_GPIO_FPGA_ADDR_1))
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX,  1000);
#endif
#if (defined(GPIO_FPGA_ADDR_2) || defined(ALT_GPIO_FPGA_ADDR_2))
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX,  2000);
#endif
#if (defined(GPIO_FPGA_ADDR_3) || defined(ALT_GPIO_FPGA_ADDR_3))
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX,  3000);
#endif
#if (defined(GPIO_FPGA_ADDR_4) || defined(ALT_GPIO_FPGA_ADDR_4))
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX,  4000);
#endif
#if (defined(GPIO_FPGA_ADDR_5) || defined(ALT_GPIO_FPGA_ADDR_5))
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX,  5000);
#endif
#if (defined(GPIO_FPGA_ADDR_6) || defined(ALT_GPIO_FPGA_ADDR_6))
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX,  6000);
#endif
#if (defined(GPIO_FPGA_ADDR_7) || defined(ALT_GPIO_FPGA_ADDR_7))
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX,  7000);
#endif
#if (defined(GPIO_FPGA_ADDR_8) || defined(ALT_GPIO_FPGA_ADDR_8))
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX,  8000);
#endif
#if (defined(GPIO_FPGA_ADDR_9) || defined(ALT_GPIO_FPGA_ADDR_9))
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX,  9000);
#endif
#if (defined(GPIO_FPGA_ADDR_10) || defined(ALT_GPIO_FPGA_ADDR_10))
  ALT_EXT_GPIO_INT_HNDL(ALT_GPIO_PREFIX, 1000);
#endif

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
