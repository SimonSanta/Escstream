/* ------------------------------------------------------------------------------------------------ */
/* FILE :		alt_gpio.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for Altera's SocFPGA GPIOs on the HPS.										*/
/*				FPGA GPIO is the PIO core with Avalon Interface										*/
/*																									*/
/*																									*/
/* Copyright (c) 2017-2019, Code-Time Technologies Inc. All rights reserved.						*/
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
/*	$Revision: 1.8 $																				*/
/*	$Date: 2019/01/10 18:07:05 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* NOTES:																							*/
/*		The exclusive access protection used is through ISR disabling / re-enabling (+spinlock for	*/
/*		multi-core) instead of mutexes.  This has been chosen to allow the use of gpio_set()		*/
/*		in interrupt handlers. If mutexes were used, it would have made gpio_set() not-usable in	*/
/*		ISRs.																						*/
/*																									*/
/*		This GPIO driver has the build option GPIO_FPGA_CONF_% not described in the doc. See text	*/
/*		below.																						*/
/*																									*/
/*		Currently supported are Altera Avalon's PIO Core											*/
/*		FPGA buils options:																			*/
/*			GPIO_FPGA_ADDR_#																		*/
/*				Base address of module PIO Core.  # must start from 1 and be contiguous				*/
/*			GPIO_FPGA_CONF_#																		*/
/*				Hove the PIO core has been configured (what it physically supports)					*/
/*				Nibbles are for hex number , Nibble #0 is LSnibble									*/
/*				Nibble #0 : == 0 : output only														*/
/*				            == 1 : input only														*/
/*				            >= 2 : both input and output (either in-out or BiDur)					*/
/*				Nibble #1 : == 0 : interrupts not available											*/
/*				            == 1 : interrupts supported (Level)										*/
/*				            == 2 : interrupts supported (Edge)										*/
/*				            >= 3 : interrupts supported (Edge with bit clearing register)			*/
/*				Nibble #2 : == 0 : bit set / clr not supported										*/
/*				            != 0 : bit set / clr supported											*/
/*				When not defined, defaults to 0x113													*/
/*			The Altera PIO Core doc states that writing to a non-existent register does nothing		*/
/*			and reading a non-existent register returns an undefined value.  This is taken advantge	*/
/*			of to eliminate conditonals.															*/
/*																									*/
/* LIMITATIONS:																						*/
/*		On the Avalon FPGA PIO cores, the interrupt handler takes care of clearing the edge			*/
/*		detection register, menaing the handler does remove the raised interrupts.  It cannot		*/
/*		handle the level interrupts.  Either the call-back function associated with GPIOs that		*/
/*		are interrupt level sensitive removes the source of the level or it has to disable the 		*/
/*		corresponding bit in the interrupt mask register.											*/
/*		Use gpio_cfg(Pin, GPIO_CFG_ISR_OFF) for that.												*/
/*																									*/
/*																									*/
/*		gpio_dir() support:																			*/
/*				HPS:     fully supported															*/
/*				Avalion: supported. If the peripheral is implemented for input only or for output	*/
/*				         only, gpio_dir() reports an error id the direction is setting is wrong.	*/
/*																									*/
/*		gpio_cfg() support:																			*/
/*			GPIO_CFG_ISR_LEVEL_0:																	*/
/*				HPS:    recognized and a level of 0 activates the interrupt							*/
/*				Avalon: enable the interrupt but the type of interrupt is set at implementeation	*/
/*			GPIO_CFG_ISR_LEVEL_1:																	*/
/*				HPS:    recognized and a level of 1 activates the interrupt							*/
/*				Avalon: enable the interrupt but the type of interrupt is set at implementeation	*/
/*			GPIO_CFG_ISR_EDGE_0_1:																	*/
/*				HPS:    recognized and a transtion from 0 to 1 activates the interrupt				*/
/*				Avalon: enable the interrupt but the type of interrupt is set at implementeation	*/
/*			GPIO_CFG_ISR_EDGE_1_0:																	*/
/*				HPS:    recognized and a transtion from 1 to 0 activates the interrupt				*/
/*				Avalon: enable the interrupt but the type of interrupt is set at implementeation	*/
/*			GPIO_CFG_ISR_EDGE_ANY:																	*/
/*				HPS:    recognized and a transtion from 1 to 0 or 0 to 1 activates the interrupt	*/
/*				Avalon: enable the interrupt but the type of interrupt is set at implementeation	*/
/*			GPIO_CFG_ISR_OFF:																		*/
/*				HPS:    recognized and fully supported												*/
/*				Avalon: recognized and fully supported												*/
/*			GPIO_CFG_DEB_OFF:																		*/
/*				HPS:    recognized and fully supported												*/
/*				Avalon: ignored																		*/
/*			GPIO_CFG_DEB_ON:																		*/
/*				HPS:    recognized and fully supported												*/
/*				Avalon: ignored																		*/
/*																									*/
/* NOT YET SUPPORTED:																				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* HPS pin numbering mapping & associated interruot handlers										*/
/*																									*/
/* Cyclone V / Arria V:		85 I/O pins and 3 interrtup handlers									*/
/*				IOpin   0-> 28 : bank #0 : HPS_GPIO_0  -> HPS_GPIO28								*/
/*				                           GPIOintHndl_0()											*/
/*				IOpin  29-> 57 : bank #1 : HPS_GPIO_29 -> HPS_GPIO57								*/
/*				                           GPIOintHndl_1()											*/
/*				IOpin  58-> 84 : bank #2 : HPS_GPIO_58 -> HPS_GPIO70								*/
/*				                           GPIOintHndl_2()											*/
/*				                           HPS_HLGPI0  -> HPS_HLGPI13								*/
/*																									*/
/* Arria 10:				70 I/O pins and 3 interrtup handlers									*/
/*				IOpin   0-> 23 : bank #0 : HPS_GPIO_0  -> HPS_GPIO23								*/
/*				                           GPIOintHndl_0()											*/
/*				IOpin  24-> 47 : bank #1 : HPS_GPIO_24 -> HPS_GPIO47								*/
/*				                           GPIOintHndl_1()											*/
/*				IOpin  48-> 70 : bank #2 : HPS_GPIO_48 -> HPS_GPIO70								*/
/*				                           GPIOintHndl_2()											*/
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

#include "alt_gpio.h"

#ifndef GPIO_USE_ISR
  #define GPIO_USE_ISR				1				/* If interrupts are use on GPIOs				*/
#endif

#ifndef GPIO_ARG_CHECK								/* If checking validity of function arguments	*/
  #define GPIO_ARG_CHECK			1
#endif

/* ------------------------------------------------------------------------------------------------ */
/* These are internal defintions that enable the use of multiple devices type GPIO drivers			*/
/* Doing this keep the whole core code of the GPIO the same across all types of GPIO				*/
/* the GPIO generic GPIO_??? and/or specific ???_GPIO_ tokens are remapped to MY_GPIO_??? tokens	*/

#ifdef ALT_GPIO_USE_ISR
  #define MY_GPIO_USE_ISR			(ALT_GPIO_USE_ISR)
#lse
  #define MY_GPIO_USE_ISR			(GPIO_USE_ISR)
#endif

#ifdef ALT_GPIO_ARG_CHECK							/* If checking validity of function arguments	*/
  #define MY_GPIO_ARG_CHECK			(ALT_GPIO_ARG_CHECK)
#else
  #define MY_GPIO_ARG_CHECK			(GPIO_ARG_CHECK)
#endif

#ifdef ALT_GPIO_FPGA_ADDR_1
  #define MY_GPIO_FPGA_ADDR_1		(ALT_GPIO_FPGA_ADDR_1)
#elif defined(GPIO_FPGA_ADDR_1)
  #define MY_GPIO_FPGA_ADDR_1		(GPIO_FPGA_ADDR_1)
#endif

#ifdef ALT_GPIO_FPGA_ADDR_2
  #define MY_GPIO_FPGA_ADDR_2		(ALT_GPIO_FPGA_ADDR_2)
#elif defined(GPIO_FPGA_ADDR_2)
  #define MY_GPIO_FPGA_ADDR_2		(GPIO_FPGA_ADDR_2)
#endif

#ifdef ALT_GPIO_FPGA_ADDR_3
  #define MY_GPIO_FPGA_ADDR_3		(ALT_GPIO_FPGA_ADDR_3)
#elif defined(GPIO_FPGA_ADDR_3)
  #define MY_GPIO_FPGA_ADDR_3		(GPIO_FPGA_ADDR_3)
#endif

#ifdef ALT_GPIO_FPGA_ADDR_4
  #define MY_GPIO_FPGA_ADDR_4		(ALT_GPIO_FPGA_ADDR_4)
#elif defined(GPIO_FPGA_ADDR_4)
  #define MY_GPIO_FPGA_ADDR_4		(GPIO_FPGA_ADDR_4)
#endif

#ifdef ALT_GPIO_FPGA_ADDR_5
  #define MY_GPIO_FPGA_ADDR_5		(ALT_GPIO_FPGA_ADDR_5)
#elif defined(GPIO_FPGA_ADDR_5)
  #define MY_GPIO_FPGA_ADDR_5		(GPIO_FPGA_ADDR_5)
#endif

#ifdef ALT_GPIO_FPGA_ADDR_6
  #define MY_GPIO_FPGA_ADDR_6		(ALT_GPIO_FPGA_ADDR_6)
#elif defined(GPIO_FPGA_ADDR_6)
  #define MY_GPIO_FPGA_ADDR_6		(GPIO_FPGA_ADDR_6)
#endif

#ifdef ALT_GPIO_FPGA_ADDR_7
  #define MY_GPIO_FPGA_ADDR_7		(ALT_GPIO_FPGA_ADDR_7)
#elif defined(GPIO_FPGA_ADDR_7)
  #define MY_GPIO_FPGA_ADDR_7		(GPIO_FPGA_ADDR_7)
#endif

#ifdef ALT_GPIO_FPGA_ADDR_8
  #define MY_GPIO_FPGA_ADDR_8		(ALT_GPIO_FPGA_ADDR_8)
#elif defined(GPIO_FPGA_ADDR_8)
  #define MY_GPIO_FPGA_ADDR_8		(GPIO_FPGA_ADDR_8)
#endif

#ifdef ALT_GPIO_FPGA_ADDR_9
  #define MY_GPIO_FPGA_ADDR_9		(ALT_GPIO_FPGA_ADDR_9)
#elif defined(GPIO_FPGA_ADDR_9)
  #define MY_GPIO_FPGA_ADDR_9		(GPIO_FPGA_ADDR_9)
#endif

#ifdef ALT_GPIO_FPGA_ADDR_10
  #define MY_GPIO_FPGA_ADDR_10		(ALT_GPIO_FPGA_ADDR_10)
#elif defined(GPIO_FPGA_ADDR_10)
  #define MY_GPIO_FPGA_ADDR_10		(GPIO_FPGA_ADDR_10)
#endif

#ifdef ALT_GPIO_FPGA_CONF_1
  #define MY_GPIO_FPGA_CONF_1		(ALT_GPIO_FPGA_CONF_1)
#elif defined(GPIO_FPGA_CONF_1)
  #define MY_GPIO_FPGA_CONF_1		(GPIO_FPGA_CONF_1)
#endif

#ifdef ALT_GPIO_FPGA_CONF_2
  #define MY_GPIO_FPGA_CONF_2		(ALT_GPIO_FPGA_CONF_2)
#elif defined(GPIO_FPGA_CONF_2)
  #define MY_GPIO_FPGA_CONF_2		(GPIO_FPGA_CONF_2)
#endif

#ifdef ALT_GPIO_FPGA_CONF_3
  #define MY_GPIO_FPGA_CONF_3		(ALT_GPIO_FPGA_CONF_3)
#elif defined(GPIO_FPGA_CONF_3)
  #define MY_GPIO_FPGA_CONF_3		(GPIO_FPGA_CONF_3)
#endif

#ifdef ALT_GPIO_FPGA_CONF_4
  #define MY_GPIO_FPGA_CONF_4		(ALT_GPIO_FPGA_CONF_4)
#elif defined(GPIO_FPGA_CONF_4)
  #define MY_GPIO_FPGA_CONF_4		(GPIO_FPGA_CONF_4)
#endif

#ifdef ALT_GPIO_FPGA_CONF_5
  #define MY_GPIO_FPGA_CONF_5		(ALT_GPIO_FPGA_CONF_5)
#elif defined(GPIO_FPGA_CONF_5)
  #define MY_GPIO_FPGA_CONF_5		(GPIO_FPGA_CONF_5)
#endif

#ifdef ALT_GPIO_FPGA_CONF_6
  #define MY_GPIO_FPGA_CONF_6		(ALT_GPIO_FPGA_CONF_6)
#elif defined(GPIO_FPGA_CONF_6)
  #define MY_GPIO_FPGA_CONF_6		(GPIO_FPGA_CONF_6)
#endif

#ifdef ALT_GPIO_FPGA_CONF_7
  #define MY_GPIO_FPGA_CONF_7		(ALT_GPIO_FPGA_CONF_7)
#elif defined(GPIO_FPGA_CONF_7)
  #define MY_GPIO_FPGA_CONF_7		(GPIO_FPGA_CONF_7)
#endif

#ifdef ALT_GPIO_FPGA_CONF_8
  #define MY_GPIO_FPGA_CONF_8		(ALT_GPIO_FPGA_CONF_8)
#elif defined(GPIO_FPGA_CONF_8)
  #define MY_GPIO_FPGA_CONF_8		(GPIO_FPGA_CONF_8)
#endif

#ifdef ALT_GPIO_FPGA_CONF_9
  #define MY_GPIO_FPGA_CONF_9		(ALT_GPIO_FPGA_CONF_9)
#elif defined(GPIO_FPGA_CONF_9)
  #define MY_GPIO_FPGA_CONF_9		(GPIO_FPGA_CONF_9)
#endif

#ifdef ALT_GPIO_FPGA_CONF_10
  #define MY_GPIO_FPGA_CONF_10		(ALT_GPIO_FPGA_CONF_10)
#elif defined(GPIO_FPGA_CONF_10)
  #define MY_GPIO_FPGA_CONF_10		(GPIO_FPGA_CONF_10)
#endif

#define MY_GPIO_PREFIX				ALT_GPIO_PREFIX
#define MY_GPIO_ASCII				ALT_GPIO_ASCII

#undef  GPIO_ADD_PREFIX
#define GPIO_ADD_PREFIX				ALT_GPIO_ADD_PREFIX

#define MY_GPIO_NMB_INTS			ALTGPIO_NMB_INTS

/* ------------------------------------------------------------------------------------------------ */
/* Base addresses of the HW GPIO modules															*/

#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)		/* Arria V / Cyclone V							*/
  static volatile uint32_t * const g_HWaddr[3]  =  {(volatile uint32_t *)0xFF708000,
                                                    (volatile uint32_t *)0xFF709000,
                                                    (volatile uint32_t *)0xFF70A000
                                                   };

  #define GPIO_RESET()			do {																\
									*((volatile uint32_t *)0xFFD05014) |=  (7U << 25);				\
									*((volatile uint32_t *)0xFFD05014) &= ~(7U << 25);				\
								} while(0)

  #define GPIO_NMB_IO			85					/* Total number of individual GPIOs				*/
  #define GPIO_NMB_BANK			3					/* Number of GPIO device controllers			*/

  static const int g_BankSize[4] = {29, 29, 27};

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)	/* Arria 10										*/
  static volatile uint32_t * const g_HWaddr[3]  =  {(volatile uint32_t *)0xFFC02900,
                                                    (volatile uint32_t *)0xFFC02A00,
                                                    (volatile uint32_t *)0xFFC02B00
                                                   };

  #define GPIO_RESET()			do {																\
									*((volatile uint32_t *)0xFFD05028) |=  (1U << 24);				\
									*((volatile uint32_t *)0xFFD05028) &= ~(1U << 24);				\
								} while(0)

  #define GPIO_NMB_IO			71					/* Total number of individual GPIOs				*/
  #define GPIO_NMB_BANK			3					/* Number of GPIO device controllers			*/

  static const int g_BankSize[4] = {24, 24, 24};

#else
  	#error "Unsupported platform specified by OS_PLATFORM"

#endif

/* ------------------------------------------------------------------------------------------------ */
/* GPIO register index definitions (are accessed as uint32_t)										*/

#define GPIO_SWPORTA_DR_REG				(0x00/4)
#define GPIO_SWPORTA_DDR_REG			(0x04/4)
#define GPIO_INTEN_REG					(0x30/4)
#define GPIO_INTMASK_REG				(0x34/4)
#define GPIO_INTTYPE_LEVEL_REG			(0x38/4)
#define GPIO_INT_POLARITY_REG			(0x3C/4)
#define GPIO_INTSTATUS_REG				(0x40/4)
#define GPIO_RAW_INTSTATUS_REG			(0x44/4)
#define GPIO_DEBOUNCE_REG				(0x48/4)
#define GPIO_PORTA_EOI_REG				(0x4C/4)
#define GPIO_EXT_PORTA_REG				(0x50/4)
#define GPIO_LS_SYNC_REG				(0x60/4)
#define GPIO_CONFIG_REG2_REG			(0x70/4)
#define GPIO_CONFIG_REG1_REG			(0x74/4)

#define GPIO_FPGA_DATA_REG				(0x000/4)	/* I/O data register							*/
#define GPIO_FPGA_DIR_REG				(0x004/4)	/* I/O Direction register						*/
#define GPIO_FPGA_INT_REG				(0x008/4)	/* Interrupt enable mask						*/
#define GPIO_FPGA_EDGE_REG				(0x00C/4)	/* Edge capture register						*/
#define GPIO_FPGA_SET_REG				(0x010/4)	/* Set output register							*/
#define GPIO_FPGA_CLR_REG				(0x014/4)	/* Clear output register						*/

/* ------------------------------------------------------------------------------------------------ */
/* Number of extra "banks"																			*/

#if defined(MY_GPIO_FPGA_ADDR_10)
  #define FPGA_NMB_BANK			10
#elif defined(MY_GPIO_FPGA_ADDR_9)
  #define FPGA_NMB_BANK			9
#elif defined(MY_GPIO_FPGA_ADDR_8)
  #define FPGA_NMB_BANK			8
#elif defined(MY_GPIO_FPGA_ADDR_7)
  #define FPGA_NMB_BANK			7
#elif defined(MY_GPIO_FPGA_ADDR_6)
  #define FPGA_NMB_BANK			6
#elif defined(MY_GPIO_FPGA_ADDR_5)
  #define FPGA_NMB_BANK			5
#elif defined(MY_GPIO_FPGA_ADDR_4)
  #define FPGA_NMB_BANK			4
#elif defined(MY_GPIO_FPGA_ADDR_3)
  #define FPGA_NMB_BANK			3
#elif defined(MY_GPIO_FPGA_ADDR_2)
  #define FPGA_NMB_BANK			2
#elif defined(MY_GPIO_FPGA_ADDR_1)
  #define FPGA_NMB_BANK			1
#else
  #define FPGA_NMB_BANK			0
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Exclusive access protection macros																*/
/* In multi-core in extra to SPINlock(), ints are disable to allow the gpios to be called from		*/
/* interrupts																						*/

#if ((OX_N_CORE) == 1)
  #define GPIO_LOCK(_x)				do {_x=OSintOff();} while(0)
  #define GPIO_UNLOCK(_x)			(OSintBack(_x))

#else												/* Multi-core requires to use a spinlock		*/
  #define GPIO_LOCK(_x)				do {															\
										_x=OSintOff();												\
										SPINlock();													\
									} while(0)														\

  #define GPIO_UNLOCK(_x)			do {															\
										SPINunlock();												\
										OSintBack(_x);												\
									} while(0)														\

#endif

/* ------------------------------------------------------------------------------------------------ */
/* Local variables																					*/

static int g_GPIOinit = 0;							/* To track first time an init occurs			*/

 #if (defined(MY_GPIO_FPGA_ADDR_1))					/* To handle PIO without set / clr				*/
  #ifndef MY_GPIO_FPGA_CONF_1
	#define MY_GPIO_FPGA_CONF_1	0x113
  #endif
  static int g_FPGAreg[] = {
										 0
  #ifdef MY_GPIO_FPGA_ADDR_2
  #ifndef MY_GPIO_FPGA_CONF_2
	#define MY_GPIO_FPGA_CONF_2	0x113
  #endif
										,0
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_3
  #ifndef MY_GPIO_FPGA_CONF_3
	#define MY_GPIO_FPGA_CONF_3	0x113
  #endif
										,0
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_4
  #ifndef MY_GPIO_FPGA_CONF_4
	#define MY_GPIO_FPGA_CONF_4	0x113
  #endif
										,0
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_5
  #ifndef MY_GPIO_FPGA_CONF_5
	#define MY_GPIO_FPGA_CONF_5	0x113
  #endif
										,0
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_6
  #ifndef MY_GPIO_FPGA_CONF_6
	#define MY_GPIO_FPGA_CONF_6	0x113
  #endif
										,0
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_7
  #ifndef MY_GPIO_FPGA_CONF_7
	#define MY_GPIO_FPGA_CONF_7	0x113
  #endif
										,0
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_8
  #ifndef MY_GPIO_FPGA_CONF_8
	#define MY_GPIO_FPGA_CONF_8	0x113
  #endif
										,0
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_9
  #ifndef MY_GPIO_FPGA_CONF_9
	#define MY_GPIO_FPGA_CONF_9	0x113
  #endif
										,0
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_10
  #ifndef MY_GPIO_FPGA_CONF_10
	#define MY_GPIO_FPGA_CONF_10	0x113
  #endif
										,0
  #endif
								};
 #endif


/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: GetFPGAaddr																			*/
/*																									*/
/* GetFPGAaddr - retreive the base address of a FPGA GPIO module									*/
/*																									*/
/* SYNOPSIS:																						*/
/*		uint32_t *GetFPGAaddr(int IOpin, int *Config);												*/
/*																									*/
/* ARGUMENTS																						*/
/*		IOpin : GPIO I/O number																		*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int : != NULL : GPIO registers base address													*/
/*		      == NULL : not a FPGA GPIO module														*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
#ifdef MY_GPIO_FPGA_ADDR_1

static uint32_t *GetFPGAaddr(int IOpin, int *Config)
{

	if ((IOpin >= 1000)
	&&  (IOpin <  1032)) {
		*Config = MY_GPIO_FPGA_CONF_1;
		return((uint32_t *)MY_GPIO_FPGA_ADDR_1);
	}

  #ifdef MY_GPIO_FPGA_ADDR_2
	if ((IOpin >= 2000)
	&&  (IOpin <  2032)) {
		*Config = MY_GPIO_FPGA_CONF_2;
		return((uint32_t *)MY_GPIO_FPGA_ADDR_2);
	}
  #endif

  #ifdef MY_GPIO_FPGA_ADDR_3
	if ((IOpin >= 3000)
	&&  (IOpin <  3032)) {
		*Config = MY_GPIO_FPGA_CONF_3;
		return((uint32_t *)MY_GPIO_FPGA_ADDR_3);
	}
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_4
	if ((IOpin >= 4000)
	&&  (IOpin <  4032)) {
		*Config = MY_GPIO_FPGA_CONF_4;
		return((uint32_t *)MY_GPIO_FPGA_ADDR_4);
	}
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_5
	if ((IOpin >= 5000)
	&&  (IOpin <  5032)) {
		*Config = MY_GPIO_FPGA_CONF_5;
		return((uint32_t *)MY_GPIO_FPGA_ADDR_5);
	}
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_6
	if ((IOpin >= 6000)
	&&  (IOpin <  6032)) {
		*Config = MY_GPIO_FPGA_CONF_6;
		return((uint32_t *)MY_GPIO_FPGA_ADDR_6);
	}
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_7
	if ((IOpin >= 7000)
	&&  (IOpin <  7032)) {
		*Config = MY_GPIO_FPGA_CONF_7;
		return((uint32_t *)MY_GPIO_FPGA_ADDR_7);
	}
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_8
	if ((IOpin >= 8000)
	&&  (IOpin <  8032)) {
		*Config = MY_GPIO_FPGA_CONF_8;
		return((uint32_t *)MY_GPIO_FPGA_ADDR_8);
	}
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_9
	if ((IOpin >= 9000)
	&&  (IOpin <  9032)) {
		*Config = MY_GPIO_FPGA_CONF_9;
		return((uint32_t *)MY_GPIO_FPGA_ADDR_9);
	}
  #endif
  #ifdef MY_GPIO_FPGA_ADDR_10
	if ((IOpin >= 10000)
	&&  (IOpin <  10032)) {
		*Config = MY_GPIO_FPGA_CONF_10;
		return((uint32_t *)MY_GPIO_FPGA_ADDR_10);
	}
  #endif

	return(NULL);
}
#endif

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: gpio_cfg																				*/
/*																									*/
/* gpio_cfg - configure the behavior of a GPIO pin													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int gpio_cfg(int IOpin, int Cfg);															*/
/*																									*/
/* ARGUMENTS																						*/
/*		IOpin : GPIO I/O number																		*/
/*		Cfg   : requested configuration																*/
/*		        defintions are in alt_gpio.h														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int : == 0 success																			*/
/*		      != 0 out of range IOpin number														*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int GPIO_ADD_PREFIX(gpio_cfg)(int IOpin, int Cfg)
{
int Bank;
int ISRstate;
volatile uint32_t *Reg;
#ifdef MY_GPIO_FPGA_ADDR_1
  int                Config;						/* How the PIO core is physically configured	*/
  volatile uint32_t *FPGAaddr;						/* Address of the Avalon PIO core				*/
  int                RetVal;						/* Return value									*/
#endif


  #ifdef MY_GPIO_FPGA_ADDR_1
	FPGAaddr = GetFPGAaddr(IOpin, &Config);
	if (FPGAaddr != (uint32_t*)NULL) {
		Bank   = (GPIO_NMB_BANK)
		       + (IOpin/1000)-1;
		IOpin   = 1U << (IOpin % 1000);
		RetVal = 0;

		GPIO_LOCK(ISRstate);

		if ((Cfg & ((GPIO_CFG_ISR_LEVEL_0)			/* Consider any of these as enabling ISRs		*/
	              | (GPIO_CFG_ISR_LEVEL_1)
	              | (GPIO_CFG_ISR_EDGE_0_1)
	              | (GPIO_CFG_ISR_EDGE_1_0)
		          | (GPIO_CFG_ISR_EDGE_ANY))) != 0) {
			if ((Config & 0x0F0) == 0x030) {		/* Only clear when bit clearing register exists	*/
				FPGAaddr[GPIO_FPGA_EDGE_REG] = IOpin;/* otherwise all bits would be cleared			*/
			}
			FPGAaddr[GPIO_FPGA_INT_REG] |= IOpin;	/* Enable the interrupts						*/
			if ((Config & 0x0F0) == 0x000) {		/* Interrupts are not supported					*/
				RetVal = -1;						/* Report the error								*/
			}
		}

		if ((Cfg & (GPIO_CFG_ISR_OFF)) != 0) {
			FPGAaddr[GPIO_FPGA_INT_REG] &= ~IOpin;	/* Disable the interrupts						*/
			if ((Config & 0x0F0) == 0x020) {		/* Interrupts are  not suppported				*/
				RetVal = -1;						/* Report the error								*/
			}
		}

		GPIO_UNLOCK(ISRstate);

		return(RetVal);
	}
  #endif

  #if ((GPIO_ARG_CHECK) != 0)
	if ((IOpin < 0)
	||  (IOpin >= GPIO_NMB_IO)) {
		return(-1);
	}
  #endif

	Bank = 0;										/* Determine which controller the IOpin is in	*/
	while (IOpin >= g_BankSize[Bank]) {				/* and find out which GPIO in that controller	*/
		IOpin -= g_BankSize[Bank++];
		if (Bank >= (GPIO_NMB_BANK)) {				/* Safety check									*/
			return(-1);
		}
	}

	Reg   = g_HWaddr[Bank];
	IOpin = 1U << IOpin;							/* Bit mask for GPIO number on that controller	*/

	GPIO_LOCK(ISRstate);

	if ((Cfg & (GPIO_CFG_DEB_OFF)) != 0) {
		Reg[GPIO_DEBOUNCE_REG] &= ~IOpin;
	}
	if ((Cfg & (GPIO_CFG_DEB_ON)) != 0) {
		Reg[GPIO_DEBOUNCE_REG] |= IOpin;		
	}
	if ((Cfg & (GPIO_CFG_ISR_LEVEL_0)) != 0) {
		Reg[GPIO_INTEN_REG]         &= ~IOpin;
		Reg[GPIO_INTTYPE_LEVEL_REG] &= ~IOpin;
		Reg[GPIO_INT_POLARITY_REG]  &= ~IOpin;
		Reg[GPIO_INTEN_REG]         |=  IOpin;
		Reg[GPIO_INTMASK_REG]       &= ~IOpin;
	}
	if ((Cfg & (GPIO_CFG_ISR_LEVEL_1)) != 0) {
		Reg[GPIO_INTEN_REG]         &= ~IOpin;
		Reg[GPIO_INTTYPE_LEVEL_REG] &= ~IOpin;
		Reg[GPIO_INT_POLARITY_REG]  |=  IOpin;
		Reg[GPIO_INTEN_REG]         |=  IOpin;
		Reg[GPIO_INTMASK_REG]       &= ~IOpin;
	}
	if ((Cfg & (GPIO_CFG_ISR_EDGE_0_1)) != 0) {
		Reg[GPIO_INTEN_REG]         &= ~IOpin;
		Reg[GPIO_INTTYPE_LEVEL_REG] |=  IOpin;
		Reg[GPIO_INT_POLARITY_REG]  |=  IOpin;
		Reg[GPIO_INTEN_REG]         |=  IOpin;
		Reg[GPIO_INTMASK_REG]       &= ~IOpin;
	}
	if ((Cfg & (GPIO_CFG_ISR_EDGE_1_0)) != 0) {
		Reg[GPIO_INTEN_REG]         &= ~IOpin;
		Reg[GPIO_INTTYPE_LEVEL_REG] |=  IOpin;
		Reg[GPIO_INT_POLARITY_REG]  &= ~IOpin;
		Reg[GPIO_INTEN_REG]         |=  IOpin;
		Reg[GPIO_INTMASK_REG]       &= ~IOpin;
	}
	if ((Cfg & (GPIO_CFG_ISR_OFF)) != 0) {
		Reg[GPIO_INTEN_REG]   &= ~IOpin;
		Reg[GPIO_INTMASK_REG] |=  IOpin;
	}

	GPIO_UNLOCK(ISRstate);

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: gpio_dir																				*/
/*																									*/
/* gpio_dir - set the direction of a gpio															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int gpio_dir(int IOpin, int Dir);															*/
/*																									*/
/* ARGUMENTS																						*/
/*		IOpin : GPIO I/O number																		*/
/*		Dir   : Direction																			*/
/*		        == 0 : output																		*/
/*		        != 0 : input																		*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int : == 0 success																			*/
/*		      != 0 error																			*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int GPIO_ADD_PREFIX(gpio_dir)(int IOpin, int Dir)
{
int Bank;
int ISRstate;
#ifdef MY_GPIO_FPGA_ADDR_1
  int                Config;						/* How the PIO core is physically configured	*/
  volatile uint32_t *FPGAaddr;						/* Address of the Avalon PIO core				*/
  int                RetVal;						/* Return value									*/
#endif

  #ifdef MY_GPIO_FPGA_ADDR_1
	FPGAaddr = GetFPGAaddr(IOpin, &Config);

	if (FPGAaddr != NULL) {
		Bank   = (GPIO_NMB_BANK)
		       + (IOpin/1000)-1;
		IOpin  = IOpin % 1000;
		RetVal = 0;

		GPIO_LOCK(ISRstate);

		if (Dir == 0) {								/* Output direction								*/
			FPGAaddr[GPIO_FPGA_DIR_REG] |= IOpin;
			if ((Config & 0x00F) == 0x001) {		/* PIO Core configured in input only			*/
				RetVal = -1;						/* Report the error								*/
			}
		}
		else {										/* Input direction								*/
			FPGAaddr[GPIO_FPGA_DIR_REG] &= ~IOpin;
			if ((Config & 0x00F) == 0x000) {		/* PIO Core configured in output only			*/
				RetVal = -1;						/* Report the error								*/
			}
		}

		GPIO_UNLOCK(ISRstate);

		return(RetVal);
	}
  #endif

  #if ((GPIO_ARG_CHECK) != 0)
	if ((IOpin < 0)
	||  (IOpin >= GPIO_NMB_IO)) {
		return(-1);
	}
  #endif


	Bank = 0;										/* Determine which controller the IOpin is in	*/
	while (IOpin >= g_BankSize[Bank]) {				/* and find out which GPIO in that controller	*/
		IOpin -= g_BankSize[Bank++];
		if (Bank >= (GPIO_NMB_BANK)) {				/* Safety check									*/
			return(-1);
		}
	}

	GPIO_LOCK(ISRstate);

	if (Dir == 0) {									/* Output direction								*/
		g_HWaddr[Bank][GPIO_SWPORTA_DDR_REG] |=  (1U << IOpin);
		g_HWaddr[Bank][GPIO_SWPORTA_DR_REG]  &= ~(1U << IOpin);
	}
	else {
		g_HWaddr[Bank][GPIO_SWPORTA_DDR_REG] &= ~(1U << IOpin);
	}

	GPIO_UNLOCK(ISRstate);

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: gpio_get																				*/
/*																									*/
/* gpio_get - get the value of an input gpio														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int gpio_get(int IOpin);																	*/
/*																									*/
/* ARGUMENTS																						*/
/*		IOpin : GPIO I/O number																		*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int : == 0 read a 0 (Low Level)																*/
/*		      == 1 read a 1 (High Level)															*/
/*		      -ve  error																			*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int GPIO_ADD_PREFIX(gpio_get)(int IOpin)
{
int Bank;
int Value;
#ifdef MY_GPIO_FPGA_ADDR_1
  int                Config;						/* How the PIO core is physically configured	*/
  int                Idx;							/* Index in g_FPGAreg[]							*/
  volatile uint32_t *FPGAaddr;						/* Address of the Avalon PIO core				*/
#endif


  #ifdef MY_GPIO_FPGA_ADDR_1
	FPGAaddr = GetFPGAaddr(IOpin, &Config);
	if (FPGAaddr != NULL) {
		Idx  = IOpin/1000;
		if ((Config & 0x00F) == 0x000) {			/* When output only, return the value memoed	*/
			Value = g_FPGAreg[Idx];
		}
		else {										/* Not output only, the register is good		*/
			Value = FPGAaddr[GPIO_FPGA_DATA_REG];	/* Read the data register 						*/
		}
		Value = (Value >> (IOpin % 1000))			/* Return 1 or 0								*/
		      & 1U;
		return(Value);
	}
  #endif

  #if ((GPIO_ARG_CHECK) != 0)
	if ((IOpin < 0)
	||  (IOpin >= GPIO_NMB_IO)) {
		return(-1);
	}
  #endif

	Bank = 0;										/* Determine which controller the IOpin is in	*/
	while (IOpin >= g_BankSize[Bank]) {				/* and find out which GPIO in that controller	*/
		IOpin -= g_BankSize[Bank++];
		if (Bank >= (GPIO_NMB_BANK)) {				/* Safety check									*/
			return(-1);
		}
	}

	Value = (g_HWaddr[Bank][GPIO_EXT_PORTA_REG] >> IOpin)
	      & 1U;
	
	return(Value);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: gpio_set																				*/
/*																									*/
/* gpio_set - set the value of an output gpio														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int gpio_set(int IOpin, int Value);															*/
/*																									*/
/* ARGUMENTS																						*/
/*		IOpin : GPIO I/O number																		*/
/*		Value : == 0 : output 0 (Low Level)															*/
/*		        != 0 : output 1 (High Level)														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int : == 0 success																			*/
/*		      != 0 error																			*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int GPIO_ADD_PREFIX(gpio_set)(int IOpin, int Value)
{
int Bank;
int ISRstate;
#ifdef MY_GPIO_FPGA_ADDR_1
  int                Config;						/* How the PIO core is physically configured	*/
  volatile uint32_t *FPGAaddr;						/* Address of the Avalon PIO core				*/
  int                Idx;							/* Index in g_FPGAreg[]							*/
#endif


  #ifdef MY_GPIO_FPGA_ADDR_1
	FPGAaddr = GetFPGAaddr(IOpin, &Config);
	if (FPGAaddr != NULL) {
		Idx    = IOpin/1000;
		Bank   = (GPIO_NMB_BANK)
		       + Idx - 1;
		IOpin  = 1U << (IOpin%1000);

		if ((Config & 0x00F) == 0) {				/* PIO Core is configured in input only			*/
			return(-1);								/* Report the error								*/
		}

		GPIO_LOCK(ISRstate);

		if (((Config & 0x00F) == 0x000)				/* When output only, memo the value written		*/
		||  ((Config & 0xF00) == 0x000)) {			/* Same when not configured for bit set / clr	*/
			if (Value == 0) {
				g_FPGAreg[Idx] &= ~IOpin;			/* Set the IOpin to 0, clear the bit			*/
			}
			else {
				g_FPGAreg[Idx] |= IOpin;			/* Set the IOpin to 1, set the bit				*/
			}
		}
		if ((Config & 0xF00) == 0) {				/* Bit set / clr not supported					*/
			FPGAaddr[GPIO_FPGA_DATA_REG] = g_FPGAreg[Idx];	/* Update the data register				*/
		}
		else {										/* Bit set / clr supported						*/
			if (Value == 0) {
				FPGAaddr[GPIO_FPGA_CLR_REG] = IOpin;/* Clear the IO				 					*/
			}
			else {
				FPGAaddr[GPIO_FPGA_SET_REG] = IOpin;/* Set the IOpin								*/
			}
		}

		GPIO_UNLOCK(ISRstate);

		return(0);
	}
  #endif

  #if ((GPIO_ARG_CHECK) != 0)
	if ((IOpin < 0)
	||  (IOpin >= GPIO_NMB_IO)) {
		return(-1);
	}
  #endif

	Bank = 0;										/* Determine which controller the IOpin is in	*/
	while (IOpin >= g_BankSize[Bank]) {				/* and find out which GPIO in that controller	*/
		IOpin -= g_BankSize[Bank++];
		if (Bank >= (GPIO_NMB_BANK)) {				/* Safety check									*/
			return(-1);
		}
	}

	GPIO_LOCK(ISRstate);

	if (Value == 0) {
		g_HWaddr[Bank][GPIO_SWPORTA_DR_REG]  &= ~(1U << IOpin);
	}
	else {
		g_HWaddr[Bank][GPIO_SWPORTA_DR_REG]  |=  (1U << IOpin);
	}

	GPIO_UNLOCK(ISRstate);

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: gpio_init																				*/
/*																									*/
/* gpio_init - intialize the GPIO driver															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int gpio_init(int Forced);																	*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Forced : if re-initializing when has already been initialized								*/
/*		         == 0 : do not re-init if is already initialized									*/
/*		         != 0 : force a re-init event if is already initialized								*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int : == 0 success																			*/
/*		      != 0 error																			*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int GPIO_ADD_PREFIX(gpio_init)(int Forced)
{
#ifdef MY_GPIO_FPGA_ADDR_1
  int                Config;						/* How the PIO core is physically configured	*/
  volatile uint32_t *FPGAaddr;						/* Address of the Avalon PIO core				*/
  int                ii;
#endif

	if (Forced != 0) {
		g_GPIOinit = 0;
	}

	if (g_GPIOinit == 0) {
		GPIO_RESET();

	  #ifdef MY_GPIO_FPGA_ADDR_1
		memset(&g_FPGAreg[0], 0, sizeof(g_FPGAreg));
		ii = 1000;
		do {
			FPGAaddr = GetFPGAaddr(ii, &Config);
			if (FPGAaddr != (uint32_t *)NULL) {		/* Make sure we have a valid PIO address		*/
				FPGAaddr[GPIO_FPGA_DATA_REG] = 0;	/* Make sure all output are 0's					*/
				FPGAaddr[GPIO_FPGA_INT_REG]  = 0;	/* Disable all ints								*/
			}
			ii += 1000;
		} while (FPGAaddr != (uint32_t *)NULL);
	  #endif

		g_GPIOinit = 1;
	}

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* Interrupt handlers for the GPIO																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static void GPIO_intHndl(int Dev)
{
int      ii;
 #ifdef MY_GPIO_FPGA_ADDR_1
  int                Config;						/* How the PIO core is physically configured	*/
  volatile uint32_t *FPGAaddr;						/* Address of the Avalon PIO core				*/
 #endif
#if ((MY_GPIO_USE_ISR) != 0)
 int      ISRstate;
 uint32_t IOpend;
 int      Offset;
 volatile uint32_t *Reg;
 #ifdef MY_GPIO_FPGA_ADDR_1
  uint32_t           IOenab;
 #endif

	GPIO_LOCK(ISRstate);

  #ifdef MY_GPIO_FPGA_ADDR_1

	FPGAaddr = GetFPGAaddr(Dev, &Config);
	if (FPGAaddr != (uint32_t *)NULL) {

		if ((Config & 0x0F0) != 0) {				/* Only check PIO Core with ints configured		*/
			IOenab      = FPGAaddr[GPIO_FPGA_INT_REG];	/* Read the interrupt enable mask register	*/
			IOpend      = FPGAaddr[GPIO_FPGA_EDGE_REG];	/* Read the  transition detected register	*/
			IOpend     &= IOenab;					/* Merge int enable with transition detected	*/
			FPGAaddr[GPIO_FPGA_EDGE_REG] = IOpend;	/* Clear all edge captured						*/
			for (ii=0 ; ((ii<32) && (IOpend!=0)) ; ii++) {
				if ((IOpend & 1U) != 0) {			/* If this I/O has transitioned & int enabled	*/
					GPIO_ADD_PREFIX(GPIOintHndl)(Dev+ii);	/* Callback function provided by app	*/
				}
				IOpend = IOpend >> 1;
			}
		}
	}
	else
  #endif
	{
		Offset = 0;
		for(ii=0 ; ii<Dev ; ii++) {
			Offset += g_BankSize[ii];
		}

		Reg                     = g_HWaddr[Dev];
		IOpend                  = Reg[GPIO_INTSTATUS_REG];
		Reg[GPIO_PORTA_EOI_REG] = IOpend;			/* Clear the interrupts							*/
		for (ii=0 ; ((ii<32) && (IOpend!=0)) ; ii++) {	/* Scan all 32. Invalid ones are reserved	*/
			if ((IOpend & 1U) != 0) {
				GPIO_ADD_PREFIX(GPIOintHndl)(ii+Offset);	/* Callback function provided by app	*/
			}
			IOpend = IOpend >> 1;
		}
	}

	GPIO_UNLOCK(ISRstate);

 #endif												/* #endif for MY_GPIO_USE_ISR != 0				*/

  #ifdef MY_GPIO_FPGA_ADDR_1
	FPGAaddr = GetFPGAaddr(Dev, &Config);
	if (FPGAaddr != (uint32_t *)NULL) {
		FPGAaddr[GPIO_FPGA_INT_REG] = 0;
	}
  #endif

	for (ii=0 ; ii<(GPIO_NMB_BANK) ; ii++) {
		g_HWaddr[ii][GPIO_INTEN_REG]   = 0;			/* Disable all interrupts						*/
		g_HWaddr[ii][GPIO_INTMASK_REG] = 0xFFFFFFFF;
	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Individual interrupt handlers macro. It calls the common handler with the device #				*/

  #define  GPIO_INT_HNDL(Prefix, Device) _GPIO_INT_HNDL(Prefix, Device)
  #define _GPIO_INT_HNDL(Prefix, Device)															\
	void Prefix##GPIOintHndl_##Device(void)															\
	{																								\
		GPIO_intHndl(Device);														\
		return;																						\
	}

/* ------------------------------------------------------------------------------------------------ */


#if ((GPIO_NMB_BANK) > 0)
  GPIO_INT_HNDL(MY_GPIO_PREFIX, 0)
#endif
#if ((GPIO_NMB_BANK) > 1)
  GPIO_INT_HNDL(MY_GPIO_PREFIX, 1)
#endif
#if ((GPIO_NMB_BANK) > 2)
  GPIO_INT_HNDL(MY_GPIO_PREFIX, 2)
#endif
#if ((GPIO_NMB_BANK) > 3)
  GPIO_INT_HNDL(MY_GPIO_PREFIX, 3)
#endif
#if ((GPIO_NMB_BANK) > 4)
  GPIO_INT_HNDL(MY_GPIO_PREFIX, 4)
#endif
#if ((GPIO_NMB_BANK) > 5)
  GPIO_INT_HNDL(MY_GPIO_PREFIX, 5)
#endif
#if ((GPIO_NMB_BANK) > 6)
  GPIO_INT_HNDL(MY_GPIO_PREFIX, 6)
#endif
#if ((GPIO_NMB_BANK) > 7)
  GPIO_INT_HNDL(MY_GPIO_PREFIX, 7)
#endif
#if ((GPIO_NMB_BANK) > 8)
  GPIO_INT_HNDL(MY_GPIO_PREFIX, 8)
#endif
#if ((GPIO_NMB_BANK) > 9)
  GPIO_INT_HNDL(MY_GPIO_PREFIX, 9)
#endif

#ifdef MY_GPIO_FPGA_ADDR_1
  GPIO_INT_HNDL(MY_GPIO_PREFIX,  1000)
#endif
#ifdef MY_GPIO_FPGA_ADDR_2
  GPIO_INT_HNDL(MY_GPIO_PREFIX,  2000)
#endif
#ifdef MY_GPIO_FPGA_ADDR_3
  GPIO_INT_HNDL(MY_GPIO_PREFIX,  3000)
#endif
#ifdef MY_GPIO_FPGA_ADDR_4
  GPIO_INT_HNDL(MY_GPIO_PREFIX,  4000)
#endif
#ifdef MY_GPIO_FPGA_ADDR_5
  GPIO_INT_HNDL(MY_GPIO_PREFIX,  5000)
#endif
#ifdef MY_GPIO_FPGA_ADDR_6
  GPIO_INT_HNDL(MY_GPIO_PREFIX,  6000)
#endif
#ifdef MY_GPIO_FPGA_ADDR_7
  GPIO_INT_HNDL(MY_GPIO_PREFIX,  7000)
#endif
#ifdef MY_GPIO_FPGA_ADDR_8
  GPIO_INT_HNDL(MY_GPIO_PREFIX,  8000)
#endif
#ifdef MY_GPIO_FPGA_ADDR_9
  GPIO_INT_HNDL(MY_GPIO_PREFIX,  9000)
#endif
#ifdef MY_GPIO_FPGA_ADDR_10
  GPIO_INT_HNDL(MY_GPIO_PREFIX, 10000)
#endif

/* EOF */
