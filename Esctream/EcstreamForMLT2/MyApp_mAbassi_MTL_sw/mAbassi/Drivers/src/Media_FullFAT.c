/* ------------------------------------------------------------------------------------------------ */
/* FILE :		Media_FullFAT_CY5.c																	*/
/*																									*/
/* CONTENTS :																						*/
/*				Interface between FullFAT & the SD/MMC / QSPI drivers.								*/
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
#include "ff_time.h"
#include "ff_types.h"
#include <time.h>

/* ------------------------------------------------------------------------------------------------ */

#define MY_FS_NAME	"FullFAT"

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_init_FullFAT																	*/
/*																									*/
/* media_init_FullFAT - initialize a drive 															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int media_init_FullFAT(int Drv);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv : physical drive # to access															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 1 : media is initialiuzd and accessible													*/
/*		== 0 : media cannot be initialized															*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int media_init_FullFAT(int Drv)
{
	return(0 == MEDIAinit(MY_FS_NAME, Drv));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_read_FullFAT																	*/
/*																									*/
/* media_read_FullFAT - read data from the physical device											*/
/*																									*/
/* SYNOPSIS:																						*/
/*		FF_T_SINT32 media_read_FullFAT(FF_T_UINT8 *buff,FF_T_UINT32 sector, FF_T_UINT32 count,		*/
/*		                               void *pParam);												*/
/*																									*/
/* ARGUMENTS:																						*/
/*		buff   : buff contents to read from the drive												*/
/*		sector : number of the first sector to read from											*/
/*		count  : number of sectors to read															*/
/*		pParam : pointer which value in (int) is the drive #										*/
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

FF_T_SINT32 media_read_FullFAT(FF_T_UINT8 *buff, FF_T_UINT32 sector, FF_T_UINT32 count, void *pParam)
{
	return(0 == MEDIAread(MY_FS_NAME, (int)(intptr_t)pParam, (uint8_t *)buff, (uint32_t)sector,
	                     (int)count));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_write_FullFAT																	*/
/*																									*/
/* media_write_FullFAT - write data to the physical device											*/
/*																									*/
/* SYNOPSIS:																						*/
/*		FF_T_SINT32 media_write_FullFAT(FF_T_UINT8 *buff, FF_T_UINT32 sector, FF_T_UINT32 count,	*/
/*		                                void *pParam);												*/
/*																									*/
/* ARGUMENTS:																						*/
/*		buff   : buff contents to write on the drive												*/
/*		sector : number of the first sector to write to												*/
/*		count  : number of sectors to write															*/
/*		pParam : pointer which value in (int) is the drive #										*/
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

FF_T_SINT32 media_write_FullFAT(FF_T_UINT8 *buff, FF_T_UINT32 sector, FF_T_UINT32 count, void *pParam)
{
	return(0 == MEDIAwrite(MY_FS_NAME, (int)(intptr_t)pParam, (const uint8_t *)buff, (uint32_t)sector,
	                       (int)count));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_uninit_FullFAT																*/
/*																									*/
/* media_uninit_FullFAT - de-initialize a media device												*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int media_uninit_FullFAT(int Drv);															*/
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

int media_uninit_FullFAT(int Drv)
{
	return(MEDIAsectsz(Drv, 0) < 0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_size_FullFAT																	*/
/*																									*/
/* media_size_FullFAT - report the size of the media device											*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int64_t media_size_FullFAT(int Drv);														*/
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

int64_t media_size_FullFAT(int Drv)
{
	return(MEDIAsize(MY_FS_NAME, Drv));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_blksize_FullFAT																*/
/*																									*/
/* media_blksize_FullFAT - report the block size of a media device									*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int32_t media_blksize_FullFAT(int Drv);														*/
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

int32_t media_blksize_FullFAT(int Drv)
{
	return(MEDIAblksize(Drv));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_sectsize_FullFAT																*/
/*																									*/
/* media_sectsize_FullFAT - report the block size of a media device									*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int32_t media_sectsize_FullFAT(int Drv);													*/
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

int32_t media_sectsize_FullFAT(int Drv)
{
	return(MEDIAsectsz(Drv, -1));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    media_info_FullFAT																	*/
/*																									*/
/* media_info_FullFAT - report the physical device a drive number is  attached to					*/
/*																									*/
/* SYNOPSIS:																						*/
/*		const char *media_info_FullFAT(int Drv);													*/
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

const char *media_info_FullFAT(int Drv)
{
	return(MEDIAinfo(Drv));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    FF_GetSystemTime																	*/
/*																									*/
/* FF_GetSystemTime - report the local date/time													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		FF_T_SINT32	FF_GetSystemTime(FF_SYSTEMTIME *pTime);											*/
/*																									*/
/* ARGUMENTS:																						*/
/*		pTime : where to deposit the local date/time												*/
/*																									*/
/* RETURN VALUE:																					*/
/*		always 0																					*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

FF_T_SINT32	FF_GetSystemTime(FF_SYSTEMTIME *pTime)
{
#ifdef FF_TIME_SUPPORT
time_t     TimeNow;
struct tm *TimeTmp;

	TimeNow = time(NULL);
	TimeTmp = localtime(&TimeNow);

	pTime->Second = TimeTmp->tm_sec;
	pTime->Minute = TimeTmp->tm_min;
	pTime->Hour   = TimeTmp->tm_hour;
	pTime->Day    = TimeTmp->tm_mday;
	pTime->Month  = TimeTmp->tm_mon + 1;
	pTime->Year   = TimeTmp->tm_year + 1900;
#else
	memset(pTime, 0, sizeof(*pTime));
#endif

	return(0);
}

/* EOF */
