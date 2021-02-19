/* ------------------------------------------------------------------------------------------------ */
/* FILE :		ff_config.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Configuration file for FullFAT														*/
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
/*	$Revision: 1.6 $																				*/
/*	$Date: 2019/01/10 18:07:14 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef _FF_CONFIG_H_
#define _FF_CONFIG_H_

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

  #define FF_LITTLE_ENDIAN							/* Processor endianess 							*/
//#define FF_BIG_ENDIAN
  #define FF_LFN_SUPPORT							/* Support long file name 						*/
//#define FF_INCLUDE_SHORT_NAME						/* Also have short name of LFN in directory		*/
  #define FF_SHORTNAME_CASE							/* Shortname case insensitive					*/
//#define FF_UNICODE_SUPPORT						/* Always use wchar (UTF-16)					*/
  #define FF_FAT12_SUPPORT							/* Supports FAT12								*/
//#define FF_OPTIMISE_UNALIGNED_ACCESS				/* Optimize unaligned accesses (DO NOT DEFINE!)	*/
  #define FF_CACHE_WRITE_THROUGH					/* Bypass cache on write						*/
  #define FF_WRITE_BOTH_FATS						/* Write the back FAT (2nd FAT)					*/
//#define FF_MIRROR_FATS_UMOUNT						/* Duplicate FAT tables on unmount				*/
//#define FF_WRITE_FREE_COUNT						/* Update internal free byte count at run time	*/
//#define FF_FSINFO_TRUSTED							/* Trust values in the FSINFO sector			*/
//#define FF_TIME_SUPPORT							/* Local Date/Time available and used			*/
  #define FF_REMOVABLE_MEDIA						/* If media is removable						*/
  #define FF_ALLOC_DEFAULT							/* Only allocate what is needed					*/
//#define FF_ALLOC_DOUBLE							/* 2X size of a file on alloc (for performance)	*/
//#define FF_USE_NATIVE_STDIO						/* Use stdio.h									*/
//#define FF_MOUNT_FIND_FREE						/* Calculate free byte count on mount			*/
  #define FF_FINDAPI_ALLOW_WILDCARDS				/* Handle wild cards file names for dir read	*/
  #define FF_WILDCARD_CASE_INSENSITIVE				/* Use case insensitive in file names from wildc*/
  #define FF_PATH_CACHE								/* Caching of the paths used					*/
  #define FF_PATH_CACHE_DEPTH	5					/* Number of cached path						*/
//#define FF_HASH_CACHE								/* Enable HASH (speeds up file creation)		*/
  #define FF_HASH_CACHE_DEPTH	10					/* # of directories to be hashed				*/
  #define FF_HASH_FUNCTION		CRC16				/* CRC16 or CRC8 hashing						*/
  #define FF_BLKDEV_USES_SEM						/* To use mutexes when accessing media			*/
  #define FF_INLINE_MEMORY_ACCESS					/* In-lining of the memory independence			*/
  #define FF_INLINE_BLOCK_CALCULATIONS				/* In-lining of the block calculation			*/
  #define FF_64_NUM_SUPPORT							/* If int64_t is supported						*/
  #define FF_DRIVER_BUSY_SLEEP	20					/* Sleep time in ms when device is busy			*/
//#define FF_DEBUG									/* To enable FF_GetErrMessage() string messages	*/


  extern void *FFalloc(size_t Size);
  extern void  FFfree(void *Ptr);					/* Can't use OSalloc() because of freeing		*/
  #define FF_MALLOC(aSize)		FFalloc((size_t)aSize)
  #define FF_FREE(apPtr)	 	FFfree(apPtr)

/* ------------------------------------------------------------------------------------------------ */
/* End of configuration setting.																	*/
/* The rest are checks to validate the settings of the configuration								*/
/* ------------------------------------------------------------------------------------------------ */

#if defined(__CC_ARM)
  #define FF_INLINE static __inline					/* In-lining compiler keyword					*/
#else
  #define FF_INLINE static inline					/* In-lining compiler keyword					*/
#endif

#ifdef FF_LFN_SUPPORT
  #define FF_MAX_FILENAME		(260)
#else
  #define FF_MAX_FILENAME		(13)
#endif

#ifdef FF_USE_NATIVE_STDIO
  #ifdef MAX_PATH
	#define FF_MAX_PATH			MAX_PATH
  #elif PATH_MAX
	#define FF_MAX_PATH			PATH_MAX
  #else
	#define FF_MAX_PATH			2600
  #endif
#else
  #define FF_MAX_PATH			2600
#endif

#if ((!defined(FF_ALLOC_DOUBLE)) && (!defined(FF_ALLOC_DEFAULT)))
  #error "Must define FF_ALLOC_DOUBLE or FF_ALLOC_DEFAULT"
#endif

#if ((defined(FF_ALLOC_DOUBLE)) && (defined(FF_ALLOC_DEFAULT)))
  #error "Do not define both FF_ALLOC_DOUBLE and FF_ALLOC_DEFAULT"
#endif

#ifdef FF_UNICODE_SUPPORT
  #ifdef FF_HASH_CACHE
	#error "Can't enable HASH cache (FF_HASH_CACHE) with unicode (FF_UNICODE_SUPPORT)"
  #endif
  #ifdef FF_INCLUDE_SHORT_NAME
	#error "Short names (INCLUDE SHORT NAME) are not UNICODE compatible (FF_UNICODE_SUPPORT)"
  #endif
  #ifdef FF_UNICODE_UTF8_SUPPORT
	#error "Do not define both  FF_UNICODE_SUPPORT (UTF-16) and FF_UNICODE_UTF8_SUPPORT (UTF-8)"
  #endif
#endif

#ifndef FF_FAT_CHECK
  //#define FF_FAT_CHECK
#endif

#if ((!defined(FF_LITTLE_ENDIAN)) && (!defined(FF_BIG_ENDIAN)))
  #error "Must define FF_LITTLE_ENDIAN or FF_BIG_ENDIAN"
#endif

#if (defined(FF_LITTLE_ENDIAN) && defined(FF_BIG_ENDIAN))
  #error "Do not define both FF_LITTLE_ENDIAN and FF_BIG_ENDIAN"
#endif

#ifdef FF_HASH_CACHE
  #if ((FF_HASH_FUNCTION) == CRC16)
	#define FF_HASH_TABLE_SIZE		8192
  #elif ((FF_HASH_FUNCTION) == CRC8)
	#define FF_HASH_TABLE_SIZE		32
  #else
	#error "Invalid Hashing function (FF_HASH_CACHE) Select CRC16 or CRC8"
  #endif
#endif

#if (defined(FF_UNICODE_UTF8_SUPPORT) && defined(FF_UNICODE_SUPPORT))
  #error "Unicode support, use FF_UNICODE_SUPPORT (UTF-16) or FF_UNICODE_UTF8_SUPPORT (UTF-8)"
#endif

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
