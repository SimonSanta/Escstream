/* ------------------------------------------------------------------------------------------------ */
/* FILE :		MediaIF.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Common interface between the drivers & all file system stacks						*/
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
/*	$Revision: 1.10 $																				*/
/*	$Date: 2019/01/10 18:07:04 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* - The file is a bit big for what it does and it is due to all the "C" pre-processor directives	*/
/*   needed to deal with the auto selection & checks.  The run-time code is less than half the		*/
/*   file size.																						*/
/*																									*/
/* Drive numbers (drive index) must be assigned to individual mass storage devices.					*/
/* The way this is done is with the use of build options MEDIA_?????_IDX, where these defines		*/
/* assign a drive number to a device controller (and possibly slave) number.						*/
/* The MEDIA_????_IDX values are NOT the device numbers used by the SDMMC & QSPI drivers,			*/
/* they are media drive numbers. For example, MEDIA_SDMMC0_IDX set to 2 and MEDIA_SDMMC0_DEV set	*/
/* to 1	indicates to map the SDMMC device #1 (device number used by the SDMMC driver) to the		*/
/* drive #2.																						*/
/*																									*/
/* Nomenclature:																					*/
/*		The expression drive number means the number used with the FS stack to select a mass 		*/
/*		mass storage media. e.g. 1:\MyFile.txt is MyFile.txt on the mass storage drive #1.			*/
/*																									*/
/* Build Options:																					*/
/*		MEDIA_SDMMC0_IDX:																			*/
/*		MEDIA_SDMMC1_IDX:																			*/
/*		MEDIA_QSPI0_IDX:																			*/
/*		MEDIA_QSPI1_IDX:																			*/
/*		MEDIA_QSPI2_IDX:																			*/
/*		MEDIA_QSPI3_IDX:																			*/
/*		MEDIA_MDRV_IDX:																				*/
/*			These define the drive numbers (from 0 to N) assigned to:								*/
/*				- a SDMMC mass storage device (MEDIA_SDMMC0_IDX)									*/
/*				- a SDMMC mass storage device (MEDIA_SDMMC1_IDX)									*/
/*				- a QSPI  mass storage device (MEDIA_QSPI0_IDX)										*/
/*				- a QSPI  mass storage device (MEDIA_QSPI1_IDX)										*/
/*				- a QSPI  mass storage device (MEDIA_QSPI2_IDX)										*/
/*				- a QSPI  mass storage device (MEDIA_QSPI3_IDX)										*/
/*				- a memory drive (MEDIA_MDRV_IDX)													*/
/*			When un-assigned, the index can be left undefined or set to -1 (See MEDIA_AUTO_SELECT).	*/
/*			There cannot be any "holes" in the index numbering; the order is not important but the	*/
/*			drive numbering must be contiguous. For example, if 3 indexes are non-negative, the		*/
/*			MEDIA_????_IDX can only have the values 0,1 and 2 alike this example:					*/
/*					MEDIA_QSPI0_IDX  == 0															*/
/*					MEDIA_MDRV_IDX   == 1															*/
/*					MEDIA_SDMMC0_IDX == 2															*/
/*			No two MEDIA_????_IDX can have the same non-negative value.								*/
/*																									*/
/*		MEDIA_SDMMC0_DEV:																			*/
/*		MEDIA_SDMMC1_DEV:																			*/
/*		MEDIA_QSPI0_DEV:																			*/
/*		MEDIA_QSPI0_SLV:																			*/
/*		MEDIA_QSPI1_DEV:																			*/
/*		MEDIA_QSPI1_SLV:																			*/
/*		MEDIA_QSPI2_DEV:																			*/
/*		MEDIA_QSPI2_SLV:																			*/
/*		MEDIA_QSPI3_DEV:																			*/
/*		MEDIA_QSPI3_SLV:																			*/
/*			These defines associate the device number (and slave number) used by the SDMMC & QSPI	*/
/*			drivers to the media drive number. e.g. MEDIA_SDMMC1_DEV is the SDMMC device number		*/
/*			to be used by the drive number MEDIA_SDMMC1_IDX.										*/
/*																									*/
/*		MEDIA_AUTO_SELECT:																			*/
/*			Boolean enabling the auto selection of the drives using the devices defined in the		*/
/*			Platform.h include file.																*/
/*			If a device defined in the Platform.h file is not to be included, then either all		*/
/*			MEDIA_SDMMC?_IDX / MEDIA_QSPI?_IDX must be defined and set to -1. If one or more are	*/
/*			not set to -1 and MEDIA_SDMMC?_DEV / MEDIA_QSPI?_DEV are not defined then the auto		*/
/*			selection will still kick in to fill the missing device(s) (and possibly slave).		*/
/*			Or the #include "Platform.h" statement can be commented out.							*/
/*			MEDIA_AUTO_SELECT also applies to the memory drive.  If the build option				*/
/*			MEDIA_MDRV_SIZE is defined and set to a non-zero value and MEDIA_MDRV_IDX is not		*/
/*			defined, the drive number of the memory drive will be auto selected.					*/
/*			MEDIA_AUTO_SELECT is enable (set to !=0) by default.									*/
/*			To know the media drive assignments, use MEDIAinfo().									*/
/*																									*/
/*		MEDIA_SDMMC_SECT_SZ:																		*/
/*		MEDIA_SDMMC0_SECT_SZ:																		*/
/*		MEDIA_SDMMC1_SECT_SZ:																		*/
/*			These build options over-rides the use of SD/MMC block size as the drive sector size.	*/
/*			When set to a positive value (> 0), it defines the sector size in bytes.				*/
/*			By default they are set to 512 as it is the standard sector size for SD/MMC cards.		*/
/*			Be aware that formatting a SD/MMC card with  a different sector size than 512 will		*/
/*			likely make that card un-usable in most systems.										*/
/*			When MEDIA_SDMMC_SECT_SZ is defined, unless a specific MEDIA_SDMMC#_SECT_SZ is defined,	*/
/*			the value assigned to MEDIA_SDMMC_SECT_SZ applied to all SDMMC							*/
/*																									*/
/*		MEDIA_QSPI_SECT_SZ:																			*/
/*		MEDIA_QSPI0_SECT_SZ:																		*/
/*		MEDIA_QSPI1_SECT_SZ:																		*/
/*		MEDIA_QSPI2_SECT_SZ:																		*/
/*		MEDIA_QSPI3_SECT_SZ:																		*/
/*			These build options over-rides the use of the QSPI sub-sector size as the drive sector	*/
/*			size.																					*/
/*			When set to a positive value (> 0), it defines the sector size in bytes.				*/
/*			By default they are set to -1 to use the the QSPI sub-sector size.						*/
/*			It is important to understand that most QSPI sub-sector size are 4096 bytes and			*/
/*			setting the sector size to a fixed value (smaller than the native sub-sector size)		*/
/*			will involve a lot of read-erase-merge-write operations.								*/
/*			When MEDIA_QSPI_SECT_SZ is defined, unless a specific MEDIA_QSPI#_SECT_SZ is defined,	*/
/*			the value assigned to MEDIA_QSPI_SECT_SZ applied to all QSPI							*/
/*																									*/
/*		MEDIA_QSPI_SECT_BUF:																		*/
/*			When MEDIA_QSPI?_SECT_SZ is positive (> 0) an interim buffer buffer is needed if the 	*/
/*			sector size set by MEDIA_QSPI?_SECT_SZ is smaller than the native QSPI sub-sector		*/
/*			size. MEDIA_QSPI_SECT_BUF is the size in byte of the interim buffer (1 per QSPI drive)	*/
/*			and it must be set to a value greater or equal to the QSPI native sub-sector size.		*/
/*																									*/
/*		MEDIA_QSPI_OPT_WRT:																			*/
/*			Boolean controlling if the QSPI write operations is used or not.  It is enable when		*/
/*			set to a non-zero value.																*/
/*			The QSPI write optimization helps greatly reduce the number of erase operation.			*/
/*			When enable, instead of blindly erasing and writing the data, the memory area of the	*/
/*			QSPI memory area to be written is read and the data from it is checked against the		*/
/*			data to write.  On QSPI flash when a bit in the flash is 1, it can be set to 0 without	*/
/*			a need for erasure.  When a QSPI bit is 0 and it must be set to 1, this is when erasure	*/
/*			is required.  The write optimization does 2 things:										*/
/*				- check if (WRITEdata == (WRITEdata & QSPIdata)) over a maximum of a QSPI sub-sector*/
/*				  size and if all data to write match this condition, the erasure is skipped.		*/
/*			    - When the erasure is skipped, it also skip leading and trailing data that are		*/
/*				  identical between WRITEdata and QSPIdata.  This possibly reduce the number of		*/
/*				  to write.																			*/
/*																									*/
/*		MEDIA_QSPI_CHK_WRT:																			*/
/*			For reliability, when set to a value N>0, all QSPI write are read-back and checked to	*/
/*			be OK, if there are mismatches, it will redo the write operation but it retries a		*/
/*			maximum of MEDIA_QSPI_CHK_WRT times														*/
/*																									*/
/*		MEDIA_SDMMC0_START:																			*/
/*		MEDIA_SDMMC1_START:																			*/
/*		MEDIA_QSPI0_START:																			*/
/*		MEDIA_QSPI1_START:																			*/
/*		MEDIA_QSPI2_START:																			*/
/*		MEDIA_QSPI3_START:																			*/
/*			These build options change the base address used when accessing the media.  When >0		*/
/*			it specifies the base address to use in blocks of 512 bytes; i.e. when sector #0 is		*/
/*			read or written to, it's the media address 512*MEDIA_NNNNN#_START that's accessed and	*/
/*			not address 0. If the corresponding MEDIA_NNNNN#_SIZE hasn't been defined, then the		*/
/*			size of the media device is reduced by 512*MEDIA_NNNNN#_START bytes.			 		*/
/*																									*/
/*		MEDIA_SDMMC0_SIZE:																			*/
/*		MEDIA_SDMMC1_SIZE:																			*/
/*		MEDIA_QSPI0_SIZE:																			*/
/*		MEDIA_QSPI1_SIZE:																			*/
/*		MEDIA_QSPI2_SIZE:																			*/
/*		MEDIA_QSPI3_SIZE:																			*/
/*			These build options change the size of the media reported to the caller.  When >0		*/
/*			it specifies the size in blocks of 512 bytes. If the defined size exceeds the size of	*/
/*			the device (taking into account when MEDIA_NNNNN#_START is defined) it will instead		*/
/*			report the accessible size.																*/
/*																									*/
/* NOTE:																							*/
/*		Make sure FatFS's maximum number of drives (build option _VOLUMES) is set large enough.		*/
/*		The system call layer inherits the value of _VOLUMES for the maximum number of drives		*/
/*		it supports.																				*/
/*		If using the System Call Layer with multiple file system stacks, then you must define		*/
/*		the build option SYS_CALL_N_DRV.															*/
/*		No verification is performed on the address / sector # etc to access as the FS stacks are	*/
/*		supposed to know the available range it can work with.										*/
/*																									*/
/*		SDMMC:																						*/
/*		  There isn't anything special about SD/MMC. The SD/MMC driver internally always perform	*/
/*		  data read and write with block sizes of 512 bytes, which is a block size supported by all	*/
/*		  SDMMC cards. That's why MEDIA_SDMMC?_SECT_SZ are set to 512 by default.					*/
/*		  See the description of MEDIA_SDMMC?_SECT_SZ on how to use a different sector size.		*/
/*																									*/
/*		QSPI:																						*/
/*		  Many QSPI chips have sub-sectors of 4K or 64K (minimum erase size). By itself this leads	*/
/*		  to FAT sector sizes of 4K or 64K.  The maximum sector size FatFS can handle is 4K, so		*/
/*		  64K sub-sector QSPI chip would not be usable.  Plus some other FAT file systems can only	*/
/*		  handle 512 bytes sectors. The build option MEDIA_QSPI_SECT_SIZE, when set to !=0, changes	*/
/*		  the way the QSPI is written.  What it does is to report to the caller the sector size		*/
/*		  defined by MEDIA_QSPI_SECT_SIZE but internally it deals with the real QSPI sub-sector		*/
/*        size.																						*/
/*		  This does not affect the QSPI read operations, only the QSPI write operations. It is		*/
/*		  done by reading the full QSPI sub-sector, erasing the whole sub-sector, inserting the 512	*/
/*		  or 1024 ... bytes to be written in the original sub-sector data and then writing the		*/
/*		  the updated sub-sector to the QSPI. One must be aware this involves a lot of erase so it	*/
/*		  will likely wear-out the QSPI much faster than if the real sub-sector size was used.		*/
/*		  See MEDIA_QSPI_CHK_FF below. When MEDIA_QSPI_SECT_SIZE is enabled, the intermediate		*/
/*		  buffer size is defined by the build option MEDIA_QSPI_SECT_BUF. The buffer size MUST be	*/
/*		  greater or equal to the QSPI sub-sector size. It is set by default to 64K and a run-time	*/
/*		  trap & optional error report will occur if the buffer is too small. MEDIA_QSPI_SECT_SIZE	*/
/*		  must be at least 512 and an exact power of two.											*/
/*																									*/
/*		MEMORY DRIVE:																				*/
/*		  A memory drive is supported where memory area in the processor address space is reserved	*/
/*		  for a drive. To use a memory driver:														*/
/*			- define and set the drive number with MEDIA_MDRV_IDX									*/
/*			- The memory reserved for the drive can be defined here or by the linker:				*/
/*				- MEDIA_MDRV_SIZE > 0																*/
/*				  A value larger than 0 defines the size of the drive with the memory allocated		*/
/*				  in the BSS through the uint8_t array g_MemDrvBase[]								*/
/*			    - MEDIA_MDRV_SIZE <= 0																*/
/*				  A value less or equal to zero uses the linker to reserve the memory.				*/
/*				  Have a look at Demos #20 -> #29 linker files and check where where the			*/
/*				  symbol _MDRV_SIZE is used.														*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include <stdio.h>
#include <string.h>

#include "Platform.h"
#include "MediaIF.h"

#if ((((OS_PLATFORM) & 0x00FFFFFF) == 0x0000AAC5)													\
 ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x0000AA10))
	#include "cd_qspi.h"
	#include "dw_sdmmc.h"
#elif ((((OS_PLATFORM) & 0x00FFFFFF) == 0x00007020)													\
   ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x00007753))
	#include "xlx_lqspi.h"
	#include "xlx_sdmmc.h"
#elif ((((OS_PLATFORM) & 0x00FFFFFF) == 0x0032F207)													\
   ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x0032F407)) 
	#include "stm32_sdmmc.h"
#else
  #error "Unknown OS_PLATFORM value"
#endif

/* ------------------------------------------------------------------------------------------------ */

#ifndef MEDIA_AUTO_SELECT							/* If selecting medias from Platform.h			*/
  #define MEDIA_AUTO_SELECT		1					/* ==0: Don't use Platform.h					*/
#endif												/* !=0: Select the devices from Platform.h		*/

#ifndef MEDIA_SDMMC_SECT_SZ							/* Sector size for all MEDIA_SDMMC except spec	*/
  #define MEDIA_SDMMC_SECT_SZ	512					/* <= 0: sector size is the card block length	*/
#endif												/* >  0: sector size to use						*/

#ifndef MEDIA_SDMMC0_SECT_SZ						/* Sector size for MEDIA_SDMMC0					*/
  #define MEDIA_SDMMC0_SECT_SZ	(MEDIA_SDMMC_SECT_SZ)	/* <= 0: sector size is the card block len	*/
#endif												/* >  0: sector size to use						*/

#ifndef MEDIA_SDMMC1_SECT_SZ
  #define MEDIA_SDMMC1_SECT_SZ	(MEDIA_SDMMC_SECT_SZ)	/* <= 0: sector size is the card block len	*/
#endif												/* >  0: sector size to use						*/

#ifndef MEDIA_QSPI_SECT_SZ							/* <= 0: sector size is flash sub-sector size	*/
  #define MEDIA_QSPI_SECT_SZ	0					/* >  0: sector size to use						*/
#endif												/* Applies to all unless individual specified	*/

#ifndef MEDIA_QSPI0_SECT_SZ							/* <= 0: sector size is flash sub-sector size	*/
  #define MEDIA_QSPI0_SECT_SZ	(MEDIA_QSPI_SECT_SZ)
#endif												/* >  0: sector size to use						*/

#ifndef MEDIA_QSPI1_SECT_SZ							/* <= 0: sector size is flash sub-sector size	*/
  #define MEDIA_QSPI1_SECT_SZ	(MEDIA_QSPI_SECT_SZ)
#endif												/* >  0: sector size to use						*/

#ifndef MEDIA_QSPI2_SECT_SZ							/* <= 0: sector size is flash sub-sector size	*/
  #define MEDIA_QSPI2_SECT_SZ	(MEDIA_QSPI_SECT_SZ)
#endif												/* >  0: sector size to use						*/

#ifndef MEDIA_QSPI3_SECT_SZ							/* <= 0: sector size is flash sub-sector size	*/
  #define MEDIA_QSPI3_SECT_SZ	(MEDIA_QSPI_SECT_SZ)
#endif												/* >  0: sector size to use						*/

#ifndef MEDIA_QSPI_SECT_BUF							/* When QSPI max sector size enable, this		*/
  #define MEDIA_QSPI_SECT_BUF	(64*1024)			/* It defines the size of the temp buffers		*/
#endif												/* **** Must be >= real QSPI sector size		*/

#ifndef MEDIA_QSPI_OPT_WRT							/* Check if sectors are blank, skip if they are	*/
  #define MEDIA_QSPI_OPT_WRT	1					/* ==0: Disable / !=0: Enable					*/
#endif												/* Enable by default: erase is a slow operation	*/

#ifndef MEDIA_QSPI_CHK_WRT							/* Check if checking if QSPI write is OK		*/
  #define MEDIA_QSPI_CHK_WRT	0					/* <= 0 : no checking							*/
#endif												/* > 0  : # of retries							*/

#ifndef MEDIA_MDRV_SIZE								/* Size & location of the m-driver size if used	*/
  #define MEDIA_MDRV_SIZE		0					/* < 0 : linker supplied						*/
#endif												/* > 0 : local buffer size						*/
													/* ==0 : not used								*/
#ifndef MEDIA_ARG_CHECK
  #define MEDIA_ARG_CHECK		0					/* If checking validity of function arguments	*/
#endif

#ifndef MEDIA_DEBUG									/* Printing debug messages						*/
  #define MEDIA_DEBUG			0					/* <=0: Disable									*/
#endif												/* ==1: errors  / > 1 operations + errors		*/

/* ------------------------------------------------------------------------------------------------ */
/* Nothing should be modified from here																*/
/* - AUTO-SELECTION when enable																		*/
/* - VALIDATION CHECKS																				*/

#if (((MEDIA_AUTO_SELECT) != 0) && defined(SDMMC_DEV))
  #if (defined(MEDIA_SDMMC0_DEV) && ((MEDIA_SDMMC0_DEV) == (SDMMC_DEV)))
	#if !defined(MEDIA_SDMMC0_IDX)					/* Index hasn't been associated to default dev	*/
	  #define MEDIA_SDMMC0_GET_IDX		1			/* Get new drive # to validate the association	*/
	#endif
  #elif (defined(MEDIA_SDMMC1_DEV) && ((MEDIA_SDMMC1_DEV) == (SDMMC_DEV)))
	#if !defined(MEDIA_SDMMC1_IDX)					/* Index hasn't been associated to default dev	*/
	  #define MEDIA_SDMMC1_GET_IDX		1			/* Get new drive # to validate the association	*/
	#endif
  #elif !defined(MEDIA_SDMMC0_IDX)
	#define MEDIA_SDMMC0_GET_IDX		1			/* Get new drive # to validate the association	*/
  #elif !defined(MEDIA_SDMMC1_IDX)
	#define MEDIA_SDMMC1_GET_IDX		1			/* Get new drive # to validate the association	*/
  #endif

  #ifdef MEDIA_SDMMC0_GET_IDX						/* SDMMC0 check redundant: it avoids mistakes	*/
	#ifndef MEDIA_SDMMC0_DEV
	  #define MEDIA_SDMMC0_DEV			(SDMMC_DEV)	/* Associate default SDMMC to MEDIA_SDMMC0		*/
    #endif

	#if   !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 0))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 0))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 0))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 0)))
	  #define MEDIA_SDMMC0_IDX			0
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 1))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 1))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 1))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 1)))
	  #define MEDIA_SDMMC0_IDX			1
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 2))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 2))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 2))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 2)))
	  #define MEDIA_SDMMC0_IDX			2
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 3))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 3))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 3))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 3)))
	  #define MEDIA_SDMMC0_IDX			3
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 4))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 4))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 4))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 4)))
	  #define MEDIA_SDMMC0_IDX			4
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 5))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 5))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 5))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 5)))
	  #define MEDIA_SDMMC0_IDX			5
	#else
	  #define MEDIA_SDMMC0_IDX			6
	#endif
  #endif

  #ifdef MEDIA_SDMMC1_GET_IDX						/* SDMMC0 check redundant: it avoids mistakes	*/
	#ifndef MEDIA_SDMMC1_DEV
	  #define MEDIA_SDMMC1_DEV			(SDMMC_DEV)	/* Associate default SDMMC to MEDIA_SDMMC1		*/
    #endif

	#if   !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 0))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 0))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 0))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 0)))
	  #define MEDIA_SDMMC1_IDX			0
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 1))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 1))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 1))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 1)))
	  #define MEDIA_SDMMC1_IDX			1
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 2))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 2))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 2))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 2)))
	  #define MEDIA_SDMMC1_IDX			2
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 3))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 3))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 3))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 3)))
	  #define MEDIA_SDMMC1_IDX			3
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 4))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 4))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 4))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 4)))
	  #define MEDIA_SDMMC1_IDX			4
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 5))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 5))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 5))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 5)))
	  #define MEDIA_SDMMC1_IDX			5
	#else
	  #define MEDIA_SDMMC1_IDX			6
	#endif
  #endif
#endif

													/* -------------------------------------------- */
#if (((MEDIA_AUTO_SELECT) != 0) && defined(QSPI_DEV) && !defined(MEDIA_QSPI_IDX))
  #if (defined(MEDIA_QSPI0_DEV) && ((MEDIA_QSPI0_DEV) == (QSPI_DEV)))
	#if !defined(MEDIA_QSPI0_IDX)					/* Index hasn't been associated to default dev	*/
	  #define MEDIA_QSPI0_GET_IDX		1			/* Get new drive # to validate the association	*/
	#endif
  #elif (defined(MEDIA_QSPI1_DEV) && ((MEDIA_QSPI1_DEV) == (QSPI_DEV)))
	#if !defined(MEDIA_QSPI1_IDX)					/* Index hasn't been associated to default dev	*/
	  #define MEDIA_QSPI1_GET_IDX		1			/* Get new drive # to validate the association	*/
	#endif
  #elif (defined(MEDIA_QSPI2_DEV) && ((MEDIA_QSPI2_DEV) == (QSPI_DEV)))
	#if !defined(MEDIA_QSPI2_IDX)					/* Index hasn't been associated to default dev	*/
	  #define MEDIA_QSPI2_GET_IDX		1			/* Get new drive # to validate the association	*/
	#endif
  #elif (defined(MEDIA_QSPI3_DEV) && ((MEDIA_QSPI3_DEV) == (QSPI_DEV)))
	#if !defined(MEDIA_QSPI3_IDX)					/* Index hasn't been associated to default dev	*/
	  #define MEDIA_QSPI3_GET_IDX		1			/* Get new drive # to validate the association	*/
	#endif
  #elif !defined(MEDIA_QSPI0_IDX)
	#define MEDIA_QSPI0_GET_IDX			1			/* Get new drive # to validate the association	*/
  #elif !defined(MEDIA_QSPI1_IDX)
	#define MEDIA_QSPI1_GET_IDX			1			/* Get new drive # to validate the association	*/
  #elif !defined(MEDIA_QSPI2_IDX)
	#define MEDIA_QSPI2_GET_IDX			1			/* Get new drive # to validate the association	*/
  #elif !defined(MEDIA_QSPI3_IDX)
	#define MEDIA_QSPI3_GET_IDX			1			/* Get new drive # to validate the association	*/
  #endif

  #ifdef MEDIA_QSPI0_GET_IDX
	#ifndef MEDIA_QSPI0_DEV
	  #define MEDIA_QSPI0_DEV			(QSPI_DEV)	/* Associate default QSPI to MEDIA_QSPI0		*/
	#endif
	#ifndef MEDIA_QSPI0_SLV
	  #define MEDIA_QSPI0_SLV			(QSPI_SLV)
	#endif
													/* QSPI0 check redundant: it avoids mistakes	*/
	#if   !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 0))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 0))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 0))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 0)))
	  #define MEDIA_QSPI0_IDX			0
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 1))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 1))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 1))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 1)))
	  #define MEDIA_QSPI0_IDX			1
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 2))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 2))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 2))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 2)))
	  #define MEDIA_QSPI0_IDX			2
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 3))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 3))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 3))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 3)))
	  #define MEDIA_QSPI0_IDX			3
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 4))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 4))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 4))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 4)))
	  #define MEDIA_QSPI0_IDX			4
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 5))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 5))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 5))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 5)))
	  #define MEDIA_QSPI0_IDX			5
	#else
	  #define MEDIA_QSPI0_IDX			6
	#endif
  #endif

  #ifdef MEDIA_SDMMC1_GET_IDX
	#ifndef MEDIA_QSPI1_DEV
	  #define MEDIA_QSPI1_DEV			(QSPI_DEV)	/* Associate default QSPI to MEDIA_QSPI1		*/
	#endif
	#ifndef MEDIA_QSPI1_SLV
	  #define MEDIA_QSPI1_SLV			(QSPI_SLV)
	#endif
													/* QSPI0 check redundant: it avoids mistakes	*/
	#if   !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 0))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 0))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 0))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 0)))
	  #define MEDIA_QSPI1_IDX			0
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 1))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 1))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 1))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 1)))
	  #define MEDIA_QSPI1_IDX			1
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 2))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 2))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 2))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 2)))
	  #define MEDIA_QSPI1_IDX			2
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 3))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 3))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 3))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 3)))
	  #define MEDIA_QSPI1_IDX			3
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 4))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 4))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 4))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 4)))
	  #define MEDIA_QSPI1_IDX			4
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 5))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 5))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 5))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 5)))
	  #define MEDIA_QSPI1_IDX			5
	#else
	  #define MEDIA_QSPI1_IDX			6
	#endif
  #endif

  #ifdef MEDIA_SDMMC2_GET_IDX
	#ifndef MEDIA_QSPI2_DEV
	  #define MEDIA_QSPI2_DEV			(QSPI_DEV)	/* Associate default QSPI to MEDIA_QSPI2		*/
	#endif
	#ifndef MEDIA_QSPI2_SLV
	  #define MEDIA_QSPI2_SLV			(QSPI_SLV)
	#endif
													/* QSPI2 check redundant: it avoids mistakes	*/
	#if   !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 0))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 0))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 0))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 0)))
	  #define MEDIA_QSPI2_IDX			0
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 1))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 1))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 1))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 1)))
	  #define MEDIA_QSPI2_IDX			1
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 2))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 2))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 2))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 2)))
	  #define MEDIA_QSPI2_IDX			2
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 3))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 3))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 3))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 3)))
	  #define MEDIA_QSPI2_IDX			3
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 4))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 4))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 4))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 4)))
	  #define MEDIA_QSPI2_IDX			4
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 5))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 5))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 5))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 5)))
	  #define MEDIA_QSPI2_IDX			5
	#else
	  #define MEDIA_QSPI2_IDX			6
	#endif
  #endif

  #ifdef MEDIA_SDMMC2_GET_IDX
	#ifndef MEDIA_QSPI3_DEV
	  #define MEDIA_QSPI3_DEV			(QSPI_DEV)	/* Associate default QSPI to MEDIA_QSPI3		*/
	#endif
	#ifndef MEDIA_QSPI3_SLV
	  #define MEDIA_QSPI3_SLV			(QSPI_SLV)
	#endif
													/* QSPI2 check redundant: it avoids mistakes	*/
	#if   !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 0))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 0))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 0))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 0)))
	  #define MEDIA_QSPI3_IDX			0
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 1))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 1))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 1))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 1)))
	  #define MEDIA_QSPI3_IDX			1
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 2))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 2))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 2))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 2)))
	  #define MEDIA_QSPI3_IDX			2
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 3))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 3))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 3))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 3)))
	  #define MEDIA_QSPI3_IDX			3
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 4))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 4))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 4))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 4)))
	  #define MEDIA_QSPI3_IDX			4
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 5))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 5))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 5))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 5)))
	  #define MEDIA_QSPI3_IDX			5
	#else
	  #define MEDIA_QSPI3_IDX			6
	#endif
  #endif


#endif

													/* -------------------------------------------- */
#if (((MEDIA_AUTO_SELECT) != 0) && defined(MEDIA_MDRV_SIZE))
 #if !defined(MEDIA_MDRV_IDX) && ((MEDIA_MDRV_SIZE) != 0)
													/* MDRV check redundant: it avoids mistakes		*/
	#if   !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 0))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 0))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 0))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 0))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 0)))
	  #define MEDIA_MDRV_IDX			0
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 1))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 1))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 1))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 1))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 1)))
	  #define MEDIA_MDRV_IDX			1
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 2))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 2))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 2))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 2))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 2)))
	  #define MEDIA_MDRV_IDX			2
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 3))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 3))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 3))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 3))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 3)))
	  #define MEDIA_MDRV_IDX			3
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 4))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 4))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 4))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 4))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 4)))
	  #define MEDIA_MDRV_IDX			4
	#elif !((defined(MEDIA_SDMMC0_IDX) && ((MEDIA_SDMMC0_IDX) == 5))								\
	 ||     (defined(MEDIA_SDMMC1_IDX) && ((MEDIA_SDMMC1_IDX) == 5))								\
	 ||     (defined(MEDIA_QSPI0_IDX)  && ((MEDIA_QSPI0_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI1_IDX)  && ((MEDIA_QSPI1_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI2_IDX)  && ((MEDIA_QSPI2_IDX)  == 5))								\
	 ||     (defined(MEDIA_QSPI3_IDX)  && ((MEDIA_QSPI3_IDX)  == 5))								\
	 ||     (defined(MEDIA_MDRV_IDX)   && ((MEDIA_MDRV_IDX)   == 5)))
	  #define MEDIA_MDRV_IDX			5
	#else
	  #define MEDIA_MDRV_IDX			6
	#endif
  #endif
#endif
													/* -------------------------------------------- */
#ifndef MEDIA_SDMMC0_IDX
  #define MEDIA_SDMMC0_IDX		-1					/* Not defined, then not used					*/
#endif

#ifndef MEDIA_SDMMC1_IDX
  #define MEDIA_SDMMC1_IDX		-1					/* Not defined, then not used					*/
#endif

#ifndef MEDIA_QSPI0_IDX
  #define MEDIA_QSPI0_IDX		-1					/* Not defined, then not used					*/
#endif

#ifndef MEDIA_QSPI1_IDX
  #define MEDIA_QSPI1_IDX		-1					/* Not defined, then not used					*/
#endif

#ifndef MEDIA_QSPI2_IDX
  #define MEDIA_QSPI2_IDX		-1					/* Not defined, then not used					*/
#endif

#ifndef MEDIA_QSPI3_IDX
  #define MEDIA_QSPI3_IDX		-1					/* Not defined, then not used					*/
#endif

#ifndef MEDIA_MDRV_IDX
  #define MEDIA_MDRV_IDX		-1					/* Not defined, then not used					*/
#endif

#ifndef MEDIA_SDMMC0_START
  #define MEDIA_SDMMC0_START	0					/* First sector # to use						*/
#endif

#ifndef MEDIA_SDMMC1_START
  #define MEDIA_SDMMC1_START	0					/* First sector # to use						*/
#endif

#ifndef MEDIA_QSPI0_START
  #define MEDIA_QSPI0_START		0					/* First sector # to use						*/
#endif

#ifndef MEDIA_QSPI1_START
  #define MEDIA_QSPI1_START		0					/* First sector # to use						*/
#endif

#ifndef MEDIA_QSPI2_START
  #define MEDIA_QSPI2_START		0					/* First sector # to use						*/
#endif

#ifndef MEDIA_QSPI3_START
  #define MEDIA_QSPI3_START		0					/* First sector # to use						*/
#endif

#ifndef MEDIA_SDMMC0_SIZE
  #define MEDIA_SDMMC0_SIZE		0					/* # sector of the media (overloads real size	*/
#endif

#ifndef MEDIA_SDMMC1_SIZE
  #define MEDIA_SDMMC1_SIZE		0					/* # sector of the media (overloads real size	*/
#endif

#ifndef MEDIA_QSPI0_SIZE
  #define MEDIA_QSPI0_SIZE		0					/* # sector of the media (overloads real size	*/
#endif

#ifndef MEDIA_QSPI1_SIZE
  #define MEDIA_QSPI1_SIZE		0					/* # sector of the media (overloads real size	*/
#endif

#ifndef MEDIA_QSPI2_SIZE
  #define MEDIA_QSPI2_SIZE		0					/* # sector of the media (overloads real size	*/
#endif

#ifndef MEDIA_QSPI3_SIZE
  #define MEDIA_QSPI3_SIZE		0					/* # sector of the media (overloads real size	*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Checks to validate the build options 															*/

#if ((MEDIA_SDMMC0_IDX) >= 0)
  #ifndef MEDIA_SDMMC0_DEV
	#error "MEDIA_SDMMC0_DEV hasn't been defined"
  #endif
#endif
#if ((MEDIA_SDMMC1_IDX) >= 0)
  #ifndef MEDIA_SDMMC1_DEV
	#error "MEDIA_SDMMC1_DEV hasn't been defined"
  #endif
#endif

#if ((MEDIA_QSPI0_IDX) >= 0)
  #ifndef MEDIA_QSPI0_DEV
	#error "MEDIA_QSPI0_DEV hasn't been defined"
  #endif
  #ifndef MEDIA_QSPI0_SLV
	#error "MEDIA_QSPI0_SLV hasn't been defined"
  #endif
#endif
#if ((MEDIA_QSPI1_IDX) >= 0)
  #ifndef MEDIA_QSPI1_DEV
	#error "MEDIA_QSPI1_DEV hasn't been defined"
  #endif
  #ifndef MEDIA_QSPI1_SLV
	#error "MEDIA_QSPI1_SLV hasn't been defined"
  #endif
#endif
#if ((MEDIA_QSPI2_IDX) >= 0)
  #ifndef MEDIA_QSPI2_DEV
	#error "MEDIA_QSPI2_DEV hasn't been defined"
  #endif
  #ifndef MEDIA_QSPI2_SLV
	#error "MEDIA_QSPI2_SLV hasn't been defined"
  #endif
#endif
#if ((MEDIA_QSPI3_IDX) >= 0)
  #ifndef MEDIA_QSPI3_DEV
	#error "MEDIA_QSPI3_DEV hasn't been defined"
  #endif
  #ifndef MEDIA_QSPI3_SLV
	#error "MEDIA_QSPI3_SLV hasn't been defined"
  #endif
#endif

#if ((MEDIA_SDMMC0_IDX) >= 0)
  #if ((MEDIA_SDMMC0_IDX) == (MEDIA_SDMMC1_IDX))
	#error "SDMMC #0 drive number is the same as SDMMC #1"
  #endif
  #if ((MEDIA_SDMMC0_IDX) == (MEDIA_QSPI0_IDX))
	#error "SDMMC #0 drive number is the same as QSPI #0"
  #endif
  #if ((MEDIA_SDMMC0_IDX) == (MEDIA_QSPI1_IDX))
	#error "SDMMC #0 drive number is the same as QSPI #1"
  #endif
  #if ((MEDIA_SDMMC0_IDX) == (MEDIA_QSPI2_IDX))
	#error "SDMMC #0 drive number is the same as QSPI #2"
  #endif
  #if ((MEDIA_SDMMC0_IDX) == (MEDIA_QSPI3_IDX))
	#error "SDMMC #0 drive number is the same as QSPI #3"
  #endif
  #if ((MEDIA_SDMMC0_IDX) == (MEDIA_MDRV_IDX))
	#error "SDMMC #0 drive number is the same as memory drive"
  #endif
#endif
#if ((MEDIA_SDMMC1_IDX) >= 0)
  #if ((MEDIA_SDMMC1_IDX) == (MEDIA_QSPI0_IDX))
	#error "SDMMC #1 drive number is the same as QSPI #0"
  #endif
  #if ((MEDIA_SDMMC1_IDX) == (MEDIA_QSPI1_IDX))
	#error "SDMMC #1 drive number is the same as QSPI #1"
  #endif
  #if ((MEDIA_SDMMC1_IDX) == (MEDIA_QSPI2_IDX))
	#error "SDMMC #1 drive number is the same as QSPI #2"
  #endif
  #if ((MEDIA_SDMMC1_IDX) == (MEDIA_QSPI3_IDX))
	#error "SDMMC #1 drive number is the same as QSPI #3"
  #endif
  #if ((MEDIA_SDMMC1_IDX) == (MEDIA_MDRV_IDX))
	#error "SDMMC #1 drive number is the same as memory drive"
  #endif
#endif

#if ((MEDIA_QSPI0_IDX) >= 0)
  #if ((MEDIA_QSPI0_IDX) == (MEDIA_QSPI1_IDX))
	#error "QSPI #0 drive number is the same as QSPI #1"
  #endif
  #if ((MEDIA_QSPI0_IDX) == (MEDIA_QSPI2_IDX))
	#error "QSPI #0 drive number is the same as QSPI #2"
  #endif
  #if ((MEDIA_QSPI0_IDX) == (MEDIA_QSPI3_IDX))
	#error "QSPI #0 drive number is the same as QSPI #3"
  #endif
  #if ((MEDIA_QSPI0_IDX) == (MEDIA_MDRV_IDX))
	#error "QSPI #0 drive number is the same as memory drive"
  #endif
#endif
#if ((MEDIA_QSPI1_IDX) >= 0)
  #if ((MEDIA_QSPI1_IDX) == (MEDIA_QSPI2_IDX))
	#error "QSPI #1 drive number is the same as QSPI #2"
  #endif
  #if ((MEDIA_QSPI1_IDX) == (MEDIA_QSPI3_IDX))
	#error "QSPI #1 drive number is the same as QSPI #3"
  #endif
  #if ((MEDIA_QSPI1_IDX) == (MEDIA_MDRV_IDX))
	#error "QSPI #1 drive number is the same as memory drive"
  #endif
#endif
#if ((MEDIA_QSPI2_IDX) >= 0)
  #if ((MEDIA_QSPI2_IDX) == (MEDIA_QSPI3_IDX))
	#error "QSPI #2 drive number is the same as QSPI #3"
  #endif
  #if ((MEDIA_QSPI2_IDX) == (MEDIA_MDRV_IDX))
	#error "QSPI #2 drive number is the same as memory drive"
  #endif
#endif
#if ((MEDIA_QSPI3_IDX) >= 0)
  #if ((MEDIA_QSPI3_IDX) == (MEDIA_MDRV_IDX))
	#error "QSPI #3 drive number is the same as memory drive"
  #endif
#endif

#define MEDIA_SDMMC_NMB_DRV	( ((MEDIA_SDMMC0_IDX) >=0)												\
							+ ((MEDIA_SDMMC1_IDX) >=0))

#define MEDIA_QSPI_NMB_DRV	( ((MEDIA_QSPI0_IDX)  >=0)												\
							+ ((MEDIA_QSPI1_IDX)  >=0)												\
							+ ((MEDIA_QSPI2_IDX)  >=0)												\
							+ ((MEDIA_QSPI3_IDX)  >=0))

#define MEDIA_NMB_DRV		(  (MEDIA_SDMMC_NMB_DRV)												\
							+  (MEDIA_QSPI_NMB_DRV)													\
							+ ((MEDIA_MDRV_IDX) >= 0))


#if ((MEDIA_NMB_DRV) == 0)
  #error "No media drive defined / selected"
#endif
#if ((MEDIA_SDMMC0_IDX) >= (MEDIA_NMB_DRV))
  #error "SDMMC #0 drive number is out of range. Drive numbers must be contiguous starting at 0"
#endif
#if ((MEDIA_SDMMC1_IDX) >= (MEDIA_NMB_DRV))
  #error "SDMMC #1 drive number is out of range. Drive numbers must be contiguous starting at 0"
#endif
#if ((MEDIA_QSPI0_IDX)  >= (MEDIA_NMB_DRV))
  #error "QSPI #0 drive number is out of range. Drive numbers must be contiguous starting at 0"
#endif
#if ((MEDIA_QSPI1_IDX)  >= (MEDIA_NMB_DRV))
  #error "QSPI #1 drive number is out of range. Drive numbers must be contiguous starting at 0"
#endif
#if ((MEDIA_QSPI2_IDX)  >= (MEDIA_NMB_DRV))
  #error "QSPI #2 drive number is out of range. Drive numbers must be contiguous starting at 0"
#endif
#if ((MEDIA_QSPI3_IDX)  >= (MEDIA_NMB_DRV))
  #error "QSPI #3 drive number is out of range. Drive numbers must be contiguous starting at 0"
#endif
#if ((MEDIA_MDRV_IDX) >= (MEDIA_NMB_DRV))
  #error "Memory drive number is out of range. Drive numbers must be contiguous starting at 0"
#endif

#if ((MEDIA_SDMMC0_START) < 0)
  #error "MEDIA_SDMMC0_START must be >= 0"
#endif
#if ((MEDIA_SDMMC1_START) < 0)
  #error "MEDIA_SDMMC1_START must be >= 0"
#endif
#if ((MEDIA_QSPI0_START) < 0)
  #error "MEDIA_QSPI0_START must be >= 0"
#endif
#if ((MEDIA_QSPI1_START) < 0)
  #error "MEDIA_QSPI1_START must be >= 0"
#endif
#if ((MEDIA_QSPI2_START) < 0)
  #error "MEDIA_QSPI2_START must be >= 0"
#endif
#if ((MEDIA_QSPI3_START) < 0)
  #error "MEDIA_QSPI3_START must be >= 0"
#endif
#if ((MEDIA_SDMMC0_SIZE) < 0)
  #error "MEDIA_SDMMC0_SIZE must be >= 0"
#endif
#if ((MEDIA_SDMMC1_SIZE) < 0)
  #error "MEDIA_SDMMC1_SIZE must be >= 0"
#endif
#if ((MEDIA_QSPI0_SIZE) < 0)
  #error "MEDIA_QSPI0_SIZE must be >= 0"
#endif
#if ((MEDIA_QSPI1_SIZE) < 0)
  #error "MEDIA_QSPI1_SIZE must be >= 0"
#endif
#if ((MEDIA_QSPI2_SIZE) < 0)
  #error "MEDIA_QSPI2_SIZE must be >= 0"
#endif
#if ((MEDIA_QSPI3_SIZE) < 0)
  #error "MEDIA_QSPI3_SIZE must be >= 0"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Uncached memory for the DMA (Must be at least 512 bytes and N*512)    							*/
/* The bigger the buffer the more efficient are the transfers            							*/

#if (((MEDIA_SDMMC_NMB_DRV) > 0)																	\
 &&  ((SDMMC_BUFFER_TYPE) == (SDMMC_BUFFER_UNCACHED)))
 #if (defined(__CC_ARM) || defined(__GNUC__))
  #define DMA_ATTRIB __attribute__ ((aligned (8))) __attribute__ ((section (".uncached")))
 #else
  #define DMA_ATTRIB
 #endif
  static char g_DMAbuff[MEDIA_SDMMC_NMB_DRV][4096] DMA_ATTRIB;
#endif
													/* Using 1 buf per QSPI in case not protected	*/
#if ((MEDIA_QSPI_NMB_DRV) > 0)
  static uint8_t g_QspiBuf[MEDIA_QSPI_NMB_DRV][MEDIA_QSPI_SECT_BUF];
 #if ((MEDIA_QSPI_CHK_WRT) > 0)
  static uint8_t g_QspiChkBuf[MEDIA_QSPI_NMB_DRV][MEDIA_QSPI_SECT_BUF];
 #endif
#endif

#if ((MEDIA_MDRV_IDX) >= 0)
 #if ((MEDIA_MDRV_SIZE) > 0)						/* Use local buffer for the memory drive		*/
   static uint8_t   G_MemDrvBase[MEDIA_MDRV_SIZE];	/* Import symbols for the Memory Drive			*/
 #else
  #if defined(__CC_ARM)
   extern uint8_t   Image$$G_MemDrvBase$$Base;		/* Import symbols for the Memory Drive			*/
   extern uint8_t   Image$$G_MemDrvEnd$$Base;
   static uint8_t  *G_MemDrvBase;
   static uint8_t  *G_MemDrvEnd;
  #else
   extern uint8_t   G_MemDrvBase[];					/* Import symbols for the Memory Drive			*/
   extern uint8_t   G_MemDrvEnd[];
  #endif
 #endif
   static volatile uint32_t g_MemDrvSize;
#endif

													/* [0] sector size in use						*/
static int g_MediaIsOK[MEDIA_NMB_DRV][2] =  {{0,0}	/* [1] requested sector size to use				*/
                                           #if ((MEDIA_NMB_DRV) > 1)
                                            ,{0,0}
                                           #endif
                                           #if ((MEDIA_NMB_DRV) > 2)
                                            ,{0,0}
                                           #endif
                                           #if ((MEDIA_NMB_DRV) > 3)
                                            ,{0,0}
                                           #endif
                                           #if ((MEDIA_NMB_DRV) > 4)
                                            ,{0,0}
                                           #endif
                                           #if ((MEDIA_NMB_DRV) > 5)
                                            ,{0,0}
                                           #endif
                                           #if ((MEDIA_NMB_DRV) > 6)
                                            ,{0,0}
                                           #endif
                                           #if ((MEDIA_NMB_DRV) > 7)
                                            ,{0,0}
                                           #endif
                                           #if ((MEDIA_NMB_DRV) > 8)
                                            ,{0,0}
                                           #endif
                                           #if ((MEDIA_NMB_DRV) > 9)
                                            ,{0,0}
                                           #endif
                                           #if ((MEDIA_NMB_DRV) > 10)
                                            ,{0,0}
                                           #endif
                                            };

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    MEDIAstatus																			*/
/*																									*/
/* MEDIAstatus - report if the media can be accessed												*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int MEDIAstatus(const char *FSname, int Drv);												*/
/*																									*/
/* ARGUMENTS:																						*/
/*		FSname : name of the file system stack calling MEDIAstatus()								*/
/*		Drv    : physical drive # to check															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : media is OK																			*/
/*		!= 0 : media is not OK																		*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int MEDIAstatus(const char *FSname, int Drv)
{
int Ret;
#if (((MEDIA_SDMMC_NMB_DRV) > 0) || ((MEDIA_QSPI_NMB_DRV) > 0))
  int   Dev;										/* Device number used by the media driver		*/
#endif

  #if ((MEDIA_ARG_CHECK) != 0)
	if ((Drv < 0)									/* Check valid range for the drive #			*/
	||  (Drv > (MEDIA_NMB_DRV))) {
	  #if ((MEDIA_DEBUG) > 0)
		printf("MEDIA [Drv:%d] %s  - Error - status drive # out of range\n", Drv, FSname);
	  #endif
		return(-1);
	}
  #endif

	Ret = -1;										/* Assume invalid drive #						*/
													/* -------------------------------------------- */
  #if ((MEDIA_SDMMC_NMB_DRV) > 0)
	Dev = -1;										/* Assume not a SDMMC drive						*/
   #if ((MEDIA_SDMMC0_IDX) >= 0)					/* MEDIA_SDMMC0 is supported					*/
	if (Drv == (MEDIA_SDMMC0_IDX)) {				/* Check if the drive is MEDIA_SDMMC0			*/
		Dev = MEDIA_SDMMC0_DEV;						/* This is MEDIA_SDMMC0 device #				*/
	}
   #endif
   #if ((MEDIA_SDMMC1_IDX) >= 0)					/* MEDIA_SDMMC1 is supported					*/
	if (Drv == (MEDIA_SDMMC1_IDX)) {				/* Check if the drive is the MEDIA_SDMMC1		*/
		Dev = MEDIA_SDMMC1_DEV;						/* This is MEDIA_SDMMC1 device #				*/
	}
   #endif

	if (Dev >= 0) {									/* If this is a SDMMC device					*/
		Ret = (g_MediaIsOK[Drv][0] <= 0);			/* Needs to have been initialized				*/
	}
  #endif
													/* -------------------------------------------- */
  #if ((MEDIA_QSPI_NMB_DRV) > 0)
	Dev = -1;										/* Assume not a QSPI drive						*/
   #if ((MEDIA_QSPI0_IDX) >= 0)						/* MEDIA_QSPI0 is supported						*/
	if (Drv == (MEDIA_QSPI0_IDX)) {					/* Check if the drive is MEDIA_QSPI0			*/
		Dev = MEDIA_QSPI0_DEV;						/* This is MEDIA_QSPI0 device #					*/
	}
   #endif
   #if ((MEDIA_QSPI1_IDX) >= 0)						/* MEDIA_QSPI1 is supported						*/
	if (Drv == (MEDIA_QSPI1_IDX)) {					/* Check if the drive is the MEDIA_QSPI1		*/
		Dev = MEDIA_QSPI1_DEV;						/* This is MEDIA_QSPI1 device #					*/
	}
   #endif
   #if ((MEDIA_QSPI2_IDX) >= 0)						/* MEDIA_QSPI2 is supported						*/
	if (Drv == (MEDIA_QSPI2_IDX)) {					/* Check if the drive is the MEDIA_QSPI2		*/
		Dev = MEDIA_QSPI2_DEV;						/* This is MEDIA_QSPI2 device #					*/
	}
   #endif
   #if ((MEDIA_QSPI3_IDX) >= 0)						/* MEDIA_QSPI3 is supported						*/
	if (Drv == (MEDIA_QSPI3_IDX)) {					/* Check if the drive is the MEDIA_QSPI3		*/
		Dev = MEDIA_QSPI3_DEV;						/* This is MEDIA_QSPI3 device #					*/
	}
   #endif

	if (Dev >= 0) {									/* If this is a QSPI device						*/
		Ret = (g_MediaIsOK[Drv][0] <= 0);			/* Needs to have been initialized				*/
	}
  #endif
													/* -------------------------------------------- */
  #if ((MEDIA_MDRV_IDX) >= 0)						/* Memory media is always ready					*/
	if (Drv == (MEDIA_MDRV_IDX)) {					/* Check if the drive is the Memory Drive		*/
		Ret = (g_MediaIsOK[Drv][0] <= 0);			/* Needs to have been initialized				*/
	}
  #endif

  #if ((MEDIA_DEBUG) > 1)
	printf("MEDIA [Drv:%d] %s - status result:%s\n", (int)Drv, FSname, (Ret == 0) ? "OK" : "ERROR");
  #endif

	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    MEDIAIinit																			*/
/*																									*/
/* MEDIAinit - initialize a drive	 																*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int MEDIAinit(const char *FSname, int Drv);													*/
/*																									*/
/* ARGUMENTS:																						*/
/*		FSname : name of the file system stack calling MEDIAinit()									*/
/*		Drv    : physical drive # to access															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : media has been initialized and is accessible											*/
/*		!= 0 : media is not accessible																*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int MEDIAinit(const char *FSname, int Drv)
{
int      Ret;
#if (((MEDIA_SDMMC_NMB_DRV) > 0) || ((MEDIA_QSPI_NMB_DRV) > 0))
  int      Dev;										/* Device number used by the media driver		*/
  int      SectSz;									/* Media sector size							*/
  uint64_t Start;									/* Start address when using partial media		*/
#endif
  #if ((MEDIA_QSPI_NMB_DRV) > 0)
  int   Slv;										/* QSPI slave number							*/
#endif

  #if ((MEDIA_ARG_CHECK) != 0)
	if ((Drv < 0)									/* Check valid range for the drive #			*/
	||  (Drv > (MEDIA_NMB_DRV))) {
	  #if ((MEDIA_DEBUG) > 0)
		printf("MEDIA [Drv:%d] %s - Error - init drive # out of range\n", Drv, FSname);
	  #endif
		return(-1);
	}
  #endif

	Ret = -1;										/* Assume failure								*/
													/* -------------------------------------------- */
  #if ((MEDIA_SDMMC_NMB_DRV) > 0)
	Dev = -1;										/* Assume not a SDMMC drive						*/
   #if ((MEDIA_SDMMC0_IDX) >= 0)					/* MEDIA_SDMMC0 is supported					*/
	if (Drv == (MEDIA_SDMMC0_IDX)) {				/* Check if the drive is MEDIA_SDMMC0			*/
		Dev    = MEDIA_SDMMC0_DEV;					/* This is MEDIA_SDMMC0 device #				*/
		SectSz = MEDIA_SDMMC0_SECT_SZ;				/* Sector size to use							*/
		Start  = 512LL * (MEDIA_SDMMC0_START);
	}
   #endif
   #if ((MEDIA_SDMMC1_IDX) >= 0)					/* MEDIA_SDMMC1 is supported					*/
	if (Drv == (MEDIA_SDMMC1_IDX)) {				/* Check if the drive is the MEDIA_SDMMC1		*/
		Dev    = MEDIA_SDMMC1_DEV;					/* This is MEDIA_SDMMC1 device #				*/
		SectSz = MEDIA_SDMMC1_SECT_SZ;				/* Sector size to use							*/
		Start  = 512LL * (MEDIA_SDMMC1_START);
	}
   #endif

	if (Dev >= 0) {									/* If this is a SDMMC device					*/
		if (0 == mmc_init(Dev)) {					/* Init and see what happen						*/
			g_MediaIsOK[Drv][0] = 0;				/* Assume sector size out of range				*/
		  	if (SectSz <= 0) {						/* Auto sector selection						*/
				SectSz = mmc_blklen(Dev);			/* Get the SDMMC block size						*/
			}
			if (g_MediaIsOK[Drv][1] > 0) {			/* If the media size is fixed					*/
				SectSz = g_MediaIsOK[Drv][1];		/* g_MediaIsOK[][1] is set with MEDIAsectsz()	*/
			}
			if (SectSz > 0) {
				g_MediaIsOK[Drv][0] = SectSz;
				Ret                 = 0;			/* We are go									*/
			}
		  #if ((MEDIA_DEBUG) > 0)
			else {
				printf("MEDIA [Drv:%d] %s - Error - SD/MMC sector size out of range\n",
				        (int)Drv, FSname);
			}
		  #endif
			if (Start >= mmc_capacity(Dev)) {		/* If start address > device size, can't		*/
				Ret = -2;							/* use it, report initialization failure		*/
			  #if ((MEDIA_DEBUG) > 0)
				printf("MEDIA [Drv:%d] %s - Error - MEDIA_SDMMC%d_START > device size\n",
				        (int)Drv, FSname, (Drv == (MEDIA_SDMMC0_IDX)) ? 0 : 1);
			  #endif
			}
		}
	  #if ((MEDIA_DEBUG) > 0)
		else {
			printf("MEDIA [Drv:%d] %s - Error - SD/MMC controller init failed\n",
			       (int)Drv, FSname);
		}
	  #endif
	}
  #endif
													/* -------------------------------------------- */
  #if ((MEDIA_QSPI_NMB_DRV) > 0)
	Dev = -1;										/* Assume not a QSPI drive						*/
   #if ((MEDIA_QSPI0_IDX) >= 0)						/* MEDIA_QSPI0 is supported						*/
	if (Drv == (MEDIA_QSPI0_IDX)) {					/* Check if the drive is MEDIA_QSPI0			*/
		Dev    = MEDIA_QSPI0_DEV;					/* This is MEDIA_QSPI0 device #					*/
		SectSz = MEDIA_QSPI0_SECT_SZ;				/* Sector size to use							*/
		Slv    = MEDIA_QSPI0_SLV;					/* This is MEDIA_QSPI0 slave #					*/
		Start  = 512LL * (MEDIA_QSPI0_START);
	}
   #endif
   #if ((MEDIA_QSPI1_IDX) >= 0)						/* MEDIA_QSPI1 is supported						*/
	if (Drv == (MEDIA_QSPI1_IDX)) {					/* Check if the drive is the MEDIA_QSPI1		*/
		Dev    = MEDIA_QSPI1_DEV;					/* This is MEDIA_QSPI1 device #					*/
		SectSz = MEDIA_QSPI1_SECT_SZ;				/* Sector size to use							*/
		Slv    = MEDIA_QSPI1_SLV;					/* This is MEDIA_QSPI1 device #					*/
		Start  = 512LL * (MEDIA_QSPI1_START);
	}
   #endif
   #if ((MEDIA_QSPI2_IDX) >= 0)						/* MEDIA_QSPI2 is supported						*/
	if (Drv == (MEDIA_QSPI2_IDX)) {					/* Check if the drive is the MEDIA_QSPI2		*/
		Dev    = MEDIA_QSPI2_DEV;					/* This is MEDIA_QSPI2 device #					*/
		SectSz = MEDIA_QSPI2_SECT_SZ;				/* Sector size to use							*/
		Slv    = MEDIA_QSPI2_SLV;					/* This is MEDIA_QSPI2 device #					*/
		Start  = 512LL * (MEDIA_QSPI2_START);
	}
   #endif
   #if ((MEDIA_QSPI3_IDX) >= 0)						/* MEDIA_QSPI3 is supported						*/
	if (Drv == (MEDIA_QSPI3_IDX)) {					/* Check if the drive is the MEDIA_QSPI2		*/
		Dev    = MEDIA_QSPI3_DEV;					/* This is MEDIA_QSPI3 device #					*/
		SectSz = MEDIA_QSPI3_SECT_SZ;				/* Sector size to use							*/
		Slv    = MEDIA_QSPI3_SLV;					/* This is MEDIA_QSPI3 device #					*/
		Start  = 512LL * (MEDIA_QSPI3_START);
	}
   #endif

	if (Dev >= 0) {									/* If this is a QSPI device						*/
		if (0 == qspi_init(Dev, Slv, 0)) {
			g_MediaIsOK[Drv][0] = 0;				/* Assume sector size out of range				*/
		  	if (SectSz <= 0) {						/* Auto sector selection						*/
				SectSz = qspi_blksize(Dev, Slv);	/* Get the QSPI block size						*/
				if (SectSz < 512) {					/* Some QSPI have 256 byte sectors				*/
					SectSz = 512;
				}
			}
			if (g_MediaIsOK[Drv][1] > 0) {			/* If the media size is fixed					*/
				SectSz = g_MediaIsOK[Drv][1];		/* g_MediaIsOK[][1] is set with MEDIAsectsz()	*/
			}
			if (SectSz > 0) {
				if (qspi_blksize(Dev, Slv) <= sizeof(g_QspiBuf[0])) {
					g_MediaIsOK[Drv][0] = SectSz;
					Ret                 = 0;		/* We are go									*/
				}
			   #if ((MEDIA_DEBUG) > 0)
				else {
					printf("MEDIA [Drv:%d] %s - Error - QSPI temp buffer too small\n",
					       (int)Drv, FSname);
				}
			   #endif
				if (Start >= qspi_size(Dev, Slv)) {	/* If start address > device size, can't		*/
					Ret = -2;						/* use it, report initialization failure		*/
			  #if ((MEDIA_DEBUG) > 0)
				printf("MEDIA [Drv:%d] %s - Error - MEDIA_QSPI%d_START > device size\n",
				        (int)Drv, FSname, (Drv == (MEDIA_QSPI0_IDX)) ? 0
				                          : ((Drv == (MEDIA_QSPI2_IDX)) ? 1
				                          : ((Drv == (MEDIA_QSPI2_IDX)) ? 2 : 3)));
			  #endif
				}
			}
		  #if ((MEDIA_DEBUG) > 0)
			else {
				printf("MEDIA [Drv:%d] %s - Error - QSPI sector size out of range\n",
				       (int)Drv, FSname);
			}
		  #endif
		}
	  #if ((MEDIA_DEBUG) > 0)
		else {
			printf("MEDIA [Drv:%d] %s - Error - QSPI controller init failed\n",
			       (int)Drv, FSname);
		}
	  #endif
	}
  #endif
													/* -------------------------------------------- */
   #if ((MEDIA_MDRV_IDX) >= 0)						/* Memory drive is supported					*/
	if (Drv ==( MEDIA_MDRV_IDX)) {					/* Check if the drive is the Memory Drive		*/
	  #if defined(__CC_ARM)
		G_MemDrvBase = (uint8_t *)&Image$$G_MemDrvBase$$Base;
 		G_MemDrvEnd  = (uint8_t *)&Image$$G_MemDrvEnd$$Base;
	  #endif

	  #if ((MEDIA_MDRV_SIZE) > 0)
		g_MemDrvSize = MEDIA_MDRV_SIZE;
	  #else
		g_MemDrvSize = (uint32_t) (((uintptr_t)&G_MemDrvEnd[0])
		             -             ((uintptr_t)&G_MemDrvBase[0]));
	  #endif

		g_MediaIsOK[Drv][0] = (g_MediaIsOK[Drv][1] > 0)
		                    ?  g_MediaIsOK[Drv][1]	/* If the sector has been fixed, use that		*/
		                    :  512;					/* otherwise always use 512 byte sectors		*/
		Ret                 = 0;					/* We are go									*/
	}
   #endif

  #if ((MEDIA_DEBUG) > 1)
	printf("MEDIA [Drv:%d] %s - initialize result:%s (Sector size:%d)\n", (int)Drv, FSname,
	       (Ret == 0) ? "OK" : "INIT FAILED", g_MediaIsOK[Drv][0]);
	                                                           
  #endif

	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    MEDIAread																			*/
/*																									*/
/* MEDIAread - read data from the physical device													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int MEDIAread(const char *FSname, int Drv, uint8_t *Buff, uint32_t Sect, int Count);		*/
/*																									*/
/* ARGUMENTS:																						*/
/*		FSname : name of the file system stack calling MEDIAread()									*/
/*		Drv    : physical drive # to access															*/
/*		Buff   : buffer to fill with contents from the drive										*/
/*		Sect   : number of the first sector to read from											*/
/*		Count  : number of sectors to read															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int MEDIAread(const char *FSname, int Drv, uint8_t *Buff, uint32_t Sect, int Count)
{
uint64_t   Addr64;									/* Address to write to (SDMMC sizes are > 4 G	*/
 uint8_t  *Buffer;									/* Pointer in buf to read from					*/
uint32_t   Nbyte;									/* Number of bytes to write						*/
int        Retry;									/* When write failure, will init and try again	*/
#if (((MEDIA_SDMMC_NMB_DRV) > 0) &&  ((SDMMC_BUFFER_TYPE) == (SDMMC_BUFFER_UNCACHED)))
  uint32_t Size;									/* Number of bytes to read with non-DMA buffer	*/
  int      DMAbufIdx;								/* Index in the DMA buffer						*/
#endif
#if (((MEDIA_SDMMC_NMB_DRV) > 0) || ((MEDIA_QSPI_NMB_DRV) > 0))
  int      Dev;										/* Device number used by the media driver		*/
#endif
#if ((MEDIA_QSPI_NMB_DRV) > 0)
  int      Slv;										/* QSPI slave number							*/
#endif
#if (((MEDIA_QSPI_NMB_DRV) > 0) || ((MEDIA_MDRV_IDX) >= 0))
  uint32_t Addr;									/* Address to write to							*/
#endif

  #if ((MEDIA_DEBUG) > 1)
	printf("MEDIA [Drv:%d] %s - read sector:#%lu count:%d\n", (int)Drv, FSname, (unsigned long)Sect,
	       (int)Count);
  #endif

  #if ((MEDIA_ARG_CHECK) != 0)
	if ((Drv < 0)									/* Check valid range for the drive #			*/
	||  (Drv > (MEDIA_NMB_DRV))) {
	  #if ((MEDIA_DEBUG) > 0)
		printf("MEDIA [Drv:%d] %s - Error - read drive # out of range\n", Drv, FSname);
	  #endif
		return(-1);
	}
  #endif

	Retry = 0;
	do {
		Addr64 = (int64_t)Sect						/* g_MediaIsOK[Drv] is the sector size that		*/
		       * (int64_t)g_MediaIsOK[Drv][0];		/* has been reported to the FAT file system		*/
	  #if (((MEDIA_QSPI_NMB_DRV) > 0) || ((MEDIA_MDRV_IDX) >= 0))
		Addr   = (int32_t)Addr64;
	  #endif
		Nbyte  = Count
		       * g_MediaIsOK[Drv][0];
		Buffer = (uint8_t *)Buff;					/* Preserve in case we have to retry			*/
													/* -------------------------------------------- */
	  #if ((MEDIA_SDMMC_NMB_DRV) > 0)
		Dev = -1;									/* Assume not a SDMMC drive						*/
	   #if ((MEDIA_SDMMC0_IDX) >= 0)				/* MEDIA_SDMMC0 is supported					*/
		if (Drv == (MEDIA_SDMMC0_IDX)) {			/* Check if the drive is MEDIA_SDMMC0			*/
			Dev     = MEDIA_SDMMC0_DEV;				/* This is MEDIA_SDMMC0 device #				*/
			Addr64 += 512LL * (MEDIA_SDMMC0_START);	/* Take into account start sector #				*/
		  #if ((SDMMC_BUFFER_TYPE) == (SDMMC_BUFFER_UNCACHED))
			DMAbufIdx = 0;
		  #endif
		}
	   #endif
	   #if ((MEDIA_SDMMC1_IDX) >= 0)				/* MEDIA_SDMMC1 is supported					*/
		if (Drv == (MEDIA_SDMMC1_IDX)) {			/* Check if the drive is the MEDIA_SDMMC1		*/
			Dev     = MEDIA_SDMMC1_DEV;				/* This is MEDIA_SDMMC1 device #				*/
			Addr64 += 512LL * (MEDIA_SDMMC1_START);	/* Take into account start sector #				*/
		  #if ((SDMMC_BUFFER_TYPE) == (SDMMC_BUFFER_UNCACHED))
			DMAbufIdx = ((MEDIA_SDMMC0_IDX) >= 0);
		  #endif
		}
	   #endif

		if ((Dev >= 0)								/* If this is a SDMMC device					*/
		&&  (g_MediaIsOK[Drv][0] != 0)) {			/* If the device has been initialized			*/
	  	  #if ((SDMMC_BUFFER_TYPE) != (SDMMC_BUFFER_UNCACHED))
			if (0 == mmc_read(Dev, (uint32_t)(Addr64 >> 9), (void *)Buffer, Nbyte >> 9)) {
			  #if ((MEDIA_DEBUG) > 0)
				if (Retry != 0) {
					printf("MEDIA [Drv:%d] %s           Read successful after re-init\n",
					      (int)Drv, FSname);
				}
			  #endif
			  #if ((MEDIA_DEBUG) > 1)
				printf("MEDIA [Drv:%d] %s - read result OK\n", (int)Drv, FSname);
			  #endif
				return(0);
			}
		  #else
			do {
				Size = Nbyte;
				if (Size > sizeof(g_DMAbuff[0])) {	/* If too big for the DMA buffer				*/
					Size = sizeof(g_DMAbuff[0]);	/* Don't read more than DMA buff holds			*/
				}
					if (0 != mmc_read(Dev, (uint32_t)(Addr64 >> 9), (void *)&g_DMAbuff[DMAbufIdx][0],
					                  Size >> 9)) {
					break;
				}
				else {
					memmove(&Buffer[0], &g_DMAbuff[DMAbufIdx][0], Size);
				}

				Addr64 += Size;
				Buffer += Size;
				Nbyte  -= Size;
			} while (Nbyte != 0);

			if (Nbyte == 0) {
			  #if ((MEDIA_DEBUG) > 0)
				if (Retry != 0) {
					printf("MEDIA [Drv:%d] %s           Read successful after re-init\n",
					       (int)Drv, FSname);
				}
			  #endif
			  #if ((MEDIA_DEBUG) > 1)
				printf("MEDIA [Drv:%d] %s - read result OK\n", (int)Drv, FSname);
			  #endif
				return(0);
			}
		  #endif
		}
	  #endif
													/* -------------------------------------------- */
	  #if ((MEDIA_QSPI_NMB_DRV) > 0)
		Dev = -1;									/* Assume not a QSPI drive						*/
	   #if ((MEDIA_QSPI0_IDX) >= 0)					/* MEDIA_QSPI0 is supported						*/
		if (Drv == (MEDIA_QSPI0_IDX)) {				/* Check if the drive is MEDIA_QSPI0			*/
			Dev   = MEDIA_QSPI0_DEV;				/* This is MEDIA_QSPI0 device #					*/
			Slv   = MEDIA_QSPI0_SLV;				/* This is MEDIA_QSPI0 slave  #					*/
			Addr += 512*(MEDIA_QSPI0_START);		/* Take into account start sector #				*/
		}
	   #endif
	   #if ((MEDIA_QSPI1_IDX) >= 0)					/* MEDIA_QSPI1 is supported						*/
		if (Drv == (MEDIA_QSPI1_IDX)) {				/* Check if the drive is the MEDIA_QSPI1		*/
			Dev   = MEDIA_QSPI1_DEV;				/* This is MEDIA_QSPI1 device #					*/
			Slv   = MEDIA_QSPI1_SLV;				/* This is MEDIA_QSPI1 slave  #					*/
			Addr += 512*(MEDIA_QSPI1_START);		/* Take into account start sector #				*/
		}
	   #endif
	   #if ((MEDIA_QSPI2_IDX) >= 0)					/* MEDIA_QSPI2 is supported						*/
		if (Drv == (MEDIA_QSPI2_IDX)) {				/* Check if the drive is the MEDIA_QSPI2		*/
			Dev   = MEDIA_QSPI2_DEV;				/* This is MEDIA_QSPI2 device #					*/
			Slv   = MEDIA_QSPI2_SLV;				/* This is MEDIA_QSPI2 slave  #					*/
			Addr += 512*(MEDIA_QSPI2_START);		/* Take into account start sector #				*/
		}
	   #endif
	   #if ((MEDIA_QSPI3_IDX) >= 0)					/* MEDIA_QSPI3 is supported						*/
		if (Drv == (MEDIA_QSPI3_IDX)) {				/* Check if the drive is the MEDIA_QSPI2		*/
			Dev   = MEDIA_QSPI3_DEV;				/* This is MEDIA_QSPI3 device #					*/
			Slv   = MEDIA_QSPI3_SLV;				/* This is MEDIA_QSPI3 slave  #					*/
			Addr += 512*(MEDIA_QSPI3_START);		/* Take into account start sector #				*/
		}
	   #endif
		if ((Dev >= 0)								/* If this is a QSPI device						*/
		&&  (g_MediaIsOK[Drv][0] != 0)) {			/* If the device has been initialized			*/
			if (0 == qspi_read(Dev, Slv, Addr, &Buffer[0], Nbyte) ) {
			  #if ((MEDIA_DEBUG) > 0)
				if (Retry != 0) {
					printf("MEDIA [Drv:%d] %s           Read successful after re-init\n",
					       (int)Drv, FSname);
				}
			  #endif
			  #if ((MEDIA_DEBUG) > 1)
				printf("MEDIA [Drv:%d] %s - read result OK\n", (int)Drv, FSname);
			  #endif
				return(0);
			}
		}
	  #endif

													/* -------------------------------------------- */
	  #if ((MEDIA_MDRV_IDX) >= 0)
		if ((Drv == (MEDIA_MDRV_IDX))				/* Check if the drive is the Memory Drive		*/
		&&  (g_MediaIsOK[Drv][0] != 0)) {
			memmove(&Buffer[0], &G_MemDrvBase[Addr], Nbyte);
		  #if ((MEDIA_DEBUG) > 1)
			printf("MEDIA [Drv:%d] %s - read result OK\n", (int)Drv, FSname);
		  #endif
			return(0);
		}
	  #endif

	  #if ((MEDIA_DEBUG) > 0)
		printf("MEDIA [Drv:%d] %s - Error - Read failure, re-initializing the media controller\n",
		       (int)Drv, FSname);
	  #endif

		MEDIAinit(FSname, Drv);						/* Has to retry, re-init in case it helps		*/

	} while (Retry++ < 1);							/* Retry once									*/

  #if ((MEDIA_DEBUG) > 1)
	printf("MEDIA [Drv:%d] %s - read result ERROR\n", (int)Drv, FSname);
  #endif

	return(-1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    MEDIAwrite																			*/
/*																									*/
/* MEDIAwrite - write data to the physical device													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int MEDIAwrite(const char *FSname, int Drv, const uint8_t *Buff, uint32_t Sect, int Count);	*/
/*																									*/
/* ARGUMENTS:																						*/
/*		FSname : name of the file system stack calling MEDIAwrite()									*/
/*		Drv    : physical drive to access															*/
/*		Buff   : buffer holding the data to write on the drive										*/
/*		Sect   : number of the first sector to write to												*/
/*		Count  : number of sectors to write															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int MEDIAwrite(const char *FSname, int Drv, const uint8_t *Buff, uint32_t sector, int Count)
{
uint64_t       Addr64;								/* Address to write to (SDMMC sizes are > 4 G	*/
const uint8_t *Buffer;								/* Pointer in buf to read from					*/
uint32_t       Nbyte;								/* Number of bytes to write						*/
int            Retry;								/* When write failure, will init and try again	*/
#if (((MEDIA_SDMMC_NMB_DRV) > 0) &&  ((SDMMC_BUFFER_TYPE) == (SDMMC_BUFFER_UNCACHED)))
  uint32_t     Size;								/* Number of bytes to write with non-DMA buffer	*/
  int      DMAbufIdx;								/* Index in the DMA buffer						*/
#endif
#if (((MEDIA_SDMMC_NMB_DRV) > 0) || ((MEDIA_QSPI_NMB_DRV) > 0))
  int          Dev;									/* Device number used by the media driver		*/
#endif
  #if ((MEDIA_QSPI_NMB_DRV) > 0)
  int          BlkSize;								/* QSPI sector size								*/
  int          Err;									/* Error tracking								*/
  int          Slv;									/* QSPI target slave number						*/
  int          BtmpIdx;								/* Index #0 for the temp buffer					*/
  int          WrtSz;								/* Number of bytes to write						*/
 #if ((MEDIA_QSPI_OPT_WRT) != 0)
  uint8_t      Data;								/* Data byte read from QSPI						*/
  int          ChkSize;								/* # of bytes to check per pass (typical 512)	*/
  int          Erase;								/* If we need to erase the QSPI					*/
  int          GotDiff;								/* If got a difference between Data & QSPI		*/
  int          jj;									/* General purpose								*/
  int          kk;									/* General purpose								*/
  int          SkipEnd;								/* Number of contiguous Data==QSPI at the end	*/
  int          SkipStart;							/* Number of contiguous Data==QSPI at start		*/
  uint8_t      BufChk[512];							/* Don't use static in case not protected		*/
 #endif
 #if ((MEDIA_QSPI_CHK_WRT) > 0)
  int          ChkRetry;							/* When checking if write OK, # of retries		*/
  int          WrtBad;								/* If read-back on write is OK					*/
 #endif
#endif
#if (((MEDIA_QSPI_NMB_DRV) > 0) || ((MEDIA_MDRV_IDX) >= 0))
  uint32_t     Addr;								/* Address to write to							*/
#endif


  #if ((MEDIA_DEBUG) > 1)
	printf("MEDIA [Drv:%d] %s - write sector:#%lu count:%u\n", (int)Drv, FSname,
	        (unsigned long)sector, (unsigned int)Count);
  #endif

  #if ((MEDIA_ARG_CHECK) != 0)
	if ((Drv < 0)									/* Check valid range for the drive #			*/
	||  (Drv > (MEDIA_NMB_DRV))) {
	  #if ((MEDIA_DEBUG) > 0)
		printf("MEDIA [Drv:%d] %s - Error - read drive # out of range\n", Drv, FSname);
	  #endif
		return(-1);
	}
  #endif

	Retry = 0;
	do {
		Addr64 = (int64_t)sector					/* g_MediaIsOK[Drv] is the sector size that		*/
		       * (int64_t)g_MediaIsOK[Drv][0];		/* has been reported to the FAT file system		*/
	  #if (((MEDIA_QSPI_NMB_DRV) > 0) || ((MEDIA_MDRV_IDX) >= 0))
		Addr   = (int32_t)Addr64;
	  #endif
		Nbyte  = Count
		       * g_MediaIsOK[Drv][0];
		Buffer = (uint8_t *)Buff;					/* Preserve in case we have to retry			*/
													/* -------------------------------------------- */
	  #if ((MEDIA_SDMMC_NMB_DRV) > 0)
		Dev = -1;									/* Assume not a SDMMC drive						*/
	   #if ((MEDIA_SDMMC0_IDX) >= 0)				/* MEDIA_SDMMC0 is supported					*/
		if (Drv ==( MEDIA_SDMMC0_IDX)) {			/* Check if the drive is MEDIA_SDMMC0			*/
			Dev     = MEDIA_SDMMC0_DEV;				/* This is MEDIA_SDMMC0 device #				*/
			Addr64 += 512LL * (MEDIA_SDMMC0_START);	/* Take into account start sector #				*/
		  #if ((SDMMC_BUFFER_TYPE) == (SDMMC_BUFFER_UNCACHED))
			DMAbufIdx = 0;
	      #endif
		}
	   #endif
	   #if ((MEDIA_SDMMC1_IDX) >= 0)				/* MEDIA_SDMMC1 is supported					*/
		if (Drv == (MEDIA_SDMMC1_IDX)) {			/* Check if the drive is the MEDIA_SDMMC1		*/
			Dev     = MEDIA_SDMMC1_DEV;				/* This is MEDIA_SDMMC1 device #				*/
			Addr64 += 512LL * (MEDIA_SDMMC1_START);	/* Take into account start sector #				*/
		  #if ((SDMMC_BUFFER_TYPE) == (SDMMC_BUFFER_UNCACHED))
			DMAbufIdx = ((MEDIA_SDMMC0_IDX) >= 0);
		  #endif
		}
	   #endif

		if ((Dev >= 0)								/* If this is a SDMMC device					*/
		&&  (g_MediaIsOK[Drv][0] != 0)) {			/* If the device has been initialized			*/
	  	  #if ((SDMMC_BUFFER_TYPE) != (SDMMC_BUFFER_UNCACHED))
			if (0 == mmc_write(Dev, (uint32_t)(Addr64 >> 9), (void *)Buffer, Nbyte >> 9)) {
			  #if ((MEDIA_DEBUG) > 0)				/* SDMMC driver works with 512 byte blocks		*/
				if (Retry != 0) {
					printf("MEDIA [Drv:%d] %s           Write successful after re-init\n",
					       (int)Drv, FSname);
				}
			  #endif
			  #if ((MEDIA_DEBUG) > 1)
				printf("MEDIA [Drv:%d] %s - write result OK\n", Drv, FSname);
			  #endif
				return(0);
			}
		  #else
			do {
				Size = Nbyte;
				if (Size > sizeof(g_DMAbuff[0])) {	/* If too big for the DMA buffer				*/
					Size = sizeof(g_DMAbuff[0]);	/* Don't write more than DMA buff holds			*/
				}

				memmove(&g_DMAbuff[DMAbufIdx][0], &Buffer[0], Size);
													/* SDMMC driver works with 512 byte blocks		*/
				if (0 != mmc_write(Dev, (uint32_t)(Addr64 >> 9), (void *)&g_DMAbuff[DMAbufIdx][0],
				                   Size >> 9)) {
					break;
				}

				Addr64 += Size;
				Buffer += Size;
				Nbyte  -= Size;
			} while (Nbyte != 0);

			if (Nbyte == 0) {
			  #if ((MEDIA_DEBUG) > 0)
				if (Retry != 0) {
					printf("MEDIA [Drv:%d] %s           Write successful after re-init\n",
					       (int)Drv, FSname);
				}
			  #endif
			  #if ((MEDIA_DEBUG) > 1)
				printf("MEDIA [Drv:%d] %s - write result OK\n", FSname, Drv);
			  #endif
				return(0);
			}
		  #endif
		}
	  #endif
													/* -------------------------------------------- */
	  #if ((MEDIA_QSPI_NMB_DRV) > 0)
		Dev = -1;									/* Assume not a QSPI drive						*/
	   #if ((MEDIA_QSPI0_IDX) >= 0)					/* MEDIA_QSPI0 is supported						*/
		if (Drv == (MEDIA_QSPI0_IDX)) {				/* Check if the drive is MEDIA_QSPI0			*/
			Dev     = MEDIA_QSPI0_DEV;				/* This is MEDIA_QSPI0 device #					*/
			Slv     = MEDIA_QSPI0_SLV;				/* This is MEDIA_QSPI0 slave  #					*/
			Addr   += 512*(MEDIA_QSPI0_START);		/* Take into account start sector #				*/
			BtmpIdx = 0;							/* Index #0 of the temporary buffer				*/
		}
	   #endif
	   #if ((MEDIA_QSPI1_IDX) >= 0)					/* MEDIA_QSPI1 is supported						*/
		if (Drv == (MEDIA_QSPI1_IDX)) {				/* Check if the drive is the MEDIA_QSPI1		*/
			Dev     = MEDIA_QSPI1_DEV;				/* This is MEDIA_QSPI1 device #					*/
			Slv     = MEDIA_QSPI1_SLV;				/* This is MEDIA_QSPI1 slave  #					*/
			Addr   += 512*(MEDIA_QSPI1_START);		/* Take into account start sector #				*/
			BtmpIdx = ((MEDIA_QSPI0_IDX) >= 0);
		}
	   #endif
	   #if ((MEDIA_QSPI2_IDX) >= 0)					/* MEDIA_QSPI2 is supported						*/
		if (Drv == (MEDIA_QSPI2_IDX)) {				/* Check if the drive is the MEDIA_QSPI2		*/
			Dev     = MEDIA_QSPI2_DEV;				/* This is MEDIA_QSPI2 device #					*/
			Slv     = MEDIA_QSPI2_SLV;				/* This is MEDIA_QSPI2 slave  #					*/
			Addr   += 512*(MEDIA_QSPI2_START);		/* Take into account start sector #				*/
			BtmpIdx = (((MEDIA_QSPI0_IDX) >= 0)+((MEDIA_QSPI1_IDX) >= 0));
		}
	   #endif
	   #if ((MEDIA_QSPI3_IDX) >= 0)					/* MEDIA_QSPI3 is supported						*/
		if (Drv == (MEDIA_QSPI3_IDX)) {				/* Check if the drive is the MEDIA_QSPI2		*/
			Dev     = MEDIA_QSPI3_DEV;				/* This is MEDIA_QSPI3 device #					*/
			Slv     = MEDIA_QSPI3_SLV;				/* This is MEDIA_QSPI3 slave  #					*/
			Addr   += 512*(MEDIA_QSPI3_START);		/* Take into account start sector #				*/
			BtmpIdx = (((MEDIA_QSPI0_IDX) >= 0)+((MEDIA_QSPI1_IDX) >= 0)+((MEDIA_QSPI2_IDX) >= 0));
		}
	   #endif

		if ((Dev >= 0)								/* If this is a QSPI device						*/
		&&  (g_MediaIsOK[Drv][0] != 0)) {			/* If the device has been initialized			*/
			BlkSize = qspi_blksize(Dev, Slv);		/* Real sub-sector size of the QSPI				*/
			Err     = 0;
			do {									/* We process one QSPI sector size at a time	*/
			 #if ((MEDIA_QSPI_CHK_WRT) > 0)
			  ChkRetry = (MEDIA_QSPI_CHK_WRT);
			  do {
			 #endif
				WrtSz = Nbyte;						/* Need local to check QSPI sector by sector	*/
				if (((Addr & (BlkSize-1)) + WrtSz) > BlkSize) {
					WrtSz = BlkSize					/* Is too large for 1 QSPI sector, reduce to	*/
					      - (Addr & (BlkSize-1));	/* fit in										*/
				}									/* WrtSz == # of bytes to write in QSPI sector	*/

			  #if ((MEDIA_QSPI_OPT_WRT) == 0)		/* Not optimizing the writes, always erase		*/
				if (WrtSz == BlkSize) {				/* Full QSPI sector write, no need to merge		*/
					Err = qspi_erase(Dev, Slv, Addr, BlkSize);
					if (Err == 0) {
						Err = qspi_write(Dev, Slv, Addr, &Buffer[0], BlkSize);
					}
				}
				else {								/* Partial QSPI sector write need to merge		*/
					Err = qspi_read(Dev, Slv, Addr & ~(BlkSize-1), &g_QspiBuf[BtmpIdx][0], BlkSize);
					if (Err == 0) {
						Err = qspi_erase(Dev, Slv, Addr & ~(BlkSize-1), BlkSize);
					}
					if (Err == 0) {
						memmove(&g_QspiBuf[BtmpIdx][Addr & (BlkSize-1)], &Buffer[0], WrtSz);
						Err = qspi_write(Dev, Slv, Addr&~(BlkSize-1), &g_QspiBuf[BtmpIdx][0],BlkSize);
					}
				}
			  #else									/* Check if we can skip the erase				*/
				Erase     = 0;						/* Assume we don't have to erase				*/
				GotDiff   = 0;
				SkipEnd   = 0;
				SkipStart = 0;
				ChkSize   = (BlkSize >= 512)		/* Some QSPI have sectors smaller than  512		*/
				          ? 512						/* So yo can erase smaller than 512 byte		*/
				          : BlkSize;				/* otherwise, check by 512 individual blocks	*/

				for (kk=0 ; (kk<WrtSz) && (Erase==0) ; kk+=ChkSize) {
					Err = qspi_read(Dev, Slv, Addr+kk, &BufChk[0], ChkSize);
					if (Err != 0) {					/* Done if there was a read error				*/
						break;
					}
					for (jj=0 ; jj<ChkSize ; jj++) {
						Data = Buffer[kk+jj];		/* OK when QSPI & Data == Data					*/
						if ((BufChk[jj] & Data) != Data) {
							Erase = 1;				/* Can't write OK without erasing				*/
							break;
						}
						if (Data == BufChk[jj]) {	/* Can skip write when identical				*/
							if (GotDiff == 0) {		/* When still no diff at start, one more to		*/
								SkipStart++;		/* skip at the beginning						*/
							}
							else {					/* Got a diff, now count the end part that is	*/
								SkipEnd++;			/* identical. Everytime a diff is encountered	*/
							}						/* SkipEnd is reset to 0						*/
						}
						else {						/* Data != QSPI									*/
							GotDiff = 1;			/* Done with counting first to skip				*/
							SkipEnd = 0;			/* Reset count of end to skip					*/
						}
					}
				}

				if (Err == 0) {						/* No read error, keep going					*/
					if (Erase == 0) {				/* No need to erase, do an immediate write		*/
						if (WrtSz != SkipStart) {	/* Nothing to do when data already the same		*/
							Err = qspi_write(Dev, Slv, Addr+SkipStart, &Buffer[SkipStart],
							                 WrtSz-(SkipStart+SkipEnd));
						}
					}
					else if (WrtSz == BlkSize) {	/* Full QSPI sector write						*/
						Err = qspi_erase(Dev, Slv, Addr, BlkSize);
						if (Err == 0) {
							Err = qspi_write(Dev, Slv, Addr, &Buffer[0], BlkSize);
						}
					}
					else {							/* Partial QSPI sector write					*/
						Err = qspi_read(Dev, Slv, Addr & ~(BlkSize-1), &g_QspiBuf[BtmpIdx][0],
						                BlkSize);
						if (Err == 0) {
							Err = qspi_erase(Dev, Slv, Addr & ~(BlkSize-1), BlkSize);
						}
						if (Err == 0) {
							memmove(&g_QspiBuf[BtmpIdx][Addr & (BlkSize-1)], &Buffer[0], WrtSz);
							Err = qspi_write(Dev, Slv, Addr & ~(BlkSize-1),
							                 &g_QspiBuf[BtmpIdx][0], BlkSize);
						}
					}
				}

			  #endif

			  #if ((MEDIA_QSPI_CHK_WRT) > 0)
				WrtBad = 0;							/* In case DEBUG to not print retry info		*/
				if (Err == 0) {						/* when an error has occurred					*/
					if (Erase == 0) {				/* If read error, will likely get a mismatch	*/
						qspi_read(Dev, Slv, Addr,&g_QspiChkBuf[BtmpIdx][0],WrtSz);
						WrtBad = memcmp(&g_QspiChkBuf[BtmpIdx][0], &Buffer[0],
						                WrtSz);
					}
					else if (WrtSz == BlkSize) {
						qspi_read(Dev, Slv, Addr, &g_QspiChkBuf[BtmpIdx][0], BlkSize);
						WrtBad = memcmp(&g_QspiChkBuf[BtmpIdx][0], &Buffer[0], BlkSize);
					}
					else {
						qspi_read(Dev, Slv, Addr & ~(BlkSize-1), &g_QspiChkBuf[BtmpIdx][0], BlkSize);
						WrtBad = memcmp(&g_QspiChkBuf[BtmpIdx][0], &g_QspiBuf[BtmpIdx][0], BlkSize);
					}
				}

				if (WrtBad == 0) {
					Addr   += WrtSz;				/* Update Buffer, base address and # bytes		*/
					Buffer += WrtSz;
					Nbyte  -= WrtSz;
				  #if ((MEDIA_DEBUG) > 0)
					if (ChkRetry != (MEDIA_QSPI_CHK_WRT)) {
						printf("MEDIA [Drv:%d] %s           QSPI write retry OK\n",
						       (int)Drv, FSname);
					}
				  #endif
				}
				else {
					if (ChkRetry <= 0) {
					  #if ((MEDIA_DEBUG) > 0)
						printf("MEDIA [Drv:%d] %s           QSPI write mismatch, too many retries\n",
						       (int)Drv, FSname);
					  #endif
						Err = 1;					/* Abort the outer loop							*/
					}
				  #if ((MEDIA_DEBUG) > 0)
					else {
						printf("MEDIA [Drv:%d] %s           QSPI write mismatch, retrying\n",
						       (int)Drv, FSname);
					}
				  #endif
				}
			  } while ((--ChkRetry >= 0)
			    &&     (WrtBad != 0));

			 #else									/* Not checking if write was OK					*/
				Addr   += WrtSz;					/* Update Buffer, base address and # bytes		*/
				Buffer += WrtSz;
				Nbyte  -= WrtSz;
			 #endif


			} while ((Nbyte != 0)
			  &&     (Err == 0));

			if (Err == 0) {
			  #if ((MEDIA_DEBUG) > 0)
				if (Retry != 0) {
					printf("MEDIA [Drv:%d] %s           Write successful after re-init\n",
					       (int)Drv, FSname);
				}
			  #endif
			  #if ((MEDIA_DEBUG) > 1)
				printf("MEDIA [Drv:%d] %s - write result OK\n", (int)Drv, FSname);
			  #endif
				return(0);
			}
		}
	  #endif

	  #if ((MEDIA_MDRV_IDX) >= 0)
		if ((Drv == (MEDIA_MDRV_IDX))				/* Check if the drive is the Memory Drive		*/
		&&  (g_MediaIsOK[Drv][0] != 0)) {			/* And has been initialized						*/
			memmove(&G_MemDrvBase[Addr], &Buffer[0], Nbyte);
		  #if ((MEDIA_DEBUG) > 1)
			printf("MEDIA [Drv:%d] %s - write result OK\n", (int)Drv, FSname);
		  #endif
			return(0);
		}
	  #endif

	  #if ((MEDIA_DEBUG) > 0)
		printf("MEDIA [Drv:%d] %s - Error - Write failure, re-initializing the media controller\n",
		       (int)Drv, FSname);
	  #endif

		MEDIAinit(FSname, Drv);						/* Has to retry, re-init in case it helps		*/

	} while (Retry++ < 1);							/* Retry once									*/

  #if ((MEDIA_DEBUG) > 1)
	printf("MEDIA [Drv:%d] %s - write result ERROR\n", (int)Drv, FSname);
  #endif

	return(-1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    MEDIAsize																			*/
/*																									*/
/* MEDIAsize - report the size of the media device													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int64_t MEDIAsize(int Drv);																	*/
/*																									*/
/* ARGUMENTS:																						*/
/*		FSname : name of the file system stack calling MEDIAsize()									*/
/*		Drv    : physical drive # to access															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		!= 0 : size of the media in bytes															*/
/*		== 0 : media is not accessible or unknown size												*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*		MEDIAinit() must have been called to report the mass storage size.							*/
/*		If not, 0 is reported																		*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int64_t MEDIAsize(const char *FSname, int Drv)
{
int64_t Ret;										/* Return value									*/

  #if ((MEDIA_ARG_CHECK) != 0)
	if ((Drv < 0)									/* Check valid range for the drive #			*/
	||  (Drv > (MEDIA_NMB_DRV))) {
	  #if ((MEDIA_DEBUG) > 0)
		printf("MEDIA [Drv:%d] %s - Error - media_size drive # out of range\n", Drv, FSname);
	  #endif
		return((int64_t)0);
	}
  #endif

	Ret = (int64_t)0;								/* Assume unknown drive #						*/

	if (g_MediaIsOK[Drv][0] != 0)	{				/* The drive must have been initialized			*/
													/* -------------------------------------------- */
	  #if ((MEDIA_SDMMC0_IDX) >= 0)					/* MEDIA_SDMMC0 is supported					*/
		if (Drv == (MEDIA_SDMMC0_IDX)) {			/* Check if the drive is MEDIA_SDMMC0			*/
			Ret = mmc_capacity(MEDIA_SDMMC0_DEV)
			    - (512LL * (MEDIA_SDMMC0_START));	/* Remove size before 1 sector					*/
		  #if ((MEDIA_SDMMC0_SIZE) > 0)
			if (Ret > (512LL * (MEDIA_SDMMC0_SIZE))) {
			  #if ((MEDIA_DEBUG) > 1)
				printf("MEDIA [Drv:%d] %s - MEDIA_SDMMC0_START+MEDIA_SDMMC0_SIZE > device size\n",
				        (int)Drv, FSname);
			  #endif
				Ret = 512LL * (MEDIA_SDMMC0_SIZE);	/* Use smallest between real left over and the	*/
			}										/* user specified device size					*/
		  #endif
		}
	  #endif
	  #if ((MEDIA_SDMMC1_IDX) >= 0)					/* MEDIA_SDMMC1 is supported					*/
		if (Drv == (MEDIA_SDMMC1_IDX)) {			/* Check if the drive is MEDIA_SDMMC1			*/
			Ret = mmc_capacity(MEDIA_SDMMC1_DEV)
			    - (512LL * (MEDIA_SDMMC1_START));	/* Remove size before 1 sector					*/
		  #if ((MEDIA_SDMMC1_SIZE) > 0)
			if (Ret > (512LL * (MEDIA_SDMMC1_SIZE))) {
			  #if ((MEDIA_DEBUG) > 1)
				printf("MEDIA [Drv:%d] %s - MEDIA_SDMMC1_START+MEDIA_SDMMC1_SIZE > device size\n",
				        (int)Drv, FSname);
			  #endif
				Ret = 512LL * (MEDIA_SDMMC1_SIZE);	/* Use smallest between real left over and the	*/
			}										/* user specified device size					*/
		  #endif
		}
	  #endif

	  #if ((MEDIA_QSPI0_IDX) >= 0)					/* MEDIA_QSPI0 is supported						*/
		if (Drv == (MEDIA_QSPI0_IDX)) {				/* Check if the drive is MEDIA_QSPI0			*/
			Ret = qspi_size(MEDIA_QSPI0_DEV, MEDIA_QSPI0_SLV)
			    - (512LL * (MEDIA_QSPI0_START));	/* Remove size before 1 sector					*/
		  #if ((MEDIA_QSPI0_SIZE) > 0)
			if (Ret > (512LL * (MEDIA_QSPI0_SIZE))) {
			  #if ((MEDIA_DEBUG) > 1)
				printf("MEDIA [Drv:%d] %s - MEDIA_QSPI0_START+MEDIA_QSPI0_SIZE > device size\n",
				        (int)Drv, FSname);
			  #endif
				Ret = 512LL * (MEDIA_QSPI0_SIZE);	/* Use smallest between real left over and the	*/
			}										/* user specified device size					*/
		  #endif
		}
	  #endif
	  #if ((MEDIA_QSPI1_IDX) >= 0)					/* MEDIA_QSPI1 is supported						*/
		if (Drv == (MEDIA_QSPI1_IDX)) {				/* Check if the drive is MEDIA_QSPI1			*/
			Ret = qspi_size(MEDIA_QSPI1_DEV, MEDIA_QSPI1_SLV)
			    - (512LL * (MEDIA_QSPI1_START));	/* Remove size before 1 sector					*/
		  #if ((MEDIA_QSPI1_SIZE) > 1)
			if (Ret > (512LL * (MEDIA_QSPI1_SIZE))) {
			  #if ((MEDIA_DEBUG) > 0)
				printf("MEDIA [Drv:%d] %s - MEDIA_QSPI2_START+MEDIA_QSPI2_SIZE > device size\n",
				        (int)Drv, FSname);
			  #endif
				Ret = 512LL * (MEDIA_QSPI1_SIZE);	/* Use smallest between real left over and the	*/
			}										/* user specified device size					*/
		  #endif
		}
	  #endif
	  #if ((MEDIA_QSPI2_IDX) >= 0)					/* MEDIA_QSPI1 is supported						*/
		if (Drv == (MEDIA_QSPI2_IDX)) {				/* Check if the drive is MEDIA_QSPI2			*/
			Ret = qspi_size(MEDIA_QSPI2_DEV, MEDIA_QSPI2_SLV)
			    - (512LL * (MEDIA_QSPI2_START));	/* Remove size before 1 sector					*/
		  #if ((MEDIA_QSPI2_SIZE) > 0)
			if (Ret > (512LL * (MEDIA_QSPI2_SIZE))) {
			  #if ((MEDIA_DEBUG) > 1)
				printf("MEDIA [Drv:%d] %s - MEDIA_QSPI2_START+MEDIA_QSPI2_SIZE > device size\n",
				        (int)Drv, FSname);
			  #endif
				Ret = 512LL * (MEDIA_QSPI2_SIZE);	/* Use smallest between real left over and the	*/
			}										/* user specified device size					*/
		  #endif
		}
	  #endif
	  #if ((MEDIA_QSPI3_IDX) >= 0)					/* MEDIA_QSPI1 is supported						*/
		if (Drv == (MEDIA_QSPI3_IDX)) {				/* Check if the drive is MEDIA_QSPI3			*/
			Ret = qspi_size(MEDIA_QSPI3_DEV, MEDIA_QSPI3_SLV)
			    - (512LL * (MEDIA_QSPI3_START));	/* Remove size before 1 sector					*/
		  #if ((MEDIA_QSPI3_SIZE) > 0)
			if (Ret > (512LL * (MEDIA_QSPI3_SIZE))) {
			  #if ((MEDIA_DEBUG) > 1)
				printf("MEDIA [Drv:%d] %s - MEDIA_QSPI3_START+MEDIA_QSPI3_SIZE > device size\n",
				        (int)Drv, FSname);
			  #endif
				Ret = 512LL * (MEDIA_QSPI3_SIZE);	/* Use smallest between real left over and the	*/
			}										/* user specified device size					*/
		  #endif
		}
	  #endif

	  #if ((MEDIA_MDRV_IDX) >= 0)					/* Memory drive is supported					*/
		if (Drv == (MEDIA_MDRV_IDX)) {				/* Check if the drive is the Memory Drive		*/
			Ret = (int64_t)g_MemDrvSize;
		}
	  #endif
	}

	if (Ret < 0) {
		Ret = 0;
	}

  #if ((MEDIA_DEBUG) > 1)
	printf("MEDIA [Drv:%d] - media_size result %llu\n", (int)Drv, (long long unsigned)Ret);
  #endif

	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    MEDIAblksize																		*/
/*																									*/
/* MEDIAblksize - report the block size of a media device											*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int32_t MEDIAblksize(int Drv);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv : physical drive # to access															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		!= 0 : block size of the media in bytes														*/
/*		== 0 : media is not accessible or unknown block size										*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*		MEDIAinit() must have been called to report the mass storage block size.					*/
/*		If not, 0 is reported																		*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int32_t MEDIAblksize(int Drv)
{
int32_t Ret;										/* Return value									*/

  #if ((MEDIA_ARG_CHECK) != 0)
	if ((Drv < 0)									/* Check valid range for the drive #			*/
	||  (Drv > (MEDIA_NMB_DRV))) {
	  #if ((MEDIA_DEBUG) > 0)
		printf("MEDIA [Drv:%d] - Error - media_blksize drive # out of range\n", Drv);
	  #endif
		return((int32_t)0);
	}
  #endif

	Ret = (int32_t)0;								/* Assume unknown drive #						*/

	if (g_MediaIsOK[Drv][0] != 0)	{				/* The drive must have been initialized			*/
													/* -------------------------------------------- */
	  #if ((MEDIA_SDMMC0_IDX) >= 0)					/* MEDIA_SDMMC0 is supported					*/
		if (Drv == (MEDIA_SDMMC0_IDX)) {			/* Check if the drive is MEDIA_SDMMC0			*/
			 Ret = mmc_blklen(MEDIA_SDMMC0_DEV);
		}
	  #endif
	  #if ((MEDIA_SDMMC1_IDX) >= 0)					/* MEDIA_SDMMC1 is supported					*/
		if (Drv == (MEDIA_SDMMC1_IDX)) {			/* Check if the drive is MEDIA_SDMMC1			*/
			Ret = mmc_blklen(MEDIA_SDMMC1_DEV);
		}
	  #endif

	  #if ((MEDIA_QSPI0_IDX) >= 0)					/* MEDIA_QSPI0 is supported						*/
		if (Drv == (MEDIA_QSPI0_IDX)) {				/* Check if the drive is MEDIA_QSPI0			*/
			Ret = qspi_blksize(MEDIA_QSPI0_DEV, MEDIA_QSPI0_SLV);
		}
	  #endif
	  #if ((MEDIA_QSPI1_IDX) >= 0)					/* MEDIA_QSPI1 is supported						*/
		if (Drv == (MEDIA_QSPI1_IDX)) {				/* Check if the drive is MEDIA_QSPI1			*/
			Ret = qspi_blksize(MEDIA_QSPI1_DEV, MEDIA_QSPI1_SLV);
		}
	  #endif
	  #if ((MEDIA_QSPI2_IDX) >= 0)					/* MEDIA_QSPI2 is supported						*/
		if (Drv == (MEDIA_QSPI2_IDX)) {				/* Check if the drive is MEDIA_QSPI2			*/
			Ret = qspi_blksize(MEDIA_QSPI2_DEV, MEDIA_QSPI2_SLV);
		}
	  #endif
	  #if ((MEDIA_QSPI3_IDX) >= 0)					/* MEDIA_QSPI3 is supported						*/
		if (Drv == (MEDIA_QSPI3_IDX)) {				/* Check if the drive is MEDIA_QSPI3			*/
			Ret = qspi_blksize(MEDIA_QSPI3_DEV, MEDIA_QSPI3_SLV);
		}
	  #endif

	  #if ((MEDIA_MDRV_IDX) >= 0)					/* Memory drive is supported					*/
		if (Drv == (MEDIA_MDRV_IDX)) {				/* Check if the drive is the Memory Drive		*/
			Ret = (int32_t)512;						/* Mdrive always uses block size of 512			*/
		}
	  #endif
	}

  #if ((MEDIA_DEBUG) > 1)
	printf("MEDIA [Drv:%d] - media_blksize result %u\n", Drv, (unsigned int)Ret);
  #endif

	if ((Ret > 0)
	&&  (Ret < 512)) {
		Ret = 512;
	}
	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    MEDIAsectsz																			*/
/*																									*/
/* MEDIAsectsz - report /set the sector size of a media device										*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int32_t MEDIAsectsz(int Drv, int Size);														*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv  : physical drive # to access															*/
/*		Size : >  0 : set the sector size to use upon init											*/
/*		              this does not change the current sector size if the media has been init		*/
/*		       == 0 : de-initialize the drive														*/
/*		       <  0 : do nothing, only return the current sector size								*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>  0 : sector size of the media in bytes													*/
/*		== 0 : media is not accessible or unknown sector size										*/
/*		<  0 : invalid drive number																	*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*		MEDIAinit() must have been called to report the mass storage block size.					*/
/*		If not, 0 is reported																		*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int32_t MEDIAsectsz(int Drv, int Size)
{
  #if ((MEDIA_ARG_CHECK) != 0)
	if ((Drv < 0)									/* Check valid range for the drive #			*/
	||  (Drv > (MEDIA_NMB_DRV))) {
	  #if ((MEDIA_DEBUG) > 0)
		printf("MEDIA [Drv:%d] - Error - media_sectsz drive # out of range\n", Drv);
	  #endif
		return((int32_t)-1);
	}
  #endif

	if (Size == 0) {								/* Request to un-initialize the drive			*/
		g_MediaIsOK[Drv][0] = 0;					/* This will force a re-init					*/
	}
	else if (Size > 0) {							/* Request to set the sector size				*/
		if (Size < 512) {							/* FAT min sector size is 512					*/
			Size = 512;
		}
		g_MediaIsOK[Drv][1] = Size;					/* Memo for future use. Note is index 1 not 0	*/
	}

	return((int32_t)g_MediaIsOK[Drv][0]);			/* Return new or current sector size			*/
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    MEDIAinfo																			*/
/*																									*/
/* MEDIAinfo - report the physical device a drive number is  attached to							*/
/*																									*/
/* SYNOPSIS:																						*/
/*		const char *MEDIAinfo(int Drv);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv  : physical drive # to retrieve the type of medium										*/
/*																									*/
/* RETURN VALUE:																					*/
/*		String with the medium type and physical device number (and slave number for QSPI)			*/
/*				e.g "SDMMC 0" or "QSPI  1:3"														*/
/*		NULL for out of range drive number															*/
/*																									*/
/* DESCRIPTION:																						*/
/*		The string is always constructed with these fields/char indexes:							*/
/*			#0->4 : media type ("SDMMC", "QSPI " or "MDRV ")										*/
/*			#5    : white space																		*/
/*				SD/MMC & MDRV:																		*/
/*			#6+   : device controller number														*/
/*				QSPI:																				*/
/*			#6+:+ : Device controller number ":" slave number										*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
#define MEDIA_STRING_S(x)	#x
#define MEDIA_STRING(x)		MEDIA_STRING_S(x)

const char *MEDIAinfo(int Drv)
{													/* If drive not in range, NULL is returned		*/

  #if ((MEDIA_SDMMC0_IDX) >= 0)
	if (Drv == (MEDIA_SDMMC0_IDX)) {
		return("SDMMC " MEDIA_STRING(MEDIA_SDMMC0_DEV));
	}
  #endif

  #if ((MEDIA_SDMMC1_IDX) >= 0)
	if (Drv == (MEDIA_SDMMC1_IDX)) {
		return("SDMMC " MEDIA_STRING(MEDIA_SDMMC1_DEV));
	}
  #endif

  #if ((MEDIA_QSPI0_IDX) >= 0)
	if (Drv == (MEDIA_QSPI0_IDX)) {
		return("QSPI  " MEDIA_STRING(MEDIA_QSPI0_DEV) ":" MEDIA_STRING(MEDIA_QSPI0_SLV));
	}
  #endif

  #if ((MEDIA_QSPI1_IDX) >= 0)
	if (Drv == (MEDIA_QSPI1_IDX)) {
		return("QSPI  " MEDIA_STRING(MEDIA_QSPI1_DEV) ":" MEDIA_STRING(MEDIA_QSPI1_SLV));
	}
  #endif

  #if ((MEDIA_QSPI2_IDX) >= 0)
	if (Drv == (MEDIA_QSPI2_IDX)) {
		return("QSPI  " MEDIA_STRING(MEDIA_QSPI2_DEV) ":" MEDIA_STRING(MEDIA_QSPI2_SLV));
	}
  #endif

  #if ((MEDIA_QSPI3_IDX) >= 0)
	if (Drv == (MEDIA_QSPI3_IDX)) {
		return("QSPI  " MEDIA_STRING(MEDIA_QSPI3_DEV) ":" MEDIA_STRING(MEDIA_QSPI3_SLV));
	}
  #endif

  #if ((MEDIA_MDRV_IDX) >= 0)
	if (Drv == (MEDIA_MDRV_IDX)) {
		return("MDRV  0");
	}
  #endif

	return((const char *)NULL);
}

/* EOF */
