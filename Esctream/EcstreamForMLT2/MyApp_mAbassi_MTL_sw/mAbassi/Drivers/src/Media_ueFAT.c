/* ------------------------------------------------------------------------------------------------ */
/* FILE :		Media_ueFAT.c																		*/
/*																									*/
/* CONTENTS :																						*/
/*				Interface between ultra-embedded FAT & the SD/MMC / QSPI drivers.					*/
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
/*	$Revision: 1.19 $																				*/
/*	$Date: 2019/01/10 18:07:04 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "MediaIF.h"
#include "fat_access.h"

/* ------------------------------------------------------------------------------------------------ */

#define MY_FS_NAME	"ueFAT"

/* ------------------------------------------------------------------------------------------------ */

static int g_Drive = -1;							/* ueFAT is single drive, use to access a 		*/
													/* a drive number different than 0				*/
/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_init_ueFAT																	*/
/*																									*/
/* media_init_ueFAT - initialize a drive 															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int media_init_ueFAT(int Drv);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv : physical drive # to access															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 1 : media is initialized and accessible													*/
/*		== 0 : media cannot be initialized															*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int media_init_ueFAT(int Drv)
{
int Ret;

	Ret = MEDIAinit(MY_FS_NAME, Drv);
	g_Drive = (Ret == 0)							/* Memo the drive number ueFAT will be using	*/
	        ? Drv
	        : -1;

	return(0 == Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_read_ueFAT																	*/
/*																									*/
/* media_read_ueFAT - read data from the physical device											*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int media_read_ueFAT(uint32 sector, uint8 *buff, uint32 count);								*/
/*																									*/
/* ARGUMENTS:																						*/
/*		buff   : buff contents to read from the drive												*/
/*		sector : number of the first sector to read from											*/
/*		count  : number of sectors to read															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 1 : success																				*/
/*		== 0 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int media_read_ueFAT(uint32 sector, uint8 *buff, uint32 count)
{
	return(0 == MEDIAread(MY_FS_NAME, g_Drive, (uint8_t *)buff, (uint32_t)sector, (int)count));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_write_ueFAT																	*/
/*																									*/
/* media_write_ueFAT - write data to the physical device											*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int media_write_ueFAT(uint32 sector, uint8 *buff, uint32 count);							*/
/*																									*/
/* ARGUMENTS:																						*/
/*		buff   : buff contents to write on the drive												*/
/*		sector : number of the first sector to write to												*/
/*		count  : number of sectors to write															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 1 : success																				*/
/*		== 0 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int media_write_ueFAT(uint32 sector, uint8 *buff, uint32 count)
{
	return(0 == MEDIAwrite(MY_FS_NAME, g_Drive, (const uint8_t *)buff, (uint32_t)sector, (int)count));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_uninit_ueFAT																	*/
/*																									*/
/* media_uninit_ueFAT - de-initialize a media device												*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int media_uninit_ueFAT(int Drv);															*/
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

int media_uninit_ueFAT(int Drv)
{
	return(MEDIAsectsz(Drv, 0) < 0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_size_ueFAT																	*/
/*																									*/
/* media_size_ueFAT - report the size of the media device											*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int64_t media_size_ueFAT(int Drv);															*/
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

int64_t media_size_ueFAT(int Drv)
{
	return(MEDIAsize(MY_FS_NAME, Drv));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_blksize_ueFAT																	*/
/*																									*/
/* media_blksize_ueFAT - report the block size of a media device									*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int32_t media_blksize_ueFAT(int Drv);														*/
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

int32_t media_blksize_ueFAT(int Drv)
{
	return(MEDIAblksize(Drv));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_sectsize_ueFAT																*/
/*																									*/
/* media_sectsize_ueFAT - report the block size of a media device									*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int32_t media_sectsize_ueFAT(int Drv);														*/
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

int32_t media_sectsize_ueFAT(int Drv)
{
	return(MEDIAsectsz(Drv, -1));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_info_ueFAT																	*/
/*																									*/
/* media_info_ueFAT - report the physical device a drive number is  attached to						*/
/*																									*/
/* SYNOPSIS:																						*/
/*		const char *media_info_ueFAT(int Drv);														*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv  : physical to retrieve the type of medium												*/
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

const char *media_info_ueFAT(int Drv)
{
	return(MEDIAinfo(Drv));
}

/* EOF */
