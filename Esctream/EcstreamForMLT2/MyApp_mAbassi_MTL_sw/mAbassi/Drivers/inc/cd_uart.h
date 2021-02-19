/* ------------------------------------------------------------------------------------------------ */
/* FILE :		cd_uart.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the Cadence UART															*/
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
/*	$Revision: 1.10 $																				*/
/*	$Date: 2019/01/10 18:07:03 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* See cd_uart.c for NOTE / LIMITATIONS / NOT YET SUPPORTED											*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Supported value for the build options															*/
/*																									*/
/* OS_PLATFORM & 0x00FFFFFF:																		*/
/*					0x007020 : Zynq 7020 (Default if OS_PLATFORM not defined)						*/
/*					0x007753 : UltraScale+ (A53s processors)										*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Default values that depend on (OS_PLATFORM & 0x00FFFFFF):										*/
/* UART_MAX_DEVICES:																				*/
/*					0x7020 : 2																		*/
/*					0x7753 : 2																		*/
/* UART_LIST_DEVICE:																				*/
/*					0x7020 : 3																		*/
/*					0x7753 : 3																		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __CD_UART_H
#define __CD_UART_H

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef UART_MULTI_DRIVER							/* To select if multiple UART drivers are used	*/
  #define UART_MULTI_DRIVER		0					/* in an application. Examples are SocFPGA with */
#endif												/* UART added in the FPGA in extra to processor	*/


#if ((UART_MULTI_DRIVER) == 0)						/* When not mutliple driver, don't prefix the	*/
  #define CD_UART_PREFIX							/* API names									*/
  #define CD_UART_ASCII			""
#else
 #ifndef CD_UART_PREFIX								/* When multiple drivers, use a prefix			*/
  #define CD_UART_PREFIX		 cd_				/* Default prefix, always the same as the file	*/
  #define CD_UART_ASCII			"cd "				/* name prefix to uart.c						*/
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */

#if ((UART_MULTI_DRIVER) == 0)
 #if !defined(UART_MAX_DEVICES)
  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0x7020)
	#define UART_MAX_DEVICES	2					/* Zynq 7020 has 2 UARTs						*/
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x7753)
	#define UART_MAX_DEVICES	2					/* UltraScale+ has 2 UARTs						*/
  #else
	#define UART_MAX_DEVICES	2					/* Undefined platform							*/
  #endif
 #endif

 #ifndef UART_LIST_DEVICE
  #define UART_LIST_DEVICE		((1U<<(UART_MAX_DEVICES))-1U)
 #endif

 #if (((1U<<(UART_MAX_DEVICES))-1U) < (UART_LIST_DEVICES))
  #error "too many devices in UART_LIST_DEVICE vs the max # device defined by UART_MAX_DEVICES"
 #endif

 #undef  CD_UART_MAX_DEVICES
 #define CD_UART_MAX_DEVICES (UART_MAX_DEVICES)

 #undef  CD_UART_LIST_DEVICE
 #define CD_UART_LIST_DEVICE (UART_LIST_DEVICE)

#else
 #if (!defined(CD_UART_LIST_DEVICE) && !defined(CD_UART_MAX_DEVICES))
	#if defined(UART_LIST_DEVICE)
	  #define CD_UART_LIST_DEVICE	(UART_LIST_DEVICE)
	#endif
	#if defined(UART_MAX_DEVICES)
	  #define CD_UART_MAX_DEVICES	(UART_MAX_DEVICES)
	#endif
 #endif

 #if !defined(CD_UART_MAX_DEVICES)
  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0x7020)
	#define CD_UART_MAX_DEVICES	2					/* Zynq 7020 has 2 UARTs						*/
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x7753)
	#define CD_UART_MAX_DEVICES	2					/* UltraScale+ has 2 UARTs						*/
  #else
	#define CD_UART_MAX_DEVICES	2					/* Undefined platform							*/
  #endif
 #endif

 #ifndef CD_UART_LIST_DEVICE
  #define CD_UART_LIST_DEVICE		((1U<<(CD_UART_MAX_DEVICES))-1U)
 #endif

 #if (((1U<<(CD_UART_MAX_DEVICES))-1U) < (CD_UART_LIST_DEVICE))
  #error "too many devices in CD_UART_LIST_DEVICE vs the max # device defined by CD_UART_MAX_DEVICES"
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
/* NOTE: on Xilinx, using either UART_FILT_HW_FLOW_CTS or UART_FILT_HW_FLOW_RTS will enable the		*/
/*       CTS/RTS flow control.																		*/
/*       UART_FILT_HW_FLOW_DTR and UART_FILT_HW_FLOW_DSR are ignored on Xilinx						*/

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

#define   CD_UART_ADD_PREFIX(Token)				 _CD_UART_ADD_PREFIX(CD_UART_PREFIX, Token)
#define  _CD_UART_ADD_PREFIX(Prefix, Token)		__CD_UART_ADD_PREFIX(Prefix, Token)
#define __CD_UART_ADD_PREFIX(Prefix, Token)		Prefix##Token

int CD_UART_ADD_PREFIX(uart_filt) (int Dev, int Enable, int Filter);
int CD_UART_ADD_PREFIX(uart_init) (int Dev, int Baud, int Nbits, int Parity, int Stop,
                                  int RXsize, int TXsize,int Filter);
int CD_UART_ADD_PREFIX(uart_recv) (int Dev, char *Buff, int Len);
int CD_UART_ADD_PREFIX(uart_send) (int Dev, const char *Buff, int Len);

/* ------------------------------------------------------------------------------------------------ */
/* Only make the valid interrupt handlers available to flag user's error							*/

#define  CD_EXT_UART_INT_HNDL(Prefix, Device) _CD_EXT_UART_INT_HNDL(Prefix, Device)
#define _CD_EXT_UART_INT_HNDL(Prefix, Device) extern void Prefix##UARTintHndl_##Device(void)

#if (((CD_UART_LIST_DEVICE) & 0x001) != 0)
 CD_EXT_UART_INT_HNDL(CD_UART_PREFIX, 0);
#endif
#if (((CD_UART_LIST_DEVICE) & 0x002) != 0)
  CD_EXT_UART_INT_HNDL(CD_UART_PREFIX, 1);
#endif
#if (((CD_UART_LIST_DEVICE) & 0x004) != 0)
  CD_EXT_UART_INT_HNDL(CD_UART_PREFIX, 2);
#endif
#if (((CD_UART_LIST_DEVICE) & 0x008) != 0)
  CD_EXT_UART_INT_HNDL(CD_UART_PREFIX, 3);
#endif
#if (((CD_UART_LIST_DEVICE) & 0x010) != 0)
  CD_EXT_UART_INT_HNDL(CD_UART_PREFIX, 4);
#endif
#if (((CD_UART_LIST_DEVICE) & 0x020) != 0)
  CD_EXT_UART_INT_HNDL(CD_UART_PREFIX, 5);
#endif
#if (((CD_UART_LIST_DEVICE) & 0x040) != 0)
  CD_EXT_UART_INT_HNDL(CD_UART_PREFIX, 6);
#endif
#if (((CD_UART_LIST_DEVICE) & 0x080) != 0)
  CD_EXT_UART_INT_HNDL(CD_UART_PREFIX, 7);
#endif
#if (((CD_UART_LIST_DEVICE) & 0x100) != 0)
  CD_EXT_UART_INT_HNDL(CD_UART_PREFIX, 8);
#endif
#if (((CD_UART_LIST_DEVICE) & 0x200) != 0)
  CD_EXT_UART_INT_HNDL(CD_UART_PREFIX, 9);
#endif

/* ------------------------------------------------------------------------------------------------ */

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
