/* ------------------------------------------------------------------------------------------------ */
/* File :		MediaIF.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Media interface common to all file system stacks									*/
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
/*	$Date: 2019/01/10 18:07:01 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __MEDIAIF_H
#define __MEDIAIF_H

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

int32_t     MEDIAblksize (int Drv);
const char *MEDIAinfo    (int Drv);
int         MEDIAinit    (const char *FSname, int Drv);
int         MEDIAread    (const char *FSname, int Drv, uint8_t *Buff, uint32_t Sect, int Nsect);
int32_t     MEDIAsectsz  (int Drv, int Size);
int64_t     MEDIAsize    (const char *FSname, int drv);
int         MEDIAstatus  (const char *FSname, int Drv);
int         MEDIAwrite   (const char *FSname, int Drv, const uint8_t *Buff, uint32_t Sect, int Nsect);

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
