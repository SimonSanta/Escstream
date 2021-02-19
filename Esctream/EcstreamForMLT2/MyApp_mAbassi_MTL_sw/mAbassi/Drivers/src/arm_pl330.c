/* ------------------------------------------------------------------------------------------------ */
/* FILE :		arm_pl330.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the ARM PL330 DMA controller												*/
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
/*	$Revision: 1.15 $																				*/
/*	$Date: 2019/01/10 18:07:05 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* NOTES:																							*/
/*																									*/
/*		Build options not described in the documentation:											*/
/*		- DMA_PGM_SIZE																				*/
/*			The PL330 is basically a small programmable engine and uses a real program. This build	*/
/*			defines how many bytes are allocated to each channel of the PL330						*/
/*		- DMA_NMB_CHAN																				*/
/*			The PL330 is an 8 channel DMA engine.  By default DMA_NMB_CHAN is set to 8 but the		*/
/*			maximum number of channel can be reduced for data memory space saving.					*/
/*		- DMA_NMB_EVTIRQ																			*/
/*			Allowa s reduction of the number of evenmt / Interrupt requests.  This reduce a tiny	*/
/*			the memory used by the DMA controller configuration data structure						*/
/*																									*/
/* LIMITATIONS:																						*/
/*																									*/
/* NOT YET SUPPORTED:																				*/
/*		2D memory to memory transfer not yet implemented. This is when IncSrc != DataSize and/or	*/
/*		IncDst != DataSize.																			*/
/*		The API supports multiple PL330 modules but the code does not yet handle cross-device		*/
/*		event triggering.																			*/
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

#include "arm_pl330.h"
#include <stdarg.h>

#ifndef DMA_USE_MUTEX								/* If a mutex is used to protect accesses		*/
  #define DMA_USE_MUTEX			1					/* != 0 : Use mutex								*/
#endif												/* == 0 : ISR disable + spinlock for > 2 cores	*/

#ifndef DMA_OPERATION								
  #define DMA_OPERATION			0x01				/* Bit #0: allow the use of ISR for EOT when 1	*/
#endif

#ifndef DMA_PGM_SIZE
  #define DMA_PGM_SIZE			2048				/* 256 is sufficient for 64M memory transfers	*/
#endif												/* N times 256 for N times 64M transfers		*/

#ifndef DMA_MULTICORE_ISR							/* When operating on a multicore, set to != 0	*/
  #define DMA_MULTICORE_ISR		1					/* if more than 1 core process the interrupt	*/
#endif												/* Enable by default as it's safer				*/

#ifndef DMA_ARG_CHECK								/* If checking validity of function arguments	*/
  #define DMA_ARG_CHECK			1
#endif

#ifndef DMA_DEBUG									/* == 0 no debug information is printed			*/
  #define DMA_DEBUG				0					/* == 1 print only initialization information	*/
#endif												/* >= 2 print all information					*/

#ifndef DMA_NMB_CHAN								/* In the rare case it is desired to reduce		*/
  #define DMA_NMB_CHAN			8					/* the number of channels the DMA has/uses		*/
#endif

#ifndef DMA_NMB_EVTIRQ								/* In the rare case it is desired to reduce		*/
  #define DMA_NMB_EVTIRQ		32					/* the number of events/irqs the DMA has/uses	*/
#endif

#ifndef DMA_NMB_PEND								/* When using DMA_CFG_TRG_PEND, total number	*/
  #define DMA_NMB_PEND			3					/* of trigger that can be pending defore being	*/
#endif												/* set later on									*/

/* ------------------------------------------------------------------------------------------------ */
/* Base addresses of the HW DMA modules																*/

#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)		/* Arria V / Cyclone V							*/
													/* Secure mode set of registers					*/
  static volatile uint32_t * const g_HWaddr[1]  = { (volatile uint32_t *)0xFFE01000
                                                  };
  #define DMA_RESET(Dev)		do {																\
									*((volatile uint32_t *)0xFFD05014) |=  (1U << 28);				\
									*((volatile uint32_t *)0xFFD05014) &= ~(1U << 28);				\
									*((volatile uint32_t *)0xFFD05018) |=  (0xFFU);					\
									*((volatile uint32_t *)0xFFD05018) &= ~(0xFFU);					\
								} while(0)

  #ifndef DMA_CY5_CAN_FPGA_4						/* If using CAN or FPGA for peripheral #4		*/
    #define DMA_CY5_CAN_FPGA_4		0				/* ==0 is CAN / !=0 is FPGA						*/
  #endif
  #ifndef DMA_CY5_CAN_FPGA_5						/* If using CAN or FPGA for peripheral #5		*/
    #define DMA_CY5_CAN_FPGA_5		0				/* ==0 is CAN / !=0 is FPGA						*/
  #endif
  #ifndef DMA_CY5_CAN_FPGA_6						/* If using CAN or FPGA for peripheral #6		*/
    #define DMA_CY5_CAN_FPGA_6		0				/* ==0 is CAN / !=0 is FPGA						*/
  #endif
  #ifndef DMA_CY5_CAN_FPGA_7						/* If using CAN or FPGA for peripheral #7		*/
    #define DMA_CY5_CAN_FPGA_7		0				/* ==0 is CAN / !=0 is FPGA						*/
  #endif
  #ifndef OX_N_CORE
    #ifndef OS_N_CORE
	  #define OX_N_CORE				2
    #else
	  #define OX_N_CORE				(OS_N_CORE)
    #endif
  #endif

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)	/* Arria 10										*/
													/* Secure mode set of registers					*/
  static volatile uint32_t * const g_HWaddr[1]  = { (volatile uint32_t *)0xFFDA1000
                                                  };
  #define DMA_RESET(Dev)		do {																\
									*((volatile uint32_t *)0xFFD05014) |=  (1U << 28);				\
									*((volatile uint32_t *)0xFFD05014) &= ~(1U << 28);				\
									*((volatile uint32_t *)0xFFD05018) |=  (0xFFU);					\
									*((volatile uint32_t *)0xFFD05018) &= ~(0xFFU);					\
								} while(0)

  #ifndef DMA_AR10_DMASEC_FPGA_5					/* If using DMA SEC or FPGA for peripheral #5	*/
    #define DMA_AR10_DMASEC_EMAC_FPGA_5		0		/* ==0 is I2C-EMAC / !=0 is FPGA				*/
  #endif
  #ifndef DMA_AR10_I2C4TX_FPGA_6					/* If using I2C/EMAC or FPGA for peripheral #6	*/
    #define DMA_AR10_I2C4TX_EMAC_FPGA_6		0		/* ==0 is I2C-EMAC / !=0 is FPGA				*/
  #endif
  #ifndef DMA_AR10_I2C4RX_FPGA_7					/* If using I2C/EMAC or FPGA for peripheral #7	*/
    #define DMA_AR10_I2C4RX_FPGA_7			0		/* ==0 is I2C-EMAC / !=0 is FPGA				*/
  #endif
  #ifndef OX_N_CORE
    #ifndef OS_N_CORE
	  #define OX_N_CORE						2
    #else
	  #define OX_N_CORE						(OS_N_CORE)
    #endif
  #endif

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x007020)	/* Zynq 7020									*/
													/* Secure mode set of registers					*/
  static volatile uint32_t * const g_HWaddr[1]  = { (volatile uint32_t *)0xF8003000
                                                  };
  #define DMA_RESET(Dev)		do {																\
									*((volatile uint32_t *)0xF800020C) = 0;							\
									*((volatile uint32_t *)0xF800020C) = 1;							\
								} while(0)
  #ifndef OX_N_CORE
    #ifndef OS_N_CORE
	  #define OX_N_CORE						2
    #else
	  #define OX_N_CORE						(OS_N_CORE)
    #endif
  #endif

#else
  	#error "Unsupported platform specified by OS_PLATFORM"

#endif

/* ------------------------------------------------------------------------------------------------ */
/* This section uses a lots of macros but it's to reduce the size of the data requirements by only	*/
/* creating descriptor / mutexes etc for the devices in use ("1" in DMA_LIST_DEVICE)				*/

#ifndef DMA_MAX_DEVICES
 #if   (((DMA_LIST_DEVICE) & 0x200) != 0U)
  #define DMA_MAX_DEVICES	10
 #elif (((DMA_LIST_DEVICE) & 0x100) != 0U)
  #define DMA_MAX_DEVICES	9
 #elif (((DMA_LIST_DEVICE) & 0x080) != 0U)
  #define DMA_MAX_DEVICES	8
 #elif (((DMA_LIST_DEVICE) & 0x040) != 0U)
  #define DMA_MAX_DEVICES	7
 #elif (((DMA_LIST_DEVICE) & 0x020) != 0U)
  #define DMA_MAX_DEVICES	6
 #elif (((DMA_LIST_DEVICE) & 0x010) != 0U)
  #define DMA_MAX_DEVICES	5
 #elif (((DMA_LIST_DEVICE) & 0x008) != 0U)
  #define DMA_MAX_DEVICES	4
 #elif (((DMA_LIST_DEVICE) & 0x004) != 0U)
  #define DMA_MAX_DEVICES	3
 #elif (((DMA_LIST_DEVICE) & 0x002) != 0U)
  #define DMA_MAX_DEVICES	2
 #elif (((DMA_LIST_DEVICE) & 0x001) != 0U)
  #define DMA_MAX_DEVICES	1
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* This section figures out how to remap the individual devices specified by DMA_LIST_DEVICE		*/
/* e.g. if DMA_LIST_DEVICE == 0x12, the only descriptors/mutexes for 2 devices are required and		*/
/*      device#1 is remapped to [0] and device #4 is remapped to [1]								*/

#if (((DMA_LIST_DEVICE) & 0x001) != 0)
  #define DMA_CNT_0	0
  #define DMA_IDX_0	0
#else
  #define DMA_CNT_0	(-1)
  #define DMA_IDX_0	(-1)
#endif
#if (((DMA_LIST_DEVICE) & 0x002) != 0)
  #define DMA_CNT_1	((DMA_CNT_0) + 1)
  #define DMA_IDX_1	((DMA_CNT_0) + 1)
#else
  #define DMA_CNT_1	(DMA_CNT_0)
  #define DMA_IDX_1	-1
#endif
#if (((DMA_LIST_DEVICE) & 0x004) != 0)
  #define DMA_CNT_2	((DMA_CNT_1) + 1)
  #define DMA_IDX_2	((DMA_CNT_1) + 1)
#else
  #define DMA_CNT_2	(DMA_CNT_1)
  #define DMA_IDX_2	-1
#endif
#if (((DMA_LIST_DEVICE) & 0x008) != 0)
  #define DMA_CNT_3	((DMA_CNT_2) + 1)
  #define DMA_IDX_3	((DMA_CNT_2) + 1)
#else
  #define DMA_CNT_3	(DMA_CNT_2)
  #define DMA_IDX_3	-1
#endif
#if (((DMA_LIST_DEVICE) & 0x010) != 0)
  #define DMA_CNT_4	((DMA_CNT_3) + 1)
  #define DMA_IDX_4	((DMA_CNT_3) + 1)
#else
  #define DMA_CNT_4	(DMA_CNT_3)
  #define DMA_IDX_4	-1
#endif
#if (((DMA_LIST_DEVICE) & 0x020) != 0)
  #define DMA_CNT_5	((DMA_CNT_4) + 1)
  #define DMA_IDX_5	((DMA_CNT_4) + 1)
#else
  #define DMA_CNT_5	(DMA_CNT_4)
  #define DMA_IDX_5	-1
#endif
#if (((DMA_LIST_DEVICE) & 0x040) != 0)
  #define DMA_CNT_6	((DMA_CNT_5) + 1)
  #define DMA_IDX_6	((DMA_CNT_5) + 1)
#else
  #define DMA_CNT_6	(DMA_CNT_5)
  #define DMA_IDX_6	-1
#endif
#if (((DMA_LIST_DEVICE) & 0x080) != 0)
  #define DMA_CNT_7	((DMA_CNT_6) + 1)
  #define DMA_IDX_7	((DMA_CNT_6) + 1)
#else
  #define DMA_CNT_7	(DMA_CNT_6)
  #define DMA_IDX_7	-1
#endif
#if (((DMA_LIST_DEVICE) & 0x100) != 0)
  #define DMA_CNT_8	((DMA_CNT_7) + 1)
  #define DMA_IDX_8	((DMA_CNT_7) + 1)
#else
  #define DMA_CNT_8	(DMA_CNT_7)
  #define DMA_IDX_8	-1
#endif
#if (((DMA_LIST_DEVICE) & 0x200) != 0)
  #define DMA_CNT_9	((DMA_CNT_8) + 1)
  #define DMA_IDX_9	((DMA_CNT_8) + 1)
#else
  #define DMA_CNT_9	(DMA_CNT_8)
  #define DMA_IDX_9	-1
#endif

#define DMA_NMB_DEVICES	((DMA_CNT_9)+1)

/* ------------------------------------------------------------------------------------------------ */
/* Remapping table:																					*/

static const int g_DevReMap[] = { DMA_IDX_0
                               #if ((DMA_MAX_DEVICES) > 1)
                                 ,DMA_IDX_1
                               #endif 
                               #if ((DMA_MAX_DEVICES) > 2)
                                 ,DMA_IDX_2
                               #endif 
                               #if ((DMA_MAX_DEVICES) > 3)
                                 ,DMA_IDX_3
                               #endif 
                               #if ((DMA_MAX_DEVICES) > 4)
                                 ,DMA_IDX_4
                               #endif 
                               #if ((DMA_MAX_DEVICES) > 5)
                                 ,DMA_IDX_5
                               #endif 
                               #if ((DMA_MAX_DEVICES) > 6)
                                 ,DMA_IDX_6
                               #endif 
                               #if ((DMA_MAX_DEVICES) > 7)
                                 ,DMA_IDX_7
                               #endif 
                               #if ((DMA_MAX_DEVICES) > 8)
                                 ,DMA_IDX_8
                               #endif 
                               #if ((DMA_MAX_DEVICES) > 9)
                                 ,DMA_IDX_9
                               #endif
                               };

/* ------------------------------------------------------------------------------------------------ */
#if ((DMA_USE_MUTEX) > 0)
static const char g_DmaNames[][5] = {
                                  "DMA0"
                                #if ((DMA_MAX_DEVICES) > 1)
                                  ,"DMA1"
                                #endif 
                                #if ((DMA_MAX_DEVICES) > 2)
                                  ,"DMA2"
                                #endif 
                                #if ((DMA_MAX_DEVICES) > 3)
                                  ,"DMA3"
                                #endif 
                                #if ((DMA_MAX_DEVICES) > 4)
                                  ,"DMA4"
                                #endif 
                                #if ((DMA_MAX_DEVICES) > 5)
                                  ,"DMA5"
                                #endif 
                                #if ((DMA_MAX_DEVICES) > 6)
                                  ,"DMA6"
                                #endif 
                                #if ((DMA_MAX_DEVICES) > 7)
                                  ,"DMA7"
                                #endif 
                                #if ((DMA_MAX_DEVICES) > 8)
                                  ,"DMA8"
                                #endif 
                                #if ((DMA_MAX_DEVICES) > 9)
                                  ,"DMA9"
                                #endif
                                };
#endif
static const char g_SemNames[][DMA_NMB_CHAN][7] = {
                                 {"DMA0-0"
                                #if ((DMA_NMB_CHAN) > 1)
                                  ,"DMA0-1"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 2)
                                  ,"DMA0-2"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 3)
                                  ,"DMA0-3"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 4)
                                  ,"DMA0-4"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 5)
                                  ,"DMA0-5"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 6)
                                  ,"DMA0-6"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 7)
                                  ,"DMA0-7"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 8)
                                  ,"DMA0-8"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 9)
                                  ,"DMA0-9"
                                #endif
                                }
                              #if ((DMA_MAX_DEVICES) > 1)
                                 ,{"DMA1-0"
                                #if ((DMA_NMB_CHAN) > 1)
                                  ,"DMA1-1"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 2)
                                  ,"DMA1-2"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 3)
                                  ,"DMA1-3"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 4)
                                  ,"DMA1-4"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 5)
                                  ,"DMA1-5"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 6)
                                  ,"DMA1-6"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 7)
                                  ,"DMA1-7"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 8)
                                  ,"DMA1-8"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 9)
                                  ,"DMA1-9"
                                #endif
                                }
                              #endif
                              #if ((DMA_MAX_DEVICES) > 2)
                                 ,{"DMA2-0"
                                #if ((DMA_NMB_CHAN) > 1)
                                  ,"DMA0-1"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 2)
                                  ,"DMA2-2"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 3)
                                  ,"DMA2-3"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 4)
                                  ,"DMA2-4"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 5)
                                  ,"DMA2-5"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 6)
                                  ,"DMA2-6"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 7)
                                  ,"DMA2-7"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 8)
                                  ,"DMA2-8"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 9)
                                  ,"DMA2-9"
                                #endif
                                }
                              #endif
                              #if ((DMA_MAX_DEVICES) > 3)
                                 ,{"DMA3-0"
                                #if ((DMA_NMB_CHAN) > 1)
                                  ,"DMA3-1"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 2)
                                  ,"DMA3-2"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 3)
                                  ,"DMA3-3"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 4)
                                  ,"DMA3-4"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 5)
                                  ,"DMA3-5"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 6)
                                  ,"DMA3-6"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 7)
                                  ,"DMA3-7"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 8)
                                  ,"DMA3-8"
                                #endif 
                                #if ((DMA_NMB_CHAN) > 9)
                                  ,"DMA3-9"
                                #endif
                                }
                              #endif
                               };

/* ------------------------------------------------------------------------------------------------ */
/* Checks and some definitions																		*/

#if ((DMA_LIST_DEVICE) == 0U)
  #error "DMA_LIST_DEVICE cannot be zero"
#endif

#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)
  #if (((DMA_NMB_DEVICES) > 1)  || ((DMA_MAX_DEVICES) > 1))
	#error "Too many DMA devices for Arria V / Cyclone V:"
    #error "      Set DMA_MAX_DEVICES <= 1 and/or DMA_LIST_DEVICE <= 1"
  #endif
#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)
  #if (((DMA_NMB_DEVICES) > 1)  || ((DMA_MAX_DEVICES) > 1))
	#error "Too many DMA devices for Arria 10:"
    #error "      Set DMA_MAX_DEVICES <= 1 and/or DMA_LIST_DEVICE <= 0xF"
  #endif
#endif

#if ((DMA_NMB_CHAN) > 8)
  #error "The build option DMA_NMB_CHAN can't be set > 8"
#endif

#if ((DMA_NMB_EVTIRQ) > 32)
  #error "The build option DMA_NMB_EVTIRQ can't be set > 32"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Two types pf lock are used: DMA_LOCK & REG_LOCK & ISR_LOCK.										*/
/* The fisrt is to protect accesses to the dma configuration and the 2nd is for the register		*/
/* accesses.  Register are needed in case a callback function in the ISR uses the dma API.			*/
/* Last one is only used in the ISR handler															*/

#if ((DMA_USE_MUTEX) > 0)
  #define DMA_LOCK(Cfg, State)			do {														\
											if (COREisInISR() == 0) {								\
												MTXlock(Cfg->MyMutex, -1);							\
											}														\
										} while(0)

  #define DMA_UNLOCK(Cfg, State)		do {														\
											if (COREisInISR() == 0) {								\
												MTXunlock(Cfg->MyMutex);							\
											}														\
										} while(0)
 #if ((OX_N_CORE) >= 2)
  #define REG_LOCK(Cfg, State)			do {														\
											State = OSintOff();										\
											CORElock(DMA_SPINLOCK, &Cfg->SpinLock, 0,COREgetID()+1);\
										} while(0)

  #define REG_UNLOCK(Cfg, State)		do {														\
											COREunlock(DMA_SPINLOCK, &Cfg->SpinLock, 0);			\
											OSintBack(State);										\
										} while(0)
 #else
  #define REG_LOCK(Cfg, State)			do {														\
											State = OSintOff();										\
										} while(0)

  #define REG_UNLOCK(Cfg, State)		do {														\
											OSintBack(State);										\
										} while(0)
 #endif

#elif ((DMA_USE_MUTEX) < 0)
 #if ((OX_N_CORE) >= 2)
  #define DMA_LOCK(Cfg, State)			do {														\
											State = OSintOff();										\
											CORElock(DMA_SPINLOCK, &Cfg->SpinLock, 0,COREgetID()+1);\
										} while(0)

  #define DMA_UNLOCK(Cfg, State)		do {														\
											COREunlock(DMA_SPINLOCK, &Cfg->SpinLock, 0);			\
											OSintBack(State);										\
										} while(0)
 #else
  #define DMA_LOCK(Cfg, State)			do {														\
											State = OSintOff();										\
										} while(0)

  #define DMA_UNLOCK(Cfg, State)		do {														\
											OSintBack(State);										\
										} while(0)
 #endif
  #define REG_LOCK(Cfg, State)			OX_DO_NOTHING()
  #define REG_UNLOCK(Cfg, State)		OX_DO_NOTHING()

#else
  #define DMA_LOCK(Cfg, State)			OX_DO_NOTHING()
  #define DMA_UNLOCK(Cfg, State)		OX_DO_NOTHING()
  #define REG_LOCK(Cfg, State)			OX_DO_NOTHING()
  #define REG_UNLOCK(Cfg, State)		OX_DO_NOTHING()


  #define REG_LOCK(Cfg, State)			OX_DO_NOTHING()
  #define REG_UNLOCK(Cfg, State)		OX_DO_NOTHING()

#endif

#if ((OX_N_CORE) >= 2)
  #define ISR_LOCK(Cfg, State)			do {														\
											State = OSintOff();										\
											CORElock(DMA_SPINLOCK, &Cfg->SpinLock, 0,COREgetID()+1);\
										} while(0)

  #define ISR_UNLOCK(Cfg, State)		do {														\
											COREunlock(DMA_SPINLOCK, &Cfg->SpinLock, 0);			\
											OSintBack(State);										\
										} while(0)
#else
  #define ISR_LOCK(Cfg, State)			do {														\
											State = OSintOff();										\
										} while(0)

  #define ISR_UNLOCK(Cfg, State)		do {														\
											OSintBack(State);										\
										} while(0)
#endif


#define DMA_ID(Uniq,Dev,Ch,Evt)	(((Evt)&0x3F)|((Ch)<<6)|((Dev)<<9)|(((Uniq)&0x1F)<<11)|0x00C0000)
#define DMA_ID_UNIQ(x)			(((x) >> 11) & 0x1F)
#define DMA_ID_DEV(x)			(((x) >>  9) & 0x03)
#define DMA_ID_CHN(x)			(((x) >>  6) & 0x07)
#define DMA_ID_EVT(x)			(((x) & 0x20) ? 0xFFFFFFFF : (((x) >>  0) & 0x3F))

#if ((DMA_NMB_DEVICES) > 4)
  #error "DMA_NMB_DEVICES is too large for transfer IDs construction"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* PL330 register index defintions																	*/

#define DMA_DSR_REG				(0x000/4)
#define DMA_DPC_REG				(0x004/4)
#define DMA_INTEN_REG			(0x020/4)
#define DMA_INT_EVENT_RIS_REG	(0x024/4)		
#define DMA_INTMIS_REG			(0x028/4)
#define DMA_INTCLR_REG			(0x02C/4)
#define DMA_FSRD_REG			(0x030/4)
#define DMA_FSRC_REG			(0x034/4)
#define DMA_FTRD_REG			(0x038/4)
#define DMA_FTRn_REG(n)			((0x040+( 4*(n)))/4)
#define DMA_CSRn_REG(n)			((0x100+( 8*(n)))/4)
#define DMA_CPCn_REG(n)			((0x104+( 8*(n)))/4)
#define DMA_SARn_REG(n)			((0x400+(32*(n)))/4)	
#define DMA_DARn_REG(n)			((0x404+(32*(n)))/4)	
#define DMA_CCRn_REG(n)			((0x408+(32*(n)))/4)	
#define DMA_LC0n_REG(n)			((0x40C+(32*(n)))/4)	
#define DMA_LC1n_REG(n)			((0x410+(32*(n)))/4)	
#define DMA_DBGSTATUS_REG		(0xD00/4)
#define DMA_DBGCMD_REG			(0xD04/4)
#define DMA_DBGINST0_REG		(0xD08/4)
#define DMA_DBGINST1_REG		(0xD0C/4)
#define DMA_CR0_REG				(0xE00/4)
#define DMA_CR1_REG				(0xE04/4)
#define DMA_CR2_REG				(0xE08/4)
#define DMA_CR3_REG				(0xE0C/4)
#define DMA_CR4_REG				(0xE10/4)
#define DMA_CRDn_REG			(0xE14/4)

/* ------------------------------------------------------------------------------------------------ */
/* DMA Assembler op-code & arguments																*/
/*																									*/
/* NOTE:																							*/
/*       for CCR_DC() & CCR_SC(), the cache setting, only the values of 0 an 7 are used in here		*/
/*       0 is uncached    : used for all I/O accesses												*/
/*       7 is full cached : used for all memory accessesd. If the memory is uncached, it's still OK	*/
/*																									*/
													/* OP-CODES										*/
#define OP_DMAADDH			0x000054				/* Add halfword									*/
#define OP_DMAADNH			0x00005C				/* Add negative halfword						*/
#define OP_DMAEND			0x0000					/* End											*/
#define OP_DMAFLUSHP		0x0035					/* Flush and notify the peripheral				*/
#define OP_DMAGO			0x00A0					/* Go											*/
#define OP_DMALD			0x0004
#define OP_DMALDB			0x0007
#define OP_DMALDS			0x0005
#define OP_DMALDPB			0x0027
#define OP_DMALDPS			0x0025
#define OP_DMALP			0x0020					/* Loop: Loop #0 or #1 selected by assembler	*/
#define OP_DMALPEND			0x0028
#define OP_DMALPENDB		0x002B
#define OP_DMALPENDS		0x0029
#define OP_DMALPFE			0x002C
#define OP_DMAKILL			0x0001
#define OP_DMAMOV			0x00BC
  #define ARG_SAR			0
  #define ARG_CCR			1
  #define ARG_DAR			2
#define OP_DMANOP			0x0018
#define OP_DMARMB			0x0012
#define OP_DMASEV			0x0034
#define OP_DMAST			0x0008
#define OP_DMASTB			0x000B
#define OP_DMASTS			0x000A
#define OP_DMASTPB			0x002B					/* There is a conflict with OP_DMALPENDB		*/
#define OP_DMASTPS			0x0029					/* There is a conflict with OP_DMALPENDS		*/
#define OP_DMASTZ			0x0C
#define OP_DMAWFE			0x0036
  #define ARG_VALID			0
  #define ARG_INVALID		2
#define OP_DMAWFPB			0x0032
#define OP_DMAWFPP			0x0031
#define OP_DMAWFPS			0x0030
#define OP_DMAWMB			0x13
  #define ARG_SAR			0						/* Used in DMAMOV & DMAADDH						*/
  #define ARG_CCR			1
  #define ARG_DAR			2

													/* PSEUDO OP-CODES								*/
#define OP_DCD				0xFFFFFFFF
#define OP_DCB				0xFFFFFFFE

													/* ARGUMENTS for DMAMOV CCR, ...				*/
#define CCR_DAF				(0<<14)
#define CCR_DAI				(1<<14)
#define CCR_DB(x)			(((x)-1)<<18)
#define CCR_DC(x)			((x)<<25)
#define CCR_DP(x)			((x)<<22)
#define CCR_DP_DEF			(0<<22)
#define CCR_DS(x)			(CCR__LOG(x)<<15)
 #define CCR_DS8			(0<<15)
 #define CCR_DS16			(1<<15)
 #define CCR_DS32			(2<<15)
 #define CCR_DS64			(3<<15)
 #define CCR_DS128			(4<<15)
#define CCR_ES(x)			(CCR__LOG(x)<<28)
 #define CCR_ES8			(0<<28)
 #define CCR_ES16			(1<<28)
 #define CCR_ES32			(2<<28)
 #define CCR_ES64			(3<<28)
 #define CCR_ES128			(4<<28)
#define CCR_SAF				(0<<0)
#define CCR_SAI				(1<<0)
#define CCR_SB(x)			(((x)-1)<<4)
#define CCR_SC(x)			((x)<<11)
#define CCR_SP(x)			((x)<<8)
#define CCR_SP_DEF			(0<<8)
#define CCR_SS(x)			(CCR__LOG(x)<<1)
 #define CCR_SS8			(0<<1)
 #define CCR_SS16			(1<<1)
 #define CCR_SS32			(2<<1)
 #define CCR_SS64			(3<<1)
 #define CCR_SS128			(4<<1)
#define CCR__LOG(x)	(((x)>=128)?4:((x)>=64)?3:((x)>=32)?2:((x)/16))

/* ------------------------------------------------------------------------------------------------ */
/* ARCACHE & AWCACHE values information																*/

#if 0

With Abassi cache set-up, only ARCACHE/AWCACHE == 0 or == 7 are needed.
This is why with memory as source/destination the defaults are set 7 and for I/O they are set to 0
When using DMA_CFG_ARCACHE() or DMA_CFG_AWCACHE(), only the 3 LSBits are used
ARCACHE[2:0]:
0: 0 0 0   Non-bufferable, non-cacheable
1: 0 0 1   Bufferable only
2: 0 1 0   Cacheable but do not allocate
3: 0 1 1   Cacheable and bufferable, do not allocate
4: 1 0 0   Invalid: violation of the AXI protocol
5: 1 0 1   Invalid: violation of the AXI protocol
6: 1 1 0   Cacheable write-through, allocate on read
7: 1 1 1   Cacheable write-back, allocate on read

AWCACHE[3,1:0]:
0: 0 0 0   Non-bufferable, non-cacheable
1: 0 0 1   Bufferable only
2: 0 1 0   Cacheable but do not allocate
3: 0 1 1   Cacheable and bufferable, do not allocate
4: 1 0 0   Invalid: violation of the AXI protocol
5: 1 0 1   Invalid: violation of the AXI protocol
6: 1 1 0   Cacheable write-through, allocate on write
7: 1 1 1   Cacheable write-back, allocate on write

#endif

/* ------------------------------------------------------------------------------------------------ */
/* Local typedefs and variables																		*/

typedef struct _DMApgm_t {							/* Used by DMAasm() to crete the DMA program	*/
    uint8_t *BufBase;								/* Aligned on cache line & beginning of program	*/
	uint8_t *BufNow;								/* Next programming location					*/
	int      LoopIdx;								/* Current loop index							*/
	int      LoopNow;								/* Current loop level							*/
    uint8_t *LoopAddr[3];							/* Where to branch back for Loop #0 & #1		*/
	uint32_t LoopCnt[3];							/* If a counter is used or infinite (-1)		*/
	uint32_t LoopOp[3];								/* Opcode that started the loop					*/
    uint8_t  Program[DMA_PGM_SIZE+OX_CACHE_LSIZE];	/* The DMA program itself						*/
} DMApgm_t;

typedef struct {									/* Per channel configuration					*/
	int         OpEnd;								/* Operation to perform upon EOT				*/
	void       *PtrEnd;								/* Pointer for the operation upon EOT			*/
	intptr_t    ValEnd;								/* Value for the operartion upon EOT			*/
	int         TimeOut;							/* Timeout when waiting							*/
	int         Started;							/* Indicates if the transfer was started		*/
	int         WaitEnd;							/* Type of waiting requested through config ops	*/
	int         ChanWait			;				/* Non-semaphore: channel to wait on for EOT	*/
	void       *AddDst;								/* Destination base address & memory size used	*/
	void       *AddDstOri;							/* Destination base address & memory seize used	*/
	uint32_t    DstSize;							/* to invalidate in case branch pred is enable	*/
	const void *AddSrc;								/* Destination base address & memory size used	*/
	uint32_t    SrcSize;							/* to invalidate in case branch pred is enable	*/
	uint8_t    *PendStart[DMA_NMB_PEND];			/* Trigger pending to be set					*/
	uint8_t    *PendEnd[DMA_NMB_PEND];				/* Trigger pending to be set					*/
  #if (((DMA_OPERATION) & 1) != 0)
	SEM_t      *MySema;								/* Semaphore posted upon EOT 					*/
  #endif
   #if (((DMA_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1))
	volatile int ISRonGoing;						/* Inform other cores if one handling the ISR	*/
	int          SpinLock;							/* Spinlock for exclusive access to ISRonGoing	*/
   #endif
	DMApgm_t    Pgm;								/* DMA program descriptor						*/
} CHNcfg_t;

typedef struct {									/* Per device: configuration					*/
	volatile uint32_t *HW;							/* Base address of the PL330 DMA module			*/
	int      ChanCnt;								/* Number of channels the DMA has				*/
	int      PeriphCnt;								/* Number of peripherals the DMA handles		*/
	int      EvtIrqCnt;								/* Number of EVT/IRQ in the PL3330 DMA module	*/
	int      IsInit;								/* If this device has been initialized			*/
    int      DMAunik;								/* Unique # to identify the transfer ID			*/
	int32_t  InUse[DMA_NMB_CHAN];					/* Channel in use (can't use bit field)			*/
	int32_t  ReUse[DMA_NMB_CHAN];					/* If the channel is to be reused				*/
	int32_t  EvtOwn[DMA_NMB_EVTIRQ];				/* Index N is the owner of the event #			*/
	int32_t  EvtSent[DMA_NMB_EVTIRQ];				/* Event # to send  by a channel (for clean-up)	*/
  #if ((DMA_USE_MUTEX) != 0)						/* Mutex to protect access to device cfg		*/
	MTX_t   *MyMutex;
  #endif
  #if ((OX_N_CORE) > 1)
	int      SpinLock;								/* Spinlock for exclusive access in multi-core	*/
  #endif											/* dealing with multiple core handling the ISR	*/
	CHNcfg_t ChCfg[DMA_NMB_CHAN];					/* Individual channel configuration				*/
} DMAcfg_t;

DMAcfg_t g_DMAcfg[DMA_NMB_DEVICES];					/* Device configuration							*/
static int g_CfgIsInit = 0;							/* To track first time an init occurs			*/
#if ((DMA_USE_MUTEX) != 0)
  MTX_t *g_MyMutex = (MTX_t *)NULL;					/* This one only used in dma_init()				*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Local function prototypes																		*/

static void dma_mem_1D(DMApgm_t *Pgm,
                       void *AddDst, const void *AddSrc, uint32_t Size,
			 		   uint32_t CCRarcache, uint32_t CCRawcache);

static int dma_IO     (DMApgm_t *Pgm,
                       void *AddDst, int IncDst, int TypeDst,
                       const void *AddSrc, int IncSrc, int TypeSrc,
                       int DataSize, int BrstLen, uint32_t Nxfer,
					   uint32_t CCRarcache, uint32_t CCRawcache, int Barrier);

static int DMAasm     (struct _DMApgm_t *Pgm, int Op, ...);

static void dma_cleanup(DMAcfg_t *Cfg, int XferID);
static int  dma_kill_X(int XferID, int Reent);
static int  dma_start_X(DMAcfg_t *Cfg, int XferID, int Reent);

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: dma_xfer																				*/
/*																									*/
/* dma_xfer - program a DMA transfer																*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int dma_xfer(int Dev,																		*/
/*		             void *AddDst, int IncDst, int TypeDst,											*/
/*		             const void *AddSrc, int IncSrc, int TypeSrc,									*/
/*		             int DataSize, int BrstLen, unsigned int Nxfer,									*/
/*		             int OpEnd, void *PtrEnd, intptr_t ValEnd,										*/
/*		             uint32_t *CfgOp, int *XferID, int TimeOut);									*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev      : DMA controller number															*/
/*		AddDst   : Base address of the destination													*/
/*		IncDst   : increment of the destination address for each individual transfer (not burst)	*/
/*		TypeDst  : type for the destination															*/
/*		                 < 0 memory																	*/
/*		                 >=0 I/O number																*/
/*		                 == INT_MAX: no event is used to trigger the transfer						*/
/*		AddSrc   : Base address of source															*/
/*		                 NULL is equivalent to always reading zeros									*/
/*		IncSrc   : increment of the source address for each individual transfer (not burst)			*/
/*		TypeSrc  : type for the source																*/
/*		                 < 0 memory																	*/
/*		                 >=0 I/O number																*/
/*		                 == INT_MAX: no event is used to trigger the transfer						*/
/*		DataSize : size in bytes of each individual transfer (not the burst size)					*/
/*		BrstLen  : number of individual transfers grouped to form bursts							*/
/*		Nxfer    : number of individual transfer (not the # bytes, nor the #of bursts)				*/
/*		OpEnd    : Operation to perform when the transfer is completed. One of these:				*/
/*		                     DMA_OPEND_NONE : do nothing											*/
/*		                     DMA_OPEND_FCT  : call a function (PtrEnd) with argument (ValEnd)		*/
/*		                     DMA_OPEND_VAR  : write to a variable (PtrEnd) the value (ValEnd)		*/
/*		                     DMA_OPEND_SEM  : post the semaphire (PtrEnd)							*/
/*		                     DMA_OPEND_MTX  : unlock the mutec (PtrEnd)								*/
/*		                     DMA_OPEND_MBX  : put in  mailbox (PtrEnd) the value (ValEnd) no wait	*/
/*		                     DMA_OPEND_EVT  : set the events (ValEnd) of the task (PtrEnd)			*/
/*		PtrEnd   : pointer to use with the operation specified by OpEnd								*/
/*		ValEnd   : value to use  with the operation specified by OpEnd								*/
/*		CfgOp    : configuration of the operation of the DMA										*/
/*		XFerID   : unique identifier for the transfer												*/
/*		TimeOut  : timeout for end of transfer														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/* ------------------------------------------------------------------------------------------------ */

int dma_xfer(int Dev,
             void *AddDst, int IncDst, int TypeDst,
             const void *AddSrc, int IncSrc, int TypeSrc,
             int DataSize, int BrstLen, int Nxfer,
             int OpEnd, void *PtrEnd, intptr_t ValEnd,
             const uint32_t *CfgOp, int *XferID, int TimeOut)
{
void       *AddDstPhys;								/* Physical address								*/
const void *AddSrcPhys;								/* Physical address								*/
int       Barrier;									/* If using I/O memory barrier					*/
int       CCRarcache;								/* ARCACHE[] to use								*/
int       CCRawcache;								/* AWCACHE[] to use								*/
DMAcfg_t *Cfg;										/* Configuration for the requested DMA device	*/
CHNcfg_t *ChCfg;									/* Configuration for the channel to use			*/
int       Chan;										/* DMA channel to use							*/
int       Chan2Wait;								/* Channe # to wait for EOT						*/
int       Evt;										/* Event number to use							*/
int       Expiry;									/* G_OStimCnt value for time-out				*/
int       ii;										/* General purpose								*/
int       kk;										/* General purpose								*/
const uint32_t *MyCfg;								/* CfgOp or MyCfgNone if CfgOp is NULL			*/
uint32_t  MyCfgNone;
int       MyXferID;									/* Transfer ID for this request					*/
DMApgm_t *Pgm;										/* DMA program descriptor being used			*/
volatile uint32_t *Reg;								/* To access the PL330 registers				*/
int       ReMap;									/* Remapped device index						*/
int       Repeat;									/* If the whole sequence is repeated forever	*/
int       RetVal;									/* Return value									*/
uint32_t  Size;										/* Total number of bytes to transfer			*/
int       Start;									/* If transfer is stated of using dma_start()	*/
int       WaitType;									/* Type of wait: no wait or polling or ISR		*/
#if ((DMA_USE_MUTEX) < 0)
  int     ISRstate;									/* ISR state before didable for exclusive access*/
#endif

	if (XferID != NULL) {
		*XferID = 0;								/* Make sure will return invalid ID if error	*/
	}

  #if ((DMA_ARG_CHECK) == 0)						/* No arg check, remapping without checkings	*/
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_DMAcfg[ReMap];

  #else
	if ((Dev < 0)									/* Check the validity of "Dev"					*/
	||  (Dev >= (DMA_MAX_DEVICES))) {
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_xfer() out of range device");
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];

	if (ReMap < 0) {								/* This is set by DMA_LIST_DEVICE				*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_xfer() cannot remap device");
	  #endif
		return(-2);
	}

	Cfg = &g_DMAcfg[ReMap];							/* This is this device configuration			*/

	if ((g_CfgIsInit == 0)							/* Need to have one init() at least				*/
	||  (0 == Cfg->IsInit)) {						/* This controller was never initialized		*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_xfer() device not initialized");
	  #endif
		return(-3);
	}

	if ((TypeDst >= 0)								/* Can't do IO to IO							*/
	&&  (TypeSrc >= 0)) {
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_xfer() can't do IO to IO transfers");
	  #endif
		return(-4);
	}

	if (((TypeDst < 0) && (IncDst < -65535))		/* Right now, memory to memory can only deal	*/
	||  ((TypeDst < 0) && (IncDst >  65535))		/* contiguous arrays							*/
	||  ((TypeSrc < 0) && (IncSrc < -65535))
	||  ((TypeSrc < 0) && (IncSrc >  65535)) ) {
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_xfer() memory to memory only handles contiguous arrays");
	  #endif
		return(-5);
	}

	if ((TypeDst > Cfg->PeriphCnt)					/* If peripheral # is too high					*/
	&&  (TypeDst != INT_MAX)) {
	  #if ((DMA_DEBUG) != 0)
		printf("DMA  - Error - dma_xfer()  Dst peripheral #%d exceeds the maximum %d\n",
		        TypeDst, Cfg->PeriphCnt);
	  #endif
		return(-6);
	}

	if ((TypeSrc > Cfg->PeriphCnt)	 				/* If peripheral # is too high					*/
	&&  (TypeSrc != INT_MAX)) {
	  #if ((DMA_DEBUG) != 0)
		printf("DMA  - Error - dma_xfer() Src peripheral #%d exceeds the maximum %d\n",
		        TypeSrc, Cfg->PeriphCnt);
	  #endif
		return(-7);
	}

	if ((OpEnd != DMA_OPEND_NONE)					/* If an op is requtesed at end of Xfer, make	*/
	&&  (PtrEnd == NULL)) {							/* sure the pointrt is  not NULL				*/
	  #if ((DMA_DEBUG) != 0)
		printf("DMA  - Error - dma_xfer() NULL pointer for PtrEnd\n");
	  #endif
		return(-8);
	}
  #endif

  #if ((DMA_DEBUG) > 1)
	printf("\nDMA  - New transfer\n");
	printf("DMA  - Data size      : %d\n", DataSize);
	printf("DMA  - Burst size     : %d\n", BrstLen);
	printf("DMA  - # of transfers : %d\n", Nxfer);
	if (TypeSrc < 0) {
		printf("DMA  - Source is      : memory\n");
	}
	else {
		printf("DMA  - Source is      : periph #%d\n", TypeSrc);
	}
	printf("DMA  - Source incr    : %d\n", IncSrc);
	if ((TypeSrc <0) && (AddSrc == NULL)) {
		printf("DMA  - Writing zeros\n");
	}
	else {
		printf("DMA  - Source address : %p\n", AddSrc);
	}	
	printf("DMA  - Destin incr    : %d\n", IncDst);
	printf("DMA  - Destin address : %p\n", AddDst);
	if (TypeDst < 0) {
		printf("DMA  - Destin is      : memory\n");
	}
	else {
		printf("DMA  - Destin is      : periph #%d\n", TypeDst);
	}
  #endif

	MyCfg     = CfgOp;
	MyCfgNone = 0;		
	if (CfgOp == NULL) {							/* When CfgOP is NULL, use a local "array" 		*/
		MyCfg = &MyCfgNone;							/* with a single entry set to 0. It eliminates	*/
	}												/* Checking for NULL all the times				*/

	Evt    = -1;									/* -ve is no need for an trigger				*/
	Expiry =  0;									/* Assume no expiry waiting for avail chan		*/
	ii     = -1;
	Repeat =  0;									/* Assume not repeating							*/
	while (MyCfg[++ii] != 0) {
		if ((MyCfg[ii] & DMA_CFG_WAIT_AVAIL(0)) ==  DMA_CFG_WAIT_AVAIL(0)) {
			kk     = MyCfg[ii] & ~DMA_CFG_WAIT_AVAIL(0);
			Expiry = OS_TICK_EXP(kk);
		}
		if (MyCfg[ii] == DMA_CFG_WAIT_TRG) {
			Evt = 0;								/* Yes it needs an event						*/
		}
		if (MyCfg[ii] == DMA_CFG_REPEAT) {
			Repeat = 1;
		}
	}

	Reg = Cfg->HW;
													/* and release the spinlock for channel freeing	*/
	do {											/* Find a DMA channel not in use				*/
		DMA_LOCK(Cfg, ISRstate);					/* Get exclusive access to the DMA descriptors	*/
		for (Chan=0 ; Chan<Cfg->ChanCnt ; Chan++) {
			if ((Cfg->InUse[Chan] == 0U)			/* This channel is not used, but check if it	*/
			&&  (Cfg->EvtOwn[Chan] == 0)) {			/* is used by an event. If used or event can't	*/
				Cfg->InUse[Chan] = 1;				/* use that one	- busy it out					*/
				break;								/* We'll put the XferID later on				*/
			}
		}
		if (Chan >= Cfg->ChanCnt) {
			DMA_UNLOCK(Cfg, ISRstate);				/* Unlock only if no channel available			*/
		}											/* If while() loop terminates, will return		*/
	} while ((Chan >= Cfg->ChanCnt)					/* Retry if requested to wait for an avail chan	*/
	  &&     (0 == OS_HAS_TIMEDOUT(Expiry)));

	if (Chan >= Cfg->ChanCnt) {						/* No channel available during the time waiting	*/
	  #if ((DMA_DEBUG) != 0)						/* for one to be avail							*/
		puts("DMA  - Error - dma_xfer() No more channel available");
	  #endif
		return(-9);
	}

	ChCfg = &Cfg->ChCfg[Chan];
													/* When looking for an unused channel, timeout	*/
	if (Evt == 0) {									/* may be used, so need to re-enable the ISRs	*/
		DMA_UNLOCK(Cfg, ISRstate);					/* for the RTOS timer tick to increment			*/
													/* and release the spinlock for channel freeing	*/
		do {										/* Find a DMA channel not in use				*/
			DMA_LOCK(Cfg, ISRstate);
			for (Evt=Cfg->EvtIrqCnt-1 ; Evt>=0 ; Evt--) {	/* Pick event from highest #			*/
				if ((Cfg->EvtOwn[Evt] == 0)			/* Find an event that is not used				*/
				&&  (Evt != Chan)) {				/* Make sure event is not using the new channel	*/
					if (Evt < Cfg->ChanCnt) {		/* Make sure this channel is not used, nor		*/
						if ((Cfg->InUse[Evt] == 0U)	/* kept as a re-usable one						*/
						&&  (Cfg->ReUse[Evt] == 0U)) {
							break;					/* Is free use it. No need to busy out as the	*/
						}							/* lock remains set								*/
					}
					else {							/* Not a channel, is free	 					*/
						break;
					}
				}
			}
			if (Evt < 0) {
				DMA_UNLOCK(Cfg, ISRstate);			/* Unlock only if no channel available			*/
			}
		} while ((Evt < 0)							/* Retry if requested to wait for an avail chan	*/
		  &&     (0 == OS_HAS_TIMEDOUT(Expiry)));

		if (Evt < 0) {								/* No more available channels, abort			*/
			Cfg->InUse[Chan] = 0;					/* Release the channel that was busy out		*/
		  #if ((DMA_DEBUG) != 0)
			puts("DMA  - Error - dma_xfer() No more event available");
		  #endif
			return(-10);
		}
	}

  #if ((DMA_DEBUG) > 1)
	printf("DMA  - Using channel  : #%d\n", Chan);
  #endif

	MyXferID = DMA_ID(Cfg->DMAunik++, Dev, Chan, Evt);	/* Unique identifier for this transfer		*/

	Cfg->InUse[Chan]    = MyXferID;					/* Take ownership of the channel				*/
	Reg[DMA_INTEN_REG] |= (1<<Chan);				/* Set to Interrupt in case they are used		*/
	ChCfg->Started      &= 0;						/* Will be set when trully started				*/

	if (Evt >= 0) {									/* If transfer also need ownership of an event	*/
		Cfg->EvtOwn[Evt]    = MyXferID;				/* Take ownership of the channel				*/
		Cfg->InUse[Evt]     = MyXferID;
		Reg[DMA_INTEN_REG] &= ~(1<<Evt);			/* This is an event, not an interrupt			*/
	}

	DMA_UNLOCK(Cfg, ISRstate);

	if (XferID != NULL) {
		*XferID = MyXferID;							/* Report the transfer ID						*/
	}

  #if ((DMA_DEBUG) > 1)
	printf("DMA  - My transfer ID : %08X\n", (unsigned)MyXferID);
  #endif
													/* -------------------------------------------- */
													/* Reserved channel/event, time for processing	*/
													/* We own it, so can release exclusive access	*/
	ii         = -1;								/* an available channel							*/
	AddDstPhys = AddDst;							/* Assume no remapping from logical to physical	*/
	AddSrcPhys = AddSrc;
	Barrier    = 1;									/* Default is using I/O memory barrier			*/
	RetVal     = 0;
	ChCfg->AddDst = (TypeDst < 0)
	              ? AddDst							/* Assume has to do cache maintenance			*/
	              : NULL;
	ChCfg->AddSrc = (TypeSrc < 0)
	              ? AddSrc							/* NULL when writing 0 (no cache maintenance	*/
	              : NULL;
	while (MyCfg[++ii] != 0) {
		if (MyCfg[ii] == DMA_CFG_LOGICAL_DST) {
		  #if ((DMA_DEBUG) > 1)
			puts("DMA  - Using Physical destination address");
		  #endif
			AddDstPhys = MMUlog2Phy(AddDst);
		}
		if (MyCfg[ii] == DMA_CFG_LOGICAL_SRC) {
		  #if ((DMA_DEBUG) > 1)
			puts("DMA  - Using Physical source address");
		  #endif
			AddSrcPhys = MMUlog2Phy(AddSrc);
		}
		if (MyCfg[ii] == DMA_CFG_REUSE) {			/* No need to set to 0 by default (already is)	*/
		  #if ((DMA_DEBUG) > 1)
			printf("DMA  - Channel is to be re-used\n");
		  #endif
			Cfg->ReUse[Chan] = MyXferID;			/* Yes it's set to be reused					*/
		}
		if ((TypeDst < 0) 
		&&  (MyCfg[ii] == DMA_CFG_NOCACHE_DST)) {	/* Destination buffer is not cached				*/
		  #if ((DMA_DEBUG) > 1)
			printf("DMA  - Cache maint dst: skipped\n");
		  #endif
			ChCfg->AddDst = NULL;
		}
		if ((TypeSrc < 0)
		&&  (MyCfg[ii] == DMA_CFG_NOCACHE_SRC)) {	/* Source buffer is not cached					*/
		  #if ((DMA_DEBUG) > 1)
			printf("DMA  - Cache maint src: skipped\n");
		  #endif
			ChCfg->AddSrc = NULL;
		}
		if (MyCfg[ii] == DMA_CFG_NO_MBARRIER) {		/* Not using memory barrier with I/O			*/
		  #if ((DMA_DEBUG) > 1)
			printf("DMA  - no memory barrier on I/O\n");
		  #endif
			Barrier = 0;		
		}
	}

	if (Repeat != 0) {								/* This transfer repeats itself forever.		*/
		Cfg->ReUse[Chan] = MyXferID;				/* This means it is also re-useable				*/
	}

	if (ChCfg->AddDst != NULL) {					/* Wasn't provided with DMA_CFG_NOCACHE_DST, so	*/
		ii = (signed int)DataSize;					/* do the cache maintenance on dst memory		*/
		if (IncDst != 0) {							/* Only valid for 1D copy, 2D not yet supported	*/
			ii *= (signed int)Nxfer;				/* for IncDst == 0								*/
		}
		ChCfg->DstSize = ii;						/* Total number of bytes to invalidate at EOT	*/
	}

	ChCfg->AddDstOri = ChCfg->AddDst;				/* Needed when channel is re-used				*/

	if (ChCfg->AddSrc != NULL) {					/* Wasn't provided with DMA_CFG_NOCACHE_SRC, so	*/
		ii = (signed int)DataSize;					/* do the cache maintenance on dst memory		*/
		if (IncSrc != 0) {							/* Only valid for 1D copy, 2D not yet supported	*/
			ii *= (signed int)Nxfer;				/* for IncDst == 0								*/
		}
		ChCfg->SrcSize = ii;						/* Total number of bytes to invalidate at EOT	*/
	}

	ChCfg->OpEnd  = OpEnd;							/* Memo the EOT operation information			*/
	ChCfg->PtrEnd = PtrEnd;
	ChCfg->ValEnd = ValEnd;

	CCRarcache = (TypeSrc < 0)						/* Set all default values						*/
	           ? CCR_SC(7)							/* Memory reading: default is fully cached		*/
	           : CCR_SC(0);							/* I/O reading: default is non-cached			*/
	CCRawcache = (TypeDst < 0)
	           ? CCR_DC(7)							/* Memory writing: default is fully cached		*/
	           : CCR_DC(0);							/* I/O writing: default is non-cached			*/

	Chan2Wait  = Chan;								/* Channel to use for declaring EOT				*/
	Pgm        = &ChCfg->Pgm;
	WaitType   = DMA_CFG_EOT_POLLING;				/* Default: do polling unless told differently	*/
													/* Could become NOWAIT in here					*/
	ChCfg->WaitEnd = WaitType;
	Pgm->BufNow    = Pgm->BufBase;
	Pgm->LoopIdx   = 0;
	Pgm->LoopNow   = 0;
													/* Start of the DMA program creation			*/
													/* Set-up the src & dst addresses				*/
	if (Repeat != 0) {								/* Requested to repeat the sequence forever		*/
		DMAasm(Pgm, OP_DMALPFE);
	}
	DMAasm(Pgm, OP_DMAMOV, ARG_SAR, (uint32_t)AddSrcPhys);
	DMAasm(Pgm, OP_DMAMOV, ARG_DAR, (uint32_t)AddDstPhys);
													/* Check for "wait for links"					*/
	ii = -1;										/* TO DO: multi DMA devices cross trigger		*/
	memset(&(ChCfg->PendEnd),   0, sizeof(ChCfg->PendEnd));
	memset(&(ChCfg->PendStart), 0, sizeof(ChCfg->PendStart));

	if (Evt >= 0) {									/* Wait for an event to start the transfer		*/
		DMAasm(Pgm, OP_DMAWMB);
		DMAasm(Pgm, OP_DMARMB);						/* The event is my own event #					*/
		DMAasm(Pgm, OP_DMAWFE, (uint32_t)Evt, ARG_VALID);
		WaitType = DMA_CFG_NOWAIT;					/* Can't locally wait for EOT (will have end)	*/
	  #if ((DMA_DEBUG) > 1)
		printf("DMA  - Wait on event  : #%d\n", Evt);
	  #endif
	}

	Cfg->EvtSent[Chan] = 0;
	while (MyCfg[++ii] != 0) {						/* At start, if needs to send an event			*/
		if ((MyCfg[ii] & DMA_CFG_MASK_ARG) ==  DMA_CFG_TRG_ON_START(0)) {
			if (((MyCfg[ii] & 0x000C0000) == 0x000C0000)	/* Make sure the transfer ID is valid	*/
			&&  ((DMA_ID_DEV(MyCfg[ii])) == Dev)) {
				DMAasm(Pgm, OP_DMAWMB);				/* The event # is in the MyCfg entry			*/
				DMAasm(Pgm, OP_DMASEV, (uint32_t)DMA_ID_EVT(MyCfg[ii]));
				Cfg->EvtSent[Chan] |= 1 << (DMA_ID_CHN(MyCfg[ii]));
			  #if ((DMA_DEBUG) > 1)
				printf("DMA  - Sending event  : #%d at start\n", (int)DMA_ID_EVT(MyCfg[ii]));
			  #endif
			}										/* This supports multiple event sending			*/
		}
		else if ((MyCfg[ii] & DMA_CFG_MASK_ARG) ==  DMA_CFG_TRG_PEND_START) {
			for (ii=0 ; ii<(sizeof(ChCfg->PendStart)/sizeof(ChCfg->PendStart[0])) ; ii++) {
				if (ChCfg->PendStart[ii] == NULL) {
					DMAasm(Pgm, OP_DMAWMB);
					ChCfg->PendStart[ii] = Pgm->BufNow;
					DMAasm(Pgm, OP_DMASEV, (uint32_t)0);
					break;
				  #if ((DMA_DEBUG) > 1)
					printf("DMA  - Pending event at start\n");
				  #endif
				}
				if (ii >= (sizeof(ChCfg->PendStart)/sizeof(ChCfg->PendStart[0]))) {
					Cfg->InUse[Chan] = 0;			/* Release the channel that was busy out		*/
					Cfg->ReUse[Chan] = 0;
					if (Evt >= 0) {
						Cfg->InUse[Evt] = 0;
						Cfg->ReUse[Evt] = 0;
					}
				  #if ((DMA_DEBUG) != 0)
					puts("DMA  - Error - dma_xfer() too many DMA_CFG_TRG_PEND_START");
				  #endif
					return(-111);
				}
			}
		}
		else if ((MyCfg[ii] & DMA_CFG_MASK_ARG) ==  DMA_CFG_TRG_PEND_END) {
			for (ii=0 ; ii<(sizeof(ChCfg->PendEnd)/sizeof(ChCfg->PendEnd[0])) ; ii++) {
				if (ChCfg->PendStart[ii] == NULL) {
					DMAasm(Pgm, OP_DMAWMB);
					ChCfg->PendEnd[ii] = Pgm->BufNow;
					DMAasm(Pgm, OP_DMASEV, (uint32_t)0);
					break;
				  #if ((DMA_DEBUG) > 1)
					printf("DMA  - Pending event at end\n");
				  #endif
				}
				if (ii >= (sizeof(ChCfg->PendEnd)/sizeof(ChCfg->PendEnd[0]))) {
					Cfg->InUse[Chan] = 0;			/* Release the channel that was busy out		*/
					Cfg->ReUse[Chan] = 0;
					if (Evt >= 0) {
						Cfg->InUse[Evt] = 0;
						Cfg->ReUse[Evt] = 0;
					}
				  #if ((DMA_DEBUG) != 0)
					puts("DMA  - Error - dma_xfer() too many DMA_CFG_TRG_PEND_END");
				  #endif
					return(-111);
				}
			}
		}											/* What to do for the end of transfer			*/
		else if ((MyCfg[ii] == DMA_CFG_EOT_POLLING)
		     ||  (MyCfg[ii] == DMA_CFG_EOT_ISR)) {
		  #if (((DMA_OPERATION) & 1) != 0)			/* If no ISR, can only do polling				*/
			ChCfg->WaitEnd = MyCfg[ii];				/* WaitEnd was set to POLLING 					*/
		  #endif

			if (WaitType != DMA_CFG_NOWAIT) {		/* Once is set to NOWAIT, that's it				*/
				WaitType = ChCfg->WaitEnd;
			  #if ((DMA_DEBUG) > 1)
			   #if (((DMA_OPERATION) & 1) != 0)
				if (MyCfg[ii] ==  DMA_CFG_EOT_POLLING) {
					puts("DMA  - EOT wait       : polling");
				}
				else {
					puts("DMA  - EOT wait       : ISR");
				}
			   #else
				if (MyCfg[ii] ==  DMA_CFG_EOT_POLLING) {
					puts("DMA  - EOT wait       : polling");
				}
				else {
					puts("DMA  - EOT wait       : ISR requested but polling done (DMA_OPERATION)");
				}
			   #endif
			  #endif
			}
		}
		else if (MyCfg[ii] == DMA_CFG_NOWAIT) {
			WaitType = MyCfg[ii];
		  #if ((DMA_DEBUG) > 1)
			puts("DMA  - Wait for EOT   : skipped");
		  #endif
		}											/* Waiting EOT on another channel				*/
		else if ((MyCfg[ii] & DMA_CFG_MASK_ARG) ==  DMA_CFG_WAIT_ON_END(0)) {
			if ((MyCfg[ii] & 0x000C0000) == 0x000C0000) {	/* Make sure the transfer ID is valid	*/
				Chan2Wait = DMA_ID_CHN(MyCfg[ii]);
			  #if ((DMA_DEBUG) > 1)
				printf("DMA  - Source is      : memory\n");
				printf("DMA  - EOT on channel : #%d\n", Chan2Wait);
			  #endif
			}
		}										
		else if ((MyCfg[ii] & DMA_CFG_MASK_ARG) ==  DMA_CFG_ARCACHE(0)) {
			CCRarcache = CCR_SC(MyCfg[ii] & 7);		/* Removbe op-copde and isolate cache bits		*/
		  #if ((DMA_DEBUG) > 1)
			printf("DMA  - ARCACHE set to : #%d\n", (int)(CCR_SC(MyCfg[ii] & 7)));
		  #endif
		}											/* Using a different cache setting when writing	*/
		else if ((MyCfg[ii] & DMA_CFG_MASK_ARG) ==  DMA_CFG_AWCACHE(0)) {
			CCRawcache = CCR_DC(MyCfg[ii] & 7);		/* Removbe op-copde and isolate cache bits		*/
		  #if ((DMA_DEBUG) > 1)
			printf("DMA  - AWCACHE set to : #%d\n", (int)(CCR_SC(MyCfg[ii] & 7)));
		  #endif
		}
	}
	ChCfg->ChanWait  = Chan2Wait;
	ChCfg->TimeOut   = TimeOut;

	if  ((TypeDst < 0)								/* Memory to memory transfer					*/
	&&   (TypeSrc < 0)) {
		if ((IncDst == DataSize)					/* Select 1D or 2D memory transfers				*/
		&&  (IncSrc == DataSize)) {
			Size   = DataSize						/* Total number of bytes to copy				*/
			       * Nxfer;
			dma_mem_1D(Pgm, AddDstPhys, AddSrcPhys, Size, CCRarcache, CCRawcache);
		}
		else {
		  #if ((DMA_DEBUG) != 0)
			puts("DMA  - Error - dma_xfer() 2D memory copy not yet supported");
		  #endif
			RetVal = -11;			
		}
	}
	else if ((TypeSrc < 0)							/* Memory to I/O transfers or I/O to memory		*/
	     ||  (TypeDst < 0)) {
			RetVal = dma_IO(Pgm, AddDstPhys, IncDst, TypeDst, AddSrcPhys, IncSrc, TypeSrc,
			                DataSize, BrstLen, Nxfer, CCRarcache, CCRawcache, Barrier);
	}
	else {											/* I/O to I/O transfer: not supported			*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_xfer() I/O to I/O transfers not supported");
	  #endif
		RetVal = -12;
	}

	if (RetVal == 0) {								/* Xfer done, insert event sending at EOT		*/
		Start = 1;
		if (MyCfg != NULL) {						/* Check for wait for links						*/
			ii = -1;								/* TO DO: multi DMA devices cross trigger		*/
			while (MyCfg[++ii] != 0) {				/* Send an event at the end of the transfer		*/
				if ((MyCfg[ii] & DMA_CFG_MASK_ARG) ==  DMA_CFG_TRG_ON_END(0)) {
					if ((MyCfg[ii] & 0x000C0000) == 0x000C0000) {/* Make sure transfer ID is valid	*/
						if ((DMA_ID_DEV(MyCfg[ii])) == Dev) {	/* Event # is in the MyCfg entry	*/
							DMAasm(Pgm, OP_DMASEV, (uint32_t)DMA_ID_EVT(MyCfg[ii]), ARG_VALID);
							Cfg->EvtSent[Chan] |= 1 << (DMA_ID_CHN(MyCfg[ii]));
						  #if ((DMA_DEBUG) > 1)
							printf("DMA  - Sending event  : #%d on end\n",(int)DMA_ID_EVT(MyCfg[ii]));
						  #endif
						}							/* This supports sending multiple events		*/
					}			
				}
				if (MyCfg[ii] == DMA_CFG_NOSTART) {
					Start = 0;
				}
			}
		}
													/* Memory barriers & raising channel interrupt	*/
		DMAasm(Pgm, OP_DMAWMB);						/* Make sure all data read / written is out of	*/
		DMAasm(Pgm, OP_DMARMB);						/* the pipeline									*/

	  #if (((DMA_OPERATION) & 1) != 0)
		if ((ChCfg->WaitEnd == DMA_CFG_EOT_ISR)		/* Raise the channel processor interrupt only	*/
		||  (OpEnd != DMA_OPEND_NONE)) {			/* if using the ISRs and asked to EOT with ISR	*/
			DMAasm(Pgm, OP_DMASEV, Chan);			/* Or if an operation must be performed 		*/
		}
	  #endif

		if (Repeat != 0) {							/* Was requested to repeat the sequence			*/
			DMAasm(Pgm, OP_DMALPEND);				/* End of the loop forever						*/
		}

		ii = DMAasm(Pgm, OP_DMAEND);				/* Terminate the transfer. Program creation done*/

		if (ii != 0) {								/* Last DMAasm, make sure the DMA program does	*/
		  #if ((DMA_DEBUG) != 0)					/* fit in the available memory					*/
			puts("DMA  - Error - dma_xfer() program area too small");
		  #endif
			RetVal = -13;
		}
	}
													/* Make sure the DMA program, is not in cache	*/
	DCacheFlushRange((void *)&Pgm->BufBase, DMA_PGM_SIZE);

	if ((RetVal == 0)								/* So far so good								*/
	&&  (Start != 0)) {								/* Tell the DMA to execute the program			*/

		dma_start(MyXferID);

		if (WaitType != (DMA_CFG_NOWAIT)) {
			if (dma_wait(MyXferID) != 0) {
			  #if ((DMA_DEBUG) != 0)
				puts("DMA  - Error - dma_xfer() Timeout waiting for end of transfer");
			  #endif
				RetVal = -14;
			}
		}
	}

	if (RetVal != 0) {								/* There was a transfer error					*/
		Cfg->InUse[Chan] = Cfg->ReUse[Chan];		/* Release the channel.. or not if is reuseable	*/
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: dma_mem_1D																				*/
/*																									*/
/* dma_mem_1D - memory to memory transfer (1 dimension: vector copy)								*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void dma_mem_1D(DMApgm_t *Pgm,																*/
/*                      uint8_t *AddDst, const uint8_t *AddSrc, uint32_t Size,						*/
/*		                uint32_t CCRarcache, uint32_t CCRawcache);									*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Pgm        : DMA program data structure														*/
/*		AddDst     : base address of the destination												*/
/*		AddSrc     : base address of the source														*/
/*		Size       : number of bytes to copy														*/
/*		CCRarcache : source cache attributes														*/
/*		CCRawcache : destination cache attributes													*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*																									*/
/* IMPLEMENTATION:																					*/
/* ------------------------------------------------------------------------------------------------ */

static void dma_mem_1D(DMApgm_t *Pgm,
                       void *AddDst, const void *AddSrc, uint32_t Size,
                       uint32_t CCRarcache, uint32_t CCRawcache)
{
uint32_t CCRcommon;									/* Many entries in CCR are always the same		*/
int      Dst8;										/* When dst is unaligned leftover to store		*/
int      LoopOut;									/* Outer loop count								*/
int      LoopIn;									/* Inner loop count								*/
uint32_t Src8;										/* # of read to align src address on 8 byte		*/
													/* Common CCR set-up value						*/
	CCRcommon = CCR_SP_DEF							/* Source cache protection						*/
	          | CCR_SAI								/* Source addres increment						*/
	          | CCRarcache							/* Source cache control							*/
	          | CCR_DAI								/* Destination address increment				*/
	          | CCR_DP_DEF							/* Destination cache protection					*/
	          | CCRawcache							/* Destination cache control					*/
	          | CCR_ES(8);							/* Endian swap size: 1 byte == none				*/

	Src8 = 0x7 & (uintptr_t)AddSrc;					/* Align reads on 8 bytes						*/
	Src8 = 0x7 & (8 - Src8);
	if (Src8 > Size) {
		Src8 = Size;
	}

	if (Src8 != 0) {								/* Align reads in preparation for bursting		*/
		DMAasm(Pgm, OP_DMAMOV, ARG_CCR, CCRcommon	/* Do bytes transfers, for no leftover in MFIFO	*/
		                              | CCR_SB(Src8)/* Source burst length							*/
		                              | CCR_SS8		/* Source burst size: 1 byte					*/
		                              | CCR_DB(Src8)/* Destination burst length						*/
		                              | CCR_DS8);	/* Destination burst size: 1 byte				*/
		DMAasm(Pgm, OP_DMALD);						/* Read Src8 bytes								*/
		DMAasm(Pgm, OP_DMAST);
													/* When writing zeros, this if {} is not entered*/
		Size -= Src8;								/* This has been transfered						*/
	}
													/* From now on, the source address is aligned	*/
	Dst8 = 0;										/* When bursts of 8 are used, there may be left	*/
	if (Size >= 8) {								/* over in MFIFO.  The fisrt DMAST only fills	*/
		Dst8 = (Src8 + (uintptr_t)AddDst)			/* from AddDst to next address an exact *8		*/
		     & 0x7;									/* Src8 + AddDst is AddDst after the DMAST above*/
	}												/* When Size >= 8, burst of 8 is/are used		*/

	if (Size >= (16*8)) {
		DMAasm(Pgm, OP_DMAMOV, ARG_CCR, CCRcommon
		                              | CCR_SB(16)	/* Source burst length							*/
		                              | CCR_SS64	/* Source burst size: 8 bytes					*/
		                              | CCR_DB(16)	/* Destination burst length						*/
		                              | CCR_DS64);	/* Destination burst size: 8 bytes				*/
	}

	while (Size >= (16*8)) {						/* To achieve fastest transfer rate, transfer	*/
		if (Size >= 256*256*16*8) {					/* 64 bits (8 bytes) words in bursts of 16		*/
			LoopIn  = 256;
			LoopOut = 256;
		}
		else if (Size >= 256*16*8) {
			LoopIn  = 256;
			LoopOut = Size >> (8+4+3);				/* >> is  / (256*16*8)							*/
		}
		else {
			LoopOut = 1;
			LoopIn  = Size >> (4+3);
		}

		Size -= LoopOut
		      * LoopIn
		      * 16
		      * 8;

		if (LoopOut != 1) {
			DMAasm(Pgm, OP_DMALP, (uint32_t)LoopOut);	/* Outer loop								*/
		}
		DMAasm(Pgm, OP_DMALP, (uint32_t)LoopIn);	/* Inner loop									*/

		if (AddSrc == NULL) {						/* Writing zeros								*/
			DMAasm(Pgm, OP_DMASTZ);
		}
		else {
			DMAasm(Pgm, OP_DMALD);					/* Read Src8 bytes								*/
			DMAasm(Pgm, OP_DMAST);
		}

		DMAasm(Pgm, OP_DMALPEND);					/* End of inner loop							*/
		if (LoopOut != 1) {
			DMAasm(Pgm, OP_DMALPEND);				/* End of outer loop							*/
		}	
	}

	Src8 = Size >> 3;								/* Number of 8 byte bursts to perform			*/
	if (Src8 != 0) {								/* If some 8 byte transfer left to perform		*/
		DMAasm(Pgm, OP_DMAMOV, ARG_CCR, CCRcommon
		                              | CCR_SB(Src8)	/* Source burst length						*/
		                              | CCR_SS64		/* Source burst size: 8 bytes				*/
		                              | CCR_DB(Src8)	/* Destination burst length					*/
		                              | CCR_DS64);		/* Destination burst size: 8 bytes			*/
		if (AddSrc == NULL) {						/* Writing zeros								*/
			DMAasm(Pgm, OP_DMASTZ);
		}
		else {
			DMAasm(Pgm, OP_DMALD);
			DMAasm(Pgm, OP_DMAST);
		}
	}

	if (Dst8 != 0) {								/* DMAST needed when destination is unaligned	*/
		DMAasm(Pgm, OP_DMAMOV, ARG_CCR, CCRcommon	/* Doesn't work if combined with Src8 below		*/
		                              | CCR_SB(Dst8)	/* Source burst length (is a don't care)	*/
		                              | CCR_SS8			/* Source burst size: 1 byte				*/
		                              | CCR_DB(Dst8)	/* Destination burst length					*/
		                              | CCR_DS8);		/* Destination burst size: 1 byte			*/
		if (AddSrc == NULL) {						/* Writing zeros								*/
			DMAasm(Pgm, OP_DMASTZ);
		}
		else {
			DMAasm(Pgm, OP_DMAST);
		}
	}

	Src8 = Size & 0x7;								/* The final transfer size non multiple of 8	*/
	if (Src8 != 0) {								/* Left over to transfer in byte				*/
		DMAasm(Pgm, OP_DMAMOV, ARG_CCR, CCRcommon
		                              | CCR_SB(Src8)	/* Source burst length						*/
		                              | CCR_SS8			/* Source burst size: 1 byte				*/
		                              | CCR_DB(Src8)	/* Destination burst length					*/
		                              | CCR_DS8);		/* Destination burst size: 1 byte			*/
		if (AddSrc == NULL) {						/* Writing zeros								*/
			DMAasm(Pgm, OP_DMASTZ);
		}
		else {
			DMAasm(Pgm, OP_DMALD);
			DMAasm(Pgm, OP_DMAST);
		}
	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: dma_IO																					*/
/*																									*/
/* dma_IO - I/O to memory or memory to I/O transfer													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void dma_IO(DMApgm_t *Pgm,																	*/
/*                  uint8_t *AddDst, int IncDst, int TypeDst,										*/
/*                  uint8_t *AddSrc, int IncSrc, int TypeSrc,										*/
/*                  uint32_t DataSize, uint32_t BrstLen, uint32_t Nxfer,							*/
/*                  uint32_t CCRarcache, uint32_t CCRawcache, int Barrier)							*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Pgm        : DMA program data structure														*/
/*		AddDst     : Base address for the destination 												*/
/*		IncDst     : Increment of the destination address in bytes									*/
/*		TypeDst    : Type of destination : <  0 is memory											*/
/*		                                   >= 0 is a peripheral										*/
/*		AddSrc     : Base address for the source 													*/
/*		IncSrc     : Increment of the source address in bytes										*/
/*		TypeSrc    : Type of source : <  0 is memory												*/
/*		                              >= 0 is a peripheral											*/
/*		DataSize   : Size of each individual dat atransfer in bytes									*/
/*		BrstLen    : Number of data to access in one shot											*/
/*		Nxfer      : Total number of data transfer (this is not the number of BurstLen)				*/
/*		CCRarcache : Value in the CCR to use for the read cache										*/
/*		CCRawcache : Value in the CCR to use for the write cache									*/
/*		Barrier    : Boolean to use or not memory barrier											*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		As this is IO to memory or memory to IO transfers, the number of bytes read/written to		*/
/*		memory is done relying on filling as much the the MFIFO to try to get the fastest data		*/
/*		transfer possible.																			*/
/*		The argument BrstLen indicates the number of data that can be read/written to the IO in one	*/
/*		shot once the trigger is validated. It is assume that less than this amount can also be		*/
/*		transfered.	If BrstLen is > 16, it is set to 16 as this is the limits of the PL330.			*/
/*		The DMA code generated is possibly constructed with 3 sections and here N is defined by		*/
/*						N = BrstLen*(16/BrstLen)													*/
/*					which is the number of BrstLen that fits into 16								*/
/*			- transfers in one shot < BrstLen														*/
/*			- transfers BrstLen when less than N*BrstLen											*/
/*			- transfers N*BrstLen																	*/
/*		If the transfer is from I/O to memory, the blocks are in sequence from smallest xfer size	*/
/*		to the largest.  This is needed as the BrstLen is most likely the watermark that triggers	*/
/*		the transfer.  So it is necessary to make sure the last transfer is at least of BrstLen		*/
/*		size, otherwise it will never be trigger.													*/
/*		If the transfer is from memory to I/O,  the blocks are in sequence from largest xfer size	*/
/*		to the smallest. This will make sure the TX FIFO is filled above the watermark as quickly	*/
/*		as possible.																				*/
/*		In both caae: I/O to memory or memory to I/O, is the base of the memory is not aligned with	*/
/*		the burst size, before the 3 blocks enough bytes are read/written from/to the memory to		*/
/*		make it aligned. After the 3 blocks, the remainder of the bytes to make up a burst size i	*/
/*		then read/written from/to the memory.														*/
/*																									*/
/*		NOTE: athough the PL330 can only perform burst length of 16, the MFIFO is most likely		*/
/*		      able to accumulte more the 16 data.  16 is the limit selected as the size of the		*/
/*		      MFIFO is not the same across all implementation and the MFIFO is share amongst		*/
/*		      all the DMA channels so we dont want to use all of it to starve other channels.		*/
/* ------------------------------------------------------------------------------------------------ */

static int dma_IO(DMApgm_t *Pgm, 
                  void *AddDst, int IncDst, int TypeDst,
                  const void *AddSrc, int IncSrc, int TypeSrc,
                  int DataSize, int BrstLen, uint32_t Nxfer,
                  uint32_t CCRarcache, uint32_t CCRawcache, int Barrier)
{
int      Block;										/* Counter for the 3 blocks to process			*/
int      BrstLenMem;								/* Burst length for reading/writing memory		*/
int      BrstNow;									/* Current burst len in the block				*/
int      BrstIO;									/* Current sub-burst len in the block			*/
uint32_t CCRcommon;									/* Many entries in CCR are always the same		*/
uint32_t Dst8;
int      IncDstMan;									/* Doing manual addr increment or no increment	*/
int      IncSrcMan;									/* Doing manual addr increment or no increment	*/
int      ii;										/* General purpose								*/
int      jj;										/* General purpose								*/
int      LoopIn;									/* Inner loop count								*/
int      LoopOut;									/* Outer loop counft							*/
int      MyXfer;									/* In each block, the number fo transfer to do	*/
uint32_t Src8;
uint32_t WFPbrst;									/* Waiting for single or burst from periph		*/


	if (BrstLen > 16) {								/* 16 is the maximum burst length supported		*/
		BrstLen = 16;								/* by the PL330									*/
	}
	if (BrstLen > Nxfer) {							/* Burst cannot be larger than # to transfers	*/
		BrstLen = Nxfer;
	}
	BrstLenMem = BrstLen							/* Values to use when doing as large of a burst	*/
	           * (16 / BrstLen);					/* as possible									*/

	if (BrstLenMem > Nxfer) {						/* Can't do more than the # to transfer			*/
		BrstLenMem = BrstLen						/* Keep it an exact multiple of BrstLen			*/
		           * (Nxfer/BrstLen);
	}

	IncDstMan = (IncDst == 0);						/* When no increment, is alike manual increment	*/
	if ((IncDst != 0)								/* Check if we need to increment the destination*/
	&&  (IncDst != DataSize)) {						/* manually. If so, the burst length must be 1	*/
		if (BrstLen != 1) {
			return(-20);
		}
		IncDstMan  = 1;
		BrstLenMem = 1;
	}

	IncSrcMan = (IncSrc == 0);						/* When no increment, is alike manual increment	*/
	if ((IncSrc != 0)								/* Check if we need to increment the destination*/
	&&  (IncSrc != DataSize)) {						/* manually. If so, the burst length must be 1	*/
		if (BrstLen != 1) {
			return(-21);
		}
		IncSrcMan  = 1;
		BrstLenMem = 1;
	}

	CCRcommon = CCR_SP_DEF							/* Source cache protection						*/
	          | ((IncSrcMan == 0) ? CCR_SAI : CCR_SAF)	/* Source addres increment					*/
	          | CCRarcache							/* Source cache control							*/
	          | ((IncDstMan == 0) ? CCR_DAI : CCR_DAF)	/* Destination address increment			*/
	          | CCR_DP_DEF							/* Destination cache protection					*/
	          | CCRawcache							/* Destination cache control					*/
	          | CCR_ES(8);							/* Endian swap size: 1 byte == none				*/

													/* -------------------------------------------- */
	Dst8   = 0;										/* Need to deal with the MFIFO 					*/
	Src8   = 0;
	if (TypeDst < 0) {								/* When I/O to memory, may need a final DMAST	*/
		if (DataSize >= 8) {						/* Bursts size of 8 bytes or +, 8 bytes align	*/
			Dst8 = 0x7 & (uintptr_t)AddDst;			/* is sufficient								*/
		}
		else {
			Dst8 = (DataSize-1) & (uintptr_t)AddDst;
		}
	}
	else {											/* Memory to I/O: align the memory read address	*/
		if (DataSize >= 8) {						/* Done by reading # bytes to become aligned	*/
			Src8 = 0x7 & (uintptr_t)AddSrc;			/* Bursts size of 8 bytes or +, 8 bytes align	*/
		}											/* is sufficient								*/
		else {										/* Src8 is 0 (aligned) when writing zeros		*/
			Src8 = (DataSize-1) & (uintptr_t)AddSrc;
		}
	}

	for (Block=0 ; Block<3 ; Block++) {				/* Generate the 3 transfers sections			*/
		if (TypeDst < 0) {							/* I/O -> memory								*/
			if (Block == 0) {						/* First block is < BrstLen transfers			*/
				MyXfer  = Nxfer % BrstLen;			/* Transfers done 1 data at a time				*/
				BrstIO  = 1;								
				BrstNow = MyXfer;
			}
			else if (Block == 1) {					/* Second block is >= BrstLen and < BrstLenMem	*/
				MyXfer  = Nxfer % BrstLenMem;		/* I/O transfers done BrstLen at a time but mem	*/
				BrstIO  = BrstLen;					/* transfers done N*BrstLen at a time			*/
				BrstNow = MyXfer;
			}
			else {									/* Last block is BrstLenMem transfers			*/
				MyXfer  = Nxfer;
				BrstIO  = BrstLen;
				BrstNow = BrstLenMem;
			}
		}
		else {										/* Memory -> IO									*/
			if (Block == 0) {						/* First block is BrstLenMem transfers			*/
				MyXfer  = BrstLenMem
				        * (Nxfer/BrstLenMem);
				BrstIO  = BrstLen;
				BrstNow = BrstLenMem;
			}
			else if (Block == 1) {					/* Second block is >= BrstLen and < BrstLenMem	*/
				MyXfer  = BrstLen					/* I/O ransfers done BrstLen at a time but mem	*/
				        * (Nxfer / BrstLen);		/* transfers done N*BrstLen at a time			*/
				BrstIO  = BrstLen;
				BrstNow = MyXfer;
			}
			else {									/* Last block is < BrstLen transfers			*/
				MyXfer  = Nxfer;					/* Transfers done 1 data at a time				*/
				BrstIO  = 1;								
				BrstNow = MyXfer;
			}
		}

		Nxfer  -= MyXfer;							/* Remove from Nxfer what will be handled		*/
		WFPbrst = (BrstIO == 1)						/* Select the type of wait for peripheral		*/
		        ? OP_DMAWFPS						/* Burst length == 1, single data per Xfer		*/
		        : OP_DMAWFPB;						/* Burst length != 1, burst len data per Xfer	*/

		if (MyXfer != 0) {							/* We have to do this transfer					*/
			while (MyXfer >= BrstNow) {				/* Determine the outer & inner loop counts		*/
				if (MyXfer >= 256*256*BrstNow) {	/* >= 256*256, both loops are set to 256		*/
					LoopOut = 256;
					LoopIn  = 256;
				}
				else if (MyXfer >= 256*BrstNow) {	/* >= 256, Inner is 256, Outer is as required	*/
					LoopOut = MyXfer / (256*BrstNow);
					LoopIn  = 256;
				}
				else {
					LoopOut = 1;					/* < 256, Outer is 1, Inner is set as required	*/
					LoopIn  = MyXfer / BrstNow;
				}

				MyXfer -= LoopOut 					/* # of transfers left after this programming	*/
				        * LoopIn
				        * BrstNow;

				if (Src8 != 0) {					/* When memory read is not aligned, must		*/
					DMAasm(Pgm, OP_DMAMOV, ARG_CCR,	/* Set-up the CCR register						*/
					                       CCRcommon
					                     | CCR_SB(Src8)	/* Src burst len to compensate MFIFO		*/
					                     | CCR_SS(DataSize*8)	/* Src burst size : as requested	*/
					                     | CCR_DB(1) 			/* Dst burst len : is a don't care	*/
				                         | CCR_DS(DataSize*8));	/* Dst burst size: MUST be as rqst	*/
																/* as used for MFIFO alignment		*/
					DMAasm(Pgm, OP_DMALD);			/* Perform the extra burst load					*/
					Src8 = 0;						/* Done, don't it again							*/
				}

				DMAasm(Pgm, OP_DMAMOV, ARG_CCR,		/* Set-up the CCR register						*/
				                       CCRcommon
				                     | CCR_SB((TypeSrc<0)?BrstNow:BrstIO)	/* Src burst length		*/
				                     | CCR_SS(DataSize*8)	/* Src burst size : as requested		*/
			                         | CCR_DB((TypeDst<0)?BrstNow:BrstIO)	/* Dst burst length		*/
			                         | CCR_DS(DataSize*8));	/* Dst burst size: as requested			*/

				if (LoopOut != 1) {					/* No loop instruction needed when loop of 1	*/
					DMAasm(Pgm, OP_DMALP, (uint32_t)LoopOut);/* Outer loop							*/
				}
				if (LoopIn != 1) {					/* No loop instruction needed when loop of 1	*/
					DMAasm(Pgm, OP_DMALP, (uint32_t)LoopIn);/* Inner loop							*/
				}

				if (TypeSrc < 0) {					/* Source is memory								*/
					if (AddSrc == NULL) {			/* Request to write 0s							*/
						for (jj=0 ; jj<BrstNow ; jj+=BrstIO) {
							if (TypeDst != INT_MAX) {		/* == INT_MAX is no trigger to wait on	*/
								DMAasm(Pgm, OP_DMAFLUSHP, TypeDst);	/* Re-sync with the I/O			*/
								DMAasm(Pgm, WFPbrst, TypeDst);		/* Wait for room in I/O			*/
							}
							if (Barrier != 0) {		/* Memory barrier are used when I/O read from	*/
								DMAasm(Pgm, OP_DMARMB);	/* same reg also going on					*/
								DMAasm(Pgm, OP_DMASTZ);	/* Write one IO burst of zeros each time	*/
								DMAasm(Pgm, OP_DMAWMB);	/* Make sure the I/O got everything 		*/
							}
							else {
								DMAasm(Pgm, OP_DMASTZ);
							}
						}
					}
					else {
						DMAasm(Pgm, OP_DMALD);		/* Load as much data as possible from memory	*/
						if (Barrier != 0) {
							DMAasm(Pgm, OP_DMARMB);	/* Make sure the data from memory all read		*/
						}
						for (jj=0 ; jj<BrstNow ; jj+=BrstIO) {
							if (TypeDst != INT_MAX) {		/* == INT_MAX is no trigger to wait on	*/
								DMAasm(Pgm, OP_DMAFLUSHP, TypeDst);	/* Re-sync with the I/O			*/
								DMAasm(Pgm, WFPbrst, TypeDst);		/* Wait for room in I/O			*/
							}
							if (Barrier != 0) {		/* Memory barrier are used when I/O read from	*/
								DMAasm(Pgm, OP_DMARMB);	/* from same reg also going on				*/
								DMAasm(Pgm, OP_DMAST);	/* Write one IO burst length each time		*/
								DMAasm(Pgm, OP_DMAWMB);	/* Make sure the I/O got everything 		*/
							}
							else {
								DMAasm(Pgm, OP_DMAST);
							}
						}
					}
				}
				else {								/* Source is IO / Destination is memory			*/
					for (ii=0 ; ii<BrstNow ; ii+=BrstIO) {	/* Get as much to fill up to 16 data	*/
						if (TypeSrc != INT_MAX) {		/* == INT_MAX is no trigger to wait on		*/
							DMAasm(Pgm, OP_DMAFLUSHP, TypeSrc);	/* Re-sync with the I/O				*/
							DMAasm(Pgm, WFPbrst, TypeSrc);		/* Wait for the IO to have data		*/
						}
						if (Barrier != 0) {			/* Memory barrier are used when I/O write to	*/
							DMAasm(Pgm, OP_DMAWMB);	/* same reg also going on						*/
							DMAasm(Pgm, OP_DMALD);	/* Read one IO burst length each time			*/
							DMAasm(Pgm, OP_DMARMB);	/* Make sure got everything from I/O			*/
						}
						else {
							DMAasm(Pgm, OP_DMALD);
						}
					}
					DMAasm(Pgm, OP_DMAST);			/* All data read from I/O is written to memory	*/
					if (Barrier != 0) {	
						DMAasm(Pgm, OP_DMAWMB);		/* Make sure all landed in memory				*/
					}
				}

				if (IncDstMan != 0) {				/* If the requested increment is not the size	*/
					ii = IncDst						/* of the individual transfers, do it manually	*/
					   * BrstNow;
					jj = OP_DMAADDH;				/* Assume +ve increment							*/
					if (ii < 0) {					/* Negatibe increment, change sign & opcode		*/
						jj = OP_DMAADNH;
						ii = -ii;
					}
					while (ii >= 0xFFFF) {			/* Increment / decrement the addresses			*/
						DMAasm(Pgm, jj, ARG_DAR, 0xFFFF);
						ii -= 0xFFFF;
					}
					if (ii != 0) {					/* Remainder									*/
						DMAasm(Pgm, jj, ARG_DAR, ii);
					 	if (Barrier != 0) {
							DMAasm(Pgm, OP_DMAWMB);	
						}
					}
				}
	
				if (IncSrcMan != 0) {				/* If the requested increment is not the size	*/
					ii = IncSrc						/* of the individual transfers, do it manually	*/
					   * BrstNow;
					jj = OP_DMAADDH;				/* Assume +ve increment							*/
					if (ii < 0) {					/* Negatibe increment, change sign & opcode		*/
						jj = OP_DMAADNH;
						ii = -ii;
					}
					while (ii >= 0xFFFF) {			/* Increment / decrement the addresses			*/
						DMAasm(Pgm, jj, ARG_SAR, 0xFFFF);
						ii -= 0xFFFF;
					}
					if (ii != 0) {					/* Remainder									*/
						DMAasm(Pgm, jj, ARG_SAR, ii);
					}
				}

				if (LoopIn != 1) {					/* No loop instruction needed when single loop	*/
					DMAasm(Pgm, OP_DMALPEND);		/* End of inner loop							*/
				}
				if (LoopOut != 1) {					/* No loop instruction needed when single loop	*/
					DMAasm(Pgm, OP_DMALPEND);		/* End of outer loop							*/
				}
			}
		}
	}												/* End of loop of 3 blocks						*/

	if (Dst8 != 0) {								/* I/O to mem final DMAST to flush the MFIFO	*/
		DMAasm(Pgm, OP_DMAMOV, ARG_CCR,				/* Set-up the CCR register						*/
		                       CCRcommon
		                     | CCR_SB(DataSize*8)	/* Src burst length: it's a don't care			*/
		                     | CCR_SS8				/* Src burst size : it's a don't care			*/
	                         | CCR_DB(Dst8)			/* Dst burst length: write all at once			*/
	                         | CCR_DS8);			/* Dst burst size: write bytes					*/
		DMAasm(Pgm, OP_DMAST);						/* Write the remainder # of bytes				*/
	}

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: dma_cleanup																			*/
/*																									*/
/* dma_cleanup - invalidate the cache for the destination buffer									*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int dma_cleanup(DMAcfg_t *Cfg, int XferID)													*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Cfg    : Configuration for the target DMA controller										*/
/*		XferID : ID of the transfer to start														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : invalid XferID																		*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		Is always called from code region where the device access is ocked, so no need to lock.		*/
/* ------------------------------------------------------------------------------------------------ */

static void dma_cleanup(DMAcfg_t *Cfg, int XferID)
{
int       Chan;										/* DMA channel number							*/
CHNcfg_t *ChCfg;									/* Cahnnel configuration						*/
uint32_t  Dep;										/* Bit field of dependent transfers				*/
int       ii;										/* General purpose								*/
int32_t  *InUse;									/* CFG InUse field (XferID or 0)				*/

  #if ((DMA_ARG_CHECK) != 0)						/* Arg check, remapping without checkings		*/
	if ((XferID >> 16) != 0xC) {					/* Make sure the transfer ID is valid			*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_cleanup() invalid ID");
	  #endif
		return;
	}
  #endif

	Chan  = DMA_ID_CHN(XferID);						/* Extract the channel							*/
	ChCfg = &Cfg->ChCfg[Chan];
	Dep   = Cfg->EvtSent[Chan];						/* De-reference for better real time			*/
	InUse = &Cfg->InUse[0];							/* De-reference for better real time			*/

	while (Dep != 0U) {								/* Invalidate all my dependents and theirs etc	*/
		if ((Dep & 1) != 0) {						/* When this is one of my dependents			*/
			dma_cleanup(Cfg, *InUse);				/* Re-enter: invalidate it and its dependents	*/
		}
		Dep >>= 1;									/* Check next dependent							*/
		InUse++;									/* Next channel									*/
	}
													/* Invalidate my destination buffer				*/
	if (ChCfg->AddDst != NULL) {					/* Is NULL if no invalidation was requested		*/
		DCacheInvalRange((void *)ChCfg->AddDst, ChCfg->DstSize);	/* Or is I/O register			*/
		ChCfg->AddDst = NULL;
	}

	Cfg->InUse[Chan] = Cfg->ReUse[Chan];			/* Free or keep it busy out if re-used			*/

	ChCfg->Started   = 0;

	for (ii=0 ; ii<Cfg->EvtIrqCnt ; ii++) {			/* Free the evnts I'm the owner					*/
		if (Cfg->EvtOwn[ii] == XferID) {			/* Yes, I'm the owner, release it				*/
			Cfg->EvtOwn[ii] = 0;
			Cfg->InUse[ii]  = 0;
			Cfg->ReUse[ii]  = 0;
			Cfg->ChCfg[ii].Started = 1;				/* This is never be set with event, for safety	*/
		}
	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: dma_done																				*/
/*																									*/
/* dma_done - report if a DMA transfer is done														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int dma_done(int XferID);																	*/
/*																									*/
/* ARGUMENTS:																						*/
/*		XferID : ID of the transfer to report if done or not										*/
/*																									*/
/* RETURN VALUE:																					*/
/*		<  0 : invalid transfer ID																	*/
/*		== 0 : transfer not yet done																*/
/*		>  0 : transfer done																		*/
/*																									*/
/* IMPLEMENTATION:																					*/
/* ------------------------------------------------------------------------------------------------ */

int dma_done(int XferID)
{
DMAcfg_t *Cfg;										/* Configuration for the requested DMA device	*/
int       Chan;										/* DMA channel number							*/
int       ii;										/* General purpose								*/
int       RetVal;									/* Return value									*/
volatile uint32_t *Reg;								/* Base address of the DMA device registers		*/
#if ((DMA_USE_MUTEX) != 0)
  int     ISRstate;									/* ISR state before disable for exclusive access*/
#endif

  #if ((DMA_ARG_CHECK) != 0)						/* Arg check, remapping without checkings		*/
	if ((XferID >> 16) != 0xC) {					/* Make sure the transfer ID is valid			*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_done() invalid ID");
	  #endif
		return(-1);
	}
  #endif

	RetVal = 0;										/* Assume it's not done yet						*/

	Cfg    = &g_DMAcfg[g_DevReMap[DMA_ID_DEV(XferID)]];	/* Select the device/controller config		*/
	Chan   = DMA_ID_CHN(XferID);					/* Extract the channel							*/
	Reg    = Cfg->HW;								/* Registers for this device/controller			*/

	if ((XferID != Cfg->InUse[Chan])				/* When is new transfer, requested one is done	*/
	&&  (Cfg->InUse[Chan] != 0U)) {					/* for sure as a new transfer has been set-up	*/
		RetVal = 1;									/* and replaced the requested one				*/
	}
	else {											/* Ok channel to check if done					*/
		if (Cfg->ChCfg[Chan].Started != 0) {		/* Must be started for done to be valid			*/

			DMA_LOCK(Cfg, ISRstate);				/* Exclusive access to the DMA registers		*/

			ii = 0xF								/* Extract the channel state					*/
			    & Cfg->HW[DMA_CSRn_REG(Chan)];

			if ((ii == 0xF)							/* DMA Faulting									*/
			||  (ii == 0xE)) {						/* DMA Faulting completing						*/
				REG_LOCK(Cfg, ISRstate);

				while((Reg[DMA_DBGSTATUS_REG] & 1U) != 0U);

				Reg[DMA_DBGINST0_REG] = (0x01 << 16)	/* Byte #0 DMAKILL command					*/
				                      | (Chan << 8)		/* Target DMA channel						*/
				                      | 1U;				/* DMA channel debug operation				*/
				Reg[DMA_DBGINST1_REG] = 0U;
				Reg[DMA_DBGCMD_REG]   = 0U;			/* Execute the commands in DBGINST[1:0]			*/

				REG_UNLOCK(Cfg, ISRstate);

				RetVal = 1;
			}
			else if (ii == 0) {						/* DMA reports it is Idle						*/
				RetVal = 1;
			}										/* In case ISR did not kick in e.g. ISR disable	*/

			DMA_UNLOCK(Cfg, ISRstate);				/* Release exclusive access						*/
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: dma_event																				*/
/*																									*/
/* dma_event - send an event to a dma channel														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int dma_event(int XferID);																	*/
/*																									*/
/* ARGUMENTS:																						*/
/*		XferID : ID of the transfer to send the event to											*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/* ------------------------------------------------------------------------------------------------ */

int dma_event(int XferID)
{
DMAcfg_t *Cfg;										/* Configuration for the requested DMA device	*/
int       Chan;										/* DMA channel number							*/
volatile uint32_t *Reg;								/* Base address of the DMA device registers		*/
#if ((DMA_USE_MUTEX) != 0)
  int     ISRstate;									/* ISR state before disable for exclusive access*/
#endif

  #if ((DMA_ARG_CHECK) != 0)						/* Arg check, remapping without checkings		*/
	if ((XferID >> 16) != 0xC) {					/* Make sure the transfer ID is valid			*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_event() invalid ID");
	  #endif
		return(-1);
	}
  #endif

	Cfg    = &g_DMAcfg[g_DevReMap[DMA_ID_DEV(XferID)]];
	Chan   = DMA_ID_CHN(XferID);
	Reg    = Cfg->HW;

	DMA_LOCK(Cfg, ISRstate);						/* Exclusive access to the DMA registers		*/
	REG_LOCK(Cfg, ISRstate);

	while((Reg[DMA_DBGSTATUS_REG] & 1U) != 0U);

	Reg[DMA_DBGINST0_REG] = ((DMA_ID_EVT(XferID)) << 24)	/* Event number							*/
	                      | (0x34 << 16)			/* Byte #0 DMASEV command						*/
	                      | (Chan << 8)				/* Target DMA channel							*/
	                      | 1U;						/* DMA channel debug operation					*/
	Reg[DMA_DBGINST1_REG] = 0U;
	Reg[DMA_DBGCMD_REG]   = 0U;						/* Execute the commands in DBGINST[1:0]			*/

	REG_UNLOCK(Cfg, ISRstate);						/* Release exclusive access						*/
	DMA_UNLOCK(Cfg, ISRstate);						/* Release exclusive access						*/

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: dma_kill																				*/
/*																									*/
/* dma_kill - kill / abort a DMA transfer / release													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int dma_kill(int XferID);																	*/
/*																									*/
/* ARGUMENTS:																						*/
/*		XferID : ID of the transfer to kill / abort													*/
/*																									*/
/* RETURN VALUE:																					*/
/*		<  0 : error																				*/
/*		== 0 : success																				*/
/*		>  0 : transfer was already completed and released 											*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		Need to use a sub-function for re-entrance in case spinlocks are used as protection			*/
/* ------------------------------------------------------------------------------------------------ */

int dma_kill(int XferID)
{

	if (XferID == 0) {								/* XferID == 0 accepted as OK as it simplifies	*/
		return(0);									/* the calling when a DMA was not set-up		*/
	}

  #if ((DMA_ARG_CHECK) != 0)						/* Arg check, remapping without checkings		*/
	if ((XferID >> 16) != 0xC) {					/* Make sure the transfer ID is valid			*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_kill() invalid ID");
	  #endif
		return(-1);
	}
  #endif

	return(dma_kill_X(XferID, 0));					/* Need to use a sub because of re-eentrance	*/
}													/* spinlock can only be locked once				*/

/* ------------------------------------------------------------------------------------------------ */

static int dma_kill_X(int XferID, int Reent)
{
int       Chan;										/* DMA channel number							*/
DMAcfg_t *Cfg;										/* Configuration for the requested DMA device	*/
int       Dev;										/* DMA device number (DMA controller)			*/
uint32_t  Dep;										/* Bit field of dependent transfers				*/
int32_t  *InUse;									/* CFG InUse field (XferID or 0)				*/
int       ReMap;									/* Index remap of device # to configuration #	*/
volatile uint32_t *Reg;								/* Base address of the DMA device registers		*/
#if ((DMA_USE_MUTEX) != 0)
  int     ISRstate;									/* ISR state before disable for exclusive access*/
#endif

  #if ((DMA_ARG_CHECK) != 0)
	if ((XferID >> 16) != 0xC) {					/* Make sure the transfer ID is valid			*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_kill_X() invalid ID");
	  #endif
		return(-1);
	}
  #endif

	Dev   = DMA_ID_DEV(XferID);
  #if ((DMA_ARG_CHECK) != 0)
	if ((Dev < 0)									/* Check the validity of "Dev"					*/
	||  (Dev >= (DMA_MAX_DEVICES))) {
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_kill_X() out of range device");
	  #endif
		return(-2);
	}
  #endif

	ReMap = g_DevReMap[Dev];
  #if ((DMA_ARG_CHECK) != 0)
	if (ReMap < 0) {								/* Check the validitiy of ReMap					*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_kill_X() cannot remap device");
	  #endif
		return(-3);
	}
  #endif

	Cfg   = &g_DMAcfg[ReMap];
	Chan  = DMA_ID_CHN(XferID);
	Reg   = Cfg->HW;

	if (Cfg->InUse[Chan] != XferID) {				/* Make sure the channel is for this transfer	*/
		return(1);									/* Has been completed (0) or is a new transfer	*/
	}

	if (Reent++ == 0) {								/* Need to do this because if spinlocks are		*/
		DMA_LOCK(Cfg, ISRstate);					/* used, they are not re-entrant				*/
		REG_LOCK(Cfg, ISRstate);
	}

	while((Reg[DMA_DBGSTATUS_REG] & 1U) != 0U);		/* Wait for the debug of the DMA to be idle		*/

	Reg[DMA_DBGINST0_REG] = (0x01 << 16)			/* Byte #0 DMAKILL command, default secure mode	*/
	                      | (Chan << 8)				/* Target DMA channel							*/
	                      | 1U;						/* DMA channel debug operation					*/
	Reg[DMA_DBGINST1_REG] = 0U;
	Reg[DMA_DBGCMD_REG]   = 0U;						/* Execute the commands in DBGINST[1:0]			*/

	if (--Reent == 0) {
		REG_UNLOCK(Cfg, ISRstate);
	}

	Dep = Cfg->EvtSent[Chan];

	if (Cfg->ReUse[Chan] == 0U) {					/* If this configuration is to be re-used		*/
		Cfg->EvtSent[Chan] = 0;
	}

	InUse = &(Cfg->InUse[0]);
	while (Dep != 0U) {								/* Kill all my dependents, their dependents, etc*/
		if (((Dep & 1U) != 0U)
		&&  (*InUse != 0)) {						/* This one is for safety						*/
			dma_kill_X(*InUse, Reent);
		}
		Dep >>= 1;
		InUse++;
	}

	dma_cleanup(Cfg, XferID);						/* Full cleanup									*/

	if (Reent == 0) {
		DMA_UNLOCK(Cfg, ISRstate);
	}

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: dma_pend_trig																			*/
/*																									*/
/* dma_pend_trig - set pending trigger																*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int dma_pend_trig(int XferID, int IsEnd, int ToTrig);										*/
/*																									*/
/* ARGUMENTS:																						*/
/*		XferID : ID of the transfer to set the pending trigger										*/
/*		IsEnd  : == 0 set trigger at start															*/
/*		         != 0 set trigger ate end															*/
/*		ToTrig : ID of t he transfer to send the trigger to											*/
/*																									*/
/* RETURN VALUE:																					*/
/*		<  0 : error																				*/
/*		== 0 : success																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/* ------------------------------------------------------------------------------------------------ */

int dma_pend_trig(int XferID, int IsEnd, int ToTrig)
{
int       Chan;										/* DMA channel number							*/
CHNcfg_t *ChCfg;									/* Cahnnel configuration						*/
DMAcfg_t *Cfg;										/* Configuration for the requested DMA device	*/
int       Dev;										/* DMA device number (DMA controller)			*/
int       ii;										/* General purpose								*/
int       ReMap;									/* Index remap of device # to configuration #	*/
int       RetVal;
uint8_t  *TmpPtr;
#if ((DMA_USE_MUTEX) < 0)
  int       ISRstate;								/* ISR state before disable for exclusive access*/
#endif

  #if ((DMA_ARG_CHECK) != 0)
	if (((XferID >> 16) != 0xC)						/* Make sure the transfer ID is valid			*/
	||  ((ToTrig >> 16) != 0xC)) {
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_pend_trig() invalid ID");
	  #endif
		return(-1);
	}
  #endif

	Dev = DMA_ID_DEV(XferID);
  #if ((DMA_ARG_CHECK) != 0)
	ii = DMA_ID_DEV(ToTrig);
	if ((Dev < 0)									/* Check the validity of "Dev"					*/
	||  (Dev >= (DMA_MAX_DEVICES))
	||  (ii  < 0)
	||  (ii  >= (DMA_MAX_DEVICES))) {
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_pend_trig() out of range device");
	  #endif
		return(-2);
	}
  #endif

	ReMap = g_DevReMap[Dev];
  #if ((DMA_ARG_CHECK) != 0)
	ii = g_DevReMap[ii];
	if ((ReMap < 0)									/* Check the validitiy of ReMap					*/
	||  (ii    < 0)
	||  (ii != ReMap)) {
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_pend_trig() cannot remap device");
	  #endif
		return(-3);
	}
  #endif

	Cfg   = &g_DMAcfg[ReMap];
	Chan  = DMA_ID_CHN(XferID);
	ChCfg = &Cfg->ChCfg[Chan]; 

	if (Cfg->InUse[Chan] != XferID) {				/* Make sure the channel is for this transfer	*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_pend_trig() transfer was completed");
	  #endif
		return(-4);									/* Has been completed (0) or is a new transfer	*/
	}

	RetVal = -5;									/* Assume no more pending triggers				*/
	DMA_LOCK(Cfg, ISRstate);

	if (IsEnd != 0) {
		for (ii=0 ; ii<(sizeof(ChCfg->PendEnd)/sizeof(ChCfg->PendEnd[0])) ; ii++) {
			if (ChCfg->PendStart[ii] != NULL) {
				TmpPtr               = ChCfg->Pgm.BufNow;
				ChCfg->Pgm.BufNow    = ChCfg->PendStart[ii];
				DMAasm(&ChCfg->Pgm, OP_DMASEV, (uint32_t)DMA_ID_EVT(ToTrig));
				ChCfg->Pgm.BufNow    = TmpPtr;
				ChCfg->PendStart[ii] = NULL;
				RetVal = 0;
				break;
			}
		}
	}
	else {
		for (ii=0 ; ii<(sizeof(ChCfg->PendStart)/sizeof(ChCfg->PendStart[0])) ; ii++) {
			if (ChCfg->PendStart[ii] != NULL) {
				TmpPtr               = ChCfg->Pgm.BufNow;
				ChCfg->Pgm.BufNow    = ChCfg->PendStart[ii];
				DMAasm(&ChCfg->Pgm, OP_DMASEV, (uint32_t)DMA_ID_EVT(ToTrig));
				ChCfg->Pgm.BufNow    = TmpPtr;
				ChCfg->PendStart[ii] = NULL;
				RetVal = 0;
				break;
			}
		}
	}

	DMA_UNLOCK(Cfg, ISRstate);

  #if ((DMA_DEBUG) != 0)
	if (RetVal != 0) {
		puts("DMA  - Error - dma_pend_trig() no more pending trigger were left");
	}
  #endif

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: dma_release																			*/
/*																									*/
/* dma_release - remove the info a configuration is to be re-used									*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int dma_release(int XferID);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		XferID : ID of the transfer to release														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		<  0 : error																				*/
/*		== 0 : success																				*/
/*		>  0 : transfer was already completed and released 											*/
/*																									*/
/* IMPLEMENTATION:																					*/
/* ------------------------------------------------------------------------------------------------ */

int dma_release(int XferID)
{
int       Chan;										/* DMA channel number							*/
DMAcfg_t *Cfg;										/* Configuration for the requested DMA device	*/
uint32_t  Dep;										/* Bit field of dependent transfers				*/
int       Dev;										/* DMA device number (DMA controller)			*/
int       ReMap;									/* Index remap of device # to configuration #	*/
int       RetVal;
int32_t  *ReUse;									/* CFG ReUsed field (XferID or 0				*/
#if ((DMA_USE_MUTEX) < 0)
  int       ISRstate;								/* ISR state before disable for exclusive access*/
#endif

  #if ((DMA_ARG_CHECK) != 0)
	if ((XferID >> 16) != 0xC) {					/* Make sure the transfer ID is valid			*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_release() invalid ID");
	  #endif
		return(-1);
	}
  #endif

	Dev   = DMA_ID_DEV(XferID);
  #if ((DMA_ARG_CHECK) != 0)
	if ((Dev < 0)									/* Check the validity of "Dev"					*/
	||  (Dev >= (DMA_MAX_DEVICES))) {
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_release() out of range device");
	  #endif
		return(-2);
	}
  #endif

	ReMap = g_DevReMap[Dev];
  #if ((DMA_ARG_CHECK) != 0)
	if (ReMap < 0) {								/* Check the validitiy of ReMap					*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_release() cannot remap device");
	  #endif
		return(-3);
	}
  #endif

	Cfg   = &g_DMAcfg[ReMap];
	Chan  = DMA_ID_CHN(XferID);

	if (Cfg->InUse[Chan] != XferID) {				/* Make sure the channel is for this transfer	*/
		return(1);									/* Has been completed (0) or is a new transfer	*/
	}

	DMA_LOCK(Cfg, ISRstate);

	Dep         = Cfg->EvtSent[Chan];
	Dev         = 1;
	ReUse       = &(Cfg->ReUse[0]);
	ReUse[Chan] = 0U;

	while (Dep != 0U) {								/* Remove reuse tag to all my dependnets, and	*/
		if ((Dep & 1) != 0U) {						/* theirs, etc									*/
			*ReUse = 0U;
		}
		Dep >>= 1;
		Dev <<= 1;
		ReUse++;
	}

	RetVal = dma_kill_X(XferID, 1);					/* Make sure DMA xfers stopped then free all	*/

	DMA_UNLOCK(Cfg, ISRstate);

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: dma_start																				*/
/*																									*/
/* dma_start - start a DMA transfer																	*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int dma_start(int XferID);																	*/
/*																									*/
/* ARGUMENTS:																						*/
/*		XferID : ID of the transfer to start														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		<  0 : error																				*/
/*		== 0 : success																				*/
/*		>  0 : transfer was started and is still going on 											*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		Need to use a sub-function for re-entrance in case spinlocks are used as protection			*/
/* ------------------------------------------------------------------------------------------------ */

int  dma_start(int XferID)
{
DMAcfg_t *Cfg;										/* Configuration for the requested DMA device	*/
int       ReMap;									/* Index remap of device # to configuration #	*/

  #if ((DMA_ARG_CHECK) != 0)
	if ((XferID >> 16) != 0xC) {					/* Make sure the transfer ID is valid			*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_start() invalid ID");
	  #endif
		return(-1);
	}

	if (g_DevReMap[DMA_ID_DEV(XferID)] < 0) {		/* Check the validitiy of ReMap					*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_start() cannot remap device");
	  #endif
		return(-2);
	}
  #endif

	ReMap = g_DevReMap[DMA_ID_DEV(XferID)];
	Cfg   = &g_DMAcfg[ReMap];

	return(dma_start_X(Cfg, XferID, 0));
}

/* ------------------------------------------------------------------------------------------------ */

static int dma_start_X(DMAcfg_t *Cfg, int XferID, int Reent)
{
int       Chan;										/* DMA channel number							*/
CHNcfg_t *ChCfg;									/* Channel configuration						*/
uint32_t  Dep;										/* Bit field of dependent transfers				*/
int32_t  *InUse;									/* CFG InUsed field (XferID or 0				*/
volatile uint32_t *Reg;								/* Base address of the DMA device registers		*/
int32_t  *ReUse;									/* CFG ReUsed field (XferID or 0				*/
#if ((DMA_USE_MUTEX) != 0)
  int     ISRstate = 0;								/* ISR state before disable for exclusive access*/
#endif

  #if ((DMA_ARG_CHECK) != 0)
	if ((XferID >> 16) != 0xC) {					/* Make sure the transfer ID is valid			*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_start() invalid ID");
	  #endif
		return(-1);
	}
  #endif

	Chan  = DMA_ID_CHN(XferID);
	ChCfg = &Cfg->ChCfg[Chan];
	Reg   = Cfg->HW;
	Dep   = Cfg->EvtSent[Chan];
	InUse = &(Cfg->InUse[0]);
	ReUse = &(Cfg->ReUse[0]);

	if (Reent++ == 0) {								/* Need to do this because if spinlocks are		*/
		if (ChCfg->Started != 0) {					/* used, they are not re-entrant				*/
			return(1);								/* This channel was started &  still going on	*/
		}
		DMA_LOCK(Cfg, ISRstate);					
	}

	while (Dep != 0U) {								/* Start all my dependents, and theirs, etc		*/
		if (((Dep & 1) != 0U)						/* Bit set, this is one of my dependents		*/
		&&  (*InUse != 0U)) {						/* And it is in-use (safety)					*/
			if (ReUse[Chan] != 0U) {				/* This allows to not specify re-use for leaf	*/
				*ReUse = *InUse;					/* transfers									*/
			}
			dma_start_X(Cfg, *InUse, Reent);
		}
		Dep >>= 1;
		InUse++;
		ReUse++;
	}

	ChCfg->AddDst  = ChCfg->AddDstOri;				/* Need to put back when configured as reuse	*/
	ChCfg->Started = 1;;							/* and transfer(s) have already happened		*/

	if (ChCfg->AddDst != NULL) {					/* Flush in case the xfer not cache aligned		*/
		DCacheFlushRange((void *)ChCfg->AddDst, ChCfg->DstSize);
	}
	if (ChCfg->AddSrc != NULL) {
		DCacheFlushRange((void *)ChCfg->AddSrc, ChCfg->SrcSize);
	}

  #if (((DMA_OPERATION) & 1) != 0)
	SEMreset(ChCfg->MySema);						/* Make sure there are no pending postings		*/
  #endif

	if (--Reent == 0) {								/* Lock it right before accessing register to	*/
		REG_LOCK(Cfg, ISRstate);					/* not disable ISR for too long					*/
	}

	Reg[DMA_INTCLR_REG] = 1<<Chan;					/* Clear the interrupt request					*/

	while((Reg[DMA_DBGSTATUS_REG] & 1U) != 0U);		/* Wait for the debug of the DMA to be idle		*/

	Reg[DMA_DBGINST0_REG] = (Chan << 24)			/* Byte #0 DMAGO command, default secure mode	*/
	                      | (0xA0 << 16);			/* Argument to DMAGO (3 LSbits are channel #)	*/
	Reg[DMA_DBGINST1_REG] = (uintptr_t)ChCfg->Pgm.BufBase;			/* Byte 2->5					*/
	Reg[DMA_DBGCMD_REG]   = 0U;						/* Execute the commands in DBGINST[1:0]			*/

	if (Reent == 0) {
		REG_UNLOCK(Cfg, ISRstate);					
		DMA_UNLOCK(Cfg, ISRstate);
	}

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: dma_state																				*/
/*																									*/
/* dma_state - report the state of the DMA															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int dma_state(int XferID, int Disp);														*/
/*																									*/
/* ARGUMENTS:																						*/
/*		XferID : ID of the transfer to report the state												*/
/*		Disp   : Boolean requesting to display all channel register of the transfer ID				*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : state value - CSR register for that transfer											*/
/*		<  0 : invalid transfer ID																	*/
/*																									*/
/* IMPLEMENTATION:																					*/
/* ------------------------------------------------------------------------------------------------ */

int dma_state(int XferID, int Disp)
{
DMAcfg_t *Cfg;										/* Configuration for the requested DMA device	*/
int       Chan;										/* DMA channel number							*/

  #if ((DMA_ARG_CHECK) != 0)
	if ((XferID >> 16) != 0xC) {					/* Make sure the transfer ID is valid			*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_state() invalid ID");
	  #endif
		return(-1);
	}

	if (g_DevReMap[DMA_ID_DEV(XferID)] < 0) {		/* Check the validitiy of ReMap					*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_state() cannot remap device");
	  #endif
		return(-2);
	}
  #endif

	Cfg  = &g_DMAcfg[g_DevReMap[DMA_ID_DEV(XferID)]];

	Chan = DMA_ID_CHN(XferID);

	if (Disp != 0) {
		printf("DMA  - channel #%d register dump\n", Chan);
		printf("DSR  : %08X\n",        (unsigned int)Cfg->HW[DMA_DSR_REG]);
		printf("FSRD : %08X\n",        (unsigned int)Cfg->HW[DMA_FSRD_REG]);
		printf("FSRC : %08X\n",        (unsigned int)Cfg->HW[DMA_FSRC_REG]);
		printf("FTRD : %08X\n",        (unsigned int)Cfg->HW[DMA_FTRD_REG]);
		printf("EVNT : %08X\n",        (unsigned int)Cfg->HW[DMA_INT_EVENT_RIS_REG]);
		printf("CCR_%d: %08X\n", Chan, (unsigned int)Cfg->HW[DMA_CCRn_REG(Chan)]);
		printf("SAR_%d: %08X\n", Chan, (unsigned int)Cfg->HW[DMA_SARn_REG(Chan)]);
		printf("DAR_%d: %08X\n", Chan, (unsigned int)Cfg->HW[DMA_DARn_REG(Chan)]);
		printf("FTR_%d: %08X\n", Chan, (unsigned int)Cfg->HW[DMA_FTRn_REG(Chan)]);
		printf("CSR_%d: %08X\n", Chan, (unsigned int)Cfg->HW[DMA_CSRn_REG(Chan)]);
		printf("CPC_%d: %08X\n", Chan, (unsigned int)Cfg->HW[DMA_CPCn_REG(Chan)]);
		printf("LC0_%d: %08X\n", Chan, (unsigned int)Cfg->HW[DMA_LC0n_REG(Chan)]);
		printf("LC1_%d: %08X\n", Chan, (unsigned int)Cfg->HW[DMA_LC1n_REG(Chan)]);
	}

	return(Cfg->HW[DMA_CSRn_REG(Chan)] & 0xF);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: dma_wait																				*/
/*																									*/
/* dma_wait - wait for a DMA transfer to be done													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int dma_wait(int XferID, int TimeOut, int UseISR);											*/
/*																									*/
/* ARGUMENTS:																						*/
/*		XferID : ID of the transfer to wait for														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		<  0 : not a valid transfer ID																*/
/*		== 0 : done																					*/
/*		>  0 : timeout																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/* ------------------------------------------------------------------------------------------------ */

int dma_wait(int XferID)
{
DMAcfg_t *Cfg;										/* Configuration for the requested DMA device	*/
CHNcfg_t *ChCfg;									/* Channel configuration						*/
int       Chan;										/* DMA channel number							*/
int       Expiry;									/* Value of G_OStimCnt for polling time out		*/
void     (*FctEnd)(intptr_t);						/* Need interim function ptr for DMA_OPEND_FCT	*/
int       RetVal;									/* Return value									*/
int       TimeOut;
int       WaitChan;
int       WaitID;									/* Channel ID to wait on						*/
#if ((DMA_USE_MUTEX) < 0)
  int     ISRstate;									/* ISR state before disable for exclusive access*/
#endif

  #if ((DMA_ARG_CHECK) != 0)
	if ((XferID >> 16) != 0xC) {					/* Make sure the transfer ID is valid			*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_wait() invalid ID");
	  #endif
		return(-1);
	}

	if (g_DevReMap[DMA_ID_DEV(XferID)] < 0) {		/* Check the validitiy of ReMap					*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - dma_wait() cannot remap device");
	  #endif
		return(-2);
	}
  #endif

	Chan     = DMA_ID_CHN(XferID);					/* Here, we know we have a valid ID				*/
	Cfg      = &g_DMAcfg[g_DevReMap[DMA_ID_DEV(XferID)]];
	WaitChan = Cfg->ChCfg[Chan].ChanWait;
	ChCfg    = &Cfg->ChCfg[WaitChan];
	TimeOut  = ChCfg->TimeOut;
	WaitID   = Cfg->InUse[WaitChan];				/* This is the transfer I must wait on			*/

  #if (((DMA_OPERATION) & 1) != 0)
	RetVal = 0;

	if ((WaitID != 0) 								/* If ISRs are used and already done in ISR		*/
	||  (ChCfg->WaitEnd == DMA_CFG_EOT_POLLING)		/* and no re-use, the Xfer ID has become 0		*/
	||  (ChCfg->Started  == 0))						/* We know this is the case if started bit is 1	*/
  #endif
	{
		RetVal = (0 == dma_done(WaitID));			/* DMA is done when > 0, error when <0			*/
	}

	if (RetVal != 0) {								/* dma_done() was 0, then is not done			*/
	  #if (((DMA_OPERATION) & 1) != 0)
		if (ChCfg->WaitEnd != DMA_CFG_EOT_POLLING) {
			RetVal = (0 != SEMwait(ChCfg->MySema, TimeOut));
		} else										/* != 0 to make sure to return +ve for timed out*/
	  #endif
		{											/* Requested to use polling			*/
			Expiry = OS_TICK_EXP(TimeOut+20);
			do {
				RetVal = (0 == dma_done(WaitID));
			} while ((RetVal != 0)
			  &&     (0 == OS_HAS_TIMEDOUT(Expiry)));
		}
	}

	if ((RetVal == 0)								/* If requested to perform an op at end do it	*/
	&&  (ChCfg->WaitEnd == DMA_CFG_EOT_POLLING)) {	/* Not need with ISR as done in ISR		*/
		switch(ChCfg->OpEnd) {						/* Operation to perform upon EOT				*/
		case DMA_OPEND_FCT:							/* Select what to perform according to request	*/
			FctEnd = (void (*)(intptr_t))ChCfg->PtrEnd;
			FctEnd(ChCfg->ValEnd);
			break;
		case DMA_OPEND_VAR:
			(*(volatile int *)ChCfg->PtrEnd) = ChCfg->ValEnd;
			break;
		case DMA_OPEND_SEM:
			SEMpost((SEM_t *)ChCfg->PtrEnd);
			break;
		case DMA_OPEND_MTX:
			MTXunlock((MTX_t *)ChCfg->PtrEnd);
			break;
	  #if ((OX_MAILBOX) != 0)
		case DMA_OPEND_MBX:
			MBXput((MBX_t *)ChCfg->PtrEnd, ChCfg->ValEnd, 0);
			break;
	  #endif
	  #if ((OX_EVENTS) != 0)
		case DMA_OPEND_EVT:
			EVTset((TSK_t *)ChCfg->PtrEnd, (intptr_t)ChCfg->ValEnd);
			break;
	  #endif
		default:									/* Anything else is a do nothing				*/
			break;
		}
	}

	if (RetVal == 0) {								/* Full clean-up 								*/
		DMA_LOCK(Cfg, ISRstate);
		dma_cleanup(Cfg, XferID);
		DMA_UNLOCK(Cfg, ISRstate);
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: dma_init																				*/
/*																									*/
/* dma_init - init the PL330 DMA																	*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int dma_init(int Dev);																		*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : SPI controller number																*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/* ------------------------------------------------------------------------------------------------ */

int dma_init(int Dev)
{
DMAcfg_t *Cfg;										/* Configuration for the requested DMA device	*/
int      ii;										/* General purpose								*/
int      ReMap;										/* Index remap of device # to configuration #	*/
volatile uint32_t *Reg;								/* Base address of the DMA device registers		*/
#if (((DMA_OPERATION) & 1) != 0)	
  SEM_t   *SemBack[DMA_NMB_CHAN];
#endif
#if ((DMA_USE_MUTEX) != 0)
  MTX_t   *MtxBack;
#endif
#if ((((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5) || (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10))
  uint32_t Tmp32;
  volatile int Dummy;
#endif


  #if ((DMA_DEBUG) > 1)
	printf("DMA  - Initializing   : Dev:%d\n", Dev);
  #endif

  #if ((DMA_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
  #else
	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (DMA_MAX_DEVICES))) {
		return(-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by DMA_LIST_DEVICE				*/
	  #if ((DMA_DEBUG) != 0)
		puts("DMA  - Error - cannot remap device");
	  #endif
		return(-2);
	}
  #endif

	if (g_CfgIsInit == 0) {							/* Must make sure all cfg are set to default	*/
		memset(&g_DMAcfg, 0, sizeof(g_DMAcfg));		/* And create mutexes & init unique ID counters	*/
		g_CfgIsInit = 1;
	  #if ((DMA_USE_MUTEX) != 0)
		g_MyMutex = MTXopen("DMA");
	  #endif
	}

	Cfg = &g_DMAcfg[ReMap];							/* This is this device configuration			*/

  #if ((DMA_USE_MUTEX) != 0)
	MTXlock(g_MyMutex, -1);
  #endif

	DMA_RESET(Dev);									/* Hardware reset								*/

	if (Cfg->IsInit != 0) {							/* If is not the first time it's initialized	*/
	  #if (((DMA_OPERATION) & 1) != 0)				/* Keep the semaphore descriptors and re-insert	*/
		for (ii=0 ; ii<(sizeof(Cfg->ChCfg)/sizeof(Cfg->ChCfg[0])) ; ii++) {
			SemBack[0] = Cfg->ChCfg[ii].MySema;
		}
	  #endif
	  #if ((DMA_USE_MUTEX) != 0)
		MtxBack = Cfg->MyMutex;
	  #endif

		memset(&g_DMAcfg[ReMap], 0, sizeof(g_DMAcfg[ReMap]));

	  #if (((DMA_OPERATION) & 1) != 0)
		for (ii=0 ; ii<(sizeof(Cfg->ChCfg)/sizeof(Cfg->ChCfg[0])) ; ii++) {
			Cfg->ChCfg[ii].MySema = SemBack[0];
		}
	  #endif
	  #if ((DMA_USE_MUTEX) != 0)
		Cfg->MyMutex = MtxBack;
	  #endif
	}
	else {											/* First time been initialized					*/
	  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)/* Cyclone V & Arria 5							*/
		*((volatile uint32_t *)0xFFD05014) |=  (1U << 28);	/* DMA reset							*/
		for (Dummy=0 ; Dummy<100 ; Dummy++);
		*((volatile uint32_t *)0xFFD05014) &= ~(1U << 28);
	  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)	/* Arria 10									*/
		*((volatile uint32_t *)0xFFD05024) |=  ((0xFFU << 24)	/* DMA FPGA I/F reset				*/
		                                   |    (1U    << 22) 	/* DMA ECC 							*/
		                                   |    (1U    << 16));	/* PL330 reset						*/
		for (Dummy=0 ; Dummy<100 ; Dummy++);
		*((volatile uint32_t *)0xFFD05024) &= ~((0xFFU << 24)	/* DMA FPGA I/F reset				*/
		                                   |    (1U    << 22) 	/* DMA ECC 							*/
		                                   |    (1U    << 16));	/* PL330 reset						*/
	  #endif
	  #if (((DMA_OPERATION) & 1) != 0)
		for (ii=0 ; ii<(sizeof(Cfg->ChCfg)/sizeof(Cfg->ChCfg[0])) ; ii++) {
			Cfg->ChCfg[ii].MySema = SEMopen(&g_SemNames[Dev][ii][0]);
		}
	  #endif
	  #if ((DMA_USE_MUTEX) > 0)
			Cfg->MyMutex = MTXopen(&g_DmaNames[Dev][0]);
	  #endif
	}

	Reg     = g_HWaddr[g_DevReMap[Dev]];
	Cfg->HW = Reg;

  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)	/* Cyclone V & Arria 5 							*/
	Tmp32 =  (0 << 4)								/* DMA manager in secure mode					*/
	  #if ((DMA_CY5_CAN_FPGA_4) != 0)
	      | 1
	  #endif
	  #if ((DMA_CY5_CAN_FPGA_5) != 0)
	      | 2
	  #endif
	  #if ((DMA_CY5_CAN_FPGA_6) != 0)
	      | 4
	  #endif
	  #if ((DMA_CY5_CAN_FPGA_7) != 0)
	      | 8
	  #endif
		  | 0;//(0xFF << 5);						/* Non-secure mode are 1's						*/
	*((volatile uint32_t *)0xFFD08070) = Tmp32;		/* All peripherals secure state					*/
	*((volatile uint32_t *)0xFFD08074) = 0x0;		/* All peripherals secure state					*/

	*((volatile uint32_t *)0xFFD05014) |=  (1U << 28);	/* Reset the DMA module						*/
	for (Dummy=0 ; Dummy<100 ; Dummy++);
	*((volatile uint32_t *)0xFFD05014) &= ~(1U << 28);

  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)	/* Arria 10										*/
	Tmp32 =  (0 << 4)								/* DMA manager in secure mode					*/
	  #if ((DMA_AR10_I2C4RX_FPGA_7) != 0)
	      | (1<<0)
	  #endif
	  #if ((DMA_AR10_I2C4TX_FPGA_6) != 0)
	      | (1<<4)
	  #endif
	  #if ((DMA_AR10_DMASEC_EMAC_FPGA_5) != 0)
	      | (1<<8);
	  #endif
	      | 0;//((0xFF<<24)|(1<<16))				/* Non-secure mode are 1's						*/
	*((volatile uint32_t *)0xFFD06020) = Tmp32;		/* All peripherals secure state					*/
	*((volatile uint32_t *)0xFFD06024) = 0x0;		/* All peripherals secure state					*/

	*((volatile uint32_t *)0xFFD05024) |=  (1U << 16);	/* Reset the DMA module						*/
	for (Dummy=0 ; Dummy<100 ; Dummy++);
	*((volatile uint32_t *)0xFFD05024) &= ~(1U << 16);

  #endif

	Cfg->ChanCnt   = 1 + ((Cfg->HW[DMA_CR0_REG]>> 4) & 0x07);
	if (Cfg->ChanCnt > DMA_NMB_CHAN) {
		Cfg->ChanCnt = DMA_NMB_CHAN;
	}

	Cfg->PeriphCnt = 0;
	if ((Cfg->HW[DMA_CR0_REG] & 1) != 0) {
		Cfg->PeriphCnt = 1 + ((Cfg->HW[DMA_CR0_REG]>>12) & 0x1F);
	}

	Cfg->EvtIrqCnt = 1 + ((Cfg->HW[DMA_CR0_REG]>>17) & 0x1F);
	if (Cfg->EvtIrqCnt > DMA_NMB_EVTIRQ) {
		Cfg->EvtIrqCnt = DMA_NMB_EVTIRQ;
	}

	for (ii=0 ; ii<(sizeof(Cfg->ChCfg)/sizeof(Cfg->ChCfg[0])) ; ii++) {
		Cfg->ChCfg[ii].Pgm.BufBase = (uint8_t *)(((((uintptr_t)(&Cfg->ChCfg[ii].Pgm.Program[0]))
		                           + ((OX_CACHE_LSIZE)-1)))
		                          &  ~((OX_CACHE_LSIZE)-1));
	}

  #if ((DMA_DEBUG) > 1)
	printf("DMA  - # of channels  : %2d\n", Cfg->ChanCnt);
	printf("DMA  - # of periph    : %2d\n", Cfg->PeriphCnt);
	printf("DMA  - # of EVT / IRQ : %2d\n", Cfg->EvtIrqCnt);
  #endif

	Cfg->IsInit = 1;

  #if ((DMA_USE_MUTEX) != 0)
	MTXunlock(g_MyMutex);
  #endif

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: DMAasm																					*/
/*																									*/
/* DMAasm - generate an assembly instruction														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int DMAasm(struct _DMApgm_t *Pgm, int Op, ...);												*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Pgm  : data structure holding the information about the channel being programmed			*/
/*		Op   : DMA op-code																			*/
/*		...  : see NOTES																			*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* NOTES:																							*/
/*		This function deals with selecting the loop counter and also computes the backward jump		*/
/*		at the end of the loops.																	*/
/*		It also select the nf bit in DMALPEND depending if DAMLPor DMALPFE was used.				*/
/*																									*/
/* Usage for each op-code:																			*/
/*		DMAasm(Pgm, OP_DMAADDH, ARG_RA, Increment);													*/
/*		DMAasm(Pgm, OP_DMAADNH, ARG_SA, Increment);													*/
/*		DMAasm(Pgm, OP_DMAEND);																		*/
/*		DMAasm(Pgm, OP_DMAFLUSHP, Periph);															*/
/*		DMAasm(Pgm, OP_DMAGO, Channel);																*/
/*		DMAasm(Pgm, OP_DMALD);																		*/
/*		DMAasm(Pgm, OP_DMALDB);																		*/
/*		DMAasm(Pgm, OP_DMALDS);																		*/
/*		DMAasm(Pgm, OP_DMALDPB, Periph);															*/
/*		DMAasm(Pgm, OP_DMALDPS, Periph);															*/
/*		DMAasm(Pgm, OP_DMALP, LoopCount);															*/
/*		DMAasm(Pgm, OP_DMALPEND);																	*/
/*		DMAasm(Pgm, OP_DMALPENDB);																	*/
/*		DMAasm(Pgm, OP_DMALPENDS);																	*/
/*		DMAasm(Pgm, OP_DMALPFE);																	*/
/*		DMAasm(Pgm, OP_DMAKILL);																	*/
/*		DMAasm(Pgm, OP_DMAMOV, ARG_DAR, Value);														*/
/*		DMAasm(Pgm, OP_DMAMOV, ARG_SAR, Value);														*/
/*		DMAasm(Pgm, OP_DMAMOV, ARG_CCR, Value);														*/
/*		DMAasm(Pgm, OP_DMANOP);																		*/
/*		DMAasm(Pgm, OP_DMARMB);																		*/
/*		DMAasm(Pgm, OP_DMASEV, Event);																*/
/*		DMAasm(Pgm, OP_DMAST);																		*/
/*		DMAasm(Pgm, OP_DMASTB);																		*/
/*		DMAasm(Pgm, OP_DMASTS);																		*/
/*		DMAasm(Pgm, OP_DMASTPB, Periph);															*/
/*		DMAasm(Pgm, OP_DMASTPS, Periph);															*/
/*		DMAasm(Pgm, OP_DMASTZ);																		*/
/*		DMAasm(Pgm, OP_DMAWFE, Event, ARG_VALID);													*/
/*		DMAasm(Pgm, OP_DMAWFE, Event, ARG_INVALID);													*/
/*		DMAasm(Pgm, OP_DMAWFPB, Periph);															*/
/*		DMAasm(Pgm, OP_DMAWFPP, Periph);															*/
/*		DMAasm(Pgm, OP_DMAWFPS, Periph);															*/
/*		DMAasm(Pgm, OP_DMAWMB);																		*/
/* ------------------------------------------------------------------------------------------------ */

static int DMAasm(struct _DMApgm_t *Pgm, int Op, ...)
{
va_list  argp;
uint8_t *Buf;
uint32_t Tmp32;


	Buf = Pgm->BufNow;
													/* Check for program larger than avail area		*/
	if ((6+(uintptr_t)Buf) >= ((uintptr_t)(&Pgm->BufBase[DMA_PGM_SIZE]))) {
		return(-1);									/* Done at beginning as it allows using lots of	*/
	}												/* DMAasm and only check last one return value	*/
													/* 6 is largest instruction size				*/
	va_start(argp, Op);

	Tmp32 = va_arg(argp, uint32_t);

	switch(Op) {
	case OP_DMAADDH:
	case OP_DMAADNH:
		*Buf++ = ((uint8_t)Op)
		       | ((uint8_t)Tmp32);					/* Insert ra bit according to ARG_DA or ARG_SA	*/
		Tmp32  = va_arg(argp, uint32_t);
		*Buf++ = (uint8_t)((Tmp32 >>  0) & 0xFF);	/* Immediate 16 bit address						*/
		*Buf++ = (uint8_t)((Tmp32 >>  8) & 0xFF);
		break;

	case OP_DMAFLUSHP:
	case OP_DMALDPB:
	case OP_DMALDPS:
	case OP_DMASEV:									/* DMASEV <event>								*/
	case OP_DMAWFPB:
	case OP_DMAWFPP:
	case OP_DMAWFPS:
	case OP_DMASTPB:								/* Here due to conflict with DMALPENDB			*/
	case OP_DMASTPS:								/* with loop #0 & started with LPE				*/
		*Buf++ = (uint8_t)Op;
		*Buf++ = (uint8_t)(Tmp32 << 3);				/* Insert peripheral / event number				*/
		break;

	case OP_DMAMOV:									/* DMAMOV   ARG_SAD, <imm32>					*/
	case OP_DMAGO:
		*Buf++ = (uint8_t)Op;						/* ns bit is left untouched to operate in the	*/
		*Buf++ = (uint8_t)Tmp32;					/* default security mode						*/
		Tmp32  = va_arg(argp, uint32_t);
		*Buf++ = (uint8_t)(Tmp32 >>  0);
		*Buf++ = (uint8_t)(Tmp32 >>  8);
		*Buf++ = (uint8_t)(Tmp32 >> 16);
		*Buf++ = (uint8_t)(Tmp32 >> 24);
		break;

	case OP_DMALP:									/* DMALP										*/
		*Buf++ = ((uint8_t)Op)
		       | ((uint8_t)(Pgm->LoopNow << 1));
		*Buf++ = ((uint8_t)Tmp32-1);
		Pgm->LoopCnt[Pgm->LoopIdx]    = Pgm->LoopNow++;
		Pgm->LoopOp[Pgm->LoopIdx]     = Op;			/* Memo Op to control nf in DMALPEND			*/
		Pgm->LoopAddr[Pgm->LoopIdx++] = Buf;
		break;

	case OP_DMALPFE:								/* DMALPFE										*/
		Pgm->LoopOp[Pgm->LoopIdx]     = Op;			/* Memo Op to control nf in DMALPEND			*/
		Pgm->LoopAddr[Pgm->LoopIdx++] = Buf;
		break;


	case OP_DMALPEND:
//	case OP_DMALPENDB:
//	case OP_DMALPENDS:
		if (Pgm->LoopOp[--Pgm->LoopIdx] == OP_DMALPFE) {
			*Buf++ = OP_DMALPFE;
		}
		else {
			*Buf++ = ((uint8_t)(Op & 0xFF))
			       | ((Pgm->LoopOp[Pgm->LoopIdx] == OP_DMALP)
			         ? 0x10							/* Set nf if OP_DMALP started the loop			*/
			         : 0)
			       | (Pgm->LoopCnt[Pgm->LoopIdx] << 2);
			Pgm->LoopNow--;
		}
		*Buf   = ((uintptr_t)Buf)
		       - ((uintptr_t)Pgm->LoopAddr[Pgm->LoopIdx])
		       - 1;
		Buf++;
		break;

	case OP_DMAWFE:
		*Buf++  = (uint8_t)Op;
		*Buf    = (uint8_t)(Tmp32<<3);
		Tmp32   = va_arg(argp, uint32_t);
		*Buf++ |= (uint8_t)Tmp32;
		break;


	case OP_DCB:
		*Buf++ = (uint8_t)Tmp32;
		break;

	case OP_DCD:
		*Buf++ = (uint8_t)(Tmp32 >>  0);
		*Buf++ = (uint8_t)(Tmp32 >>  8);
		*Buf++ = (uint8_t)(Tmp32 >> 16);
		*Buf++ = (uint8_t)(Tmp32 >> 24);
		break;

	default:
//	case OP_DMAEND:									/* All 1 byte opcode with no arguments			*/
//	case OP_DMALD:
//	case OP_DMALDB:
//	case OP_DMALDS:
//	case OP_DMANOP:
//	case OP_DMAST:
//	case OP_DMASTB:
//	case OP_DMASTS:
//	case OP_DMASTZ:
//	case OP_DMAKILL:
//	case OP_DMARMB:
//	case OP_DMAWMB:
		*Buf++ = ((uint8_t)(Op & 0xFF));
		break;
	}

	va_end(argp);

	Pgm->BufNow = Buf;

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* Interrupt handlers for the DMA transfers															*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void DMAintHndl(int DevChan)
{
#if (((DMA_OPERATION) & 1) != 0)					/* Real processing only if ISRs are to be used	*/
DMAcfg_t *Cfg;										/* Configuration for the requested DMA device	*/
CHNcfg_t *ChCfg;									/* Channel configuration						*/
int       Chan;										/* DMA channel number							*/
void    (*FctEnd)(intptr_t);						/* Need interim function ptr for DMA_OPEND_FCT	*/
#if (((DMA_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1))
  int     zz;
#endif

	Cfg   = &g_DMAcfg[g_DevReMap[DevChan>>3]];		/* DMA device configration						*/
	Chan  = DevChan & 0x7;							/* The channel on that device					*/
	ChCfg = &Cfg->ChCfg[Chan];						/* Channel configuration						*/

	if ((Cfg->HW[DMA_INTMIS_REG] & (1U << Chan)) != 0) {	/* Make sure this channel int is raised	*/

	  #if (((DMA_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1))

	   #if ((OX_NESTED_INTS) != 0)					/* Get exclusive access first					*/
		{ int ISRstate;
		ISRstate = OSintOff();						/* and then set the OnGoing flag to inform the	*/
	   #endif										/* other cores to skip as I'm handlking the ISR	*/
		CORElock(DMA_SPINLOCK, &ChCfg->SpinLock, 0, COREgetID()+1);

		zz = ChCfg->ISRonGoing;						/* Memo if is current being handled				*/
		if (zz == 0) {
			ChCfg->ISRonGoing = 1;					/* Declare as being handle / will be handle		*/
		}

		COREunlock(DMA_SPINLOCK, &ChCfg->SpinLock, 0);
	  #if ((OX_NESTED_INTS) != 0)
		OSintBack(ISRstate);
		}
	  #endif

		if (zz != 0) {								/* Another core was handling it					*/
			return;
		}

	  #endif

		Cfg->HW[DMA_INTCLR_REG] = 1 << Chan;		/* Clear the interrupt request					*/

		if (Cfg->InUse[Chan] != 0U) {				/* Handle only if the channel is in used		*/
			switch(ChCfg->OpEnd) 		{			/* Operation to perform upon EOT				*/
			case DMA_OPEND_FCT:						/* Select what to perform according to request	*/
				FctEnd = (void (*)(intptr_t))ChCfg->PtrEnd;
			  #if ((((DMA_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1))  || ((OX_NESTED_INTS) != 0))
				FctEnd(ChCfg->ValEnd);				/* Reg access has already been locked			*/
			  #else
				{ int ISRstate;
				ISR_LOCK(Cfg, ISRstate);			/* Needed if dma_XXXX() used the fct called		*/
				FctEnd(ChCfg->ValEnd);
				ISR_UNLOCK(Cfg, ISRstate);
				}
			  #endif
				break;
			case DMA_OPEND_VAR:
				(*(volatile int *)ChCfg->PtrEnd) = ChCfg->ValEnd;
				break;
			case DMA_OPEND_SEM:
				SEMpost((SEM_t *)ChCfg->PtrEnd);
				break;
			case DMA_OPEND_MTX:
				MTXunlock((MTX_t *)ChCfg->PtrEnd);
				break;
		  #if ((OX_MAILBOX) != 0)
			case DMA_OPEND_MBX:
				MBXput((MBX_t *)ChCfg->PtrEnd, ChCfg->ValEnd, 0);
				break;
		  #endif
		  #if ((OX_EVENTS) != 0)
			case DMA_OPEND_EVT:
				EVTset((TSK_t *)ChCfg->PtrEnd, (intptr_t)ChCfg->ValEnd);
				break;
	 	 #endif
			default:								/* Anything else is a do nothing				*/
				break;
			}

			if (ChCfg->MySema != NULL) {			/* Post the channel semaphore					*/
				SEMpost(ChCfg->MySema);
			}

			Cfg->InUse[Chan] = Cfg->ReUse[Chan];
		}

	  #if (((DMA_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1))
		ChCfg->ISRonGoing = 0;						/* Report I'm not handling the interrupt 		*/
	  #endif
	}

#else												/* ISRs not to be used, clear ISR for safety	*/
	g_DMAcfg[g_DevReMap[DevChan>>3]].HW[DMA_INTCLR_REG] = 1 << (DevChan & 0x7);

#endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Individual interrupt handlers macro. It calls the common handler with the device #				*/

#define DMA_INT_HNDL(Dev)																			\
																									\
	void DMAintHndl_##Dev(void)																		\
	{																								\
		DMAintHndl(Dev);																			\
		return;																						\
	}

/* ------------------------------------------------------------------------------------------------ */

	DMA_INT_HNDL(0)
	DMA_INT_HNDL(1)
	DMA_INT_HNDL(2)
	DMA_INT_HNDL(3)
	DMA_INT_HNDL(4)
	DMA_INT_HNDL(5)
	DMA_INT_HNDL(6)
	DMA_INT_HNDL(7)

/* EOF */
