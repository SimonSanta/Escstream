/* ------------------------------------------------------------------------------------------------ */
/* FILE :		dw_uart.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the Designware implementation of the "standard" NS 16550 UART.			*/
/*				IMPORTANT: The interrupt controller MUST be set to level triggering when using		*/
/*				           the interrupts (uart_init() with TXsize != 0 or RXsize != 0)				*/
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
/*	$Revision: 1.11 $																				*/
/*	$Date: 2019/01/10 18:07:03 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* See dw_uart.c for NOTE / LIMITATIONS / NOT YET SUPPORTED											*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Supported value for the build options															*/
/* (OS_PLATFORM & 0x00FFFFFF):																		*/
/*					0x00AAC5 : Altera Cyclone V and Arria 5											*/
/*					0x00AA10 : ALtera Arria 10														*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Default values that depend on (OS_PLATFORM & 0x00FFFFFF):										*/
/* UART_MAX_DEVICES:																				*/
/*					0xAAC5 : 2																		*/
/*					0xAA10 : 2																		*/
/* UART_LIST_DEVICE:																				*/
/*					0xAAC5 : 0x03																	*/
/*					0xAA10 : 0x03																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __DW_UART_H
#define __DW_UART_H

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef UART_MULTI_DRIVER							/* To select if multiple UART drivers are used	*/
  #define UART_MULTI_DRIVER		0					/* in an application. Examples are SocFPGA with */
#endif												/* UART added in the FPGA in extra to processor	*/

#if ((UART_MULTI_DRIVER) == 0)						/* When not mutliple driver, don't prefix the	*/
  #define DW_UART_PREFIX							/* API names									*/
  #define DW_UART_ASCII			""
#else
 #ifndef DW_UART_PREFIX								/* When multiple drivers, use a prefix			*/
  #define DW_UART_PREFIX		 dw_				/* Default prefix, always the same as the file	*/
  #define DW_UART_ASCII			"dw "				/* name prefix to uart.c						*/
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */

#if ((UART_MULTI_DRIVER) == 0)
 #if !defined(UART_MAX_DEVICES)
  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0xAAC5)
	#define UART_MAX_DEVICES	2					/* Arria V / Cyclone V has a total of 2 UARTs	*/
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0xAA10)
	#define UART_MAX_DEVICES	2					/* Arria 10 has a total of 2 UARTs				*/
  #else
	#define UART_MAX_DEVICES	2					/* Undefined platform							*/
  #endif
 #endif

 #ifndef UART_LIST_DEVICE
  #define UART_LIST_DEVICE		((1U<<(UART_MAX_DEVICES))-1U)
 #endif

 #if (((1U<<(UART_MAX_DEVICES))-1U) < (UART_LIST_DEVICE))
  #error "too many devices in UART_LIST_DEVICE vs the max # device defined by UART_MAX_DEVICES"
 #endif

 #undef  DW_UART_MAX_DEVICES
 #define DW_UART_MAX_DEVICES (UART_MAX_DEVICES)

 #undef  DW_UART_LIST_DEVICE
 #define DW_UART_LIST_DEVICE (UART_LIST_DEVICE)

#else
 #if (!defined(DW_UART_LIST_DEVICE) && !defined(DW_UART_MAX_DEVICES))
	#if defined(UART_LIST_DEVICE)
	  #define DW_UART_LIST_DEVICE	(UART_LIST_DEVICE)
	#endif
	#if defined(UART_MAX_DEVICES)
	  #define DW_UART_MAX_DEVICES	(UART_MAX_DEVICES)
	#endif
 #endif

 #if !defined(DW_UART_MAX_DEVICES)
  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0xAAC5)
	#define DW_UART_MAX_DEVICES	2					/* Arria V / Cyclone V has a total of 2 UARTs	*/
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0xAA10)
	#define DW_UART_MAX_DEVICES	2					/* Arria 10 has a total of 2 UARTs				*/
  #else
	#define DW_UART_MAX_DEVICES	2					/* Undefined platform							*/
  #endif
 #endif

 #ifndef DW_UART_LIST_DEVICE
  #define DW_UART_LIST_DEVICE		((1U<<(DW_UART_MAX_DEVICES))-1U)
 #endif

 #if (((1U<<(DW_UART_MAX_DEVICES))-1U) < (DW_UART_LIST_DEVICE))
  #error "too many devices in DW_UART_LIST_DEVICE vs the max # device defined by DW_UART_MAX_DEVICES"
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Input & output character filter control masks													*/
/*																									*/
/* *** The values assigned are kept the same across all UARTs in case multiple drivers are used		*/
/*     in an application.																			*/
/*																									*/
/*     The EOL and EOF are used when uart_recv() is requested to fill a buffer.  When the EOL or	*/
/*     EOT characters are RXed, uart_recv() returns, not trying to completely fill the buffer.		*/
/*     The EOL and EOF are for the RAW character.													*/
/*     e.g. if UART_FILT_EOL_LF is set and UART_FILT_IN_CR_LF is also set, the LF generated when	*/
/*     CR is RXed will not trigger uart_recv() to return.											*/
/*     Same with UART_FILT_EOL_CRLF being set and UART_FILT_IN_CR_CRLF being also set, when CR is	*/
/*     RXed will not trigger uart_recv() to return, unless it is followed by LF (pre-substitution).	*/
/*     But if UART_FILT_EOL_LF is set and UART_FILT_IN_CR_CRLF is also set, the added LF generated	*/
/*     when CR is RXed WILL trigger uart_recv() to return.											*/
/*     When EOL or EOF are used, the trigger character is NOT dropped (unless it is set to be		*/
/*     dropped with UART_FILT_OUT_LF_DROP or UART_FILT_OUT_LF_DROP)									*/
/*																									*/
/* NOTE: UART_FILT_HW_FLOW_CTS, UART_FILT_HW_FLOW_RTS, UART_FILT_HW_FLOW_DTR,						*/
/*       and UART_FILT_HW_FLOW_DSR are ignored by dw_uart.c											*/

#define UART_ECHO				(1U<<0)				/* Echo input to output							*/
#define UART_FILT_IN_CR_CRLF	(1U<<1)				/* Replace input  CR by CR LF					*/
#define UART_FILT_IN_CR_LF		(1U<<2)				/* Replace input  CR by LF						*/
#define UART_FILT_IN_CR_DROP	(1U<<3)				/* Remove all CR received						*/
#define UART_FILT_IN_LF_CRLF	(1U<<4)				/* Replace input  LF by CR LF					*/
#define UART_FILT_IN_LF_CR		(1U<<5)				/* Replace input  LF by CR						*/
#define UART_FILT_IN_LF_DROP	(1U<<6)				/* Remove all LF received						*/
#define UART_FILT_OUT_CR_CRLF	(1U<<7)				/* Replace output CR by CR LF					*/
#define UART_FILT_OUT_CR_LF		(1U<<8)				/* Replace output CR by LF						*/
#define UART_FILT_OUT_CR_DROP	(1U<<9)				/* Remove all CR to output						*/
#define UART_FILT_OUT_LF_CRLF	(1U<<10)			/* Replace output LF by CR LF					*/
#define UART_FILT_OUT_LF_CR		(1U<<11)			/* Replace output LF by CR						*/
#define UART_FILT_OUT_LF_DROP	(1U<<12)			/* Remove all LF to output						*/
#define UART_ECHO_BS_EXPAND		(1U<<13)			/* When echoing <BS>, send <BS><SPACE><BS>		*/
#define UART_FILT_EOL_LF		(1U<<14)			/* End of line is \n							*/
#define UART_FILT_EOL_CR		(1U<<15)			/* End of line is \r							*/
#define UART_FILT_EOL_CRLF		(1U<<16)			/* End of line is \r\n							*/
#define UART_FILT_EOF_CTRLD		(1U<<17)			/* End of input is CTRL-D						*/
#define UART_FILT_EOF_CTRLZ		(1U<<18)			/* End of input is CTRL-Z						*/
#define UART_FILT_HW_FLOW_CTS	(1U<<19)			/* Using Hardware flow control on CTS			*/
#define UART_FILT_HW_FLOW_RTS	(1U<<20)			/* Using Hardware flow control on RTS			*/
#define UART_FILT_HW_FLOW_DTR	(1U<<21)			/* Using Hardware flow control on DTR			*/
#define UART_FILT_HW_FLOW_DSR	(1U<<22)			/* Using Hardware flow control on DSR			*/

/* ------------------------------------------------------------------------------------------------ */

#define   DW_UART_ADD_PREFIX(Token)				 _DW_UART_ADD_PREFIX(DW_UART_PREFIX, Token)
#define  _DW_UART_ADD_PREFIX(Prefix, Token)		__DW_UART_ADD_PREFIX(Prefix, Token)
#define __DW_UART_ADD_PREFIX(Prefix, Token)		Prefix##Token

int DW_UART_ADD_PREFIX(uart_filt) (int Dev, int Enable, int Filter);
int DW_UART_ADD_PREFIX(uart_init) (int Dev, int Baud, int Nbits, int Parity, int Stop,
                                  int RXsize, int TXsize,int Filter);
int DW_UART_ADD_PREFIX(uart_recv) (int Dev, char *Buff, int Len);
int DW_UART_ADD_PREFIX(uart_send) (int Dev, const char *Buff, int Len);

/* ------------------------------------------------------------------------------------------------ */
/* Only make the valid interrupt handlers available to flag user's error							*/

#define  DW_EXT_UART_INT_HNDL(Prefix, Device) _DW_EXT_UART_INT_HNDL(Prefix, Device)
#define _DW_EXT_UART_INT_HNDL(Prefix, Device) extern void Prefix##UARTintHndl_##Device(void)

#if (((DW_UART_LIST_DEVICE) & 0x001) != 0)
 DW_EXT_UART_INT_HNDL(DW_UART_PREFIX, 0);
#endif
#if (((DW_UART_LIST_DEVICE) & 0x002) != 0)
  DW_EXT_UART_INT_HNDL(DW_UART_PREFIX, 1);
#endif
#if (((DW_UART_LIST_DEVICE) & 0x004) != 0)
  DW_EXT_UART_INT_HNDL(DW_UART_PREFIX, 2);
#endif
#if (((DW_UART_LIST_DEVICE) & 0x008) != 0)
  DW_EXT_UART_INT_HNDL(DW_UART_PREFIX, 3);
#endif
#if (((DW_UART_LIST_DEVICE) & 0x010) != 0)
  DW_EXT_UART_INT_HNDL(DW_UART_PREFIX, 4);
#endif
#if (((DW_UART_LIST_DEVICE) & 0x020) != 0)
  DW_EXT_UART_INT_HNDL(DW_UART_PREFIX, 5);
#endif
#if (((DW_UART_LIST_DEVICE) & 0x040) != 0)
  DW_EXT_UART_INT_HNDL(DW_UART_PREFIX, 6);
#endif
#if (((DW_UART_LIST_DEVICE) & 0x080) != 0)
  DW_EXT_UART_INT_HNDL(DW_UART_PREFIX, 7);
#endif
#if (((DW_UART_LIST_DEVICE) & 0x100) != 0)
  DW_EXT_UART_INT_HNDL(DW_UART_PREFIX, 8);
#endif
#if (((DW_UART_LIST_DEVICE) & 0x200) != 0)
  DW_EXT_UART_INT_HNDL(DW_UART_PREFIX, 9);
#endif

/* ------------------------------------------------------------------------------------------------ */

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
