/* ------------------------------------------------------------------------------------------------ */
/* FILE :		dw_ethernet.c																		*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the Designware Ethernet.													*/
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
/*	$Revision: 1.41 $																				*/
/*	$Date: 2019/02/10 16:54:18 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* NOTES:																							*/
/*																									*/
/* LIMITATIONS:																						*/
/*																									*/
/*			Supported PHYs:																			*/
/*				- Lantiq PEF7071		- token name PEF7071										*/
/*				- Marvel 88E1518		- token name 88E1510, 88E1512, 88E1514, 88E1518				*/
/*				- Micrel KSZ9021		- token name KSZ9021										*/
/*				- Micrel KSZ9031		- token name KSZ9031										*/
/*				- Micrel KSZ9071		- token name KSZ9031 (ss functionnaly same as KSZ0031)		*/
/*				- TI     DP83867		- token name TIDP83867										*/
/*																									*/
/* NOT YET SUPPORTED:																				*/
/*			MMU logical <--> physical remapping.  This means the DMA descriptors and data buffers	*/
/*			must have their logical addresses identical to their physical address					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* How to use this driver:																			*/
/*																									*/
/* NOTE: working examples for lwIP are located in:													*/
/*       lwip-f/dw/RTOS/ethernetif.c																*/
/*       lwip-f/dw/RTOS/ethernetif.h																*/
/*																									*/
/* Defines (in dw_ethernet.h)																		*/
/*																									*/
/*	ETH_N_RXBUF:	 Number of chained DMA descriptors in the RX direction							*/
/*	ETH_RX_BUFSIZE:	 Number of bytes in each DMA buffer in the RX direction							*/
/*	ETH_N_TXBUF:	 Number of chained DMA descriptors in the TX direction							*/
/*	ETH_TX_BUFSIZE:	 Number of bytes in each DMA buffer in the TX direction							*/
/*																									*/
/*	ETH_BUFFER_TYPE: Type memory for the DMA buffers												*/
/*	                 ETH_BUFFER_UNCACHED (Value == 0) Buffer in uncached memory						*/
/*	                 ETH_BUFFER_CACHED   (Value == 1) Buffers in cached memory						*/
/*	                 ETH_BUFFER_PBUF     (Value == 3) RX direction: lwIP pbuf in cached memory		*/
/*	                                                  TX direction: Buffers in cached memory		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* RX initialization:																				*/
/*	ETH_DMARxDescChainInit(Device);																	*/
/*																									*/
/* TX initialization:																				*/
/*	ETH_DMATxDescChainInit(Device);																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* RX with polling (full and segmented packets)														*/
/*																									*/
/*	do {																							*/
/*		Len = ETH_Get_Received_Frame_Length(Dev);													*/
/*	while (Len < 0);																				*/
/*																									*/
/*	"Select Memory to receive the packet"															*/
/*	do {																							*/
/*		Frame = ETH_Get_Received_Multi(Dev);														*/
/*      if (Frame.length >= 0) {																	*/
/*			"Concatenate Frame->buffer ((#bytes is Frame->length) to Memory"						*/
/*		}																							*/
/*		Len -= Frame->length;																		*/
/*	} while ((Frame->length >= 0)																	*/
/*	  &&     (Len > 0));																			*/
/*	ETH_Release_Received();																			*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* RX with interrupts (full and segmented packets)													*/
/*																									*/
/*	Len = ETH_Get_Received_Frame_Length(Dev);														*/
/*	if (Len >= 0) {																					*/
/*		"Select Memory to receive the packet"														*/
/*		do {																						*/
/*			Frame = ETH_Get_Received_Multi(Dev);													*/
/*          if (Frame->length >= 0) {																*/
/*				"Concatenate Frame->buffer ((#bytes is Frame->length) to Memory"					*/
/*			}																						*/
/*			Len -= Frame->length;																	*/
/*		} while ((Frame->length >= 0)																*/
/*		  &&     (Len > 0));																		*/
/*		ETH_Release_Received();																		*/
/*	}																								*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* TX (full packets only)																			*/
/*																									*/
/*	do {																							*/
/*		DMAbuf = ETH_Get_Transmit_Buffer();															*/
/*	} while (DMAbuf == NULL);																		*/
/*																									*/
/*	"Copy the Nbytes of the packet into DMAbuf"														*/
/*																									*/
/*	ETH_Prepare_Transmit_Descriptors(Dev, Nbytes);													*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* TX (full and segmented packets)																	*/
/*																									*/
/*	IsFirst = 1;																					*/
/*	IsLast  = 0;																					*/
/*																									*/
/*	while (Nbytes > 0) {																			*/
/*		do {																						*/
/*			DMAbuf = ETH_Get_Transmit_Buffer();														*/
/*		} while (DMAbuf == NULL);																	*/
/*		if (Nbytes > ETH_TX_BUFSIZE) {																*/
/*			Size = ETH_TX_BUFSIZE;																	*/
/*		}																							*/
/*		else {																						*/
/*			Size = Nbytes;																			*/
/*		}																							*/
/*		"Copy the next 'Size' bytes into DMAbuf"													*/
/*		if (Size == Nbytes) {																		*/
/*			IsLast = 1;																				*/
/*		}																							*/
/*		ETH_Prepare_Multi_Transmit(Dev, Nbytes, IsFirst, IsLast);									*/
/*		IsFirst = 0;																				*/
/*		Nbytes  = Nbytes - Size;																	*/
/*	}																								*/
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
#include "dw_ethernet.h"

#ifndef ETH_ALT_PHY_IF
  #define ETH_ALT_PHY_IF	 0						/* Boolean selecting if alternate PHY I/Fs used	*/
#endif

#ifndef ETH_ALT_PHY_MTX								/* <  0 : no mutex used when calling AltPhyIF()	*/
  #define ETH_ALT_PHY_MTX	 0						/* == 0 : one mutex per per device (std one)	*/
#endif												/* >  0 : one mutex for all devices				*/
													/* If multiple PHY are attached					*/
#ifndef ETH_MULTI_PHY								/* <  0 is a single pHY							*/
  #define ETH_MULTI_PHY		-1						/* >= 0 is the PHY that will report the rate 	*/
#endif												/*      to set-up the RGMII I/F					*/
													/* >= 32 use the lowest address PHY as set-up	*/
#ifndef ETH_DEBUG
  #define ETH_DEBUG			 0						/* Set to non-zero to print debug info			*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* CLock / Control / Data skews defintions 															*/

#ifndef ETH_0_SKEW_RXCTL							/* Dev #0 - Skew between clock & RX control		*/
  #define ETH_0_SKEW_RXCTL	-1
#endif
#ifndef ETH_0_SKEW_RXCLK							/* Dev #0 - RX Clock Skew						*/
  #define ETH_0_SKEW_RXCLK	-1
#endif
#ifndef ETH_0_SKEW_RX0								/* Dev #0 - Skew between clock & RX data pair 0	*/
  #define ETH_0_SKEW_RX0	-1
#endif
#ifndef ETH_0_SKEW_RX1								/* Dev #0 - Skew between clock & RX data pair 1	*/
  #define ETH_0_SKEW_RX1	-1
#endif
#ifndef ETH_0_SKEW_RX2								/* Dev #0 - Skew between clock & RX data pair 2	*/
  #define ETH_0_SKEW_RX2	-1
#endif
#ifndef ETH_0_SKEW_RX3								/* Dev #0 - Skew between clock & RX data pair 3	*/
  #define ETH_0_SKEW_RX3	-1
#endif
#ifndef ETH_0_SKEW_TXCTL							/* Dev #0 - Skew between clock & TX control		*/
  #define ETH_0_SKEW_TXCTL	-1
#endif
#ifndef ETH_0_SKEW_TXCLK							/* Dev #0 - TX Clock Skew						*/
  #define ETH_0_SKEW_TXCLK	-1
#endif
#ifndef ETH_0_SKEW_TX0								/* Dev #0 - Skew between clock & TX data pair 0	*/
  #define ETH_0_SKEW_TX0	-1
#endif
#ifndef ETH_0_SKEW_TX1								/* Dev #0 - Skew between clock & TX data pair 1	*/
  #define ETH_0_SKEW_TX1	-1
#endif
#ifndef ETH_0_SKEW_TX2								/* Dev #0 - Skew between clock & TX data pair 2	*/
  #define ETH_0_SKEW_TX2	-1
#endif
#ifndef ETH_0_SKEW_TX3								/* Dev #0 - Skew between clock & TX data pair 3	*/
  #define ETH_0_SKEW_TX3	-1
#endif


#ifndef ETH_1_SKEW_RXCTL							/* Dev #1 - Skew between clock & RX control		*/
  #define ETH_1_SKEW_RXCTL	-1
#endif
#ifndef ETH_1_SKEW_RXCLK							/* Dev #1 - RX Clock Skew						*/
  #define ETH_1_SKEW_RXCLK	-1
#endif
#ifndef ETH_1_SKEW_RX0								/* Dev #1 - Skew between clock & RX data pair 0	*/
  #define ETH_1_SKEW_RX0	-1
#endif
#ifndef ETH_1_SKEW_RX1								/* Dev #1 - Skew between clock & RX data pair 1	*/
  #define ETH_1_SKEW_RX1	-1
#endif
#ifndef ETH_1_SKEW_RX2								/* Dev #1 - Skew between clock & RX data pair 2	*/
  #define ETH_1_SKEW_RX2	-1
#endif
#ifndef ETH_1_SKEW_RX3								/* Dev #1 - Skew between clock & RX data pair 3	*/
  #define ETH_1_SKEW_RX3	-1
#endif
#ifndef ETH_1_SKEW_TXCTL							/* Dev #1 - Skew between clock & TX control		*/
  #define ETH_1_SKEW_TXCTL	-1
#endif
#ifndef ETH_1_SKEW_TXCLK							/* Dev #1 - TX Clock Skew						*/
  #define ETH_1_SKEW_TXCLK	-1
#endif
#ifndef ETH_1_SKEW_TX0								/* Dev #1 - Skew between clock & TX data pair 0	*/
  #define ETH_1_SKEW_TX0	-1
#endif
#ifndef ETH_1_SKEW_TX1								/* Dev #1 - Skew between clock & TX data pair 1	*/
  #define ETH_1_SKEW_TX1	-1
#endif
#ifndef ETH_1_SKEW_TX2								/* Dev #1 - Skew between clock & TX data pair 2	*/
  #define ETH_1_SKEW_TX2	-1
#endif
#ifndef ETH_1_SKEW_TX3								/* Dev #1 - Skew between clock & TX data pair 3	*/
  #define ETH_1_SKEW_TX3	-1
#endif


#ifndef ETH_2_SKEW_RXCTL							/* Dev #2 - Skew between clock & RX control		*/
  #define ETH_2_SKEW_RXCTL	-1
#endif
#ifndef ETH_2_SKEW_RXCLK							/* Dev #2 - RX Clock Skew						*/
  #define ETH_2_SKEW_RXCLK	-1
#endif
#ifndef ETH_2_SKEW_RX0								/* Dev #2 - Skew between clock & RX data pair 0	*/
  #define ETH_2_SKEW_RX0	-1
#endif
#ifndef ETH_2_SKEW_RX1								/* Dev #2 - Skew between clock & RX data pair 1	*/
  #define ETH_2_SKEW_RX1	-1
#endif
#ifndef ETH_2_SKEW_RX2								/* Dev #2 - Skew between clock & RX data pair 2	*/
  #define ETH_2_SKEW_RX2	-1
#endif
#ifndef ETH_2_SKEW_RX3								/* Dev #2 - Skew between clock & RX data pair 3	*/
  #define ETH_2_SKEW_RX3	-1
#endif
#ifndef ETH_2_SKEW_TXCTL							/* Dev #2 - Skew between clock & TX control		*/
  #define ETH_2_SKEW_TXCTL	-1
#endif
#ifndef ETH_2_SKEW_TXCLK							/* Dev #2 - TX Clock Skew						*/
  #define ETH_2_SKEW_TXCLK	-1
#endif
#ifndef ETH_2_SKEW_TX0								/* Dev #2 - Skew between clock & TX data pair 0	*/
  #define ETH_2_SKEW_TX0	-1
#endif
#ifndef ETH_2_SKEW_TX1								/* Dev #2 - Skew between clock & TX data pair 1	*/
  #define ETH_2_SKEW_TX1	-1
#endif
#ifndef ETH_2_SKEW_TX2								/* Dev #2 - Skew between clock & TX data pair 2	*/
  #define ETH_2_SKEW_TX2	-1
#endif
#ifndef ETH_2_SKEW_TX3								/* Dev #2 - Skew between clock & TX data pair 3	*/
  #define ETH_2_SKEW_TX3	-1
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Base addresses of the HW EMAC modules															*/

#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)		/* Arria V / Cyclone V							*/
  volatile uint32_t * const G_EMACaddr[2] = { (volatile uint32_t *)0xFF700000,
                                              (volatile uint32_t *)0xFF702000
                                            };

  #define EMAC_RESET(Dev)					do {													\
											  *(volatile uint32_t *)0xFFD05014 |=  1<<(Dev);		\
											  *(volatile uint32_t *)0xFFD08060 &=  ~(3<<(2*(Dev)));	\
											  *(volatile uint32_t *)0xFFD08060 |=    1<<(2*(Dev));	\
											  *(volatile uint32_t *)0xFFD05014 &=  ~(1<<(Dev));		\
											} while(0)

  #ifndef OX_CACHE_LSIZE							/* Abassi.h is not included. Put in #ifdef in	*/
	#define OX_CACHE_LSIZE			32				/* case it ever gets included					*/
  #endif


  #define UDELAY_MULT				(1000/6)		/* Cached: Multiplier for loop counter for 1 us	*/

  #if ((ETH_NMB_DEVICES) > 2)
	#warning "Too many EMAC devices for Arria 5 / Cyclone V:"
    #error "      Set ETH_MAX_DEVICES <= 2 and/or ETH_LIST_DEVICE <= 0x3"
  #endif

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)	/* Arria 10										*/
  volatile uint32_t * const G_EMACaddr[3] = { (volatile uint32_t *)0xFF800000,
                                              (volatile uint32_t *)0xFF802000,
                                              (volatile uint32_t *)0xFF804000
                                            };

  #define EMAC_RESET(Dev)					do {													\
											  *(volatile uint32_t *)0xFFD05024 |=  1<<(Dev);		\
											  *(volatile uint32_t *)(0xFFD06044+(4*(Dev))) &= ~0x3;	\
											  *(volatile uint32_t *)(0xFFD06044+(4*(Dev))) |=  0x1;	\
											  *(volatile uint32_t *)0xFFD05024 &= ~(1<<(Dev));		\
											} while(0)

  #ifndef OX_CACHE_LSIZE							/* Abassi.h is not included. Put in #ifdef in	*/
	#define OX_CACHE_LSIZE			32				/* case it ever gets included					*/
  #endif

  #define UDELAY_MULT				(500/6)			/* Cached: Multiplier for loop counter for 1 us	*/

  #if ((ETH_NMB_DEVICES) > 3)
	#warning "Too many EMAC devices for Arria 10:"
    #error "      Set ETH_MAX_DEVICES <= 3 and/or ETH_LIST_DEVICE <= 0x7"
  #endif

#else
  	#error "Unsupported platform specified by OS_PLATFORM"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* The following sections use a lots of macros but it's to reduce the size of the data requirements	*/
/* by only	creating descriptor / mutexes etc for the devices in use ("1" in ETH_LIST_DEVICE)		*/

#ifndef ETH_MAX_DEVICES
 #if   (((ETH_LIST_DEVICE) & 0x200) != 0U)
  #define ETH_MAX_DEVICES	10
 #elif (((ETH_LIST_DEVICE) & 0x100) != 0U)
  #define ETH_MAX_DEVICES	9
 #elif (((ETH_LIST_DEVICE) & 0x080) != 0U)
  #define ETH_MAX_DEVICES	8
 #elif (((ETH_LIST_DEVICE) & 0x040) != 0U)
  #define ETH_MAX_DEVICES	7
 #elif (((ETH_LIST_DEVICE) & 0x020) != 0U)
  #define ETH_MAX_DEVICES	6
 #elif (((ETH_LIST_DEVICE) & 0x010) != 0U)
  #define ETH_MAX_DEVICES	5
 #elif (((ETH_LIST_DEVICE) & 0x008) != 0U)
  #define ETH_MAX_DEVICES	4
 #elif (((ETH_LIST_DEVICE) & 0x004) != 0U)
  #define ETH_MAX_DEVICES	3
 #elif (((ETH_LIST_DEVICE) & 0x002) != 0U)
  #define ETH_MAX_DEVICES	2
 #elif (((ETH_LIST_DEVICE) & 0x001) != 0U)
  #define ETH_MAX_DEVICES	1
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* This section figures out how to remap the individual devices specified by ETH_LIST_DEVICE		*/
/* e.g. if ETH_LIST_DEVICE == 0x12, the only descriptors/mutexes for 2 devices are required and	*/
/*      device#1 is remapped to [0] and device #4 is remapped to [1]								*/

#if (((ETH_LIST_DEVICE) & 0x001) != 0)
  #define ETH_CNT_0		0
  #define ETH_IDX_0		0
#else
  #define ETH_CNT_0		(-1)
  #define ETH_IDX_0		(-1)
#endif
#if (((ETH_LIST_DEVICE) & 0x002) != 0)
  #define ETH_CNT_1		((ETH_CNT_0) + 1)
  #define ETH_IDX_1		((ETH_CNT_0) + 1)
#else
  #define ETH_CNT_1		(ETH_CNT_0)
  #define ETH_IDX_1		-1
#endif
#if (((ETH_LIST_DEVICE) & 0x004) != 0)
  #define ETH_CNT_2		((ETH_CNT_1) + 1)
  #define ETH_IDX_2		((ETH_CNT_1) + 1)
#else
  #define ETH_CNT_2		(ETH_CNT_1)
  #define ETH_IDX_2		-1
#endif
#if (((ETH_LIST_DEVICE) & 0x008) != 0)
  #define ETH_CNT_3		((ETH_CNT_2) + 1)
  #define ETH_IDX_3		((ETH_CNT_2) + 1)
#else
  #define ETH_CNT_3		(ETH_CNT_2)
  #define ETH_IDX_3		-1
#endif
#if (((ETH_LIST_DEVICE) & 0x010) != 0)
  #define ETH_CNT_4		((ETH_CNT_3) + 1)
  #define ETH_IDX_4		((ETH_CNT_3) + 1)
#else
  #define ETH_CNT_4		(ETH_CNT_3)
  #define ETH_IDX_4		-1
#endif
#if (((ETH_LIST_DEVICE) & 0x020) != 0)
  #define ETH_CNT_5		((ETH_CNT_4) + 1)
  #define ETH_IDX_5		((ETH_CNT_4) + 1)
#else
  #define ETH_CNT_5		(ETH_CNT_4)
  #define ETH_IDX_5		-1
#endif
#if (((ETH_LIST_DEVICE) & 0x040) != 0)
  #define ETH_CNT_6		((ETH_CNT_5) + 1)
  #define ETH_IDX_6		((ETH_CNT_5) + 1)
#else
  #define ETH_CNT_6		(ETH_CNT_5)
  #define ETH_IDX_6		-1
#endif
#if (((ETH_LIST_DEVICE) & 0x080) != 0)
  #define ETH_CNT_7		((ETH_CNT_6) + 1)
  #define ETH_IDX_7		((ETH_CNT_6) + 1)
#else
  #define ETH_CNT_7		(ETH_CNT_6)
  #define ETH_IDX_7		-1
#endif
#if (((ETH_LIST_DEVICE) & 0x100) != 0)
  #define ETH_CNT_8		((ETH_CNT_7) + 1)
  #define ETH_IDX_8		((ETH_CNT_7) + 1)
#else
  #define ETH_CNT_8		(ETH_CNT_7)
  #define ETH_IDX_8		-1
#endif
#if (((ETH_LIST_DEVICE) & 0x200) != 0)
  #define ETH_CNT_9		((ETH_CNT_8) + 1)
  #define ETH_IDX_9		((ETH_CNT_8) + 1)
#else
  #define ETH_CNT_9		(ETH_CNT_8)
  #define ETH_IDX_9		-1
#endif

#define ETH_NMB_DEVICES	((ETH_CNT_9)+1)

/* ------------------------------------------------------------------------------------------------ */

const int G_EMACreMap[] = { ETH_IDX_0
                         #if ((ETH_MAX_DEVICES) > 1)
                           ,ETH_IDX_1
                         #endif 
                         #if ((ETH_MAX_DEVICES) > 2)
                           ,ETH_IDX_2
                         #endif 
                         #if ((ETH_MAX_DEVICES) > 3)
                           ,ETH_IDX_3
                         #endif 
                         #if ((ETH_MAX_DEVICES) > 4)
                           ,ETH_IDX_4
                         #endif 
                         #if ((ETH_MAX_DEVICES) > 5)
                           ,ETH_IDX_5
                         #endif 
                         #if ((ETH_MAX_DEVICES) > 6)
                           ,ETH_IDX_6
                         #endif 
                         #if ((ETH_MAX_DEVICES) > 7)
                           ,ETH_IDX_7
                         #endif 
                         #if ((ETH_MAX_DEVICES) > 8)
                           ,ETH_IDX_8
                         #endif 
                         #if ((ETH_MAX_DEVICES) > 9)
                           ,ETH_IDX_9
                         #endif
                         };

/* ------------------------------------------------------------------------------------------------ */
/* Supported PHY IDs - defined from (((PHY_ID2&0xFFF0) << 16) | (PHY_ID1))							*/

#define PHY_ID_KSZ9021						(0x16100022)
#define PHY_ID_KSZ9031						(0x16200022)
#define PHY_ID_PEF7071						(0xA400D565)
#define PHY_ID_MARV88E1518					(0x0DD00141)
#define PHY_ID_TIDP83867					(0xA2302000)

/* ------------------------------------------------------------------------------------------------ */
/* IEEE 802.3 stamdard register defintions - Section 22.2.4											*/

#define PHY_BCR_REG							 0					/* Transceiver Basic Control Regs	*/
 #define PHY_RESET							((uint16_t)(1<<15))	/* PHY Reset						*/
 #define PHY_AUTO_NEGOTIATION				((uint16_t)(1<<12))	/* Enable auto-negotiation function	*/
 #define PHY_PWRDWN							((uint16_t)(1<<11))	/* PHY power down					*/
 #define PHY_AUTO_NEGOTIATION_RESTART		((uint16_t)(1<< 9))
#define PHY_BSR_REG							 1					/* Transceiver Basic Status Regs	*/
 #define PHY_AUTO_NEGOTIATION_COMPLETE		((uint16_t)0x0020)	/* Auto-Negotiation process done	*/
 #define PHY_LINK_STATUS					((uint16_t)0x0004)	/* Valid link established			*/
#define PHY_ID1_REG							 2			/* PHY Identifier Register					*/
#define PHY_ID2_REG							 3			/* PHY Identifier Register					*/
#define PHY_AUTON_REG						 4			/* PHY AutoNegotiate Register				*/
 #define AUTON_ADV_HALFDUPLEX_10M			(1<< 5)		/* Try for 10mbps half-duplex				*/
 #define AUTON_ADV_FULLDUPLEX_10M			(1<< 6)		/* Try for 10mbps full-duplex				*/
 #define AUTON_ADV_HALFDUPLEX_100M			(1<< 7)		/* Try for 100mbps half-duplex				*/
 #define AUTON_ADV_FULLDUPLEX_100M			(1<< 8)		/* Try for 100mbps full-duplex				*/
#define PHY_AUTON_PARTN_REG					 5			/* PHY Auto-Negotiatioon Partner Ability	*/
#define PHY_AUTON_EXPAN_REG					 6			/* PHY Auto-Negotiation Expansion			*/
#define PHY_AUTON_NXTPG_REG					 7			/* PHY Auto-Negotiation Next Page			*/
#define PHY_AUTON_RX_NXTPG_REG				 8			/* PHY Auto-Negotiation partner RX Next Page*/
#define PHY_MAST_SLV_REG					 9			/* PHY Master-Slave Control Register		*/
 #define PHY_MAST_SLV_FULL_DUPLEX			(1<<9)
 #define PHY_MAST_SLV_HALF_DUPLEX			(1<<8)
#define PHY_MAST_SLV_STAT_REG				10			/* PHY Master-Slave Status Register			*/
 #define _MAST_SLV_STAT_FULL_1000M			(1<<11)
 #define _MAST_SLV_STAT_HALF_1000M			(1<<10)
#define PHY_PSE_CTL_REG						11			/* PHY PSE Control							*/
#define PHY_PSE_STAT_REG					12			/* PHY PSE Status							*/
#define PHY_MMD_CTL_REG						13			/* PHY MMD control							*/
#define PHY_MMD_DATA_REG					14			/* PHY MMD Data								*/
#define PHY_EXT_STAT_REG					15			/* PHY Extended Status						*/

/* ------------------------------------------------------------------------------------------------ */
/* ETH DMA Descriptors definition																	*/

#define ETH_DMARxDesc_DIC		((uint32_t)0x80000000)		/* Disable Interrupt on Completion		*/
#define ETH_DMARxDesc_FL		((uint32_t)0x3FFF0000)		/* Receive descriptor frame length		*/
#define ETH_DMARxDesc_FS		((uint32_t)0x00000200)		/* First descriptor of the frame		*/
#define ETH_DMARxDesc_LD		((uint32_t)0x00000100)		/* Last descriptor of the frame 		*/
#define ETH_DMARxDesc_OWN		((uint32_t)0x80000000)		/* OWN bit: descriptor is owned by DMA	*/
#define ETH_DMATxDesc_OWN		((uint32_t)0x80000000)		/* OWN bit: descriptor is owned by DMA	*/

#ifdef USE_ENHANCED_DMA_DESCRIPTORS
  #define ETH_DMARxDesc_RBS1	((uint32_t)0x00001FFF)		/* Receive Buffer1 Size					*/
  #define ETH_DMATxDesc_FS		((uint32_t)0x10000000)		/* First Segment						*/
  #define ETH_DMATxDesc_LS		((uint32_t)0x20000000)		/* Last Segment							*/
  #define ETH_DMATxDesc_IC		((uint32_t)0x40000000)		/* IC bit: interrupt once TXed			*/
  #define ETH_DMATxDesc_TBS1	((uint32_t)0x00001FFF)		/* Transmit Buffer1 Size				*/
  #define ETH_DMARxDesc_RCH		((uint32_t)0x00004000)		/* Second Address Chained				*/
  #define ETH_DMATxDesc_TCH		((uint32_t)0x00100000)		/* Second Address Chained				*/
#else
  #define ETH_DMARxDesc_RBS1	((uint32_t)0x000007FF)		/* Receive Buffer1 Size					*/
  #define ETH_DMATxDesc_FS		((uint32_t)0x20000000)		/* First Segment						*/
  #define ETH_DMATxDesc_LS		((uint32_t)0x40000000)		/* Last Segment							*/
  #define ETH_DMATxDesc_IC		((uint32_t)0x80000000)		/* IC bit: interrupt once TXed			*/
  #define ETH_DMATxDesc_TBS1	((uint32_t)0x000007FF)		/* Transmit Buffer1 Size				*/
  #define ETH_DMARxDesc_RCH		((uint32_t)0x01000000)		/* Second Address Chained				*/
  #define ETH_DMATxDesc_TCH		((uint32_t)0x01000000)		/* Second Address Chained				*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* EMACS registers																					*/

#define SYS_CTRL_ADDR						0xF8000000
#define SYS_CTRL_LOCK_ADDR					(SYS_CTRL_ADDR + 0x4)
#define SYS_CTRL_UNLOCK_ADDR				(SYS_CTRL_ADDR + 0x8)
 #define SYS_CTRL_LOCK_KEY_VALUE 			0x767B
 #define SYS_CTRL_UNLOCK_KEY_VALUE			0xDF0D
#define GEM0_CLK_CTRL_ADDR					(SYS_CTRL_ADDR + 0x140)
#define GEM1_CLK_CTRL_ADDR					(SYS_CTRL_ADDR + 0x144)
 #define GEM_CLK_CTRL_DIV_MASK				0xFC0FC0FF
 #define GEM_CLK_CTRL_1000MBPS_DIV0			8
 #define GEM_CLK_CTRL_1000MBPS_DIV1			1
 #define GEM_CLK_CTRL_100MBPS_DIV0			8
 #define GEM_CLK_CTRL_100MBPS_DIV1			5
 #define GEM_CLK_CTRL_10MBPS_DIV0			8
 #define GEM_CLK_CTRL_10MBPS_DIV1			50
#define SYS_CTRL_GEM_RESET					(SYS_CTRL_ADDR + 0x214)

/* ------------------------------------------------------------------------------------------------ */

#define PHY_READ_TO							(  100000)	/* Max wait time (us) when reading from PHY	*/
#define PHY_WRITE_TO						(  100000)	/* Max wait time (us) when reading from PHY	*/
#define PHY_LINKUP_TO						( 5000000)	/* Max wait time (us) for link to come up	*/
#define PHY_AUTON_TO						(15000000)	/* Max wait time (us) for auto-ng to succeed*/
#define PHY_RESET_DELAY			 			(    2000)	/* Wait around  2 ms						*/
#define PHY_CONFIG_DELAY					(   25000)	/* Wait around 25 ms						*/
#define PHY_POWERCHANGE_DELAY				(   10000)	/* Minimum 1ms, go a bit on the safe side	*/

/* ------------------------------------------------------------------------------------------------ */
/* Local & global variables																			*/

#if ((ETH_IS_BUF_UNCACHED) != 0)
  #define DMA_ATTRIB __attribute__ ((aligned (8))) __attribute__ ((section (".uncached")))
  #define DSC_ATTRIB __attribute__ ((aligned (8))) __attribute__ ((section (".uncached")))
#else
 #if ((OX_CACHE_LSIZE) <= 8)
  #define DMA_ATTRIB __attribute__ ((aligned (8)))
  #define DSC_ATTRIB __attribute__ ((aligned (8)))
  #if (((ETH_RX_BUFSIZE) & 7) != 0)
	#error "The buffer size ETH_RX_BUFSIZE must be an exact multiple of 8"
  #endif
  #if (((ETH_TX_BUFSIZE) & 7) != 0)
	#error "The buffer size ETH_TX_BUFSIZE must be an exact multiple of 8"
  #endif
 #else
  #define DMA_ATTRIB __attribute__ ((aligned (OX_CACHE_LSIZE)))
  #define DSC_ATTRIB __attribute__ ((aligned (OX_CACHE_LSIZE)))
  #if (((ETH_RX_BUFSIZE) & ((OX_CACHE_LSIZE)-1)) != 0)
	#error "The buffer size ETH_RX_BUFSIZE must be an exact multiple of OX_CACHE_LSIZE"
  #endif
  #if (((ETH_TX_BUFSIZE) & ((OX_CACHE_LSIZE)-1)) != 0)
	#error "The buffer size ETH_TX_BUFSIZE must be an exact multiple of OX_CACHE_LSIZE"
  #endif
 #endif
#endif

ETH_DMAdesc_t G_DMArxDescTbl[ETH_NMB_DEVICES][ETH_N_RXBUF] DSC_ATTRIB;		/* Ethernet RX DMA Desc	*/
ETH_DMAdesc_t G_DMAtxDescTbl[ETH_NMB_DEVICES][ETH_N_TXBUF] DSC_ATTRIB;		/* Ethernet TX DMA Desc	*/

#if ((ETH_IS_BUF_PBUF) == 0)
  uint8_t G_EthRXbuf[ETH_NMB_DEVICES][ETH_N_RXBUF][ETH_RX_BUFSIZE] DMA_ATTRIB;/* Ethernet RX Buffer	*/
#endif
uint8_t G_EthTXbuf[ETH_NMB_DEVICES][ETH_N_TXBUF][ETH_TX_BUFSIZE] DMA_ATTRIB;/* Ethernet TX Buffer	*/

static char g_PhyMtxName[][6] = {
                                 "PHY-0"
                              #if ((ETH_MAX_DEVICES) > 1)
                                ,"PHY-1"
                              #endif
                              #if ((ETH_MAX_DEVICES) > 2)
                                ,"PHY-2"
                              #endif
                              #if ((ETH_MAX_DEVICES) > 3)
                                ,"PHY-3"
                              #endif
                              #if ((ETH_MAX_DEVICES) > 4)
                                ,"PHY-4"
                              #endif
                              #if ((ETH_MAX_DEVICES) > 5)
                                ,"PHY-5"
                              #endif
                              #if ((ETH_MAX_DEVICES) > 6)
                                ,"PHY-6"
                              #endif
                              #if ((ETH_MAX_DEVICES) > 7)
                                ,"PHY-7"
                              #endif
                              #if ((ETH_MAX_DEVICES) > 8)
                                ,"PHY-8"
                              #endif
                              #if ((ETH_MAX_DEVICES) > 9)
                                ,"PHY-9"
                              #endif
                              #if ((ETH_MAX_DEVICES) > 10)
                                #error "Increase g_PhyMtxName[] init"
                              #endif
                                };

#define SKEW_RXCTL							 0		/* 2nd Index in g_Skews[][]						*/
#define SKEW_RXCLK							 1
#define SKEW_RX0							 2
#define SKEW_RX1							 3
#define SKEW_RX2							 4
#define SKEW_RX3							 5
#define SKEW_TXCTL							 6
#define SKEW_TXCLK							 7
#define SKEW_TX0							 8
#define SKEW_TX1							 9
#define SKEW_TX2							10
#define SKEW_TX3							11


static const int g_Skews[3][12] = {
								  {	ETH_0_SKEW_RXCTL,
									ETH_0_SKEW_RXCLK,
									ETH_0_SKEW_RX0,
									ETH_0_SKEW_RX1,
									ETH_0_SKEW_RX2,
									ETH_0_SKEW_RX3,
									ETH_0_SKEW_TXCTL,
									ETH_0_SKEW_TXCLK,
									ETH_0_SKEW_TX0,
									ETH_0_SKEW_TX1,
									ETH_0_SKEW_TX2,
									ETH_0_SKEW_TX3 }
								 ,{	ETH_1_SKEW_RXCTL,
									ETH_1_SKEW_RXCLK,
									ETH_1_SKEW_RX0,
									ETH_1_SKEW_RX1,
									ETH_1_SKEW_RX2,
									ETH_1_SKEW_RX3,
									ETH_1_SKEW_TXCTL,
									ETH_1_SKEW_TXCLK,
									ETH_1_SKEW_TX0,
									ETH_1_SKEW_TX1,
									ETH_1_SKEW_TX2,
									ETH_1_SKEW_TX3 }
								 ,{ ETH_2_SKEW_RXCTL,
									ETH_2_SKEW_RXCLK,
									ETH_2_SKEW_RX0,
									ETH_2_SKEW_RX1,
									ETH_2_SKEW_RX2,
									ETH_2_SKEW_RX3,
									ETH_2_SKEW_TXCTL,
									ETH_2_SKEW_TXCLK,
									ETH_2_SKEW_TX0,
									ETH_2_SKEW_TX1,
									ETH_2_SKEW_TX2,
									ETH_2_SKEW_TX3 }
								};

#if ((ETH_MAX_DEVICES) > 3)
  #error "Must update g_Skews[][] because ETH_MAX_DEVICES > 3"
#endif

typedef struct {									/* Per device: configuration & state			*/
	volatile uint32_t *HW;							/* Base address of the EMAC peripheral			*/
	int                IsInInit;					/* If currently in PHY_init()					*/
	int                PHYaddr;						/* Address of the PHY							*/
  #if ((ETH_MULTI_PHY) < 0)
	int                PHYid;						/* ID of the PHY								*/
  #else
	int                PHYid[32];					/* ID of the PHY								*/
	uint32_t           PHYlist;						/* Valid PHYs : bit set 						*/
  #endif
	MTX_t             *Mutex;						/* When 1 mutex per device						*/
	int                TXstate;						/* State of the transmitted for multi segments	*/
  #if ((ETH_ALT_PHY_IF) != 0)
	int                IsAltIF;						/* If is an alternate PHY I/F					*/
  #endif
	volatile int              RxByteCnt;			/* Cumulative # byte for multi-frame packet		*/
	         ETH_DMAdesc_t   *RxDesc;				/* Pointer to this device RX descriptors		*/
	volatile ETH_DMAdesc_t   *RxDescEnd;			/* Last frame to read in a packet				*/
             ETH_DMAdesc_t   *RxDescToGet;			/* Next descriptor to use						*/
	volatile ETH_DMAdesc_t   *RxDescToStart;		/* First frame in a packet						*/
	         ETH_DMAdesc_t   *TxDesc;				/* Pointer to this device TX descriptors		*/
	volatile ETH_DMAdesc_t   *TxDescToSet;			/* Next descriptor to use						*/
	volatile ETH_DMAdesc_t   *TxDescToStart;		/* First frame in a packet						*/
  #if ((ETH_IS_BUF_CACHED) != 0)
	int                       Idx2Flush;			/* Index where the next to flush is				*/
	ETH_DMAdesc_t            *Desc2Flush[ETH_N_RXBUF];	/* Descriptors to flush once all read		*/
  #endif
} EMACcfg_t;

static EMACcfg_t    g_EMACcfg[ETH_NMB_DEVICES];		/* Device configuration							*/
static int          g_CfgIsInit = 0;				/* To track first time an init occurs			*/
static volatile int g_Dummy;						/* Used for dummy reads & and SW delay			*/

#if ((ETH_ALT_PHY_MTX) > 0)
  MTX_t *g_MtxAltIF;								/* If one mutex for all AltPhyIF() is requested	*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* function prototypes																				*/

static int  ETH_RdPhyReg    (int Dev, int PHYaddr, int PHYreg);
static int  ETH_RdPhyRegExt (int PHYid, int Dev, int PHYaddr, int PHYreg);
static int  ETH_WrtPhyReg   (int Dev, int PHYaddr, int PHYreg, int PHYval);
static int  ETH_WrtPhyRegExt(int PHYid, int Dev, int PHYaddr, int PHYreg, int PHYval);
static void uDelay          (int Time);

/* ------------------------------------------------------------------------------------------------ */
/* Rough delay of N us																				*/
/* ------------------------------------------------------------------------------------------------ */

static void uDelay(int Time)
{
int ii;												/* Loop counter									*/

	Time *= UDELAY_MULT;
	for (ii=0 ; ii<Time ; ii++) {
		g_Dummy++;
	}
	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* ETH_MACAddressConfig - Configure the MAC address													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void ETH_MACAddressConfig(int Dev, uint32_t MacAddr, uint8_t *Addr);						*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev     : controller number																	*/
/*		MacAddr : One of these MAC address registers:												*/
/*		          ETH_MAC_Address0 -> ETH_MAC_Address15												*/
/*		Addr[6] : MAC address [0]:LSB, [5]:MSB														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_MACAddressConfig(int Dev, uint32_t MacAddr, uint8_t *Addr)
{

	EMACreg(Dev, EMAC_MAC_ADDR_HI_REG + MacAddr) = ((uint32_t)Addr[5] <<  8)
	                                             |  (uint32_t)Addr[4];
	EMACreg(Dev, EMAC_MAC_ADDR_LO_REG + MacAddr) = ((uint32_t)Addr[3] << 24)
	                                             | ((uint32_t)Addr[2] << 16)
	                                             | ((uint32_t)Addr[1] <<  8)
	                                             |  (uint32_t)Addr[0];
	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* ETH_MACAddressGet - Get the MAC address that was configured										*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void ETH_MACAddressGet(int Dev, uint32_t MacAddr, uint8_t *Addr);							*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev     : controller number																	*/
/*		MacAddr : One of these MAC address registers:												*/
/*		          ETH_MAC_Address0 -> ETH_MAC_Address15												*/
/*		Addr[6] : MAC address [0]:LSB, [5]:MSB														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*		MAC address in Addr[6] 																		*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_MACAddressGet(int Dev, uint32_t MacAddr, uint8_t *Addr)
{
uint32_t Tmp;

	Tmp     = EMACreg(Dev, EMAC_MAC_ADDR_HI_REG + MacAddr);
	Addr[5] = (uint8_t)((Tmp >> 8) & 0x000000FF);
	Addr[4] = (uint8_t) (Tmp       & 0x000000FF);

	Tmp     = EMACreg(Dev, EMAC_MAC_ADDR_LO_REG + MacAddr);
	Addr[3] = (uint8_t)((Tmp >> 24) & 0xFF);
	Addr[2] = (uint8_t)((Tmp >> 16) & 0xFF);
	Addr[1] = (uint8_t)((Tmp >> 8 ) & 0xFF);
	Addr[0] = (uint8_t)( Tmp        & 0xFF);

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Receive a frame,  possibly multiple segments														*/
/*																									*/
/* Must call																						*/
/*        ETH_Release_Received(int Dev);															*/
/* once the last DMA buffer has been used															*/
/*																									*/
/* Returns FrameTypeDef :																			*/
/*   .length																						*/
/*          <  0 when no new segment / packet is available or when a packet was fully extracted		*/
/*          >= 0 Number of bytes in this segment / packet											*/
/*   .buffer																						*/
/*          Pointer to the buffer holding the received segment / packet								*/
/*   .descriptor																					*/
/*          Pointer to the RX DMA descriptor for this segment / packet								*/
/* ------------------------------------------------------------------------------------------------ */

FrameTypeDef ETH_Get_Received_Multi(int Dev)
{
EMACcfg_t    *MyCfg;								/* Configuration / state of this device			*/
FrameTypeDef  Frame;


	MyCfg = &g_EMACcfg[G_EMACreMap[Dev]];

  #if ((ETH_IS_BUF_CACHED) != 0)					/* Make sure to get updated desc from DMA		*/
	DCacheInvalRange((void *)MyCfg->RxDescToGet, sizeof(*MyCfg->RxDescToGet));
  #endif
													/* Report error if nothing has been received	*/
	if (((MyCfg->RxDescToGet->Status & ETH_DMARxDesc_OWN) != 0)
	||  (MyCfg->RxDescEnd != NULL)) {				/* If all the packet has been read				*/
		Frame.length     = -1;
		Frame.buffer     = (uint32_t)NULL;
		Frame.descriptor = NULL;
	}
	else {											/* The DMA doesn't own the descript, is new one	*/
		if (MyCfg->RxDescToStart == NULL) {			/* It is the first segment of the packet		*/
			MyCfg->RxByteCnt     = 0;
			MyCfg->RxDescToStart = MyCfg->RxDescToGet;	/* Memo desc that can stop DMA				*/
		}
													/* Last segment of the RXed packet				*/
		if ((MyCfg->RxDescToGet->Status & ETH_DMARxDesc_LD) != 0) {
			MyCfg->RxDescEnd = MyCfg->RxDescToGet;
			Frame.length     = ((MyCfg->RxDescToGet->Status & ETH_DMARxDesc_FL) >> 16)
			                 - 4					/* Don't include the CRC						*/
			                 - MyCfg->RxByteCnt;
		}
		else {
			Frame.length     = MyCfg->RxDescToGet->ControlBufferSize
			                 & ETH_DMARxDesc_RBS1;
			MyCfg->RxByteCnt += Frame.length;
		}
		Frame.descriptor   = MyCfg->RxDescToGet;
		Frame.buffer       = MyCfg->RxDescToGet->Buff;
		if (MyCfg->RxDescToStart != MyCfg->RxDescToGet) {
			MyCfg->RxDescToGet->Status = ETH_DMARxDesc_OWN;
		}

	  #if ((ETH_IS_BUF_CACHED) != 0)				/* Must invalidate when DMA will RX next time	*/
		MyCfg->Desc2Flush[MyCfg->Idx2Flush++] = (void *)MyCfg->RxDescToGet;
		DCacheInvalRange((void *)Frame.buffer, ETH_RX_BUFSIZE);	/* Re-do in case branch prediction	*/
	  #endif

		MyCfg->RxDescToGet = MyCfg->RxDescToGet->NextDesc;
	}

	return(Frame);
}

/* ------------------------------------------------------------------------------------------------ */
/* Report the length of a packet.																	*/
/* The DMA RX descriptor chain is scanned to find the last packet and from the last segment, the	*/
/* packet length is extracted																		*/
/* Must be called before calling ETH_Get_Received_Multi()											*/
/* ------------------------------------------------------------------------------------------------ */

int ETH_Get_Received_Frame_Length(int Dev)
{
EMACcfg_t *MyCfg;									/* Configuration / state of this device			*/
int        RetVal;
volatile ETH_DMAdesc_t *Desc;


	RetVal = -1;									/* Assume no packet has been received			*/
	MyCfg  = &g_EMACcfg[G_EMACreMap[Dev]];
	Desc   = MyCfg->RxDescToGet;

  #if ((ETH_IS_BUF_CACHED) != 0)
	DCacheInvalRange((void *)Desc, sizeof(*Desc));
  #endif

	while ((Desc->Status & ETH_DMARxDesc_OWN) == 0) {
		if ((Desc->Status & ETH_DMARxDesc_LD) == 0) {
			Desc = Desc->NextDesc;
		  #if ((ETH_IS_BUF_CACHED) != 0)
			DCacheInvalRange((void *)Desc, sizeof(*Desc));
		  #endif
		}
		else {
			RetVal = ((Desc->Status & ETH_DMARxDesc_FL) >> 16)
			       - 4;								/* Don't include the CRC						*/
			break;
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Release RX DMA buffer that were owned by the host												*/
/* This must be called once all segments or a packet has been extracted using						*/
/* ETH_Get_Received_Multi()																			*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_Release_Received(int Dev)
{
EMACcfg_t *MyCfg;									/* Configuration / state of this device			*/
volatile uint32_t *Reg;


	MyCfg = &g_EMACcfg[G_EMACreMap[Dev]];
	Reg   = MyCfg->HW;

	if (MyCfg->RxDescToStart != NULL) {
	  #if ((ETH_IS_BUF_CACHED) != 0)
		while (MyCfg->Idx2Flush > 0) {
			DCacheInvalRange((void *)MyCfg->Desc2Flush[--MyCfg->Idx2Flush]->Buff, ETH_RX_BUFSIZE);
			DCacheFlushRange(MyCfg->Desc2Flush[MyCfg->Idx2Flush], sizeof(ETH_DMAdesc_t));
		}
	  #endif

		MyCfg->RxDescToStart->Status = ETH_DMARxDesc_OWN;
	  #if ((ETH_IS_BUF_CACHED) != 0)
		DCacheFlushRange((void *)MyCfg->RxDescToStart, sizeof(*MyCfg->RxDescToStart));
	  #endif

		MyCfg->RxDescToStart = NULL;
		MyCfg->RxDescEnd     = NULL;
													/* When RX unavailable flag set, resume RX		*/
		if (Reg[EMAC_DMA_STAT_REG] & EMAC_DMA_STAT_RU) {
			do {									/* Loop to eliminate possible spurious interrupt*/
				Reg[EMAC_DMA_STAT_REG] = EMAC_DMA_STAT_RU;
			} while (Reg[EMAC_DMA_STAT_REG] & (EMAC_DMA_STAT_RU));
			Reg[EMAC_DMA_RX_POLL_REG] = 0;			/* And resume DMA reception						*/
		}
	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */

void *ETH_Get_Transmit_Buffer(int Dev)
{
EMACcfg_t *MyCfg;									/* Configuration / state of this device			*/
void      *RetVal;


	MyCfg = &g_EMACcfg[G_EMACreMap[Dev]];

  #if ((ETH_IS_BUF_CACHED) != 0)					/* Make sure to get updated desc from DMA		*/
	DCacheInvalRange((void *)MyCfg->TxDescToSet, sizeof(*MyCfg->TxDescToSet));
  #endif

	RetVal = (void *)(MyCfg->TxDescToSet->Buff);

	if (((MyCfg->TxDescToSet->Status & ETH_DMATxDesc_OWN) != 0)	/* The DMA still using it			*/
	||  (MyCfg->TxDescToStart == MyCfg->TxDescToSet)) {	/* This one is a real problem				*/
		if (MyCfg->TxDescToStart == MyCfg->TxDescToSet) {/* 1 packet meeds more that all DMA desc	*/
			ETH_Restart(Dev);						/* Let's try to recover by restarting			*/
		}
		RetVal = (void *)NULL;
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Prepare the transmission of an Ethernet frame													*/
/* Can only be used when the whole packet fits in the DMA buffer									*/
/* ------------------------------------------------------------------------------------------------ */

int ETH_Prepare_Transmit_Descriptors(int Dev, int Len)
{
EMACcfg_t    *MyCfg;								/* Configuration / state of this device			*/
volatile uint32_t *Reg;

	MyCfg = &g_EMACcfg[G_EMACreMap[Dev]];
	Reg   = MyCfg->HW;

  #if ((ETH_IS_BUF_CACHED) != 0)					/* Make sure to use descriptor updated by DMA	*/
	DCacheInvalRange((void *)MyCfg->TxDescToSet, sizeof(*MyCfg->TxDescToSet));
  #endif

	if ((MyCfg->TxDescToSet->Status & ETH_DMATxDesc_OWN) != 0) {
		ETH_DMAtxRestart(Dev);						/* In case is suspended, restart it				*/
		return(1);									/* OWN bit set, DMA owns it, CPU can't use it	*/
	}												/* ETH_Get_Transmit_Buffer wasn't called and if	*/
 													/* was called, return val not checked for NULL	*/
	if (Len <= ETH_TX_BUFSIZE) {					/* The frame must fit in a single buffer		*/
	  #if ((ETH_IS_BUF_CACHED) != 0)
		DCacheFlushRange((void *)MyCfg->TxDescToSet->Buff, Len);
	  #endif

		MyCfg->TxDescToSet->ControlBufferSize = (Len & ETH_DMATxDesc_TBS1);

		MyCfg->TxDescToSet->Status |= ETH_DMATxDesc_OWN
		                           |  ETH_DMATxDesc_IC
		                           |  ETH_DMATxDesc_FS	/* Set first segment						*/
		                           |  ETH_DMATxDesc_LS;	/* Set last segment							*/

	  #if ((ETH_IS_BUF_CACHED) != 0)
		DCacheFlushRange((void *)MyCfg->TxDescToSet, sizeof(*MyCfg->TxDescToSet));
	  #endif

		MyCfg->TxDescToSet = MyCfg->TxDescToSet->NextDesc;

		if (Reg[EMAC_DMA_STAT_REG] & EMAC_DMA_STAT_TU) {
			do {										/* Loop to eliminate possible spurious int	*/
				Reg[EMAC_DMA_STAT_REG] = EMAC_DMA_STAT_TU;
			} while (Reg[EMAC_DMA_STAT_REG] & (EMAC_DMA_STAT_TU));
			Reg[EMAC_DMA_TX_POLL_REG] = 0;				/* Resume TX								*/
		}
		return(0);
	}

	return(2);
}

/* ------------------------------------------------------------------------------------------------ */
/* Prepare the transmission of an Ethernet frame possibly broken into multiple segments				*/
/* ------------------------------------------------------------------------------------------------ */

int ETH_Prepare_Multi_Transmit(int Dev, int Len, int IsFirst, int IsLast)
{
EMACcfg_t     *MyCfg;								/* Configuration / state of this device			*/
ETH_DMAdesc_t *NextDsc;
volatile uint32_t *Reg;

	MyCfg = &g_EMACcfg[G_EMACreMap[Dev]];

  #if ((ETH_IS_BUF_CACHED) != 0)					/* Make sure to use descriptor updated by DMA	*/
	DCacheInvalRange((void *)MyCfg->TxDescToSet, sizeof(*MyCfg->TxDescToSet));
  #endif

	if ((MyCfg->TxDescToSet->Status & ETH_DMATxDesc_OWN) != 0) {
		ETH_DMAtxRestart(Dev);						/* In case is suspended, restart it				*/
		return(1);									/* OWN bit is set, DMA owns it, CPU can't use it*/
	}

	if (Len > ETH_TX_BUFSIZE) {
		return(2);
	}

	if (IsFirst != 0) {								/* First segment								*/
		if (MyCfg->TXstate != 0) {					/* The previous packet was never terminated		*/
			if (MyCfg->TxDescToStart != NULL) {		/* Remove DMA ownership from these descriptors	*/
				NextDsc = (ETH_DMAdesc_t *)MyCfg->TxDescToStart;
				while (NextDsc != MyCfg->TxDescToSet) {
					NextDsc->Status &= ~ETH_DMATxDesc_OWN;
				  #if ((ETH_IS_BUF_CACHED) != 0)
					DCacheFlushRange((void *)NextDsc, sizeof(*NextDsc));
				  #endif
					NextDsc = (ETH_DMAdesc_t *)NextDsc->NextDesc;
				}
				NextDsc->Status &= ~ETH_DMATxDesc_OWN;
			  #if ((ETH_IS_BUF_CACHED) != 0)
				DCacheFlushRange((void *)NextDsc, sizeof(*NextDsc));
			  #endif
				MyCfg->TxDescToSet   = MyCfg->TxDescToStart;
				MyCfg->TxDescToStart = NULL;
			}
		}
		MyCfg->TXstate = 1;
	}
	else {											/* Not the first one. Make sure the first		*/
		if (MyCfg->TXstate == 0) {					/* segment was inserted							*/
			return(3);								/* Nope, then abort								*/
		}
	}

  #if ((ETH_IS_BUF_CACHED) != 0)					/* Force the buffer to land in memory			*/
	DCacheFlushRange((void *)MyCfg->TxDescToSet->Buff, Len);
  #endif

	MyCfg->TxDescToSet->Status &= ~(ETH_DMATxDesc_FS|ETH_DMATxDesc_LS|ETH_DMATxDesc_IC);

	if (IsFirst != 0) {
		MyCfg->TxDescToStart = MyCfg->TxDescToSet;	/* Will be DMA owned when all programmed		*/
	}
	else {
		MyCfg->TxDescToSet->Status |= ETH_DMATxDesc_OWN
		                           |  ETH_DMATxDesc_IC;
	}
													/* Grab it before flushing the cache otherwise	*/
	NextDsc = MyCfg->TxDescToSet->NextDesc;			/* will reload the cache						*/

	MyCfg->TxDescToSet->ControlBufferSize = (Len & ETH_DMATxDesc_TBS1);

  #if ((ETH_IS_BUF_UNCACHED) != 0)
	if (IsLast != 0) {								/* Is the last segment of the packet			*/
		MyCfg->TXstate = 0;							/* Report for next time is ready for 1st segment*/
		MyCfg->TxDescToSet->Status   |= ETH_DMATxDesc_LS;
		MyCfg->TxDescToStart->Status |= ETH_DMATxDesc_OWN
		                             |  ETH_DMATxDesc_IC
		                             |  ETH_DMATxDesc_FS;
  #else
	if (IsLast != 0) {								/* Is the last segment of the packet			*/
		MyCfg->TXstate = 0;							/* Report for next time is ready for 1st segment*/
		MyCfg->TxDescToSet->Status   |= ETH_DMATxDesc_LS;
		if (IsFirst != 0) {
			DCacheFlushRange((void *)MyCfg->TxDescToStart, sizeof(*MyCfg->TxDescToStart));
			MyCfg->TxDescToStart->Status |= ETH_DMATxDesc_OWN
			                             |  ETH_DMATxDesc_IC
			                             |  ETH_DMATxDesc_FS;
		}
	}

	DCacheFlushRange((void *)MyCfg->TxDescToSet, sizeof(*MyCfg->TxDescToSet));

	if (IsLast != 0) {	
		if (IsFirst == 0) {
			MyCfg->TxDescToStart->Status |= ETH_DMATxDesc_OWN
			                             |  ETH_DMATxDesc_IC
			                             |  ETH_DMATxDesc_FS;
			DCacheFlushRange((void *)MyCfg->TxDescToStart, sizeof(*MyCfg->TxDescToStart));
		}
  #endif
		Reg = MyCfg->HW;

		if (Reg[EMAC_DMA_STAT_REG] & EMAC_DMA_STAT_TU) {
			do {									/* Loop to eliminate possible spurious interrupt*/
				Reg[EMAC_DMA_STAT_REG] = EMAC_DMA_STAT_TU;
			} while (Reg[EMAC_DMA_STAT_REG] & (EMAC_DMA_STAT_TU));
			Reg[EMAC_DMA_TX_POLL_REG] = 0;			/* Resume TX									*/
		}
		MyCfg->TxDescToStart = NULL;
	}

	MyCfg->TxDescToSet = NextDsc;

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* Init the RX DMA descriptor chain																	*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMARxDescChainInit(int Dev)
{
EMACcfg_t    *MyCfg;								/* Configuration / state of this device			*/
int ii;
ETH_DMAdesc_t *DMARxDesc;
int            ReMap;
uint8_t       *RxBuff;


	ReMap = G_EMACreMap[Dev];
	MyCfg = &g_EMACcfg[ReMap];

  #if ((ETH_IS_BUF_PBUF) == 0)
	RxBuff = &G_EthRXbuf[ReMap][0][0];
  #else
	RxBuff = NULL;
  #endif

	DMARxDesc          = MyCfg->RxDesc;				/* Start at first in the list					*/
	MyCfg->RxDescToGet = DMARxDesc;					/* Will traverse the array						*/
	for(ii=0 ; ii<ETH_N_RXBUF ; ii++) {				/* Put the value in all descriptors				*/
		DMARxDesc->Status              = ETH_DMARxDesc_OWN;
		DMARxDesc->ControlBufferSize   = ETH_DMARxDesc_RCH
		                               | (uint32_t)ETH_RX_BUFSIZE;  
	  #if ((ETH_IS_BUF_PBUF) == 0)
		DMARxDesc->Buff                = (uint32_t)(&RxBuff[ii*ETH_RX_BUFSIZE]);
	  #endif
		DMARxDesc->NextDesc = DMARxDesc+1;			/* Set next										*/
		DMARxDesc++;
	}
	DMARxDesc--;									/* Last descriptor, loop back: circular			*/
	DMARxDesc->NextDesc = MyCfg->RxDesc;

  #if ((ETH_IS_BUF_CACHED) != 0)
	MyCfg->Idx2Flush = 0;
	if (RxBuff != (uint8_t *)NULL) {				/* Use flush in case buffer is not all aligned	*/
		DCacheFlushRange((void *)&RxBuff[0], ETH_N_RXBUF*ETH_RX_BUFSIZE);
	}
	DCacheFlushRange((void *)MyCfg->RxDesc, ETH_N_RXBUF*sizeof(*G_DMArxDescTbl[0]));
  #endif

	EMACreg(Dev, EMAC_DMA_RXDSC_ADDR_REG) = (uint32_t)MyCfg->RxDesc; 

	MyCfg->RxDescToStart = NULL;					/* Tag the DMA does not own any descriptor		*/

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Init the TX DMA descriptor chain																	*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMATxDescChainInit(int Dev)
{
int            ii;
ETH_DMAdesc_t *DMATxDesc;
EMACcfg_t     *MyCfg;								/* Configuration / state of this device			*/
int            ReMap;
uint8_t       *TxBuff;

	ReMap = G_EMACreMap[Dev];
	MyCfg = &g_EMACcfg[ReMap];

	TxBuff = &G_EthTXbuf[ReMap][0][0];

	DMATxDesc          = MyCfg->TxDesc;				/* Start at first in the list					*/
	MyCfg->TxDescToSet = DMATxDesc;					/* Will traverse the array						*/
	for(ii=0 ; ii<ETH_N_TXBUF ; ii++) {				/* Put the value in all descriptors				*/
		DMATxDesc->Status   = ETH_DMATxDesc_TCH;	/* Set Second Address Chained bit				*/
		DMATxDesc->Buff     = (uint32_t)(&TxBuff[ii*ETH_TX_BUFSIZE]);/* Set Buffer address pointer	*/
		DMATxDesc->NextDesc = DMATxDesc+1;			/* Set next										*/
		DMATxDesc++;
	}
	DMATxDesc--;									/* Last descriptor, loop back: circular			*/
	DMATxDesc->NextDesc = MyCfg->TxDesc;

  #if ((ETH_IS_BUF_CACHED) != 0)
	DCacheFlushRange((void *)MyCfg->TxDesc, sizeof(G_DMAtxDescTbl[0]));
  #endif
													/* Set Transmit Descriptor List Address Register*/
	EMACreg(Dev, EMAC_DMA_TXDSC_ADDR_REG) = (uint32_t)MyCfg->TxDesc;

	MyCfg->TXstate       = 0;						/* All OK, ready for a new packet				*/
	MyCfg->TxDescToStart = NULL;					/* Tag the DMA does not own any descriptor		*/

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Enable / disable the DMA receive interrupt														*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMARxDescReceiveITConfig(int Dev, int NewState)
{
ETH_DMAdesc_t *DMARxDesc;
int            ii;


	DMARxDesc = g_EMACcfg[G_EMACreMap[Dev]].TxDesc;

  #if ((ETH_IS_BUF_CACHED) != 0)
	DCacheInvalRange((void *)DMARxDesc, sizeof(G_DMArxDescTbl[0]));
  #endif

	if (NewState != 0) {							/* Enable										*/
		for (ii=0 ; ii<ETH_N_TXBUF ; ii++) {	
			DMARxDesc[ii].ControlBufferSize &=(~(uint32_t)ETH_DMARxDesc_DIC);
		}
	}
	else {											/* Disable										*/
		for (ii=0 ; ii<ETH_N_TXBUF ; ii++) {	
			DMARxDesc[ii].ControlBufferSize |= ETH_DMARxDesc_DIC;
		}
	}

  #if ((ETH_IS_BUF_CACHED) != 0)
	DCacheFlushRange((void *)DMARxDesc, sizeof(G_DMArxDescTbl[0]));
  #endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Enable / disable the DMA transmit interrupt														*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMARxDescTransmitITConfig(int Dev, int NewState)
{
	Dev      = Dev;
	NewState = NewState;

	return;
}

/* ------------------------------------------------------------------------------------------------ */

void ETH_DMATxDescChecksumInsertionConfig(int Dev, int ChkSum)
{
EMACcfg_t *MyCfg;

int ii;


	MyCfg = &g_EMACcfg[G_EMACreMap[Dev]];

  #if ((ETH_IS_BUF_CACHED) != 0)					/* In case hasn't been done right upon start	*/
	DCacheInvalRange((void *)MyCfg->TxDesc, sizeof(G_DMAtxDescTbl[0]));
  #endif

	if (ChkSum != 0) {
		for (ii=0 ; ii<ETH_N_TXBUF ; ii++) {
			MyCfg->TxDesc[ii].Status |= ETH_HW_CHKSUM;
		}
	}
	else {
		for (ii=0 ; ii<ETH_N_TXBUF ; ii++) {
			MyCfg->TxDesc[ii].Status &= ~(ETH_HW_CHKSUM);
		}
	}

  #if ((ETH_IS_BUF_CACHED) != 0)
	DCacheFlushRange((void *)MyCfg->TxDesc, sizeof(G_DMAtxDescTbl[0]));
  #endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: PHY_init																				*/
/*																									*/
/* PHY_init - init the PHY																			*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int PHY_init(int Dev, int Rate)																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : controller number																	*/
/*		Rate : link rate (See below)																*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int : > 0 success																			*/
/*		      < 0 error																				*/
/*																									*/
/* Rate values:																						*/
/*		  10 :   10 Mbps full duplex			 													*/
/*		  11 :   10 Mbps half duplex			 													*/
/*		 100 :  100 Mbps full duplex			 													*/
/*		 101 :  100 Mbps half duplex			 													*/
/*		1000 : 1000 Mbps full duplex			 													*/
/*		1001 : 1000 Mbps half duplex		 														*/
/*		-ve  : auto-negotiation (only as the argument, not for the return value)					*/
/*																									*/
/* NOTE:																							*/
/*		Do not use in an interrupt because the PHY accesses are mutex protected.					*/
/* ------------------------------------------------------------------------------------------------ */

int PHY_init(int Dev, int Rate)
{
int      Error;										/* For multi-PHY - also single (code simpler)	*/
int      ii;
uint32_t MACset;
EMACcfg_t *MyCfg;									/* Configuration / state of this device			*/
int      PHYaddr;
uint32_t PHYid;
int      RegVal;
int      RegTmp;
int      Tout;
volatile uint32_t *Register;						/* uint32_t pointer to access EMAC registers	*/

	if ((Dev < 0)
	||  (Dev >= ETH_MAX_DEVICES)
	||  (G_EMACreMap[Dev] < 0)) {
		return(-1);
	}

	MyCfg    = &g_EMACcfg[G_EMACreMap[Dev]];
	Register = MyCfg->HW;
	PHYaddr  = MyCfg->PHYaddr;

	if (PHYaddr >= 0) {								/* Keep link state stable during init			*/
	  #if ((ETH_MULTI_PHY) < 0)						/* Single PHY attached							*/
		ii = ETH_RdPhyReg(Dev, PHYaddr, PHY_BSR_REG)
		   & (PHY_LINK_STATUS);
		if (ii != 0) {
			MyCfg->IsInInit = 1;					/* Force the link status as being up			*/
		}
		else {
			MyCfg->IsInInit = -1;					/* Force the link status as being up			*/
		}
	  #else											/* Multiple PHY attached						*/
		MyCfg->IsInInit = 0xFFFFFFFF;				/* Force all link status as being up			*/
	  #endif
	}
	else {											/* When >=, is a re-connect, the address known	*/
		for (PHYaddr=0 ; PHYaddr<32 ; PHYaddr++) {	/* Find the PHY address using the part ID		*/
			PHYid =   ETH_RdPhyReg(Dev, PHYaddr, PHY_ID1_REG)
			      | ((ETH_RdPhyReg(Dev, PHYaddr, PHY_ID2_REG) & 0xFFF0) << 16);
		  #if ((ETH_DEBUG) != 0)
			printf("PHY   [Dev:%d] - Checking address %2d (ID: 0x%08X)\n", Dev, PHYaddr,
			       (unsigned int)PHYid);
		  #endif

			ii = 0;
			if (PHYid == PHY_ID_MARV88E1518) {
			  #if ((ETH_DEBUG) != 0)
				printf("PHY   [Dev:%d] - Found Marvell 88E1518 PHY at address %d\n", Dev, PHYaddr);
			  #endif
				ii = 1;
			}
			else if (PHYid == PHY_ID_KSZ9021) {
			  #if ((ETH_DEBUG) != 0)
				printf("PHY   [Dev:%d] - Found Micrel KSZ9021 PHY at address %d\n", Dev, PHYaddr);
			  #endif
				ii = 1;
			}
			else if (PHYid == PHY_ID_KSZ9031) {
			  #if ((ETH_DEBUG) != 0)
				printf("PHY   [Dev:%d] - Found Micrel KSZ9031/KSZ9071 PHY at address %d\n", Dev,
				        PHYaddr);
			  #endif
				ii = 1;
			}
			else if (PHYid == PHY_ID_PEF7071) {
			  #if ((ETH_DEBUG) != 0)
				printf("PHY   [Dev:%d] - Found Lantiq PEF7071 PHY at address %d\n", Dev, PHYaddr);
			  #endif
				ii = 1;
			}
			else if (PHYid == PHY_ID_TIDP83867) {
			  #if ((ETH_DEBUG) != 0)
				printf("PHY   [Dev:%d] - Found TI DP83867 PHY at address %d\n", Dev, PHYaddr);
			  #endif
				ii = 1;
			}
		  #if ((ETH_ALT_PHY_IF) != 0)
			else if ((PHYid != 0xFFF0FFFF)
			     &&  (PHYid != 0)
			     &&  (MyCfg->IsAltIF != 0)) {
			  #if ((ETH_DEBUG) != 0)
				printf("PHY   [Dev:%d] - Found Alternate PHY I/F at address %d\n", Dev, PHYaddr);
				printf("                PHY ID is 0x%08X\n", (unsigned int)PHYid);
			  #endif
				ii = 1;
			}
		  #endif
			if (ii != 0) {
			  #if ((ETH_MULTI_PHY) < 0)
				MyCfg->PHYaddr = PHYaddr;			/* Memo for later use							*/
				MyCfg->PHYid   = PHYid;
				break;
			  #else
				MyCfg->PHYlist       |= 1 << PHYaddr;
				MyCfg->PHYid[PHYaddr] = PHYid;
			   #if ((ETH_MULTI_PHY) < 32)
				MyCfg->PHYaddr = ETH_MULTI_PHY;
			   #else
				if (MyCfg->PHYaddr < 0) {
					MyCfg->PHYaddr = PHYaddr;
				}
			   #endif
			  #endif
			}
		}

	  #if ((ETH_MULTI_PHY) < 0)
		if (PHYaddr >= 32) {						/* If did not find a recognized part, error		*/
	  #else
		if (MyCfg->PHYlist == 0) {
	  #endif
		  #if ((ETH_DEBUG) != 0)
			printf("PHY   [Dev:%d] - Error - No recognized PHY found\n", Dev);
		  #endif
			return(-1);
		}
	}

 #if ((ETH_MULTI_PHY) < 0)
	PHYid = MyCfg->PHYid;
 #else
  for (PHYaddr=0 ; PHYaddr<32 ; PHYaddr++) {
   if ((MyCfg->PHYlist & (1 <<PHYaddr)) != 0) {
	PHYid = MyCfg->PHYid[PHYaddr];
 #endif

  #if (((ETH_ALT_PHY_IF) >= 0) && ((ETH_ALT_PHY_MTX) > 0))
	if (MyCfg->IsAltIF != 0) {						/* Doing a bunch of read-modify-write, so keep	*/
		MTXlock(g_MtxAltIF, -1);					/* the whole region protected					*/
	}												/* Even if ETH_ALT_PHY_MTX indicates no mutex	*/
	else											/* protection is still needed because of the	*/
  #endif											/* registers read-modify-write					*/
	{
		MTXlock(MyCfg->Mutex, -1);
	}

	Error = 0;
													/* Marvell validates 1000BASE-T settings after	*/
	if (Rate >= 1000) {								/* a reset only. Do this for all PHY			*/
		RegVal  = (Rate & 1)
		        ? PHY_MAST_SLV_HALF_DUPLEX
		        : PHY_MAST_SLV_FULL_DUPLEX;
		ETH_WrtPhyReg(Dev, PHYaddr, PHY_MAST_SLV_REG, RegVal);
	}
													/* Reset the PHY								*/
	ETH_WrtPhyReg(Dev, PHYaddr, PHY_BCR_REG, PHY_RESET);

	for (ii=32; ii>0; ii--) {						/* Wait for the reset to terminate				*/
		if ((ETH_RdPhyReg(Dev, PHYaddr, PHY_BCR_REG) & PHY_RESET) == 0) {
			break;
		}
		uDelay(PHY_RESET_DELAY);
	}

	if (ii <= 0) {									/* Got a problem as the PHY never terminates	*/
	  #if ((ETH_DEBUG) != 0)						/* its reset sequence							*/
		printf("PHY   [Dev:%d] - Error - Reset timeout\n", Dev);
	  #endif
		while(1);
	}

  #if ((ETH_ALT_PHY_IF) != 0)						/* Configure the PHY							*/
	if (MyCfg->IsAltIF != 0) {						/* Alternate PHY I/F							*/
		AltPhyIF(Dev, ETH_ALT_PHY_IF_CFG, PHYaddr, Rate, 0);
	}
	else
  #endif
	if (PHYid == PHY_ID_KSZ9021) {					/* Configure the PHY							*/
		RegVal  = 0x000F &
		        (   (g_Skews[Dev][SKEW_TXCTL] != -1)
		        ? (((g_Skews[Dev][SKEW_TXCTL] / 120) + 7) << 0)
		        :  (0x0 << 0));						/* Chip def is 0x7 - 0x0 was legacy driver 		*/
		RegVal |= 0x00F0 & 
		        (   (g_Skews[Dev][SKEW_TXCLK] != -1)
		        ? (((g_Skews[Dev][SKEW_TXCLK] / 120) + 7) << 4) 
		        :  (0xD << 4));						/* Chip def is 0x7 - 0xD was legacy driver 		*/
		RegVal |= 0x0F00 & 
		        (   (g_Skews[Dev][SKEW_RXCTL] != -1)
		        ? (((g_Skews[Dev][SKEW_RXCTL] / 120) + 7) << 6) 
		        :  (0x0 << 8));						/* Chip def is 0x7 - 0x0 was legacy driver 		*/
		RegVal |= 0xF000 & 
		        (   (g_Skews[Dev][SKEW_RXCLK] != -1)
		        ? (((g_Skews[Dev][SKEW_RXCLK] / 120) + 7) << 12) 
		        :  (0xA << 12));					/* Chip def is 0x7 - 0xA was legacy driver	 	*/
		ETH_WrtPhyRegExt(PHYid, Dev, PHYaddr, 0x104, RegVal);	/* Ref #0x104 - Clk & Ctl Skew		*/

		RegVal  = 0x000F &
		        (   (g_Skews[Dev][SKEW_RX0] != -1)
		        ? (((g_Skews[Dev][SKEW_RX0] / 120) + 7) << 0)
		        :   (0x0 << 0));					/* Chip def is 0x7 - 0x0 was legacy driver 		*/
		RegVal |= 0x00F0 & 
		        (   (g_Skews[Dev][SKEW_RX1] != -1)
		        ? (((g_Skews[Dev][SKEW_RX1] / 120) + 7) << 4) 
		        :   (0x0 << 4));					/* Chip def is 0x7 - 0x0 was legacy driver 		*/
		RegVal |= 0x0F00 & 
		        (   (g_Skews[Dev][SKEW_RX2] != -1)
		        ? (((g_Skews[Dev][SKEW_RX2] / 120) + 7) << 6) 
		        :   (0x0 << 8));					/* Chip def is 0x7 - 0x0 was legacy driver 		*/
		RegVal |= 0xF000 & 
		        (   (g_Skews[Dev][SKEW_RX3] != -1)
		        ? (((g_Skews[Dev][SKEW_RX3] / 120) + 7) << 12) 
		        :   (0x0 << 12));					/* Chip def is 0x7 - 0x0 was legacy driver 		*/
		ETH_WrtPhyRegExt(PHYid, Dev, PHYaddr, 0x105, RegVal);	/* Ref #0x105 - RX data Skew		*/

		RegVal  = 0x000F &
		        (   (g_Skews[Dev][SKEW_TX0] != -1)
		        ? (((g_Skews[Dev][SKEW_TX0] / 120) + 7) << 0)
		        :   (0x0 << 0));					/* Chip def is 0x7 - 0x0 was legacy driver 		*/
		RegVal |= 0x00F0 & 
		        (   (g_Skews[Dev][SKEW_TX1] != -1)
		        ? (((g_Skews[Dev][SKEW_TX1] / 120) + 7) << 4) 
		        :   (0x0 << 4));					/* Chip def is 0x7 - 0x0 was legacy driver 		*/
		RegVal |= 0x0F00 & 
		        (   (g_Skews[Dev][SKEW_TX2] != -1)
		        ? (((g_Skews[Dev][SKEW_TX2] / 120) + 7) << 6) 
		        :   (0x0 << 8));					/* Chip def is 0x7 - 0x0 was legacy driver 		*/
		RegVal |= 0xF000 & 
		        (   (g_Skews[Dev][SKEW_TX3] != -1)
		        ? (((g_Skews[Dev][SKEW_TX3] / 120) + 7) << 12) 
		        :   (0x0 << 12));					/* Chip def is 0x7 - 0x0 was legacy driver 		*/
		ETH_WrtPhyRegExt(PHYid, Dev, PHYaddr, 0x106, RegVal);	/* Ref #0x106 - TX data Skew		*/

		g_Dummy = ETH_RdPhyReg(Dev, PHYaddr, 0x02);	/* Dummy read to make sure is configured		*/
	}
	else if (PHYid == PHY_ID_KSZ9031) {				/* Configure the PHY							*/
		RegVal  = 0x000F &
		        (   (g_Skews[Dev][SKEW_TXCTL] != -1)
		        ? (((g_Skews[Dev][SKEW_TXCTL] / 60) + 7) << 0)
		        :  (0x0 << 0));						/* Chip def is 0x7 - 0x0 was legacy driver 		*/
		RegVal |= 0x00F0 & 
		        (   (g_Skews[Dev][SKEW_RXCTL] != -1)
		        ? (((g_Skews[Dev][SKEW_RXCTL] / 60) + 7) << 4) 
		        :  (0x0 << 4));						/* Chip def is 0x7 - 0x0 was legacy driver 		*/
		ETH_WrtPhyRegExt(PHYid, Dev, PHYaddr, 4, RegVal);	/* Reg 2-4 - Control Signal Path Skew	*/

		RegVal  = 0x000F &
		        (   (g_Skews[Dev][SKEW_RX0] != -1)
		        ? (((g_Skews[Dev][SKEW_RX0] / 60) + 7) << 0)
		        :   (0x7 << 0));					/* Default - Wasn't set-up in original code		*/
		RegVal |= 0x00F0 & 
		        (   (g_Skews[Dev][SKEW_RX1] != -1)
		        ? (((g_Skews[Dev][SKEW_RX1] / 60) + 7) << 4) 
		        :   (0x7 << 4));					/* Default - Wasn't set-up in original code		*/
		RegVal |= 0x0F00 & 
		        (   (g_Skews[Dev][SKEW_RX2] != -1)
		        ? (((g_Skews[Dev][SKEW_RX2] / 60) + 7) << 8) 
		        :   (0x7 << 8))	;					/* Default - Wasn't set-up in original code		*/
		RegVal |= 0xF000 & 
		        (   (g_Skews[Dev][SKEW_RX3] != -1)
		        ? (((g_Skews[Dev][SKEW_RX3] / 60) + 7) << 12) 
		        :   (0x7 << 12));					/* Default - Wasn't set-up in original code		*/
		ETH_WrtPhyRegExt(PHYid, Dev, PHYaddr, 5, RegVal);	/* Reg 2-5 - RX Data Path Skew			*/

		RegVal  = 0x000F &
		        (   (g_Skews[Dev][SKEW_TX0] != -1)
		        ? (((g_Skews[Dev][SKEW_TX0] / 60) + 7) << 0)
		        :   (0x7 << 0));					/* Default - Wasn't set-up in original code		*/
		RegVal |= 0x00F0 & 
		        (   (g_Skews[Dev][SKEW_TX1] != -1)
		        ? (((g_Skews[Dev][SKEW_TX1] / 60) + 7) << 4) 
		        :   (0x7 << 4));					/* Default - Wasn't set-up in original code		*/
		RegVal |= 0x0F00 & 
		        (   (g_Skews[Dev][SKEW_TX2] != -1)
		        ? (((g_Skews[Dev][SKEW_TX2] / 60) + 7) << 8) 
		        :   (0x7 << 8));					/* Default - Wasn't set-up in original code		*/
		RegVal |= 0xF000 & 
		        (   (g_Skews[Dev][SKEW_TX3] != -1)
		        ? (((g_Skews[Dev][SKEW_TX3] / 60) + 7) << 12) 
		        :   (0x7 << 12));					/* Default - Wasn't set-up in original code		*/
		ETH_WrtPhyRegExt(PHYid, Dev, PHYaddr, 6, RegVal);	/* Reg 2-6 - TX Data Path Skew			*/

		RegVal  = 0x001F &
		        (   (g_Skews[Dev][SKEW_RXCLK] != -1)
		        ? (((g_Skews[Dev][SKEW_RXCLK] / 60) + 15) << 0)
		        :  (0x1F << 0));					/* 0x1F - Chip def and legacy driver			*/
		RegVal |= 0x03E0 & 
		        (   (g_Skews[Dev][SKEW_TXCLK] != -1)
		        ? (((g_Skews[Dev][SKEW_TXCLK] / 60) + 15) << 5) 
		        :  (0x1F << 5));					/* 0x1F - Chip def and legacy driver			*/
		ETH_WrtPhyRegExt(PHYid, Dev, PHYaddr, 8, RegVal);	/* Reg 2-8 - Clock Path Skew			*/

		g_Dummy = ETH_RdPhyReg(Dev, PHYaddr, 0x02);	/* Dummy read to make sure is configured		*/
	}
	else if (PHYid == PHY_ID_PEF7071) {
		ETH_WrtPhyReg(Dev, PHYaddr, 0x19, 0);		/* Reg #0x19 - Interrupt mask reg - disable all	*/

		RegVal  = ETH_RdPhyReg(Dev, PHYaddr, PHY_MAST_SLV_REG)
		        | 1<<10;							/* Advertise single port device					*/
		ETH_WrtPhyReg(Dev, PHYaddr, PHY_MAST_SLV_REG, RegVal);

		RegVal  = ETH_RdPhyReg(Dev, PHYaddr, 0x17);	/* Reg 0x17 - MII control register				*/
		if (g_Skews[Dev][SKEW_TXCLK] != -2) {		/* If == -2, do not overload / use pin straps	*/
			RegVal &= ~0x0700;						/* Keep intact non-skew bits					*/
			RegVal |=  0x0700 &
			        (  (g_Skews[Dev][SKEW_TXCLK] != -1)	
	             ? ((g_Skews[Dev][SKEW_TXCLK] / 500) << 8)
			        :  (0x3 << 8));					/* Chip def is pin - 0x3 was legacy driver		*/
		}
		if (g_Skews[Dev][SKEW_RXCLK] != -2) {		/* If == -2, do not overload / use pin straps	*/
			RegVal &= ~0x7000;						/* Keep intact non-skew bits					*/
			RegVal |=  0x7000 &
		        (  (g_Skews[Dev][SKEW_RXCLK] != -1)
		        ? ((g_Skews[Dev][SKEW_RXCLK] / 500) << 12)
		        :  (0x3 << 12));					/* Chip def is pin - 0x3 was legacy driver 		*/
		}
		ETH_WrtPhyReg(Dev, PHYaddr, 0x17, RegVal);

		g_Dummy = ETH_RdPhyReg(Dev, PHYaddr, 0x02);	/* Dummy read to make sure is configured		*/

	}
	else if (PHYid == PHY_ID_MARV88E1518) {
		ETH_WrtPhyReg(Dev, PHYaddr, 22, 2);			/* Reg #22  - Page Address						*/

		RegVal  = ETH_RdPhyReg(Dev, PHYaddr, 21)	/* Reg 2-21 - MAC specific control register		*/
		        & ~0x0030;
		RegVal |= (1 << 5);							/* RX clk transition when data stable			*/
		RegVal |= (g_Skews[Dev][SKEW_TXCLK] == 0)
		        ? (0 << 4)							/* TX clk set to no skew, so do not delay		*/
		        : (1 << 4);							/* Default or TX skew non-zero					*/
		ETH_WrtPhyReg(Dev, PHYaddr, 21, RegVal);	/* Reg 2-21 - MAC specific control register		*/

		ETH_WrtPhyReg(Dev, PHYaddr, 22, 5);			/* Reg #22  - Page Address						*/
		RegVal  = 0x000F &
		        ( (g_Skews[Dev][SKEW_RX0] != -1)
		        ?  ((g_Skews[Dev][SKEW_RX0] / 8000) << 0)
		        :  (0 << 0));
		RegVal  = 0x00F0 &
		        ( (g_Skews[Dev][SKEW_RX1] != -1)
		        ?  ((g_Skews[Dev][SKEW_RX1] / 8000) << 4)
		        :  (0 << 4));
		RegVal  = 0x0F00 &
		        ( (g_Skews[Dev][SKEW_RX2] != -1)
		        ?  ((g_Skews[Dev][SKEW_RX2] / 8000) << 8)
		        :  (0 << 8));
		RegVal  = 0xF000 &
		        ( (g_Skews[Dev][SKEW_RX3] != -1)
		        ?  ((g_Skews[Dev][SKEW_RX3] / 8000) << 12)
		        :  (0 << 12));
		ETH_WrtPhyReg(Dev, PHYaddr, 20, RegVal);	/* Pair Skew Register, back to 0				*/

		RegVal  = ETH_RdPhyReg(Dev, PHYaddr, 21)	/* Pair Swap and Polarity Register				*/
		        | (1 << 6);							/* Enable Reg 20_5 skew values					*/
		ETH_WrtPhyReg(Dev, PHYaddr, 21, RegVal);

		ETH_WrtPhyReg(Dev, PHYaddr, 22, 0);			/* Reg #22  - Page Address back to 0			*/

		g_Dummy = ETH_RdPhyReg(Dev, PHYaddr, 0x02);	/* Dummy read to make sure is configured		*/
	}
	else if (PHYid == PHY_ID_TIDP83867) {
		RegVal = ETH_RdPhyReg(Dev, PHYaddr, 0x1F)	/* Reg #0x1F - Control Regsiter					*/
		       | (1<<15);							/* Global software reset						*/
		ETH_WrtPhyReg(Dev, PHYaddr, 0x1F, RegVal);
		uDelay (1000);								/* Wait for reset over (spec is 195 uS)			*/

		RegVal = (0x1 << 14)						/* TX FIFO depth set to 4 bytes / nibles		*/
		       | (0x2 <<  5);						/* Enable automatic cross-over					*/
		ETH_WrtPhyReg(Dev, PHYaddr, 0x10, RegVal);	/* Reg 0x10 - PHY control						*/

		RegVal  = 0x000F &
		        (   (g_Skews[Dev][SKEW_RXCLK] != -1)
		        ? (((g_Skews[Dev][SKEW_RXCLK] / 250) - 1) << 0)
		        :  (0x8 << 0))	;					/* Chip def is 0x7 - 0x8 was legacy driver		*/
		RegVal |= 0x00F0 &
		        (   (g_Skews[Dev][SKEW_TXCLK] != -1)
		        ? (((g_Skews[Dev][SKEW_TXCLK] / 250) - 1) << 4)
		        :  (0xA << 4))	;					/* Chip def is 0x7 - 0xA was legacy driver		*/
		ETH_WrtPhyRegExt(PHYid, Dev, PHYaddr, 0x86, RegVal);

		RegVal  = (0x1 << 7)						/* Enable RGMII I/F								*/
		        | (0x2 << 5)						/* RX FIFO 1/2 full threshold					*/
		        | (0x2 << 3);						/* TX FIFO 1/2 full threshold					*/
		RegVal |= (((g_Skews[Dev][SKEW_RXCLK] / 250) != 0) || (g_Skews[Dev][SKEW_RXCLK] == -1))
		        ? (0x1 << 0)						/* RX clock delayed								*/
		        : (0x0 << 0);						/* RX clock not delayed	- aligned with data		*/
		RegVal |= (((g_Skews[Dev][SKEW_TXCLK] / 250) != 0) || (g_Skews[Dev][SKEW_TXCLK] == -1))
		        ? (0x1 << 1)						/* TX clock delayed								*/
		        : (0x0 << 1);						/* TX clock not delayed	- aligned with data		*/
		ETH_WrtPhyRegExt(PHYid, Dev, PHYaddr, 0x32, RegVal);

		RegVal = ETH_RdPhyRegExt(PHYid, Dev, PHYaddr, 0x0031)
		       & ~0x0080;							/* Reserved bit: SW patch for unstable link		*/
		ETH_WrtPhyRegExt(PHYid, Dev, PHYaddr, 0x31, RegVal);

		g_Dummy = ETH_RdPhyReg(Dev, PHYaddr, 0x02);	/* Dummy read to make sure is configured		*/
	}

	RegVal = ETH_RdPhyReg(Dev, PHYaddr, PHY_AUTON_REG)
	       & ~(0xF<<5);								/* Invalidate 10 / 100 Mbps						*/
	RegVal |= 1;
	ii     = ETH_RdPhyReg(Dev, PHYaddr, PHY_MAST_SLV_REG)
	       & ~(3<<8);								/* Invalidate 1 Gbps							*/

	if (Rate >= 1000) {								/* Request to set the speed at 1 Gbps			*/
		ii     |= ((Rate & 1) != 0)					/* Advertise 1 G bps only.						*/
		        ? (1<<8)
		        : (1<<9);
	}
	else if (Rate >= 100) {							/* Request to set the speed at 100 Mbps			*/
		RegVal |= ((Rate & 1) != 0)					/* Advertise 100 Mbps only.						*/
		        ? (1<<7)
		        : (1<<8);
	}
	else if (Rate >= 10) {							/* Request to set the speed at 10 Mbps			*/
		RegVal |= ((Rate & 1) != 0)
		        ? (1<<5)
		        : (1<<6);
	}
	else {											/* No fixed speed								*/
		RegVal |= 0xF<<5;							/* Advertise 10 / 100 & 1000 Mbps. All PHY		*/
		ii     |= 3<<8;								/* supported here are OK with this				*/
	}

	ETH_WrtPhyReg(Dev, PHYaddr, PHY_AUTON_REG, RegVal);
	ETH_WrtPhyReg(Dev, PHYaddr, PHY_MAST_SLV_REG, ii);

  #if ((ETH_DEBUG) != 0)
	printf("PHY   [Dev:%d] - Starting auto-negotiation at [addr:%d]\n", Dev, PHYaddr);
  #endif

	RegVal = ETH_RdPhyReg(Dev, PHYaddr, PHY_BCR_REG);
	RegVal |= PHY_AUTO_NEGOTIATION
	       |  PHY_AUTO_NEGOTIATION_RESTART;
	ETH_WrtPhyReg(Dev, PHYaddr, PHY_BCR_REG, RegVal);

	for (Tout=PHY_LINKUP_TO/100 ; Tout>0 ; Tout--) {	/* Wait for the link to be up				*/
		if (ETH_RdPhyReg(Dev, PHYaddr, PHY_BSR_REG) & PHY_LINK_STATUS) {
			break;
		}
		uDelay(100);
	}

	if (Tout <= 0) {
	  #if ((ETH_DEBUG) != 0)
		printf("PHY   [Dev:%d] - Error - Auto-negotiation start timeout at [addr:%d]\n", Dev,PHYaddr);
	  #endif
		Error = 1;
	}

	if (Error == 0) {
	  #if ((ETH_DEBUG) != 0)
		printf("PHY   [Dev:%d] - Link is up [Addr:%d]\n", Dev, PHYaddr);
	  #endif
													/* Enable Auto-Negotiation						*/
		if (0 != ETH_WrtPhyReg(Dev, PHYaddr, PHY_BCR_REG, PHY_AUTO_NEGOTIATION
		                                                | PHY_AUTO_NEGOTIATION_RESTART)) {
			Error = 1;
		}
	}

	if (Error == 0) {
	  #if ((ETH_DEBUG) != 0)
		printf("PHY   [Dev:%d] - Enabling auto-negotiation [Addr:%d]\n", Dev, PHYaddr);
	  #endif

		for (Tout=PHY_AUTON_TO/10 ; Tout>0 ; Tout--) {	/* Wait for auto-negotiation to complete	*/
			if (ETH_RdPhyReg(Dev, PHYaddr, PHY_BSR_REG) & PHY_AUTO_NEGOTIATION_COMPLETE) {
				break;
			}
			uDelay(10);
		}

	  #if ((ETH_MULTI_PHY) < 0)
		MyCfg->IsInInit = 0;						/* Link status now report real state			*/
	  #else
		MyCfg->IsInInit &= ~(1 << PHYaddr);
	  #endif

		if (Tout <= 0) {
		  #if ((ETH_DEBUG) != 0)
			printf("PHY   [Dev:%d] - Error - Auto-negotiation timeout [Addr:%d]\n", Dev, PHYaddr);
		  #endif
			Error = 1;
		}
	}

	if (Error == 0) {
	  #if ((ETH_DEBUG) != 0)						/* Report the partner link capabilities			*/
		printf("PHY   [Dev:%d] - Link Partner Capabilities [Addr:%d] :\n", Dev, PHYaddr);
		RegVal = ETH_RdPhyReg(Dev, PHYaddr, PHY_MAST_SLV_STAT_REG);
		printf("PHY              1000 Mbps Full Duplex (%s)\n", (RegVal & _MAST_SLV_STAT_FULL_1000M)
		                                                  ? "YES" : "NO");
		printf("PHY              1000 Mbps Half Duplex (%s)\n", (RegVal & _MAST_SLV_STAT_HALF_1000M)
		                                                  ? "YES" : "NO");
		RegVal = ETH_RdPhyReg(Dev, PHYaddr, PHY_AUTON_PARTN_REG);	
		printf("PHY               100 Mbps Full Duplex (%s)\n", (RegVal & AUTON_ADV_FULLDUPLEX_100M)
		                                                  ? "YES" : "NO");
		printf("PHY               100 Mbps Half Duplex (%s)\n", (RegVal & AUTON_ADV_HALFDUPLEX_100M)
		                                                  ? "YES" : "NO");
		printf("PHY                10 Mbps Full Duplex (%s)\n", (RegVal & AUTON_ADV_FULLDUPLEX_10M)
		                                                  ? "YES" : "NO");
		printf("PHY                10 Mbps Half Duplex (%s)\n", (RegVal & AUTON_ADV_HALFDUPLEX_10M)
		                                                  ? "YES" : "NO");
	  #endif

	  #if ((ETH_ALT_PHY_IF) != 0)
		if (MyCfg->IsAltIF != 0) {					/* Alternate PHY I/F							*/
			Rate = AltPhyIF(Dev, ETH_ALT_PHY_IF_GET_RATE, PHYaddr, 0, 0);
		}
		else
	  #endif

		if ((PHYid == PHY_ID_KSZ9021)				/* Extract from the PHY the rate it is at  		*/
		||  (PHYid == PHY_ID_KSZ9031)) {			/* Same register for 9021 & 9031				*/
			RegVal = ETH_RdPhyReg(Dev, PHYaddr, 31);	/* Reg #31 - auto neg results				*/
			RegTmp = (RegVal >> 4)
			       & 7;
			if (RegTmp & (1<<2)) {
				Rate = 1000;
			}
			else if (RegTmp & (1<<1)) {
				Rate = 100;
			} 
			if ((RegVal & (1<<3)) == 0) {			/* If Full Duplex								*/
				Rate |= 1; 
			}
		}
		else if (PHYid == PHY_ID_PEF7071) {
			RegVal = ETH_RdPhyReg(Dev, PHYaddr, 0x18);	/* Reg #0x18 - MII status regsiter			*/
			RegTmp = RegVal & 3;
			if (RegTmp == 2) {
				Rate = 1000;
			}
			else if (RegTmp == 1) {
				Rate = 100;
			} 
			if ((RegVal & (1<<3)) == 0) {			/* If Full Duplex								*/
				Rate |= 1;
			}
		}
		else if (PHYid == PHY_ID_MARV88E1518) {
			RegVal = ETH_RdPhyReg(Dev, PHYaddr, 17);	/* Reg #17 -  Copper Specific Status		*/
			RegTmp = (RegVal >> 14)
			       & 3;
			if (RegTmp == 2) {
				Rate = 1000;
			}
			else if (RegTmp == 1) {
				Rate = 100;
			}
			if ((RegVal & (1<<13)) == 0) {			/* If Full Duplex								*/
				Rate |= 1;
			} 
		}
		else if (PHYid == PHY_ID_TIDP83867) {
			RegVal = ETH_RdPhyReg(Dev, PHYaddr, 0x11);	/* Reg #0x11 - PHY staus 					*/
			RegTmp = (RegVal >> 14)
			       & 3;
			if (RegTmp == 2) {
				Rate = 1000;
			}
			else if (RegTmp == 1) {
				Rate = 100;
			}
			if ((RegVal & (1<<13)) == 0) {			/* If Full Duplex								*/
				Rate |= 1;
			} 
		}
		else {
			Rate = 10;
		}

	  #if ((ETH_DEBUG) != 0)
		printf("PHY   [Dev:%d] - Link is up at %d Mbps %s duplex\n", Dev, Rate & ~1,
		                                                (Rate & 1) ? "half" : "full");
	  #endif

		if (PHYaddr == MyCfg->PHYaddr) {
			MACset  = Register[EMAC_CONF_REG]		/* Only update the PHY I/F speed bits			*/
			        & ~(EMAC_CONF_FES | EMAC_CONF_DM);
			MACset |= EMAC_CONF_PS;

			if (Rate >= 1000) {						/* Set MAC requested speed & duplexing info		*/
				MACset &= ~EMAC_CONF_PS;
			}
			else if (Rate >= 100) {
				MACset |= EMAC_CONF_FES;
			}
			if ((Rate & 1) == 0) {
				MACset |= EMAC_CONF_DM;
			}

			Register[EMAC_CONF_REG] = MACset;		/* Set the MAC Configuration Register			*/
		}
	}

  #if (((ETH_ALT_PHY_IF) >= 0) && ((ETH_ALT_PHY_MTX) > 0))
	if (MyCfg->IsAltIF != 0) {						/* Done with the whoel region prtoection		*/
		MTXunlock(g_MtxAltIF);
	}
	else
  #endif
	{
		MTXunlock(MyCfg->Mutex);
	}

	if ((PHYaddr == MyCfg->PHYaddr)					/* Can't get the info on how to set the RGMII	*/
	&&  (Error != 0)) {								/* rate so we can't move on						*/
		MyCfg->PHYaddr = -1;						/* Make sure next time PHY_init() is called		*/
	  #if ((ETH_MULTI_PHY) < 0)						/* it will be a fresh start						*/
		MyCfg->PHYid   =  0;
	  #else
		MyCfg->PHYlist = 0;
		memset(&MyCfg->PHYid, 0, sizeof(MyCfg->PHYid));
	   #if ((ETH_DEBUG) != 0)
		printf("PHY   [Dev:%d] - Error - Can\'t set-up RGMII interface - aborting\n", Dev);
	   #endif
	  #endif

		return(-1);
	}
 #if ((ETH_MULTI_PHY) >= 0)
   }
  }
 #endif

	return(Rate);
}

/* ------------------------------------------------------------------------------------------------ */
/* Start the EMAC transfers																			*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_Start(int Dev)
{

	ETH_TXenable(Dev);								/* Enable TX FSM for sending data to the MII	*/
	ETH_DMAflushTX(Dev);							/* Flush Transmit FIFO							*/
	ETH_RXenable(Dev);								/* Enable RX FSM for receiving data from MII	*/

	ETH_DMAtxEnable(Dev);							/* Start DMA transmission						*/
	ETH_DMArxEnable(Dev);							/* Start DMA reception							*/
  
	uDelay(PHY_RESET_DELAY);

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Stop the EMAC transfers																			*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_Stop(int Dev)
{
	ETH_DMAtxDisable(Dev);							/* Stop DMA transmission						*/
	ETH_DMArxDisable(Dev);							/* Stop DMA reception							*/

	ETH_TXdisable(Dev);								/* Disable TX FSM								*/
	ETH_DMAflushTX(Dev);							/* Flush Transmit FIFO							*/
	ETH_RXdisable(Dev);								/* Disable RX FSM								*/

	uDelay(PHY_RESET_DELAY);

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Restart from init the EMAC transfers																*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_Restart(int Dev)
{

	ETH_Stop(Dev);
	ETH_DMARxDescChainInit(Dev);
	ETH_DMATxDescChainInit(Dev);
	ETH_Start(Dev);

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Power down the PHY																				*/
/*																									*/
/* NOTE:																							*/
/*		Do not use in an interrupt because the PHY accesses are mutex protected.					*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_PowerDown(int Dev)
{
	mETH_PowerDown(Dev, g_EMACcfg[G_EMACreMap[Dev]].PHYaddr);

	return;

}
/* ------------------------------------------------------------------------------------------------ */

void mETH_PowerDown(int Dev, int PHYaddr)
{
EMACcfg_t *MyCfg;
int       RegVal;

	MyCfg = &g_EMACcfg[G_EMACreMap[Dev]];

  #if ((ETH_MULTI_PHY) >= 0)						/* Alternate PHYs supported						*/
	if ((MyCfg->IsAltIF != 0)						/* If it is an alternate PHY					*/
	&&  ((MyCfg->PHYlist & (1U << PHYaddr)) == 0U)) {/* And is a not valid PHY, do nothing			*/
		return;
	}
  #endif

  #if (((ETH_ALT_PHY_IF) >= 0) && ((ETH_ALT_PHY_MTX) > 0))
	if (MyCfg->IsAltIF != 0) {						/* Doing a read-modify-write, need wrapping		*/
		MTXlock(g_MtxAltIF, -1);					/* protection									*/
	}												/* Even if ETH_ALT_PHY_MTX indicates no mutex	*/
	else											/* protection is still needed because of the	*/
  #endif											/* registers read-modify-write					*/
	{
		MTXlock(MyCfg->Mutex, -1);
	}

	RegVal = ETH_RdPhyReg(Dev, PHYaddr, PHY_BCR_REG)
	       | PHY_PWRDWN;

	ETH_WrtPhyReg(Dev, PHYaddr, PHY_BCR_REG, RegVal);

  #if (((ETH_ALT_PHY_IF) >= 0) && ((ETH_ALT_PHY_MTX) > 0))
	if (MyCfg->IsAltIF != 0) {						/* Done with the whoel region prtoection		*/
		MTXunlock(g_MtxAltIF);
	}
	else
  #endif
	{
		MTXunlock(MyCfg->Mutex);
	}

	uDelay(PHY_POWERCHANGE_DELAY);

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Power up the PHY																					*/
/*																									*/
/* NOTE:																							*/
/*		Do not use in an interrupt because the PHY accesses are mutex protected.					*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_PowerUp(int Dev)
{
	mETH_PowerUp(Dev, g_EMACcfg[G_EMACreMap[Dev]].PHYaddr);
	return;
}

/* ------------------------------------------------------------------------------------------------ */

void mETH_PowerUp(int Dev, int PHYaddr)
{
EMACcfg_t *MyCfg;
int       RegVal;

	MyCfg = &g_EMACcfg[G_EMACreMap[Dev]];

  #if ((ETH_MULTI_PHY) >= 0)						/* Alternate PHYs supported						*/
	if ((MyCfg->IsAltIF != 0)						/* If it is an alternate PHY					*/
	&&  ((MyCfg->PHYlist & (1U << PHYaddr)) == 0U)) {/* And is a not valid PHY, do nothing			*/
		return;
	}
  #endif

  #if (((ETH_ALT_PHY_IF) >= 0) && ((ETH_ALT_PHY_MTX) > 0))
	if (MyCfg->IsAltIF != 0) {						/* Doing a read-modify-write, need wrapping		*/
		MTXlock(g_MtxAltIF, -1);					/* protection									*/
	}												/* Even if ETH_ALT_PHY_MTX indicates no mutex	*/
	else											/* protection is still needed because of the	*/
  #endif											/* registers read-modify-write					*/
	{
		MTXlock(MyCfg->Mutex, -1);
	}


	RegVal = ETH_RdPhyReg(Dev, PHYaddr, PHY_BCR_REG)
           & ~PHY_PWRDWN;

	ETH_WrtPhyReg(Dev, PHYaddr, PHY_BCR_REG, RegVal);

  #if (((ETH_ALT_PHY_IF) >= 0) && ((ETH_ALT_PHY_MTX) > 0))
	if (MyCfg->IsAltIF != 0) {						/* Done with the whoel region prtoection		*/
		MTXunlock(g_MtxAltIF);
	}
	else
  #endif
	{
		MTXunlock(MyCfg->Mutex);
	}

	uDelay(PHY_POWERCHANGE_DELAY);

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Restart PHY auto-negotiation																		*/
/*																									*/
/* NOTE:																							*/
/*		Do not use in an interrupt because the PHY accesses are mutex protected.					*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_RestartNegotiation(int Dev)
{
	mETH_RestartNegotiation(Dev, g_EMACcfg[G_EMACreMap[Dev]].PHYaddr);

	return;
}

/* ------------------------------------------------------------------------------------------------ */

void mETH_RestartNegotiation(int Dev, int PHYaddr)
{
EMACcfg_t *MyCfg;
int       RegVal;

	MyCfg = &g_EMACcfg[G_EMACreMap[Dev]];

  #if ((ETH_MULTI_PHY) >= 0)						/* Alternate PHYs supported						*/
	if ((MyCfg->IsAltIF != 0)						/* If it is an alternate PHY					*/
	&&  ((MyCfg->PHYlist & (1U << PHYaddr)) == 0U)) {/* And is a not valid PHY, do nothing			*/
		return;
	}
  #endif

  #if (((ETH_ALT_PHY_IF) >= 0) && ((ETH_ALT_PHY_MTX) > 0))
	if (MyCfg->IsAltIF != 0) {						/* Doing a read-modify-write, need wrapping		*/
		MTXlock(g_MtxAltIF, -1);					/* protection									*/
	}												/* Even if ETH_ALT_PHY_MTX indicates no mutex	*/
	else											/* protection is still needed because of the	*/
  #endif											/* registers read-modify-write					*/
	{
		MTXlock(MyCfg->Mutex, -1);
	}

	RegVal = ETH_RdPhyReg(Dev, PHYaddr, PHY_BCR_REG)
	       | PHY_AUTO_NEGOTIATION_RESTART;

	ETH_WrtPhyReg(Dev, PHYaddr, PHY_BCR_REG, RegVal);

  #if (((ETH_ALT_PHY_IF) >= 0) && ((ETH_ALT_PHY_MTX) > 0))
	if (MyCfg->IsAltIF != 0) {						/* Done with the whoel region prtoection		*/
		MTXunlock(g_MtxAltIF);
	}
	else
  #endif
	{
		MTXunlock(MyCfg->Mutex);
	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Restart DMA TX if in suspended state																*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMArxRestart(int Dev)
{
volatile uint32_t *Reg;

	Reg = g_EMACcfg[G_EMACreMap[Dev]].HW;

	if (Reg[EMAC_DMA_STAT_REG] & EMAC_DMA_STAT_RU) {
		do {										/* Loop to eliminate possible spurious interrupt*/
			Reg[EMAC_DMA_STAT_REG] = EMAC_DMA_STAT_RU;
		} while (Reg[EMAC_DMA_STAT_REG] & (EMAC_DMA_STAT_RU));
		Reg[EMAC_DMA_RX_POLL_REG] = 0;				/* And resume DMA reception						*/
     }

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Restart DMA RX if in suspended state																*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMAtxRestart(int Dev)
{
volatile uint32_t *Reg;

	Reg = g_EMACcfg[G_EMACreMap[Dev]].HW;

	if (Reg[EMAC_DMA_STAT_REG] & EMAC_DMA_STAT_TU) {
		do {										/* Loop to eliminate possible spurious interrupt*/
			Reg[EMAC_DMA_STAT_REG] = EMAC_DMA_STAT_TU;
		} while (Reg[EMAC_DMA_STAT_REG] & (EMAC_DMA_STAT_TU));
		Reg[EMAC_DMA_TX_POLL_REG] = 0;				/* And resume DMA reception						*/
     }

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Disable EMAC transmission																		*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_TXdisable(int Dev)
{
	EMACreg(Dev, EMAC_CONF_REG) &= ~EMAC_CONF_TE;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Enable EMAC transmission																			*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_TXenable(int Dev)
{
   	EMACreg(Dev, EMAC_CONF_REG) |= EMAC_CONF_TE;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Disable EMAC reception																			*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_RXdisable(int Dev)
{
    EMACreg(Dev, EMAC_CONF_REG) &= ~EMAC_CONF_RE;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Enable EMAC reception																			*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_RXenable(int Dev)
{
    EMACreg(Dev, EMAC_CONF_REG) |= EMAC_CONF_RE;

	return;
}

/* ------------------------------------------------------------------------------------------------ */

int ETH_FlowControlStatus(int Dev)
{
	return(EMAC_FLOW_CTL_FCA_BPA & EMACreg(Dev, EMAC_FLOW_CTL_REG));
}

/* ------------------------------------------------------------------------------------------------ */
/* When Set In full duplex MAC initiates pause control frame										*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_PauseCtrlFrame(int Dev)
{ 
	EMACreg(Dev, EMAC_FLOW_CTL_REG) |= EMAC_FLOW_CTL_FCA_BPA;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Control of MAC back pressure																		*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_BackPressureDisable(int Dev)
{
	EMACreg(Dev, EMAC_FLOW_CTL_REG) &= ~EMAC_FLOW_CTL_FCA_BPA;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Control of MAC back pressure																		*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_BackPressureEnable(int Dev)
{
	EMACreg(Dev, EMAC_FLOW_CTL_REG) |= EMAC_FLOW_CTL_FCA_BPA;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Get the EMAC interrupt status																	*/
/* ------------------------------------------------------------------------------------------------ */

uint32_t ETH_IRQget(int Dev)
{
	return(EMACreg(Dev, EMAC_INT_STAT_REG));
}

/* ------------------------------------------------------------------------------------------------ */
/* Disable EMAC interrupts																			*/
/* Flags bit == 0:don't change / bit == 1:disable													*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_IRQclr(int Dev, uint32_t ISRflags)
{
   	EMACreg(Dev, EMAC_INT_MASK_REG) &= ~ISRflags;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Enable EMAC interrupts																			*/
/* Flags bit == 0:don't change / bit == 1:enable													*/
/* ------------------------------------------------------------------------------------------------ */


void ETH_IRQset(int Dev, uint32_t ISRflags)
{
   	EMACreg(Dev, EMAC_INT_MASK_REG) |= ISRflags;

	return;
}

/* ------------------------------------------------------------------------------------------------ */

int ETH_DMAgetSWR(int Dev)
{
	return(EMAC_DMA_BUS_SWR & EMACreg(Dev, EMAC_DMA_BUS_REG));
}

/* ------------------------------------------------------------------------------------------------ */
/* Disable DMA interrupts																			*/
/*																									*/
/* Flags bit == 0:don't change / bit == 1:disable													*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMAisrDisable(int Dev, uint32_t ISRflags)
{
	EMACreg(Dev, EMAC_DMA_INT_REG) &= ~ISRflags;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Enable DMA interrupts																			*/
/*																									*/
/* Flags bit == 0:don't change / bit == 1:enable													*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMAisrEnable(int Dev, uint32_t ISRflags)
{
	EMACreg(Dev, EMAC_DMA_INT_REG) |= ISRflags;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Read the DMA interrupt status register															*/
/* ------------------------------------------------------------------------------------------------ */

uint32_t ETH_DMAgetIRQstatus(int Dev)
{
	return(EMACreg(Dev, EMAC_DMA_STAT_REG));
}

/* ------------------------------------------------------------------------------------------------ */
/* Clear the selected ETHERNET DMA FLAG																*/
/* Flags bit == 0:don't change / bit == 1:clear														*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMAclrIRQstatus(int Dev, uint32_t IRQflags)
{
	EMACreg(Dev, EMAC_DMA_STAT_REG) = IRQflags;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Clear the Ethernet DMA Rx/Tx IT pending bits														*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMAclrPendIRQ(int Dev)
{
	EMACreg(Dev, EMAC_DMA_STAT_REG) = EMAC_DMA_INT_NIE
	                                | EMAC_DMA_INT_RIE
	                                | EMAC_DMA_INT_TIE;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Get the state of the transmission DMA															*/
/*																									*/
/* returns:																							*/
/*			== 0 : currently transmitting															*/
/*			!= 0 : no transmission active															*/
/* ------------------------------------------------------------------------------------------------ */

uint32_t ETH_DMAgetTXstate(int Dev)
{
	return(EMAC_DMA_STAT_TI & EMACreg(Dev, EMAC_DMA_STAT_REG));
}

/* ------------------------------------------------------------------------------------------------ */
/* Flush the Transmit FIFO																			*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMAflushTX(int Dev)
{
	EMACreg(Dev, EMAC_DMA_OPMODE_REG) |= EMAC_DMA_OPMODE_FTF;

	return;
}

/* ------------------------------------------------------------------------------------------------ */

int ETH_DMAgetFlushTXstatux(int Dev)
{   
	return(EMAC_DMA_OPMODE_FTF & EMACreg(Dev, EMAC_DMA_OPMODE_REG));
}

/* ------------------------------------------------------------------------------------------------ */
/* Disable transmission																				*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMAtxDisable(int Dev)
{
	EMACreg(Dev, EMAC_DMA_OPMODE_REG) &= ~EMAC_DMA_OPMODE_ST;

	return;
}
   
/* ------------------------------------------------------------------------------------------------ */
/* Enable transmission																				*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMAtxEnable(int Dev)
{
	EMACreg(Dev, EMAC_DMA_OPMODE_REG) |= EMAC_DMA_OPMODE_ST;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Disable reception																				*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMArxDisable(int Dev)
{
	EMACreg(Dev, EMAC_DMA_OPMODE_REG) &= ~EMAC_DMA_OPMODE_SR;

	return;
}
   
/* ------------------------------------------------------------------------------------------------ */
/* Enable reception																					*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_DMArxEnable(int Dev)
{ 
	EMACreg(Dev, EMAC_DMA_OPMODE_REG) |= EMAC_DMA_OPMODE_SR;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Returns OVflagMask ANDed with register contents													*/
/* ------------------------------------------------------------------------------------------------ */
 
uint32_t ETH_DMAoverflowStat(int Dev, uint32_t OVflagMask)
{
	return(EMACreg(Dev, EMAC_DMA_MF_OVFL_CNT_REG) & OVflagMask);
}

/* ------------------------------------------------------------------------------------------------ */
/* Disable EMAC's checksum interrupts																*/
/*																									*/
/* Flags bit == 0:don't change / bit == 1:disable													*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_MMCchksumDisable(int Dev, uint32_t IRQflags)
{
	EMACreg(Dev, EMAC_MMC_CKS_INT_MASK_REG) |= IRQflags;	/* Negative logic  0:enable / 1:disable	*/

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Enable EMAC's checksum interrupts																*/
/*																									*/
/* Flags bit == 0:don't change / bit == 1:enable													*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_MMCchksumEnable(int Dev, uint32_t IRQflags)
{
	EMACreg(Dev, EMAC_MMC_CKS_INT_MASK_REG) &= ~IRQflags;	/* Negative logic  0:enable / 1:disable	*/

	return;
}

/* ------------------------------------------------------------------------------------------------ */

uint32_t ETH_MMCchksumStat(int Dev)
{
	return(EMACreg(Dev, EMAC_MMC_CKS_INT_STAT_REG));
}

/* ------------------------------------------------------------------------------------------------ */
/* Disable EMAC's MMC RX interrupts																	*/
/*																									*/
/* Flags bit == 0:don't change / bit == 1:disable													*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_MMCrxDisable(int Dev, uint32_t IRQflags)
{
	EMACreg(Dev, EMAC_MMC_RX_INT_MASK_REG) |= IRQflags;		/* Negative logic  0:enable / 1:disable	*/

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Enable EMAC's MMC RX interrupts																	*/
/*																									*/
/* Flags bit == 0:don't change / bit == 1:enable													*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_MMCrxEnable(int Dev, uint32_t IRQflags)
{
	EMACreg(Dev, EMAC_MMC_RX_INT_MASK_REG) &= ~IRQflags;	/* Negative logic  0:enable / 1:disable	*/

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Read the EMAC RX interrupt status register														*/
/* ------------------------------------------------------------------------------------------------ */

uint32_t ETH_MMCrxStat(int Dev)
{
	return(EMACreg(Dev, EMAC_MMC_RX_INT_STAT_REG));
}

/* ------------------------------------------------------------------------------------------------ */
/* Enable EMAC TX interrupts																		*/
/*																									*/
/* Flags bit == 0:don't change / bit == 1:disable													*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_MMCtxDisable(int Dev, uint32_t IRQflags)
{
	EMACreg(Dev, EMAC_MMC_TX_INT_MASK_REG) |= IRQflags;		/* Negative logic  0:enable / 1:disable	*/

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Enable EMAC TX interrupts																		*/
/*																									*/
/* Flags bit == 0:don't change / bit == 1:enable													*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_MMCtxEnable(int Dev, uint32_t IRQflags)
{
	EMACreg(Dev, EMAC_MMC_TX_INT_MASK_REG) &= ~IRQflags;	/* Negative logic  0:enable / 1:disable	*/

	return;
}

/* ------------------------------------------------------------------------------------------------ */

uint32_t ETH_MMCtxStat(int Dev)
{
	return(EMACreg(Dev, EMAC_MMC_TX_INT_STAT_REG));
}

/* ------------------------------------------------------------------------------------------------ */
/* Report of the link status : connected or not connected											*/
/*																									*/
/* returns																							*/
/*			== 0 : no link connected																*/
/*			!= 0 : link connected																	*/
/*																									*/
/* NOTE:																							*/
/*		Do not use in an interrupt because the PHY accesses are mutex protected.					*/
/* ------------------------------------------------------------------------------------------------ */

int ETH_LinkStatus(int Dev)
{
	return(mETH_LinkStatus(Dev, g_EMACcfg[G_EMACreMap[Dev]].PHYaddr));

}

/* ------------------------------------------------------------------------------------------------ */

int mETH_LinkStatus(int Dev, int PHYaddr)
{
EMACcfg_t     *MyCfg;								/* Configuration / state of this device			*/
int            RegVal;

	MyCfg = &g_EMACcfg[G_EMACreMap[Dev]];

  #if ((ETH_MULTI_PHY) < 0)
	if (MyCfg->IsInInit > 0) {						/* In PHY_init & link was up so it's			*/
		RegVal = PHY_LINK_STATUS;					/* a re-connect. Keep link up report			*/
	} else if (MyCfg->IsInInit < 0) {				/* In PHY_init & link was down so it's			*/
		RegVal = 0;									/* a re-connect. Keep link down report			*/
	}
	else {											/* Not in PHY_init() report true state			*/
		RegVal = ETH_RdPhyReg(Dev, PHYaddr, PHY_BSR_REG)
		       & (PHY_LINK_STATUS);
	}
  #else
	if ((MyCfg->PHYlist & (1U << PHYaddr)) == 0U) {
		RegVal = -1;								/* Non-existent PHYs are reported as link up	*/
	}												/* but as -1 if non-existence info needed		*/
	else if ((MyCfg->IsInInit & (1 << PHYaddr)) != 0) {
		RegVal = PHY_LINK_STATUS;
	}
	else {
		RegVal = ETH_RdPhyReg(Dev, PHYaddr, PHY_BSR_REG)
		       & (PHY_LINK_STATUS);
	}
  #endif

	return(RegVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Read a PHY register																				*/
/* ------------------------------------------------------------------------------------------------ */

static int ETH_RdPhyReg(int Dev, int PHYaddr, int PHYreg)
{
EMACcfg_t         *MyCfg;							/* Configuration / state of this device			*/
volatile uint32_t *Register;						/* uint32_t pointer to access EMAC registers	*/
int                RetVal;
int                Tout;

	MyCfg    = &g_EMACcfg[G_EMACreMap[Dev]];
	Register = MyCfg->HW;

  #if ((ETH_ALT_PHY_IF) != 0)
	if (MyCfg->IsAltIF != 0) {						/* If is an alternate PHY I/F					*/
	  #if ((ETH_ALT_PHY_MTX) > 0)
		MTXlock(g_MtxAltIF, -1);
		RetVal = AltPhyIF(Dev, ETH_ALT_PHY_IF_REG_RD, PHYaddr, PHYreg, 0);
		MTXunlock(g_MtxAltIF);
	  #elif ((ETH_ALT_PHY_MTX) == 0)
		MTXlock(MyCfg->Mutex, -1);
		RetVal = AltPhyIF(Dev, ETH_ALT_PHY_IF_REG_RD, PHYaddr, PHYreg, 0);
		MTXunlock(MyCfg->Mutex);
	  #else
		RetVal = AltPhyIF(Dev, ETH_ALT_PHY_IF_REG_RD, PHYaddr, PHYreg, 0);
	  #endif
		return(0xFFFF& RetVal);
	}
  #endif

	MTXlock(MyCfg->Mutex, -1);

	Register[EMAC_GMII_ADDR_REG] = EMAC_GMII_ADDR_GB
	                    | ((PHYaddr                    << EMAC_GMII_ADDR_PA_SHF) & EMAC_GMII_ADDR_PA)
	                    | ((PHYreg                     << EMAC_GMII_ADDR_GR_SHF) & EMAC_GMII_ADDR_GR)
	                    | ((EMAC_GMII_ADDR_CR_E_DIV102 << EMAC_GMII_ADDR_CR_SHF) & EMAC_GMII_ADDR_CR);

	for (Tout=PHY_READ_TO ; Tout>0 ; Tout--) {
		if (((EMAC_GMII_ADDR_GB & Register[EMAC_GMII_ADDR_REG])) == 0) {
			break;	 								/* Wait until not busy							*/
		}
		uDelay(1);
	}

	RetVal = (Tout <= 0)							/* Return ERROR in case of timeout				*/
	       ? -1
	       : (int)(0xFFFF							/* Return data register value					*/
		         & Register[EMAC_GMII_DATA_REG]);

	MTXunlock(MyCfg->Mutex);

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Write a PHY register																				*/
/* ------------------------------------------------------------------------------------------------ */

static int ETH_WrtPhyReg(int Dev, int PHYaddr, int PHYreg, int PHYval)
{
EMACcfg_t         *MyCfg;							/* Configuration / state of this device			*/
volatile uint32_t *Register;						/* uint32_t pointer to access EMAC registers	*/
int                Tout;


	MyCfg    = &g_EMACcfg[G_EMACreMap[Dev]];
	Register = MyCfg->HW;

  #if ((ETH_ALT_PHY_IF) != 0)
	if (MyCfg->IsAltIF != 0) {						/* If is an alternate PHY I/F					*/
	  #if ((ETH_ALT_PHY_MTX) > 0)
		MTXlock(g_MtxAltIF, -1);
		Tout = ! AltPhyIF(Dev, ETH_ALT_PHY_IF_REG_WRT, PHYaddr, PHYreg, PHYval);
		MTXunlock(g_MtxAltIF);
	  #elif ((ETH_ALT_PHY_MTX) == 0)
		MTXlock(MyCfg->Mutex, -1);
		Tout = ! AltPhyIF(Dev, ETH_ALT_PHY_IF_REG_WRT, PHYaddr, PHYreg, PHYval);
		MTXunlock(MyCfg->Mutex);
	  #else
		Tout = ! AltPhyIF(Dev, ETH_ALT_PHY_IF_REG_WRT, PHYaddr, PHYreg, PHYval);
	  #endif
		return(Tout == 0);
	}
  #endif

	MTXlock(MyCfg->Mutex, -1);

	Register[EMAC_GMII_DATA_REG] = 0xFFFF & PHYval;	/* Give value to MII data reg					*/
	Register[EMAC_GMII_ADDR_REG] = EMAC_GMII_ADDR_GW
	                             | EMAC_GMII_ADDR_GB
	                    | ((PHYaddr                    << EMAC_GMII_ADDR_PA_SHF) & EMAC_GMII_ADDR_PA)
	                    | ((PHYreg                     << EMAC_GMII_ADDR_GR_SHF) & EMAC_GMII_ADDR_GR)
                        | ((EMAC_GMII_ADDR_CR_E_DIV102 << EMAC_GMII_ADDR_CR_SHF) & EMAC_GMII_ADDR_CR);

	for (Tout=PHY_WRITE_TO ; Tout>0 ; Tout--) {
		if (((EMAC_GMII_ADDR_GB & Register[EMAC_GMII_ADDR_REG])) == 0) {
			break;	 								/* Wait until not busy							*/
		}
		uDelay(1);
	}

	MTXunlock(MyCfg->Mutex);

	return(Tout == 0);								/* Return ERROR in case of timeout				*/
}

/* ------------------------------------------------------------------------------------------------ */
/* Read a PHY extended register																		*/
/* ------------------------------------------------------------------------------------------------ */

static int ETH_RdPhyRegExt(int PHYid, int Dev, int PHYaddr, int PHYreg)
{
EMACcfg_t *MyCfg;									/* Configuration / state of this device			*/
int        RetVal;

	RetVal = 0;
	MyCfg  = &g_EMACcfg[G_EMACreMap[Dev]];

	MTXlock(MyCfg->Mutex, -1);

	if (PHYid == PHY_ID_KSZ9021) {
		ETH_WrtPhyReg(Dev, PHYaddr, PHY_PSE_CTL_REG, PHYreg);

		RetVal = ETH_RdPhyReg(Dev, PHYaddr, PHY_MMD_CTL_REG);
	}
	else if (PHYid == PHY_ID_KSZ9031) {
		ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_CTL_REG, 2);
		ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_DATA_REG, PHYreg);
		ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_CTL_REG, 0x8002);

		RetVal = ETH_RdPhyReg(Dev, PHYaddr, PHY_MMD_DATA_REG);
	}
	else if (PHYid == PHY_ID_TIDP83867) {
		ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_CTL_REG, 0x001F);	/* DEVAD can only be 0x1F			*/
		ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_DATA_REG, PHYreg);	/* Set address						*/
		ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_CTL_REG, 0x401F);	/* Data rd++ and wrt++				*/

		RetVal = ETH_RdPhyReg(Dev, PHYaddr, PHY_MMD_DATA_REG);
	}

	MTXunlock(MyCfg->Mutex);

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Write to a PHY extended register																	*/
/* ------------------------------------------------------------------------------------------------ */

static int ETH_WrtPhyRegExt(int PHYid, int Dev, int PHYaddr, int PHYreg, int PHYval)
{
EMACcfg_t *MyCfg;									/* Configuration / state of this device			*/
int        RetVal; 

	RetVal = 0;
	MyCfg  = &g_EMACcfg[G_EMACreMap[Dev]];

	MTXlock(MyCfg->Mutex, -1);

	if (PHYid == PHY_ID_KSZ9021) {
		RetVal  = ETH_WrtPhyReg(Dev, PHYaddr, PHY_PSE_CTL_REG, 0x8000 | PHYreg);
		RetVal |= ETH_WrtPhyReg(Dev, PHYaddr, PHY_PSE_STAT_REG, PHYval);
	}
	else if (PHYid == PHY_ID_KSZ9031) {
		RetVal  = ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_CTL_REG, 2);
		RetVal |= ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_DATA_REG, PHYreg);
		RetVal |= ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_CTL_REG, 0x4002);
		RetVal |= ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_DATA_REG, PHYval);
	}
	else if (PHYid == PHY_ID_TIDP83867) {
		RetVal  = ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_CTL_REG, 0x001F);
		RetVal |= ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_DATA_REG, PHYreg);
		RetVal |= ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_CTL_REG, 0x401F);
		RetVal |= ETH_WrtPhyReg(Dev, PHYaddr, PHY_MMD_DATA_REG, PHYval);
	}

	MTXunlock(MyCfg->Mutex);

    return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* ETH_MacConfigDMA - configure and init the MAC													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int  ETH_MacConfigDMA(int Dev, int Rate)													*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : controller number																	*/
/*		Rate : link rate (See below)																*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int : > 0 success, value for the rate established											*/
/*		      < 0 error																				*/
/*																									*/
/* Rate values:																						*/
/*		  10 :   10 Mbps full duplex			 													*/
/*		  11 :   10 Mbps half duplex			 													*/
/*		 100 :  100 Mbps full duplex			 													*/
/*		 101 :  100 Mbps half duplex			 													*/
/*		1000 : 1000 Mbps full duplex			 													*/
/*		1001 : 1000 Mbps half duplex		 														*/
/*		-ve  : auto-negotiation (only as the argument, not for the return value)					*/
/*		       any other values that 10,11,100,101,1000,1001 remaps to -ve							*/
/* ------------------------------------------------------------------------------------------------ */

int ETH_MacConfigDMA(int Dev, int Rate)
{                                        
int ii;												/* General purpose								*/
volatile uint32_t *Register;						/* uint32_t pointer to access EMAC registers	*/
uint32_t RegBus;
uint32_t RegMode;

	if ((Dev < 0)
	||  (Dev >= ETH_MAX_DEVICES)
	||  (G_EMACreMap[Dev] < 0)) {
		return(-1);
	}

	Register = G_EMACaddr[G_EMACreMap[Dev]];
	RegBus   = Register[EMAC_DMA_BUS_REG];
	RegMode  = Register[EMAC_DMA_OPMODE_REG];
													/* Reset the DMA block							*/
	Register[EMAC_DMA_BUS_REG] |= EMAC_DMA_BUS_SWR;
	for (ii=32; ii>=0; ii--) {						/* Wait for the reset to be cleared (done)		*/
		uDelay(PHY_RESET_DELAY);					/* We use >=0 instead of >0 for fall trough		*/
		if (ETH_DMAgetSWR(Dev) == 0) {
			break;
		}
	}

	Register[EMAC_DMA_BUS_REG]    = RegBus;
	Register[EMAC_DMA_OPMODE_REG] = RegMode;

	if (ii >= 0) {									/* Configure the ethernet PHY					*/
		ii = PHY_init(Dev, Rate);
	}

	if (ii >= 0) {									/* Enable the selected DMA interrupts if no err	*/
		Register[EMAC_DMA_INT_REG] |= EMAC_DMA_INT_NIE
		                           |  EMAC_DMA_INT_RIE
		                           |  EMAC_DMA_INT_TIE;
	}

	return(ii);
}
/* ------------------------------------------------------------------------------------------------ */
/* Reset & initialize all EMACs																		*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_ResetEMACs(void)
{
int Dev;

	for (Dev=0 ; Dev<(sizeof(G_EMACaddr)/sizeof(G_EMACaddr[0])) ; Dev++) {
		ETH_ResetEMAC(Dev);							/* ETH_ResetEMAC() checks for valid EMAC		*/
	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Reset & initialize individual EMAC																*/
/* ------------------------------------------------------------------------------------------------ */

void ETH_ResetEMAC(int Dev)
{
EMACcfg_t         *MyCfg;							/* Configuration / state of this device			*/
volatile uint32_t *Register;						/* uint32_t pointer to access EMAC registers	*/
uint32_t           RegVal;
int                ReMap;

	if ((Dev < 0)
	||  (Dev >= ETH_MAX_DEVICES)) {
		return;
	}

	if (G_EMACreMap[Dev] < 0) {						/* Only reset supported EMACs					*/
		return;
	}

	ReMap = G_EMACreMap[Dev];
	MyCfg = &g_EMACcfg[ReMap];

	MTXlock(G_OSmutex, -1);
	 if (g_CfgIsInit == 0) {
		memset(&g_EMACcfg[0], 0, sizeof(g_EMACcfg));
		g_CfgIsInit = 1;
	  #if ((ETH_ALT_PHY_MTX) > 0)
		g_MtxAltIF = MTXopen("ETH ALT IF");
	  #endif
	 }
	 if (MyCfg->Mutex == (MTX_t *)NULL) {
		MyCfg->Mutex = MTXopen(g_PhyMtxName[Dev]);
		MyCfg->HW    = G_EMACaddr[ReMap];
	 }
	MTXunlock(G_OSmutex);

	MTXlock(MyCfg->Mutex, -1);

	Register = MyCfg->HW;

	MyCfg->IsInInit =  0;
	MyCfg->RxDesc   = &G_DMArxDescTbl[ReMap][0];
	MyCfg->TxDesc   = &G_DMAtxDescTbl[ReMap][0];
	MyCfg->PHYaddr  = -1;
  #if ((ETH_MULTI_PHY) < 0)
	MyCfg->PHYid    =  0;
  #else
	MyCfg->PHYlist  = 0;
	memset(&MyCfg->PHYid, 0, sizeof(MyCfg->PHYid));
  #endif

	EMAC_RESET(Dev);								/* Reset this EMAC device						*/

	uDelay(PHY_RESET_DELAY);						/* Small delay to allow it to start-up			*/

  #if ((ETH_ALT_PHY_IF) != 0)
   #if ((ETH_ALT_PHY_MTX) > 0)
	MTXlock(g_MtxAltIF, -1);
	MyCfg->IsAltIF = AltPhyIF(Dev, ETH_ALT_PHY_IF_INIT, 0, 0, 0);
	MTXunlock(g_MtxAltIF);
   #else											/* Device muyex (MyCfg->Mutex) already locked	*/
	MyCfg->IsAltIF = AltPhyIF(Dev, ETH_ALT_PHY_IF_INIT, 0, 0, 0);
   #endif
  #endif

	g_Dummy = Register[EMAC_PHY_CTL_STAT_REG];
													/* Disable the MMC interrupts					*/
	Register[EMAC_MMC_TX_INT_MASK_REG]  = 0xFFFFFFFF;
	Register[EMAC_MMC_RX_INT_MASK_REG]  = 0xFFFFFFFF;
	Register[EMAC_MMC_CKS_INT_MASK_REG] = 0xFFFFFFFF;
													/* Disable MAC interrupts						*/
	RegVal                      = Register[EMAC_INT_MASK_REG]
	                            & 0xFFFFF9F8;		/* Don't touch the reserved bits				*/
	Register[EMAC_INT_MASK_REG] = RegVal
	                            | EMAC_INT_STAT_LPIIS
	                            | EMAC_INT_STAT_TSIS;

	RegVal                      = Register[EMAC_CONF_REG]
	                            & 0xF4000000;		/* Don't touch the reserved bits				*/
	Register[EMAC_CONF_REG]     = RegVal
	                            | EMAC_CONF_IPC		/* Checksum Offload								*/
	                            | EMAC_CONF_PS		/* Port Select = MII							*/
	                            | EMAC_CONF_BE		/* Frame Burst Enable							*/
	                            | EMAC_CONF_WD;		/* Watchdog Disable								*/
													/* Config the DMA								*/
	RegVal                      = Register[EMAC_MAC_FILT_REG]
	                            & 0x7FCEF800;		/* Don't touch the reserved bits				*/
	Register[EMAC_MAC_FILT_REG] = RegVal
	                            | (1<<6)			/* Block all control frames						*/
	                            | (1<<0);			/* Promiscuous mode disable						*/

	RegVal                      = Register[EMAC_FLOW_CTL_REG]
	                            & 0x0000FF40;		/* Don't touch the reserved bits				*/
	Register[EMAC_FLOW_CTL_REG] = RegVal
	                            | EMAC_FLOW_CTL_DZPQ;	/* Disable zero quanta pause				*/

													/* Set the DMA Bus Mode Register				*/
	EMACreg(Dev, EMAC_DMA_BUS_REG) = EMAC_DMA_BUS_USP 	/* Use separate PBL							*/
		                           | EMAC_DMA_BUS_AAL	/* Address Aligned Beats					*/
		                         #ifdef USE_ENHANCED_DMA_DESCRIPTORS
		                           | EMAC_DMA_BUS_ATDS	/* Alternate Desc Size						*/
		                         #endif
		                           | EMAC_DMA_BUS_EIGHTXPBL
		                           | ((8 << EMAC_DMA_BUS_PBL_SHF)  & EMAC_DMA_BUS_PBL)
		                           | ((8 << EMAC_DMA_BUS_RPBL_SHF) & EMAC_DMA_BUS_RPBL);

	EMACreg(Dev, EMAC_DMA_OPMODE_REG) |= EMAC_DMA_OPMODE_RSF	/* RX store and forward				*/
	                                  |  EMAC_DMA_OPMODE_OSF	/* Operate early on 2nd TX Frame	*/
	                                  |  EMAC_DMA_OPMODE_TSF;	/* TX Store and Forward				*/

	MTXunlock(MyCfg->Mutex);

	uDelay(10000);

	return;
}

/* EOF */
