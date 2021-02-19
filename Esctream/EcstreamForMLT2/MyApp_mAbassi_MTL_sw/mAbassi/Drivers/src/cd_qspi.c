/* ------------------------------------------------------------------------------------------------ */
/* FILE :		cd_qspi.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the Cadence QSPI															*/
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
/*	$Revision: 1.25 $																				*/
/*	$Date: 2019/02/10 16:54:18 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* Some info on how things are done:																*/
/*																									*/
/*		qspi_read()																					*/
/*			EOT is declared when the last byte has been read from the RX FIFO.						*/
/*			Is always verified at the end that the controler has rerached the Idle state			*/
/*																									*/
/*		qspi_write()																				*/
/*			The TX FIFO is 256 byte deep and all SQPI part currently supported have pages of 256	*/
/*			bytes or less.  So there are no gain transferring the data to the TX FIFO with ISRs		*/
/*			or DMA du rto the overhead of the ISR and DMA. So that's why write transfers in the ISR	*/
/*			or with the DMA is not supported.  ISRs can be used though to block on a semaphore		*/
/*			until all the data has been sent out. The biuld option keeps its menaing but the		*/
/*			build option QSPI_ISR_TX_THRS becomes the level at which the EOT is declared in the		*/
/*			ISR.  By using a non-zero value, it can optimize the response time by taking into		*/
/*			account the ISR ovberhead as the time left to truly empty the TX FIFO					*/
/*																									*/
/* NOTES:																							*/
/*	1-	When the QSPI flash part is larger than 16 MBytes (requiring 32 address bits), the access	*/
/*		is done using 32 bit addresses when possible: i.e. the part supports 4 byte addresses.		*/
/*	2-  When adding a new part, add all the information for it in g_PartInfo[]						*/
/*		All sections that may require to be updated when a new part is added are enclosed between	*/
/*				+++++ PART ADD START +++++++++++++++++++++++										*/
/*								and																	*/
/*				+++++ PART ADD END +++++++++++++++++++++++++										*/
/*																									*/
/* LIMITATIONS:																						*/
/*		1- DDR (Double Data Rate) is not supported by the QSPI controller.							*/
/*		2- When writing (qspi_write()), transfers are always done with polling bacause almost all	*/
/*		   QSPI parts have programming pages of 256 and the TX FIFO is 256 bytes deep. So the		*/
/*		   largest amount of data to program all fit in the TX FIFO.  The TX FIFO is filled and		*/
/*		   then the driver wait for the end of the transfer.										*/
/*          																						*/
/*																									*/
/* NOT YET SUPPORTED:																				*/
/*																									*/
/* Micron Parts:																					*/
/* Some Micron N25Q parts exist in two variants: with die erase or with bulk erase, never both		*/
/* To make the table applicable to both variants, the bulk / die erase of these parts is not		*/
/* supported meaning a full chip erase is performed sector by sector.								*/
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

#include "cd_qspi.h"

#ifndef QSPI_USE_MUTEX								/* If a mutex is used to protect accesses		*/
  #define QSPI_USE_MUTEX		1					/* == 0 : No mutex       != 0 : Use mutex		*/
#endif
													/* Bit #0->7 are for RX / Bit #8->15 are for TX	*/
#ifndef QSPI_OPERATION								/* Bit #0/#8  : Disable ISRs on burst			*/
  #define QSPI_OPERATION		0x10101				/* Bit #1/#9  : Interrupts are used				*/
#endif												/* Bit #2     : DMA is used (RX ONLY)			*/
													/* Bit #16    : use TSKsleep() during erase		*/
#ifndef QSPI_ISR_RX_THRS							/* When using RX ISRs, percentage (0->100) of	*/
  #define QSPI_ISR_RX_THRS		50					/* the RX FIFO when the ISR is triggered		*/
#endif												/* This is when RX FIFO contents > threshold	*/

#ifndef QSPI_ISR_TX_THRS							/* When using TX ISRs, percentage (0->100) of	*/
  #define QSPI_ISR_TX_THRS		32					/* the TX FIFO when the ISR is triggered		*/
#endif												/* This is when TX FIFO contents <= threshold	*/

#ifndef QSPI_MIN_4_RX_DMA
  #define QSPI_MIN_4_RX_DMA		64					/* Minimum number of bytes to use RX DMA		*/
#endif

#ifndef QSPI_MIN_4_TX_DMA
  #define QSPI_MIN_4_TX_DMA		16					/* Minimum number of bytes to use TX DMA		*/
#endif												/* IGNORED: DMA is not used for writing			*/

#ifndef QSPI_MIN_4_RX_ISR
  #define QSPI_MIN_4_RX_ISR		64					/* Minimum number of bytes to use RX interrupts	*/
#endif

#ifndef QSPI_MIN_4_TX_ISR
  #define QSPI_MIN_4_TX_ISR		16					/* Minimum number of bytes to use TX interrupts	*/
#endif												/* This sets the threshold to declared EOT		*/

#ifndef QSPI_MAX_BUS_FREQ							/* If the target bus BW is limited for one		*/
  #define QSPI_MAX_BUS_FREQ		0					/* or another, set to != 0 with the value is	*/
#endif												/* specified in MHz (4 is 4,000,000 Hz)			*/

#ifndef QSPI_READ_ONLY
  #define QSPI_READ_ONLY		0					/* Set to non-zero to disable write & erase		*/
#endif

#ifndef QSPI_MULTICORE_ISR
  #define QSPI_MULTICORE_ISR	1					/* When operating on a multicore, set to != 0	*/
#endif												/* if more than 1 core process the interrupt	*/

#ifndef QSPI_TOUT_ISR_ENB							/* Set to 0 if timeouts not required in polling	*/
  #define QSPI_TOUT_ISR_ENB		1					/* When timeout used, if ISRs requested to be	*/
#endif												/* disable, enable-disable to update/read tick	*/
													/* to allow update of the RTOS timer tick count	*/
#ifndef QSPI_REMAP_LOG_ADDR
  #define QSPI_REMAP_LOG_ADDR	1					/* If remapping logical to physical adresses	*/
#endif												/* Only used/needed with DMA transfers			*/

#ifndef QSPI_RDDLY_0_0
  #define QSPI_RDDLY_0_0		-1					/* RdDly table entry overload for Dev #0 Slv #0	*/
#endif												/* < 0 use table entry; >= 0 use QSPI_RDDLY_0_0	*/

#ifndef QSPI_RDDLY_0_1
  #define QSPI_RDDLY_0_1		-1					/* RdDly table entry overload for Dev #0 Slv #1	*/
#endif												/* < 0 use table entry; >= 0 use QSPI_RDDLY_0_1	*/

#ifndef QSPI_RDDLY_0_2
  #define QSPI_RDDLY_0_2		-1					/* RdDly table entry overload for Dev #0 Slv #2	*/
#endif												/* < 0 use table entry; >= 0 use QSPI_RDDLY_0_2	*/

#ifndef QSPI_RDDLY_0_3
  #define QSPI_RDDLY_0_3		-1					/* RdDly table entry overload for Dev #0 Slv #3	*/
#endif												/* < 0 use table entry; >= 0 use QSPI_RDDLY_0_3	*/

#ifndef QSPI_ARG_CHECK								/* If checking validity of function arguments	*/
  #define QSPI_ARG_CHECK		1
#endif

#ifndef QSPI_CMD_LOWEST_FRQ							/* == 0 sending R/W commands use current freq	*/
  #define QSPI_CMD_LOWEST_FRQ	-1					/* >  0 Lowest freq + control dis/enb changing	*/
#endif												/* <  0 Lowest freq without control dis/enb		*/

#ifndef QSPI_DEBUG									/* <= 0 no debug information is printed			*/
  #define QSPI_DEBUG			0					/* == 1 print only initialization information	*/
#endif												/* >= 2 print all information					*/

#ifndef QSPI_ID_ONLY								/* Usefull when adding/checking the driver		*/
  #define QSPI_ID_ONLY			0					/* Set to non-zero will print the JEDEC ID and	*/
#endif												/* if ID is in the table and exit with error	*/

#ifndef QSPI_MX25R_LOW_POW							/* For Macronix MX25R series					*/
  #define QSPI_MX25R_LOW_POW	0					/* Set to != 0 if to be use in ultra low power	*/
#endif												/* instead of being used in high performance	*/

/* ------------------------------------------------------------------------------------------------ */
/* Base addresses of the HW QSPI modules															*/

#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)		/* Arria V / Cyclone V							*/
  static volatile uint32_t * const g_HWaddr[1]  = { (volatile uint32_t *)0xFF705000};
  static volatile uint32_t * const g_AHBbase[1] = { (volatile uint32_t *)0xFFA00000}; 

  #define QSPI_RESET_DEV(Dev)	do {																\
									*((volatile uint32_t *)0xFFD05014) |=  (1U << 5);				\
									*((volatile uint32_t *)0xFFD05014) &= ~(1U << 5);				\
								} while(0)

 #if (((QSPI_OPERATION) & 0x00404) != 0)
  #include "arm_pl330.h"

  #define DMA_DEV				0					/* Only one DMA on Arria V & Ccyclone V			*/
  static int g_DMAchan[][2] = {{24, 25}				/* QSPI #0 DMA: TX chan, RX chan				*/
                              };
 #endif

 #ifndef QSPI_CLK
  #if ((OS_PLATFORM) == 0x4000AAC5)					/* Arria 5 uses a different clock				*/
   #define QSPI_CLK				350000000
  #else
   #define QSPI_CLK				370000000
  #endif
 #endif

 #ifndef QSPI_MAX_SLAVES
  #define QSPI_MAX_SLAVES		4
 #endif

  #define QSPI_SRAM_SIZE		512					/* Combined RX & TX FIFO size in bytes			*/
  #define QSPI_RX_FIFO_SIZE		256					/* SRAM distribution is programmable... but it	*/
  #define QSPI_TX_FIFO_SIZE		256					/* does not seem to work when not half-half.	*/

  #if ((QSPI_MAX_DEVICES) > 1)
    #error "Too many QSPI devices: set QSPI_MAX_DEVICES <= 1 and/or QSPI_LIST_DEVICE <= 0x1"
  #endif

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)	/* Arria 10										*/
  static volatile uint32_t * const g_HWaddr[1]  = { (volatile uint32_t *)0xFF809000};
  static volatile uint32_t * const g_AHBbase[1] = { (volatile uint32_t *)0xFFA00000}; 

  #define QSPI_RESET_DEV(Dev)	do {																\
									*((volatile uint32_t *)0xFFD05024) |=  (1U << 6);				\
									*((volatile uint32_t *)0xFFD05024) &= ~(1U << 6);				\
								} while(0)

 #if (((QSPI_OPERATION) & 0x00404) != 0)
  #include "arm_pl330.h"

  #define DMA_DEV				0					/* Only one DMA on Arria 10						*/
  static int g_DMAchan[][2] = {{24, 25}				/* QSPI #0 DMA: TX chan, RX chan				*/
                              };
 #endif

 #ifndef QSPI_CLK
  #define QSPI_CLK				370000000
 #endif

 #ifndef QSPI_MAX_SLAVES
  #define QSPI_MAX_SLAVES		4
 #endif

  #define QSPI_SRAM_SIZE		512					/* Combined RX & TX FIFO size in bytes			*/
  #define QSPI_RX_FIFO_SIZE		256					/* SRAM distribution is programmable... but it	*/
  #define QSPI_TX_FIFO_SIZE		256					/* does not seem to work when not half-half.	*/

  #if ((QSPI_MAX_DEVICES) > 1)
    #error "Too many QSPI devices: set QSPI_MAX_DEVICES <= 1 and/or QSPI_LIST_DEVICE <= 0x1"
  #endif

#else
  	#error "Unsupported platform specified by OS_PLATFORM"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* QSPI flash command supported by most manufacturers.												*/
/*		- Macronix																					*/
/*		- Micron																					*/
/*		- Winbond																					*/
/*		- Spansion (Cypress)																		*/
/*																									*/
/* If you use them, make sure they are the correct value for the part you are using.				*/
/*																									*/
/* The read & write op-code definition are constructed with 3 nibbles and 1 byte:					*/
/*		0xDAIOO																						*/
/*		    where D : # lanes for the data															*/
/*		          A : # lanes for the address														*/
/*		          I : # lanes for the opcode														*/
/*		         OO : op-code (1 byte)																*/
/* 																									*/
/*		And the # of lanes is specified with:														*/
/*		          0 : 1 lane																		*/
/*		          1 : 2 lanes																		*/
/*		          2 : 4 lanes																		*/
/*																									*/
/* IMPORTANT:																						*/
/*		The order for D-A-I is the reverse of what is normally used.								*/
/*		Normally, when e.g. (1-2-4) is shown, it means opcode:1 lane / addr:2 lanes / data:4 lanes	*/
/*		As the command encoding is D-A-I, this example is encoded with 0x210nn						*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

													/* (n-n-n) is #lanes (opcode-addr-inst)			*/
#define QSPI_CMD_READ				0x00003			/* Read (1-1-1)									*/
#define QSPI_CMD_READ_FAST			0x0000B			/* Fast Read (1-1-1)							*/
#define QSPI_CMD_READ_2OUT			0x1003B			/* Read dual out (1-1-2)						*/
#define QSPI_CMD_READ_4OUT			0x2006B			/* Read quad out (1-1-4)						*/
#define QSPI_CMD_READ_2IO			0x110BB			/* Read dual I/O (1-2-2)						*/
#define QSPI_CMD_READ_4IO			0x220EB			/* Read quad I/O (1-4-4)						*/

#define QSPI_CMD_READ4				0x00013			/* Read (4 byte address)						*/
#define QSPI_CMD_READ4_FAST			0x0000C			/* Fast Read (4 byte address)					*/
#define QSPI_CMD_READ4_2OUT			0x1003C			/* Read dual out (4 byte address)				*/
#define QSPI_CMD_READ4_4OUT			0x2006C			/* Read quad out (4 byte address)				*/
#define QSPI_CMD_READ4_2IO			0x110BC			/* Read dual I/O (4 byte address)				*/
#define QSPI_CMD_READ4_4IO			0x220EC			/* Read quad I/O (4 byte address)				*/

#define QSPI_CMD_PAGE_PGM			0x00002			/* Page program (1-1-1)							*/
#define QSPI_CMD_2PAGE_PGM			0x100A2			/* Dual page program (1-1-2)					*/
#define QSPI_CMD_4PAGE_PGM			0x20032			/* Quad page program (1-1-4)					*/

#define QSPI_CMD_ERASE_4K			0x00020			/* Erase 4 kByte								*/
#define QSPI_CMD_ERAGE_SECT			0x000D8			/* Erase sector: 64K or 256K					*/
#define QSPI_CMD_ERASE_CHIP			0x000C7			/* Bulk / chip erase							*/

#define QSPI_CMD_RD_ID				0x0009F			/* Read JEDEC ID								*/
#define QSPI_CMD_RD_JEDEC_PARAM		0x0005A			/* Read JEDEC discoverable parameters			*/

#define QSPI_CMD_WRT_ENB			0x00006			/* Write enable									*/
#define QSPI_CMD_WRT_DIS			0x00004			/* Write disable								*/

#define QSPI_CMD_READ_REG			0x00005			/* Status register read							*/
#define QSPI_CMD_WRT_REG			0x00001			/* Status register write						*/

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* Part definition table																			*/
/*																									*/
/* This table needs a bit of information to understand its entries and purpose						*/
/* DevID     : that the 4 bytes read using the Read ID command. It's 4 bytes that identify a part	*/
/*                                    0xSSSSPPMM													*/
/*             MM  : Manufacturer																	*/
/*             PP  : Part family																	*/
/*             SSSS: ID for the memory size & variant												*/
/* Size      : Sizes in bytes of the followings, as ordered in the array							*/
/*             [0] Page size (Set to sub-sector size if not supported)								*/
/*             [1] Sub-Sector size (Set to sector size if not supported)							*/
/*             [2] Sector size																		*/
/*             [3] Die size (Set to device size if the chip is single die)							*/
/*             [4] Device size																		*/
/* EraseCap  : Erase capability. Set to the 8 bit op-code for the matching size, set to 0 if the	*/
/*             the matching size does not have an erase capability.									*/
/*             e.g.:																				*/
/*                   Page erase not supported, set entry [0] to 0									*/
/*                   Page erase supported, set entry [0] to the 8 bit op-code						*/
/*                   Sub-sector erase not supported, set entry [1] to 0								*/
/*                   Sub-sector erase supported, set entry [1] to the 8 bit op-code					*/
/*                   ...																			*/
/* EraseTime : Erase time, matching EraseCap[]														*/
/*             Time specified in ms																	*/
/* OpRd      : Op-code to use for reading															*/
/*             Is a combination of 4 things:														*/
/*             Bits  #7-0 : 8 bit op-code															*/
/*             Bits #11-8 : # lanes for the instruction (0: 1 lane, 1: 2 lanes, 2: 4 lanes)			*/
/*             Bits #15-12: # lanes for the address (0: 1 lane, 1: 2 lanes, 2: 4 lanes)				*/
/*             Bits #19-16: # lanes for the data (0: 1 lane, 1: 2 lanes, 2: 4 lanes)				*/
/* ModeRD    : if the read operation selected requires a mode byte and its value                    */
/*             XXXXXXMM: MM is the mode byte: XXXXXXMM==0 no mode byte, XXXXXXMM!=0, is a mode byte	*/
/* OpWrt     : Op-code to use for writing															*/
/*             Is a combination of 4 things:														*/
/*             Bits  #7-0 : 8 bit op-code															*/
/*             Bits #11-8 : # lanes for the instruction (0: 1 lane, 1: 2 lanes, 2: 4 lanes)			*/
/*             Bits #15-12: # lanes for the address (0: 1 lane, 1: 2 lanes, 2: 4 lanes)				*/
/*             Bits #19-16: # lanes for the data (0: 1 lane, 1: 2 lanes, 2: 4 lanes)				*/
/* WrtMaxFrq : Max QSPI clock frequency for the write op-code specified by OpWrt					*/
/* RdDly     : Internal read logic delay (in ns)													*/
/* Delay     : The 4 delays (in ns) used by the QSPI controller										*/
/*             [0] = Delay between chip select valid and first bit exchanged						*/
/*             [1] = Delay between last data bit exchanged and the chip select invalid				*/
/*             [2] = Delay between a chip select invalid and a chips select valid					*/
/*             [3] = Delay between 2 transactions (on the same slave)								*/
/* DumClkR   : Defines the # of dummy clock for read needed according the maximum frequency			*/
/*             Entry #0 is max frequency for 0 dummy clock											*/
/*             Entry #1 is max frequency for 1 dummy clock											*/
/*             Entry #2 is max frequency for 2 dummy clocks											*/
/*             ...																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

typedef struct {									/* MM:manufact, PP:part SS:size					*/
	uint32_t	DevID;								/* Device ID: 0x00SSPPMM						*/
	uint32_t    Size[5];							/* Page, ssect, sect, die & device sizes		*/
	uint8_t     EraseCap[5];						/* Erase capabilities (if != 0, op-code)		*/
	uint32_t    EraseTime[5];						/* Erase time in ms								*/
	uint32_t    OpRd;								/* Read op-code to use							*/
	uint32_t    ModeRd;								/* If mode byte required for the read operation	*/
	uint32_t    OpWrt;								/* Write op-code to use							*/
	uint8_t     WrtMaxFrq;							/* Maximum QSPI clock frequency for write		*/
    uint8_t     RdDly;								/* Read logic delay (in ns)						*/
	int         Delay[4];							/* What to put in "delay" register				*/
	uint8_t     DumClkR[16];						/* Max Frequency for #Index Rd dummy clock		*/
} Pinfo_t;

static const Pinfo_t g_PartInfo[] = {				/* -------------------------------------------- */
			{										/* Macronix MX25L512 (512Kb / 64KB)				*/
             0x001020C2,
             { 256, 4096, 0x010000, 0x00010000, 0x00010000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 36, 360, 0, 360 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,    104,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L5121 (512Kb / 64KB)			*/
             0x001022C2,
             { 256, 4096, 0x010000, 0x00010000, 0x00010000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 36, 900, 0, 900 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,     25,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 45, 45, 45, 45, 45, 45, 45, 45 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L10XX (1Mb / 128KB)				*/
             0x001120C2,
             { 256, 4096, 0x010000, 0x00020000, 0x00020000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 36, 360, 0, 720 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,    104,
             10,
             {  20, 20, 40, 40 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L10XX (1Mb / 128KB)				*/
             0x001122C2,
             { 256, 4096, 0x010000, 0x00020000, 0x00020000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 36, 900, 0, 1350 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,     25,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 45, 45, 45, 45, 45, 45, 45, 45 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L20XX (2Mb / 256KB)				*/
             0x001220C2,
             { 256, 4096, 0x010000, 0x00040000, 0x00040000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 36, 360, 0, 1530 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,     86,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L40XX (4Mb / 512KB)				*/
             0x001320C2,
             { 256, 4096, 0x010000, 0x00080000, 0x00080000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 36, 360, 0, 1530 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,     86,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L80XX (8Mb / 1MB)				*/
             0x001420C2,
             { 256, 4096, 0x010000, 0x00100000, 0x00100000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 54, 360, 0, 3150 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,     86,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L16XX (16Mb / 2MB)				*/
             0x001520C2,
             { 256, 4096, 0x010000, 0x00200000, 0x00200000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 36, 360, 0, 5850 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,     80,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L16XX (16Mb / 2MB)				*/
             0x001524C2,
             { 256, 4096, 0x010000, 0x00200000, 0x00200000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 36, 360, 0, 4500 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,     80,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L16XX (16Mb / 2MB)				*/
             0x001525C2,
             { 256, 4096, 0x010000, 0x00200000, 0x00200000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 27, 360, 0, 5400 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,     80,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L3206 (32Mb / 4MB)				*/
             0x001620C2,
             { 256, 4096, 0x010000, 0x00400000, 0x00400000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands (has 32K erase, play safe		*/
             { 0, 27, 225, 0, 9000 },				/* and stick with 64K only for future proofing	*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,    104,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 86, 86, 86, 86, 86, 86, 86, 86 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L64XX (64Mb / 8MB)				*/
             0x001720C2,
             { 256, 4096, 0x010000, 0x00800000, 0x00800000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 30, 300, 0, 18000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,     86,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 86, 86, 86, 86, 86, 86, 86, 86 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L64XX (64Mb / 8MB)				*/
             0x001725C2,
             { 256, 4096, 0x010000, 0x00800000, 0x00800000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 27, 225, 0, 18000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,    104,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 86, 86, 86, 86, 86, 86, 86, 86 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L128XX (128Mb / 16MB)			*/
             0x001820C2,
             { 256, 4096, 0x010000, 0x01000000, 0x01000000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 27, 180, 0, 34000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,    104,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 84, 84, 84, 84, 84, 84, 84, 84 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L256XX (256Mb / 32MB)			*/
             0x001920C2,
             { 256, 4096, 0x010000, 0x02000000, 0x02000000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 15, 180, 0, 100000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,    120,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25L512XX (512Mb / 64MB)			*/
             0x001A20C2,
             { 256, 4096, 0x010000, 0x04000000, 0x04000000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 15, 180, 0, 100000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,
             QSPI_CMD_PAGE_PGM,    120,
             10,
             {  20, 20, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25R512 (512Kb / 64KB)				*/
             0x001028C2,							/* High performance set-up						*/
             { 256, 4096, 0x010000, 0x00010000, 0x00010000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 612 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             0x22038, 104,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #else
             0x22038, 33,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #endif
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
		  #if ((QSPI_MX25R_LOW_POW) == 0)			/**** Can't reach 104 MHz on Code Time test jig	*/
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 75 }
		  #else
             {  0, 0, 0, 0, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 }
		  #endif
            },										/* -------------------------------------------- */
			{										/* Macronix MX25R1035 (1Mb / 128KB)				*/
             0x001128C2,							/* High performance set-up						*/
             { 256, 4096, 0x010000, 0x00020000, 0x00020000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 612 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             0x22038, 104,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #else
             0x22038, 33,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #endif
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
		  #else
             {  0, 0, 0, 0, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 }
		  #endif
            },										/* -------------------------------------------- */
			{										/* Macronix MX25R2035 (2Mb / 256KB)				*/
             0x001228C2,							/* High performance set-up						*/
             { 256, 4096, 0x010000, 0x00040000, 0x00040000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 2430 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             0x22038, 104,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #else
             0x22038, 33,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #endif
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
		  #if ((QSPI_MX25R_LOW_POW) == 0)			/* NOTE 75 as CT set-up does not work @ 104		*/
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 75 }
		  #else
             {  0, 0, 0, 0, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 }
		  #endif
            },										/* -------------------------------------------- */
			{										/* Macronix MX25R4035 (4Mb / 512KB)				*/
             0x001328C2,							/* High performance set-up						*/
             { 256, 4096, 0x010000, 0x00080000, 0x00080000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 2430 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             0x22038, 104,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #else
             0x22038, 33,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #endif
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
		  #else
             {  0, 0, 0, 0, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 }
		  #endif
            },										/* -------------------------------------------- */
			{										/* Macronix MX25R8035 (8Mb / 1MB)				*/
             0x001428C2,							/* High performance set-up						*/
             { 256, 4096, 0x010000, 0x00100000, 0x00100000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 4860 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             0x22038, 104,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #else
             0x22038, 33,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #endif
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
		  #else
             {  0, 0, 0, 0, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 }
		  #endif
            },										/* -------------------------------------------- */
			{										/* Macronix MX25R1635 (16Mb / 2MB)				*/
             0x001528C2,							/* High performance set-up						*/
             { 256, 4096, 0x010000, 0x00080000, 0x00200000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 10800 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             0x22038, 80,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #else
             0x22038, 33,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #endif
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             {  0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80 }
		  #else
             {  0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 }
		  #endif
            },										/* -------------------------------------------- */
			{										/* Macronix MX25R3235 (32Mb / 4MB)				*/
             0x001628C2,							/* High performance set-up						*/
             { 256, 4096, 0x010000, 0x00080000, 0x00400000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 21600 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             0x22038, 80,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #else
             0x22038, 33,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #endif
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             {  0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80 }
		  #else
             {  0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 }
		  #endif
            },										/* -------------------------------------------- */
			{										/* Macronix MX25R6435 (64Mb / 8MB)				*/
             0x001728C2,							/* High performance set-up						*/
             { 256, 4096, 0x010000, 0x00080000, 0x00800000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 40000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             0x22038, 80,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #else
             0x22038, 33,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
		  #endif
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
		  #if ((QSPI_MX25R_LOW_POW) == 0)
             {  0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80 }
		  #else
             {  0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 }
		  #endif
            },										/* -------------------------------------------- */
			{										/* Macronix MX25V512 (512Kb / 64KB)				*/
             0x001023C2,
             { 256, 4096, 0x010000, 0x00010000, 0x00010000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 630 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             0x22038, 104,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25V1035 (1Mb / 128KB)				*/
             0x001123C2,
             { 256, 4096, 0x010000, 0x00020000, 0x00020000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 630 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             0x22038, 104,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25V2035 (2Mb / 256KB)				*/
             0x001223C2,
             { 256, 4096, 0x010000, 0x00040000, 0x00040000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 2520 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             0x22038, 104,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25V4035 (4Mb / 512KB)				*/
             0x001323C2,
             { 256, 4096, 0x010000, 0x00080000, 0x00080000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 2250 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             0x22038, 104,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25V8035 (8Mb / 1MB)				*/
             0x001423C2,
             { 256, 4096, 0x010000, 0x00100000, 0x00100000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 4500 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             0x22038, 104,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
			{										/* Macronix MX25V1635 (16Mb / 2MB)				*/
             0x001523C2,
             { 256, 4096, 0x010000, 0x00200000, 0x00200000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 33, 400, 0, 10800 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             0x22038, 80,							/* QSPI_CMD_4PAGE_PGM : Non standard value		*/
             10,
             {  5, 5, 30, 30 },						/* Delays										*/
             {  0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80 }
            },										/* -------------------------------------------- */
			{										/* Micron M25P05A (512Kb / 64KB)				*/
             0x00102020,							/* 40 MHz used because 50 MHz is a variant		*/
             { 256, 0x008000, 0x008000, 0x00010000, 0x00010000},
             { 0x00, 0x00, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 0, 585, 0, 765 },					/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     40,
             10,
             {  10, 10, 100, 100 },					/* Delays										*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 40, 40, 40, 40, 40, 40, 40, 40 }
            },										/* -------------------------------------------- */
			{										/* Micron M25P10A (1Mb / 128KB)					*/
             0x00112020,
             { 256, 0x008000, 0x008000, 0x00020000, 0x00020000},
             { 0x00, 0x00, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 0, 585, 0, 1530 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     50,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50 }
            },										/* -------------------------------------------- */
			{										/* Micron M25P20 (2Mb / 256KB)					*/
             0x00122020,
             { 256, 0x010000, 0x010000, 0x00040000, 0x00040000},
             { 0x00, 0x00, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 0, 900, 0, 2520 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M25P40 (4Mb / 512KB)					*/
             0x00132020,
             { 256, 0x010000, 0x010000, 0x00080000, 0x00080000},
             { 0x00, 0x00, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 0, 540, 0, 4050 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  10, 10, 100, 100 },					/* Delays, some are way higher					*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M25P80 (8Mb / 1MB)					*/
             0x00142020,
             { 256, 0x010000, 0x010000, 0x00100000, 0x00100000},
             { 0x00, 0x00, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 0, 540, 0, 7200 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M25P16 (16Mb / 2MB)					*/
             0x00152020,
             { 256, 0x010000, 0x010000, 0x00200000, 0x00200000},
             { 0x00, 0x00, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 0, 540, 0, 11700 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  10, 10, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M25P32 (32Mb / 4MB)					*/
             0x00162020,
             { 256, 0x010000, 0x010000, 0x00400000, 0x00400000},
             { 0x00, 0x00, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 0, 540, 0, 11700 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M25P64 (64Mb / 8MB)					*/
             0x00172020,
             { 256, 0x008000, 0x008000, 0x00800000, 0x00800000},
             { 0x00, 0x00, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 0, 450, 0, 31500 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M25P128 (128Mb / 16MB)				*/
             0x00182020,
             { 256, 0x040000, 0x040000, 0x01000000, 0x01000000},
             { 0x00, 0x00, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 0, 1440, 0, 50400 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     50,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50 }
            },										/* -------------------------------------------- */
			{										/* Micron M25PE10 (1Mb / 128KB)					*/
             0x00118020,
             { 256, 4096, 0x0010000, 0x00020000, 0x00020000},
             { 0xDB, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 8, 72, 1350, 0, 4050 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M25PE20 (2Mb / 256KB)					*/
             0x00128020,
             { 256, 4096, 0x0010000, 0x00040000, 0x00040000},
             { 0xDB, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 8, 72, 1350, 0, 4050 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M25PE40 (4Mb / 512KB)					*/
             0x00138020,
             { 256, 4096, 0x010000, 0x00080000, 0x00080000},
             { 0xDB, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 8, 72, 1350, 0, 4050 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  10, 10, 100, 100 },					/* Delays, some are way higher					*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M25PE80 (8Mb / 1MB)					*/
             0x00148020,
             { 256, 4096, 0x010000, 0x00100000, 0x00100000},
             { 0xDB, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 8, 45, 900, 0, 9000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M25PE16 (16Mb / 2MB)					*/
             0x00158020,
             { 256, 4096, 0x010000, 0x00200000, 0x00200000},
             { 0xDB, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 8, 45, 900, 0, 22500 },				/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Fast read : 8 dummy cycles & no mode byte	*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  10, 10, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M25PX80 (8Mb / 1MB)					*/
             0x00147120,
             { 256, 4096, 0x010000, 0x00100000, 0x00100000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 63, 540, 0, 7200 },				/* Erase time in ms								*/
             QSPI_CMD_READ_2OUT, 0x000,				/* Dual fast read: 8 dummy cycles & no mode byte*/
             QSPI_CMD_2PAGE_PGM,    75,
             10,
             {  5, 5, 80, 80 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M25PX16 (16Mb / 2MB)					*/
             0x00157120,
             { 256, 4096, 0x010000, 0x00200000, 0x00200000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 63, 540, 0, 13500 },				/* Erase time in ms								*/
             QSPI_CMD_READ_2OUT, 0x000,				/* Dual fast read: 8 dummy cycles & no mode byte*/
             QSPI_CMD_2PAGE_PGM,    75,
             10,
             {  5, 5, 80, 80 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M25PX32 (32Mb / 4MB)					*/
             0x00167120,
             { 256, 4096, 0x010000, 0x00400000, 0x00400000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 63, 630, 0, 30600 },				/* Erase time in ms								*/
             QSPI_CMD_READ_2OUT, 0x000,				/* Dual fast read: 8 dummy cycles & no mode byte*/
             QSPI_CMD_2PAGE_PGM,    75,
             10,
             {  5, 5, 80, 80 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M45PE10 (1Mb / 128KB)					*/
             0x00114020,
             { 256, 0x010000, 0x010000, 0x00020000, 0x00020000},
             { 0xDB, 0x00, 0xD8, 0x00, 0x00 },		/* Erase commands								*/
             { 9, 0, 1350, 0, 0 },					/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Dual fast read: 8 dummy cycles & no mode byte*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M45PE20 (2Mb / 256KB)					*/
             0x00124020,
             { 256, 0x010000, 0x010000, 0x00040000, 0x00040000},
             { 0xDB, 0x00, 0xD8, 0x00, 0x00 },		/* Erase commands								*/
             { 9, 0, 1350, 0, 0 },					/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Dual fast read: 8 dummy cycles & no mode byte*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M45PE40 (4Mb / 512KB)					*/
             0x00134020,
             { 256, 0x010000, 0x010000, 0x00080000, 0x00080000},
             { 0xDB, 0x00, 0xD8, 0x00, 0x00 },		/* Erase commands								*/
             { 9, 0, 1350, 0, 0 },					/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Dual fast read: 8 dummy cycles & no mode byte*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M45PE80 (8Mb / 1MB)					*/
             0x00144020,
             { 256, 0x010000, 0x010000, 0x00100000, 0x00100000},
             { 0xDB, 0x00, 0xD8, 0x00, 0x00 },		/* Erase commands								*/
             { 9, 0, 900, 0, 0 },					/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Dual fast read: 8 dummy cycles & no mode byte*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron M45PE16 (16Mb / 2MB)					*/
             0x00154020,
             { 256, 0x010000, 0x010000, 0x00200000, 0x00200000},
             { 0xDB, 0x00, 0xD8, 0x00, 0x00 },		/* Erase commands								*/
             { 9, 0, 900, 0, 0 },					/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x000,				/* Dual fast read: 8 dummy cycles & no mode byte*/
             QSPI_CMD_PAGE_PGM,     75,
             10,
             {  5, 5, 100, 100 },
             {   0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 75, 75, 75, 75, 75, 75 }
            },										/* -------------------------------------------- */
			{										/* Micron N25Q032 (32Mb / 4MB)					*/
             0x0016BA20,
             { 256, 4096, 0x010000, 0x00400000, 0x00400000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 225, 540, 0, 22500 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x000,
             QSPI_CMD_4PAGE_PGM,  108,
             10,									/* 5ns not enough, 16ns too may, pick middle	*/
             {  5, 5, 50, 50 },
             {  0, 30, 40, 50, 60, 70, 80, 86, 95, 105, 108, 108, 108, 108, 108, 108 }
            },										/* -------------------------------------------- */
			{										/* Micron N25Q064 (64Mb / 8MB) 3V				*/
             0x0017BA20,
             { 256, 4096, 0x010000, 0x00800000, 0x00800000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 225, 540, 0, 45000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x000,
             QSPI_CMD_4PAGE_PGM,  108,
             10,									/* 5ns not enough, 16ns too may, pick middle	*/
             {  5, 5, 50, 50 },
             {  0, 20, 39, 49, 59, 69, 78, 86, 95, 105, 108, 108, 108, 108, 108, 108 }
            },										/* -------------------------------------------- */
			{										/* Micron N25Q064 (64Mb / 8MB) 1.8V				*/
             0x0017BB20,
             { 256, 4096, 0x010000, 0x00800000, 0x00800000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 225, 540, 0, 45000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x000,
             QSPI_CMD_4PAGE_PGM,  108,
             10,									/* 5ns not enough, 16ns too may, pick middle	*/
             {  5, 5, 50, 50 },
             {  0, 20, 39, 49, 59, 69, 78, 86, 95, 105, 108, 108, 108, 108, 108, 108 }
            },										/* -------------------------------------------- */
			{										/* Micron N25Q128 (128Mb / 16MB) 3V				*/
             0x0018BA20,
             { 256, 4096, 0x010000, 0x01000000, 0x01000000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 54, 270, 0, 41400 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x000,
             QSPI_CMD_4PAGE_PGM,  108,
             10,									/* 5ns not enough, 16ns too may, pick middle	*/
             {  5, 5, 50, 50 },
             {  0, 30, 40, 50, 60, 70, 80, 86, 95, 105, 108, 108, 108, 108, 108, 108 }
            },										/* -------------------------------------------- */
			{										/* Micron N25Q128 (128Mb / 16MB) 1.8V			*/
             0x0018BB20,
             { 256, 4096, 0x010000, 0x01000000, 0x01000000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 54, 270, 0, 41400 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x000,
             QSPI_CMD_4PAGE_PGM,  108,
             10,									/* 5ns not enough, 16ns too may, pick middle	*/
             {  5, 5, 50, 50 },
             {  0, 30, 40, 50, 60, 70, 80, 86, 95, 105, 108, 108, 108, 108, 108, 108 }
            },										/* -------------------------------------------- */
			{										/* Micron N25Q256 (256Mb / 32MB) 3V				*/
             0x0019BA20,
             { 256, 4096, 0x010000, 0x02000000, 0x02000000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 225, 540, 0, 180000 },			/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x000,
             QSPI_CMD_4PAGE_PGM,  108,
             10,									/* 5ns not enough, 16ns too may, pick middle	*/
             {  5, 5, 50, 50 },
             {  0, 30, 40, 50, 60, 70, 80, 86, 95, 105, 108, 108, 108, 108, 108, 108 }
            },										/* -------------------------------------------- */
			{										/* Micron N25Q256 (256Mb / 32MB) 1.8V			*/
             0x0019BB20,
             { 256, 4096, 0x010000, 0x02000000, 0x02000000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 225, 540, 0, 180000 },			/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x000,
             QSPI_CMD_4PAGE_PGM,  108,
             10,									/* 5ns not enough, 16ns too may, pick middle	*/
             {  5, 5, 50, 50 },
             {  0, 30, 40, 50, 60, 70, 80, 86, 95, 105, 108, 108, 108, 108, 108, 108 }
            },										/* -------------------------------------------- */
			{										/* Micron N25Q512 (512Mb / 64MB) 3V				*/
             0x0020BA20,
             { 256, 4096, 0x010000, 0x02000000, 0x04000000},	/* Bulk erase not on variant		*/
             { 0x00, 0x20, 0xD8, 0x00, 0x00 },		/* Erase cmds - sector only see file header		*/
             { 0, 225, 540, 0, 0 },					/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x000,
             QSPI_CMD_4PAGE_PGM,  108,
             10,									/* 5ns not enough, 16ns too may, pick middle	*/
             {  5, 5, 50, 50 },
             {  0, 30, 40, 50, 60, 70, 80, 86, 95, 105, 108, 108, 108, 108, 108, 108 }
            },										/* -------------------------------------------- */
			{										/* Micron N25Q512 (512Mb / 64MB) 1.8V			*/
             0x0020BB20,
             { 256, 4096, 0x010000, 0x02000000, 0x04000000},	/* Bulk erase not on variant		*/
             { 0x00, 0x20, 0xD8, 0x00, 0x00 },		/* Erase cmds - sector only see file header		*/
             { 0, 225, 540, 0, 0 },					/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x000,
             QSPI_CMD_4PAGE_PGM,  108,
             10,									/* 5ns not enough, 16ns too may, pick middle	*/
             {  5, 5, 50, 50 },
             {  0, 30, 40, 50, 60, 70, 80, 86, 95, 105, 108, 108, 108, 108, 108, 108 }
            },										/* -------------------------------------------- */
			{							 			/* Micron N25Q00 (1Gb / 128MB) 3V				*/
             0x0021BA20,
             { 256, 4096, 0x010000, 0x02000000, 0x08000000},
             { 0x00, 0x20, 0xD8, 0x00, 0x00 },		/* Erase cmds - sector only see file header		*/
             { 0, 225, 540, 0, 0 },					/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x000,
             QSPI_CMD_4PAGE_PGM,  108,
             10,									/* 5ns not enough, 16ns too many, pick middle	*/
             {  5, 5, 50, 50 },
             {  0, 30, 40, 50, 60, 70, 80, 86, 95, 105, 108, 108, 108, 108, 108, 108 }
            },										/* -------------------------------------------- */
			{							 			/* Micron N25Q00 (1Gb / 128MB) 1.8V				*/
             0x0021BB20,
             { 256, 4096, 0x010000, 0x02000000, 0x08000000},
             { 0x00, 0x20, 0xD8, 0x00, 0x00 },		/* Erase cmds - sector only see file header		*/
             { 0, 225, 540, 0, 0 },					/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x000,
             QSPI_CMD_4PAGE_PGM,  108,
             10,									/* 5ns not enough, 16ns too many, pick middle	*/
             {  5, 5, 50, 50 },
             {  0, 30, 40, 50, 60, 70, 80, 86, 95, 105, 108, 108, 108, 108, 108, 108 }
            },										/* -------------------------------------------- */
			{							 			/* Spansion S25FL127S/128S/128P (128Mb / 16MB)	*/
             0x01182001,							/* 64K Sectors									*/
             { 256, 0x010000, 0x010000, 0x01000000, 0x01000000},	/* 512B page is an option		*/
             { 0x00, 0x00, 0xD8, 0x00, 0xC7 },		/* Erase commands (4K only on 1 sector)			*/
             { 0, 0, 475, 0, 29700 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  3, 3, 50, 50 },
             {  0, 50, 50, 50, 80, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
			{							 			/* Spansion S25FL127S/128S/128P (128Mb / 16MB)	*/
             0x00182001,							/* 256K Sectors									*/
             { 256, 0x040000, 0x040000, 0x01000000, 0x01000000},	/* 512B page is an option		*/
             { 0x00, 0x00, 0xD8, 0x00, 0xC7 },		/* Erase commands (4K only on 1 sector)			*/
             { 0, 0, 475, 0, 29700 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  3, 3, 50, 50 },
             {  0, 50, 50, 50, 80, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Spansion S25FL256S/256P (256Mb / 32MB)		*/
             0x01190201,							/* 64K Sectors									*/
             { 256, 0x010000, 0x010000, 0x02000000, 0x02000000},	/* 512B page is an option		*/
             { 0x00, 0x00, 0xD8, 0x00, 0xC7 },		/* Erase commands (4K only on 1 sector)			*/
             { 0, 0, 475, 0, 59400 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  3, 3, 50, 50 },
             {  0, 50, 50, 50, 80, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
			{							 			/* Spansion S25FL256S/256P (256Mb / 32MB)		*/
             0x00190201,							/* 256K sectors (4K only on 1 sector)			*/
             { 256, 0x040000, 0x040000, 0x02000000, 0x02000000},	/* 512B page is an option		*/
             { 0x00, 0x00, 0xD8, 0x00, 0xC7 },	 	/* Erase commands								*/
             { 0, 0, 475, 0, 59400 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  3, 3, 50, 50 },
             {  0, 50, 50, 50, 80, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Spansion S25FL512S (512Mb / 64MB)			*/
             0x00200201,
             { 256, 0x040000, 0x040000, 0x04000000, 0x04000000},
             { 0x00, 0x00, 0xD8, 0x00, 0x60 },		/* Erase commands								*/
             { 0, 0, 475, 0, 92700 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  3, 3, 50, 50 },						/**** Can't reach 104 MHz on Code Time test jig	*/
             {  0, 50, 50, 50, 80, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 90 }
			},										/* -------------------------------------------- */
 			{							 			/* Spansion S25FL116K (16Mb / 2MB)				*/
             0x00154001,
             { 256, 4096, 0x010000, 0x00200000, 0x00200000},
             { 0x00, 0x20, 0xD8, 0x00, 0x60 },		/* Erase commands								*/
             { 0, 45, 475, 0, 10080 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_PAGE_PGM,    80,
             10,
             {  5, 5, 40, 40 },
             { 0, 49, 59, 69, 78, 86, 95, 108, 108, 108, 108, 108, 108, 108, 108, 108}
            },										/* -------------------------------------------- */
 			{							 			/* Spansion S25FL132K (32Mb / 4MB)				*/
             0x00164001,
             { 256, 4096, 0x010000, 0x00400000, 0x00400000},
             { 0x00, 0x20, 0xD8, 0x00, 0x60 },		/* Erase commands								*/
             { 0, 45, 475, 0, 28800 },				/* Erase time in ms								*/
             QSPI_CMD_READ_2IO, 0x100,				/**** 4 lanes have random error on CT test jig	*/
             QSPI_CMD_PAGE_PGM,    80,				/**** Can't reach 108 MHz on Code Time test jig	*/
             10,
             {  5, 5, 40, 40 },						/**** Can't reach 108 MHz on Code Time test jig	*/
             { 0, 94, 105, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 80}
            },										/* -------------------------------------------- */
 			{							 			/* Spansion S25FL164K (64Mb / 8MB)				*/
             0x00174001,
             { 256, 4096, 0x010000, 0x00800000, 0x00800000},
             { 0x00, 0x20, 0xD8, 0x00, 0x60 },		/* Erase commands								*/
             { 0, 45, 475, 0, 57600 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_PAGE_PGM,   108,
             10,
             {  5, 5, 40, 40 },
             { 0, 49, 59, 69, 78, 86, 95, 108, 108, 108, 108, 108, 108, 108, 108, 108}
            },										/* -------------------------------------------- */
 			{							 			/* Spansion S25FL208K (8Mb / 1MB)				*/
             0x00144001,
             { 256, 4096, 0x010000, 0x00100000, 0x00100000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 45, 475, 0, 6300 },				/* Erase time in ms								*/
             QSPI_CMD_READ_2OUT, 0x00,
             QSPI_CMD_PAGE_PGM,    76,
             10,									/**** Can't reach 76 MHz on Code Time test jig	*/
             {  5, 5, 100, 100 },					/**** Although READ_FAST does reach 76 MHz		*/
             {  0, 0, 0, 0, 0, 0, 0, 0, 76, 76, 76, 76, 76, 76, 76, 60 }
            },										/* -------------------------------------------- */
 			{							 			/* SST S25FV016K (16Mb / 2MB)					*/
			 0x004125BF,
             {  1, 4096, 0x010000, 0x00200000, 0x00200000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 22, 22, 0, 50 },					/* Erase time in ms								*/
             QSPI_CMD_READ_FAST, 0x00,
             QSPI_CMD_PAGE_PGM,    80,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 0, 0, 0, 0, 80, 80, 80, 80, 80, 80, 80, 80 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25Q20 (2Mb / 256KB)					*/
             0x001240EF,
             { 256, 4096, 0x010000, 0x00040000, 0x00040000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 27, 135, 0, 450 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  5, 5, 100, 100 },
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25Q40 (4Mb / 512KB)					*/
             0x001340EF,
             { 256, 4096, 0x010000, 0x00080000, 0x00080000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 27, 135, 0, 900 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  5, 5, 50, 50 },
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25Q80 (8Mb / 1MB)					*/
             0x001440EF,
             { 256, 4096, 0x010000, 0x00100000, 0x00100000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 40, 135, 0, 1800 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  5, 5, 50, 50 },
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25Q16 (16Mb / 2MB)					*/
             0x001540EF,
             { 256, 4096, 0x010000, 0x00200000, 0x00200000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 27, 135, 0, 2700 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  5, 5, 50, 50 },						/**** Can't reach 104 MHz on Code Time test jig	*/
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 50 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25Q32 SPI (32Mb / 4MB)				*/
             0x001640EF,							/* SPI variant									*/
             { 256, 4096, 0x010000, 0x00400000, 0x00400000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 40, 135, 0, 9000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  5, 5, 50, 50 },
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25Q32 QPI (32Mb / 4MB)				*/
             0x001660EF,							/* QPI variant									*/
             { 256, 4096, 0x010000, 0x00400000, 0x00400000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 40, 135, 0, 9000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  5, 5, 50, 50 },
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25Q64 (64Mb / 8MB)					*/
             0x001740EF,							/* SPI variant									*/
             { 256, 4096, 0x010000, 0x00800000, 0x00800000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 40, 135, 0, 18000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM, 104,
             10,
             {  5, 5, 50, 50 },
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25Q64 (64Mb / 8MB)					*/
             0x001760EF,							/* QPI variant									*/
             { 256, 4096, 0x010000, 0x00800000, 0x00800000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 40, 135, 0, 18000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  5, 5, 50, 50 },
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25Q128 (128Mb / 16MB)				*/
             0x001840EF,							/* SPI variant									*/
             { 256, 4096, 0x010000, 0x01000000, 0x01000000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 40, 135, 0, 36000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  10, 10, 100, 100 },
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25Q128 (128Mb / 16MB)				*/
             0x001860EF,							/* QPI variant									*/
             { 256, 4096, 0x010000, 0x01000000, 0x01000000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 40, 135, 0, 36000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  5, 5, 50, 50 },
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25Q256 / W25Q257 (256Mb / 32MB)		*/
             0x001940EF,							/* SPI variant									*/
             { 256, 4096, 0x010000, 0x02000000, 0x02000000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 40, 135, 0, 72000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  5, 5, 50, 50 },
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25Q256 / W25Q257 (256Mb / 32MB)		*/
             0x001960EF,							/* QPI variant									*/
             { 256, 4096, 0x010000, 0x02000000, 0x02000000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 40, 135, 0, 72000 },				/* Erase time in ms								*/
             QSPI_CMD_READ_4IO, 0x100,
             QSPI_CMD_4PAGE_PGM,  104,
             10,
             {  5, 5, 50, 50 },
             {  0, 0, 0, 0, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25X05 (512Kb / 64KB)				*/
             0x001030EF,
             { 256, 4096, 0x008000, 0x00010000, 0x00010000},
             { 0x00, 0x20, 0x52, 0x00, 0xC7 },		/* Erase commands (32K sectors instead of 64K)	*/
             { 0, 27, 108, 0, 225 },				/* Erase time in ms								*/
             QSPI_CMD_READ_2IO, 0x100,
             QSPI_CMD_PAGE_PGM,   104,
             10,
             {  5, 5, 100, 100 },
             {  104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25X10 (1Mb / 128KB)					*/
             0x001130EF,
             { 256, 4096, 0x008000, 0x00020000, 0x00020000},
             { 0x00, 0x20, 0x52, 0x00, 0xC7 },		/* Erase commands  (32K sectors instead of 64K)	*/
             { 0, 27, 108, 0, 225 },				/* Erase time in ms								*/
             QSPI_CMD_READ_2IO, 0x100,
             QSPI_CMD_PAGE_PGM,   104,
             10,
             {  5, 5, 100, 100 },
             {  104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25X20 (2Mb / 256KB)					*/
             0x001230EF,
             { 256, 4096, 0x010000, 0x00040000, 0x00040000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 27, 135, 0, 450 },				/* Erase time in ms								*/
             QSPI_CMD_READ_2IO, 0x100,
             QSPI_CMD_PAGE_PGM,   104,
             10,
             {  5, 5, 100, 100 },
             {  104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            },										/* -------------------------------------------- */
 			{							 			/* Winbond W25X40 (4Mb / 512KB)					*/
             0x001330EF,
             { 256, 4096, 0x010000, 0x00080000, 0x00080000},
             { 0x00, 0x20, 0xD8, 0x00, 0xC7 },		/* Erase commands								*/
             { 0, 27, 135, 0, 900 },				/* Erase time in ms								*/
             QSPI_CMD_READ_2IO, 0x100,
             QSPI_CMD_PAGE_PGM,   104,
             7,
             {  5, 5, 100, 100 },
             {  104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104 }
            }
                              };

#define PART_PAGE_SZ	0							/* Index for the part page size					*/
#define PART_SSECT_SZ	1							/* Index for the part sub-sector size			*/
#define PART_SECT_SZ	2							/* Index for the part sector size				*/
#define PART_DIE_SZ		3							/* Index for the part die size					*/
#define PART_DEV_SZ		4							/* Index for the part device size				*/

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* Pre-compute the FIFO thresholds																	*/

#define QSPI_RX_SRAM_PART	 	(((QSPI_RX_FIFO_SIZE)+3)/4)
#define QSPI_TX_SRAM_PART		((((QSPI_SRAM_SIZE)-(QSPI_TX_FIFO_SIZE))+3)/4)

#if (((((QSPI_RX_FIFO_SIZE)-4) * QSPI_ISR_RX_THRS)/100) < 1)
  #define QSPI_RX_WATERMARK		1					/* 1 is the minimum that can be used			*/
#else
  #define QSPI_RX_WATERMARK		((((QSPI_RX_FIFO_SIZE)-4) * QSPI_ISR_RX_THRS)/100)
#endif

#if (((((QSPI_TX_FIFO_SIZE)-4) * QSPI_ISR_TX_THRS)/100) < 1)
  #define QSPI_TX_WATERMARK		1					/* 1 is the minimum that can be used			*/
#else
  #define QSPI_TX_WATERMARK		((((QSPI_TX_FIFO_SIZE)-4) * QSPI_ISR_TX_THRS)/100)
#endif

/* ------------------------------------------------------------------------------------------------ */
/* This section uses a lots of macros but it's to reduce the size of the data requirements by only	*/
/* creating descriptor / mutexes etc for the devices in use ("1" in QSPI_LIST_DEVICE)				*/

#ifndef QSPI_MAX_DEVICES
 #if   (((QSPI_LIST_DEVICE) & 0x200) != 0U)
  #define QSPI_MAX_DEVICES	10
 #elif (((QSPI_LIST_DEVICE) & 0x100) != 0U)
  #define QSPI_MAX_DEVICES	9
 #elif (((QSPI_LIST_DEVICE) & 0x080) != 0U)
  #define QSPI_MAX_DEVICES	8
 #elif (((QSPI_LIST_DEVICE) & 0x040) != 0U)
  #define QSPI_MAX_DEVICES	7
 #elif (((QSPI_LIST_DEVICE) & 0x020) != 0U)
  #define QSPI_MAX_DEVICES	6
 #elif (((QSPI_LIST_DEVICE) & 0x010) != 0U)
  #define QSPI_MAX_DEVICES	5
 #elif (((QSPI_LIST_DEVICE) & 0x008) != 0U)
  #define QSPI_MAX_DEVICES	4
 #elif (((QSPI_LIST_DEVICE) & 0x004) != 0U)
  #define QSPI_MAX_DEVICES	3
 #elif (((QSPI_LIST_DEVICE) & 0x002) != 0U)
  #define QSPI_MAX_DEVICES	2
 #elif (((QSPI_LIST_DEVICE) & 0x001) != 0U)
  #define QSPI_MAX_DEVICES	1
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* This section figures out how to remap the individual devices specified by QSPI_LIST_DEVICE		*/
/* e.g. if QSPI_LIST_DEVICE == 0x12, the only descriptors/mutexes for 2 devices are required and	*/
/*      device#1 is remapped to [0] and device #4 is remapped to [1]								*/

#if (((QSPI_LIST_DEVICE) & 0x001) != 0)
  #define QSPI_CNT_0	0
  #define QSPI_IDX_0	0
#else
  #define QSPI_CNT_0	(-1)
  #define QSPI_IDX_0	(-1)
#endif
#if (((QSPI_LIST_DEVICE) & 0x002) != 0)
  #define QSPI_CNT_1	((QSPI_CNT_0) + 1)
  #define QSPI_IDX_1	((QSPI_CNT_0) + 1)
#else
  #define QSPI_CNT_1	(QSPI_CNT_0)
  #define QSPI_IDX_1	-1
#endif
#if (((QSPI_LIST_DEVICE) & 0x004) != 0)
  #define QSPI_CNT_2	((QSPI_CNT_1) + 1)
  #define QSPI_IDX_2	((QSPI_CNT_1) + 1)
#else
  #define QSPI_CNT_2	(QSPI_CNT_1)
  #define QSPI_IDX_2	-1
#endif
#if (((QSPI_LIST_DEVICE) & 0x008) != 0)
  #define QSPI_CNT_3	((QSPI_CNT_2) + 1)
  #define QSPI_IDX_3	((QSPI_CNT_2) + 1)
#else
  #define QSPI_CNT_3	(QSPI_CNT_2)
  #define QSPI_IDX_3	-1
#endif
#if (((QSPI_LIST_DEVICE) & 0x010) != 0)
  #define QSPI_CNT_4	((QSPI_CNT_3) + 1)
  #define QSPI_IDX_4	((QSPI_CNT_3) + 1)
#else
  #define QSPI_CNT_4	(QSPI_CNT_3)
  #define QSPI_IDX_4	-1
#endif
#if (((QSPI_LIST_DEVICE) & 0x020) != 0)
  #define QSPI_CNT_5	((QSPI_CNT_4) + 1)
  #define QSPI_IDX_5	((QSPI_CNT_4) + 1)
#else
  #define QSPI_CNT_5	(QSPI_CNT_4)
  #define QSPI_IDX_5	-1
#endif
#if (((QSPI_LIST_DEVICE) & 0x040) != 0)
  #define QSPI_CNT_6	((QSPI_CNT_5) + 1)
  #define QSPI_IDX_6	((QSPI_CNT_5) + 1)
#else
  #define QSPI_CNT_6	(QSPI_CNT_5)
  #define QSPI_IDX_6	-1
#endif
#if (((QSPI_LIST_DEVICE) & 0x080) != 0)
  #define QSPI_CNT_7	((QSPI_CNT_6) + 1)
  #define QSPI_IDX_7	((QSPI_CNT_6) + 1)
#else
  #define QSPI_CNT_7	(QSPI_CNT_6)
  #define QSPI_IDX_7	-1
#endif
#if (((QSPI_LIST_DEVICE) & 0x100) != 0)
  #define QSPI_CNT_8	((QSPI_CNT_7) + 1)
  #define QSPI_IDX_8	((QSPI_CNT_7) + 1)
#else
  #define QSPI_CNT_8	(QSPI_CNT_7)
  #define QSPI_IDX_8	-1
#endif
#if (((QSPI_LIST_DEVICE) & 0x200) != 0)
  #define QSPI_CNT_9	((QSPI_CNT_8) + 1)
  #define QSPI_IDX_9	((QSPI_CNT_8) + 1)
#else
  #define QSPI_CNT_9	(QSPI_CNT_8)
  #define QSPI_IDX_9	-1
#endif

#define QSPI_NMB_DEVICES	((QSPI_CNT_9)+1)

/* ------------------------------------------------------------------------------------------------ */
/* Remaping table:																					*/

static const int g_DevReMap[] = { QSPI_IDX_0
                               #if ((QSPI_MAX_DEVICES) > 1)
                                 ,QSPI_IDX_1
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 2)
                                 ,QSPI_IDX_2
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 3)
                                 ,QSPI_IDX_3
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 4)
                                 ,QSPI_IDX_4
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 5)
                                 ,QSPI_IDX_5
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 6)
                                 ,QSPI_IDX_6
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 7)
                                 ,QSPI_IDX_7
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 8)
                                 ,QSPI_IDX_8
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 9)
                                 ,QSPI_IDX_9
                               #endif
                               };

/* ------------------------------------------------------------------------------------------------ */

#if (((QSPI_USE_MUTEX) != 0) || ((QSPI_OPERATION) & 0x00202))

  static const char g_Names[][7] = {
                                 "QSPI-0"
                               #if ((QSPI_MAX_DEVICES) > 1)
                                ,"QSPI-1"
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 2)
                                ,"QSPI-2"
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 3)
                                ,"QSPI-3"
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 4)
                                ,"QSPI-4"
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 5)
                                ,"QSPI-5"
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 6)
                                ,"QSPI-6"
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 7)
                                ,"QSPI-7"
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 8)
                                ,"QSPI-8"
                               #endif 
                               #if ((QSPI_MAX_DEVICES) > 9)
                                ,"QSPI-9"
                               #endif
                               };
#endif

static const int g_RdDly[1][4] = {					/* Array to overload RdDLy from the part table	*/
                                  {  QSPI_RDDLY_0_0	/* The validity of the 2 dimensions is checked	*/
                                    ,QSPI_RDDLY_0_1	/* a bit below									*/
                                    ,QSPI_RDDLY_0_2
                                    ,QSPI_RDDLY_0_3
                                  }
                                 };

/* ------------------------------------------------------------------------------------------------ */
/* Checks																							*/

#if ((QSPI_MIN_4_RX_DMA) < 4)						/* 4 is needed due to 4 byte wide RX FIFO		*/
  #error "QSPI_MIN_4_RX_DMA must be >= 4"
#endif

//#if ((QSPI_MIN_4_TX_DMA) < 0)
//  #error "QSPI_MIN_4_TX_DMA must be >= 1"
//#endif

#if ((QSPI_MIN_4_RX_ISR) < 4)						/* 4 is needed due to 4 byte wide RX FIFO		*/
  #error "QSPI_MIN_4_RX_ISR must be >= 4"
#endif

#if ((QSPI_MIN_4_TX_ISR) < 0)
  #error "QSPI_MIN_4_TX_ISR must be >= 1"
#endif

#if (((QSPI_OPERATION) & ~0x10707) != 0)
  #error "Invalid value assigned to QSPI_OPERATION"
#endif

#if (((QSPI_OPERATION) & 0x10000) != 0)
 #if ((OX_TIMER_US) <= 0)
  #error "The QSPI driver is set-up to rely on timeouts."
  #error "The RTOS timer must be enabled by setting OS_TIMER_US to > 0"
 #endif
 #if ((OX_TIMEOUT) <= 0)
  #error "The QSPI driver is set-up to rely on timeouts. OS_TIMEOUT must be set to > 0"
 #endif
#endif

#if (((QSPI_ISR_RX_THRS) < 0) || ((QSPI_ISR_RX_THRS) > 100))
  #error "QSPI_ISR_RX_THRS must be set between 0 and 100"
#endif

#if (((QSPI_ISR_TX_THRS) < 0) || ((QSPI_ISR_TX_THRS) > 100))
  #error "QSPI_ISR_TX_THRS must be set between 0 and 100"
#endif

#if (((QSPI_MAX_DEVICES) > 1) || ((QSPI_MAX_SLAVES) > 4))
  #error "QSPI_RDDLY_X_X defintions must be updated"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Some definitions																					*/

#if ((QSPI_USE_MUTEX) != 0)							/* Mutex for exclusive access					*/
  #define QSPI_MTX_LOCK(Cfg)		MTXlock(Cfg->MyMutex, -1)
  #define QSPI_MTX_UNLOCK(Cfg)		MTXunlock(Cfg->MyMutex)
#else
  #define QSPI_MTX_LOCK(Cfg)	  	0
  #define QSPI_MTX_UNLOCK(Cfg)		OX_DO_NOTHING()
#endif
													/* DO NOT MODIFY !!!  DO NOT MODIFY !!!  		*/
#define QSPI_4_BYTE_ADDRESS			1				/* Internally used to keep the code for all		*/
													/* controllers as identical as possible			*/

#if ((((QSPI_OPERATION) & 0x00001) == 0) || ((QSPI_TOUT_ISR_ENB) == 0))
  #define QSPI_EXPIRY_RX(_IsExp, _IsrState, _Expiry)	do {										\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
														} while(0)
#else
  #define QSPI_EXPIRY_RX(_IsExp, _IsrState, _Expiry)	do {										\
															OSintBack(_IsrState);					\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
															_IsrState = OSintOff();					\
														} while(0)
#endif
#if ((((QSPI_OPERATION) & 0x00100) == 0) || ((QSPI_TOUT_ISR_ENB) == 0))
  #define QSPI_EXPIRY_TX(_IsExp, _IsrState, _Expiry)	do {										\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
														} while(0)
#else
  #define QSPI_EXPIRY_TX(_IsExp, _IsrState, _Expiry)	do {										\
															OSintBack(_IsrState);					\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
															_IsrState = OSintOff();					\
														} while(0)
#endif
#if ((((QSPI_OPERATION) & 0x00101) == 0) || ((QSPI_TOUT_ISR_ENB) == 0))
  #define QSPI_EXPIRY_RXTX(_IsExp, _IsrState, _Expiry)	do {										\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
														} while(0)
#else
  #define QSPI_EXPIRY_RXTX(_IsExp, _IsrState, _Expiry)	do {										\
															OSintBack(_IsrState);					\
															_IsExp = OS_HAS_TIMEDOUT(_Expiry);		\
															_IsrState = OSintOff();					\
														} while(0)
#endif

/* ------------------------------------------------------------------------------------------------ */
/* QSPI register index definitions (/4 as are accessed as uint32_t)									*/

#define QSPI_CFG_REG				(0x00/4)
#define QSPI_DEVRD_REG				(0x04/4)
#define QSPI_DEVWR_REG				(0x08/4)
#define QSPI_DELAY_REG				(0x0C/4)
#define QSPI_RDDATACAP_REG			(0x10/4)
#define QSPI_DEVSZ_REG				(0x14/4)
#define QSPI_SRAMPART_REG			(0x18/4)
#define QSPI_INDADDRTRIG_REG		(0x1C/4)
#define QSPI_DMAPER_REG				(0x20/4)
#define QSPI_REMAPADDR_REG			(0x24/4)
#define QSPI_MODEBIT_REG			(0x28/4)
#define QSPI_SRAMFILL_REG			(0x2C/4)
#define QSPI_TXTHRESH_REG			(0x30/4)
#define QSPI_RXTHRESH_REG			(0x34/4)
#define QSPI_IRQSTAT_REG			(0x40/4)
#define QSPI_IRQMASK_REG			(0x44/4)
#define QSPI_LOWWRPROT_REG			(0x50/4)
#define QSPI_UPPWRTPROT_REG			(0x54/4)
#define QSPI_WRPROT_REG				(0x58/4)
#define QSPI_INDRD_REG				(0x60/4)
#define QSPI_INDRDWATER_REG			(0x64/4)
#define QSPI_INDRDSTADDR_REG		(0x68/4)
#define QSPI_INDRDCNT_REG			(0x6C/4)
#define QSPI_INDWR_REG				(0x70/4)
#define QSPI_INDWRWATER_REG			(0x74/4)
#define QSPI_INDWRSTADDR_REG		(0x78/4)
#define QSPI_INDWRCNT_REG			(0x7C/4)
#define QSPI_FLASHCMD_REG			(0x90/4)
#define QSPI_FLASHCMDADDR_REG		(0x94/4)
#define QSPI_FLASHCMDRDDATALO_REG	(0xA0/4)
#define QSPI_FLASHCMDRDDATAUP_REG	(0xA4/4)
#define QSPI_FLASHCMDWRDATALO_REG	(0xA8/4)
#define QSPI_FLASHCMDWRDATAUP_REG	(0xAC/4)

/* ------------------------------------------------------------------------------------------------ */
/* Local variables																					*/

typedef struct {									/* Per device: configuration					*/
	volatile uint32_t *HW;							/* Base address of the QSPI peripheral			*/
	int IsInit;										/* If this device has been initialized			*/
	int SlvInit;									/* Slave that have been initialized (bit field)	*/
	const Pinfo_t *Part[QSPI_MAX_SLAVES];			/* Part information table						*/
	uint32_t CfgRd[QSPI_MAX_SLAVES];				/* Configuration register when reading data		*/
	uint32_t CfgWr[QSPI_MAX_SLAVES];				/* Configuration register when writing data		*/
	uint32_t RdCap[QSPI_MAX_SLAVES];				/* Read capturing logic							*/
	uint32_t DevRd[QSPI_MAX_SLAVES];				/* devrd register to set (per slave)			*/
	uint32_t DevWr[QSPI_MAX_SLAVES];				/* devwr register to set (per slave)			*/
	uint32_t Delay[QSPI_MAX_SLAVES];				/* delay register to set (per slave)			*/
	uint32_t DevSz[QSPI_MAX_SLAVES];				/* devsz register to set (per slave)			*/
	uint32_t Mode[QSPI_MAX_SLAVES];					/* Configuration mode supplied in qspi_init()	*/
	uint32_t NsecByteRd[QSPI_MAX_SLAVES];			/* Time in ns to transfer 1 byte (reading)		*/
	uint32_t NsecByteWr[QSPI_MAX_SLAVES];			/* Time in ns to transfer 1 byte (writing)		*/
  #if ((QSPI_USE_MUTEX) != 0)						/* If mutex protection enable					*/
	MTX_t            *MyMutex;						/* One mutex per controller						*/
  #endif
  #if (((QSPI_OPERATION) & 0x00202) != 0)			/* If interrupts are used (Read and/or write)	*/
	SEM_t *MySema;									/* Semaphore posted in the ISR					*/
	volatile int      ISRisRD;						/* If is a read or write transfer				*/
   #if (((QSPI_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1))
	volatile int      ISRonGoing;					/* Inform other core if one handling the ISR	*/
	int               SpinLock;						/* Spinlock to provide exclusive ISR access		*/
   #endif
  #endif
  #if (((QSPI_OPERATION) & 0x00002) != 0)			/* When reading in interrupts					*/
	volatile uint8_t *ISRbuf;						/* Current buffer address to fill				*/
	volatile uint32_t ISRleftRX;					/* # byte left in the transfer					*/
  #endif
} QSPIcfg_t;

static QSPIcfg_t g_QSPIcfg[QSPI_NMB_DEVICES];		/* Device configuration							*/
static int       g_CfgIsInit = 0;					/* To track first time an init occurs			*/

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: CmdRead																				*/
/*																									*/
/* CmdRead - read from the flash																	*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void CmdRead(QSPIcfg_t *Cfg, int Cmd, uint8_t *Buf, int Nbytes);							*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Cfg    : QSPI configuration descriptor														*/
/*		Cmd    : 8 bit command to send to the flash													*/
/*		Buf    : buffer to deposit the data read into												*/
/*		Nbytes : number of bytes to read															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		*** Internal usage ***																		*/
/*		For the exported function, see qspi_cmd_read()												*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static void CmdRead(QSPIcfg_t *Cfg, int Cmd, uint8_t *Buf, int Nbytes)
{
uint32_t Data;										/* Holds the data received from the flash		*/
int      ii;										/* Loop counter									*/
int      jj;
volatile uint32_t *Reg;								/* Base address of the QSPI controller registers*/
#if ((QSPI_CMD_LOWEST_FRQ) != 0)
  uint32_t CfgBack;									/* Holds original CFG register					*/
#endif

	Reg  = Cfg->HW;
	Data = ((Cmd) << 24);							/* Put the command op-code in the register		*/
	if (Nbytes > 0) {
		Data |= (1 << 23)							/* Inform controller there are data to read		*/
		     |  ((Nbytes-1) << 20);					/* Inform ctrl on the number of bytes to read	*/
	}

  #if ((QSPI_CMD_LOWEST_FRQ) != 0)					/* If changing the bus clock for read command	*/
	CfgBack                 = Reg[QSPI_CFG_REG];
   #if ((QSPI_CMD_LOWEST_FRQ) > 0)					/* If disabling ./ enabkling when changing		*/
	Reg[QSPI_CFG_REG]      &= ~1;					/* Disable the controller						*/
	Reg[QSPI_CFG_REG]      = (0xF << 19)			/* Use slowest clock speed						*/
	                       | (CfgBack & ~1);
	Reg[QSPI_CFG_REG]      |=  1;					/* Re-enable the controller						*/
   #else
	Reg[QSPI_CFG_REG]      |= (0xF << 19);			/* Use slowest clock speed						*/
   #endif
  #endif

	Reg[QSPI_FLASHCMD_REG]  = Data;

	Reg[QSPI_FLASHCMD_REG] |= 1;					/* Execute the command							*/

	ii = 4*1024*1024;								/* Timeout: use counter							*/
	while((0 != (Reg[QSPI_FLASHCMD_REG] & (1U<< 1)))	/* Wait for command completion				*/
	&&    (0 == (Reg[QSPI_CFG_REG]      & (1U<<31)))	/* Wait for controller to be Idle			*/
	&&    (--ii > 0));

	jj = (Nbytes <= 4)								/* Copy the data received in Buf[]				*/
	   ? Nbytes
	   : 4;
	Data = Reg[QSPI_FLASHCMDRDDATALO_REG];			
	for (ii=0 ; ii<jj ; ii++) {
		Buf[ii] = 0xFF
		        & (Data >> (ii*8));
	}

	Data = Reg[QSPI_FLASHCMDRDDATAUP_REG];
	for (ii=4 ; ii<Nbytes ; ii++) {
		Buf[ii] = 0xFF
		        & (Data >> ((ii-4)*8));
	}

  #if ((QSPI_CMD_LOWEST_FRQ) != 0)					/* If changing the bus clock for read command	*/
   #if ((QSPI_CMD_LOWEST_FRQ) > 0)					/* If disabling ./ enabkling when changing		*/
	Reg[QSPI_CFG_REG]  = CfgBack & ~1;				/* Go back to original bus clock & enable back	*/
	Reg[QSPI_CFG_REG] |= 1;
   #else
	Reg[QSPI_CFG_REG]  = CfgBack;					/* Go back to original bus clock & enable back	*/
   #endif
  #endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: CmdWrt																					*/
/*																									*/
/* CmdWrt - write to the flash																		*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void CmdWrt(QSPIcfg_t *Cfg, int Cmd, uint8_t *Buf, int Nbytes);								*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Cfg    : QSPI configuration descriptor														*/
/*		Cmd    : 8 bit command to send to the flash													*/
/*		Buf    : buffer holding the data to write to the flash										*/
/*		Nbytes : number of bytes to write															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		*** Internal usage ***																		*/
/*		For the exported function, see qspi_cmd_write()												*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static void CmdWrt(QSPIcfg_t *Cfg, int Cmd, uint8_t *Buf, int Nbytes)
{
uint32_t Data;
int      ii;										/* Loop counter									*/
int      jj;
volatile uint32_t *Reg;								/* Base address of the QSPI controller registers*/
#if ((QSPI_CMD_LOWEST_FRQ) != 0)					/* If changing the bus clock for read command	*/
  uint32_t CfgBack;									/* Holds original CFG register					*/
#endif

	Reg  = Cfg->HW;
	Data = ((Cmd) << 24);							/* Put the command op-code in the register		*/
	if (Nbytes > 0) {
		Data |= (1U << 15)							/* Inform controller there are data to write	*/
		     |  ((Nbytes-1) << 12);					/* Inform ctrl on the number of bytes to write	*/
	}

  #if ((QSPI_CMD_LOWEST_FRQ) != 0)					/* If changing the bus clock for read command	*/
	CfgBack                 = Reg[QSPI_CFG_REG];
   #if ((QSPI_CMD_LOWEST_FRQ) > 0)					/* If disabling ./ enabkling when changing		*/
	Reg[QSPI_CFG_REG]      &= ~1;					/* Disable the controller						*/
	Reg[QSPI_CFG_REG]      = (0xF << 19)			/* Use slowest clock speed						*/
	                       | (CfgBack & ~1);
	Reg[QSPI_CFG_REG]      |=  1;					/* Re-enable the controller						*/
   #else
	Reg[QSPI_CFG_REG]      |= (0xF << 19);			/* Use slowest clock speed						*/
   #endif
  #endif

	Reg[QSPI_FLASHCMD_REG]  = Data;

	jj = (Nbytes <= 4)								/* Copy the data received in Buf[]				*/
	   ? Nbytes
	   : 4;

	Data = 0;
	for (ii=0 ; ii<jj ; ii++) {
		Data |= (0xFF & (uint32_t)Buf[ii])
		     << (ii*8);
	}
	Reg[QSPI_FLASHCMDWRDATALO_REG] = Data;

	Data = 0;
	for (ii=4 ; ii<Nbytes ; ii++) {
		Data |= (0xFF & (uint32_t)Buf[ii])
		     << ((ii-4)*8);
	}
	Reg[QSPI_FLASHCMDWRDATAUP_REG] = Data;

	Reg[QSPI_FLASHCMD_REG] |= 1;					/* Execute the command							*/

	ii = 4*1024*1024;								/* Timeout: use counter							*/
	while((0 != (Reg[QSPI_FLASHCMD_REG] & (1U<< 1)))	/* Wait for command completion				*/
	&&    (0 == (Reg[QSPI_CFG_REG]      & (1U<<31)))	/* Wait for controller to be Idle			*/
	&&    (--ii > 0));

  #if ((QSPI_CMD_LOWEST_FRQ) != 0)					/* If changing the bus clock for read command	*/
   #if ((QSPI_CMD_LOWEST_FRQ) > 0)					/* If disabling ./ enabkling when changing		*/
	Reg[QSPI_CFG_REG]  = CfgBack & ~1;				/* Go back to original bus clock & enable back	*/
	Reg[QSPI_CFG_REG] |= 1;
   #else
	Reg[QSPI_CFG_REG]  = CfgBack;					/* Go back to original bus clock & enable back	*/
   #endif
  #endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: WaitRdy																				*/
/*																									*/
/* WaitRdy - Report if, or wait for the part to be ready for a new command							*/
/*																									*/
/* SYNOPSIS																							*/
/*		int WaitRdy(QSPIcfg_t *Cfg, int Slv, int TimeOut);											*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Cfg  : QSPI configuration descriptor														*/
/*		Slv  : QSPI part index (Slave index)														*/
/*		Stay : If staying in here until ready														*/
/*		       == 0 : check if ready and report														*/
/*		       != 0 : Max time to wait (infinity when -1)											*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : is ready																				*/
/*		!= 0 : is not ready																			*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		*** Internal usage ***																		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int WaitRdy(QSPIcfg_t *Cfg, int Slv, int TimeOut)
{
int      Expiry;
int      IsExp;
uint32_t Part2B;									/* 2 lower bytes of part ID						*/
uint8_t  RegBuf[8];									/* Buffer to read & write to flash registers	*/
int      Ret;										/* Return value									*/

	Expiry = OS_TICK_EXP(TimeOut);
	IsExp  = (TimeOut == 0)							/* TimeOut == 0 is a request to not wait		*/
	       ? -1										/* Set to -1 to not iterate in the loop			*/
	       : 0;

	do {
		if (TimeOut > 0) {
			IsExp = OS_HAS_TIMEDOUT(Expiry);
		}
		CmdRead(Cfg, QSPI_CMD_READ_REG, &RegBuf[0], 1);
		Ret = RegBuf[0] & 0x01;						/* Bit #0 (busy) non-zero means not ready		*/
	} while ((Ret != 0)								/* Done if ready or not requested to stay until	*/
	  &&     (IsExp >= 0));							/* ready										*/
													/* +++++ PART ADD START +++++++++++++++++++++++ */
	if (Ret == 0) {									/* Extra checks for some parts					*/
		Part2B = Cfg->Part[Slv]->DevID
		       & 0xFFFF;
		if ((Part2B == 0xBA20)						/* MICRON N25Q series							*/
		||  (Part2B == 0xBB20)) {
			do {
				if (TimeOut > 0) {
					IsExp = OS_HAS_TIMEDOUT(Expiry);
				}
				CmdRead(Cfg, 0x70, &RegBuf[0], 1);	/* Read flag stat register until write is done	*/
				Ret = !(RegBuf[0] & 0x80);			/* When bit #7 is zero it means not-ready		*/
			} while ((Ret != 0)						/* Done if ready or not requested to stay until	*/
			  &&     (IsExp >= 0));					/* ready										*/
		}
	}												/* +++++ PART ADD END +++++++++++++++++++++++++ */

	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: SetHiAddr																				*/
/*																									*/
/* SetHoAddr - Set the upper 24 bits of the address when applicable									*/
/*																									*/
/* SYNOPSIS																							*/
/*		void SetHiAddr(QSPIcfg_t *Cfg, int Slv , uint32_t Addr);									*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Cfg  : QSPI configuration descriptor														*/
/*		Slv  : QSPI part index (Slave index)														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		*** Internal usage ***																		*/
/*																									*/
/*		Setting the upper address bits (31:24) is manufacturer / part specific.						*/
/*		Do nothing when the part is <= 16 MByte in size or if the part handles 32 bit addresses		*/
/*		instead of only using an extended address register.											*/
/*		Is kept here as a place holder for future if a part is larger than 16 MByte and can't use	*/
/*		32 bit addresses directly.																	*/
/* ------------------------------------------------------------------------------------------------ */
static void SetHiAddr(QSPIcfg_t *Cfg, int Slv , uint32_t Addr)
{
													/* +++++ PART ADD START +++++++++++++++++++++++	*/
#if ((QSPI_4_BYTE_ADDRESS) == 0)					/* Driver does not handle 4 byte address		*/
 uint32_t DevID;									/* Part JEDEC device ID							*/
 uint32_t Part2B;									/* 2 lower bytes of the Part ID					*/
 uint8_t  RegBuf[8];								/* Buffer to read & write to flash registers	*/

													/* Nothing to do if part size is <= 16 MBytes	*/
	if (Cfg->Part[Slv]->Size[PART_DEV_SZ] > 0x01000000) {
		Addr   = Addr >> 24;						/* Bring upper byte in lower byte				*/
		DevID  = Cfg->Part[Slv]->DevID;
		Part2B = DevID
		       & 0x0000FFFF;
													/* -------------------------------------------- */
		if ((Part2B == 0x20C2)						/* Macronix MX25L / MX25V						*/
		||  (Part2B == 0x22C2)
		||  (Part2B == 0x24C2)
		||  (Part2B == 0x25C2)) {
			RegBuf[0] = Addr;
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			CmdWrt(Cfg, 0xC5, &RegBuf[0], 1);		/* Write extended address register				*/
		}
													/* -------------------------------------------- */
		else if ((Part2B == 0x0201)					/* Spansion	S25FL-S series						*/
		     ||  (Part2B == 0x2001)) {
			CmdRead(Cfg, 0x16, &RegBuf[0], 1);		/* Bank address register read command			*/
			RegBuf[0] = (RegBuf[0] & 0x80)			/* Keep EXTADD bit untouched					*/
			          | Addr;
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			CmdWrt(Cfg, 0x17, &RegBuf[0], 1);		/* Bank address register write command			*/
		}
													/* -------------------------------------------- */
		else if ((Part2B == 0xBA20)					/* Micron N25Q series							*/
		     ||  (Part2B == 0xBB20)) {
			RegBuf[0] = Addr;
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			CmdWrt(Cfg, 0xC5, &RegBuf[0], 1);		/* Write extended address register				*/
		}
													/* -------------------------------------------- */
		else if ((Part2B == 0x40EF)					/* Winbond W25Q series							*/
		     ||  (Part2B == 0x60EF)) {
			RegBuf[0] = Addr;
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			CmdWrt(Cfg, 0xC5, &RegBuf[0], 1);		/* Write extended address register				*/
		}
	}
#endif
													/* +++++ PART ADD END +++++++++++++++++++++++++	*/
	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: qspi_read																				*/
/*																									*/
/* qspi_read - read data from the flash																*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int qspi_read(int Dev, int Slv, uint32_t Addr, void *Buf, uint32_t Len);					*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : QSPI controller number																*/
/*		Slv  : QSPI part index (Slave index)														*/
/*		Addr : Base address where to start reading from the flash									*/
/*		Buf  : pointer to the buffer where the data read will be deposited							*/
/*		Len  : number of bytes to read from the flash												*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		There are no restrictions on starting address, length; a single byte can be read.			*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int qspi_read(int Dev, int Slv, uint32_t Addr, void *Buf, uint32_t Len)
{
uint8_t           *Buf8;							/* Output buffer accessed one byte at a time	*/
uint32_t          *Buf32;							/* Output buffer accessed 4 bytes at a time		*/
uint32_t           Buf32Tmp[1];						/* Interim when buffer not 4 byte aligned		*/
QSPIcfg_t         *Cfg;								/* De-reference array for faster access			*/
int                Expiry;
int                ii;								/* General purpose								*/
int                IsExp;							/* If timeout expired							*/
int                IsStarted;
int                jj;								/* General purpose								*/
int                LeftRX;							/* How many bytes left to read					*/
const Pinfo_t     *Part;							/* Part information								*/
int                ReMap;							/* Device # remapped							*/
volatile uint32_t *Reg;								/* Base pointer to QSPI controller registers	*/
uint32_t           SzMask;							/* Mask used to handle max read size (die size)	*/
int                TimeOut;							/* Timer tick value to declare a timeout error	*/
uint32_t           XferLen;							/* Transfer length in bytes						*/
uint32_t           XferMax;							/* Maximum size for a write burst				*/
#if (((QSPI_OPERATION) & 0x00001) != 0)
  int              IsrState;						/* When disabling ISRs, original ISR state		*/
#endif
#if (((QSPI_OPERATION) & 0x00002) != 0)
  unsigned int     UseISR;							/* Is ISRs are used for the transfer			*/
#endif
#if (((QSPI_OPERATION) & 0x00004) != 0)
 int               DMAid;							/* ID of the DMA transfer						*/
 uint32_t          CfgOp[8];						/* DMA configuration commands					*/
#endif
#if ((QSPI_DEBUG) > 1)
 unsigned int XferType;
#endif

  #if ((QSPI_DEBUG) > 1)
	printf("QSPI  [Dev:%d - Slv:%d] - Reading 0x%08X bytes from 0x%08X\n", Dev, Slv, (unsigned int)Len,
	       (unsigned)Addr);
	XferType = 0U;
  #endif

  #if ((QSPI_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_QSPIcfg[ReMap];						/* This is this device configuration			*/
	Reg   = Cfg->HW;
	Part  = Cfg->Part[Slv];

  #else
	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (QSPI_MAX_DEVICES))
	||  (Slv < 0)
	||  (Slv >= (QSPI_MAX_SLAVES))) {
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - out of range device / slave\n", Dev, Slv);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by QSPI_LIST_DEVICE				*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - cannot remap device\n", Dev, Slv);
	  #endif
		return(-2);
	}

	if (Buf == (void *)NULL) {
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - NULL buffer\n", Dev, Slv);
	  #endif
		return(-3);
	}

	Cfg = &g_QSPIcfg[ReMap];						/* This is this device configuration			*/
	if ((g_CfgIsInit == 0)
	||  (0 == (Cfg->SlvInit & (1U << Slv)))) {		/* The controller is not initialized			*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - device / slave not initialized\n", Dev, Slv);
	  #endif
		return(-4);
	}

	Reg  = Cfg->HW;
	Part = Cfg->Part[Slv];

	if ((Addr+Len) > Part->Size[PART_DEV_SZ]) {		/* This request exceeds the device size			*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - read location exceeds device size\n", Dev, Slv);
	  #endif
		return(-5);
	}
  #endif

	if (Len == 0) {									/* Read of nothing is not considered an error	*/
		return(0);
	}
													/* Check if ACP remapping is requested			*/
	if (((Cfg->Mode[Slv] & (QSPI_CFG_CACHE_RX(1,0))) == QSPI_CFG_CACHE_RX(1,0))
	&&  ((0xC0000000 & (uintptr_t)Buf) == 0x80000000)) {	/* Check the address 2 MSBits			*/
		Buf8 = (uint8_t *)((0x3FFFFFFF & (uintptr_t)Buf)	/* When is ACP, remap to ACP address	*/
		                 | (0xC0000000 & (Cfg->Mode[Slv])));
	}
	else {
		Buf8 = (uint8_t *)Buf;
	}

	IsExp   = 0;									/* !=0 indicates a timeout						*/
	XferMax = Part->Size[PART_DIE_SZ];				/* Maximum single transfer size is die size		*/
	SzMask  = ~(XferMax-1);							/* Mask to find out size of each write op		*/

	if (0 != QSPI_MTX_LOCK(Cfg)) {					/* Exclusive access protection if enable		*/
		return(-6);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	Reg[QSPI_CFG_REG]         = Cfg->CfgRd[Slv];	/* Set-up the controller for this slave			*/
	Reg[QSPI_DEVRD_REG]       = Cfg->DevRd[Slv];
	Reg[QSPI_DELAY_REG]       = Cfg->Delay[Slv];
	Reg[QSPI_DEVSZ_REG]       = Cfg->DevSz[Slv];
	Reg[QSPI_RDDATACAP_REG]   = Cfg->RdCap[Slv];
	Reg[QSPI_MODEBIT_REG]     = 0xFF & Part->ModeRd;/* If mode byte must be sent (info in DevRd)	*/
	Reg[QSPI_SRAMPART_REG]    = QSPI_RX_SRAM_PART;
	Reg[QSPI_INDADDRTRIG_REG] = 0;

  #if (((QSPI_OPERATION) & 0x00002) != 0)			/* Using interrupts for reading					*/
	Cfg->ISRisRD  = 1;								/* Tell the ISR this is a read transfer			*/
  #endif

	do {											/* New read needed when crossing die boundaries	*/
		XferLen = Len;								/* Crossing die requires a new read command		*/
		if ((SzMask & (Addr+Len-1)) != (SzMask & Addr)) { /* If different, crossing die boundaries	*/
			XferLen = XferMax						/* Only transfer up to the die boundary			*/
			        - (Addr & ~SzMask);
		}

		IsStarted = 0;

		Reg[QSPI_INDRDWATER_REG]  = 0;				/* Disable watermark if doing polling			*/
		Reg[QSPI_INDRDSTADDR_REG] = Addr;			/* Base address to start reading from			*/
		Reg[QSPI_INDRDCNT_REG]    = XferLen;		/* Number of bytes to read from the QSPI flash	*/
		LeftRX                    = XferLen;		/* This is how many bytes are left to read		*/

		TimeOut = ((LeftRX+6) * Cfg->NsecByteRd[Slv])/* Timeout waiting for the the transfer done	*/
		        >> 17;		         				/* Doing /(128*1024) instead of *8/(1000*1000)	*/
		TimeOut = OS_MS_TO_MIN_TICK(TimeOut, 3);	/* +4 for op-code(1), address(4), may e mode(1)	*/
													/* Convert ms to RTOS timer tick count			*/
		Expiry = OS_TICK_EXP(TimeOut);				/* This is where the Xfer is considered stared	*/

	  #if (((QSPI_OPERATION) & 0x00002) != 0)		/* Using interrupts for reading					*/
		UseISR = 0U;
		if (LeftRX >= (QSPI_MIN_4_RX_ISR)) {		/* Enough data to use ISRs?						*/
			Cfg->ISRbuf    = Buf8;					/* Tell the ISR the buffer to fill				*/
			Cfg->ISRleftRX = LeftRX;				/* Tell the ISR the # bytes left to read		*/

			ii = (LeftRX <= (QSPI_RX_FIFO_SIZE))	/* When all the data to read fits in RX FIFO	*/
			   ? LeftRX								/* Wait for the RX FIFO to have all of them		*/
			   : (QSPI_RX_WATERMARK);				/* before triggering the ISR					*/
													/* Ne check below for -1 as LeftRX is >= 4		*/
			Reg[QSPI_INDRDWATER_REG] = ii-1;		/* ISR are trigger "PAST" the watermark			*/
													/* & ~3 as the FIFO are 4 bytes entries			*/
			UseISR = 1U;
		}
	  #endif

	  #if (((QSPI_OPERATION) & 0x00004) != 0)		/* Using DMA for reading						*/
		DMAid      = 0;								/* If DMA is not used, it will remain == 0		*/
		Reg[QSPI_CFG_REG] &= ~(1<<15);				/* Disable the DMA transfers					*/
													/* These 3 are constant, optmizer should reduce	*/
		if  ((LeftRX >= (QSPI_MIN_4_RX_DMA))		/* Enough data to use the DMA?					*/
		&&   (LeftRX >= (QSPI_RX_FIFO_SIZE))) {		/* No need for DMA if all fit in RX FIFO		*/

		  #if ((QSPI_DEBUG) > 1)
			XferType |= 1U << 2;					/* Use the same bits as in QSPI_OPERATION		*/
		  #endif

			ii  = (((QSPI_RX_FIFO_SIZE)-1) & ~3);	/* Figure out the largest useable burst length	*/
			if (ii > (4*(DMA_MAX_BURST))) {			/* Maximizing the burst length maximises the	*/
				ii = 4*(DMA_MAX_BURST);				/* transfer rate								*/
			}
			if (ii > Len) {
				ii = Len;
			}

			Reg[QSPI_INDRDWATER_REG] = ii-1;		/* Watermark: trigger when #RX FIFO > watermark	*/
			Reg[QSPI_CFG_REG]       |= (1<<15); 	/* Enable DMA RX operations						*/

		  #if (((QSPI_OPERATION) & 0x00002) != 0)	/* There is nothing for the ISR					*/
			UseISR = 0U;							/* No ISR tranfers (DMA does them)				*/
		  #endif									/* No EOT using ISR (DMA waits for EOT)			*/

			ii = 0;									/* Program and start the DMA but don't wait for	*/
			CfgOp[ii++] = DMA_CFG_NOWAIT;			/* EOT because need REG set-up to start xfer	*/
			CfgOp[ii++] = (((QSPI_OPERATION) & 0x00202) != 0)
			            ? DMA_CFG_EOT_ISR
			            : DMA_CFG_EOT_POLLING;
		  #if ((QSPI_REMAP_LOG_ADDR) != 0)
			CfgOp[ii++] = DMA_CFG_LOGICAL_SRC;
			CfgOp[ii++] = DMA_CFG_LOGICAL_DST;
		  #endif

			if (((Cfg->Mode[Slv] & (QSPI_CFG_CACHE_RX(1,0))) == QSPI_CFG_CACHE_RX(1,0))
			&& ((0xC0000000 & (uintptr_t)Buf) == 0x80000000)) {	/* If ACP remapping is used			*/
			  #if ((QSPI_REMAP_LOG_ADDR) != 0)
				ii--;								/* Do this to overwrite DMA_CFG_LOGICAL_DST		*/
			  #endif
				CfgOp[ii++] = DMA_CFG_NOCACHE_DST;
			}
			else if (Cfg->Mode[Slv] == QSPI_CFG_CACHE_RX(0,0)) {/* No cache maintenance requested	*/
				CfgOp[ii++] = DMA_CFG_NOCACHE_DST;
			}
			CfgOp[ii] = 0;

			ii = dma_xfer(DMA_DEV,
			              Buf, 4, -1,
				          (void *)g_AHBbase[Dev], 0, g_DMAchan[Dev][1],
			              4, ii, LeftRX>>2,			/* Need >>2 due to 4 byte transfers				*/
				          DMA_OPEND_NONE, NULL, 0,
				          &CfgOp[0], &DMAid, TimeOut);

			if (ii == 0) {
				IsStarted           = 1;
				Reg[QSPI_INDRD_REG] = (1U << 5)		/* Clear the indirect done flag					*/
				                    |  1;			/* Start the read operation						*/
													/* Wait for the DMA transfer to be completed	*/
				ii = dma_wait(DMAid);
			}

			Buf8   += LeftRX & ~3;					/* Re-adjust buffer if left over to read		*/
			LeftRX &= 3;							/* What is left to read by polling				*/

			if (ii != 0) {							/* Something went wrong setting up the DMA or	*/
			  #if ((QSPI_DEBUG) > 0)				/* waiting for the EOT							*/
				printf("QSPI  [Dev:%d - Slv:%d] - Error - DMA reported error %d\n", Dev, Slv, ii);
			  #endif
				dma_kill(DMAid);					/* Make sure the channel is released			*/
				DMAid  = 0;							/* Invalidate the ID							*/
				IsExp  = 1;							/* Report expiry as is most likely the cause	*/
				LeftRX = 0;
			}
		}
	  #endif

	  #if (((QSPI_OPERATION) & 0x00002) != 0)		/* Using interrupts for reading					*/
		if (UseISR != 0U) {
			Reg[QSPI_IRQSTAT_REG] = 0xFFFFFFFF;		/* Clear pending interrupt						*/
			SEMreset(Cfg->MySema);					/* Make sure there are no pending posting		*/
			Reg[QSPI_IRQMASK_REG] =  1U << 6;		/* Enable water level reached interrupt			*/
			Reg[QSPI_INDRD_REG]   = (1U << 5)		/* Clear the indirect done flag					*/
			                      |  1U;			/* Start the read operation						*/
			ii = SEMwait(Cfg->MySema, TimeOut);		/* Wait for the whole read to be done			*/

			Reg[QSPI_IRQMASK_REG] = 0;				/* Disable further interrupts (extra safety)	*/
			LeftRX                = 0;				/* ISRs transfers all the data					*/
			IsStarted             = 1;
			if (ii != 0) {							/* Check timeout on semaphore wait				*/
				IsExp  = 1;							/* Will skip all further processing				*/
			}

			Buf8 = (uint8_t *)Cfg->ISRbuf;			/* Needed when crossing die boundary			*/

		  #if ((QSPI_DEBUG) > 1)
			XferType |= 1U << 1;					/* Use the same bits as in QSPI_OPERATION		*/
		  #endif
		}
	  #endif

	  #if (((QSPI_OPERATION) & 0x00001) != 0)
		IsrState = OSintOff();
	  #endif

		if ((LeftRX != 0)							/* Data to read with polling?					*/
		&&  (IsStarted == 0)) {						/* If the transfer hasn't been started do it	*/
			Reg[QSPI_INDRD_REG] = (1U << 5)			/* Clear the indirect done flag					*/
			                    |  1;				/* Start the read operation						*/
		}

		while ((LeftRX != 0)						/* Polling: get everything						*/
		&&     (IsExp == 0)) {

			QSPI_EXPIRY_RX(IsExp, IsrState, Expiry);/* Check if has timed out						*/

			ii = Reg[QSPI_SRAMFILL_REG]				/* Number of 32 bit words in the RX FIFO		*/
			   & 0xFFFF;

			if ((LeftRX > 3)						/* Still 4 bytes or more to read				*/
			&&  ((0x3 & (uintptr_t)Buf8) == 0)) {	/* The buffer is aligned on 4 bytes				*/
				Buf32 = (uint32_t *)Buf8;
				jj = ii;
				if ((4*jj) > LeftRX) {				/* Needed when (LeftRX%4) != 0					*/
					jj--;
				}
				ii     -= jj;
				LeftRX -= (4*jj);					/* This less to read							*/
				for ( ; jj>0 ; jj--) {
					*Buf32++ = *g_AHBbase[Dev];
				}
				Buf8 = (uint8_t *)Buf32;
			}

			for ( ; ii>0 ; ii--) {					/* Non-aligned or less than 4 bytes to read		*/
				Buf32Tmp[0] = *g_AHBbase[Dev];		/* Copy only up to what is left to read			*/
				for (jj=0 ; (jj<4) && (LeftRX!=0) ; LeftRX--) {
					*Buf8++ = ((uint8_t *)Buf32Tmp)[jj++];
				}
			}
		}

	  #if (((QSPI_OPERATION) & 0x00001) != 0)
		OSintBack(IsrState);
	  #endif

	  #if ((QSPI_DEBUG) > 1)						/* Print after the Xfer to not impact polling	*/
		if ((XferType & (1U<<2)) != 0U) {
			printf("QSPI  [Dev:%d - Slv:%d] - RX transfer done with DMA\n", Dev, Slv);
		}
		else if ((XferType & (1U<<1)) != 0U) {
			printf("QSPI  [Dev:%d - Slv:%d] - RX transfer done through ISRs\n", Dev, Slv);
		}
		else {
			printf("QSPI  [Dev:%d - Slv:%d] - RX transfer done with polling\n", Dev, Slv);
		}
	  #endif

		if ((LeftRX != 0)							/* Something went wrong. Need to abort			*/
		||  (IsExp != 0)) {
			Reg[QSPI_CFG_REG] &= ~1;				/* Disable the controller						*/

			QSPI_MTX_UNLOCK(Cfg);
		  #if ((QSPI_DEBUG) > 0)
			printf("QSPI  [Dev:%d - Slv:%d] - Error - read error (read aborted)\n", Dev, Slv);
		  #endif
			return(-7);
		}

		while ((IsExp == 0)							/* Wait for the controler to report all the		*/
		&&     (0 == (Reg[QSPI_INDRD_REG] & (1U<<5)))) {	/* operations are done					*/
			IsExp = OS_HAS_TIMEDOUT(Expiry);		/* Don't use QSPI_EXPIRY_RX() because			*/
		}											/* interrupts will be left disabled				*/

		Len  -= XferLen;							/* This less # of bytes to read					*/
		Addr += XferLen;							/* New start address to read from				*/

	} while((Len != 0)
	  &&    (IsExp == 0));

	Reg[QSPI_CFG_REG] &= ~1;						/* Disable the controller						*/

	QSPI_MTX_UNLOCK(Cfg);

  #if ((QSPI_DEBUG) > 0)
	if (IsExp != 0) {
		printf("QSPI  [Dev:%d - Slv:%d] - Error - read timeout error\n", Dev, Slv);
	}
  #endif

	return((IsExp == 0) ? 0 : -8);	
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: qspi_write																				*/
/*																									*/
/* qspi_write - write data to the flash																*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int qspi_write(int Dev, int Slv, uint32_t Addr, const void *Buf, uint32_t Len);				*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : QSPI controller number																*/
/*		Slv  : QSPI part index (Slave index)														*/
/*		Addr : Base address where the writing to flash starts										*/
/*		Buf  : pointer to the buffer where is located the data to write								*/
/*		Len  : number of bytes to write to the flash												*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		There are no restrictions on starting address, length; a single byte can even be written.	*/
/*		The writing is broken down in chunks not exceeding the page size of the flash part.			*/
/*		Although the QSPI controller performs 32 bit writes, when there are less than 32 bit to		*/
/*		write, the bits to not write are set to 1 as this leaves the flash bits un-touched.			*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int qspi_write(int Dev, int Slv, uint32_t Addr, const void *Buf, uint32_t Len)
{
#if ((QSPI_READ_ONLY) != 0)
	return(-1);
}
#else
 const uint8_t     *Buf8;							/* Input buffer accessed one byte at a time		*/
 const uint32_t    *Buf32;							/* Input buffer accessed 4 bytes at a time		*/
 uint32_t           BufTmp[1];						/* Used to make the code Endian independent		*/
 QSPIcfg_t         *Cfg;							/* De-reference array for faster access			*/
 int                Expiry;
 int                ii;								/* General purpose								*/
 int                IsExp;							/* If timeout expired							*/
 int                jj;								/* General purpose								*/
 int                kk;								/* General purpose								*/
 int                LeftTX;							/* How many bytes left to write to TX			*/
 const Pinfo_t     *Part;							/* Part information								*/
 int                ReMap;							/* Device # remapped							*/
 volatile uint32_t *Reg;							/* Base pointer to QSPI controller registers	*/
 uint32_t           SzMask;							/* Mask used to handle max write size			*/
 int                TimeOut;						/* Timer tick value to declare a timeout error	*/
 uint32_t           XferLen;						/* Transfer length in bytes						*/
 uint32_t           XferMax;						/* Maximum size for a write burst				*/
#if (((QSPI_OPERATION) & 0x00100) != 0)				/* When using ISRs (semaphore) must also have	*/
 int                IsrState;						/* ISRs disabled								*/
#endif
#if ((QSPI_DEBUG) > 1)
 unsigned int       XferType;
#endif

  #if ((QSPI_DEBUG) > 1)
	printf("QSPI  [Dev:%d - Slv:%d] - Writing 0x%08X bytes to 0x%08X\n", Dev, Slv, (unsigned int)Len,
	       (unsigned)Addr);
	XferType = 0U;
  #endif

  #if ((QSPI_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_QSPIcfg[ReMap];						/* This is this device configuration			*/
	Reg   = Cfg->HW;
	Part  = Cfg->Part[Slv];

  #else
	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (QSPI_MAX_DEVICES))
	||  (Slv < 0)
	||  (Slv >= (QSPI_MAX_SLAVES))) {
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - out of range device / slave\n", Dev, Slv);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by QSPI_LIST_DEVICE				*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - cannot remap device\n", Dev, Slv);
	  #endif
		return(-2);
	}

	if (Buf == (void *)NULL) {
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - NULL buffer\n", Dev, Slv);
	  #endif
		return(-3);
	}

	Cfg = &g_QSPIcfg[ReMap];						/* This is this device configuration			*/
	if ((g_CfgIsInit == 0)
	||  (0 == (Cfg->SlvInit & (1U << Slv)))) {		/* The controller is not initialized			*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - device / slave not initialized\n", Dev, Slv);
	  #endif
		return(-4);
	}

	Reg  = Cfg->HW;
	Part = Cfg->Part[Slv];

	if ((Addr+Len) > Part->Size[PART_DEV_SZ]) {		/* This request exceeds the device size			*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - write location exceeds device size\n", Dev, Slv);
	  #endif
		return(-5);
	}
  #endif

	if (Len == 0) {									/* Write of nothing is not considered an error	*/
		return(0);
	}

	Buf8    = (uint8_t *)Buf;						/* We work with byte buffer						*/
	IsExp   = 0;									/* !=0 indicates a timeout						*/
	XferMax = Part->Size[PART_PAGE_SZ];				/* Maximum single transfer size is page size	*/
	SzMask  = ~(XferMax-1);							/* Mask to find out size of each write op		*/

	if (0 != QSPI_MTX_LOCK(Cfg)) {					/* Exclusive access protection if enable		*/
		return(-6);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	Reg[QSPI_CFG_REG]         = Cfg->CfgWr[Slv];	/* Set-up the controller for this slave			*/
	Reg[QSPI_DEVRD_REG]       = (Reg[QSPI_DEVRD_REG] & ~(3U << 8))
	                          | (Part->OpWrt & (3U << 8));
	Reg[QSPI_DEVWR_REG]       = Cfg->DevWr[Slv];
	Reg[QSPI_DELAY_REG]       = Cfg->Delay[Slv];
	Reg[QSPI_DEVSZ_REG]       = Cfg->DevSz[Slv];
	Reg[QSPI_RDDATACAP_REG]   = Cfg->RdCap[Slv];
	Reg[QSPI_SRAMPART_REG]    = QSPI_TX_SRAM_PART;
	Reg[QSPI_INDADDRTRIG_REG] = 0;

  #if (((QSPI_OPERATION) & 0x00200) != 0)			/* If interrupts on write are used				*/
	Cfg->ISRisRD = 0;								/* Inform ISR this is a write transfer			*/
  #endif

	do {											/* New write needed when crossing pages			*/
		XferLen = Len;
		if ((SzMask & (Addr+Len-1)) != (SzMask & Addr)) { /* If different, crossing boundaries		*/
			XferLen = XferMax						/* Only transfer up to the page boundary		*/
			        - (Addr & ~SzMask);
		}

		Reg[QSPI_INDWRWATER_REG]  = 0xFFFFFFFF;		/* Disable watermark for polling				*/
		Reg[QSPI_INDWRSTADDR_REG] = Addr;			/* Base address to start writing to				*/
		Reg[QSPI_INDWRCNT_REG]    = XferLen;		/* Number of bytes to write to the QSPI flash	*/
		LeftTX                    = XferLen;		/* Number of bytes left to write to TX FIFO		*/

		TimeOut = ((LeftTX+5) * Cfg->NsecByteWr[Slv])/* Timeout waiting for the the transfer done	*/
		        >> 17;		         				/* Doing /(128*1024) instead of *8/(1000*1000)	*/
		TimeOut = OS_MS_TO_MIN_TICK(TimeOut, 3);	/* +4 for op-code(1), address(4)				*/
													/* Convert ms to RTOS timer tick count			*/
	  #if (((QSPI_OPERATION) & 0x00200) != 0)		/* If interrupts on write are used				*/
		if (XferLen >= (QSPI_MIN_4_TX_ISR)) {		/* Back to polling as ISR has too much overhead	*/
			SEMreset(Cfg->MySema);					/* Make sure there are no pending posting		*/
			ii = ((LeftTX) >= (QSPI_TX_WATERMARK))	/* Make sure the water mark is not too high		*/
			   ? (QSPI_TX_WATERMARK)				/* otherwise the ISR will be immediately		*/
			   : LeftTX;							/* triggered									*/
			Reg[QSPI_INDWRWATER_REG] = ii;
			Reg[QSPI_IRQSTAT_REG]    = 0xFFFFFFFF;	/* Clear any pending interrupt					*/
			Reg[QSPI_IRQMASK_REG]    = 1U << 6;		/* Enable water level reached interrupt			*/

		  #if ((QSPI_DEBUG) > 1)
			XferType |= 1U << 1;					/* Use the same bits as in QSPI_OPERATION		*/
		  #endif
		}
	  #endif

	  #if (((QSPI_OPERATION) & 0x00100) != 0)
		IsrState = OSintOff();
	  #endif

		CmdWrt(Cfg, QSPI_CMD_WRT_ENB, NULL, 0);		/* Must always enable write before writing		*/

		Expiry = OS_TICK_EXP(TimeOut);				/* This is where the Xfer is considered stared	*/

		Reg[QSPI_INDWR_REG] = (1U << 5)				/* Clear the indirect done flag					*/
		                    |  1;					/* Start the write operation					*/

		Buf32   = (uint32_t *)Buf8;
		if ((LeftTX != 0)							/* Remainder of the transfer after filling		*/
		&&  (IsExp == 0)) {							/* the TX FIFO with as many 32 bit as possible	*/

			QSPI_EXPIRY_TX(IsExp, IsrState, Expiry);

			ii = ((QSPI_TX_FIFO_SIZE) & ~3)			/* Room available in the TX FIFO (# bytes)		*/
			   -  (((Reg[QSPI_SRAMFILL_REG] >> 16)
			     & 0xFFFF)
			    * 4);

			if (0 == (0x3 & (uintptr_t)Buf8)) {		/* If 32 bit transfers are possible				*/
				jj = ii;
				if (jj > LeftTX) {					/* More room in TX FIFO than left to write		*/
					jj = LeftTX & ~3;				/* ~3: this section does 4 byte write			*/
				}
				LeftTX -= jj;						/* This less to write							*/
				ii     -= jj;						/* Update the room left in the TX FIFO			*/
				for ( ; jj>0 ; jj-=4) {
					*g_AHBbase[Dev] = *Buf32++;		/* Copy the data from the buffer to the TX FIFO	*/
				}
				Buf8 = (uint8_t *)Buf32;			/* Keep Buf8 in-sync for LeftTX%4 != 0			*/
			}

			if (ii > LeftTX) {						/* If more room in TX FIFO than left to write	*/
				ii = LeftTX;
			}
			for ( ; ii>0 ; ii-=4) {
				BufTmp[0] = 0xFFFFFFFF;				/* Use BufTmp[] to create a 32 bit data			*/
													/* if <4 bytes left, 0xFF leaves byte untouched	*/
				jj = (LeftTX < 4)					/* Number of bytes to write in this iteration	*/
				   ? LeftTX
				   : 4;
				LeftTX -= jj;						/* This less to write							*/
				for (kk=0 ; jj>0 ; jj--) {
					((uint8_t *)BufTmp)[kk++] = *Buf8++;	/* 0xFF leaves the byte untouched		*/
				}
				*g_AHBbase[Dev] = BufTmp[0];		/* Put the data to write in the TX FIFO			*/
			}
		}

	  #if (((QSPI_OPERATION) & 0x00100) != 0)		/* If ISRs were disable, re-enable them to get	*/
		OSintBack(IsrState);						/* the semaphore posting in the QSPI interrupt	*/
	  #endif

		Len  -= XferLen;							/* This less to write							*/
		Addr += XferLen;							/* New start address							*/

	  #if (((QSPI_OPERATION) & 0x00200) != 0)
		if ((XferLen >= (QSPI_MIN_4_TX_ISR))
		&&  (IsExp == 0)
		&&  (((Reg[QSPI_SRAMFILL_REG] >> 16)*4) > Reg[QSPI_INDWRWATER_REG])) {
			IsExp = SEMwait(Cfg->MySema, 100*TimeOut);	/* No need to block if ISR already raised	*/
		}
		Reg[QSPI_IRQMASK_REG] = 0;					/* Disable further interrupts (extra safety)	*/
	  #endif

		while ((IsExp == 0)							/* Wait for the controler to report all the		*/
		&&     (0 != (Reg[QSPI_INDWR_REG] & (1U<<2)))) {	/* operations are done					*/
			IsExp = OS_HAS_TIMEDOUT(Expiry);		/* Don't use QSPI_EXPIRY_TX() because			*/
		}											/* interrupts will be left disabled				*/

		if (IsExp == 0) {
			if (0 != WaitRdy(Cfg, Slv, OS_TICK_PER_SEC)) {	/* Make sure the part finish writing	*/
				IsExp = -13;						/* All parts take only a few ms to write a page	*/
			  #if ((QSPI_DEBUG) > 0)
				printf("QSPI  [Dev:%d - Slv:%d] - Error - Takes too long to report ready\n",Dev,Slv);
			  #endif
			}
		}

	  #if ((QSPI_DEBUG) > 1)						/* Print after the Xfer to not impact polling	*/
		if ((XferType & (1U<<10)) != 0U) {			/* or expiry									*/
			printf("QSPI  [Dev:%d - Slv:%d] - TX transfer done with DMA\n", Dev, Slv);
		}
		else if ((XferType & (1U<<9)) != 0U) {
			printf("QSPI  [Dev:%d - Slv:%d] - TX transfer done through ISRs\n", Dev, Slv);
		}
		else {
			printf("QSPI  [Dev:%d - Slv:%d] - TX transfer done with polling\n", Dev, Slv);
		}
	  #endif

	} while ((Len != 0)
	  &&     (IsExp == 0));

	Reg[QSPI_CFG_REG] &= ~1;						/* Disable the controller						*/

	QSPI_MTX_UNLOCK(Cfg);

	return((IsExp == 0) ? 0 : -7);	
}
#endif

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: qspi_erase																				*/
/*																									*/
/* qspi_erase - erase part of, or all the chip														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int qspi_erase(int Dev, int Slv, uint32_t Addr, uint32_t Len);								*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : QSPI controller number																*/
/*		Slv  : QSPI part index (Slave index)														*/
/*		Addr : base address of the section to erase													*/
/*		Len  : number of bytes to erase																*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*             when +ve, is the smallest erase size													*/
/*                       returned when either Addr alignment or Len do not match any erase size		*/
/*                             or when Addr == 0 and Len == 0										*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		The way erasing operates is to always use the largest erase size capability of the chip		*/
/*		for each area of the erase section.  for example, assuming a sub-sector size of 4K and		*/
/*		a sector size of 64K, is the requested section to erase spans the following addresses		*/
/*		0x0000C000 -> 0x00037FFFF, then this can be broken down as follows:							*/
/*			0x0000C000 -> 0x0000FFFF has 4 sub-sectors, so 4 sub-sector erases						*/
/*		    0x00010000 -> 0x0002FFFF has 2 sectors, so 2 sector erases								*/
/*		    0x00030000 -> 0x0003FFFF has 8 sub-sectors, so 8 sub-sector erases						*/
/*																									*/
/*		The erase operation can either be set to continuously poll waiting for the section erasure	*/
/*		to be completed, or sleep time can be used. This is set with bit #3 in the build option		*/
/*		QSPI_OPERATION.  When sleep time are used, the sleep duration is determined by using the	*/
/*		manufacturer typical erase time (-20%) for the first sleep operation. After that, if the	*/
/*		erasure is not completed, periodic sleeps of 1/32 of that first sleep time are used until	*/
/*		the section is confirmed erased by treading the chip status register(s)						*/
/*																									*/
/*		Alternate full erase command if Addr == 0 and Len == Device_Size:							*/
/*				qspi_erase(Dev, Slv, 0xFFFFFFFF, 0);												*/
/*																									*/
/*		To obtain the smallest erase block size:													*/
/*				qspi_blksize(Dev, Slv);																*/
/* ------------------------------------------------------------------------------------------------ */

int qspi_erase(int Dev, int Slv, uint32_t Addr, uint32_t Len)
{
#if ((QSPI_READ_ONLY) != 0)
	return(-1);
}
#else
 QSPIcfg_t *Cfg;									/* De-reference array for faster access			*/
 int        ii;										/* General purpose								*/
 int        IsExp;									/* If timeout expired							*/
 uint32_t   Mask;									/* Mask to determine memory alignment			*/
 int        Naddr;									/* Number of address byte for the erase command	*/
 int        OpCode;									/* Op-code to use for the erase					*/
 const Pinfo_t *Part;								/* Part information								*/
 uint8_t    RegBuf[8];								/* Buffer to read & write to flash registers	*/
 int        ReMap;									/* Device # remapped							*/
 uint32_t   Size;									/* Erase size									*/
 uint32_t   Time;

 #if (((QSPI_OPERATION) & 0x10000) != 0)			/* Using TSKsleep when waiting for end of erase	*/
  uint32_t	 Tshort;								/* Short sleep time								*/
 #endif
 #if (((QSPI_OPERATION) & 0x10000) != 0)
  int        Expiry;
 #endif

  #if ((QSPI_DEBUG) > 1)
	printf("QSPI  [Dev:%d - Slv:%d] - Erasing 0x%08X bytes from 0x%08X\n", Dev, Slv,
	       (unsigned int)Len, (unsigned)Addr);
  #endif

  #if ((QSPI_ARG_CHECK) == 0)
	ReMap = g_DevReMap[Dev];
	Cfg   = &g_QSPIcfg[ReMap];						/* This is this device configuration			*/

  #else
	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (QSPI_MAX_DEVICES))
	||  (Slv < 0)
	||  (Slv >= (QSPI_MAX_SLAVES))) {
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - out of range device / slave\n", Dev, Slv);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by QSPI_LIST_DEVICE				*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - cannot remap device\n", Dev, Slv);
	  #endif
		return(-2);
	}

	Cfg = &g_QSPIcfg[ReMap];						/* This is this device configuration			*/
	if ((g_CfgIsInit == 0)
	||  (0 == (Cfg->SlvInit & (1U << Slv)))) {		/* The controller is not initialized			*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - device / slave not initialized\n", Dev, Slv);
	  #endif
		return(-3);
	}
  #endif

	Part = Cfg->Part[Slv];
													/* -------------------------------------------- */
	if ((Addr == 0xFFFFFFFF)						/* Full chip erase								*/
	&&  (Len  == 0)) {								/* Full erase start at address 0 and the erase	*/
		Addr = 0;									/* length is the device full size				*/
		Len  = Part->Size[PART_DEV_SZ];
	}

  #if ((QSPI_ARG_CHECK) != 0)
	if ((Addr+Len) > Part->Size[PART_DEV_SZ]) {		/* This request exceeds the device size			*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - erase size exceeds device size\n", Dev, Slv);
	  #endif
		return(-4);
	}
  #endif

	if (Len == 0) {									/* Erase of nothing is not considered an error	*/
		return(0);
	}

	Mask = 0 ;
	for (ii=0 ; ii<sizeof(Part->EraseCap)/sizeof(Part->EraseCap[0]) ; ii++) {
		if (Part->EraseCap[ii] != 0) {				/* A non-zero op-code, then is a valid one		*/
			Mask = Part->Size[ii]-1;
			break;
		}
	}

	if (Mask == 0) {
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - no erase capabilities in the table\n", Dev, Slv);
	  #endif
		return(-5);
	}

	if (((Len  & Mask) != 0)
	||  ((Addr & Mask) != 0)) {
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - erase Start or Start+Len are not properly aligned\n",
		       Dev, Slv);
	  #endif
		return(-6);
	}

	Naddr = (Part->Size[PART_DEV_SZ] > 0x01000000)	/* Number of address bytes as configured		*/
	      ? 4
	      : 3;
	IsExp = 0;										/* -ve indicates a timeout						*/

	if (0 != QSPI_MTX_LOCK(Cfg)) {					/* Exclusive access protection if enable		*/
		return(-7);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	Cfg->HW[QSPI_CFG_REG] = Cfg->CfgWr[Slv];		/* Must use the cfg for this slave				*/
													/* Bus speed is overriden in CmdWrt / CmdRead()	*/
	do {
		OpCode   = 0;								/* Find largest erase size cmd that fits		*/
		for (ii=0 ; ii<sizeof(Part->EraseCap)/sizeof(Part->EraseCap[0]) ; ii++) {
			if (Part->EraseCap[ii] != 0) {			/* A non-zero op-code, then is a valid one		*/
				Mask = Part->Size[ii]-1;			/* Need to check if all is aligned properly		*/
				if ((0 != (Len & ~Mask))			/* We pick the largest block for erase speed	*/
				&&  (0 == (Addr & Mask))) {			/* Simply memo when good, last memo is largest	*/
					OpCode = Part->EraseCap[ii];	/* Memo the op-code								*/
					Size   = Mask+1;				/* Memo the size this op-code erases			*/
					Time   = Part->EraseTime[ii];	/* Memo the erase time required for that size	*/
				}
			}
		}

		if (OpCode == 0) {							/* Should never happen, for safety				*/
			Cfg->HW[QSPI_CFG_REG] &= ~1;			/* Disable the controller						*/

			QSPI_MTX_UNLOCK(Cfg);
			return(-8);
		}

		if (OpCode == Part->EraseCap[PART_DEV_SZ]) {/* Bulk erase does not use an address			*/
			Naddr = 0;
		}
		else {
			SetHiAddr(Cfg, Slv, Addr);				/* Set the upper address byte if applicable		*/
			if (Naddr < 4) {
				RegBuf[0] = 0xFF & (Addr >> 16);
				RegBuf[1] = 0xFF & (Addr >> 8);
				RegBuf[2] = 0xFF & (Addr);
			}
			else {
				RegBuf[0] = 0xFF & (Addr >> 24);
				RegBuf[1] = 0xFF & (Addr >> 16);
				RegBuf[2] = 0xFF & (Addr >> 8);
				RegBuf[3] = 0xFF & (Addr);
			}
		}
		CmdWrt(Cfg, 0x06, NULL, 0);					/* Write enable									*/
		CmdWrt(Cfg, OpCode, &RegBuf[0], Naddr);		/* Send the erase command for that section size	*/

		Addr  += Size;								/* Be ready for the next erase					*/
		Len   -= Size;

		Time = OS_MS_TO_MIN_TICK(Time, 2);			/* Convert time in ms to OS ticks				*/
													/* Convert ms to RTOS timer tick count			*/
	  #if (((QSPI_OPERATION) & 0x10000) == 0)		/* No sleep requested, constant polling			*/
		ii = WaitRdy(Cfg, Slv, 10*Time);			/* Had some chips with way longer than typical	*/
		if (ii != 0) {								/* So expire after 10 time nominal				*/
			IsExp = 1;
		}

	  #else											/* Sleep requested								*/
		Tshort = Time >> 5;							/* Must wait for the erase to be finished		*/
		if (Tshort < 2) {							/* If the erase is not done after that (Time),	*/
			Tshort = 2;								/* then smaller sleeps at 1/32 of that time		*/
		}

		Expiry = OS_TICK_EXP(10*Time);				/* This is where the Xfer is considerd stared	*/

		do {										/* Had some chips with way longer than typical	*/
			TSKsleep(Time);							/* Sleep using the time from the part info		*/
			Time  = Tshort;							/* If not yet ready, must use short times		*/
			IsExp = OS_HAS_TIMEDOUT(Expiry);
		} while ((0 != WaitRdy(Cfg, Slv, 0))		/* When not yet ready, use smaller sleep time	*/
		  &&     (IsExp == 0));
	  #endif

	  #if ((QSPI_DEBUG) > 0)
		if (IsExp != 0) {
			printf("QSPI  [Dev:%d - Slv:%d] - Error - timeout waiting for end of erase\n", Dev, Slv);
		}
	  #endif

	} while ((Len > 0)								/* Redo until all area requested erased			*/
	  &&     (IsExp == 0));

	Cfg->HW[QSPI_CFG_REG] &= ~1;					/* Disable the controller						*/

	QSPI_MTX_UNLOCK(Cfg);

	return((IsExp == 0) ? 0 : -9);
}
#endif

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: qspi_size																				*/
/*																									*/
/* qspi_size - report the size of a QSPI flash														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int64_t qspi_size()																			*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : QSPI controller number																*/
/*		Slv  : QSPI part index (Slave index)														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>  0 : size in bytes of the QSPI flash part													*/
/*		<= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int64_t qspi_size(int Dev, int Slv)
{
QSPIcfg_t *Cfg;										/* De-reference array for faster access			*/
int        ReMap;									/* Device # remapped							*/

	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (QSPI_MAX_DEVICES))
	||  (Slv < 0)
	||  (Slv >= (QSPI_MAX_SLAVES))) {
		return((int64_t)-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by QSPI_LIST_DEVICE				*/
		return((int64_t)-2);
	}

	Cfg = &g_QSPIcfg[ReMap];						/* This is this device configuration			*/
	if ((g_CfgIsInit == 0)
	||  (0 == (Cfg->SlvInit & (1U << Slv)))) {		/* The controller is not initialized			*/
		return((int64_t)-3);
	}

	return((int64_t)(Cfg->Part[Slv]->Size[PART_DEV_SZ]));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: qspi_blksize																			*/
/*																									*/
/* qspi_blksize - report the smallest erase size of a QSPI flash									*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int32_t qspi_blksize(int Dev, int Slv);														*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : QSPI controller number																*/
/*		Slv  : QSPI part index (Slave index)														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>  0 : size in bytes of the smallest erase section											*/
/*		<= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int32_t qspi_blksize(int Dev, int Slv)
{
QSPIcfg_t *Cfg;										/* De-reference array for faster access			*/
int        ii;										/* General purpose								*/
const Pinfo_t *Part;
int        ReMap;									/* Device # remapped							*/

	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (QSPI_MAX_DEVICES))
	||  (Slv < 0)
	||  (Slv >= (QSPI_MAX_SLAVES))) {
		return((int32_t)-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by QSPI_LIST_DEVICE				*/
		return((int32_t)-2);
	}

	Cfg = &g_QSPIcfg[ReMap];						/* This is this device configuration			*/
	if ((g_CfgIsInit == 0)
	||  (0 == (Cfg->SlvInit & (1U << Slv)))) {		/* The controller is not initialized			*/
		return((int32_t)-3);
	}

	Part = Cfg->Part[Slv];
	for (ii=0 ; ii<((sizeof(Part->EraseCap)/sizeof(Part->EraseCap[0]))-1) ; ii++) {
		if (Part->EraseCap[ii] != 0) {
			break;
		}

	}
	return((int32_t)Part->Size[ii]);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: qspi_cmd_read																			*/
/*																									*/
/* qspi_cmd_read - to send a command to read info from the qspi memory								*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int qspi_cmd_read(int Dev, int Slv, int Cmd, uint8_t *Buf, int Nbytes);						*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev    : QSPI controller number																*/
/*		Slv    : QSPI part index (Slave index)														*/
/*		Cmd    : 8 bit command to send to the flash													*/
/*		Buf    : buffer to deposit the data read into												*/
/*		Nbytes : number of bytes to read															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
int qspi_cmd_read(int Dev, int Slv, int Cmd, uint8_t *Buf, int Nbytes)
{
QSPIcfg_t *Cfg;										/* De-reference array for faster access			*/
int        ReMap;									/* Device # remapped							*/

	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (QSPI_MAX_DEVICES))
	||  (Slv < 0)
	||  (Slv >= (QSPI_MAX_SLAVES))) {
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - out of range device / slave\n", Dev, Slv);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by QSPI_LIST_DEVICE				*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - cannot remap device\n", Dev, Slv);
	  #endif
		return(-2);
	}

	Cfg = &g_QSPIcfg[ReMap];						/* This is this device configuration			*/
	if ((g_CfgIsInit == 0)
	||  (0 == (Cfg->SlvInit & (1<<Slv)))) {			/* The controller is not initialized			*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - device / slave not initialized\n", Dev, Slv);
	  #endif
		return(-3);
	}

	if (0 != QSPI_MTX_LOCK(Cfg)) {					/* Exclusive access protection if enable		*/
		return(-3);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	Cfg->HW[QSPI_CFG_REG] = Cfg->CfgWr[Slv];		/* Must use the cfg for this slave				*/
													/* Bus speed is overriden in CmdRead()			*/
	CmdRead(Cfg, Cmd, Buf, Nbytes);

	Cfg->HW[QSPI_CFG_REG] &= ~1;					/* Disable the controller						*/

	QSPI_MTX_UNLOCK(Cfg);

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: qspi_cmd_read																			*/
/*																									*/
/* qspi_cmd_write - to send a command to write info from the qspi memory							*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void qspi_cmd_write(int Dev, int Slv, int Cmd, uint8_t *Buf, int Nbytes);					*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev    : QSPI controller number																*/
/*		Slv    : QSPI part index (Slave index)														*/
/*		Cmd    : 8 bit command to send to the flash													*/
/*		Buf    : buffer holding the data to write to the flash										*/
/*		Nbytes : number of bytes to write															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
int qspi_cmd_write(int Dev, int Slv, int Cmd, uint8_t *Buf, int Nbytes)
{
QSPIcfg_t *Cfg;										/* De-reference array for faster access			*/
int        ReMap;									/* Device # remapped							*/

	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (QSPI_MAX_DEVICES))
	||  (Slv < 0)
	||  (Slv >= (QSPI_MAX_SLAVES))) {
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - out of range device / slave\n", Dev, Slv);
	  #endif
		return(-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by QSPI_LIST_DEVICE				*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - cannot remap device\n", Dev, Slv);
	  #endif
		return(-2);
	}

	Cfg = &g_QSPIcfg[ReMap];						/* This is this device configuration			*/
	if ((g_CfgIsInit == 0)
	||  (0 == (Cfg->SlvInit & (1<<Slv)))) {			/* The controller is not initialized			*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Error - device / slave not initialized\n", Dev, Slv);
	  #endif
		return(-3);
	}

	if (0 != QSPI_MTX_LOCK(Cfg)) {					/* Exclusive access protection if enable		*/
		return(-4);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

	Cfg->HW[QSPI_CFG_REG] = Cfg->CfgWr[Slv];		/* Must use the cfg for this slave				*/
													/* Bus speed is overriden in CmdWrt()			*/
	CmdWrt(Cfg, Cmd, Buf, Nbytes);

	Cfg->HW[QSPI_CFG_REG] &= ~1;					/* Disable the controller						*/

	QSPI_MTX_UNLOCK(Cfg);

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: qspi_init																				*/
/*																									*/
/* qspi_init - initialization to access a QSPI flash part											*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int qspi_init(int Dev, int Slv, uint32_t Mode);												*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Dev  : QSPI controller number																*/
/*		Slv  : QSPI part index (Slave index)														*/
/*		Mode : QSPi operation mode																	*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int qspi_init(int Dev, int Slv, uint32_t Mode)
{
uint32_t   AndMask;									/* Some parts have special features				*/
QSPIcfg_t *Cfg;										/* De-reference array for faster access			*/
int        ClkFreq;									/* QSPI bus clock frequency						*/
int        Div;										/* Clock divider								*/
int        DumRd;									/* Number of dummy cycles required by the part	*/
int        ii;										/* Multi-purpose								*/
int        jj;										/* Multi-purpose								*/
int        LCbits;									/* Spansion LC bit for # dummy cycles			*/
const Pinfo_t *Part;
int        Part2B;									/* Part ID (lower 2 bytes only)					*/
int        Part3B;									/* Part ID (lower 3 bytes only)					*/
int        PartIdx;									/* Index in g_PartInfo of the part for Slv		*/
uint8_t    RegBuf[8];								/* Buffer to read & write to flash registers	*/
uint8_t    RegOri[8];								/* Original value of the register				*/
uint32_t   RegVal;
int        ReMap;									/* Device # remapped							*/
volatile uint32_t *Reg;								/* Base address of QSPI controller registers	*/

  #if ((QSPI_DEBUG) > 1)
	printf("QSPI - Initializing   : Dev:%d - Slv:%d\n", Dev, Slv);
  #endif

	if ((Dev < 0)									/* Check the validity of "Dev" & "Slv"			*/
	||  (Dev >= (QSPI_MAX_DEVICES))
	||  (Slv < 0)
	||  (Slv >= (QSPI_MAX_SLAVES))) {
		return(-1);
	}

	ReMap = g_DevReMap[Dev];
	if (ReMap < 0) {								/* This is set by QSPI_LIST_DEVICE				*/
		return(-2);
	}

  #if ((QSPI_USE_MUTEX) != 0)
	ii = 0;
	if (g_CfgIsInit == 0) {
		if (0 != MTXlock(G_OSmutex, -1)) {
			return(-3);								/* Although infinte wait, if deadlock detection	*/
		}											/* is enable, could return !=0 and not lock		*/
		ii = 1;
	}
  #endif

	if (g_CfgIsInit == 0) {							/* Must make sure cfg are set to default		*/
		memset(&g_QSPIcfg[0], 0, sizeof(g_QSPIcfg));
		g_CfgIsInit = 1;
	}

	Cfg = &g_QSPIcfg[ReMap];						/* This is this device configuration			*/

  #if ((QSPI_USE_MUTEX) != 0)
	if (Cfg->MyMutex == (MTX_t *)NULL) {
		Cfg->MyMutex = MTXopen(&g_Names[Dev][0]);
	}
	if (ii != 0) {
		MTXunlock(G_OSmutex);
	}
  #endif

	if (0 != QSPI_MTX_LOCK(Cfg)) {					/* Exclusive access protection if enable		*/
		return(-5);									/* Although infinte wait, if deadlock detection	*/
	}												/* is enable, could return !=0 and not lock		*/

   #if ((QSPI_OPERATION) & 0x00202)
	if (Cfg->MySema == (SEM_t *)NULL) {				/* If interrupts are used, then semaphores are	*/
		Cfg->MySema = SEMopen(&g_Names[Dev][0]);	/* the blocking mechanism						*/
	}
   #endif

	Cfg->SlvInit &= ~(1U << Slv);					/* In case re-initalizing the slave				*/

	if (Cfg->IsInit == 0) {							/* The controller is not initialized (Reset)	*/
		QSPI_RESET_DEV(Dev);

		Cfg->HW = g_HWaddr[Dev];					/* Memo the controller register base address	*/

		for (ii=100000 ; ii>=0 ; ii--) {			/* Make sure the controller is Idle				*/
			if (0 != (Cfg->HW[QSPI_CFG_REG] & (1U<<31))) {
				break;								/* This is normaly quick						*/
			}
		}
		if (ii < 0) {								/* Waited way too long, abort					*/
			QSPI_MTX_UNLOCK(Cfg);
			return(-6);
		}

		Cfg->HW[QSPI_CFG_REG]       = 0;			/* First, disable QSPI and clear all config		*/
		Cfg->HW[QSPI_REMAPADDR_REG] = 0x00000000;
		Cfg->HW[QSPI_DELAY_REG]     = 0x00000000;

		Cfg->IsInit = 1;							/* And the controller is now initialized		*/
	}

  #if ((QSPI_DEBUG) > 0)
	printf("QSPI - Device         : #%d\n", Dev);
	printf("QSPI - Slave          : #%d\n", Slv);
	printf("QSPI - RX FIFO size   : %3d bytes\n",  QSPI_RX_FIFO_SIZE);
	printf("QSPI - RX FIFO thres  : %3d bytes\n", (QSPI_RX_WATERMARK)+1);
	printf("QSPI - Min for RX DMA : %3d bytes\n",  QSPI_MIN_4_RX_DMA);
	printf("QSPI - Min for RX ISR : %3d bytes\n",  QSPI_MIN_4_RX_ISR);
	printf("QSPI - TX FIFO size   : %3d bytes\n",  QSPI_TX_FIFO_SIZE);
	printf("QSPI - TX FIFO thres  : %3d bytes\n",  QSPI_TX_WATERMARK);
//	printf("QSPI - Min for TX DMA : %3d bytes\n",  QSPI_MIN_4_TX_DMA);
	printf("QSPI - Min for TX ISR : %3d bytes\n",  QSPI_MIN_4_TX_ISR);
	putchar('\n');
  #endif

	Cfg->Mode[Slv] = Mode;
	Reg            = Cfg->HW;						/* Base address of QSPI controller registers	*/

	Reg[QSPI_IRQMASK_REG]   = 0;					/* Disable the interrupts						*/
	Reg[QSPI_REMAPADDR_REG] = 0;
	Reg[QSPI_DMAPER_REG]    = (2U << 8)				/* 4 byte for a DMA burst request				*/
	                        | (2U << 0);			/* 4 byte for a DMA single request				*/
	Reg[QSPI_DEVRD_REG]     = 0;
	Reg[QSPI_DELAY_REG]     = 0x40404040;			/* Set very long delays for safety				*/
	Reg[QSPI_CFG_REG]       = (0xFU << 19)			/* Use for now slowest clock (largest divider)	*/
	                        | (Slv << 10)			/* Chip select for this slave					*/
	                        | (0x1U <<  0);			/* Enable the QSPI controller					*/
	Reg[QSPI_FLASHCMD_REG]  = 0;

	memset(&RegBuf[0], 0, 5);						/* Make sure invalid part ID if read failure	*/

	CmdRead(Cfg, QSPI_CMD_RD_ID, &RegBuf[0], 5);	/* Read the Part ID (up to 5 bytes)				*/
	RegVal   = RegBuf[2];
	RegVal <<= 8;
	RegVal  |= RegBuf[1];
	RegVal <<= 8;
	RegVal  |= RegBuf[0];
													/* +++++ PART ADD START +++++++++++++++++++++++ */
	AndMask = 0xFFFFFFFF;
	if ((RegVal == 0x00182001)						/* These Spansion chips come in 2 flavors		*/
	||  (RegVal == 0x00190201)) {					/* Need to use ID[4] to identify the flavor		*/
		RegVal |= ((uint32_t)RegBuf[4]) << 24;
		AndMask = 0x0FFFFFFF;						/* S25FL-P has MSnibble non-zero				*/
	}
	if (RegBuf[0] == 0xC2) {						/* Macronix parts with variable dummy cycle has */
		AndMask = 0x00FFFFFF;						/* the MSByte set to non-zero					*/
	}
													/* +++++ PART ADD END +++++++++++++++++++++++++ */
  #if (((QSPI_DEBUG) > 0) || ((QSPI_ID_ONLY) != 0))
	printf("QSPI  [Dev:%d - Slv:%d] - part JEDEC ID  : 0x%08X\n", Dev, Slv, (unsigned int)RegVal);
  #endif
													/* Search for it in the part database			*/
	for (PartIdx=0 ; PartIdx<sizeof(g_PartInfo)/sizeof(g_PartInfo[0]) ; PartIdx++) {
		if ((g_PartInfo[PartIdx].DevID & AndMask) == RegVal) {
			break;
		}
	}

  #if (((QSPI_DEBUG) > 0) || ((QSPI_ID_ONLY) != 0))
	if (PartIdx < sizeof(g_PartInfo)/sizeof(g_PartInfo[0])) {
		printf("QSPI  [Dev:%d - Slv:%d] - device size    : %u\n", Dev, Slv,
		        (unsigned int)g_PartInfo[PartIdx].Size[PART_DEV_SZ]);
		for (ii=0 ; ii<5 ; ii++) {
			if (g_PartInfo[PartIdx].EraseCap[ii] != 0) {
				break;
			}
		} 
		printf("QSPI  [Dev:%d - Slv:%d] - block size     : %u\n", Dev, Slv,
		        (unsigned int)g_PartInfo[PartIdx].Size[ii]);
	}
	printf("QSPI  [Dev:%d - Slv:%d] - # part checked : %d\n", Dev, Slv,
	        sizeof(g_PartInfo)/sizeof(g_PartInfo[0]));
  #endif

  #if ((QSPI_ID_ONLY) != 0)
	if (PartIdx >= sizeof(g_PartInfo)/sizeof(g_PartInfo[0])) {
		puts("This JEDEC ID is NOT in the part definition table");
	}
   #if ((QSPI_DEBUG) > 0)
	puts("Exiting qspi_init() with error because QSPI_ID_ONLY is set to non-zero");
   #endif

	Reg[QSPI_CFG_REG] &= ~1;						/* Disable the controller						*/

	QSPI_MTX_UNLOCK(Cfg);
	return(-7);
  #endif

	if (PartIdx >= sizeof(g_PartInfo)/sizeof(g_PartInfo[0])) {
		Reg[QSPI_CFG_REG] &= ~1;					/* Disable the controller						*/
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - Unknown part %08X\n", Dev, Slv, (unsigned int)RegVal);
	  #endif

		QSPI_MTX_UNLOCK(Cfg);
		return(-8);									/* Unknown device, can't move on				*/
	}

	Part           = &g_PartInfo[PartIdx];
	Cfg->Part[Slv] = Part;							/* Memo in config for later use					*/

	/* -------------------------------------------------------------------------------------------- */
	/* The part has been identified and is supported												*/

	ii  = (int)Part->WrtMaxFrq;

  #if ((QSPI_MAX_BUS_FREQ) != 0)					/* If requested to limit the bus BW				*/
	if (ii > (QSPI_MAX_BUS_FREQ)) {
		ii = QSPI_MAX_BUS_FREQ;
	}
  #endif

	ii  *= 1000000;
	Div  = (ii-1+(QSPI_CLK))						/* Find the divisor for write max freq			*/
	     / ii;
	Div++;											/* Controller can only divide by 2*N			*/
	if (Div > 32) {									/* And /32 is the maximum division				*/
		Div = 32;
	  #if ((QSPI_DEBUG) > 1)
		printf("QSPI  [Dev:%d - Slv:%d] - SPI clk (wrt)  : Request Hz is lower than controller capability\n", Dev, Slv);
		printf("                        Using lowest possible\n");
	  #endif
	}												/* -------------- CFG REGISTER					*/
	Cfg->CfgWr[Slv]  = (((Div/2)-1) << 19)			/* Start setting up the cfg register			*/
	                   | (Slv << 10)				/* Chip select for this slave					*/
	                   | (1U  <<  0)				/* And it has to be enable						*/
	                   | (((Mode & QSPI_CFG_CS_NN) != 0)
	                     ? (1U << 9)				/* Set PERSELDEC is requested					*/
	                     : 0);

  #if ((QSPI_DEBUG) > 0)
	Div &= ~0x1;									/* Only for display; was /2 when put in regist	*/
	printf("QSPI  [Dev:%d - Slv:%d] - CTRL clock     : %9d Hz\n", Dev, Slv, QSPI_CLK);
	printf("QSPI  [Dev:%d - Slv:%d] - SPI clk (wrt)  : %9d Hz [/%d]\n", Dev, Slv, (QSPI_CLK)/Div, Div);
  #endif

	Cfg->NsecByteWr[Slv] = 8 * (1000000000			/* Nano-second per byte transfered				*/
	                        / ((QSPI_CLK)/(Div&~1)));

													/* ------------- CFG REG for writing			*/
													/* Last entry in Part->DumClk is always the max	*/
	ii      = Part->DumClkR[sizeof(Part->DumClkR)/sizeof(Part->DumClkR[0])-1];

  #if ((QSPI_MAX_BUS_FREQ) != 0)					/* If requested to limit the bus BW				*/
	if (ii > (QSPI_MAX_BUS_FREQ)) {
		ii = QSPI_MAX_BUS_FREQ;
	}
  #endif

	ii  *= 1000000;
	Div  = (ii-1+(QSPI_CLK))						/* Find the divisor for write max freq			*/
	     / ii;
	Div++;											/* Controller can only divide by 2*N			*/
	if (Div > 32) {									/* And /32 is the maximum division				*/
		Div = 32;
	  #if ((QSPI_DEBUG) > 0)
		printf("QSPI  [Dev:%d - Slv:%d] - SPI clk (read) : Request Hz is lower than controller capability\n", Dev, Slv);
		printf("                        Using lowest possible\n");
	  #endif
	}												/* -------------- CFG REGISTER					*/
	Cfg->CfgRd[Slv] = (((Div/2)-1) << 19)			/* Start setting up the cfg register			*/
	                  | (Slv << 10)					/* Chip select for this slave					*/
	                  | (1U  <<  0)					/* And it has to be enable						*/
	                  | (((Mode & QSPI_CFG_CS_NN) != 0)
	                    ? (1U << 9)					/* Set PERSELDEC is requested					*/
	                    : 0);

	Div &= ~0x1;									/* Need exact divider to get true SPI clock		*/
	ClkFreq = (QSPI_CLK)							/* Resulting SPI bus clock frequency			*/
	        / Div;									/* Find how # dummy cycle required for reading	*/
	for (DumRd=0 ; DumRd<sizeof(Part->DumClkR)/sizeof(Part->DumClkR[0]) ; DumRd++) {
		if (ClkFreq <= (1000000*Part->DumClkR[DumRd])) {
			break;
		}
	}

	Cfg->NsecByteRd[Slv] = 8 * (1000000000			/* Nano-second per byte transfered				*/
	                        / ((QSPI_CLK)/(Div&~1)));
	

	Cfg->NsecByteRd[Slv] /= (((Part->OpWrt & 0xF0000) >> 16) == 2)
	                      ? 4						/* Take into account the # of data lanes		*/
	                      : (((Part->OpWrt & 0xF0000) >> 16)+1);
  #if ((QSPI_DEBUG) > 0)
	printf("QSPI  [Dev:%d - Slv:%d] - SPI clk (read) : %9d Hz [/%d]\n", Dev, Slv, (QSPI_CLK)/Div, Div);
	printf("QSPI  [Dev:%d - Slv:%d] - Write op-code  : 0x%02X (%d-%d-%d)\n", Dev, Slv,
	                                                (unsigned int)(Part->OpWrt & 0xFF),
	                                                (int)((((Part->OpWrt & 0x00F00) >>  8) == 2) ? 4
	                                                     : ((Part->OpWrt & 0x00F00) >>  8)+1),
	                                                (int)((((Part->OpWrt & 0x0F000) >> 12) == 2) ? 4
	                                                     : ((Part->OpWrt & 0x0F000) >> 12)+1),
	                                                (int)((((Part->OpWrt & 0xF0000) >> 16) == 2) ? 4
	                                                     : ((Part->OpWrt & 0xF0000) >> 16)+1));
	printf("QSPI  [Dev:%d - Slv:%d] - Read  op-code  : 0x%02X (%d-%d-%d)\n", Dev, Slv,
	                                                (unsigned int)(Part->OpRd & 0xFF),
	                                                (int)((((Part->OpRd & 0x00F00) >>  8) == 2) ? 4
	                                                     : ((Part->OpRd & 0x00F00) >>  8)+1),
	                                                (int)((((Part->OpRd & 0x0F000) >> 12) == 2) ? 4
	                                                     : ((Part->OpRd & 0x0F000) >> 12)+1),
	                                                (int)((((Part->OpRd & 0xF0000) >> 16) == 2) ? 4
	                                                     : ((Part->OpRd & 0xF0000) >> 16)+1));
	if (Part->ModeRd != 0) {
		printf("QSPI  [Dev:%d - Slv:%d] - Read mode byte : 0x%02X\n",  Dev, Slv,
		       (unsigned int)(Part->ModeRd & 0xFF));
	}
	printf("QSPI  [Dev:%d - Slv:%d] - Read dummy clk : %d\n",  Dev, Slv, DumRd);
  #endif
													/* -------------- DEVRD REGISTER				*/

	Cfg->DevRd[Slv] = ((DumRd) << 24)				/* Number of dummy clocks for a read			*/
	                | (0x000FFFFF & Part->OpRd);	/* And the read op-code + sizes					*/
	if (0 != Part->ModeRd) {						/* If a mode byte is needed after the address	*/
		Cfg->DevRd[Slv] |= (1U << 20);				/* The mode byte is put in modebit reg in		*/
	}												/* qspi_read()									*/

													/* ------------- DEVWRT REGISTER				*/
	Cfg->DevWr[Slv] = (0xFFFFF & Part->OpWrt);		/* No dummy clocks needed for a write			*/

													/* ------------- DELAY REGISTER					*/
	RegVal = 0;
	ClkFreq = (QSPI_CLK)							/* Delays are in # clock of controller clock	*/
		    / 1000000;
	for (ii=0 ; ii<4 ; ii++) {						/* Set-up the delay register from the part info	*/
		jj = ((ClkFreq*Part->Delay[ii]) + 999)		/* Number of clock for specified delay in ns	*/
		   / 1000;
		RegVal |= (jj << (ii*8));
	}

	Cfg->Delay[Slv]     = RegVal;
	Reg[QSPI_DELAY_REG] = RegVal;

  #if ((QSPI_DEBUG) > 0)
	printf("QSPI  [Dev:%d - Slv:%d] - Delay register : 0x%08X\n",  Dev, Slv, (unsigned int)RegVal);
  #endif

	ii                = (g_RdDly[Dev][Slv] >= 0)	/* Check if RdLdy is to be overloaded for this	*/
	                  ?  g_RdDly[Dev][Slv]			/* device & slave numbers. if -ve use the table	*/
	                  :  Part->RdDly;				/* defintion.  If >= 0 use the QSPI_RDDLY_#_#	*/
	ii                = ((ClkFreq*ii) + 500)		/* build option value							*/
	                  / 1000;
	if (ii > 15) {									/* The read logic delay is held on 4 bits		*/
		ii = 15;
	}
	Cfg->RdCap[Slv]   = (ii << 1)
	                  | 1;
	Reg[QSPI_RDDATACAP_REG] = Cfg->RdCap[Slv];
  #if ((QSPI_DEBUG) > 0)
	printf("QSPI  [Dev:%d - Slv:%d] - Read delay     : %d\n", Dev, Slv, ii);
  #endif
													/* ------------- DEVSZ REGISTER					*/
	RegVal           = Part->Size[PART_SSECT_SZ]	/* Number of pages per sub-sector				*/
	                 / Part->Size[PART_PAGE_SZ];
	Cfg->DevSz[Slv]  = ((Part->Size[PART_DEV_SZ] > 0x01000000)
	                   ? 3							/* 4 byte addresses								*/
	                   : 2)							/* 3 byte addresses								*/
	                 | (Part->Size[PART_PAGE_SZ] << 4)	/* Number of bytes per page					*/
	                 | (RegVal << 16);				/* Number of block per sub-sectors				*/



													/* +++++ PART ADD START +++++++++++++++++++++++ */
	Part2B = Part->DevID
	       & 0x0000FFFF;							/* Part ID: 2 LSBytes only						*/
	Part3B = Part->DevID
	       & 0x00FFFFFF;							/* Part ID: 3 LSBytes only						*/

	/* -------------------------------------------------------------------------------------------- */
	/* Macronix specific																			*/

	if ((Part2B == 0x20C2)							/* MX25L / MX25V								*/
	||  (Part2B == 0x22C2)							/* Yes, many JEDEC IDs							*/
	||  (Part2B == 0x24C2)
	||  (Part2B == 0x25C2)) {

		CmdRead(Cfg, 0x05, &RegOri[0], 1);			/* Read status register							*/
		RegOri[0] = 0xFC & RegOri[0];				/* Ignore WEL & WIP								*/
		RegBuf[0] = 0x00 & RegOri[0];				/* Remove all write protection & set QE to 0	*/

		if (((Part->OpRd  & 0x22200) != 0)			/* Check if read or write cmd need 4 lanes		*/
		||  ((Part->OpWrt & 0x22200) != 0)) {
			RegBuf[0] |= 0x40;						/* Quad enable									*/
		  #if ((QSPI_DEBUG) > 0)					/* Dump info									*/
			printf("QSPI  [Dev:%d - Slv:%d] - Number of lanes: 4\n", Dev, Slv);
		  #endif
		}

		RegBuf[1] = 0;								/* When applicable, set the # of dummy cycles	*/
		RegOri[1] = 0;								/* in the configuration register				*/
		if ((Part->DevID & 0xFF000000) != 0) {
			jj = (Part->DevID >> 28)				/* Extract dummy cycle count bit(s) position	*/
			   & 0x0F;
			CmdRead(Cfg, 0x15, &RegOri[1], 1);		/* Read configuration register					*/
			if (Part->Size[PART_DEV_SZ] >= 0x01000000) {/* Parts >= 128 Mb use 2 or 3 bits			*/
				ii = 0x0B;							/* They are located at bit #7, #6. #4 is option	*/
			}
			else {									/* Parts smaller than 128Mb use a single bit	*/
				ii = 0x01;
			}
			RegBuf[1] = RegOri[1]					/* Zeroe the bits to be inserted				*/
			          & ~(ii << jj);
			ii        = (Part->DevID >> 24)			/* Extract the dummy cycle count				*/
			          & 0x0F;
			RegBuf[1] = RegBuf[1]					/* Insert the dummy cycle bit information		*/
			          | (ii << jj);
		}

		if ((RegOri[0] != RegBuf[0])				/* Update only if different from default		*/
		||  (RegOri[1] != RegBuf[1])) {
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			if ((Part->DevID & 0xFF000000) != 0) {
				CmdWrt(Cfg, 0x01, &RegBuf[0], 2);	/* Write back status & configurat1on reg		*/
			}
			else {
				CmdWrt(Cfg, 0x01, &RegBuf[0], 1);	/* Write back status reg						*/
			}
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure the is done						*/
		}

		if (Part->Size[PART_DEV_SZ] > 0x01000000) {	/* Set # addr byte for full memory access		*/
		  #if ((QSPI_4_BYTE_ADDRESS) == 0)			/* The drivers doesn't handle 4 bytes			*/
			CmdWrt(Cfg, 0xE9, NULL, 0);				/* Exit 4 byte addresses						*/
		  #else										/* The drivers handles 4 bytes					*/
			CmdWrt(Cfg, 0xB7, NULL, 0);				/* Enter 4 byte addresses						*/
		   #if ((QSPI_DEBUG) > 0)					/* Dump info on the flash set-up				*/
			printf("QSPI  [Dev:%d - Slv:%d] - Address bytes  : 4\n", Dev, Slv);
		   #endif
		  #endif
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure is done							*/
		}

	  #if ((QSPI_DEBUG) > 0)						/* Dump info on the flash set-up				*/
		printf("QSPI  [Dev:%d - Slv:%d] - Status reg     : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[0], RegBuf[0]);
		CmdRead(Cfg, 0x05, &RegBuf[2], 1);
		printf("    read:0x%02X\n", 0xFC & RegBuf[2]);

		if ((Part->DevID & 0xFF000000) != 0) {
			printf("QSPI  [Dev:%d - Slv:%d] - Config reg      : ori=0x%02X  new=0x%02X", Dev, Slv,
			       RegOri[1], RegBuf[1]);
			CmdRead(Cfg, 0x15, &RegBuf[2], 1);
			printf("    read:0x%02X\n", 0xFC & RegBuf[2]);
		}

	  #endif
	}
	else if (Part2B == 0x28C2) {					/* MX25R										*/

		CmdWrt(Cfg, 0x66, NULL, 0);					/* Reset Enable (RSTEN)							*/
		CmdWrt(Cfg, 0x99, NULL, 0);					/* Reset Memory (RST)							*/
		WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);			/* Make sure the is done						*/

		CmdRead(Cfg, 0x05, &RegOri[0], 1);			/* Read status register							*/
		CmdRead(Cfg, 0x15, &RegOri[1], 2);			/* Read configuration register #1 & #2			*/

		RegOri[0] = 0xFC & RegOri[0];				/* Ignore WEL & WIP								*/
		RegBuf[0] = 0x03 & RegOri[0];				/* Remove all protection & Quad Enable			*/
		RegBuf[1] =        RegOri[1];				/* Leave TB untouched							*/
		RegBuf[2] = 0xFD & RegOri[2];				/* Assume low power mode						*/

		if (((Part->OpRd  & 0x22200) != 0)			/* Check if read or write cmd need 4 lanes		*/
		||  ((Part->OpWrt & 0x22200) != 0)) {
			RegBuf[0] |= 0x40;						/* Quad enable									*/
		  #if ((QSPI_DEBUG) > 0)					/* Dump info									*/
			printf("QSPI  [Dev:%d - Slv:%d] - Number of lanes: 4\n", Dev, Slv);
		  #endif
		}

	  #if ((QSPI_MX25R_LOW_POW) == 0)
		RegBuf[2] |=  0x02;							/* High performance mode						*/
	   #if ((QSPI_DEBUG) > 0)						/* Dump											*/
		printf("QSPI  [Dev:%d - Slv:%d] - Setting to high performance mode\n", Dev, Slv);
	   #endif
	  #endif

		if ((RegOri[0] != RegBuf[0])				/* Update only if different from default		*/
		||  (RegOri[1] != RegBuf[1])
		||  (RegOri[2] != RegBuf[2])) {
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			CmdWrt(Cfg, 0x01, &RegBuf[0], 3);		/* Write back status & config #1 & config #2	*/
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure the is done						*/
		}

	  #if ((QSPI_DEBUG) > 0)						/* Dump info on the flash set-up				*/
		printf("QSPI  [Dev:%d - Slv:%d] - Status reg     : ori=0x%02X    new=0x%02X", Dev, Slv,
		       RegOri[0], RegBuf[0]);
		CmdRead(Cfg, 0x05, &RegBuf[0], 1);
		printf("    read:0x%02X\n", 0xFC & RegBuf[0]);

		printf("QSPI  [Dev:%d - Slv:%d] - Config reg     : ori=0x%02X%02X  new=0x%02X%02X", Dev, Slv,
		       RegOri[1], RegOri[2],
		                                                                 RegBuf[1], RegBuf[2]);
		CmdRead(Cfg, 0x15, &RegBuf[1], 2);
		printf("  read:0x%02X%02X\n", RegBuf[1], RegBuf[2]);
	  #endif
	}

	else if (Part2B == 0x23C2) {					/* MX25V										*/

		CmdWrt(Cfg, 0x66, NULL, 0);					/* Reset Enable (RSTEN)							*/
		CmdWrt(Cfg, 0x99, NULL, 0);					/* Reset Memory (RST)							*/
		WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);

		CmdRead(Cfg, 0x05, &RegOri[0], 1);			/* Read status register							*/
		CmdRead(Cfg, 0x15, &RegOri[1], 2);			/* Read configuration register #1 & #2			*/

		RegOri[0] = 0xFC & RegOri[0];				/* Ignore WEL & WIP								*/
		RegBuf[0] = 0x03 & RegOri[0];				/* Remove all protection & QE					*/
		RegBuf[1] = 0xBF & RegOri[1];				/* Zero dummy cycle bit							*/

		if (((Part->OpRd  & 0x22200) != 0)			/* Check if read or write cmd need 4 lanes		*/
		||  ((Part->OpWrt & 0x22200) != 0)) {
			RegBuf[0] |= 0x40;						/* Quad enable									*/
		  #if ((QSPI_DEBUG) > 0)					/* Dump info									*/
			printf("QSPI  [Dev:%d - Slv:%d] - Number of lanes: 4\n", Dev, Slv);
		  #endif
		}

		if ((Part->OpRd == (QSPI_CMD_READ_2IO))		/* Set-up the dummy cycle bit if needed			*/
		&&  (DumRd == 4)) {							/* In data sheet, 10 includes the 4 dummy for 	*/
			RegBuf[1] = 0x40 | RegBuf[1];  			/* the mode byte								*/
		  #if ((QSPI_DEBUG) > 0)					/* Dump info									*/
			printf("QSPI  [Dev:%d - Slv:%d] - Dummy cycle bit: Set\n", Dev, Slv);
		  #endif
		}

		if ((Part->OpRd == (QSPI_CMD_READ_4IO))		/* Set-up the dummy cycle bit if needed			*/
		&&  (DumRd == 8)) {							/* In data sheet, 10 includes the 2 dummy for 	*/
			RegBuf[1] = 0x40 | RegBuf[1]; 			/* the mode byte								*/
		  #if ((QSPI_DEBUG) > 0)					/* Dump info									*/
			printf("QSPI  [Dev:%d - Slv:%d] - Dummy cycle bit: Set\n", Dev, Slv);
		  #endif
		}

		if ((RegOri[0] != RegBuf[0])				/* Update only if different from default		*/
		||  (RegOri[1] != RegBuf[1])) {
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			CmdWrt(Cfg, 0x01, &RegBuf[0], 2);		/* Write back status & config #1 & config #2	*/
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure the is done						*/
		}

	  #if ((QSPI_DEBUG) > 0)						/* Dump info on the flash set-up				*/
		printf("QSPI  [Dev:%d - Slv:%d] - Status reg     : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[0], RegBuf[0]);
		CmdRead(Cfg, 0x05, &RegBuf[0], 1);
		printf("  read:0x%02X\n", RegBuf[0]);

		printf("QSPI  [Dev:%d - Slv:%d] - Config reg     : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[1], RegBuf[1]);
		CmdRead(Cfg, 0x15, &RegBuf[1], 2);
		printf("  read:0x%02X\n", RegBuf[1]);
	  #endif
	}

	/* -------------------------------------------------------------------------------------------- */
	/* Micron specific																				*/
													/* -------------------------------------------- */
	else if (Part2B == 0x4020) {					/* M45PE series don't require initialization	*/
	}												/* "else if" needed otherwise uninit error		*/
	else if ((Part2B == 0x2020)						/* M25P  series									*/
	     ||  (Part2B == 0x8020)						/* M25PE series									*/
	     ||  (Part2B == 0x7120)) {					/* M25PX series									*/

		CmdRead(Cfg, 0x05, &RegOri[0], 1);			/* Read the status register						*/
		RegOri[0] = 0xFC & RegOri[0];				/* Remove the WEL & WIP status bits				*/
		RegBuf[0] = 0x60 & RegOri[0];				/* Enable stat reg write & remove wrt protect	*/
		if (RegOri[0] != RegBuf[0]) {				/* Update only if different from default		*/
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			CmdWrt(Cfg, 0x01, &RegBuf[0], 1);		/* Write status register						*/
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure is done							*/
		}

	  #if ((QSPI_DEBUG) > 0)						/* Dump info on the flash set-up				*/
		printf("QSPI  [Dev:%d - Slv:%d] - Status reg     : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[0], RegBuf[0]);
		CmdRead(Cfg, 0x05, &RegBuf[0], 1);
		printf("  read:0x%02X\n", 0xFC & RegBuf[0]);
	  #endif		
	}
													/* -------------------------------------------- */
	else if ((Part2B == 0xBA20)						/* N25Q series									*/
	     ||  (Part2B == 0xBB20)) {
		CmdWrt(Cfg, 0x66, NULL, 0);					/* Reset enable									*/
		CmdWrt(Cfg, 0x99, NULL, 0);					/* Reset memory									*/

		CmdRead(Cfg, 0x05, &RegOri[0], 1);			/* Read the status register						*/
		RegOri[0] = 0xFC & RegOri[0];				/* Remove WEL & WIP bits						*/
		RegBuf[0] = 0x43 & RegOri[0];				/* Enable stat reg write & remove wrt protect	*/
		if (RegOri[0] != RegBuf[0]) {				/* Update only if different from default		*/
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			CmdWrt(Cfg, 0x01, &RegBuf[0], 1);			
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure the is done						*/
		}

		CmdRead(Cfg, 0x85, &RegOri[1], 1);			/* Read volatile configuration register			*/
		RegBuf[1] = (0x0F & RegOri[1])
		          | (DumRd << 4);					/* Merge the number of dummy cycles				*/
		if (RegOri[1] != RegBuf[1]) {				/* Skip write if already at the correct value	*/
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			CmdWrt(Cfg, 0x81, &RegBuf[1], 1);
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure is done							*/
		}

		CmdRead(Cfg, 0x65, &RegOri[2], 2);			/* Read enhanced register						*/
		RegBuf[2] = 0xC0 | RegOri[2];				/* Disable Dual & Quad I/O						*/
		if (RegOri[2] != RegBuf[2]) {				/* Skip write if already at the correct value	*/
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			CmdWrt(Cfg, 0x61, &RegBuf[2], 1);
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure is done							*/
		}

		if (Part->Size[PART_DEV_SZ] > 0x01000000) {	/* Set # addr byte for full memory access		*/
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
		  #if ((QSPI_4_BYTE_ADDRESS) == 0)			/* The drivers doesn't handle 4 bytes			*/
			CmdWrt(Cfg, 0xE9, NULL, 0);				/* Exit 4 byte addresses						*/
		  #else										/* The drivers handles 4 bytes					*/
			CmdWrt(Cfg, 0xB7, NULL, 0);				/* Enter 4 byte addresses						*/
		   #if ((QSPI_DEBUG) > 0)					/* Dump info on the flash set-up				*/
			printf("QSPI  [Dev:%d - Slv:%d] - Address bytes  : 4\n", Dev, Slv);
		   #endif
		  #endif
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure the is done						*/
		}

	  #if ((QSPI_DEBUG) > 0)						/* Dump info on the flash set-up				*/
		printf("QSPI  [Dev:%d - Slv:%d] - Status reg     : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[0], RegBuf[0]);
		CmdRead(Cfg, 0x05, &RegBuf[0], 1);
		printf("  read=0x%02X\n", 0xFC & RegBuf[0]);

		printf("QSPI  [Dev:%d - Slv:%d] - Volatile reg   : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[1], RegBuf[1]);
		CmdRead(Cfg, 0x85, &RegBuf[0], 1);
		printf("  read=0x%02X\n", RegBuf[0]);

		printf("QSPI  [Dev:%d - Slv:%d] - Enhanced reg   : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[2], RegBuf[2]);
		CmdRead(Cfg, 0x65, &RegBuf[0], 1);
		printf("  read=0x%02X\n", RegBuf[0]);

		CmdRead(Cfg, 0xB5, &RegBuf[0], 2);
		printf("QSPI  [Dev:%d - Slv:%d] - Non-vol reg    :   0x%02X%02X\n", Dev, Slv,
		        RegBuf[0], RegBuf[1]);
	  #endif

	}

	/* -------------------------------------------------------------------------------------------- */
	/* Cypress (Spansion) specific																	*/
													/* -------------------------------------------- */
	else if (Part3B == 0x00144001) {				/* S25FL208K / other S25FL-K parts are next		*/
		CmdRead(Cfg, 0x05, &RegOri[0], 1);			/* Read the status register #1					*/
		RegOri[0] = 0xFC & RegOri[0];				/* Remove WEL & WIP bits						*/
		RegBuf[0] = 0x40 & RegOri[0];				/* Remove all write protection (keep RFU as is)	*/

		if (RegOri[0] != RegBuf[0]) {				/* Skip write if already at the correct value	*/
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			CmdWrt(Cfg, 0x01, &RegBuf[0], 1);
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure is done							*/
		}

	  #if ((QSPI_DEBUG) > 0)						/* Dump info on the flash set-up				*/
		printf("QSPI  [Dev:%d - Slv:%d] - Status reg #1  : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[0], RegBuf[0]);
		CmdRead(Cfg, 0x05, &RegBuf[4], 1);
		printf("  read=0x%02X\n", 0xFC & RegBuf[4]);
	  #endif
	}
													/* -------------------------------------------- */
	else if (Part2B == 0x4001) {					/* S25FL-K (excluding S25FL208K / see above)	*/
		CmdWrt(Cfg, 0x66, NULL, 0);					/* Software reset enable						*/
		CmdWrt(Cfg, 0x99, NULL, 0);					/* Software reset command						*/
		WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);			/* Make sure is done							*/

		CmdRead(Cfg, 0x05, &RegOri[0], 1);			/* Read the status register #1					*/
		CmdRead(Cfg, 0x35, &RegOri[1], 1);			/* Read the status register #2					*/
		CmdRead(Cfg, 0x33, &RegOri[2], 1);			/* Read the status register #3					*/

		RegOri[0] = 0xFC & RegOri[0];				/* Remove WEL & WIP bits						*/
		RegBuf[0] = 0x00 & RegOri[0];				/* Remove all write protection					*/

		RegOri[1] = 0x3F & RegOri[1];				/* Remove SUS 									*/
		RegBuf[1] = 0x3C & RegOri[1];				/* Zero CMP & QE & SRP1							*/

		if (((Part->OpRd  & 0x22200) != 0)			/* Check if read or write cmd need 4 lanes		*/
		||  ((Part->OpWrt & 0x22200) != 0)) {
			RegBuf[1] |= 0x02;						/* Set QE bit									*/
		  #if ((QSPI_DEBUG) > 0)					/* Dump info									*/
			printf("QSPI  [Dev:%d - Slv:%d] - Number of lanes: 4\n", Dev, Slv);
		  #endif
		}

		RegBuf[2] = (0x80 & RegOri[2])				/* Keep RFU as is								*/
		          | 0x10							/* Wrap disable									*/
		          | DumRd;

		if ((RegOri[0] != RegBuf[0])				/* Skip write if already at the correct value	*/
		||  (RegOri[1] != RegBuf[1])
		||  (RegOri[2] != RegBuf[2])) {
			CmdWrt(Cfg, 0x50, NULL, 0);				/* Write enable for volatile status register	*/
			CmdWrt(Cfg, 0x01, &RegBuf[0], 3);		/* Write back status reg and may be CR1			*/
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure is done							*/
		}

	  #if ((QSPI_DEBUG) > 0)						/* Dump info on the flash set-up				*/
		printf("QSPI  [Dev:%d - Slv:%d] - Status reg #1  : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[0], RegBuf[0]);
		CmdRead(Cfg, 0x05, &RegBuf[3], 1);
		printf("  read=0x%02X\n", 0xFC & RegBuf[3]);

		printf("QSPI  [Dev:%d - Slv:%d] - Config reg #2  : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[1], RegBuf[1]);
		CmdRead(Cfg, 0x35, &RegBuf[3], 1);
		printf("  read=0x%02X\n", RegBuf[3]);

		printf("QSPI  [Dev:%d - Slv:%d] - Config reg #3  : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[2], RegBuf[2]);
		CmdRead(Cfg, 0x33, &RegBuf[3], 1);
		printf("  read=0x%02X\n", RegBuf[3]);
	  #endif
	}

	else if (((Part2B == 0x0201)					/* S25FL-P										*/
	     ||   (Part2B == 0x2001))					/* Same JEDEC ID as S25FL-S but ID in table		*/
	     &&  ((Part->DevID & 0xF0000000) != 0)) {	/* has MSNibble non-zero						*/

		CmdRead(Cfg, 0x05, &RegOri[0], 1);			/* Read status register #1						*/
		RegOri[0] = 0xFC & RegOri[0];				/* Remove WEL & WIP								*/
		RegBuf[0] = 0x00 & RegOri[0];				/* Remove all write protection					*/

		if (RegOri[0] != RegBuf[0]) {				/* Skip write if already at the correct value	*/
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			CmdWrt(Cfg, 0x01, &RegBuf[0], 1);
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure is done							*/
		}

	  #if ((QSPI_DEBUG) > 0)						/* Dump info on the flash set-up				*/
		printf("QSPI  [Dev:%d - Slv:%d] - Status reg #1  : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[0], RegBuf[0]);
		CmdRead(Cfg, 0x05, &RegBuf[4], 1);
		printf("  read=0x%02X\n", 0xFC & RegBuf[4]);
	  #endif
	}
													/* -------------------------------------------- */
	else if ((Part2B == 0x0201)						/* S25FL-S										*/
	     ||  (Part2B == 0x2001)) {

		CmdWrt(Cfg, 0x06, NULL, 0);					/* Write enable									*/
		CmdWrt(Cfg, 0xF0, NULL, 0);					/* Sofware reset								*/
		WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);			/* Make sure is done							*/

		CmdRead(Cfg, 0x05, &RegOri[0], 1);			/* Read status register #1						*/
		RegOri[0] = 0xFC & RegOri[0];				/* Remove WEL & WIP								*/
		RegBuf[0] = 0x00 & RegOri[0];				/* Remove all write protection					*/

		CmdRead(Cfg, 0x35, &RegOri[1], 1);			/* Read configuration register #1				*/
		RegBuf[1] = 0x08 | RegOri[1];				/* Set status reg BP0-BP2 volatile				*/

		LCbits = 0;									/* Assume default LC bits for dummy clocks		*/
		switch (Part->OpRd) {						/* No logical sequence in table, do manually	*/
		case QSPI_CMD_READ_FAST:					/* This code will quite likely pick the wrong	*/
		case QSPI_CMD_READ4_FAST:					/* LC bits if the DumClkR array is not set-up	*/
		case QSPI_CMD_READ_2OUT:					/* properly										*/
		case QSPI_CMD_READ4_2OUT:
		case QSPI_CMD_READ_4OUT:
		case QSPI_CMD_READ4_4OUT:
			if (DumRd == 0) {						/* LC=3 for no dummt cycles, all other LC 		*/
				LCbits = 0x3U << 6;					/* values are for 8 dummy cycles				*/
			}
			break;
		case QSPI_CMD_READ_2IO:						/* Dual I/O has different LC for 127S vs 128S	*/
		case QSPI_CMD_READ4_2IO:					/* S25FL127S needs a mode byte for Dual I/O and	*/
			if (Part->ModeRd != 0) {				/* S25FL127S does not need a mode byte			*/
				LCbits = DumRd << 6;
			}
			else {
				if (DumRd == 5) {
					LCbits = 0x1U << 6;
				}
				else if (DumRd == 6) {
					LCbits = 0x2U << 6;
				}
			}
			break;
		case QSPI_CMD_READ_4IO:
		case QSPI_CMD_READ4_4IO:
			if (DumRd == 5) {
				LCbits = 0x2U << 6;
			}
			else if (DumRd == 1) {
				LCbits = 0x3U << 6;
			}
			break;
		default:									/* QSPI_CMD_READ & QSPI_CMD_READ4				*/
			break;
		}

		if (((Part->OpRd  & 0x22200) != 0)			/* Check if read or write cmd need 4 lanes		*/
		||  ((Part->OpWrt & 0x22200) != 0)) {
			LCbits |= 0x02;							/* Must set-up the flash memory in Quad I/O		*/
		  #if ((QSPI_DEBUG) > 0)					/* Dump info									*/
			printf("QSPI  [Dev:%d - Slv:%d] - Number of lanes: 4\n", Dev, Slv);
		  #endif
		}

		RegBuf[1] = (RegBuf[1] & 0x3D)				/* Zero bits LCn (Bit #6&7) and QUAD (Bit #1)	*/
		          | LCbits;							/* Insert the LCn & QUAD for Freq & read mode	*/

		if ((RegOri[0] != RegBuf[0])				/* Skip write if already at the correct value	*/
		||  (RegOri[1] != RegBuf[1])) {
			CmdWrt(Cfg, 0x06, NULL, 0);				/* Write enable									*/
			CmdWrt(Cfg, 0x01, &RegBuf[0], 2);
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure is done							*/
		}

		if (Part->Size[PART_DEV_SZ] > 0x01000000) {	/* Set # addr byte for full memory access		*/
			CmdRead(Cfg, 0x16, &RegOri[2], 1);		/* Read the Bank Address Register (BAR)			*/
		  #if ((QSPI_4_BYTE_ADDRESS) == 0)			/* The drivers doesn't handle 4 bytes			*/
			RegBuf[2] = 0x7F & RegOri[2];			/* Clear the Extended Address Enable bit		*/
		  #else										/* The drivers handles 4 bytes					*/
			RegBuf[2] = 0x80 | RegOri[2];			/* Set the Extended Address Enable bit			*/
		   #if ((QSPI_DEBUG) > 0)					/* Dump info on the flash set-up				*/
			printf("QSPI  [Dev:%d - Slv:%d] - Address bytes  : 4\n", Dev, Slv);
		   #endif
		  #endif
			if (RegOri[2] != RegBuf[2]) {			/* Skip write if already at the correct value	*/
				CmdWrt(Cfg, 0x06, NULL, 0);			/* Write enable									*/
				CmdWrt(Cfg, 0x17, &RegBuf[2], 1);	/* Write back to the BAR						*/
				WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);	/* Make sure is done							*/
			}
		}

	  #if ((QSPI_DEBUG) > 0)						/* Dump info on the flash set-up				*/
		printf("QSPI  [Dev:%d - Slv:%d] - Status reg #1  : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[0], RegBuf[0]);
		CmdRead(Cfg, 0x05, &RegBuf[4], 1);
		printf("  read=0x%02X\n", 0xFC & RegBuf[4]);

		printf("QSPI  [Dev:%d - Slv:%d] - Config reg #2  : ori=0x%02X  new=0x%02X", Dev, Slv,
		       RegOri[1], RegBuf[1]);
		CmdRead(Cfg, 0x35, &RegBuf[4], 1);
		printf("  read=0x%02X\n", RegBuf[4]);
		if (Part->Size[PART_DEV_SZ] > 0x01000000) {	/* Set # addr byte for full memory access		*/
			printf("QSPI  [Dev:%d - Slv:%d] - Bank reg       : ori=0x%02X  new=0x%02X", Dev, Slv,
			       RegOri[2], RegBuf[2]);
			CmdRead(Cfg, 0x16, &RegBuf[4], 1);
			printf("  read=0x%02X\n", RegBuf[4]);
		}
	  #endif
	}

	/* -------------------------------------------------------------------------------------------- */
	/* SST specific																					*/

	else if (Part2B == 0x25BF)	{					/* 25FV series									*/
		CmdRead(Cfg, 0x05, &RegOri[0], 1);			/* Read status register #1						*/

		RegOri[0] = 0xFC & RegOri[0];				/* Remove WEL & WIP								*/
		RegBuf[0] = 0x43 & RegOri[0];				/* Remove all write protection					*/

		if (RegOri[0] != RegBuf[0]) {				/* Skip write if already at the correct value	*/
			CmdWrt(Cfg, 0x50, NULL, 0);				/* Write enable for volatile status register	*/
			CmdWrt(Cfg, 0x01, &RegBuf[0], 1);		/* Write back status register #1 & #2			*/
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure is done							*/
		}

	  #if ((QSPI_DEBUG) > 0)						/* Dump info on the flash set-up				*/
		printf("QSPI  [Dev:%d - Slv:%d] - Status reg #1  : ori=0x%02X    new=0x%02X", Dev, Slv,
		       RegOri[0], RegBuf[0]);
		CmdRead(Cfg, 0x05, &RegBuf[2], 1);
		printf("    read:0x%02X\n", RegBuf[2]);
	  #endif
	}

	/* -------------------------------------------------------------------------------------------- */
	/* Winbond specific																				*/

	else if ((Part2B == 0x40EF)						/* W25Q series									*/
	     ||  (Part2B == 0x60EF)) {					/* With same JEDEC ID, not all variants have	*/
													/* software reset capabilities... no reset then	*/
		CmdRead(Cfg, 0x05, &RegOri[0], 1);			/* Read status register #1						*/
		CmdRead(Cfg, 0x35, &RegOri[1], 1);			/* Read status register #2						*/

		RegOri[0] = 0xFC & RegOri[0];				/* Remove WEL & WIP								*/
		RegBuf[0] = 0x64 & RegOri[0];				/* Remove all write protection & clear QE		*/

		RegOri[1] = 0x7F & RegOri[1];				/* Remove SUS									*/
		RegBuf[1] = 0x40 & RegOri[1];				/* Remove all write protection					*/
		if (((Part->OpRd  & 0x22200) != 0)			/* Check if read or write cmd need 4 lanes		*/
		||  ((Part->OpWrt & 0x22200) != 0)) {
			RegBuf[1] |= 0x02;
		  #if ((QSPI_DEBUG) > 0)					/* Dump info									*/
			printf("QSPI  [Dev:%d - Slv:%d] - Number of lanes: 4\n", Dev, Slv);
		  #endif
		}

		if ((RegOri[0] != RegBuf[0])				/* Skip write if already at the correct value	*/
		||  (RegOri[1] != RegBuf[1])) {
			CmdWrt(Cfg, 0x50, NULL, 0);				/* Write enable for volatile status register	*/
			CmdWrt(Cfg, 0x01, &RegBuf[0], 2);		/* Write back status register #1 & #2			*/
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure is done							*/
		}

		if (Part->Size[PART_DEV_SZ] > 0x01000000) {	/* Set # addr byte for full memory access		*/
		  #if ((QSPI_4_BYTE_ADDRESS) == 0)			/* The drivers doesn't handle 4 bytes			*/
			CmdWrt(Cfg, 0xE9, NULL, 0);				/* Exit 4 byte address							*/
		  #else										/* The drivers handles 4 bytes					*/
			CmdWrt(Cfg, 0xB7, NULL, 0);				/* Enter 4 byte address							*/
		   #if ((QSPI_DEBUG) > 0)					/* Dump info									*/
			printf("QSPI  [Dev:%d - Slv:%d] - Address bytes  : 4\n", Dev, Slv);
		   #endif
		  #endif
		}

	  #if ((QSPI_DEBUG) > 0)						/* Dump info on the flash set-up				*/
		printf("QSPI  [Dev:%d - Slv:%d] - Status reg #1  : ori=0x%02X    new=0x%02X", Dev, Slv,
		       RegOri[0], RegBuf[0]);
		CmdRead(Cfg, 0x05, &RegBuf[2], 1);
		printf("    read:0x%02X\n", RegBuf[2]);
		printf("QSPI  [Dev:%d - Slv:%d] - Status reg #2  : ori=0x%02X    new=0x%02X", Dev, Slv,
		       RegOri[1], RegBuf[1]);
		CmdRead(Cfg, 0x35, &RegBuf[2], 1);
		printf("    read:0x%02X\n", RegBuf[2]);
	  #endif
	}

	else if (Part2B == 0x30EF) {					/* W25X series									*/
		CmdRead(Cfg, 0x05, &RegOri[0], 1);			/* Read status register							*/
		RegOri[0] = 0xFC & RegOri[0];				/* Remove WEL & WIP								*/
		RegBuf[0] = 0x40 & RegOri[0];				/* Remove all write protection & clear QE		*/

		if (RegOri[0] != RegBuf[0]) {				/* Skip write if already at the correct value	*/
			CmdWrt(Cfg, 0x50, NULL, 0);				/* Write enable for volatile status register	*/
			CmdWrt(Cfg, 0x01, &RegBuf[0], 1);		/* Write back status register					*/
			WaitRdy(Cfg, Slv, OS_TICK_PER_SEC);		/* Make sure is done							*/
		}

	  #if ((QSPI_DEBUG) > 0)						/* Dump info on the flash set-up				*/
		printf("QSPI  [Dev:%d - Slv:%d] - Status reg     : ori=0x%02X   new=0x%02X", Dev, Slv,
		       RegOri[0], RegBuf[0]);
		CmdRead(Cfg, 0x05, &RegBuf[1], 1);
		printf("   read:0x%02X\n", RegBuf[1]);
	  #endif
	}

	/* -------------------------------------------------------------------------------------------- */
	/* Un-supported part																			*/

	else {
		Reg[QSPI_CFG_REG] &= ~1;					/* Disable the controller						*/

		QSPI_MTX_UNLOCK(Cfg);
		return(-9);									/* Unsupported device							*/
	}												/* +++++ PART ADD END +++++++++++++++++++++++++ */

	Cfg->SlvInit |= (1U << Slv);

	Reg[QSPI_CFG_REG] &= ~1;						/* Disable the controller						*/

	QSPI_MTX_UNLOCK(Cfg);

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* Interrupt handlers for the QSPI transfers														*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void QSPIintHndl(int Dev)
{
#if (((QSPI_OPERATION) & 0x00202) != 0)				/* if interrupts are used						*/
QSPIcfg_t *Cfg;										/* De-reference array for faster access			*/
int        ReMap;									/* Device remapped index						*/
volatile uint32_t *Reg;								/* To access the QSPI controller registers		*/
#if (((QSPI_OPERATION) & 0x00002) != 0)				/* RX interrupt are used						*/
  uint8_t   *Buf8;									/* Local copy of g_BufRX						*/
  uint32_t  *Buf32;									/* Same as Buf8, but for 32 bit transfers		*/
  uint32_t   Buf32Tmp[1];							/* To read 32 bit RX FIFO with byte transfers	*/
  int        ii;									/* General purpose / loop counter / index		*/
  int        jj;									/* General purpose								*/
  int        kk;									/* General purpose								*/
  uint32_t   LeftRX;								/* Local copy of g_LeftRX						*/
#endif
#if ((QSPI_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1)
  int        zz;
#endif

	ReMap = g_DevReMap[Dev];
	Cfg   = &g_QSPIcfg[ReMap];						/* This is this device configuration			*/
	Reg   = Cfg->HW;

  #if (((QSPI_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1))	/* If multiple cores handle the ISR		*/

   #if ((OX_NESTED_INTS) != 0)						/* Get exclusive access first					*/
	ReMap = OSintOff();								/* and then set the OnGoing flag to inform the	*/
   #endif											/* other cores to skip as I'm handlking the ISR	*/
	CORElock(QSPI_SPINLOCK, &Cfg->SpinLock, 0, COREgetID()+1);

	zz = Cfg->ISRonGoing;							/* Memo if is current being handled				*/
	if (zz == 0) {
		Cfg->ISRonGoing = 1;						/* Declare as being handle / will be handle		*/
	}

	COREunlock(QSPI_SPINLOCK, &Cfg->SpinLock, 0);
  #if ((OX_NESTED_INTS) != 0)
	OSintBack(ReMap);
  #endif

	if (zz != 0) {									/* Another core was handling it					*/
		return;
	}

  #endif

													/* -------------------------------------------- */
  #if (((QSPI_OPERATION) & 0x00002) != 0)			/* RX interrupt are used						*/
	if (Cfg->ISRisRD == 1) {						/* Is a read transfer							*/
		Reg[QSPI_IRQSTAT_REG] = 0xFFFFFFFF;			/* Clear the raised interrupt					*/
		Buf8   = (uint8_t *)Cfg->ISRbuf;			/* Grab where to put the RX data and how many	*/
		LeftRX = (uint32_t)Cfg->ISRleftRX;			/* more bytes are left to read					*/

		do {										/* Need to loop as the ISR is raised when the	*/
													/* RX size crosses the watermark. Not a level	*/
			Reg[QSPI_IRQSTAT_REG] = (1U << 6);		/* Clear the raised interrupt. Agan, it's the	*/
													/* crossing and not level itself				*/
			ii = (Reg[QSPI_SRAMFILL_REG]			/* Number of 32 bit words in the RX FIFO		*/
			   & 0xFFFF);							/* Could be > LeftRX. e.g. LeftRX ==3, then ii	*/

			jj = 4*ii;								/* Number of bytes (or more) in RX FIFO			*/
			kk = Reg[QSPI_INDRDWATER_REG]+1;		/* Number of bytes that have trigger the ISR	*/
			if ((LeftRX > jj)						/* Need to make sure the last transfer length	*/
			&&  (LeftRX < (jj+kk))) {				/* is > Watermark as no interrupt issued as		*/
				ii = (LeftRX - kk)					/* RX FIFO contents <= Watermark				*/
			       / 4;
			}

			kk = ii;								/* != 0 means has read some data				*/

			if ((LeftRX > 3)						/* Still 4 bytes or more to read				*/
			&&  ((0x3 & (uintptr_t)Buf8) == 0)) {	/* The buffer is aligned on 4 bytes				*/
				Buf32 = (uint32_t *)Buf8;
				jj = ii;
				if ((4*jj) > LeftRX) {				/* Needed when (LeftRX%4) != 0					*/
					jj--;
				}
				ii     -= jj;
				LeftRX -= (4*jj);					/* This less to read							*/
				for ( ; jj>0 ; jj--) {
					*Buf32++ = *g_AHBbase[Dev];
				}
				Buf8 = (uint8_t *)Buf32;
			}

			for ( ; ii>0 ; ii--) {					/* Non-aligned or less than 4 bytes to read		*/
				Buf32Tmp[0] = *g_AHBbase[Dev];		/* Copy only up to what is left to read			*/
				for (jj=0 ; (jj<4) && (LeftRX!=0) ; LeftRX--) {
					*Buf8++ = ((uint8_t *)Buf32Tmp)[jj++];
				}
			}
		} while ((kk != 0)
		  &&     (LeftRX != 0));

		Cfg->ISRbuf    = Buf8;						/* Update where the next read will land			*/
		Cfg->ISRleftRX = LeftRX;					/* Update the real leftover count				*/
		if (LeftRX == 0) {							/* ISRs is set-up to do all the transfers		*/
			Reg[QSPI_IRQMASK_REG] = 0;				/* Done when nothing left to transfer			*/
			SEMpost(Cfg->MySema);
		}
	}
  #endif
													/* -------------------------------------------- */
  #if (((QSPI_OPERATION) & 0x00200) != 0)			/* TX interrupt are used						*/
	if (Cfg->ISRisRD == 0) {						/* Is a write transfer							*/
		Reg[QSPI_IRQMASK_REG] = 0;					/* Disable the TX interrupts					*/
		SEMpost(Cfg->MySema);						/* Post the semaphore to report transfer done	*/
	}
  #endif																		

  #if (((QSPI_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1))
	Cfg->ISRonGoing = 0;							/* Report I'm not handling the interrupt 		*/
  #endif

#else
	g_QSPIcfg[g_DevReMap[Dev]].HW[QSPI_IRQMASK_REG] = 0;

#endif

	return;															
}

/* ------------------------------------------------------------------------------------------------ */
/* Individual interrupt handlers macro. It calls the common handler with the device #				*/

#define QSPI_INT_HNDL(Dev)																			\
																									\
	void QSPIintHndl_##Dev(void)																	\
	{																								\
		QSPIintHndl(Dev);																			\
		return;																						\
	}

/* ------------------------------------------------------------------------------------------------ */

#if (((QSPI_LIST_DEVICE) & 1U) != 0U)
  QSPI_INT_HNDL(0)
#endif
#if (((QSPI_LIST_DEVICE) & 2U) != 0U)
  QSPI_INT_HNDL(1)
#endif
#if (((QSPI_LIST_DEVICE) & 4U) != 0U)
  QSPI_INT_HNDL(2)
#endif
#if (((QSPI_LIST_DEVICE) & 8U) != 0U)
  QSPI_INT_HNDL(3)
#endif
#if (((QSPI_LIST_DEVICE) & 16U) != 0U)
  QSPI_INT_HNDL(4)
#endif
#if (((QSPI_LIST_DEVICE) & 32U) != 0U)
  QSPI_INT_HNDL(5)
#endif
#if (((QSPI_LIST_DEVICE) & 64U) != 0U)
  QSPI_INT_HNDL(6)
#endif
#if (((QSPI_LIST_DEVICE) & 128U) != 0U)
  QSPI_INT_HNDL(7)
#endif
#if (((QSPI_LIST_DEVICE) & 256U) != 0U)
  QSPI_INT_HNDL(8)
#endif
#if (((QSPI_LIST_DEVICE) & 512U) != 0U)
  QSPI_INT_HNDL(9)
#endif

/* EOF */
