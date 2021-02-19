/* ------------------------------------------------------------------------------------------------ */
/* FILE :		Shell.h																				*/
/*																									*/
/* CONTENTS :																						*/
/*				Incldue for thr RTOS debug/monitoring shell											*/
/*																									*/
/*																									*/
/* Copyright (c) 2017-2019, Code-Time Technologies Inc. All rights reserved.						*/
/*																									*/
/* Code-Time Technologies retains all right, title, and interest in and to this work				*/
/*																									*/
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS							*/
/* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF										*/
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL							*/
/* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR								*/
/* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,							*/
/* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR							*/
/* OTHER DEALINGS IN THE SOFTWARE.																	*/
/*																									*/
/*																									*/
/*  $Revision: 1.5 $																				*/
/*	$Date: 2019/01/10 18:06:19 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __SHELL_H__								/* To protect against re-include					*/
#define __SHELL_H__


#ifdef __cplusplus
  extern "C" {
#endif

#if defined(_UABASSI_)							 	/* It is the same include file in all cases.	*/
  #include "mAbassi.h"								/* There is a substitution during the release 	*/
#elif defined (OS_N_CORE)							/* This file is the same for Abassi, mAbassi,	*/
  #include "mAbassi.h"								/* and uAbassi, so Code Time uses these checks	*/
#else												/* to not have to keep 3 quasi identical		*/
  #include "mAbassi.h"								/* copies of this file							*/
#endif

#include "SysCall.h"

/* ------------------------------------------------------------------------------------------------ */

extern void OSshell(void);
extern void ShellGets(char *Str, int Len);
extern int  ShellRdWrt(int *Result);
extern int  SubShell(int argc, char *argv[]);

/* ------------------------------------------------------------------------------------------------ */

#ifdef __cplusplus
  }
#endif

#endif											/* The whole file is conditional					*/

/* EOF */
