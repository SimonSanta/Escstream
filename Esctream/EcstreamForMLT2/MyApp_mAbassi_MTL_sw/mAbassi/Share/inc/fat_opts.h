/* ------------------------------------------------------------------------------------------------ */
/* FILE :		fat_opts.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Configuration file for ueFAT														*/
/*																									*/
/*																									*/
/* Copyright (c) 2016-2019, Code-Time Technologies Inc. All rights reserved.						*/
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
/*	$Revision: 1.4 $																				*/
/*	$Date: 2019/01/10 18:07:14 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __FAT_OPTS_H__
#define __FAT_OPTS_H__

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef OS_CMSIS_RTOS
 #if !defined(__ABASSI_H__) && !defined(__MABASSI_H__) && !defined(__UABASSI_H__)
  #if defined(_UABASSI_)							/* This file is shared by all Code Time RTOSes	*/
	#include "mAbassi.h"							/* All these conditionals are here to pick the	*/
  #elif defined (OS_N_CORE)							/* the correct RTOS include file				*/
	#include "mAbassi.h"							/* In the release code, the files included are	*/
  #else												/* substituted at packaging time with the one	*/
	#include "mAbassi.h"								/* of the release								*/
  #endif
 #endif
#else
  #include "cmsis_os.h"
#endif

#ifndef FATFS_IS_LITTLE_ENDIAN
  #define FATFS_IS_LITTLE_ENDIAN		1			/* 0: Big endian     1: Little endian			*/
#endif

#ifndef FATFS_MAX_LONG_FILENAME
  #define FATFS_MAX_LONG_FILENAME		260			/* Maximum file name length						*/
#endif

#ifndef FATFS_MAX_OPEN_FILES
  #define FATFS_MAX_OPEN_FILES			10			/* Maximum number of open files					*/
#endif

#ifndef FAT_BUFFER_SECTORS
  #define FAT_BUFFER_SECTORS			1			/* # of sectors per FAT buffer					*/
#endif

#ifndef FAT_BUFFERS
  #define FAT_BUFFERS					1			/* Max # of FAT sectors to buffer minus 1		*/
#endif

//#define FAT_CLUSTER_CACHE_ENTRIES		128			/* Size of cluster chains						*/

#ifndef FATFS_INC_WRITE_SUPPORT
  #define FATFS_INC_WRITE_SUPPORT		1			/* Boolean if ueFAT provides writing API		*/
#endif

#ifndef FATFS_INC_LFN_SUPPORT
  #define FATFS_INC_LFN_SUPPORT			1			/* Boolean if supporting long file names (LFN)	*/
#endif

#ifndef FATFS_DIR_LIST_SUPPORT
  #define FATFS_DIR_LIST_SUPPORT		1			/* If ueFAT dir services are supported			*/
#endif

#ifndef FATFS_INC_TIME_DATE_SUPPORT
  #define FATFS_INC_TIME_DATE_SUPPORT	0			/* If supporting local date/time				*/
#endif

#ifndef FATFS_INC_FORMAT_SUPPORT
  #define FATFS_INC_FORMAT_SUPPORT		1			/* If formatting is supported					*/
#endif

#ifndef FATFS_INC_TEST_HOOKS
  #define FATFS_INC_TEST_HOOKS			1			/* Need access to some internal					*/
#endif

#ifndef FAT_SECTOR_SIZE
  #define FAT_SECTOR_SIZE				512			/* Sector size									*/
#endif

/* ------------------------------------------------------------------------------------------------ */

#ifndef FAT_PRINTF
  #ifdef FAT_PRINTF_NOINC_STDIO
    extern int printf(const char* ctrl1, ... );
    #define FAT_PRINTF(a)               printf a
  #else
    #include <stdio.h>
    #define FAT_PRINTF(a)               printf a
  #endif
#endif

// Time/Date support requires time.h
#if FATFS_INC_TIME_DATE_SUPPORT
    #include <time.h>
#endif

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
