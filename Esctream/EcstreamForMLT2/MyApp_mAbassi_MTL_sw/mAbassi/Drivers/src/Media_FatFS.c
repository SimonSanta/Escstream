/* ------------------------------------------------------------------------------------------------ */
/* FILE :		Media_FatFS.c																		*/
/*																									*/
/* CONTENTS :																						*/
/*				Interface between FatFS & the SD/MMC / QSPI drivers.								*/
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
/*	$Revision: 1.41 $																				*/
/*	$Date: 2019/01/10 18:07:04 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "MediaIF.h"
#include "diskio.h"
#include "ffconf.h"
#include <time.h>

/* ------------------------------------------------------------------------------------------------ */

#define MY_FS_NAME		"FatFS"

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    disk_status																			*/
/*																									*/
/* disk_status - report if the media accesses are OK												*/
/*																									*/
/* SYNOPSIS:																						*/
/*		DSTATUS disk_status(BYTE Drv);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv   : physical drive # to check															*/
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

DSTATUS disk_status(BYTE Drv)
{
	return ((0 == MEDIAstatus(MY_FS_NAME, (int)Drv))
	     	? RES_OK
	        : STA_NODISK);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    disk_initialize																		*/
/*																									*/
/* disk_initialize - initialize a drive 															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		DSTATUS disk_initialize(BYTE Drv);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv : physical drive # to access															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== RES_OK     : media is initialized and accessible											*/
/*		== RES_NO_INT : cannot be initialized														*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

DSTATUS disk_initialize(BYTE Drv)
{
	return ((0 == MEDIAinit(MY_FS_NAME, (int)Drv))
	     	? RES_OK
	        : STA_NOINIT);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    disk_read																			*/
/*																									*/
/* disk_read - read data from the physical device													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		DRESULT disk_read(BYTE Drv, BYTE *buff, DWORD sector, UINT count);							*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv    : physical drive # to access															*/
/*		buff   : buff contents to read from the drive												*/
/*		sector : number of the first sector to read from											*/
/*		count  : number of sectors to read															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== RES_OK     : success																		*/
/*		== RES_ERROR  : error																		*/
/*		== RES_NOTRDY : disk not ready																*/
/*		== RES_PARERR : invalid drive #																*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

DRESULT disk_read(BYTE Drv, BYTE *buff, DWORD sector, UINT count)
{
	return((0 == MEDIAread(MY_FS_NAME, (int)Drv, (uint8_t *)buff, (uint32_t)sector, (int)count)
	      ? RES_OK
	      : RES_PARERR));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    disk_write																			*/
/*																									*/
/* disk_write - write data to the physical device													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		DRESULT disk_write(BYTE drv, const BYTE *buff, DWORD sector, UINT count);					*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv    : physical drive # to access															*/
/*		buff   : buff contents to write on the drive												*/
/*		sector : number of the first sector to write to												*/
/*		count  : number of sectors to write															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== RES_OK     : success																		*/
/*		== RES_ERROR  : error																		*/
/*		== RES_WRPRT  : drive is write protected													*/
/*		== RES_NOTRDY : disk not ready																*/
/*		== RES_PARERR : invalid drive #																*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#if ((_READONLY) == 0)
DRESULT disk_write(BYTE Drv, const BYTE *buff, DWORD sector, UINT count)
{
	return((0 == MEDIAwrite(MY_FS_NAME, (int)Drv, (const uint8_t *)buff, (uint32_t)sector, (int)count)
	      ? RES_OK
	      : RES_PARERR));
}
#endif

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_uninit_FatFS																	*/
/*																									*/
/* media_uninit_FatFS - de-initialize a media device												*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int media_uninit_FatFS(int Drv);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv : physical drive # to access															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : Success																				*/
/*		!= 0 : invalid drive #																		*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int media_uninit_FatFS(int Drv)
{
	return(MEDIAsectsz(Drv, 0) < 0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_size_FatFS																	*/
/*																									*/
/* media_size_FatFS - report the size of the media device											*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int64_t media_size_FatFS(int Drv);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv : physical drive # to access															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>  0 : size of the media in bytes															*/
/*		<= 0 : media is not accessible or have an unknown size										*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int64_t media_size_FatFS(int Drv)
{
	return(MEDIAsize(MY_FS_NAME, Drv));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_blksize_FatFS																	*/
/*																									*/
/* media_blksize_FatFS - report the block size of a media device									*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int32_t media_blksize_FatFS(int Drv);														*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv : physical drive # to access															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>  0 : block size of the media in bytes														*/
/*		<= 0 : media is not accessible or have an unknown block size								*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int32_t media_blksize_FatFS(int Drv)
{
	return(MEDIAblksize(Drv));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_sectsize_FatFS																*/
/*																									*/
/* media_sectsize_FatFS - report the block size of a media device									*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int32_t media_sectsize_FatFS(int Drv);														*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv : physical drive # to access															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>  0 : block size of the media in bytes														*/
/*		<= 0 : media is not accessible or have an unknown sector size								*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int32_t media_sectsize_FatFS(int Drv)
{
	return(MEDIAsectsz(Drv, -1));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_info_FatFS																	*/
/*																									*/
/* media_info_FatFS - report the physical device a drive number is  attached to						*/
/*																									*/
/* SYNOPSIS:																						*/
/*		const char *media_info_FatFS(int Drv);														*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv  : physical drive # to retrieve the type of medium										*/
/*																									*/
/* RETURN VALUE:																					*/
/*		String with the medium type and physical device number										*/
/*				e.g "SDMMC 0" or "QSPI  1"															*/
/*		The strings have a specific formating to easily extract dev (and slave) numbers.			*/
/*		Refer to MEDIAinfo() in MediaIF.c for a full description									*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

const char *media_info_FatFS(int Drv)
{

	return(MEDIAinfo(Drv));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    disk_ioctl																			*/
/*																									*/
/* disk_ioctl - ioctl																				*/
/*																									*/
/* SYNOPSIS:																						*/
/*		DRESULT disk_write(BYTE Drv, BYTE ctrl, void *buff);										*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv  : physical drive # to access															*/
/*		ctrl : command																				*/
/*		buff : buffer holding the parameter(s) or receiving the result(s)							*/
/*																									*/
/* RETURN VALUE:																					*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

DRESULT disk_ioctl(BYTE Drv, BYTE ctrl, void *buff)
{
DRESULT RetVal;
int     SectSz;

	RetVal = RES_PARERR;
	SectSz = MEDIAsectsz(Drv, -1);
	if (SectSz > 0) {
		if (0 != (disk_status(Drv) & STA_NOINIT)) {	/* Check if a card is there & has been init		*/
			return(RES_NOTRDY);
		}
		switch (ctrl) {	
		case CTRL_SYNC:								/* Flush the disk cache							*/
			RetVal = RES_OK;						/* There is no data cached in the SD/MMC or		*/
			break;									/* QSPI driver, so report OK and do nothing		*/

		case GET_SECTOR_COUNT:						/* Get media size								*/
			*((DWORD *)buff) = (DWORD)(MEDIAsize(MY_FS_NAME, Drv) / SectSz);
			RetVal           = RES_OK;
			break;

		case GET_SECTOR_SIZE:						/* Get sector size								*/
			*((WORD *)buff) = SectSz;
			RetVal          = RES_OK;
			break;

		case GET_BLOCK_SIZE:						/* Get erase block size in unit of sector 		*/
			*((DWORD *)buff) = (DWORD)(MEDIAblksize(Drv) / SectSz);
			RetVal           = RES_OK;
			break;

		case CTRL_TRIM:								/* Block of data no longer needed				*/
			RetVal = RES_ERROR;
			break;									/* (for only _USE_ERASE)						*/

		default:									/* Command error								*/
			RetVal = RES_PARERR;
			break;
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    get_fattime																			*/
/*																									*/
/* get_fattime - get local date / time																*/
/*																									*/
/* SYNOPSIS:																						*/
/*		DWORD get_fattime (void);																	*/
/*																									*/
/* ARGUMENTS:																						*/
/*																									*/
/* RETURN VALUE:																					*/
/*																									*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#if ((FF_FS_NORTC) == 0)

DWORD get_fattime(void)
{
int        Ret;										/* Return value									*/
time_t     TimeNow;
struct tm *TimeTmp;

	TimeNow = time(NULL);
	TimeTmp = localtime(&TimeNow);
	Ret     = (TimeTmp->tm_sec       >>  1)
	        | (TimeTmp->tm_min       <<  5)
	        | (TimeTmp->tm_hour      << 11)
	        | (TimeTmp->tm_mday      << 16)
	        | ((TimeTmp->tm_mon+1)   << 21)
	        | ((TimeTmp->tm_year-80) << 25);

	return(Ret);
}

#endif

/* EOF */
