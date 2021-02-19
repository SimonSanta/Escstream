/* ------------------------------------------------------------------------------------------------ */
/* FILE :		arm_acp.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the ARM ACP (Accelerator Coherency Port)									*/
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
/*	$Revision: 1.6 $																				*/
/*	$Date: 2019/01/10 18:07:02 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* NOTES:																							*/
/*			The ACP set-up from the processor side does not seem to be standardized.				*/
/*			Therefore for now, the API is target platform specific.									*/
/*																									*/
/*			On GGC, you should set the -finline-functions option on the compiler command line		*/
/*			otherwise on optimization diffrent from -o3, the linker will complain it can't find		*/
/*			acp_enable().																			*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __ARM_ACP_H__							/* To protect against re-include					*/
#define __ARM_ACP_H__

#include <stdint.h>

#ifdef __cplusplus
  extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------ */

#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)
  #define ACP_ADDR_MASK		0x3FFFFFFF
#else
  #define ACP_ADDR_MASK		0xFFFFFFFF
#endif

/* ------------------------------------------------------------------------------------------------ */

uint32_t acp_enable(int VID, int MID, int Page, int IsRead);
	
#ifdef __cplusplus
  }
#endif

#endif											/* The whole file is conditional					*/


/* EOF */
