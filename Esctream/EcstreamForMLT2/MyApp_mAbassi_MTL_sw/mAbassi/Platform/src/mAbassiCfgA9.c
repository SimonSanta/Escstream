/* ------------------------------------------------------------------------------------------------ */
/* FILE :		mAbassiCfg.c																		*/
/*																									*/
/* CONTENTS :																						*/
/*				Configuration file for mAbassi for packages with libraries							*/
/*																									*/
/*																									*/
/* Copyright (c) 2014-2019, Code-Time Technologies Inc. All rights reserved.						*/
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
/*	$Revision: 1.13 $																				*/
/*	$Date: 2019/01/10 18:07:12 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "mAbassi.h"

/* ------------------------------------------------------------------------------------------------ */
/* This macro is used to define the stack size and the stack memory itslef as needed by mAbassi		*/
/* start-up code.																					*/
/* The macro arguments are:																			*/
/* Mode : Stack for the indicated processor "Mode".  All following modes must be defined:			*/
/*			        	ABORT, FIQ, IRQ, SUPER, UNDEF												*/
/* Size : Size in bytes of the stack allocated for each core.										*/
/*        It is not possible to set different stack sizes between the cores.						*/
/* ------------------------------------------------------------------------------------------------ */

#define DEF_MODE_STACK_SIZE(Mode, Size)																\
				const int G_ ## Mode ## stackSize = (OX_N_CORE) * (Size);							\
				OSalign_t G_ ## Mode ## stack      [(OX_N_CORE) * ((Size)/sizeof(OSalign_t))]

/* ------------------------------------------------------------------------------------------------ */

DEF_MODE_STACK_SIZE(ABORT, 1024);					/* Abort mode stack defintions					*/
DEF_MODE_STACK_SIZE(FIQ,   1024);					/* FIQ mode stack defintions					*/
DEF_MODE_STACK_SIZE(IRQ,   16384);					/* IRQ mode stack defintions					*/
DEF_MODE_STACK_SIZE(SUPER, 16384);					/* Supervisor mode stack defintions				*/
DEF_MODE_STACK_SIZE(UNDEF, 1024);					/* Undefined mode stack defintions				*/

#if ((OS_PLATFORM) != 0xFEE6)						/* iMX6 is using hard coded MMU tables			*/
  #if (((OS_DEMO) != 50) && ((OS_DEMO) != -50))		/* Demo #50 defines the MMU tables				*/
													/* MMU table definitions. See documentation		*/
/*                                          LENGTH      BASE ADDR									*/


#ifndef MYAPP_MTL
   const unsigned int G_MMUsharedTbl[]    = { 0x40000000, 0x00000000,
                                              0
                                            };
   const unsigned int G_MMUnonCachedTbl[] = { 0xC0000000, 0x40000000,
                                              0
                                            };

#else
   const unsigned int G_MMUsharedTbl[]    = { 0x20000000, 0x00000000, 0 };
   const unsigned int G_MMUnonCachedTbl[] = { 0xE0000000, 0x20000000, 0 };
#endif

/*											LENGTH      VIRT ADDR    PHYS ADDR						*/
const unsigned int G_MMUprivateTbl[]   = { 0,
                                           0,
                                           0,
                                           0				/* 4 zeros (none) when is 4 cores		*/
                                         };					/* Superfluous 2 zeroes when is 2 cores	*/
const unsigned int G_MMUnonCprivTbl[]  = { 0,
                                           0,
                                           0,
                                           0				/* 4 zeros (none) when is 4 cores		*/
                                         };					/* Superfluous 2 zeroes when is 2 cores	*/
  #endif
#endif

/* EOF */
