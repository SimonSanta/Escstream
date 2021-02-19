/* ------------------------------------------------------------------------------------------------ */
/* FILE :		SysCall_FatFS.c																		*/
/*																									*/
/* CONTENTS :																						*/
/*				System call layer with underlying FatFS												*/
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
/*	$Revision: 1.25 $																				*/
/*	$Date: 2019/01/10 18:06:19 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* Limitations / non-standard things due to FAT														*/
/*																									*/
/* There is a limitation on the file access mode setting / use as FAT does not control access		*/
/* for group or others, nor does it control for readable or executable.  From the outside, this		*/
/* layer handles all the Unix access modes but only uses / sets the writability control. A file		*/
/* is set for read only if all three (owner / group / other) are set to non-writable. When a file	*/
/* is writable, its access mode is always reported as 0777 (rwxrwxrwx) and when its read-only, its	*/
/* access mode is always reported as 0555 (r-xr-xr-w).												*/
/*																									*/
/* open() (_open_r()) does not implement all flags, see the _open_r() function header				*/
/*																									*/
/* mount() and unmount() are non-standard															*/
/*																									*/
/* All mount points	must be at the root level.  This means mounting on "/dsk0" is valid, on "dsk0"	*/
/* is not and will trigger an error																	*/
/*																									*/
/* getcwd() is implemented according to the "C" library function prototype and not the Unix System	*/
/* Call as Newlib defines the "C" library function prototype										*/
/*																									*/
/* For the directory "C" library calls, the data type DIR is already employed by FatFS, therefore	*/
/* it is not possible to use this standard typedef.  Instead, the typedef DIR_t is used for all 	*/
/* "C" library directory calls.																		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------	*/

/* ------------------------------------------------------------------------------------------------ */
/* Limitations / non-standard things due to FatFS													*/
/*																									*/
/*																									*/
/* ------------------------------------------------------------------------------------------------	*/

/* ------------------------------------------------------------------------------------------------	*/
/* A note on the implementation:																	*/
/*																									*/
/* Internally, all file names and directory names are memorized and dealt with the physical			*/
/* physical representation:  "N:/..."																*/
/* This is why the function RealPath() is used everywhere a path is one of the function argument	*/
/* as it is where the physical path are determined. Doing so greatly simplifies the path handling	*/
/* because the mount points don't have to be dealt with anymore, only the physical drive # and		*/
/* RealPath always create the full path & file/dir name, not relying on the FS stack to keep track	*/
/* of the current working directory (CWD).															*/
/*																									*/
/* ------------------------------------------------------------------------------------------------	*/

/* ------------------------------------------------------------------------------------------------ */
/* void _exit(int status)																			*/
/* void *_sbrk_r(struct _reent *reent, ptrdiff_t incr)												*/
/*																									*/
/* Not in here because it is located in the ASM support file										*/
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */

#include "SysCall.h"

#include "ff.h"										/* FatFS include file							*/
#include "diskio.h"

/* ------------------------------------------------------------------------------------------------	*/

#ifndef OS_SYS_CALL
  #error "OS_SYS_CALL must be defined and set to non-zero to use the system call layer"
#else
  #if ((OS_SYS_CALL) == 0)
	#error "OS_SYS_CALL must be set to non-zero to use the system call layer"
  #endif
#endif

#ifndef SYS_CALL_N_FILE								/* Number of available file descriptors			*/
  #define SYS_CALL_N_FILE 		5					/* Excluding stdin, stdout & stderr				*/
#endif

#ifndef SYS_CALL_N_DIR								/* Number of available directory descriptors	*/
  #define SYS_CALL_N_DIR 		2
#endif

#ifndef SYS_CALL_N_DRV								/* Number of drives supported					*/
  #define SYS_CALL_N_DRV		_VOLUMES			/* Use as default FatFS set-up value			*/
#endif

#ifndef SYS_CALL_DEV_I2C							/* If supports the virtual /dev/i2cN for I2C	*/
  #define SYS_CALL_DEV_I2C		0					/* ==0 : not supported / != 0 supported			*/
#endif												/* -ve: supported with exclusive open()			*/

#ifndef SYS_CALL_DEV_SPI							/* If supports the virtual /dev/SPINN for SPI	*/
  #define SYS_CALL_DEV_SPI		0					/* ==0 : not supported / != 0 supported			*/
#endif												/* -ve: supported with exclusive open()			*/

#ifndef SYS_CALL_DEV_TTY							/* If supports the virtual /dev/ttyN for UART	*/
  #define SYS_CALL_DEV_TTY		0					/* ==0 : not supported / != 0 supported			*/
#endif

#ifndef SYS_CALL_TTY_EOF							/* If != 0, EOF character when reading a TTY	*/
  #define SYS_CALL_TTY_EOF		0					/* If is |= 0x80000000, the EOF trapping also	*/
#endif												/* applied to stdin/ stdout / stderr			*/

#ifndef SYS_CALL_MUTEX								/* -ve: individual descriptors not protected	*/
  #define SYS_CALL_MUTEX		0					/* ==0: descriptors share one mutex				*/
#endif												/* +ve: each descriptor has its own mutex		*/

#ifndef SYS_CALL_OS_MTX
  #define SYS_CALL_OS_MTX		0					/* If the global mutex is G_OSmutex or not		*/
#endif												/* != 0 is G_OSmutex / == 0 is local			*/

#if ((SYS_CALL_N_FILE) < 0)
  #error "SYS_CALL_N_FILE must be >= 0"
#endif

#if ((SYS_CALL_N_DIR) < 0)
  #error "SYS_CALL_N_DIR must be >= 0"
#endif

#if ((SYS_CALL_N_DRV) < 0)
  #error "SYS_CALL_N_DRV must be >= 0"
#endif

#if (((_FS_LOCK) == 0) && ((_FS_READONLY) != 0))
  #error "_FS_LOCK must non-zero for safe operation"
#endif

#if ((_STR_VOLUME_ID) != 0)
  #error "Do not enable FatFS's volume ID strings (_STR_VOLUME_ID)"
#endif
#if ((_FS_REENTRANT) == 0)
  #error "FatFS must be re-entrant safe (set _FS_REENT to non-zero / add Abassi_FatFS.c to project"
#endif

#if ((SYS_CALL_N_DRV) < (_VOLUMES))
  #error "SYS_CALL_N_DRV < _VOLUMES. SYS_CALL_N_DRV must be set greater or equal to FatFS's _VOLUMES "
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Internal definitions																				*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef SYS_CALL_MULTI_FATFS
  #define SYS_CALL_MULTI_FATFS		0				/* By default, single file system stack			*/
#endif

#if ((SYS_CALL_MULTI_FATFS) != 0)					/* When more than 1 FS stack is used			*/
  #define P_NM _FatFS								/* Each ones of the function in here are		*/
#else												/* renamed with the post-fix "_FatFS".  The		*/
  #define P_NM										/* non-post-fix function is in the multi-FS		*/
#endif												/* dispatcher									*/

#define SYS_CALL_DEV_USED	   (((SYS_CALL_DEV_I2C) != 0)											\
                             || ((SYS_CALL_DEV_SPI) != 0)											\
                             || ((SYS_CALL_DEV_TTY) != 0))

#define DEV_TYPE_I2C		(1)
#define DEV_TYPE_SPI		(2)
#define DEV_TYPE_TTY		(3)

/* ------------------------------------------------------------------------------------------------ */
/* Local variables																					*/
/* ------------------------------------------------------------------------------------------------ */
/* This one needs a bit of explanation.																*/
/* Normally, one would use simply																	*/
/* 	      FATfs  Fsys;																				*/
/* and not 																							*/
/* 	      FATfs  Fsys;																				*/
/*        uint8_t Pad[OX_CACHE_LSIZE];																*/
/* 																									*/
/* And this should work without issues. But there is no guarantee the field buf[_MAX_SS] in FATS	*/
/* will be aligned on 32 bytes (cache line alignment), so when the SD/MMC driver is used it will	*/
/* flush the data after the end of buf[_MAX_SS] when non-aligned. This should not be a problem		*/
/* but it can be depending on what is that extra data. One bad case is that data is involved in a 	*/
/* DMA transfer dealt by another task.																*/
/* The padding fills the extra data that will be flushed, safeguarding other data.					*/
/* The same issue exists for the data preceding the buffer as the smallest number of bytes in the	*/
/* the FATFS data structure that preceded the buffer is 10 bytes, so a padding of 22 is needed.		*/
/* Instead, we align the whole data structure on 32 bytes, so the before padding is not needed		*/

typedef struct {
	FATFS   Fsys;									/* FatFS file system descriptor					*/
	uint8_t Pad[OX_CACHE_LSIZE];					/* See text above								*/
	char    MntPoint[SYS_CALL_MAX_PATH+1];			/* Where the drive is mounted					*/
	int     Flags;									/* Flags needed to know if RO or R/W			*/
	int     Type;									/* Type of FS: FAT11 / FAT16 / FAT32 / exFAT	*/
} SysMnt_t
#if !defined(__IAR_SYSTEMS_ICC__)
 __attribute__ ((aligned (OX_CACHE_LSIZE)))
#endif
;

typedef struct {									/* Our own file descriptors						*/
	int    IsOpen;									/* If the file is open							*/
	FIL    Fdesc;									/* FatFS file descriptor						*/
	int    Sync;									/* If the  writes must be flushed every time	*/
	int    omode;									/* File open access mode						*/
	int    umode;									/* When file created, mode to set after closing	*/
  #if ((SYS_CALL_DEV_USED) != 0)
	int    DevNmb;
	int    DevType;
   #if ((SYS_CALL_TTY_EOF) != 0)
	int IsEOF;										/* Flag to remember if EOF has been received	*/
   #endif
  #endif
  #if ((SYS_CALL_MUTEX) >= 0)						/* Single / individual mutex(es) requested		*/
	MTX_t *MyMtx;									/* When single, all [] hold the same mutex		*/
  #endif
	char  Fname[SYS_CALL_MAX_PATH+1];
} SysFile_t;


typedef struct SysDir {								/* Our own directory descriptors				*/
	int   IsOpen;									/* If the directory is open						*/
	int   Drv;										/* Drive # where this dir is located			*/
	DIR   Ddesc;									/* FatFS directory descriptor					*/
	DIR   Dfirst;									/* First entry in the open directory (for seek)	*/
	int   Offset;									/* Current offset reading the directory			*/
	int   MntIdx;									/* Index reading mount points					*/
	struct dirent Entry;							/* Latest directory entry that was read			*/
  #if ((SYS_CALL_MUTEX) >= 0)						/* Single / individual mutex(es) requested		*/
	MTX_t *MyMtx;									/* When single, all [] hold the same mutex		*/
  #endif
	char  Lpath[SYS_CALL_MAX_PATH+1];				/* Logical path of the directory				*/
} SysDir_t;


static MTX_t    *g_GlbMtx;
static SysMnt_t  g_SysMnt[SYS_CALL_N_DRV];
static SysFile_t g_SysFile[SYS_CALL_N_FILE];
static SysDir_t  g_SysDir[SYS_CALL_N_DIR];

extern int G_UartDevIn;								/* Must be supplied by the application because	*/
extern int G_UartDevOut;							/* the UART driver must know the device to use	*/
extern int G_UartDevErr;

/* ------------------------------------------------------------------------------------------------ */
/* These macros simplify the manipulation of the mutexes											*/
/* DIR_ : to protect a directory descriptor															*/
/* FIL_ : to protect a file descriptor																*/

#if ((SYS_CALL_MUTEX) <  0)							/* Not protected								*/
  #define DIR_MTXlock(x)	0
  #define DIR_MTXunlock(x)	OX_DO_NOTHING()
  #define FIL_MTXlock(x)	0
  #define FIL_MTXunlock(x)	OX_DO_NOTHING()
#else												/* Descriptors have a mutex (same or shared)	*/
  #define DIR_MTXlock(x)	MTXlock((x)->MyMtx ,-1)
  #define DIR_MTXunlock(x)	MTXunlock((x)->MyMtx)
  #define FIL_MTXlock(x)	MTXlock(g_SysFile[x].MyMtx ,-1)
  #define FIL_MTXunlock(x)	MTXunlock(g_SysFile[x].MyMtx)
#endif

/* ------------------------------------------------------------------------------------------------ */
/* These function prototypes are hard-coded here because the ???_i2c.h, ???_uart.h etc are			*/
/* platform specific																				*/

#if ((SYS_CALL_DEV_I2C) != 0)						/* To not have warning about unused declaration	*/
  extern int i2c_init          (int Dev, int AddBits, int Freq);
  extern int i2c_recv          (int Dev, int Target, char *Buf, int Len);
  extern int i2c_send          (int Dev, int Target, const char *Buf, int Len);
#endif

#if ((SYS_CALL_DEV_SPI) != 0)						/* To not have warning about unused declaration	*/
  extern int spi_init			(int Dev, int Slv, int Freq, int Nbits, int Mode);
  extern int spi_recv			(int Dev, int Slv, void *Buf, uint32_t Len);
  extern int spi_send			(int Dev, int Slv, const void *Buf, uint32_t Len);
#endif

#if ((SYS_CALL_DEV_TTY) != 0)						/* To not have warning about unused declaration	*/
  extern int uart_init        (int Dev, int Baud, int Nbits, int Parity, int Stop, int RXsize,
                               int TXsize, int Filter);
#endif
  extern int uart_recv        (int Dev, char *Buff, int Len);
  extern int uart_send        (int Dev, const char *Buff, int Len);

/* ------------------------------------------------------------------------------------------------ */
/* These macros make this file use-able by multiple libraries										*/

#define SC_SYSCALLINIT_(P1,x)					SX_SYSCALLINIT_(P1,x)
#define SX_SYSCALLINIT_(P2,x)					SysCallInit##P2(x)
#define SC_DUP_(P1,x)							SX_DUP_(P1,x)
#define SX_DUP_(P2,x)							dup##P2(x)
#define SC_GETCWD_(P1,x,y)						SX_GETCWD_(P1,x,y)
#define SX_GETCWD_(P2,x,y)						getcwd##P2(x,y)
#define SC_CHDIR_(P1,x)							SX_CHDIR_(P1,x)
#define SX_CHDIR_(P2,x)							chdir##P2(x)
#define SC_FSTATFS_(P1,x,y)						SX_FSTATFS_(P1,x,y)
#define SX_FSTATFS_(P2,x,y)						fstatfs##P2(x,y)
#define SC_STATFS_(P1,x,y)						SX_STATFS_(P1,x,y)
#define SX_STATFS_(P2,x,y)						statfs##P2(x,y)
#define SC_CHOWN_(P1,x,y,z)						SX_CHOWN_(P1,x,y,z)
#define SX_CHOWN_(P2,x,y,z)						chown##P2(x,y,z)
#define SC_UMASK_(P1,x)							SX_UMASK_(P1,x)
#define SX_UMASK_(P2,x)							umask##P2(x)
#define SC_CHMOD_(P1,x,y)						SX_CHMOD_(P1,x,y)
#define SX_CHMOD_(P2,x,y)						chmod##P2(x,y)
#define SC_MOUNT_(P1,a,b,c,d)					SX_MOUNT_(P1,a,b,c,d)
#define SX_MOUNT_(P2,a,b,c,d)					mount##P2(a,b,c,d)
#define SC_UNMOUNT_(P1,a,b)						SX_UNMOUNT_(P1,a,b)
#define SX_UNMOUNT_(P2,a,b)						unmount##P2(a,b)
#define SC_MKFS_(P1,a,b)						SX_MKFS_(P1,a,b)
#define SX_MKFS_(P2,a,b)						mkfs##P2(a,b)
#define SC_OPENDIR_(P1,x)						SX_OPENDIR_(P1,x)
#define SX_OPENDIR_(P2,x)						opendir##P2(x)
#define SC_CLOSEDIR_(P1,x)						SX_CLOSEDIR_(P1,x)
#define SX_CLOSEDIR_(P2,x)						closedir##P2(x)
#define SC_READDIR_(P1,x)						SX_READDIR_(P1,x)
#define SX_READDIR_(P2,x)						readdir##P2(x)
#define SC_TELLDIR_(P1,x)						SX_TELLDIR_(P1,x)
#define SX_TELLDIR_(P2,x)						telldir##P2(x)
#define SC_SEEKDIR_(P1,a,b)						SX_SEEKDIR_(P1,a,b)
#define SX_SEEKDIR_(P2,a,b)						seekdir##P2(a,b)
#define SC_REWINDDIR_(P1,x)						SX_REWINDDIR_(P1,x)
#define SX_REWINDDIR_(P2,x)						rewinddir##P2(x)
#define SC_DEVCTL_(P1,a,b)						SX_DEVCTL_(P1,a,b)
#define SX_DEVCTL_(P2,a,b)						devctl##P2(a,b)
#define SC_GETKEY_(P1,x)						SX_GETKEY_(P1,x)
#define SX_GETKEY_(P2,x)						GetKey##P2(x)
#define SC_SYS_ENSURE_(P1,x)					SX_SYS_ENSURE_(P1,x)
#define SX_SYS_ENSURE_(P2,x)					_sys_ensure##P2(x)
#define SC_SYS_FLEN_(P1,x)						SX_SYS_FLEN_(P1,x)
#define SX_SYS_FLEN_(P2,x)						_sys_flen##P2(x)
#define SC_IARFLUSH_(P1,x)						SX_IARFLUSH_(P1,x)
#define SX_IARFLUSH_(P2,x)						IARflush##P2(x)


#if defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__)
  #define SC_CLOSE_(P1, Reen, Fd)				SX_CLOSE_(P1, Fd)
  #define SX_CLOSE_(P2, Fd)						close##P2(Fd)
  #define SC_FSTAT_(P1, Reen, Fd, St)			SX_FSTAT_(P1, Fd, St)
  #define SX_FSTAT_(P2, Fd, St)					fstat##P2(Fd, St)
  #define SC_ISATTY_(P1, Reen, Fd)				SX_ISATTY_(P1, Fd)
  #define SX_ISATTY_(P2, Fd)					isatty##P2(Fd)
  #define SC_LSEEK_(P1, Reen, Fd, Off, Wh)		SX_LSEEK_(P1, Fd, Off, Wh)
  #define SX_LSEEK_(P2, Fd, Off, Wh)			lseek##P2(Fd, Off, Wh)
  #define SC_MKDIR_(P1, Reen, Pat, Mod)			SX_MKDIR_(P1, Pat, Mod)
  #define SX_MKDIR_(P2, Pat, Mod)				mkdir##P2(Pat, Mod)
  #define SC_OPEN_(P1, Reen, Pat, Flg, Mod) 	SX_OPEN_(P1, Pat, Flg, Mod)
  #define SX_OPEN_(P2, Pat, Flg, Mod) 			open##P2(Pat, Flg, Mod)
  #define SC_READ_(P1, Reen, Fd, Bf, Sz)		SX_READ_(P1, Fd, Bf, Sz)
  #define SX_READ_(P2, Fd, Bf, Sz)				read##P2(Fd, Bf, Sz)
  #define SC_RENAME_(P1, Reen, Od, Nw)			SX_RENAME_(P1, Od, Nw)
  #define SX_RENAME_(P2, Od, Nw)				rename_r##P2(Od, Nw)
  #define SC_STAT_(P1, Reen, Pat, St)			SX_STAT_(P1, Pat, St)
  #define SX_STAT_(P2, Pat, St)					stat##P2(Pat, St)
  #define SC_UNLINK_(P1, Reen, Pat)				SX_UNLINK_(P1, Pat)
  #define SX_UNLINK_(P2, Pat)					unlink##P2(Pat)
  #define SC_WRITE_(P1, Reen, Fd, Bf, Sz)		SX_WRITE_(P1, Fd, Bf, Sz)
  #define SX_WRITE_(P2, Fd, Bf, Sz)				write##P2(Fd, Bf, Sz)

  #define REENT_ERRNO							(errno)
  #define REENT_ERRNO_							(errno)

  #if ((SYS_CALL_MULTI_FATFS) != 0)
	int fstat_FatFS(int fd, struct stat *pstat);
	int fstatfs_FatFS(int fs, struct statfs *buf);
	int stat_FatFS(const char *path, struct stat *pstat);
	int unlink_FatFS(const char *path);
  #endif

#elif defined(__GNUC__)
  #define SC_CLOSE_(P1, Reen, Fd)				SX_CLOSE_(P1, Reen, Fd)
  #define SX_CLOSE_(P2, Reen, Fd)				_close_r##P2(Reen, Fd)
  #define SC_FSTAT_(P1, Reen, Fd, St)			SX_FSTAT_(P1, Reen, Fd, St)
  #define SX_FSTAT_(P2, Reen, Fd, St)			_fstat_r##P2(Reen, Fd, St)
  #define SC_ISATTY_(P1, Reen, Fd)				SX_ISATTY_(P1, Reen, Fd)
  #define SX_ISATTY_(P2, Reen, Fd)				_isatty_r##P2(Reen, Fd)
  #define SC_LSEEK_(P1, Reen, Fd, Off, Wh)		SX_LSEEK_(P1, Reen, Fd, Off, Wh)
  #define SX_LSEEK_(P2, Reen, Fd, Off, Wh)		_lseek_r##P2(Reen, Fd, Off, Wh)
  #define SC_MKDIR_(P1, Reen, Pat, Mod)			SX_MKDIR_(P1, Reen, Pat, Mod)
  #define SX_MKDIR_(P2, Reen, Pat, Mod)			_mkdir_r##P2(Reen, Pat, Mod)
  #define SC_OPEN_(P1, Reen, Pat, Flg, Mod) 	SX_OPEN_(P1, Reen, Pat, Flg, Mod)
  #define SX_OPEN_(P2, Reen, Pat, Flg, Mod) 	_open_r##P2(Reen, Pat, Flg, Mod)
  #define SC_READ_(P1, Reen, Fd, Bf, Sz)		SX_READ_(P1, Reen, Fd, Bf, Sz)
  #define SX_READ_(P2, Reen, Fd, Bf, Sz)		_read_r##P2(Reen, Fd, Bf, Sz)
  #define SC_RENAME_(P1, Reen, Od, Nw)			SX_RENAME_(P1, Reen, Od, Nw)
  #define SX_RENAME_(P2, Reen, Od, Nw)			_rename_r##P2(Reen, Od, Nw)
  #define SC_STAT_(P1, Reen, Pat, St)			SX_STAT_(P1, Reen, Pat, St)
  #define SX_STAT_(P2, Reen, Pat, St)			_stat_r##P2(Reen, Pat, St)
  #define SC_UNLINK_(P1, Reen, Pat)				SX_UNLINK_(P1, Reen, Pat)
  #define SX_UNLINK_(P2, Reen, Pat)				_unlink_r##P2(Reen, Pat)
  #define SC_WRITE_(P1, Reen, Fd, Bf, Sz)		SX_WRITE_(P1, Reen, Fd, Bf, Sz)
  #define SX_WRITE_(P2, Reen, Fd, Bf, Sz)		_write_r##P2(Reen, Fd, Bf, Sz)

													/* If Abassi does not supply __getreent(),		*/
  #if ((OX_LIB_REENT_PROTECT) == 0)					/* back to _impure_ptr							*/
	#define __getreent()					_impure_ptr
  #endif

  #define REENT_ERRNO						(reent->_errno)
  #define REENT_ERRNO_						(__getreent()->_errno)

  #if ((SYS_CALL_MULTI_FATFS) != 0)
	int _fstat_r_FatFS(struct _reent *reent, int fd, struct stat *pstat);
	int _fstatfs_r_FatFS(struct _reent *reent, int fs, struct statfs *buf);
	int _stat_r_FatFS(struct _reent *reent, const char *path, struct stat *pstat);
	int _unlink_r_FatFS(struct _reent *reent, const char *path);
  #endif

  #if (((OS_N_CORE) >= 2) && defined(OS_NEWLIB_REDEF))
   #if ((OS_NEWLIB_REDEF) != 0)
	#undef close
	#undef fstat
	#undef isatty
	#undef lseek
	#undef mkdir
	#undef open
	#undef read
	#undef rename
	#undef stat
	#undef unlink
	#undef write
   #endif
  #endif

  #if ((SYS_CALL_MULTI_FATFS) == 0)
	#define SC__CLOSE(P1, Fd)				SX__CLOSE(P1, Fd)
	#define SX__CLOSE(P2, Fd)				_close##P2(Fd)
	#define SC__FSTAT(P1, Fd, St)			SX__FSTAT(P1, Fd, St)
	#define SX__FSTAT(P2, Fd, St)			_fstat##P2(Fd, St)
	#define SC__ISATTY(P1, Fd)				SX__ISATTY(P1, Fd)
	#define SX__ISATTY(P2, Fd)				_isatty##P2(Fd)
	#define SC__LSEEK(P1, Fd, Off, Wh)		SX__LSEEK(P1, Fd, Off, Wh)
	#define SX__LSEEK(P2, Fd, Off, Wh)		_lseek##P2(Fd, Off, Wh)
	#define SC__MKDIR(P1, Pat, Mod)			SX__MKDIR(P1, Pat, Mod)
	#define SX__MKDIR(P2, Pat, Mod)			mkdir##P2(Pat, Mod)
	#define SC___MKDIR(P1, Pat, Mod)		SX___MKDIR(P1, Pat, Mod)
	#define SX___MKDIR(P2, Pat, Mod)		_mkdir##P2(Pat, Mod)
	#define SC__OPEN(P1, Pat, Flg, Mod) 	SX__OPEN(P1, Pat, Flg, Mod)
	#define SX__OPEN(P2, Pat, Flg, Mod) 	_open##P2(Pat, Flg, Mod)
	#define SC__READ(P1, Fd, Bf, Sz)		SX__READ(P1, Fd, Bf, Sz)
	#define SX__READ(P2, Fd, Bf, Sz)		_read##P2(Fd, Bf, Sz)
	#define SC__RENAME(P1, Od, Nw)			SX__RENAME(P1, Od, Nw)
	#define SX__RENAME(P2, Od, Nw)			_rename##P2(Od, Nw)
	#define SC__STAT(P1, Pat, St)			SX__STAT(P1, Pat, St)
	#define SX__STAT(P2, Pat, St)			_stat##P2(Pat, St)
	#define SC__UNLINK(P1, Pat)				SX__UNLINK(P1, Pat)
	#define SX__UNLINK(P2, Pat)				_unlink##P2(Pat)
	#define SC__WRITE(P1, Fd, Bf, Sz)		SX__WRITE(P1, Fd, Bf, Sz)
	#define SX__WRITE(P2, Fd, Bf, Sz)		_write##P2(Fd, Bf, Sz)

	int SC__CLOSE(P_NM, int fd) {
	  return(SC_CLOSE_(P_NM, __getreent(), fd));
	}
	int SC__FSTAT(P_NM, int fd, struct stat *pstat) {
	  return(SC_FSTAT_(P_NM, __getreent(), fd, pstat));
	}
	int SC__ISATTY(P_NM, int fd) {
	  return(SC_ISATTY_(P_NM, __getreent(), fd));
	}
	off_t SC__LSEEK(P_NM, int fd, off_t offset, int whence) {
	  return(SC_LSEEK_(P_NM, __getreent(), fd, offset, whence));
	}
	int SC__MKDIR(P_NM, const char *path, mode_t mode) {
	  return(SC_MKDIR_(P_NM, __getreent(), path, (int)mode));
	}
	int SC___MKDIR(P_NM, const char *path, mode_t mode) {
	  return(SC_MKDIR_(P_NM, __getreent(), path, (int)mode));
	}
	int SC__OPEN(P_NM, char *path, int flags, int mode) {
	  return(SC_OPEN_(P_NM, __getreent(), path, flags, mode));
	}
	int SC__READ(P_NM, int fd, void *vbuf, size_t size) {
	  return(SC_READ_(P_NM, __getreent(), fd, vbuf, size));
	}
	int SC__RENAME(P_NM, const char *old, const char *anew) {
	  return(SC_RENAME_(P_NM, __getreent(), old, anew));
	}
	int SC__STAT(P_NM, const char *path, struct stat *pstat) {
	  return(SC_STAT_(P_NM, __getreent(), path, pstat));
	}
	int SC__UNLINK(P_NM, const char *path) {
	  return(SC_UNLINK_(P_NM, __getreent(), path));
	}
	int SC__WRITE(P_NM, int fd, const void *vbuf, size_t size) {
	  return(SC_WRITE_(P_NM, __getreent(), fd, vbuf, size));
	}
  #endif
#endif

#if ((SYS_CALL_MULTI_FATFS) != 0)
  int            closedir_FatFS(DIR_t *dirp);
#endif

#define MY_FAT_NAME	"FatFS"

#if ((SYS_CALL_MUTEX) >  0)	
static const char g_DdscName[][14] = {
                                      "Ddsc " MY_FAT_NAME "  0"
                                   #if ((SYS_CALL_N_DIR) > 1)
                                     ,"Ddsc " MY_FAT_NAME "  1"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 2)
                                     ,"Ddsc " MY_FAT_NAME "  2"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 3)
                                     ,"Ddsc " MY_FAT_NAME "  3"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 4)
                                     ,"Ddsc " MY_FAT_NAME "  4"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 5)
                                     ,"Ddsc " MY_FAT_NAME "  5"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 6)
                                     ,"Ddsc " MY_FAT_NAME "  6"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 7)
                                     ,"Ddsc " MY_FAT_NAME "  7"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 8)
                                     ,"Ddsc " MY_FAT_NAME "  8"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 9)
                                     ,"Ddsc " MY_FAT_NAME "  9"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 10)
                                     ,"Ddsc " MY_FAT_NAME " 10"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 11)
                                     ,"Ddsc " MY_FAT_NAME " 11"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 12)
                                     ,"Ddsc " MY_FAT_NAME " 12"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 13)
                                     ,"Ddsc " MY_FAT_NAME " 13"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 14)
                                     ,"Ddsc " MY_FAT_NAME " 14"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 15)
                                     ,"Ddsc " MY_FAT_NAME " 15"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 16)
                                     ,"Ddsc " MY_FAT_NAME " 16"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 17)
                                     ,"Ddsc " MY_FAT_NAME " 17"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 18)
                                     ,"Ddsc " MY_FAT_NAME " 18"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 19)
                                     ,"Ddsc " MY_FAT_NAME " 19"
                                   #endif
                                   #if ((SYS_CALL_N_DIR) > 20)
                                     #error "Increase g_DdscName array for mutex names"
                                   #endif
                                     };
static const char g_FdscName[][14] = {
                                      "Fdsc " MY_FAT_NAME "  0"
                                   #if ((SYS_CALL_N_FILE) > 1)
                                     ,"Fdsc " MY_FAT_NAME "  1"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 2)
                                     ,"Fdsc " MY_FAT_NAME "  2"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 3)
                                     ,"Fdsc " MY_FAT_NAME "  3"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 4)
                                     ,"Fdsc " MY_FAT_NAME "  4"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 5)
                                     ,"Fdsc " MY_FAT_NAME "  5"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 6)
                                     ,"Fdsc " MY_FAT_NAME "  6"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 7)
                                     ,"Fdsc " MY_FAT_NAME "  7"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 8)
                                     ,"Fdsc " MY_FAT_NAME "  8"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 9)
                                     ,"Fdsc " MY_FAT_NAME "  9"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 10)
                                     ,"Fdsc " MY_FAT_NAME " 10"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 11)
                                     ,"Fdsc " MY_FAT_NAME " 11"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 12)
                                     ,"Fdsc " MY_FAT_NAME " 12"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 13)
                                     ,"Fdsc " MY_FAT_NAME " 13"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 14)
                                     ,"Fdsc " MY_FAT_NAME " 14"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 15)
                                     ,"Fdsc " MY_FAT_NAME " 15"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 16)
                                     ,"Fdsc " MY_FAT_NAME " 16"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 17)
                                     ,"Fdsc " MY_FAT_NAME " 17"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 18)
                                     ,"Fdsc " MY_FAT_NAME " 18"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 19)
                                     ,"Fdsc " MY_FAT_NAME " 19"
                                   #endif
                                   #if ((SYS_CALL_N_FILE) > 20)
                                     #error "Increase g_FdscName array for mutex names"
                                   #endif
                                     };
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Local defintions as no Media_FatFS.h available													*/

extern int32_t     media_blksize_FatFS  (int Drv);
extern const char *media_info_FatFS     (int Drv);
extern int32_t     media_sectsize_FatFS (int Drv);
extern int64_t     media_size_FatFS     (int Drv);
extern int         media_uninit_FatFS   (int Drv);

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    SysCallInit																			*/
/*																									*/
/* SysCallInit - initialize the System Call layer (it must be called after OSstart())				*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void SysCallInit(void);																		*/
/*																									*/
/* ARGUMENTS:																						*/
/*																									*/
/* RETURN VALUE:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void SC_SYSCALLINIT_(P_NM, void)
{
#if ((SYS_CALL_MUTEX) >= 0)
  int ii;											/* General purpose								*/
#endif

  #if ((SYS_CALL_OS_MTX) == 0)						/* Using a local mutex							*/
	g_GlbMtx = MTXopen("SysCall " MY_FAT_NAME);		/* The global mutex is always required			*/
  #else
	g_GlbMtx = G_OSmutex;							/* Using the OS global mutex					*/
  #endif

	memset(&g_SysFile[0], 0, sizeof(g_SysFile));
	memset(&g_SysDir[0],  0, sizeof(g_SysDir));
	memset(&g_SysMnt[0],  0, sizeof(g_SysMnt));

  #if ((SYS_CALL_MUTEX) == 0)
	g_SysFile[0].MyMtx = MTXopen("Fdsc " MY_FAT_NAME);
	for (ii=1 ; ii<(int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])) ; ii++) {
		g_SysFile[ii].MyMtx = g_SysFile[0].MyMtx;
	}
	g_SysDir[0].MyMtx = MTXopen("Ddsc " MY_FAT_NAME);
	for (ii=1 ; ii<(int)(sizeof(g_SysDir)/sizeof(g_SysDir[0])) ; ii++) {
		g_SysDir[ii].MyMtx = g_SysDir[0].MyMtx;
	}
  #elif ((SYS_CALL_MUTEX) >  0)	
	for (ii=0 ; ii<(int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])) ; ii++) {
		g_SysFile[ii].MyMtx = MTXopen(&g_FdscName[ii][0]);
	}
	for (ii=0 ; ii<(int)(sizeof(g_SysDir)/sizeof(g_SysDir[0])) ; ii++) {
		g_SysDir[ii].MyMtx = MTXopen(&g_DdscName[ii][0]);
	}
  #endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    LogPath																				*/
/*																									*/
/* LogPath - create the full logical path of a file													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int LogPath(char *Log, const char *Path);													*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Log  : result for the logical path (always starts "/")										*/
/*		Path : logical/physical  path/file name to convert (using mount points & CWD)				*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : OK																					*/
/*		== -1 : invalid path																		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
static int LogPath(char *Log, const char *Path)
{
char  *Cwd;											/* Current working directory of the task		*/
int    Drv;											/* Physical drive for that path					*/
int    ii;											/* General purpose								*/
int    jj;											/* General purpose								*/
int    Len;											/* Length of a string							*/
char  *Ptr1;										/* General purpose to manipulate strings		*/
char  *Ptr2;										/* General purpose to manipulate strings		*/
TSK_t *MyTask;										/* Current task									*/

	MyTask = TSKmyID();
	Cwd    = (char *)(MyTask->SysCall[0]);			/* De-reference for easier access				*/
	if (Cwd == (char *)NULL) {						/* First time this task uses a system call		*/
		Cwd                = (char *)OSalloc(SYS_CALL_MAX_PATH+1);
		MyTask->SysCall[0] = (intptr_t)Cwd;			/* Allocate memory to hold the path string		*/
		Cwd[0] = '\0';								/* Set path to "" (is invalid)					*/
		if (MyTask->SysCall[1] == (intptr_t)0) {	/* If umask not set, set to default				*/
			MyTask->SysCall[1] = (intptr_t)~0022;	/* Is used inverted and invert guarantees != 0	*/
		}
	}

	Log[0] = '\0';
	Log[SYS_CALL_MAX_PATH] = '\0';					/* Use strncpy() with max of SYS_CALL_MAX_PATH	*/

	if ((Path[0] == '\0')							/* Empty path or . means current directory		*/
	|| ((Path[0] == '.') && (Path[1] == '\0'))) {
		Path = (const char *)MyTask->SysCall[0];
	}

	if (Path[1] == ':') {							/* If starts with N: then the path is physical	*/
		if (isdigit((int)Path[0])) {				/* N is a digit									*/
			if ((Path[2] == '/')					/* Need either "N:" or "N:/"					*/
			||  (Path[2] == '\0')) {
				Drv = Path[0] - '0';
				if (Drv >= (sizeof(g_SysMnt)/sizeof(g_SysMnt[0]))) {
					return(-1);						/* Drive # out of range							*/
				}					
				strcpy(Log, g_SysMnt[Drv].MntPoint);
				ii = strlen(Log);
				strncpy(&Log[ii], &Path[2], SYS_CALL_MAX_PATH-ii);
			}
			else {
				return(-1);							/* Is "N:...", but not "N:/" or "N:" only		*/
			}
		}
		else {										/* N is not a digit								*/
			return(-1);
		}
	}
	else if (Path[0] == '/') {						/* If is an absolute logical path, copy as is	*/
		strcpy(&Log[0], &Path[0]);
	}
	else {											/* Is a relative path, add task's cwd path		*/
		if (Cwd[0] == '\0') {						/* The CW hasn't been set, find a good path		*/
			for (Drv=(sizeof(g_SysMnt)/sizeof(g_SysMnt[0]))-1 ; Drv >=0 ; Drv--) {
				if (g_SysMnt[Drv].MntPoint[0] != '\0') {	/* Find one that is mounted				*/
					break;
				}
			}
			if (Drv < 0) {							/* No drive mounted, can't obtain a real path	*/
				return(-1);
			}
			strcpy(&Cwd[0], &g_SysMnt[Drv].MntPoint[0]);
		}

		strcpy(Log, &Cwd[0]);						/* Put current workin directory					*/
		Len = SYS_CALL_MAX_PATH						/* Remainder room in the destination string		*/
		    - strlen(Log);
		if (Len > 0) {
			strcat(Log, "/");						/* Play safe									*/
			Len--;
		}
		if (Len > 0) {
			strncat(Log, Path, Len);				/* Concatenate CW with the supplied path		*/
		}
	}

	ii = 0;
	jj = 0;
	do {											/* Replace multiple "/" with a single "/"		*/
		jj++;
		if ((Log[jj] != '/')
		||  (Log[ii] != '/')) {
			Log[++ii] = Log[jj];
		}
	} while(Log[jj] != '\0');

	while(NULL != (Ptr1 = strstr(Log, "/.."))) {	/* Deal with "/.." (up one dir) in the path		*/
		if (Ptr1 == Log) {							/* Is already at the root, can't go higher		*/
			memmove(&Log[1], &Log[3], SYS_CALL_MAX_PATH-2);	/* Remove the trailing "/.."			*/
		}
		else {										/* Not at the root. find 1st '/' before "/.."	*/
			Ptr1[0] = '\0';
			Ptr2    = strrchr(Log, '/');
			memmove(Ptr2, Ptr1+3, (SYS_CALL_MAX_PATH-2)-(Ptr1-Log));
		}

		if (Log[0] == '\0') {						/* If empty, so is at the root level			*/
			Log[0] = '/';							/* The path is then simply "/"					*/
			Log[1] = '\0';
		}
	}

	Len = strlen(Log)-1;							/* Remove all trailing "/"						*/
	while ((Log[Len] == '/')						/* Make sure to keep root dir lone "/"			*/
	&&     (Len > 0)) {
		Log[Len--] = '\0';
	}

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    RealPath																			*/
/*																									*/
/* RealPath - create the full physical (real) path of a file										*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int RealPath(char *Phy, const char *Log, int *Dev);											*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Phy : result for the physical path (always starts with "N:/")								*/
/*		Log : logical path/file name to convert (using mount points & CWD)							*/
/*		Dev : when non-null and Log is a /dev, returns the device # in there						*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>=  0 : drive number for Log																*/
/*		== -1 : invalid path																		*/
/*		== (-((DEV_TYPE_I2C)+1)) : when it's a /dev/i2c device and I2C devices are supported 		*/
/*		== (-((DEV_TYPE_SPI)+1)) : when it's a /dev/spi device and SPI devices are supported 		*/
/*		== (-((DEV_TYPE_TTY)+1)) : when it's a /dev/tty device and TTY devices are supported 		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int RealPath(char *Phy, const char *Log, int *Dev)
{
int    Drv;											/* Physical drive for that path					*/
int    ii;											/* General purpose								*/
int    jj;											/* General purpose								*/
int    Len;											/* Length of a string							*/


	Phy += 2;										/* Make room for the driver # & ":"				*/
	LogPath(&Phy[0], Log);							/* Get full logical path name					*/
	Drv = -1;										/* Even if log path wrong, keep going for /dev	*/

  #if ((SYS_CALL_DEV_I2C) != 0)
	if (0 == strncasecmp(&Phy[0], "/dev/i2c", 8)) {
		if ((!isdigit((int)Phy[8]))					/* Must be a single digit appended to /dev/i2c	*/
		||  (Phy[9] != '\0')) {
			return(-1);
		}
		Drv = (-((DEV_TYPE_I2C)+1));
		if (Dev != (int *)NULL) {
			*Dev = Phy[8]-'0';
		}
	}
  #endif

  #if ((SYS_CALL_DEV_SPI) != 0)
	if (0 == strncasecmp(&Phy[0], "/dev/spi", 8)) {
		if ((!isdigit((int)Phy[8]))					/* Must be a dual digit appended to /dev/spi	*/
		||  (!isdigit((int)Phy[9]))
		||  (Phy[10] != '\0')) {
			return(-1);
		}
		Drv = (-((DEV_TYPE_SPI)+1));
		if (Dev != (int *)NULL) {
			*Dev = ((Phy[8]-'0') * 16)				/* The device # is hex to simplify splitting	*/
			     +  (Phy[9]-'0');					/* controller number & slave number				*/
		}
	}
  #endif

  #if ((SYS_CALL_DEV_TTY) != 0)
	if (0 == strncasecmp(&Phy[0], "/dev/tty", 8)) {
		if ((!isdigit((int)Phy[8]))					/* Must be a single digit appended to /dev/i2c	*/
		||  (Phy[9] != '\0')) {
			return(-1);
		}
		Drv = (-((DEV_TYPE_TTY)+1));
		if (Dev != (int *)NULL) {
			*Dev = Phy[8]-'0';
		}
	}
  #endif

	if (Drv == -1) {								/* Not a /dev, so look through all mount points	*/
		Len = -1;
		for (ii=0 ; ii<(int)(sizeof(g_SysMnt)/sizeof(g_SysMnt[0])) ; ii++) {
			if (g_SysMnt[ii].MntPoint[0] != '\0') {
				jj = strlen(&g_SysMnt[ii].MntPoint[0]);
				if (0 == strncmp(Phy, &g_SysMnt[ii].MntPoint[0], jj)) {
					if ((g_SysMnt[ii].MntPoint[1] == '\0')	/* This validate the mount point "/"	*/
					||  (Phy[jj] == '/')			/* These 2 validate mount points with names		*/
				    ||  (Phy[jj] == '\0')) {		/* /name & /name/file are OK, but not /nameX	*/
						if (jj > Len) {				/* This is to retain longest match. In case 2	*/
							Len = jj;				/* Mount point are named /xxx and /xxx/yyy		*/
							Drv = ii;
						}
					}
				}
			}
		}

		if (Drv >= 0) {								/* Got mount point, remove the mount point path	*/
			Phy[0] = '/';							/* to replace it by "/".  The drive "N:" will 	*/
			if (Phy[Len] == '/') {					/* be added later								*/
				Len++;								/* Don't want 2 "/" at the beginning			*/
			}
			memmove(&Phy[1], &Phy[Len], SYS_CALL_MAX_PATH+1-Len);
		}

	}

	Phy -= 2;										/* Back to the real beginning					*/

	if (Drv >= 0) {									/* Valid drive number							*/
		Phy[0] = (char)(Drv + '0');					/* Insert the drive number						*/
		Phy[1] = ':';
	}
	else {											/* Invalid drive # or is a valid /dev			*/
		memmove(&Phy[0], &Phy[2], SYS_CALL_MAX_PATH-1);	/* Need to bring to beginning for /dev		*/
	}

	return(Drv);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    IsFsRO																				*/
/*																									*/
/* IsFsRO - report if a mounted file system is valid and mounted Read-only or not					*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int IsFsRO(int Drv);																		*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Drv : drive number to report about RO														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : The file system is R/W																*/
/*		!= 0 : the file system is RO																*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		Is considered R/O when either is true														*/
/*			- Drive # out of range																	*/
/*			- The drive is not mounted																*/
/*			- The drive is mounted and the flags indicate R/O										*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int IsFsRO(int Drv)
{
int Ret;											/* Return value									*/

	Ret = 0;

	if (Drv >= -1) {								/* -2 and below are /dev						*/
		if ((Drv <0) || (Drv >= (int)(sizeof(g_SysMnt)/sizeof(g_SysMnt[0])))) {
			REENT_ERRNO_ = ENODEV;
			Ret          = 1;						/* Drive # out of range							*/
		}
		else if ((g_SysMnt[Drv].MntPoint[0] == '\0')) {
			REENT_ERRNO_ = ENODEV;
			Ret          = 1;						/* Drive not mounted							*/
		}
		else if ((g_SysMnt[Drv].Flags & MNT_RDONLY) != 0) {
			REENT_ERRNO_ = EROFS;
			Ret          = 1;						/* Drive is mounted read-only					*/
		}
	}

	return(Ret);

}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    open / _open_r (2)																	*/
/*																									*/
/* _open_r - open a file																			*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _open_r(struct _reent *reent, const char *path, int flags, int mode);					*/
/*																									*/
/* ARGUMENTS:																						*/
/*		reent : task's re-entrance data (GCC only)													*/
/*		path  : path / file name to open															*/
/*		flags : opening control flags																*/
/*		mode  : access mode of the file upon closing												*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>= 0 : file descriptor of the opened file													*/
/*		  -1 : opening error																		*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call open() function.														*/
/*		The argument flags recognizes these:														*/
/* 			O_RDONLY, O_WRONLY, O_RDWR																*/
/*			O_APPEND, O_CREAT, O_SYNC,  O_EXCL, O_TRUNC												*/
/*		all others are ignored.																		*/
/*																									*/
/*		The argument mode can be set numerically or the application can use any combination of		*/
/*		these defines:																				*/
/*			S_IRUSR, S_IWUSR, S_IXUSR, S_IRWXU														*/
/*			S_IRGRP, S_IWGRP, S_IXGRP, S_IRWXG														*/
/*			S_IROTH, S_IWOTH, S_IXOTH, S_IRWXO														*/
/*																									*/
/*		As this is for FAT, only the writability / non-writability in mode is recognized. The		*/
/*		writability bits mask for read in mode are 0222 (S_IRUSR|S_IRGRP|S_IROTH). For portability	*/
/*		when creating a read-write file, mode should be set to 0777 (S_IRWXU|S_IRWXG|S_IRWXO),		*/
/*		or 0666 (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) and when it's read-only, mode		*/
/*		should be set to 0555 (S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) or 0444				*/
/*		(S_IRUSR|S_IRGRP|S_IROTH).																	*/
/*		A directory is always readable as FAT does not have the support to indicate if it is		*/
/*		readable or not.  A file is set writable when any of writability bit is set.				*/
/*																									*/
/* NOTE:																							*/
/*		Upon creation, the umask is properly handle and it is applied on a task per task basis as	*/
/*		umask is local to a task. The resulting closing mode is:									*/
/*				mode & ~umask()																		*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_OPEN_(P_NM, struct _reent *reent, const char *path, int flags, int mode)
{
int     Access;										/* Access type : RO or R/W						*/
int     Dev;										/* When /dev supported, device #				*/
int     Drv;										/* Drive # of the file to open					*/
int     Err;										/* Error tracking								*/
FIL    *FatFd;										/* De-referenced FatFS file descriptor			*/
BYTE    FatFlags;									/* Remapped flags to the FatFS tokens			*/
int     fd;											/* File descriptor value to return				*/
FRESULT Fres;										/* Result of FatFS calls						*/
int     Omode;										/* Open mode									*/
char    Rpath[SYS_CALL_MAX_PATH+1];					/* Real path using logical drive #				*/
													/* Also used to split path & filename			*/

	Dev = 0;										/* To remove compiler warning					*/
	Err = 0;
													/* Set-up the config before locking the mutex	*/
	Access = flags									/* First deal with R/W access mode				*/
	       & (O_RDONLY|O_WRONLY|O_RDWR);			/* Isolate the rwx permissions					*/

	if (Access == O_WRONLY) {						/* These 3 O_??? are not ORed bit definitions	*/
		FatFlags = FA_WRITE;						/* Need to do one on one comparisons			*/
		Omode    = 0333;
	}
	else if (Access == O_RDWR) {
		FatFlags = FA_READ | FA_WRITE;
		Omode    = 0777;
	}
	else {											/* O_RDONLY is the else as is a safe opening	*/
		FatFlags = FA_READ;							/* if caller forgot to set the desired access	*/
		Omode    = 0555;
	}

	FatFlags |= (0 != (flags & O_CREAT))			/* Creating, different with/without O_EXCL		*/
	            ? ((0 != (flags & O_EXCL))			/* with O_EXCL: fail if already existing		*/
		          ? FA_CREATE_NEW					/* This reports a fail if existing				*/
		          : FA_OPEN_ALWAYS)					/* This opens if existing, if not it creates	*/
	            : FA_OPEN_EXISTING;					/* Not creating, can only open existing file	*/
													/* A global lock is more R/T efficient than		*/
	Drv = RealPath(&Rpath[0], path, &Dev);			/* Get real path & validate if FS is mounted	*/
	if (Drv == -1) {								/* -1 is invalid drive							*/
		Err = EINVAL;
	}

	if (Err == 0) {									/* A global lock is more R/T efficient than		*/
		if (0 != MTXlock(g_GlbMtx, -1)) {			/* per file lock								*/
			Err = ENOLCK;							/* In case deadlock protection is enable		*/
		}
	}

	if (Err == 0) {									/* Search a free file descriptor				*/
		for (fd=0 ; fd<(int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])) ; fd++) {	/* Lower # is std	*/
			if (g_SysFile[fd].IsOpen == 0) {		/* as will be playing with .IsOpen				*/
				break;								/* This one is unused, keep processing with it	*/
			}
		}
		if (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0]))) {	/* All entries are used			*/
			Err = ENFILE;							/* Maximum # of files reached					*/
		}											/* Return error									*/

		if (Err == 0) {
		  #if ((SYS_CALL_DEV_USED) != 0)
			if (Drv < 0) {
				strcpy(&g_SysFile[fd].Fname[0], &Rpath[0]);
				g_SysFile[fd].DevNmb  = Dev;
				g_SysFile[fd].DevType = -1-Drv;
				g_SysFile[fd].omode   = Omode;
				g_SysFile[fd].IsOpen  = 1;
			  #if ((SYS_CALL_TTY_EOF) != 0)
				g_SysFile[fd].IsEOF   = 0;
			  #endif
			}
			else
		  #endif
			{										/* Opening a regular file						*/
				if ((Access != O_RDONLY)			/* If open writable or create, check if the		*/
				||  (0 != (flags & O_CREAT))) {		/* file system mounted read-only. Cannot open	*/
					if (0 != IsFsRO(Drv)) {			/* writable and cannot create on RO Fsystem		*/
						Err = REENT_ERRNO;
					}
				}

				Fres  = FR_DISK_ERR;				/* Dummy error used for conditional flow		*/
				FatFd = &g_SysFile[fd].Fdesc;		/* De-reference for efficiency					*/
				if (Err == 0) {
					Fres = f_open(FatFd, &Rpath[0], FatFlags);
					if (Fres != FR_OK) {
						Err = EACCES;
					}
				}

				if ((Fres == FR_OK)
				&&  (0 != (flags & O_TRUNC))) {		/* Request to truncate							*/
					Fres = f_lseek(FatFd, (FSIZE_t)0);/* Safety : make sure is at the start of file	*/
					if (Fres == FR_OK) {					
						Fres = f_truncate(FatFd);	/* Truncate it from the start					*/
					}
				}

				if ((Fres == FR_OK)
				&&  (0 != (flags & O_APPEND))) {	/* Request to append to end of file				*/
					Fres = f_lseek(FatFd, f_size(FatFd));	/* Set the R/W pointer to the EOF		*/
				}

				if (Fres == FR_OK) {
					strcpy(&g_SysFile[fd].Fname[0], &Rpath[0]);
				  #if ((SYS_CALL_DEV_USED) != 0)
					g_SysFile[fd].DevType = 0;
				  #endif
					g_SysFile[fd].umode   = (0 == (flags & O_CREAT))
					                      ? -1		/* -1 indicates to not chmod() upon closing		*/
					                      : (mode & (int)(TSKmyID()->SysCall[1]));
					g_SysFile[fd].omode   = Omode;	/* Opened access mode							*/
					g_SysFile[fd].Sync    = (flags & O_SYNC);/* Memo if sync after write requested	*/
					g_SysFile[fd].IsOpen  = 1;		/* And this file is now declared open			*/
				}

				if ((Err == 0)
				&&  (Fres != FR_OK)) {
					Err = EACCES;
				}
			}

			fd += 3;								/* Skip 0,1,2: they are stdin / stdout / stderr	*/
		}

		MTXunlock(g_GlbMtx);
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO = Err;
	}

	return((Err == 0) ? fd : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    close / _close_r (2)																*/
/*																									*/
/* _close_r - close an open file																	*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _close_r(struct _reent *reent, int fd);													*/
/*																									*/
/* ARGUMENTS:																						*/
/*		reent : task's re-entrance data (GCC only)													*/
/*		fd    : file descriptor of the file to close												*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : closing error																		*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call close() function.														*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		- Check validity of the file descriptor (fd), if not then error / exit						*/
/*		- Make sure the file has been opened, if not then error / exit								*/
/*		- Close the file																			*/
/*		- Flush the file																			*/
/*		- If was created, set the access mode according to the mode requested in open()				*/
/*		- Tag this file descriptor as not open anymore (always done, even with closing error)		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_CLOSE_(P_NM, struct _reent *reent, int fd)
{
int  Err;											/* Error tracking								*/

	fd  -= 3;										/* Bring back to indexing in g_SysFile[]		*/

	if ((fd < 0)									/* Closing stdin / stdout / stderr is an error	*/
	||  (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {
		Err = EBADF;
	}
	else {
		Err = 0;

		if (0 != FIL_MTXlock(fd)) {					/* Protect the descriptor access				*/
			Err = ENOLCK;							/* Althoug wait forever, could fail if mutex	*/
		}											/* deadlock detection is turned on				*/
		else {
			if (g_SysFile[fd].IsOpen == 0) {		/* If the fd to close is not open, error		*/
				Err = EBADF;
			}
		}

		if ((Err == 0)
	  #if ((SYS_CALL_DEV_USED) != 0)
		&&  (g_SysFile[fd].DevType == 0)
	  #endif
		) {
			if (FR_OK != f_sync(&g_SysFile[fd].Fdesc)) {
				Err = EACCES;
			}

			if  (FR_OK != f_close(&g_SysFile[fd].Fdesc)) {
				Err = EACCES;
			}

			if ((Err == 0)							/* Don't chmod() if closing error				*/
			&&  (g_SysFile[fd].umode != -1)) {		/* If was created, set access mode according	*/
				if (0 != chmod(&g_SysFile[fd].Fname[0], (mode_t)g_SysFile[fd].umode)) {
					Err = EACCES;
				}
				g_SysFile[fd].umode = -1;
			}
		}

		g_SysFile[fd].IsOpen = 0;					/* Always close with or without error			*/

		if (Err != ENOLCK) {
			FIL_MTXunlock(fd);
		}
	}												/* Done to not risk running out of memory		*/


	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO = Err;
	}

  	return((Err == 0) ? 0 : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    read / _read_r (2)																	*/
/*																									*/
/* _read_r - read data from an open file															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _read_r(struct _reent *reent, int fd, void *vbuf, size_t size);							*/
/*																									*/
/* ARGUMENTS:																						*/
/*		reent : task's re-entrance data (GCC only)													*/
/*		fd    : file descriptor of the file to read from											*/
/*		vbuf  : pointer to the buffer where the data read is deposited								*/
/*		size  : number of bytes to read from the file												*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>=  0 : number of bytes read from the file													*/
/*		== -1 : read error																			*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call read() function.														*/
/*																									*/
/* NOTE:																							*/
/*		stdin / stdout / stderr are properly handled through the UART								*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		- Check validity of the file descriptor (fd), if not then error / exit						*/
/*		- If stdin / stdout /stderr																	*/
/*		   - Direct use of uart_recv() from the UART driver											*/
/*		- Else (is a file)																			*/
/*		   - Make sure the file has been opened, if not then error / exit							*/
/*		   - Read from the file if error, exit														*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

_ssize_t SC_READ_(P_NM, struct _reent *reent, int fd, void *vbuf, size_t size)
{
int Err;											/* Error tracking								*/
int Ret;											/* Return value									*/
int UARTdev;										/* When reading a UART, uart device to access	*/
#if ((SYS_CALL_DEV_USED) != 0)
  int DevNmb  = 0;									/* To remove compiler warning					*/
  int DevType = 0;
#endif

	Err     =  0;
	fd     -=  3;									/* Bring back to indexing in g_SysFile[]		*/
	Ret     = -1;
	UARTdev = -1;									/* Invalid UART device number					*/

	if ((fd < -3)									/* Check if fd is within valid range			*/
	||  (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {
		Err = EBADF;
	}
	else if (fd == -3) {							/* stdin										*/
		UARTdev = G_UartDevIn;
	}
	else if (fd == -2) {							/* stdout										*/
		UARTdev = G_UartDevOut;
	}
	else if (fd == -1) {							/* stderr										*/
		UARTdev = G_UartDevErr;
	}
	else {
		if (0 != FIL_MTXlock(fd)) {					/* Protect the descriptor access				*/
			Err = ENOLCK;							/* Althoug wait forever, could fail if mutex	*/
		}											/* deadlock detection is turned on				*/
		else {
			if (g_SysFile[fd].IsOpen == 0) {		/* The file is not open, error					*/
				Err = EBADF;
			}
		}

		if ((Err == 0)
		&&  (0 == (g_SysFile[fd].omode & 0444))) {	/* If the file was not open readable, error		*/
			Err = EBADF;
		}

		if (Err == 0) {								/* File open and read operation authorized		*/
		  #if ((SYS_CALL_DEV_USED) != 0)
			DevType = g_SysFile[fd].DevType;
			if (DevType != 0) {						/* When != 0 it is a /dev device				*/
				DevNmb = g_SysFile[fd].DevNmb;
				if (DevType == DEV_TYPE_TTY) {
					UARTdev = DevNmb;
				}
				Ret = 0;
			}
			else
		  #endif
			{
			  UINT Uret;							/* Argument to f_read()							*/
				if (FR_OK != f_read(&g_SysFile[fd].Fdesc, vbuf, size, &Uret)) {
					Err = EACCES;
				}
				Ret  = (int)Uret;
			}
		}

		if (Err != ENOLCK) {
			FIL_MTXunlock(fd);
		}
	}

	if (UARTdev >= 0) {								/* Reading from a TTY device (stdio or /dev)	*/
	  #if ((SYS_CALL_TTY_EOF) == 0)
		Ret = uart_recv(UARTdev, (char *)vbuf, (int)size);	/* Not trapping EOF, use buffered		*/

	  #else											/* TTY readings must be un-buffered when EOF	*/
	   #if (((SYS_CALL_TTY_EOF) & 0x80000000) == 0)	/* When not applied to stdin / stdout / stderr	*/
		if (fd < 0) {								/* back to buffered reading						*/
			Ret = uart_recv(UARTdev, (char *)vbuf, (int)size);
		}
		else
	   #endif
		{
		  char *Bptr;								/* Pointer in the input buffer (vbuf)			*/
		  int   RdRet;								/* Return from f_read() / uart_recv()			*/
			Bptr = (char *)vbuf;					/* When trapping EOF character					*/
			for (Ret=0 ; Ret<(int)size ; Ret++, Bptr++) {
				RdRet = uart_recv(UARTdev, Bptr, 1);
				if (RdRet < 0) {
					if (Ret == 0) {					/* First read, report error						*/
						Ret = -1;					/* When not first read, report # of char in the	*/
					}								/* buffer as we don't want to lose them			*/
					break;
				}

				if ((RdRet == 0)
				|| (*Bptr == (char)((SYS_CALL_TTY_EOF) & 0x7FFFFFFF))) {
				  #if ((SYS_CALL_TTY_EOF) != 0)
					if (fd >= 0) {
						g_SysFile[fd].IsEOF = 1;
					}
				  #endif
					break;
				}
			}
		}
	   #if ((SYS_CALL_TTY_EOF) != 0)
		if (fd >= 0) {								/* When the EOF character has been received		*/
			if (g_SysFile[fd].IsEOF != 0) {			/* always return 0 as this indicates the EOF	*/
				Ret = 0;
			}
		}
	   #endif
	  #endif
		if (Ret < 0) {
			Err = EACCES;
		}
	}

  #if ((SYS_CALL_DEV_I2C) != 0)
	if (DevType == DEV_TYPE_I2C) {					/* Read from a I2C peripheral					*/
	  int   Addr;
	  char *Cptr;
		Cptr = (char *)vbuf;
		Addr = (0xFF00 & ((int)(Cptr[0])) << 8)
		     | (0x00FF & ((int)(Cptr[1])));
		if (0 != i2c_recv(DevNmb, Addr, Cptr, size)) {
			Err = EACCES;
		}
		else {
			Ret = size;
		}
	}
  #endif

  #if ((SYS_CALL_DEV_SPI) != 0)
	if (DevType == DEV_TYPE_SPI) {					/* Read from a SPI peripheral					*/
		if (0 != spi_recv((DevNmb>>4)&0xF, DevNmb&0xF, vbuf, (uint32_t)size)) {
			Err = EACCES;
		}
		else {
			Ret = size;
		}
	}
  #endif

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO = Err;
	}

	return((_ssize_t)((Err == 0) ? Ret : -1));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    write / _write_r (2)																*/
/*																									*/
/* _write_r - write data to an open file															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _write_r(struct _reent *reent, int fd, const void *vbuf, size_t size);					*/
/*																									*/
/* ARGUMENTS:																						*/
/*		reent : task's re-entrance data (GCC only)													*/
/*		fd    : file descriptor of the file to write to												*/
/*		vbuf  : pointer to the buffer where is located the data to write							*/
/*		size  : number of bytes to write to the file												*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>=  0 : number of bytes written to the file													*/
/*		== -1 : write error																			*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call write() function.														*/
/*																									*/
/* NOTE:																							*/
/*		stdin / stdout / stderr are properly handled through the UART								*/
/*																									*/
/* IMPLEMENTATION:																					*/
/* - Check validity of the file descriptor (fd), if not then error / exit							*/
/* - If stdin / stdout /stderr																		*/
/*   - Direct use of uart_send() from the UART driver												*/
/* - Else (is a file)																				*/
/*   - Make sure the file has been opened, if not then error / exit									*/
/*   - Write to the file																			*/
/*   - If file set for sync, flush the buffer														*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

_ssize_t SC_WRITE_(P_NM, struct _reent *reent, int fd, const void *vbuf, size_t len)
{
int  Err;											/* Error tracking								*/
FIL *FatFd;											/* De-referenced FatFS file descriptor			*/
int  Ret;											/* Return value									*/
int  UARTdev;										/* When writing to UART, uart device to access	*/
#if ((SYS_CALL_DEV_USED) != 0)
  int DevNmb  = 0;									/* To remove compiler warning					*/
  int DevType = 0;
#endif

	Err     =  0;
	fd     -=  3;									/* Bring back to indexing in g_SysFile[]		*/
	Ret     = -1;
	UARTdev = -1;									/* Invalid UART device number					*/

	if ((fd < -3)									/* Check fd is within valid range				*/
	||  (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {
		Err = EBADF;
	}
	else if (fd == -3) {							/* stdin										*/
		UARTdev = G_UartDevIn;
	}
	else if (fd == -2) {							/* stdout										*/
		UARTdev = G_UartDevOut;
	}
	else if (fd == -1) {							/* stderr										*/
		UARTdev = G_UartDevErr;
	}
	else {
		if (0 != FIL_MTXlock(fd)) {					/* Protect the descriptor access				*/
			Err = ENOLCK;							/* Althoug wait forever, could fail if mutex	*/
		}											/* deadlock detection is turned on				*/
		else {
			if (g_SysFile[fd].IsOpen == 0) {		/* The file is not open, error					*/
				Err = EBADF;
			}
		}

		if ((Err == 0)
		&&  (0 == (g_SysFile[fd].omode & 0222))) {	/* If the file was not open writable, error		*/
			Err = EBADF;
		}

		if (Err == 0) {
		  #if ((SYS_CALL_DEV_USED) != 0)
			DevType = g_SysFile[fd].DevType;
			if (DevType != 0) {						/* When != 0 it is a /dev device				*/
				DevNmb = g_SysFile[fd].DevNmb;
				if (DevType == DEV_TYPE_TTY) {
					UARTdev = DevNmb;
				}
				Ret = 0;
			}
			else
		  #endif
			{
			  UINT Uret;							/* Argument to f_write()						*/
				FatFd = &g_SysFile[fd].Fdesc;		/* De-reference for ease of use					*/
				if (FR_OK != f_write(FatFd, vbuf, len, &Uret)) {/* Write operation					*/
					Err = EACCES;
				}
				Ret = (int)Uret;

				if ((Err == 0)
				&&  (g_SysFile[fd].Sync != 0)) {	/* If open with sync, flush after each write	*/
					f_sync(FatFd);
				}
			}
		}

		if (Err != ENOLCK) {
			FIL_MTXunlock(fd);
		}
	}

	if (UARTdev >= 0) {								/* Writing to a TTY device (stdio or /dev)		*/
		Ret = uart_send(UARTdev, (const char *)vbuf, (int)len);
		if (Ret < 0) {
			Err = EACCES;
		}
	}

  #if ((SYS_CALL_DEV_I2C) != 0)
	if (DevType == DEV_TYPE_I2C) {					/* Write to a I2C peripheral					*/
	  int   Addr;
	  char *Cptr;
		Cptr = (char *)vbuf;
		Addr = (0xFF00 & ((int)(Cptr[0])) << 8)
		     | (0x00FF & ((int)(Cptr[1])));
		if (0 != i2c_send(DevNmb, Addr, &Cptr[2], len)) {
			Err = EACCES;
		}
		else {
			Ret = len;
		}

	}
  #endif

  #if ((SYS_CALL_DEV_SPI) != 0)
	if (DevType == DEV_TYPE_SPI) {					/* Write to a SPI peripheral					*/
		if (0 != spi_send((DevNmb>>4)&0xF, DevNmb&0xF, vbuf, (uint32_t)len)) {
			Err = EACCES;
		}
		else {
			Ret = len;
		}
	}
  #endif

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO = Err;
	}

	return((_ssize_t)((Err == 0) ? Ret : -1));
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    lseek / _lseek_r (2)																*/
/*																									*/
/* _lseek_r - reposition the read/write file offset location										*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _lseek_r(struct _reent *reent, int fd, off_t offset, int whence);						*/
/*																									*/
/* ARGUMENTS:																						*/
/*		reent  : task's re-entrance data (GCC only)													*/
/*		fd     : file descriptor of the file to read from											*/
/*		offset : offset according to whence value 													*/
/*	 	whence : position from which offset is applied												*/
/*		         values: SEEK_SET, SEEK_CUR, SEEK_END												*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>=  0 : resulting offset from start of file													*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call lseek() function.														*/
/*																									*/
/* NOTE:																							*/
/*		The use of lseek() on stdin / stdout / stderr will return an error							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

off_t SC_LSEEK_(P_NM, struct _reent *reent, int fd, off_t offset, int whence)
{
int   Err;											/* Error tracking								*/
FIL  *FatFd;										/* De-referenced FatFS file descriptor			*/
off_t OffRes;										/* Resulting offset								*/

	Err    = 0;
	fd    -= 3;										/* Bring back to indexing in g_SysFile[]		*/
	OffRes = (off_t)0;								/* To remove compiler warning					*/

	if ((fd < 0)									/* Check fd is within valid range				*/
	||  (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {
		Err = EBADF;
	}
	else {
		if (0 != FIL_MTXlock(fd)) {					/* Protect the descriptor access				*/
			Err = ENOLCK;							/* Althoug wait forever, could fail if mutex	*/
		}											/* deadlock detection is turned on				*/
		else {
			if (g_SysFile[fd].IsOpen == 0) {		/* The file is not open, error					*/
				Err = EBADF;
			}
		}

	  #if ((SYS_CALL_DEV_USED) != 0)
		if ((Err == 0) 
		&&  (g_SysFile[fd].DevType != 0)) {			/* Can't seek a /dev device						*/
			Err = EBADF;
		}
	  #endif

		if (Err == 0) {
			FatFd  = &g_SysFile[fd].Fdesc;

			OffRes = (FSIZE_t)offset;				/* Default: Set to offset						*/
			if (whence == SEEK_END) {				/* Set to EOF + offset							*/
				OffRes += f_size(FatFd);
			}
			else if (whence == SEEK_CUR) {			/* Set to current + offset						*/
				OffRes += f_tell(FatFd);
			}

			if (FR_OK != f_lseek(FatFd, OffRes)) {
				Err = EACCES;
			}

			if ((Err == 0)							/* FatFS manual states to check final pointer	*/
			&&  (FatFd->fptr != OffRes)) {
				Err = EACCES;
			}
		}

		if (Err != ENOLCK) {
			FIL_MTXunlock(fd);
		}
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO = Err;
	}

	return((Err == 0) ? OffRes : (off_t)-1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    rename / _rename_r (2)																*/
/*																									*/
/* _rename_r - rename a file / change the location of a file										*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _rename_r(struct _reent *reent, const char *old, const char *anew);						*/
/*																									*/
/* ARGUMENTS:																						*/
/*		reent : task's re-entrance data (GCC only)													*/
/*		old   : old (original) path / file name														*/
/*		anew  : new path / file name																*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call rename() function.														*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_RENAME_(P_NM, struct _reent *reent, const char *old, const char *anew)
{
char Buf[512];										/* Buffer used when copying the file			*/
int  DrvNew;										/* Drive # of the new file						*/
int  DrvOld;										/* Drive # of the old file						*/
int  Err;											/* Error tracking								*/
int  FDold;											/* old file descriptor							*/
int  FDnew;											/* new file descriptor							*/
int  ii;											/* General purpose								*/
char NewPath[SYS_CALL_MAX_PATH+1];					/* Real path using logical drive #				*/
char OldPath[SYS_CALL_MAX_PATH+1];					/* Real path using logical drive #				*/
struct stat Pstat;									/* To retrieve access mode of old				*/

	Err           = 0;
	Pstat.st_mode = 0;								/* Remove possible compiler warning				*/

	DrvNew =  RealPath(&NewPath[0], anew, NULL);	/* Validate "path" and get physical path		*/
	if (DrvNew < 0) {								/* Invalid drive or /dev (can't rename dev)		*/
		Err = EINVAL;
	}
	else if (0 != IsFsRO(DrvNew)) {					/* Check if file system is mounted read-only	*/
		Err = REENT_ERRNO;
	}

	if (Err == 0) {
		DrvOld = RealPath(&OldPath[0], old, NULL);	/* Validate "path" and get physical path		*/
		if (DrvOld < 0) {							/* Invalid drive or /dev (can't rename dev)		*/
			Err = EINVAL;
		}
		else if (0 != IsFsRO(DrvOld)) {				/* Check if file system is mounted read-only	*/
			Err = REENT_ERRNO;
		}
	}

	if (Err == 0) {
		if (DrvNew == DrvOld) {						/* Same file system, use FF_Move() directly		*/
			if (FR_OK != f_rename(&OldPath[0], &NewPath[0])) {
				Err = EACCES;
			}
		}
		else {										/* Across file systems, copy & unlink			*/
			FDold = SC_OPEN_(P_NM, reent, &OldPath[0], O_RDONLY, 0);
			if (FDold < 0) {						/* Open the old one read-only					*/
				Err = REENT_ERRNO;
			}

			if ((Err == 0)							/* The old is open, get its access mode as it	*/
			&&  (0 != SC_FSTAT_(P_NM, reent, FDold, &Pstat))) {	/* will be applied to the new one	*/
				Err = REENT_ERRNO;
			}

			FDnew = -1;
			if (Err == 0) {							/* Old is open, open the new as writable		*/
				FDnew = SC_OPEN_(P_NM, reent, &NewPath[0], O_CREAT|O_RDWR, Pstat.st_mode);
				if (FDnew < 0) {					/* and set mode to old at close					*/
					Err = REENT_ERRNO;
				}
			}

			if (Err == 0) {							/* When both files are open OK					*/
				do {								/* Copy the contents of old into the new		*/
					ii = SC_READ_(P_NM, reent, FDold, &Buf[0], sizeof(Buf));
					if (ii > 0) {
						ii = SC_WRITE_(P_NM, reent, FDnew, &Buf[0], ii);
					}
					if (ii < 0) {
						Err = REENT_ERRNO;
					}
				} while (ii > 0);
			}

			if (FDold >= 0) {						/* If old one was open successfully, close it	*/
				if (0 != SC_CLOSE_(P_NM, reent, FDold)) {
					Err = REENT_ERRNO;
				}
			}

			if (FDnew >= 0) {						/* If new one was open successfully, close it	*/
				if (0 != SC_CLOSE_(P_NM, reent, FDnew)) {
					Err = REENT_ERRNO;
				}
			}

			if (Err == 0) {							/* The file has been copied successfully		*/
				if (0 != SC_UNLINK_(P_NM, reent, &OldPath[0])) {	/* unlink the old one			*/
					Err = REENT_ERRNO;
				}
			}
			else if (FDnew >= 0) {					/* File was not copied successfully				*/
				SC_UNLINK_(P_NM, reent, &NewPath[0]);	/* unlink the new one if was created		*/
			}										/* Ignore unlink error as file may not exist	*/
		}											/* No need to preserve errno, Err will set it	*/
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO = Err;
	}

	return((Err == 0) ? 0 : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    unlink / _unlink_r (2)																*/
/*																									*/
/* _unlink_r - delete a file / directory															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _unlink_r(struct _reent *reent, const char *path);										*/
/*																									*/
/* ARGUMENTS:																						*/
/*		reent : task's re-entrance data (GCC only)													*/
/*		path  : path / file name or path / directory name											*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call unlink() function.														*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_UNLINK_(P_NM, struct _reent *reent, const char *path)
{
int  Drv;											/* Drive # of the file/directory to unlink		*/
int  Err;											/* Error tracking								*/
char Rpath[SYS_CALL_MAX_PATH+1];					/* Real path									*/

	Err = 0;
	Drv =  RealPath(&Rpath[0], path, NULL);
	if (Drv < 0) {									/* Invalid drive or /dev (can't unlink a /dev)	*/
		Err = EINVAL;
	}
	else if (0 != IsFsRO(Drv)) {					/* Need a valid & writable FS to do this		*/
		Err = REENT_ERRNO;
	}

	if ((Err == 0)
	&&  (FR_OK != f_unlink(&Rpath[0]))) {
		Err = EACCES;
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO = Err;
	}

	return((Err == 0) ? 0 : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    dup (2)																				*/
/*																									*/
/* _dup_r - duplicate a file descriptor																*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int dup(int fd);																			*/
/*																									*/
/* ARGUMENTS:																						*/
/*		fd  : file descriptor to duplicate															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>=  0 : duplicate file descriptor															*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call dup() function.														*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_DUP_(P_NM, int fd)
{
int Err;											/* Error tracking								*/
int fn;												/* New file descriptor							*/
#if ((SYS_CALL_MUTEX) >= 0)
  MTX_t *MtxBack;									/* When local mutex, back-up value				*/
#endif

	Err =  0;										/* Assume success								*/
	fd -=  3;										/* Bring back to indexing in g_SysFile[]		*/
	fn  = -1;										/* To remove compiler warnings					*/

	if ((fd < 0)									/* Don't allow dup of stdin / stdout / stderr	*/
	||  (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {	/* as we have no fd associated	*/
		Err = EBADF;								/* with them									*/
	}
	else {
		if (0 != MTXlock(g_GlbMtx, -1)) {
			Err = ENOLCK;							/* In case deadlock protection is enable		*/
		}
	}

	if (Err == 0) {
		if (g_SysFile[fd].IsOpen == 0) {			/* Make sure the file desc is valid. In other	*/
			Err = EBADF;							/* words, it need to be open					*/
		}
		else {										/* Is open, find a unused SysFile[] entry		*/
			for (fn=0 ; fn<(int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])) ; fn++) {/* Lower # is std	*/
				if (g_SysFile[fn].IsOpen == 0) {
					break;							/* This one is unused, keep processing with it	*/
				}
			}

			if (fn >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0]))) {	/* All entries are used		*/
				Err = ENFILE;						/* Maximum # of file reached					*/
			}										/* Return error									*/
			else {									/* Found an unused file descriptor				*/
			  #if ((SYS_CALL_MUTEX) >= 0)			/* Full copy, so back-up the mutex				*/
				MtxBack = g_SysFile[fn].MyMtx;		/* don't want to have unique mutex duplicated	*/
			  #endif								/* and shared between 2 SysFile					*/

				memmove(&g_SysFile[fn], &g_SysFile[fd], sizeof(g_SysFile[0]));

			  #if ((SYS_CALL_MUTEX) >= 0)
				g_SysFile[fn].MyMtx = MtxBack;
			  #endif

				fn += 3;							/* Skip 0,1,2: they are stdin / stdout / stderr	*/
			}
		}

		MTXunlock(g_GlbMtx);
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO_ = Err;
	}

	return((Err == 0) ? fn : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    getcwd (3)																			*/
/*																									*/
/* getcwd - get current working directory															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		char getcwd(void);																			*/
/*																									*/
/* ARGUMENTS:																						*/
/*		void																						*/
/*																									*/
/* RETURN VALUE:																					*/
/*		!= NULL : full path name of the current working directory									*/
/*		== NULL : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard "C" library getcwd() function.														*/
/*																									*/
/* NOTE:																							*/
/*		This implements the "C" library function (man section 3, not section 2) as it is the one	*/
/*		Newlib has the function prototype defined													*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

char *SC_GETCWD_(P_NM, char *buf, size_t size)
{
int  Drv;											/* Drive # of the current directory				*/
char Rpath[SYS_CALL_MAX_PATH+1];

	Drv = RealPath(&Rpath[0], ".", NULL);			/* "." makes RealPath() return physical CWD		*/
	if (Drv >= 0) {									/* if -ve, then no current workign directory	*/
		buf[0] = '\0';
		size--;
		LogPath(&Rpath[0], ".");
		strncpy(&buf[0], &Rpath[0], size);
		buf[size] = '\0';
	}
	else {
		REENT_ERRNO_ = ENOENT;
	}

	return((Drv >= 0) ? buf : (char *)NULL);	
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    chdir (2)																			*/
/*																									*/
/* chdir - change the current working directory														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int chdir(const char *path);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		path : new path for the current working directory											*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call chdir() function.														*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_CHDIR_(P_NM, const char *path)
{
int  Drv;											/* Drive number of the directory to set			*/
int  Err;											/* Error tracking								*/
char DirName[SYS_CALL_MAX_PATH+1];					/* We use this in case the directory is invalid	*/

	Err = 0;
  #if (_FS_RPATH != 0)								/* When this is set to 0, f_chdir not available	*/
	Drv = RealPath(&DirName[0], path, NULL);		/* Get real  path & validate if FS is mounted	*/
  #else
	Drv = LogPath(&DirName[0], path);				/* Get logical path & validate if FS is mounted	*/
  #endif
	if (Drv < 0) {									/* Invalid drive or is a /dev					*/
		Err = EINVAL;
	}
	else  {											/* Valid drive, memo new CWD in task descriptor	*/
	  #if (_FS_RPATH != 0)							/* When this is set to 0, f_chdir not available	*/
		if (FR_OK != f_chdir(&DirName[0])) {		/* Check if this is a valid directory			*/
			Err = ENOTDIR;
		}
		else {
			LogPath(&DirName[0], path);
			strcpy((char *)(TSKmyID()->SysCall[0]), &DirName[0]);
		}
	  #else
		strcpy((char *)(TSKmyID()->SysCall[0]), &DirName[0]);

	  #endif
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO_ = Err;
	}

	return((Err == 0) ? 0 : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    mkdir / _mkdir_r (2)																*/
/*																									*/
/* _mkdir_r - create a new directory																*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _mkdir_r(struct _reent *reent, const char *path, int mode);								*/
/*																									*/
/* ARGUMENTS:																						*/
/*		reent : task's re-entrance data (GCC only)													*/
/*		path  : path / directory name to create														*/
/*		mode  : directory access mode																*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call mkdir() function.														*/
/*																									*/
/* NOTE:																							*/
/*		The argument mode can be set numerically or the application can use any combination of		*/
/*		these defines:																				*/
/*			S_IRUSR, S_IWUSR, S_IXUSR, S_IRWXU														*/
/*			S_IRGRP, S_IWGRP, S_IXGRP, S_IRWXG														*/
/*			S_IROTH, S_IWOTH, S_IXOTH, S_IRWXO														*/
/*																									*/
/*		As this is for FAT, only the writability / non-writability in mode is recognized. The		*/
/*		writability bits mask for read in mode are 0222 (S_IRUSR|S_IRGRP|S_IROTH). For portability	*/
/*		when creating a read-write file, mode should be set to 0777 (S_IRWXU|S_IRWXG|S_IRWXO),		*/
/*		or 0666 (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) and when it's read-only, mode		*/
/*		should be set to 0555 (S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) or 0444				*/
/*		(S_IRUSR|S_IRGRP|S_IROTH).																	*/
/*		A directory is always readable as FAT does not have the support to indicate if it is		*/
/*		readable or not.  A file is set writable when any of writability bit is set.				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_MKDIR_(P_NM, struct _reent *reent, const char *path, int mode)
{
int  Drv;
int  Err;
char Rpath[SYS_CALL_MAX_PATH+1];

	Err = 0;
	Drv = RealPath(&Rpath[0], path, NULL);
	if (Drv < 0) {									/* Invalid drive or is a /dev					*/
		Err = EINVAL;
	}
	else if (0 != IsFsRO(Drv)) {					/* Need a valid & writable FS to do this		*/
		Err = REENT_ERRNO;
	}

	if ((Err == 0)									/* Simple: direct use of the FatFS function		*/
	&&  (FR_OK != f_mkdir(&Rpath[0]))) {
		Err = EACCES;
	}

	if ((Err == 0)
	&&  (0 != chmod(path, (mode_t)mode))) {
		Err = EACCES;
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO = Err;
	}

	return((Err == 0) ? 0 : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    fstat / _fstat_r (2)																*/
/*																									*/
/* _fstat_r - get file statistics																	*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _fstat_r(struct _reent *reent, int fd, struct stat *pstat);								*/
/*																									*/
/* ARGUMENTS:																						*/
/*		reent : task's re-entrance data (GCC only)													*/
/*		fd    : file descriptor																		*/
/*		pstat : structure to collect the file information											*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Not fully standard system call fstat() function.  Refer to stat() for full details.			*/
/*																									*/
/* NOTE:																							*/
/*     Only the following fields are set, all others are set to zero								*/
/*            st_size																				*/
/*            st_mode																				*/
/*            st_atime																				*/
/*            st_ctime																				*/
/*            st_mtime																				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		Uses _stat_r() as FatFS delivers stats using filenames										*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_FSTAT_(P_NM, struct _reent *reent, int fd, struct stat *pstat)
{
int  Ret;											/* Return value									*/

	fd -= 3;										/* Bring back to indexing in g_SysFile[]		*/
	Ret = -1;										/* Assume will fail								*/

	if ((fd < -3)									/* fd out of range								*/
	||  (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {
		REENT_ERRNO = EBADF;
	}
	else if (fd < 0) {								/* stdin / stdout / stderr						*/
		memset(pstat, 0, sizeof(*pstat));			/* report minimal information					*/
		pstat->st_mode = S_IFCHR
		               | (fd == -3)
		                 ? (S_IRUSR | S_IRGRP | S_IROTH)	 /* stdin : RO							*/
		                 : (S_IWUSR | S_IWGRP | S_IWOTH);	 /* stdout / stderr : WO				*/
		pstat->st_blksize = 1;
		Ret = 0;
	}
	else {											/* Regular file									*/
		if (g_SysFile[fd].IsOpen == 0) {			/* Must be an open file							*/
			REENT_ERRNO = EBADF;
		}
		else {
		  #if ((SYS_CALL_DEV_USED) != 0)
			if (g_SysFile[fd].DevType != 0) {		/* Is a /dev									*/
				pstat->st_size = 1;
				pstat->st_mode = g_SysFile[fd].omode
				               | S_IFCHR;
			}
			else
		  #endif
			{
				Ret = SC_STAT_(P_NM, reent, &(g_SysFile[fd].Fname[0]), pstat);
			}
		}
	}
	
	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    stat / _stat_r (2)																	*/
/*																									*/
/* _stat_r - get file statistics																	*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _stat_r(struct _reent *reent, const char *path, struct stat *pstat);					*/
/*																									*/
/* ARGUMENTS:																						*/
/*		reent : task's re-entrance data (GCC only)													*/
/*		path  : file path / name																	*/
/*		pstat : structure to collect the file information											*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Not fully standard system call stat() function.												*/
/*																									*/
/* NOTE:																							*/
/*     Only the following fields are set, all others are set to zero								*/
/*            st_size																				*/
/*            st_mode																				*/
/*            st_atime  (is always the same as st_ctime & st_mtime)									*/
/*            st_ctime  (is always the same as st_atime & st_mtime)									*/
/*            st_mtime  (is always the same as st_atime & st_stime)									*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_STAT_(P_NM, struct _reent *reent, const char *path, struct stat *pstat)
{
int       dd;										/* Index in g_SysMnt[]							*/
int       Dev;										/* When /dev supported, device #				*/
int       Drv;										/* Drive number of the file to stat				*/
int       Err;										/* Error tracking								*/
int       ii;										/* General purpose								*/
FILINFO   Info;										/* FatFS deposits the file info in there		*/
int       IsMnt;									/* Is is a mount point full or or partial path	*/
struct tm Time;
char      Rpath[SYS_CALL_MAX_PATH+1];

	Dev = 0;										/* To remove compiler warning					*/
	Err = 0;
	Drv = RealPath(&Rpath[0], path, &Dev);
	if (0 != IsFsRO(Drv)) {							/* Need a valid & writable FS to do this		*/
		Err = REENT_ERRNO;
	}

	memset(pstat, 0, sizeof(*pstat));

	if (Err == 0) {
	  #if ((SYS_CALL_DEV_USED) != 0)
	   int Len;										/* Length of path (without the file name		*/
		if (Drv < 0) {								/* < -1 is a /dev, find its file descriptor		*/
			Drv = -2-Drv;							/* Convert the return value to the device type	*/
			for (Len=(sizeof(g_SysFile)/sizeof(g_SysFile[0]))-1 ; Len>=0 ; Len--) {
				if ((g_SysFile[Len].IsOpen != 0)
				&&  (g_SysFile[Len].DevType == -2-Drv)
				&&  (g_SysFile[Len].DevNmb  == Dev)) {
					pstat->st_size = 1;
					pstat->st_mode = g_SysFile[Len].omode
					               | S_IFCHR;		/* Add character device the mode				*/
					break;
				}
			}
		}
		else
	  #endif
		{
			memset(&Info, 0, sizeof(Info));

			Drv   = LogPath(&Rpath[0], path);		/* Check if it is a mount point					*/
			IsMnt = 0;
			if (Drv == 0) {							/* Invalid drive or is a mnt point or /dev		*/
				ii = strlen(&Rpath[0]);				/* Check if path is a partial mount point path	*/
				for (dd=0 ; dd<(sizeof(g_SysMnt)/sizeof(g_SysMnt[0])) ; dd++) {
					if (g_SysMnt[dd].MntPoint[0] != '\0') {	/* Valid mount point					*/
						if ((0 == strcmp(&Rpath[0], "/"))	/* All mount points are mounted on "/"	*/
						||  (0 == strncmp(&Rpath[0], &g_SysMnt[dd].MntPoint[0], ii))) {
							IsMnt = 1;				/* Yes it is									*/
							break;
						}
					}
				}
			}

			if (IsMnt != 0) {						/* Yes it is a mount point						*/
				Time.tm_year    = 70;
				Time.tm_mon     = 1;
				Time.tm_mday    = 1;
				Time.tm_hour    = 0;
				Time.tm_min     = 0;
				Time.tm_sec     = 0;
				pstat->st_atime = mktime(&Time);
				pstat->st_ctime = pstat->st_atime;
				pstat->st_mtime = pstat->st_atime;
				pstat->st_size  = (off_t)0;
				pstat->st_mode  = (S_IRUSR | S_IRGRP | S_IROTH)	/* Always readable					*/
				                | (S_IXUSR | S_IXGRP | S_IXOTH)	/* Always executable				*/
					            | (S_IWUSR | S_IWGRP | S_IWOTH)	/* Always writable					*/
					            | S_IFLNK;			/* Set to symbolic link							*/
			}
			else {									/* Is a regular file / directory				*/
				Drv = RealPath(&Rpath[0], path, NULL);

				if (FR_OK != f_stat(&Rpath[0], &Info)) {
					Err = EACCES;
				}

				if (Err == 0) {
					Time.tm_year = (0x7F&(Info.fdate>>9)) + 80;	/* FAT32 0 time: 1980.01.01 00:00:00*/
					Time.tm_mon  =  0x0F&(Info.fdate>>5);
					Time.tm_mday =  0x1F&(Info.fdate);
					Time.tm_hour =  0x1F&(Info.ftime>>11);
					Time.tm_min  =  0x3F&(Info.ftime>>5);
					Time.tm_sec  =  0x3E&(Info.ftime<<1);

					pstat->st_atime = mktime(&Time);
					pstat->st_ctime = pstat->st_atime;
					pstat->st_mtime = pstat->st_atime;
					pstat->st_size  = (off_t)Info.fsize;/* Size of the file							*/
					pstat->st_mode  = (S_IRUSR | S_IRGRP | S_IROTH)	/* FatFS is always readable		*/
					                | (S_IXUSR | S_IXGRP | S_IXOTH)	/* FatFS is always executable	*/
					                | (((Info.fattrib) & AM_RDO)/* When read-only, no wrt privilege	*/
					                  ? 0
					                  : (S_IWUSR | S_IWGRP | S_IWOTH))
					                | (((Info.fattrib) & AM_DIR)/* Set to directory or regular file	*/
					                  ? S_IFDIR
					                  : S_IFREG);
				}
			}
		}
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO = Err;
	}

	return((Err == 0) ? 0 : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    fstatfs (2)																			*/
/*																									*/
/* fstatfs - get the file system statistics															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int fstatfs(int fs, struct statfs *buf);													*/
/*																									*/
/* ARGUMENTS:																						*/
/*		fs  : file system descriptor																*/
/*		buf : structure to collect the file system information										*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call fstatfs() function.													*/
/*																									*/
/* NOTE:																							*/
/*		Refer to SysCall.h for more information on the fields of struct statfs.						*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_FSTATFS_(P_NM, int fs, struct statfs *buf)
{
char   DrvNmb[3];									/* Drive # string as required by FatFS			*/
int    Err;											/* Error tracking								*/
FATFS *Fsys;										/* Volume to dump the stats						*/
DWORD  Ncluster;									/* Number of clusters on the disk				*/

	Err = 0;
	memset(buf, 0, sizeof(*buf));

	if ((fs < 0)
	||  (fs >= (int)(sizeof(g_SysMnt)/sizeof(g_SysMnt[0])))) {
		Err = EBADF;
	}

	if ((Err == 0)
	&&  (g_SysMnt[fs].MntPoint[0] == '\0')) {		/* This is an invalid mount point				*/
		Err = ENODEV;
	}

	if (Err == 0 ) {
		DrvNmb[0] = (char)(fs+'0');
		DrvNmb[1] = ':';
		DrvNmb[2] = '\0';

		if (FR_OK != f_getfree(DrvNmb, &Ncluster, &Fsys)) {
			Err = EACCES;
		}
	}

	if (Err == 0) {
	  #if ((_MIN_SS) == (_MAX_SS))
		buf->f_bsize = _MAX_SS;						/* Fixed size sectors							*/
	  #else
		buf->f_bsize = Fsys->ssize;					/* Variable size sectors						*/
	  #endif

		buf->f_blocks = (int64_t)(Fsys->n_fatent-2)	/* File system size is # fat entries -2			*/
		              * (int64_t)(Fsys->csize);		/* * number of sectors per clusters				*/
		buf->f_bfree  = (int64_t)(Ncluster)			/* Free size is # free clusters					*/
		              * (int64_t)(Fsys->csize);		/* * number of sectors per clusters				*/
		buf->f_flags  = g_SysMnt[fs].Flags;			/* Mount flags									*/

		strcpy(&(buf->f_mntfromname[0]), &DrvNmb[0]);
		strcpy(&(buf->f_mntonname[0]), &(g_SysMnt[fs].MntPoint[0]));

		buf->f_type = g_SysMnt[fs].Type;
		switch(buf->f_type) {
		case FS_TYPE_FAT12:
			strcpy(&(buf->f_fstypename[0]), FS_TYPE_NAME_FAT12);
			break;
		case FS_TYPE_FAT16:
			strcpy(&(buf->f_fstypename[0]), FS_TYPE_NAME_FAT16);
			break;
		case FS_TYPE_FAT32:
			strcpy(&(buf->f_fstypename[0]), FS_TYPE_NAME_FAT32);
			break;
		case FS_TYPE_EXFAT:
			strcpy(&(buf->f_fstypename[0]), FS_TYPE_NAME_EXFAT);
			break;
		default:
			strcpy(&(buf->f_fstypename[0]), FS_TYPE_NAME_AUTO);
		}
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO_ = Err;
	}

	return((Err == 0) ? 0 : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    statfs (2)																			*/
/*																									*/
/* statfs - get the file system statistics															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int statfs(const char *path, struct statfs *buf);											*/
/*																									*/
/* ARGUMENTS:																						*/
/*		path : file system mount point or drive number												*/
/*		buf  : structure to collect the file system information										*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call statfs() function.														*/
/*																									*/
/* NOTE:																							*/
/*		Refer to SysCall.h for more information on the fields of struct statfs.						*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_STATFS_(P_NM, const char *path, struct statfs *buf)
{
int  Drv;											/* Drive number									*/
int  Err;											/* Error tracking								*/
char Rpath[SYS_CALL_MAX_PATH+1];

	Err = 0;
	Drv = RealPath(&Rpath[0], path, NULL);			/* Rpath will be N:/ if path is a mount point	*/
	if (Drv < 0) {									/* Invalid drive or is a /dev					*/
		Err = EINVAL;
	}
	else if (0 != strcmp(&Rpath[2], "/")) {			/* "path" is required to be a mount point		*/
		Err = EINVAL;
	}
	else if (0 != fstatfs(Drv, buf)) {				/* Is a mount point, use fstatfs() directly		*/
		Err = ENODEV;
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO_ = Err;
	}

	return((Err == 0) ? 0 : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    chown (2)																			*/
/*																									*/
/* chown - change a file / directory ownership														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int chown(const char *path, uid_t owner, gid_t group);										*/
/*																									*/
/* ARGUMENTS:																						*/
/*		path  : file / directory path / name														*/
/*		owner : new owner id																		*/
/*		group : new group id																		*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		FAT don't not have ownership properties, therefor this is a stub function always			*/
/*		returning success.																			*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_CHOWN_(P_NM, const char *path, uid_t owner, gid_t group)
{
	return(0);										/* FAT does not have an owner					*/
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    umask (2)																			*/
/*																									*/
/* umask - change the file creation access mode mask of the current task							*/
/*																									*/
/* SYNOPSIS:																						*/
/*		mode_t umask(mode_t mask);																	*/
/*																									*/
/* ARGUMENTS:																						*/
/*		mask  : new file access mask																*/
/*																									*/
/* RETURN VALUE:																					*/
/*		previous file access mode mask																*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call umask() function.														*/
/*																									*/
/* NOTE:																							*/
/*		Each task has its own file creation access mode mask										*/
/*																									*/
/*		The argument mode can be set numerically or the application can use any combination of		*/
/*		these defines:																				*/
/*			S_IRUSR, S_IWUSR, S_IXUSR, S_IRWXU														*/
/*			S_IRGRP, S_IWGRP, S_IXGRP, S_IRWXG														*/
/*			S_IROTH, S_IWOTH, S_IXOTH, S_IRWXO														*/
/*																									*/
/*		As this is for FAT, only the writability / non-writability in mode is recognized. The		*/
/*		writability bits mask for read in mode are 0222 (S_IRUSR|S_IRGRP|S_IROTH). For portability	*/
/*		when creating a read-write file, mode should be set to 0777 (S_IRWXU|S_IRWXG|S_IRWXO),		*/
/*		or 0666 (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) and when it's read-only, mode		*/
/*		should be set to 0555 (S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) or 0444				*/
/*		(S_IRUSR|S_IRGRP|S_IROTH).																	*/
/*		A directory is always readable as FAT does not have the support to indicate if it is		*/
/*		readable or not.  A file is set writable when any of writability bit is set.				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		umask is a per task value and it is kept in the task descriptor field SysCall[1].			*/
/*		The value kept in SysCall[1] is one's complement of the mask as a '1' bit in the mask		*/
/*		indicates to set to zero the corresponding bit in the access mode of a file to close.		*/
/*		The use of 1's complement has the added benefit to easily determine if the field			*/
/*		SysCall[1] has been initialized.  When SysCall[1] hasn't been initialized, it holds 0x0,	*/
/*		which is an impossible one's complement umask.												*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

mode_t SC_UMASK_(P_NM, mode_t mask)
{
TSK_t *MyTask;										/* Current task									*/
mode_t Old;											/* original umask (return value)				*/

	MyTask = TSKmyID();
	if (MyTask->SysCall[1] == (intptr_t)0) {		/* When umask was set, is for sure non-zero		*/
		MyTask->SysCall[1] = (intptr_t)~0022;		/* When not yet set, use the default umask as	*/
	}												/* the current umask is the return value here	*/

	Old                = (mode_t)~(int)MyTask->SysCall[1];
	MyTask->SysCall[1] = (intptr_t)~(0777 & (int)mask);

	return(Old);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    chmod (2)																			*/
/*																									*/
/* chmod - change a file / directory access mode													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int chmod(const char *path, mode_t mode);													*/
/*																									*/
/* ARGUMENTS:																						*/
/*		path  : file path / name																	*/
/*		mode  : new file access mode																*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard system call chmod() function.														*/
/*																									*/
/* NOTE:																							*/
/*		The argument mode can be set numerically or the application can use any combination of		*/
/*		these defines:																				*/
/*			S_IRUSR, S_IWUSR, S_IXUSR, S_IRWXU														*/
/*			S_IRGRP, S_IWGRP, S_IXGRP, S_IRWXG														*/
/*			S_IROTH, S_IWOTH, S_IXOTH, S_IRWXO														*/
/*																									*/
/*		As this is for FAT, only the writability / non-writability in mode is recognized. The		*/
/*		writability bits mask for read in mode are 0222 (S_IRUSR|S_IRGRP|S_IROTH). For portability	*/
/*		when creating a read-write file, mode should be set to 0777 (S_IRWXU|S_IRWXG|S_IRWXO),		*/
/*		or 0666 (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) and when it's read-only, mode		*/
/*		should be set to 0555 (S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) or 0444				*/
/*		(S_IRUSR|S_IRGRP|S_IROTH).																	*/
/*		A directory is always readable as FAT does not have the support to indicate if it is		*/
/*		readable or not.  A file is set writable when any of writability bit is set.				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_CHMOD_(P_NM, const char *path, mode_t mode)
{
int  Dev;											/* When /dev supported, device #				*/
int  Drv;											/* Drive # of the file to chmod()				*/
int  Err;											/* Error tracking								*/
BYTE ReadOnly;										/* Setting read-only or read-write				*/
char Rpath[SYS_CALL_MAX_PATH+1];

	Err      = 0;
	ReadOnly = (0 == (mode & (S_IWUSR|S_IWGRP|S_IWOTH)))
	         ? AM_RDO
	         : (BYTE)0;

	Drv = RealPath(&Rpath[0], path, &Dev);
	if (Drv < 0) {									/* Invalid drive or /dev (don;t accept chmod)	*/
		Err = EINVAL;
	}
	else if (0 != IsFsRO(Drv)) {					/* Need a valid & writable FS to do this		*/
		Err = REENT_ERRNO_;
	}
	else if (FR_OK != f_chmod(&Rpath[0], ReadOnly, AM_RDO)) {
		Err = EACCES;
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO_ = Err;
	}

	return((Err ==  0) ? 0 : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* File System operations																			*/
/* ------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    mount (2)																			*/
/*																									*/
/* mount - mount a file system																		*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int mount(const char *type, void *dir, int flags, void *data);								*/
/*																									*/
/* ARGUMENTS:																						*/
/*		type  : type of file system to mount. Are recognized:										*/
/*		            "exFAT" (defined BY FS_TYPE_NAME_EXFAT)											*/
/*		            "FAT12" (defined BY FS_TYPE_NAME_FAT12)											*/
/*		            "FAT16" (defined BY FS_TYPE_NAME_FAT16)											*/
/*		            "FAT32" (defined BY FS_TYPE_NAME_FAT32)											*/
/*		dir   : mount point (must be at root level)													*/
/*		flags : Mount flags (the only ones supported here are MNT_RDONLY & MNT_UPDATE)				*/
/*		data  : for type == "FAT12", "n:" where n is a digit from 0 to SYS_CALL_N_DRV - 1			*/
/*		        for type == "FAT16", "n:" where n is a digit from 0 to SYS_CALL_N_DRV - 1			*/
/*		        for type == "FAT32", "n:" where n is a digit from 0 to SYS_CALL_N_DRV - 1			*/
/*		        for type == "exFAT", "n:" where n is a digit from 0 to SYS_CALL_N_DRV - 1			*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Non-standard system call mount() function.													*/
/*																									*/
/* NOTE:																							*/
/*		The mount points are restricted to the be at the root level.  So mounting to a mount point	*/
/*		alike "/system/mnt" will report an error													*/
/*																									*/
/*		Only the flag MNT_RDONLY is handled due to the limitations of FAT.							*/
/*																									*/
/*		Any FAT types (FAT12, FAT16, FAT32, AUTO) is accepted and no checking is performed to make	*/
/*		sure the mounted device is the same as the one in the argument "type". For example, if		*/
/*		"type" is FAT16 and the device mounted is FAT32, it will be accepted.						*/
/*																									*/
/*      FatFS mounting APIs do not specify the desired FAT type (FAT12, FAT16, or FAT32).			*/
/*		So as long as the requested format indicated is one for the three (FAT12, FAT16, FAT32),	*/
/*		the format will be accepted and the resulting format is determined by FatFS.				*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_MOUNT_(P_NM, const char *type, void *dir, int flags, void *data)
{
char  DrvNmb[3];									/* Drive # string as required by FatFS			*/
int   Drv;											/* Device number to mount						*/
int   Err;											/* Error tracking								*/
int   ii;											/* General purpose								*/
int   Len;											/* String length of the argument type			*/
char *Ptr;											/* "data" argument is the logical drive #		*/

	Drv = 0;
	Err = 0;
	Ptr = (char *)data;
	Len = strlen(Ptr);

  #if ((_FS_EXFAT) == 0)
	if ((0 != strcasecmp(type, FS_TYPE_NAME_AUTO))	/* Check for a valid file system type			*/
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_FAT12))
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_FAT16))
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_FAT32))) {
		Err = EINVAL;								/* Invalid parameter							*/
	}
  #else
	if ((0 != strcasecmp(type, FS_TYPE_NAME_AUTO))	/* Check for a valid file system type			*/
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_EXFAT))) {
		Err = EINVAL;								/* Invalid parameter							*/
	}
  #endif
													/* Double check the validity of the drive #		*/
	if (Err == 0) {									/* Must be "n" or "n:" where 'n' is a digit		*/
		if (Len > 2) {								/* First char must be a digit					*/
			Err = EINVAL;
		}
		else if (!isdigit((int)Ptr[0])) {			/* First char must be a digit					*/
			Err = EINVAL;
		}
		else if ((Ptr[1] != ':')					/* If second char, then must be ':'				*/
		     &&  (Ptr[1] != '\0')) {
				Err = EINVAL;
		}
		else {
			Drv = Ptr[0] - '0';
		}
	}

	if ((Err == 0)									/* Make sure the drive # is within range		*/
	&&  (((Drv >= (int)(sizeof(g_SysMnt)/sizeof(g_SysMnt[0]))))
	  || (Drv < 0 ))) {
		Err = EINVAL;
	}

	if ((Err == 0)									/* Mount points required to be at root level	*/
	&&  (((char *)dir)[0] != '/')) {				/* Non-standard but otherwise complex code		*/
		Err = EINVAL;
	}

	if (Err == 0) {									/* Make sure mount point name not already used	*/
		for (ii=0 ; ii<(int)sizeof(g_SysMnt)/(int)sizeof(g_SysMnt[0]) ; ii++) {
			if (0 == strcmp(&g_SysMnt[ii].MntPoint[0], (char *)dir)) {
				Err = EACCES;
				break;
			}
		}
	}

	if (Err == 0) {
		if (0 != MTXlock(g_GlbMtx, -1)) {
			Err = ENOLCK;							/* In case deadlock protection is enable		*/
		}
	}


	if (Err == 0) {
		if (g_SysMnt[Drv].MntPoint[0] != '\0') {	/* Is mounted, can only update					*/
			if ((flags & MNT_UPDATE) != 0) {		/* Request to update the way the FS operates	*/
				g_SysMnt[Drv].Flags = flags;
			}
			else {									/* Not requested to update, error				*/
				Err = EBUSY;
			}
		}
		else {										/* Not mounted, cannot request for an update	*/
			if ((flags & MNT_UPDATE) != 0) {		/* Requested to update, error					*/
				Err = ENXIO;
			}

			if (Err == 0) {							/* Make sure mount point not already used		*/
				for (ii=0 ; ii<(int)sizeof(g_SysMnt)/(int)sizeof(g_SysMnt[0]) ; ii++) {
					if (0 == strcmp(&g_SysMnt[ii].MntPoint[0], (char *)dir)) {
						Err = EACCES;
						break;
					}
				}
			}

			if ((Err == 0)							/* Force an init the media for safety			*/
			&&  (0 != disk_initialize(Drv))) {
				Err = ENXIO;
			}

			if (Err == 0) {
				DrvNmb[0] = (char)(Drv+'0');		/* Make the FatFS string for that drive			*/
				DrvNmb[1] = ':';					/* Can't use data as ':" may be missing			*/
				DrvNmb[2] = '\0';

				if (FR_OK != f_mount(&g_SysMnt[Drv].Fsys, &DrvNmb[0], 1)) {
					disk_initialize(Drv);			/* Automatic retry needed for exFAT (mystery!)	*/
					if (FR_OK != f_mount(&g_SysMnt[Drv].Fsys, &DrvNmb[0], 1)) {
						Err = ENXIO;
					}
				}
			}

			if (Err == 0) {							/* Memo the mount point in g_SysMnt				*/
				g_SysMnt[Drv].Flags = flags;
				switch (g_SysMnt[Drv].Fsys.fs_type) {
				case FS_FAT12:
					g_SysMnt[Drv].Type = FS_TYPE_FAT12;
					break;
				case FS_FAT16:
					g_SysMnt[Drv].Type = FS_TYPE_FAT16;
					break;
				case FS_FAT32:
					g_SysMnt[Drv].Type = FS_TYPE_FAT32;
					break;
				case FS_EXFAT:
					g_SysMnt[Drv].Type = FS_TYPE_EXFAT;
					break;
				default:
					g_SysMnt[Drv].Type = FS_TYPE_AUTO;
				}
				strncpy(&(g_SysMnt[Drv].MntPoint[0]), dir, SYS_CALL_MAX_PATH);
				g_SysMnt[Drv].MntPoint[SYS_CALL_MAX_PATH] = '\0';
			}										/* Memo if mounted in read-only					*/
		}

		MTXunlock(g_GlbMtx);
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO_ = Err;
	}

	return((Err == 0) ? 0 : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    unmount (2)																			*/
/*																									*/
/* unmount - unmount a file system																	*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int unmount(const char *dir, int flags);													*/
/*																									*/
/* ARGUMENTS:																						*/
/*		dir   : mount point to unmount																*/
/*		flags : ignored																				*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Non-standard system call unmount() function.												*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_UNMOUNT_(P_NM, const char *dir, int flags)
{
int  Drv;											/* Device / drive number						*/
char DrvNmb[3];										/* FAT32 drive number as "N:"					*/
int  Err;											/* Error tracking								*/
int  ii;											/* General purpose								*/
int  jj;											/* General purpose								*/
int  Len;											/* String length								*/

	Drv = -1;
	Err =  0;
	Len = -1;

	if (0 != MTXlock(g_GlbMtx, -1)) {				/* Need global protection for that				*/
		Err = ENOLCK;								/* In case deadlock protection is enable		*/
	}
	else {											/* Find the drive # of the mount point			*/
		for (ii=0 ; ii<(int)((sizeof(g_SysMnt)/sizeof(g_SysMnt[0]))) ; ii++) {
			jj = strlen(&(g_SysMnt[ii].MntPoint[0]));
			if ((jj != 0)
			&&  (jj > Len)
			&&  (0 == strncmp(&(g_SysMnt[ii].MntPoint[0]), dir, jj))) {
				Drv = ii;
				Len = jj;
			}
		}

		if (Drv < 0) {								/* Invalid drive or is a /dev or unknown mount	*/
			Err = EINVAL;
		}

		if ((Err == 0)
		&&  (g_SysMnt[Drv].MntPoint[0] == '\0')) {	/* Drv is the FS descriptor to use				*/
			Err = ENODEV;							/* Must be mounted to be unmounted				*/
		}

		if (Err == 0) {								/* Close all files that are open on that drive	*/
			for (ii=0 ; ii<(int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])) ; ii++) {
				if ((g_SysFile[ii].IsOpen != 0)
				&&  (g_SysFile[ii].Fname[0] == Drv)) {
					f_sync(&g_SysFile[ii].Fdesc);
					close(ii);
				}
			}
													/* Close all dirs that are open on that drive	*/
			for (ii=0 ; ii<(int)(sizeof(g_SysDir)/sizeof(g_SysDir[0])) ; ii++) {
				if ((g_SysDir[ii].IsOpen != 0)
				&&  (g_SysDir[ii].Drv == Drv)) {
					SC_CLOSEDIR_(P_NM, &g_SysDir[ii]);
				}
			}

			DrvNmb[0] = (char)(Drv+'0');			/* Make the FatFS string for that drive			*/
			DrvNmb[1] = ':';
			DrvNmb[2] = '\0';
			if (FR_OK != f_mount(NULL, &DrvNmb[0], 0)) {/* Direct FatFS function call				*/
				Err = EINVAL;
			}
			if (Err == 0) {
				media_uninit_FatFS(Drv);			/* This for sure will force a re-init when		*/
			}										/* remounting									*/
		}

		if (Err == 0) {								/* The unmounting was successful, then we can	*/
			g_SysMnt[Drv].MntPoint[0] = '\0';		/* make this FS descriptor available. A zero	*/
		}											/* length name for mount point/drive means free	*/

		MTXunlock(g_GlbMtx);
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO_ = Err;
	}

	return((Err == 0) ? 0 : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    mkfs																				*/
/*																									*/
/* mkfs - format a file system																		*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int mkfs(const char *type, void *data);														*/
/*																									*/
/* ARGUMENTS:																						*/
/*		type  : type of file system to format. Are recognized:										*/
/*		            "exFAT" (defined BY FS_TYPE_NAME_EXFAT)											*/
/*		            "FAT12" (defined BY FS_TYPE_NAME_FAT12)											*/
/*		            "FAT16" (defined BY FS_TYPE_NAME_FAT16)											*/
/*		            "FAT32" (defined BY FS_TYPE_NAME_FAT32)											*/
/*		data  : for type == "FAT12", "n:" where n is a digit from 0 to SYS_CALL_N_DRV - 1			*/
/*		        for type == "FAT16", "n:" where n is a digit from 0 to SYS_CALL_N_DRV - 1			*/
/*		        for type == "FAT32", "n:" where n is a digit from 0 to SYS_CALL_N_DRV - 1			*/
/*		        for type == "exFAT", "n:" where n is a digit from 0 to SYS_CALL_N_DRV - 1			*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Non-standard function.																		*/
/*																									*/
/* NOTE:																							*/
/*      FatFS formatting API uses the cluster size.  From that info, it is not possible to			*/
/*		specify the desired FAT type (FAT12, FAT16 or FAT32).  So as long as the requested format	*/
/*		indicated is one for the three (FAT12, FAT16 , FAT32), the format will be accepted and		*/
/*		the resulting format is determined by FatFS.												*/
/*																									*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_MKFS_(P_NM, const char *type, void *data)
{
void *Buf;
int   Drv;											/* Physical device / drive						*/
char  Dnmb[3];										/* Drive number as required by FatFS			*/
int   Err;											/* Error tracking								*/
BYTE  Opt;											/* File system type								*/
int   SectSize;										/* Sector size of the drive						*/

	Err = 0;

	if ((0 != strcasecmp(type, FS_TYPE_NAME_AUTO))
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_FAT12))
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_FAT16))
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_FAT32))
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_EXFAT))) {
		Err = EINVAL;								/* Invalid parameter							*/
	}

	if (Err == 0) {									/* Check the drive number is within range		*/
		Drv = ((char *)data)[0]-'0';
		if ((Drv < 0 )
		&&  (Drv >= (int)(sizeof(g_SysMnt)/sizeof(g_SysMnt[0])))) {
			Err = EBADF;
		}
	}

	if (Err == 0) {
		Dnmb[0] = Drv + '0';
		Dnmb[1] = ':';
		Dnmb[2] = '\0';
		if (g_SysMnt[Drv].MntPoint[0] != '\0') {	/* Is already mounted, unmount it, First		*/
			unmount(&g_SysMnt[Drv].MntPoint[0], 0);
		}

		disk_initialize((BYTE)Drv);					/* Force an init of the media for safety		*/

		f_mount(&g_SysMnt[Drv].Fsys, &Dnmb[0], 0);	/* f_mkfs() need a dummy mount (ignore error)	*/
													/* to fill some valid information in .Fsys		*/
		if (0 == strcasecmp(type, FS_TYPE_NAME_AUTO)) {
			Opt = FM_ANY;
		}
		else if (0 == strcasecmp(type, FS_TYPE_NAME_EXFAT)) {
			Opt = FM_EXFAT;
		}
		else if (0 == strcasecmp(type, FS_TYPE_NAME_FAT32)) {
			Opt = FM_FAT32;
		}
		else {
			Opt = FM_FAT;
		}

		SectSize = media_blksize_FatFS(Drv);		/* Work buffer must be at least	sector size		*/
		if (SectSize < 4096) {						/* FatFS doc states the larger the quicker is	*/
			SectSize = 4096;						/* the format operation.						*/
		}											/* Use 4096 if 312 byte sector spoofed on QSPI	*/

		if (0 != MTXlock(G_OSmutex, -1)) {
			Err = ENOLCK;							/* In case deadlock protection is enable		*/
		}
		else {
			Buf = malloc(SectSize);					/* Can't use OSalloc as have to free it			*/
			MTXunlock(G_OSmutex);

			if (FR_OK != f_mkfs(&Dnmb[0], Opt, 0, Buf, SectSize)) {
				Err = ENOSPC;
			}

			MTXlock(G_OSmutex, -1);
			free(Buf);
			MTXunlock(G_OSmutex);
		}
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO_ = Err;
	}

	return((Err == 0) ? 0 : -1);
}

/* ------------------------------------------------------------------------------------------------ */
/* Directory accesses																				*/
/* ------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    opendir (3)																			*/
/*																									*/
/* opendir - open a directory for reading it														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		DIR_t *opendir(const char *path);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		path : path / name of the directory															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		!= NULL : descriptor of the open directory													*/
/*		== NULL : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard "C" library call opendir() function.												*/
/*																									*/
/* NOTE:																							*/
/*		 The standard implementation uses *DIR as the data type for the return value but DIR is		*/
/*       a data type used by FatFS; so there is a conflict. As FatFS should remain untouched, it	*/
/*       was necessary to use a data type with a difference name, therefore the use of DIR_t.		*/
/*																									*/
/*       When a directory is open, it is not left open with access protection, meaning any tasks	*/
/*       can read & rewind it using the DIR_t descriptor that was returned. To not have issues,		*/
/*       DO NOT share the DIR_t * return value amongst tasks.										*/
/*       Although a mutex is still used for the descriptor access, multiple task use of the same	*/
/*		 descriptor will make readdir() skip entries as seen by each tasks							*/
/*       The proper way is for each task to use opendir() as there are no restrictions on a			*/
/*       directory been open multiple times.														*/
/*       Once the directory access is not needed anymore, it MUST be closed with closedir().		*/
/*       Not closing the directory will keep the directory descriptor busy out and will				*/
/*       result in a memory leak (running out of directory descriptors / SYS_CALL_N_DIR)			*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

DIR_t *SC_OPENDIR_(P_NM, const char *path)
{
int     dd;											/* Directory descriptor							*/
int     Drv;										/* Drive # of the directory to open				*/
int     Err;										/* Error tracking								*/
int     ii;											/* General purpose								*/
int     IsMnt;										/* This is a mount point						*/
DIR_t  *Ret;
char    Lpath[SYS_CALL_MAX_PATH+1];					/* Logical full path							*/

	Err   = 0;
	IsMnt = 0;
	Ret   = (DIR_t *)NULL;
	Drv   = LogPath(&Lpath[0], path);

	if (Drv == 0) {									/* Invalid drive or is a mnt point or /dev		*/
		ii = strlen(&Lpath[0]);						/* Check if path is a partial mount point path	*/
		for (dd=0 ; dd<(sizeof(g_SysMnt)/sizeof(g_SysMnt[0])) ; dd++) {
			if (g_SysMnt[dd].MntPoint[0] != '\0') {	/* Valid mount point						*/
				if ((0 == strcmp(&Lpath[0], "/"))	/* All mount points are mounted on "/"			*/
				||  (0 == strncmp(&Lpath[0], &g_SysMnt[dd].MntPoint[0], ii))) {
					IsMnt = 1;						/* Yes it is									*/
					break;
				}
			}
		}
	}

	Drv  = RealPath(&Lpath[0], path, NULL);			/* Re-use Lpath for real path					*/
	if (Err == 0) {
		if (0 != MTXlock(g_GlbMtx, -1)) {			/* Need global protection: modifying .IsOpen	*/
			Err = ENOLCK;							/* In case deadlock protection is enable		*/
		}
	}

	if (Err == 0) {
		for (dd=0 ; dd<(int)(sizeof(g_SysDir)/sizeof(g_SysDir[0])) ; dd++) {
			if (g_SysDir[dd].IsOpen == 0) {			/* This one is unused, keep processing with it	*/
				break;								/* Not using local lock as loop & check is		*/
			}										/* very quick									*/
		}							

		if (dd >= (int)(sizeof(g_SysDir)/sizeof(g_SysDir[0]))) {	/* All entries are used			*/
			Err = ENFILE;							/* Maximum # of file reached					*/
		}
		else {										/* Found an unused directory descriptor			*/
			if (FR_OK != f_opendir(&g_SysDir[dd].Ddesc, &Lpath[0])) {
				if (IsMnt != 0) {
					g_SysDir[dd].Drv = -1;			/* Is a partial mount point path				*/
				}
				else {
					Err = ENOENT;
				}
			}
			else {									/* Has been successful.							*/
				memmove(&g_SysDir[dd].Dfirst, &g_SysDir[dd].Ddesc, sizeof(g_SysDir[dd].Dfirst));
				g_SysDir[dd].Drv = Drv;				/* Is a real directory							*/
			}										/* Dir ops only done when .IsOpen != 0 and the	*/
		}											/* operations use local protection, not global	*/

		if (Err == 0) {								/* Is OK if dir found or partial mount point	*/
			Ret = &g_SysDir[dd];					
			if (0 != DIR_MTXlock(Ret)) {			/* Protect the descriptor access				*/
				Err = ENOLCK;						/* Althoug wait forever, could fail if mutex	*/
			}										/* deadlock detection is turned on				*/
			else {
				LogPath(&Lpath[0], path);			/* Lpath was replace by Rpath					*/
				strcpy(&g_SysDir[dd].Lpath[0], &Lpath[0]);
				g_SysDir[dd].MntIdx = (sizeof(g_SysMnt)
				                    / sizeof(g_SysMnt[0])) - 1;
				g_SysDir[dd].Offset = 0;			/* Tag the descriptor as being open				*/
				g_SysDir[dd].IsOpen = 1;			/* Done last, as will not create race condition	*/
			}										/* Dir ops only done when .IsOpen != 0 and the	*/
		}											/* operations use local protection, not global	*/

		MTXunlock(g_GlbMtx);
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO_ = Err;
		Ret          = (DIR_t *)NULL;
	}

	return(Ret);	
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    closedir (3)																		*/
/*																									*/
/* closedir - close an open directory																*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int closedir(DIR_t *dirp);																	*/
/*																									*/
/* ARGUMENTS:																						*/
/*		dirp : descriptor of the directory to close													*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard "C" library call closedir() function.												*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		Check if open, if not report an error.														*/
/*		Set 0 in g_SysDir[].IsOpen to close it.														*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_CLOSEDIR_(P_NM, DIR_t *dirp)
{
int Ret;											/* Return value									*/

	Ret = 0;										/* Assume success								*/

	if (dirp->IsOpen == 0) {						/* Can't close a dir that is not open			*/
		REENT_ERRNO_ = EBADF;
		Ret          = -1;
	}
	else if (FR_OK != f_closedir(&(dirp->Ddesc))) {
		REENT_ERRNO_ = EACCES;
		Ret          = -1;
	}
													/* Dir open or not open, close					*/
	dirp->IsOpen = 0;								/* Nothing much other than indicating is close	*/
													/* which Free the g_SysDir[] entry for re-use	*/
	DIR_MTXunlock(dirp);

	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    readdir (3)																			*/
/*																									*/
/* readdir - read the next entry in a directory														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		struct dirent *readdir(DIR_t *dirp);														*/
/*																									*/
/* ARGUMENTS:																						*/
/*		dirp : descriptor of the directory to read													*/
/*																									*/
/* RETURN VALUE:																					*/
/*		!= NULL : pointer to the directory information data structure								*/
/*		== NULL : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard "C" library call readdir() function.												*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		Check if the directory is open, if not error.												*/
/*		Use FatFS f_readdir() and if successful, extract all relevant information from it.			*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

struct dirent *SC_READDIR_(P_NM, DIR_t *dirp)
{
int     Err;										/* Error tracking								*/
FILINFO FileInfo;									/* File information reported by FatFS			*/
int     ii;											/* General purpose								*/
struct  dirent *Ret;								/* Return value									*/

	Err = 0;
	Ret = (struct dirent *)NULL;					/* Assume failure / end of directory			*/

	if (dirp->IsOpen == 0) {						/* Directory is not open, invalid				*/
		Err = EBADF;
	}

	if (Err == 0) {									/* Report mounted dirs at the beginning			*/
		ii = strlen(dirp->Lpath);					/* Length of the dir path						*/
		while ((dirp->MntIdx >= 0)					/* Scan mnt points until all done				*/
		&&    (Ret == (struct dirent *)NULL)) {
			if (0 != strlen(&(g_SysMnt[dirp->MntIdx].MntPoint[0]))) {
				if ((0 == strncmp(&dirp->Lpath[0], &g_SysMnt[dirp->MntIdx].MntPoint[0], ii))
				&&  (g_SysMnt[dirp->MntIdx].MntPoint[1] != '\0')
				&&  ((g_SysMnt[dirp->MntIdx].MntPoint[ii] == '/')	/* Sub-dir in Lpath				*/
				 || (ii == 1))) {					/* Lpath is at the root level					*/
					if (ii == 1) {					/* Lpath is at the root level					*/
						ii = 0;
					}
					Ret = &(dirp->Entry);			/* There is a valid filename, extract info		*/
					strcpy(&Ret->d_name[0],&g_SysMnt[dirp->MntIdx].MntPoint[ii+1]);
					Ret->d_namlen  = strlen(Ret->d_name);
					Ret->d_seekoff = dirp->Offset;
					Ret->d_type    = DT_LNK;
					Ret->d_drv     = dirp->MntIdx;
				}
			}
			dirp->MntIdx--;
		}
	}

	if ((Err == 0)									/* The directory is open, use FatFS directly	*/
	&&  (Ret ==  (struct dirent *)NULL)) {			/* When != NULL, is reporting a mount point		*/
		if (dirp->Drv < 0) {						/* Is a partial mount point path				*/
			FileInfo.fname[0] = '\0';				/* We are done reading the dir					*/
		}
		else {
			if (FR_OK != f_readdir(&(dirp->Ddesc), &FileInfo)) {
				Err = ENOENT;
			}
		}
	}

	if ((Err == 0)									/* Keep track of the # of read for tell/seek	*/
	&&  (Ret ==  (struct dirent *)NULL)				/* When != NULL, is reporting a mount point		*/
	&&  (FileInfo.fname[0] != '\0')) {				/* Dir reading is OK, grab the useful info		*/
		Ret            = &(dirp->Entry);			/* There is a valid filename, extract info		*/
		strncpy(Ret->d_name, &FileInfo.fname[0], SYS_CALL_MAX_PATH);
		Ret->d_name[SYS_CALL_MAX_PATH] = '\0';
		Ret->d_namlen  = strlen(Ret->d_name);
		Ret->d_seekoff = dirp->Offset;
		Ret->d_type    = (FileInfo.fattrib & AM_DIR)
		               ? DT_DIR
		               : DT_REG;
	}

	if (Err == 0) {									/* Keep track of the # of read for tell/seek	*/
		dirp->Offset++;
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO_ = Err;
		Ret          = (struct dirent *)NULL;
	}

	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    telldir (3)																			*/
/*																									*/
/* telldir - report the current read offset in an open directory									*/
/*																									*/
/* SYNOPSIS:																						*/
/*		long telldir(DIR_t *dirp);																	*/
/*																									*/
/* ARGUMENTS:																						*/
/*		dirp : descriptor of the directory to report the offset										*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>=  0 : read offset																			*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard "C" library call telldir() function.												*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		Check if the directory is open, if not error.												*/
/*		Return the value from g_SysDir[].Offset which is the index in the directory					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

long SC_TELLDIR_(P_NM, DIR_t *dirp)
{
long Ret;											/* Return value									*/

	if (dirp->IsOpen != 0) {						/* Directory is open, use the Offset value		*/
		Ret = (long)dirp->Offset;
	}
	else {											/* Directory not open, error					*/
		REENT_ERRNO_ = ENOENT;
		Ret          = (long)-1;
	}

	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    seekdir (3)																			*/
/*																									*/
/* seekdir - set the current read offset in an open directory										*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void seekdir(DIR_t *dirp, long loc);														*/
/*																									*/
/* ARGUMENTS:																						*/
/*		dirp : descriptor of the directory to report the offset										*/
/*		loc  : offset																				*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard "C" library call seekdir() function.												*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		Check if the directory is open, if not error.												*/
/*		If request location is less than g_SysDir[].Offset, rewind from to the start				*/
/*		Perform "loc"- g_SysDir[].Offset directory reads											*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void SC_SEEKDIR_(P_NM, DIR_t *dirp, long loc)
{
	if (dirp->IsOpen == 0) {						/* Directory not open, error					*/
		REENT_ERRNO_ = ENOENT;
	}
	else {											/* Directory is open, use FatFS directly		*/
		if (loc < (long)dirp->Offset) {				/* If loc is before current offset, rewind		*/
			memmove(&dirp->Ddesc, &dirp->Dfirst, sizeof(dirp->Ddesc));
			dirp->MntIdx = (sizeof(g_SysMnt)
			             / sizeof(g_SysMnt[0])) - 1;
			dirp->Offset = 0;
		}

		loc = loc									/* Number of entries to skip to reach requested	*/
		    - dirp->Offset;							/* position										*/

		while (loc > 0) {
			if (SC_READDIR_(P_NM, dirp) == (struct dirent *)NULL) {
				loc   = 0;							/* When error, we are done						*/
				REENT_ERRNO_ = ENOENT;
			}
			loc--;									/* One less entry to skip						*/
		}
	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    rewinddir (3)																		*/
/*																									*/
/* rewinddir - set the current read offset of an open directory to the beginning					*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void rewinddir(DIR_t *dirp);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		dirp : descriptor of the directory															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		void																						*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard "C" library call rewinddir() function.												*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		Check if the directory is open, if not error.												*/
/*		Put back the first directory entry (g_SysDir[].Dfirst) in g_SysDir[].Ddesc. The first entry	*/
/*		is the one obtained when opendir() was called.												*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void SC_REWINDDIR_(P_NM, DIR_t *dirp)
{
	if (dirp->IsOpen != 0) {						/* Directory open, reload first directory entry	*/
		memmove(&dirp->Ddesc, &dirp->Dfirst, sizeof(dirp->Ddesc));
		dirp->MntIdx = (sizeof(g_SysMnt) / sizeof(g_SysMnt[0])) - 1;
		dirp->Offset        = 0;
	}
	else {
		REENT_ERRNO_ = ENOENT;
	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    isatty / _isatty_r (3)																*/
/*																									*/
/* isatty - report if a file descriptor is attached to a TTY device									*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _isatty_r(struct _reent *Reent, int fd);												*/
/*																									*/
/* ARGUMENTS:																						*/
/*		reent : task's re-entrance data (GCC only)													*/
/*		fd    : file descriptor																		*/
/*																									*/
/* RETURN VALUE:																					*/
/*		!= 0 : is a TTY (stdin / stdout / stderr)													*/
/*		== 0 : is not a TTY																			*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Standard "C" library call isatty() function.												*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_ISATTY_(P_NM, struct _reent *reent, int fd)
{
int Ret;											/* Return value									*/

	Ret = ((fd>=0) && (fd<3));						/* stdin / stdout / stderr						*/

  #if ((SYS_CALL_DEV_TTY) != 0)						/* When /dev/ttyN device types are supported	*/
	if (Ret == 0) {									/* Not stdin / stdout / stderr, check for		*/
		fd -= 3;									/* /dev/ttyN									*/
		if ((fd < (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))
		&&  (fd >= 0)) {
			Ret = (g_SysFile[fd].DevType == DEV_TYPE_TTY);	/* Assume the fd is open				*/
		}
	}
  #endif

	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    devctl - device control																*/
/*																									*/
/* devctl - non FS device set-up / initialization													*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int devctl(int fd, const int *cfg);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		fd  : file descriptor																		*/
/*		cfg : configuration data (device type specific)												*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : success																				*/
/*		!= 0 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		*cfg argument according to the device type (refer to the driver docs for more info):		*/
/*																									*/
/*		I2C:																						*/
/*				cfg[0] = #of address bits (7 or 10)													*/
/*				cfg[1] = bus clock frequency in Hz													*/
/*																									*/
/*		SPI:																						*/
/*				cfg[0] = bus clock frequency in Hz													*/
/*				cfg[1] = number of bit per frame													*/
/*				cfg[2] = mode (see ??_spi.h)														*/
/*																									*/
/*		UART:																						*/
/*				cfg[0] = Baud rate																	*/
/*				cfg[1] = number of data bits														*/
/*				cfg[2] = parity																		*/
/*				cfg[3] = number of stop bits														*/
/*				cfg[4] = size of the receive queue													*/
/*				cfg[5] = size of the transmit queue													*/
/*				cfg[6] = filtering options															*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_DEVCTL_(P_NM, int fd, const int *cfg)
{
#if ((SYS_CALL_DEV_USED) != 0)
int DevNmb;											/* Physical device number (H/W module)			*/
int Err;											/* Error tracking								*/
int DevType;										/* Type of device								*/

	DevType = -1;
	Err     = 0;
	DevType = DEV_TYPE_TTY;							/* Assume is a TTY								*/

	if (fd == 0) {									/* stdin										*/
		DevNmb  = G_UartDevIn;
	}
	else if (fd == 1) {								/* stdout										*/
		DevNmb  = G_UartDevOut;
	}
	else if (fd == 2) {								/* stderr										*/
		DevNmb  = G_UartDevErr;
	}
	else {
		Err = EBADF;								/* Assume it is a regular file					*/
		fd -= 3;									/* Bring back to indexing in g_SysFile[]		*/

	  #if ((SYS_CALL_DEV_USED) != 0)
		if ((fd >= 0)
		&&  (fd  < (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))
		&&  (g_SysFile[fd].IsOpen != 0)) {
			DevType = g_SysFile[fd].DevType;
			if (DevType > 0) {						/* When >0 it is a /dev device					*/
				DevNmb = g_SysFile[fd].DevNmb;
				Err    = 0;
			}			
		}
	  #endif
	}

	if (Err == 0) {									/* No error, then configure the device			*/
	  #if ((SYS_CALL_DEV_I2C) != 0)
		if (DevType == DEV_TYPE_I2C) {
			Err = i2c_init(DevNmb, cfg[0], cfg[1]);
		}
	  #endif
	  #if ((SYS_CALL_DEV_SPI) != 0)
		if (DevType == DEV_TYPE_SPI) {
			Err = spi_init((DevNmb>>4)&0xF, DevNmb&0xF, cfg[0], cfg[1], cfg[2]);
		}
	  #endif
	  #if ((SYS_CALL_DEV_TTY) != 0)
		if (DevType == DEV_TYPE_TTY) {
			Err = uart_init(DevNmb, cfg[0], cfg[1], cfg[2], cfg[3], cfg[4], cfg[5], cfg[6]);
		}
      #endif
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO_ = Err;
	}

	return((Err == 0) ? 0 : -1);
#else
	return(-1);
#endif
}

/* ------------------------------------------------------------------------------------------------ */
/* GetKey() : non-standard but ubiquitous 															*/
/*																									*/
/* Return:																							*/
/*         == 0 : no char available from stdin														*/
/*         != 0 : char obtained from stdin															*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_GETKEY_(P_NM, void)
{
char ccc;

	ccc = (char)0;
	if (1 != uart_recv(G_UartDevIn, &ccc, -1)) {
		ccc = (char)0;
	}

	return(ccc);
}

#if ((SYS_CALL_MULTI_FATFS) == 0)

/* ------------------------------------------------------------------------------------------------ */
/* media access re-mapping																			*/
/* ------------------------------------------------------------------------------------------------ */

int64_t media_size(int Drv)
{
	return(media_size_FatFS(Drv));
}

/* ------------------------------------------------------------------------------------------------ */

int32_t media_blksize(int Drv)
{
	return(media_blksize_FatFS(Drv));
}

/* ------------------------------------------------------------------------------------------------ */

int64_t media_sectsize_size(int Drv)
{
	return(media_sectsize_FatFS(Drv));
}

/* ------------------------------------------------------------------------------------------------ */

const char *media_info(int Drv)
{
	return(media_info_FatFS(Drv));
}

#endif

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ARM CC specifics																					*/
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */

#if defined(__CC_ARM)

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    _sys_ensure																			*/
/*																									*/
/* _sys_ensure - flush a file (from internal buffer to device)										*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _sys_ensure(FILEHANDLE fh);																*/
/*																									*/
/* Refer to the compiler help or the file rt_sys.h for a description								*/
/* ------------------------------------------------------------------------------------------------ */

int SC_SYS_ENSURE_(P_NM, FILEHANDLE fh)
{
int Ret;

	fh -= 3;
	Ret = -1;

	if ((fh >= 0)									/* Make sure fh is within valid range			*/
	||  (fh < (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))
	&&  (g_SysFile[fh].IsOpen != 0)) {
		f_sync(&g_SysFile[fh].Fdesc);
		Ret = 0;
	}

	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    _sys_flen																			*/
/*																									*/
/* _sys_flen - report the size of a file															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		long _sys_flen(FILEHANDLE fh);																*/
/*																									*/
/* Refer to the compiler help or the file rt_sys.h for a description								*/
/* ------------------------------------------------------------------------------------------------ */

long SC_SYS_FLEN_(P_NM, FILEHANDLE fh)
{

long Ret;

	fh -= 3;
	Ret = (long)-1;									/* Assume error									*/

	if ((fh >= 0)									/* Make sure fh is within valid range			*/
	||  (fh < (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))
	&&  (g_SysFile[fh].IsOpen != 0)) {
		Ret = (long)g_SysFile[fh].Fdesc.obj.objsize;
	}

	return(Ret);
}
#endif

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* IAR CLIB specifics																				*/
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */

#if defined(__IAR_SYSTEMS_ICC__)

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    IARflush																			*/
/*																									*/
/* IARflush - flush a file or all files (from internal buffer to device)							*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int IARflush(FILEHANDLE fh);																*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_IARFLUSH_(P_NM, int fd)
{
int Ret;

	Ret = 0;										/* Assume success								*/
	if (fd < 0) {									/* fd-ve is flush all							*/
		for (fd=0 ; fd<(int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])) ; fd++) {
			if (0 == FIL_MTXlock(fd)) {				/* In case deadlock detection is used			*/
				if (g_SysFile[fd].IsOpen != 0) {
					f_sync(&g_SysFile[fd].Fdesc);
				}
				FIL_MTXunlock(fd);
			}
		}
	}
	else {											/* fd >= 0 is the file descriptor to flush		*/
		fd -= 3;									/* Report OK for stdio, but do nothing			*/
		if (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0]))) {
			Ret = -1;
		}
		else if (fd >= 0) {
			if (0 == FIL_MTXlock(fd)) {				/* In case deadlock detection is used			*/
				if (g_SysFile[fd].IsOpen == 0) {	/* File not open, error							*/
					Ret = -1;
				}
				else if (FR_OK != f_sync(&g_SysFile[fd].Fdesc)) {
					Ret = -1;
				}
				FIL_MTXunlock(fd);
			}
			else {
				Ret = -1;
			}
		}
	}

	return(Ret);
}

#endif

/* EOF */

