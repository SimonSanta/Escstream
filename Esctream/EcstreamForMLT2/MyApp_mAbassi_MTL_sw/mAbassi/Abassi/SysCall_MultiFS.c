/* ------------------------------------------------------------------------------------------------ */
/* FILE :		SysCall_MultiFS.c																	*/
/*																									*/
/* CONTENTS :																						*/
/*				Unix system call layer with multiple underlying FSs									*/
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
/*	$Revision: 1.19 $																				*/
/*	$Date: 2019/01/10 18:06:20 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* void _exit(int status)																			*/
/* void *_sbrk_r(struct _reent *reent, ptrdiff_t incr)												*/
/*																									*/
/* Not in here because it is located in the ASM support file										*/
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */

#include "SysCall.h"

/* ------------------------------------------------------------------------------------------------	*/

#ifndef SYS_CALL_N_FILE								/* Number of available file descriptors			*/
  #define SYS_CALL_N_FILE 		5					/* Excluding stdin, stdout & stderr				*/
#endif

#ifndef SYS_CALL_N_DIR								/* Number of available directory descriptors	*/
  #define SYS_CALL_N_DIR 		2
#endif

#ifndef SYS_CALL_N_DRV								/* Number of drives supported					*/
  #define SYS_CALL_N_DRV		1
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

/* ------------------------------------------------------------------------------------------------ */
/* Internal definitions																				*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef SYS_CALL_MULTI_FATFS
  #define SYS_CALL_MULTI_FATFS		0
#endif
#ifndef SYS_CALL_MULTI_FULLFAT
  #define SYS_CALL_MULTI_FULLFAT	0
#endif
#ifndef SYS_CALL_MULTI_UEFAT
  #define SYS_CALL_MULTI_UEFAT		0
#endif

#define SYS_CALL_DEV_USED	   (((SYS_CALL_DEV_I2C) != 0)											\
                             || ((SYS_CALL_DEV_SPI) != 0)											\
                             || ((SYS_CALL_DEV_TTY) != 0))

#define DEV_TYPE_I2C		(1)
#define DEV_TYPE_SPI		(2)
#define DEV_TYPE_TTY		(3)

/* ------------------------------------------------------------------------------------------------ */

#if ((SYS_CALL_MULTI_FATFS) != 0)
  #define MULTI_FATFS_CNT			0
  #define MULTI_FATFS_IDX			0
#else
  #define MULTI_FATFS_CNT			-1
  #define MULTI_FATFS_IDX			-1
#endif

#if ((SYS_CALL_MULTI_FULLFAT) != 0)
  #define MULTI_FULLFAT_CNT			((MULTI_FATFS_CNT)+1)
  #define MULTI_FULLFAT_IDX			((MULTI_FATFS_CNT)+1)
#else
  #define MULTI_FULLFAT_CNT			(MULTI_FATFS_CNT)
  #define MULTI_FULLFAT_IDX			-1
#endif

#if ((SYS_CALL_MULTI_UEFAT) != 0)
  #define MULTI_UEFAT_CNT			((MULTI_FULLFAT_CNT)+1)
  #define MULTI_UEFAT_IDX			((MULTI_FULLFAT_CNT)+1)
#else
  #define MULTI_UEFAT_CNT			(MULTI_FULLFAT_CNT)
  #define MULTI_UEFAT_IDX			-1
#endif

#define MULTI_N_FS	((MULTI_UEFAT_CNT)+1)

#if ((MULTI_N_FS) < 1) 
  #error "Need to define one or more SYS_CALL_MULTI_????"
#endif

/* ------------------------------------------------------------------------------------------------ */

typedef struct {
	char MntPoint[SYS_CALL_MAX_PATH+1];				/* Where the drive is mounted					*/
	int  Fsys;										/* Which file system stack to use				*/
} SysMnt_t;

typedef struct DdscMap {							/* Used to remap my dir to the file system ones	*/
	DIR_t *Ddsc;									/* FS stack' DIR_t. NULL when is unused			*/
	int    Drv;										/* Drive where this directory is located		*/
	int    Fsys;									/* Which file system stack to use				*/
} SysDir_t;

typedef struct FdscMap {							/* Used to remap my fd to the file system ones	*/
	int Drv;										/* Drive where this file is located				*/
	int Fdsc;										/* File System Stack descriptor #				*/
	int Fsys;										/* Which file system stack to use				*/
	int InUse;										/* If this directory descriptor is in use		*/
} SysFile_t;

static MTX_t    *g_GlbMtx;
static SysMnt_t  g_SysMnt[SYS_CALL_N_DRV];
static SysDir_t  g_SysDir[(MULTI_N_FS)*(SYS_CALL_N_DIR)];
static SysFile_t g_SysFile[(MULTI_N_FS)*(SYS_CALL_N_FILE)+3];	/* +3 for stdio						*/

extern int G_UartDevIn;								/* Must be supplied by the application because	*/
													/* the UART driver must know the device to use	*/

/* ------------------------------------------------------------------------------------------------ */
/* These function prototypes are hard-coded here because the ???_uart.h are platform specific		*/

extern int uart_recv(int Dev, char *Buff, int Len);

/* ------------------------------------------------------------------------------------------------ */
/* These macros make this file use-able by multiple libraries										*/
/* Lots... lots of them, but it helps a bit the readability of the code when they are used			*/

#if defined(__CC_ARM)
  #define SC_CLOSE_(Reen, Fd)				close(Fd)
  #define SC_FSTAT_(Reen, Fd, St)			fstat(Fd, St)
  #define SC_ISATTY_(Reen, Fd)				isatty(Fd)
  #define SC_LSEEK_(Reen, Fd, Off, Wh)		lseek(Fd, Off, Wh)
  #define SC_MKDIR_(Reen, Pat, Mod)			mkdir(Pat, Mod)
  #define SC_OPEN_(Reen, Pat, Flg, Mod) 	open(Pat, Flg, Mod)
  #define SC_READ_(Reen, Fd, Bf, Sz)		read(Fd, Bf, Sz)
  #define SC_RENAME_(Reen, Od, Nw)			rename_r(Od, Nw)
  #define SC_STAT_(Reen, Pat, St)			stat(Pat, St)
  #define SC_UNLINK_(Reen, Pat)				unlink(Pat)
  #define SC_WRITE_(Reen, Fd, Bf, Sz)		write(Fd, Bf, Sz)

  #define REENT_ERRNO						(errno)
  #define REENT_ERRNO_						(errno)

#elif defined(__IAR_SYSTEMS_ICC__)
  #define SC_CLOSE_(Reen, Fd)				close(Fd)
  #define SC_FSTAT_(Reen, Fd, St)			fstat(Fd, St)
  #define SC_ISATTY_(Reen, Fd)				isatty(Fd)
  #define SC_LSEEK_(Reen, Fd, Off, Wh)		lseek(Fd, Off, Wh)
  #define SC_MKDIR_(Reen, Pat, Mod)			mkdir(Pat, Mod)
  #define SC_OPEN_(Reen, Pat, Flg, Mod) 	open(Pat, Flg, Mod)
  #define SC_READ_(Reen, Fd, Bf, Sz)		read(Fd, Bf, Sz)
  #define SC_RENAME_(Reen, Od, Nw)			rename_r(Od, Nw)
  #define SC_STAT_(Reen, Pat, St)			stat(Pat, St)
  #define SC_UNLINK_(Reen, Pat)				unlink(Pat)
  #define SC_WRITE_(Reen, Fd, Bf, Sz)		write(Fd, Bf, Sz)

  #define REENT_ERRNO						(errno)
  #define REENT_ERRNO_						(errno)

#elif defined(__GNUC__)
  #define SC_CLOSE_(Reen, Fd)				_close_r(Reen, Fd)
  #define SC_FSTAT_(Reen, Fd, St)			_fstat_r(Reen, Fd, St)
  #define SC_ISATTY_(Reen, Fd)				_isatty_r(Reen, Fd)
  #define SC_LSEEK_(Reen, Fd, Off, Wh)		_lseek_r(Reen, Fd, Off, Wh)
  #define SC_MKDIR_(Reen, Pat, Mod)			_mkdir_r(Reen, Pat, Mod)
  #define SC_OPEN_(Reen, Pat, Flg, Mod) 	_open_r(Reen, Pat, Flg, Mod)
  #define SC_READ_(Reen, Fd, Bf, Sz)		_read_r(Reen, Fd, Bf, Sz)
  #define SC_RENAME_(Reen, Od, Nw)			_rename_r(Reen, Od, Nw)
  #define SC_STAT_(Reen, Pat, St)			_stat_r(Reen, Pat, St)
  #define SC_UNLINK_(Reen, Pat)				_unlink_r(Reen, Pat)
  #define SC_WRITE_(Reen, Fd, Bf, Sz)		_write_r(Reen, Fd, Bf, Sz)

													/* If Abassi does not supply __getreent(),		*/
  #if ((OX_LIB_REENT_PROTECT) == 0)					/* back to _impure_ptr							*/
	#define __getreent()					_impure_ptr
  #endif

  #define REENT_ERRNO						(reent->_errno)
  #define REENT_ERRNO_						(__getreent()->_errno)

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

  int _close(int fd) {
	return(_close_r(__getreent(), fd));
  }
  int _fstat(struct _reent *reent, int fd, struct stat *pstat) {
	return(_fstat_r(__getreent(), fd, pstat));
  }
  int _isatty(int fd) {
	return(_isatty_r(__getreent(), fd));
  }
  off_t _lseek(int fd, off_t offset, int whence) {
	return(_lseek_r(__getreent(), fd, offset, whence));
  }
  int mkdir(const char *path, mode_t mode) {
	return(_mkdir_r(__getreent(), path, (int)mode));
  }
  int _open(char *path, int flags, int mode) {
	return(_open_r(__getreent(), path, flags, mode));
  }
  int _read(int fd, void *vbuf, size_t size) {
	return(_read_r(__getreent(), fd, vbuf, size));
  }
  int rename(const char *old, const char *new) {
	return(_rename_r(__getreent(), old, new));
  }
  int _stat(const char *path, struct stat *pstat) {
	return(_stat_r(__getreent(), path, pstat));
  }
  int _unlink(const char *path) {
	return(_unlink_r(__getreent(), path));
  }
  int _write(int fd, const void *vbuf, size_t size) {
	return(_write_r(__getreent(), fd, vbuf, size));
  }
#endif

/* ------------------------------------------------------------------------------------------------ */

#define SYS_DUP_(P1,x)							SXS_DUP_(P1,x)
#define SXS_DUP_(P2,x)							dup##P2(x)
#define SYS_GETCWD_(P1,x,y)						SXS_GETCWD_(P1,x,y)
#define SXS_GETCWD_(P2,x,y)						getcwd##P2(x,y)
#define SYS_CHDIR_(P1,x)						SXS_CHDIR_(P1,x)
#define SXS_CHDIR_(P2,x)						chdir##P2(x)
#define SYS_FSTATFS_(P1,x,y)					SXS_FSTATFS_(P1,x,y)
#define SXS_FSTATFS_(P2,x,y)					fstatfs##P2(x,y)
#define SYS_STATFS_(P1,x,y)						SXS_STATFS_(P1,x,y)
#define SXS_STATFS_(P2,x,y)						statfs##P2(x,y)
#define SYS_CHOWN_(P1,x,y,z)					SXS_CHOWN_(P1,x,y,z)
#define SXS_CHOWN_(P2,x,y,z)					chown##P2(x,y,z)
#define SYS_UMASK_(P1,x)						SXS_UMASK_(P1,x)
#define SXS_UMASK_(P2,x)						umask##P2(x)
#define SYS_CHMOD_(P1,x,y)						SXS_CHMOD_(P1,x,y)
#define SXS_CHMOD_(P2,x,y)						chmod##P2(x,y)
#define SYS_MOUNT_(P1,a,b,c,d)					SXS_MOUNT_(P1,a,b,c,d)
#define SXS_MOUNT_(P2,a,b,c,d)					mount##P2(a,b,c,d)
#define SYS_UNMOUNT_(P1,a,b)					SXS_UNMOUNT_(P1,a,b)
#define SXS_UNMOUNT_(P2,a,b)					unmount##P2(a,b)
#define SYS_MKFS_(P1,a,b)						SXS_MKFS_(P1,a,b)
#define SXS_MKFS_(P2,a,b)						mkfs##P2(a,b)
#define SYS_OPENDIR_(P1,x)						SXS_OPENDIR_(P1,x)
#define SXS_OPENDIR_(P2,x)						opendir##P2(x)
#define SYS_CLOSEDIR_(P1,x)						SXS_CLOSEDIR_(P1,x)
#define SXS_CLOSEDIR_(P2,x)						closedir##P2(x)
#define SYS_READDIR_(P1,x)						SXS_READDIR_(P1,x)
#define SXS_READDIR_(P2,x)						readdir##P2(x)
#define SYS_TELLDIR_(P1,x)						SXS_TELLDIR_(P1,x)
#define SXS_TELLDIR_(P2,x)						telldir##P2(x)
#define SYS_SEEKDIR_(P1,a,b)					SXS_SEEKDIR_(P1,a,b)
#define SXS_SEEKDIR_(P2,a,b)					seekdir##P2(a,b)
#define SYS_REWINDDIR_(P1,x)					SXS_REWINDDIR_(P1,x)
#define SXS_REWINDDIR_(P2,x)					rewinddir##P2(x)
#define SYS_DEVCTL_(P1,a,b)						SXS_DEVCTL_(P1,a,b)
#define SXS_DEVCTL_(P2,a,b)						devctl##P2(a,b)
#define SYS_GETKEY_(P1,x)						SXS_GETKEY_(P1,x)
#define SXS_GETKEY_(P2,x)						GetKey##P2(x)
#define SYS_SYS_ENSURE_(P1,x)					SXS_SYS_ENSURE_(P1,x)
#define SXS_SYS_ENSURE_(P2,x)					_sys_ensure##P2(x)
#define SYS_SYS_FLEN_(P1,x)						SXS_SYS_FLEN_(P1,x)
#define SXS_SYS_FLEN_(P2,x)						_sys_flen##P2(x)
#define SYS_IARFLUSH_(P1,x)						SXS_IARFLUSH_(P1,x)
#define SXS_IARFLUSH_(P2,x)						IARflush##P2(x)

#if defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__)
  #define SYS_CLOSE_(P1, Reen, Fd)				SXS_CLOSE_(P1, Fd)
  #define SXS_CLOSE_(P2, Fd)					close##P2(Fd)
  #define SYS_FSTAT_(P1, Reen, Fd, St)			SXS_FSTAT_(P1, Fd, St)
  #define SXS_FSTAT_(P2, Fd, St)				fstat##P2(Fd, St)
  #define SYS_ISATTY_(P1, Reen, Fd)				SXS_ISATTY_(P1, Fd)
  #define SXS_ISATTY_(P2, Fd)					isatty##P2(Fd)
  #define SYS_LSEEK_(P1, Reen, Fd, Off, Wh)		SXS_LSEEK_(P1, Fd, Off, Wh)
  #define SXS_LSEEK_(P2, Fd, Off, Wh)			lseek##P2(Fd, Off, Wh)
  #define SYS_MKDIR_(P1, Reen, Pat, Mod)		SXS_MKDIR_(P1, Pat, Mod)
  #define SXS_MKDIR_(P2, Pat, Mod)				mkdir##P2(Pat, Mod)
  #define SYS_OPEN_(P1, Reen, Pat, Flg, Mod) 	SXS_OPEN_(P1, Pat, Flg, Mod)
  #define SXS_OPEN_(P2, Pat, Flg, Mod) 			open##P2(Pat, Flg, Mod)
  #define SYS_READ_(P1, Reen, Fd, Bf, Sz)		SXS_READ_(P1, Fd, Bf, Sz)
  #define SXS_READ_(P2, Fd, Bf, Sz)				read##P2(Fd, Bf, Sz)
  #define SYS_RENAME_(P1, Reen, Od, Nw)			SXS_RENAME_(P1, Od, Nw)
  #define SXS_RENAME_(P2, Od, Nw)				rename_r##P2(Od, Nw)
  #define SYS_STAT_(P1, Reen, Pat, St)			SXS_STAT_(P1, Pat, St)
  #define SXS_STAT_(P2, Pat, St)				stat##P2(Pat, St)
  #define SYS_UNLINK_(P1, Reen, Pat)			SXS_UNLINK_(P1, Pat)
  #define SXS_UNLINK_(P2, Pat)					unlink##P2(Pat)
  #define SYS_WRITE_(P1, Reen, Fd, Bf, Sz)		SXS_WRITE_(P1, Fd, Bf, Sz)
  #define SXS_WRITE_(P2, Fd, Bf, Sz)			write##P2(Fd, Bf, Sz)

  #if ((SYS_CALL_MULTI_FATFS) != 0)
	int fstat_FatFS(int fd, struct stat *pstat);
	int fstatfs_FatFS(int fs, struct statfs *buf);
	int stat_FatFS(const char *path, struct stat *pstat);
	int unlink_FatFS(const char *path);
  #endif

#elif defined(__GNUC__)
  #define SYS_CLOSE_(P1, Reen, Fd)				SXS_CLOSE_(P1, Reen, Fd)
  #define SXS_CLOSE_(P2, Reen, Fd)				_close_r##P2(Reen, Fd)
  #define SYS_FSTAT_(P1, Reen, Fd, St)			SXS_FSTAT_(P1, Reen, Fd, St)
  #define SXS_FSTAT_(P2, Reen, Fd, St)			_fstat_r##P2(Reen, Fd, St)
  #define SYS_ISATTY_(P1, Reen, Fd)				SXS_ISATTY_(P1, Reen, Fd)
  #define SXS_ISATTY_(P2, Reen, Fd)				_isatty_r##P2(Reen, Fd)
  #define SYS_LSEEK_(P1, Reen, Fd, Off, Wh)		SXS_LSEEK_(P1, Reen, Fd, Off, Wh)
  #define SXS_LSEEK_(P2, Reen, Fd, Off, Wh)		_lseek_r##P2(Reen, Fd, Off, Wh)
  #define SYS_MKDIR_(P1, Reen, Pat, Mod)		SXS_MKDIR_(P1, Reen, Pat, Mod)
  #define SXS_MKDIR_(P2, Reen, Pat, Mod)		_mkdir_r##P2(Reen, Pat, Mod)
  #define SYS_OPEN_(P1, Reen, Pat, Flg, Mod) 	SXS_OPEN_(P1, Reen, Pat, Flg, Mod)
  #define SXS_OPEN_(P2, Reen, Pat, Flg, Mod) 	_open_r##P2(Reen, Pat, Flg, Mod)
  #define SYS_READ_(P1, Reen, Fd, Bf, Sz)		SXS_READ_(P1, Reen, Fd, Bf, Sz)
  #define SXS_READ_(P2, Reen, Fd, Bf, Sz)		_read_r##P2(Reen, Fd, Bf, Sz)
  #define SYS_RENAME_(P1, Reen, Od, Nw)			SXS_RENAME_(P1, Reen, Od, Nw)
  #define SXS_RENAME_(P2, Reen, Od, Nw)			_rename_r##P2(Reen, Od, Nw)
  #define SYS_STAT_(P1, Reen, Pat, St)			SXS_STAT_(P1, Reen, Pat, St)
  #define SXS_STAT_(P2, Reen, Pat, St)			_stat_r##P2(Reen, Pat, St)
  #define SYS_UNLINK_(P1, Reen, Pat)			SXS_UNLINK_(P1, Reen, Pat)
  #define SXS_UNLINK_(P2, Reen, Pat)			_unlink_r##P2(Reen, Pat)
  #define SYS_WRITE_(P1, Reen, Fd, Bf, Sz)		SXS_WRITE_(P1, Reen, Fd, Bf, Sz)
  #define SXS_WRITE_(P2, Reen, Fd, Bf, Sz)		_write_r##P2(Reen, Fd, Bf, Sz)

													/* If Abassi does not supply __getreent(),		*/
  #if ((OX_LIB_REENT_PROTECT) == 0)					/* back to _impure_ptr							*/
	#define __getreent()						_impure_ptr
  #endif

#endif

/* ------------------------------------------------------------------------------------------------ */

extern void     SysCallInit_FatFS(void);
extern int      SYS_CHDIR_(_FatFS, const char *path);
extern int      SYS_CHMOD_(_FatFS, const char *path, mode_t mode);
extern int      SYS_CHOWN_(_FatFS, const char *path, uid_t owner, gid_t group);
extern int      SYS_CLOSE_(_FatFS, struct _reent *reent, int fd);
extern int      SYS_CLOSEDIR_(_FatFS, DIR_t *dirp);
extern int      SYS_DEVCTL_(_FatFS, int fd, const int *cfg);
extern int      SYS_DUP_(_FatFS, int fd);
extern int      SYS_FSTAT_(_FatFS, struct _reent *reent, int fd, struct stat *pstat);
extern int      SYS_FSTATFS_(_FatFS, int fs, struct statfs *buf);
extern off_t    SYS_LSEEK_(_FatFS, struct _reent *reent, int fd, off_t offset, int whence);
extern int      SYS_MKDIR_(_FatFS, struct _reent *reent, const char *path, int mode);
extern int      SYS_MKFS_(_FatFS, const char *type, void *data);
extern int      SYS_MOUNT_(_FatFS, const char *type, void *dir, int flags, void *data);
extern int      SYS_OPEN_(_FatFS, struct _reent *reent, const char *path, int flags, int mode);
extern DIR_t   *SYS_OPENDIR_(_FatFS, const char *path);
extern _ssize_t SYS_READ_(_FatFS, struct _reent *reent, int fd, void *vbuf, size_t size);
extern struct dirent* SYS_READDIR_(_FatFS, DIR_t *dirp);
extern int      SYS_RENAME_(_FatFS, struct _reent *reent, const char *old, const char *new);
extern void     SYS_REWINDDIR_(_FatFS, DIR_t *dirp);
extern void     SYS_SEEKDIR_(_FatFS, DIR_t *dirp, long loc);
extern int      SYS_STAT_(_FatFS, struct _reent *reent, const char *path, struct stat *pstat);
extern int      SYS_STATFS_(_FatFS, const char *path, struct statfs *buf);
extern long     SYS_TELLDIR_(_FatFS, DIR_t *dirp);
extern int      SYS_UNLINK_(_FatFS, struct _reent *reent, const char *path);
extern int      SYS_UNMOUNT_(_FatFS, const char *dir, int flags);
extern _ssize_t SYS_WRITE_(_FatFS, struct _reent *reent, int fd, const void *vbuf, size_t len);

#if defined(__CC_ARM)
  extern int SYS_SYS_ENSURE_(_FatFS, FILEHANDLE fh);
  extern long SYS_SYS_FLEN_(_FatFS, FILEHANDLE fh);
#endif

#if defined(__IAR_SYSTEMS_ICC__)
  extern int SYS_IARFLUSH_(_FatFS, int fd);
#endif

/* ------------------------------------------------------------------------------------------------ */

extern void     SysCallInit_FullFAT(void);
extern int      SYS_CHDIR_(_FullFAT, const char *path);
extern int      SYS_CHMOD_(_FullFAT, const char *path, mode_t mode);
extern int      SYS_CHOWN_(_FullFAT, const char *path, uid_t owner, gid_t group);
extern int      SYS_CLOSE_(_FullFAT, struct _reent *reent, int fd);
extern int      SYS_CLOSEDIR_(_FullFAT, DIR_t *dirp);
extern int      SYS_DEVCTL_(_FullFAT, int fd, const int *cfg);
extern int      SYS_DUP_(_FullFAT, int fd);
extern int      SYS_FSTAT_(_FullFAT, struct _reent *reent, int fd, struct stat *pstat);
extern int      SYS_FSTATFS_(_FullFAT, int fs, struct statfs *buf);
extern off_t    SYS_LSEEK_(_FullFAT, struct _reent *reent, int fd, off_t offset, int whence);
extern int      SYS_MKDIR_(_FullFAT, struct _reent *reent, const char *path, int mode);
extern int      SYS_MKFS_(_FullFAT, const char *type, void *data);
extern int      SYS_MOUNT_(_FullFAT, const char *type, void *dir, int flags, void *data);
extern int      SYS_OPEN_(_FullFAT, struct _reent *reent, const char *path, int flags, int mode);
extern DIR_t   *SYS_OPENDIR_(_FullFAT, const char *path);
extern _ssize_t SYS_READ_(_FullFAT, struct _reent *reent, int fd, void *vbuf, size_t size);
extern struct dirent* SYS_READDIR_(_FullFAT, DIR_t *dirp);
extern int      SYS_RENAME_(_FullFAT, struct _reent *reent, const char *old, const char *new);
extern void     SYS_REWINDDIR_(_FullFAT, DIR_t *dirp);
extern void     SYS_SEEKDIR_(_FullFAT, DIR_t *dirp, long loc);
extern int      SYS_STAT_(_FullFAT, struct _reent *reent, const char *path, struct stat *pstat);
extern int      SYS_STATFS_(_FullFAT, const char *path, struct statfs *buf);
extern long     SYS_TELLDIR_(_FullFAT, DIR_t *dirp);
extern int      SYS_UNLINK_(_FullFAT, struct _reent *reent, const char *path);
extern int      SYS_UNMOUNT_(_FullFAT, const char *dir, int flags);
extern _ssize_t SYS_WRITE_(_FullFAT, struct _reent *reent, int fd, const void *vbuf, size_t len);

#if defined(__CC_ARM)
  extern int SYS_SYS_ENSURE_(_FullFAT, FILEHANDLE fh);
  extern long SYS_SYS_FLEN_(_FullFAT, FILEHANDLE fh);
#endif

#if defined(__IAR_SYSTEMS_ICC__)
  extern int SYS_IARFLUSH_(_FullFAT, int fd);
#endif

/* ------------------------------------------------------------------------------------------------ */

extern void     SysCallInit_ueFAT(void);
extern int      SYS_CHDIR_(_ueFAT, const char *path);
extern int      SYS_CHMOD_(_ueFAT, const char *path, mode_t mode);
extern int      SYS_CHOWN_(_ueFAT, const char *path, uid_t owner, gid_t group);
extern int      SYS_CLOSE_(_ueFAT, struct _reent *reent, int fd);
extern int      SYS_CLOSEDIR_(_ueFAT, DIR_t *dirp);
extern int      SYS_DEVCTL_(_ueFAT, int fd, const int *cfg);
extern int      SYS_DUP_(_ueFAT, int fd);
extern int      SYS_FSTAT_(_ueFAT, struct _reent *reent, int fd, struct stat *pstat);
extern int      SYS_FSTATFS_(_ueFAT, int fs, struct statfs *buf);
extern off_t    SYS_LSEEK_(_ueFAT, struct _reent *reent, int fd, off_t offset, int whence);
extern int      SYS_MKDIR_(_ueFAT, struct _reent *reent, const char *path, int mode);
extern int      SYS_MKFS_(_ueFAT, const char *type, void *data);
extern int      SYS_MOUNT_(_ueFAT, const char *type, void *dir, int flags, void *data);
extern int      SYS_OPEN_(_ueFAT, struct _reent *reent, const char *path, int flags, int mode);
extern DIR_t   *SYS_OPENDIR_(_ueFAT, const char *path);
extern _ssize_t SYS_READ_(_ueFAT, struct _reent *reent, int fd, void *vbuf, size_t size);
extern struct dirent* SYS_READDIR_(_ueFAT, DIR_t *dirp);
extern int      SYS_RENAME_(_ueFAT, struct _reent *reent, const char *old, const char *new);
extern void     SYS_REWINDDIR_(_ueFAT, DIR_t *dirp);
extern void     SYS_SEEKDIR_(_ueFAT, DIR_t *dirp, long loc);
extern int      SYS_STAT_(_ueFAT, struct _reent *reent, const char *path, struct stat *pstat);
extern int      SYS_STATFS_(_ueFAT, const char *path, struct statfs *buf);
extern long     SYS_TELLDIR_(_ueFAT, DIR_t *dirp);
extern int      SYS_UNLINK_(_ueFAT, struct _reent *reent, const char *path);
extern int      SYS_UNMOUNT_(_ueFAT, const char *dir, int flags);
extern _ssize_t SYS_WRITE_(_ueFAT, struct _reent *reent, int fd, const void *vbuf, size_t len);

#if defined(__CC_ARM)
  extern int SYS_SYS_ENSURE_(_ueFAT, FILEHANDLE fh);
  extern long SYS_SYS_FLEN_(_ueFAT, FILEHANDLE fh);
#endif

#if defined(__IAR_SYSTEMS_ICC__)
  extern int SYS_IARFLUSH_(_ueFAT, int fd);
#endif

/* ------------------------------------------------------------------------------------------------ */

extern const char *media_info_FatFS(int Drv);
extern const char *media_info_FullFAT(int Drv);
extern const char *media_info_ueFAT(int Drv);

extern int32_t media_blksize_FatFS(int Drv);
extern int32_t media_blksize_FullFAT(int Drv);
extern int32_t media_blksize_ueFAT(int Drv);

extern int32_t media_sectsize_FatFS(int Drv);
extern int32_t media_sectsize_FullFAT(int Drv);
extern int32_t media_sectsize_ueFAT(int Drv);

extern int64_t media_size_FatFS(int Drv);
extern int64_t media_size_FullFAT(int Drv);
extern int64_t media_size_ueFAT(int Drv);

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

void SysCallInit(void)
{

	g_GlbMtx = MTXopen("SysCall Glb");				/* The global mutex is always required			*/

	memset(&g_SysMnt[0],  0, (int)sizeof(g_SysMnt)/(int)sizeof(&g_SysMnt[0]));
	memset(&g_SysDir[0],  0, (int)sizeof(g_SysDir)/(int)sizeof(&g_SysDir[0]));
	memset(&g_SysFile[0], 0, (int)sizeof(g_SysFile)/(int)sizeof(&g_SysFile[0]));

	g_SysFile[0].Fdsc  = 0;							/* Validate all 3 stdio file handler			*/
	g_SysFile[0].InUse = 1;

	g_SysFile[1].Fdsc  = 1;
	g_SysFile[1].InUse = 1;

	g_SysFile[2].Fdsc  = 2;
	g_SysFile[2].InUse = 1;

  #if ((SYS_CALL_MULTI_FATFS) != 0)
	SysCallInit_FatFS();
  #endif

  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
	SysCallInit_FullFAT();
  #endif

  #if ((SYS_CALL_MULTI_UEFAT) != 0)
	SysCallInit_ueFAT();
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
/*		int RealPath(char *Phy, const char *Log, int *FsIdx);										*/
/*																									*/
/* ARGUMENTS:																						*/
/*		Phy   : result for the physical path (always starts with "N:/")								*/
/*		Log   : logical path/file name to convert (using mount points & CWD)						*/
/*		FsIdx : returns the file system index this path is on										*/
/*																									*/
/* RETURN VALUE:																					*/
/*		>=  0 : drive number for Log																*/
/*		== -1 : invalid path																		*/
/*		== (-((DEV_TYPE_I2C)+1)) : when it's a /dev/i2c device and I2C devices are supported 		*/
/*		== (-((DEV_TYPE_SPI)+1)) : when it's a /dev/spi device and SPI devices are supported 		*/
/*		== (-((DEV_TYPE_TTY)+1)) : when it's a /dev/tty device and TTY devices are supported 		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int RealPath(char *Phy, const char *Log, int *FsIdx)
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
	}
  #endif

  #if ((SYS_CALL_DEV_TTY) != 0)
	if (0 == strncasecmp(&Phy[0], "/dev/tty", 8)) {
		if ((!isdigit((int)Phy[8]))					/* Must be a single digit appended to /dev/i2c	*/
		||  (Phy[9] != '\0')) {
			return(-1);
		}
		Drv = (-((DEV_TYPE_TTY)+1));
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
		*FsIdx = g_SysMnt[Drv].Fsys;				/* Report to the caller the file system stack	*/
	}
	else {											/* Invalid drive # or is a valid /dev			*/
		memmove(&Phy[0], &Phy[2], SYS_CALL_MAX_PATH-1);	/* Need to bring to beginning for /dev		*/
		*FsIdx = g_SysMnt[0].Fsys;					/* /dev always used 1st file system stack		*/
	}

	return(Drv);
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
/*																									*/
/* NOTE:																							*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_OPEN_(struct _reent *reent, const char *path, int flags, int mode)
{
int  Drv;											/* Drive # or type of /dev						*/
int  Err;											/* Value to set errno to						*/
int  fd;											/* File descriptor from the FS stack			*/
int  Fidx;											/* File system stack index						*/
int  MyFd;											/* My file descriptor number					*/
char Rpath[SYS_CALL_MAX_PATH+1];					/* Real path using logical drive #				*/

	Err  = 0;
	fd   = -1;
	MyFd = -1;

	Drv = RealPath(&Rpath[0], path, &Fidx);			/* Get real path & validate if FS is mounted	*/
	if (Drv < 0) {									/* Invalid drive or /dev						*/
		strcpy(&Rpath[0], " :");					/* Make sure to use an invalid name				*/
		Err = ENOENT;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (Fidx == MULTI_FATFS_IDX) {
			fd = SYS_OPEN_(_FatFS, reent, &Rpath[0], flags, mode);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (Fidx == MULTI_FULLFAT_IDX) {
			fd = SYS_OPEN_(_FullFAT, reent, &Rpath[0], flags, mode);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (Fidx == MULTI_UEFAT_IDX) {
			fd = SYS_OPEN_(_ueFAT, reent, &Rpath[0], flags, mode);
		}
	  #endif
	}

	if (fd >= 0) {									/* Valid file descriptor, memo and report our	*/
		if (0 != MTXlock(g_GlbMtx, -1)) {			/* own file descriptor number					*/
			Err = EBUSY;							/* Although infinte wait, check in case			*/
		}											/* deadlock protection is enable				*/
		else {
			for (MyFd=0 ; MyFd<(int)sizeof(g_SysFile)/(int)sizeof(g_SysFile[0]) ; MyFd++) {
				if (g_SysFile[MyFd].InUse == 0) {
					break;
				}
			}
			if (MyFd < (int)sizeof(g_SysFile)/(int)sizeof(g_SysFile[0])) {
				g_SysFile[MyFd].Fdsc  = fd;			/* Memo the FS stack file descriptor			*/
				g_SysFile[MyFd].Fsys  = Fidx;		/* Memo which FS stack handles it				*/
				g_SysFile[MyFd].InUse = 1;			/* And the file is declared in-use				*/
			}
			else {
				Err  = ENFILE;
			}

			MTXunlock(g_GlbMtx);
		}
	}

	if (Err != 0) {
		REENT_ERRNO = Err;
		MyFd        = -1;
	}

	return(MyFd);
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
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_CLOSE_(struct _reent *reent, int fd)
{
int Err;											/* Value to set errno to						*/
int Ret;											/* Return value									*/

	Err =  0;
	Ret = -1;

	if ((fd < 3)									/* Closing stdin / stdout / stderr is an error	*/
	||  (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {
		Err = EBADF;
	}
	else if (g_SysFile[fd].InUse == 0) {			/* Make sure the file is open					*/
		Err = EINVAL;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FATFS_IDX) {
			Ret = SYS_CLOSE_(_FatFS, reent, g_SysFile[fd].Fdsc);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FULLFAT_IDX) {
			Ret = SYS_CLOSE_(_FullFAT, reent, g_SysFile[fd].Fdsc);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_UEFAT_IDX) {
			Ret = SYS_CLOSE_(_ueFAT, reent, g_SysFile[fd].Fdsc);
		}
	  #endif

		g_SysFile[fd].InUse = 0;					/* Success or error all close invalidate Fdesc	*/
	}												/* No mutex needed in here						*/

	if (Err != 0) {
		REENT_ERRNO = Err;
	}

	return(Ret);
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
/*		   - error																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

_ssize_t SC_READ_(struct _reent *reent, int fd, void *vbuf, size_t size)
{
int      Err;										/* Value to set errno to						*/
_ssize_t Ret;

	Err = 0;
	Ret = (_ssize_t)-1;

	if ((fd < 0)									/* Make sure the file descriptor is valid		*/
	||  (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {
		Err = EBADF;
	}
	else if (g_SysFile[fd].InUse == 0) {			/* Make sure the file is open					*/
		Err = EINVAL;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FATFS_IDX) {
			Ret = SYS_READ_(_FatFS, reent, g_SysFile[fd].Fdsc, vbuf, size);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FULLFAT_IDX) {
			Ret = SYS_READ_(_FullFAT, reent, g_SysFile[fd].Fdsc, vbuf, size);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_UEFAT_IDX) {
			Ret = SYS_READ_(_ueFAT, reent, g_SysFile[fd].Fdsc, vbuf, size);
		}
	  #endif
	}

	if (Err != 0) {
		REENT_ERRNO = Err;
	}

	return(Ret);
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
/*   - Error																						*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

_ssize_t SC_WRITE_(struct _reent *reent, int fd, const void *vbuf, size_t len)
{
int      Err;										/* Value to set errno to						*/
_ssize_t Ret;										/* Return value									*/

	Err = 0;
	Ret = (_ssize_t)-1;

	if ((fd < 0)									/* Make sure the file descriptor is valid		*/
	||  (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {
		Err = EBADF;
	}
	else if (g_SysFile[fd].InUse == 0) {			/* Make sure the file is open					*/
		Err = EINVAL;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FATFS_IDX) {
			Ret = SYS_WRITE_(_FatFS, reent, g_SysFile[fd].Fdsc, vbuf, len);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FULLFAT_IDX) {
			Ret = SYS_WRITE_(_FullFAT, reent, g_SysFile[fd].Fdsc, vbuf, len);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_UEFAT_IDX) {
			Ret = SYS_WRITE_(_ueFAT, reent, g_SysFile[fd].Fdsc, vbuf, len);
		}
	  #endif
	}

	if (Err != 0) {
		REENT_ERRNO = Err;
	}

	return(Ret);
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
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

off_t SC_LSEEK_(struct _reent *reent, int fd, off_t offset, int whence)
{
int      Err;										/* Value to set errno to						*/
int Ret;											/* Return value									*/

	Err = 0;
	Ret = -1;

	if ((fd < 0)									/* Make sure the file descriptor is valid		*/
	||  (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {
		Err = EBADF;
	}
	else if (g_SysFile[fd].InUse == 0) {			/* Make sure the file is open					*/
		Err = EINVAL;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FATFS_IDX) {
			Ret = SYS_LSEEK_(_FatFS, reent, g_SysFile[fd].Fdsc, offset, whence);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FULLFAT_IDX) {
			Ret = SYS_LSEEK_(_FullFAT, reent, g_SysFile[fd].Fdsc, offset, whence);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_UEFAT_IDX) {
			Ret = SYS_LSEEK_(_ueFAT, reent, g_SysFile[fd].Fdsc, offset, whence);
		}
	  #endif
	}

	if (Err != 0) {
		REENT_ERRNO = Err;
	}

	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    rename / _rename_r (2)																*/
/*																									*/
/* _rename_r - rename a file / change the location of a file										*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _rename_r(struct _reent *reent, const char *old, const char *new);						*/
/*																									*/
/* ARGUMENTS:																						*/
/*		reent : task's re-entrance data (GCC only)													*/
/*		old   : old (original) path / file name														*/
/*		new   : new path / file name																*/
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

int SC_RENAME_(struct _reent *reent, const char *old, const char *new)
{
char Buf[512];										/* Buffer used when copying the file			*/
int  DrvNew;										/* Drive # of the new file						*/
int  DrvOld;										/* Drive # of the old file						*/
int  Err;											/* Value to set errno to						*/
int  FDnew;											/* new file descriptor							*/
int  FDold;											/* old file descriptor							*/
int  FidxNew;										/* FS stack index of old file					*/
int  FidxOld;										/* FS stack index of old file					*/
int  ii;											/* General purpose								*/
int  Ret;											/* Return value from FS stack calls				*/
char NewPath[SYS_CALL_MAX_PATH+1];					/* Real path using logical drive #				*/
char OldPath[SYS_CALL_MAX_PATH+1];					/* Real path using logical drive #				*/
struct stat Pstat;									/* To retrieve access mode of old				*/

	Err = 0;
	Ret = 0;										/* Assume OK									*/
	Pstat.st_mode = 0;								/* Remove possible compiler warning				*/

	DrvNew =  RealPath(&NewPath[0], new, &FidxNew);	/* Validate "path" and get physical path		*/
	DrvOld =  RealPath(&OldPath[0], old, &FidxOld);	/* Validate "path" and get physical path		*/

	if ((DrvNew < 0)
	||  (DrvOld < 0)) {
		REENT_ERRNO = Err;
		Ret = -1;
	}
	else {
		if (FidxNew == FidxOld) {					/* Same FS stack, use rename_XXX() directly		*/
		  #if ((SYS_CALL_MULTI_FATFS) != 0)
			if (FidxNew == MULTI_FATFS_IDX) {
				Ret = SYS_RENAME_(_FatFS, reent, &OldPath[0], &NewPath[0]);
			}
		  #endif

		  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
			if (FidxNew == MULTI_FULLFAT_IDX) {
				Ret = SYS_RENAME_(_FullFAT, reent, &OldPath[0], &NewPath[0]);
			}
		  #endif

		  #if ((SYS_CALL_MULTI_UEFAT) != 0)
			if (FidxNew == MULTI_UEFAT_IDX) {
				Ret = SYS_RENAME_(_ueFAT, reent, &OldPath[0], &NewPath[0]);
			}
		  #endif
		}

		else {										/* Across FS stacks, copy & unlink				*/
			FDnew = -1;
			FDold = SC_OPEN_(reent, &OldPath[0], O_RDONLY, 0);	/* Open the old one read-only		*/
			if (FDold < 0) {
				Ret = -1;
			}

			Ret |= SC_FSTAT_(reent, FDold, &Pstat);	/* The old is open, get old access mode as it	*/
													/* will be applied to the new one				*/
			if (Ret == 0) {							/* Old is open, open the new as writable		*/
				FDnew = SC_OPEN_(reent, &NewPath[0], O_CREAT|O_RDWR, Pstat.st_mode);
				if (FDnew < 0) {					/* and set mode to old at close					*/
					Ret = -1;
				}
			}

			if (Ret == 0) {							/* When both files are open OK					*/
				do {								/* Copy the contents of old into the new		*/
					ii = SC_READ_(reent, FDold, &Buf[0], sizeof(Buf));
					if (ii > 0) {
						ii = SC_WRITE_(reent, FDnew, &Buf[0], ii);
					}
				} while (ii > 0);
			}

			if (FDold >= 0) {						/* If old one was open successfully, close it	*/
				Ret |= SC_CLOSE_(reent, FDold);
			}

			if (FDnew >= 0) {						/* If new one was open successfully, close it	*/
				Ret |= SC_CLOSE_(reent, FDnew);
			}

			if (Ret == 0) {							/* The file has been copied successfully		*/
				Ret = SC_UNLINK_(reent, &OldPath[0]);	/* unlink the old one						*/
			}
			else if (FDnew >= 0) {					/* File was not copied successfully				*/
				SC_UNLINK_(reent, &NewPath[0]);		/* unlink the new one if was created			*/
			}										/* Ignore unlink error as file may not exist	*/
		}											/* No need to preserve errno, Err will set it	*/
	}

	return(Ret);
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
/*		new   : new path / file name or path / directory name										*/
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

int SC_UNLINK_(struct _reent *reent, const char *path)
{
int  Drv;											/* Drive #										*/
int  Fidx;											/* File system stack index						*/
int  Ret;											/* Return value									*/
char Rpath[SYS_CALL_MAX_PATH+1];					/* Real path using logical drive #				*/

	Ret = -1;	

	Drv = RealPath(&Rpath[0], path, &Fidx);			/* Get real path & validate if FS is mounted	*/
	if (Drv < 0) {									/* Invalid drive or is a /dev					*/
		strcpy(&Rpath[0], " :");					/* And make sure to use an invalid name			*/
		REENT_ERRNO = ENOENT;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (Fidx == MULTI_FATFS_IDX) {
			Ret = SYS_UNLINK_(_FatFS, reent, &Rpath[0]);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (Fidx == MULTI_FULLFAT_IDX) {
			Ret = SYS_UNLINK_(_FullFAT, reent, &Rpath[0]);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (Fidx == MULTI_UEFAT_IDX) {
			Ret = SYS_UNLINK_(_ueFAT, reent, &Rpath[0]);
		}
	  #endif
	}

	return(Ret);
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

int dup(int fd)
{
int Err;											/* Value to set errno to						*/
int NewFd;											/* Duplicate file descriptor					*/
int Ret;											/* Specific FS dup() return value				*/

	Err = 0;
	Ret = -1;

	if ((fd < 3)									/* Make sure the file descriptor is valid		*/
	||  (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {	/* Can't dup stdio				*/
		Err = EBADF;
	}
	else if (g_SysFile[fd].InUse == 0) {			/* Make sure the file is open					*/
		Err = EINVAL;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FATFS_IDX) {
			Ret = SYS_DUP_(_FatFS, g_SysFile[fd].Fdsc);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FULLFAT_IDX) {
			Ret = SYS_DUP_(_FullFAT, g_SysFile[fd].Fdsc);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_UEFAT_IDX) {
			Ret = SYS_DUP_(_ueFAT, g_SysFile[fd].Fdsc);
		}
	  #endif
	}

	NewFd = -1;
	if (Ret >= 0) {
		if (0 != MTXlock(g_GlbMtx, -1)) {			/* Although infinte wait, check in case			*/
			Err = EBUSY;							/* deadlock protection is enable				*/
		}
		else {
			for (NewFd=0 ; NewFd<(int)sizeof(g_SysFile)/(int)sizeof(g_SysFile[0]) ; NewFd++) {
				if (g_SysFile[NewFd].InUse == 0) {
					break;
				}
			}
			if (NewFd < (int)sizeof(g_SysFile)/(int)sizeof(g_SysFile[0])) {
				g_SysFile[NewFd].Fdsc  = Ret;
				g_SysFile[NewFd].Fsys  = g_SysFile[fd].Fsys;
				g_SysFile[NewFd].InUse = 1;
			}
			else {
				Err = ENFILE;
			}

			MTXunlock(g_GlbMtx);
		}
	}

	if (Err != 0) {
		REENT_ERRNO_ = Err;
		NewFd        = -1;
	}

	return(NewFd);
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

char *getcwd(char *buf, size_t size)
{
int  Drv;											/* Drive # of the current directory				*/
int  Fidx;
char Rpath[SYS_CALL_MAX_PATH+1];

	Drv = RealPath(&Rpath[0], ".", &Fidx);			/* "." makes RealPath() return physical CWD		*/
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

int chdir(const char *path)
{
int  Drv;											/* Drive number of the directory to set			*/
int  Fidx;											/* File system stack index						*/
int  Ret;											/* Return value									*/
char Rpath[SYS_CALL_MAX_PATH+1];					/* We use this in case the directory is invalid	*/

	Ret = -1;

	Drv = RealPath(&Rpath[0], path, &Fidx);			/* Get real path & validate if FS is mounted	*/
	if (Drv < 0) {									/* Invalid drive or is a /dev					*/
		strcpy(&Rpath[0], " :");					/* And make sure to use an invalid name			*/
		REENT_ERRNO_ = ENOENT;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (Fidx == MULTI_FATFS_IDX) {
			Ret = SYS_CHDIR_(_FatFS, &Rpath[0]);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (Fidx == MULTI_FULLFAT_IDX) {
			Ret = SYS_CHDIR_(_FullFAT, &Rpath[0]);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (Fidx == MULTI_UEFAT_IDX) {
			Ret = SYS_CHDIR_(_ueFAT, &Rpath[0]);
		}
	  #endif
	}

	return(Ret);
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
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_MKDIR_(struct _reent *reent, const char *path, int mode)
{
int  Drv;											/* Drive number									*/
int  Fidx;											/* FS stack index								*/
char Rpath[SYS_CALL_MAX_PATH+1];
int  Ret;											/* Return value									*/

	Ret = -1;

	Drv = RealPath(&Rpath[0], path, &Fidx);
	if (Drv < 0) {									/* Invalid drive or is a /dev					*/
		strcpy(&Rpath[0], " :");					/* And make sure to use an invalid name			*/
		REENT_ERRNO = ENOENT;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (Fidx == MULTI_FATFS_IDX) {
			Ret = SYS_MKDIR_(_FatFS, reent, &Rpath[0], mode);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (Fidx == MULTI_FULLFAT_IDX) {
			Ret = SYS_MKDIR_(_FullFAT, reent, &Rpath[0], mode);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (Fidx == MULTI_UEFAT_IDX) {
			Ret = SYS_MKDIR_(_ueFAT, reent, &Rpath[0], mode);
		}
	  #endif
	}

	return(Ret);
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
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SC_FSTAT_(struct _reent *reent, int fd, struct stat *pstat)
{
int Err;											/* Value to set errno to						*/
int Ret;											/* Return value									*/

	Err = 0;
	Ret = -1;

	if ((fd < 0)									/* Make sure the file descriptor is valid		*/
	||  (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {
		Err = EBADF;
	}
	else if (g_SysFile[fd].InUse == 0) {			/* Make sure is open							*/
		Err = EINVAL;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FATFS_IDX) {
			Ret = SYS_FSTAT_(_FatFS, reent, g_SysFile[fd].Fdsc, pstat);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FULLFAT_IDX) {
			Ret = SYS_FSTAT_(_FullFAT, reent, g_SysFile[fd].Fdsc, pstat);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_UEFAT_IDX) {
			Ret = SYS_FSTAT_(_ueFAT, reent, g_SysFile[fd].Fdsc, pstat);
		}
	  #endif
	}

	if (Err != 0) {
		REENT_ERRNO = Err;
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

int SC_STAT_(struct _reent *reent, const char *path, struct stat *pstat)
{
int  Drv;											/* Drive number									*/
int  Fidx;											/* FS stack index								*/
char Rpath[SYS_CALL_MAX_PATH+1];
int  Ret;											/* Return value									*/

	Ret = -1;

	Drv = RealPath(&Rpath[0], path, &Fidx);
	if (Drv < 0) {									/* Invalid drive or it's a /dev					*/
		strcpy(&Rpath[0], " :");					/* And make sure to use an invalid name			*/
		REENT_ERRNO = ENOENT;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (Fidx == MULTI_FATFS_IDX) {
			Ret = SYS_STAT_(_FatFS, reent, &Rpath[0], pstat);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (Fidx == MULTI_FULLFAT_IDX) {
			Ret = SYS_STAT_(_FullFAT, reent, &Rpath[0], pstat);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (Fidx == MULTI_UEFAT_IDX) {
			Ret = SYS_STAT_(_ueFAT, reent, &Rpath[0], pstat);
		}
	  #endif
	}

	return(Ret);
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

int fstatfs(int fs, struct statfs *buf)
{
int Err;											/* Value to set errno to						*/
int Fidx;											/* FS stack index								*/
int Ret;											/* Return value									*/

	Err =  0;
	Ret = -1;

	if ((fs < 0)									/* Make sure the file descriptor is valid		*/
	||  (fs >= (int)(sizeof(g_SysMnt)/sizeof(g_SysMnt[0])))) {
		Err = EBADF;
	}
	else if (g_SysMnt[fs].MntPoint[0] == '\0') {	/* Make sure is mounted							*/
		Err = EINVAL;
	}
	else {
		Fidx = g_SysMnt[fs].Fsys;

	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (Fidx == MULTI_FATFS_IDX) {
			Ret = SYS_FSTATFS_(_FatFS, fs, buf);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (Fidx == MULTI_FULLFAT_IDX) {
			Ret = SYS_FSTATFS_(_FullFAT, fs, buf);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (Fidx == MULTI_UEFAT_IDX) {
			Ret = SYS_FSTATFS_(_ueFAT, fs, buf);
		}
	  #endif
	}

	if (Err != 0) {
		REENT_ERRNO_ = Err;
	}

	return(Ret);
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

int statfs(const char *path, struct statfs *buf)
{
int  Drv;											/* Drive number									*/
int  Fidx;											/* FS stack index								*/
char Rpath[SYS_CALL_MAX_PATH+1];
int  Ret;											/* Return value									*/

	Ret = -1;

	Drv = RealPath(&Rpath[0], path, &Fidx);
	if (Drv < 0) {									/* Invalid drive or it is a /dev				*/
		strcpy(&Rpath[0], " :");					/* And make sure to use an invalid name			*/
		REENT_ERRNO_ = ENOENT;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (Fidx == MULTI_FATFS_IDX) {
			Ret = SYS_STATFS_(_FatFS, &Rpath[0], buf);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (Fidx == MULTI_FULLFAT_IDX) {
			Ret = SYS_STATFS_(_FullFAT, &Rpath[0], buf);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (Fidx == MULTI_UEFAT_IDX) {
			Ret = SYS_STATFS_(_ueFAT, &Rpath[0], buf);
		}
	  #endif
	}

	return(Ret);
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
/*		!=  0 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int chown(const char *path, uid_t owner, gid_t group)
{
int  Drv;											/* Drive number									*/
int  Fidx;											/* FS stack index								*/
char Rpath[SYS_CALL_MAX_PATH+1];
int  Ret;											/* Return value									*/

	Ret = -1;

	Drv = RealPath(&Rpath[0], path, &Fidx);
	if (Drv < 0) {									/* Invalid drive or it is a /dev				*/
		strcpy(&Rpath[0], " :");					/* And make sure to use an invalid name			*/
		REENT_ERRNO_ = ENOENT;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (Fidx == MULTI_FATFS_IDX) {
			Ret = SYS_CHOWN_(_FatFS, &Rpath[0], owner, group);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (Fidx == MULTI_FULLFAT_IDX) {
			Ret = SYS_CHOWN_(_FullFAT, &Rpath[0], owner, group);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (Fidx == MULTI_UEFAT_IDX) {
			Ret = SYS_CHOWN_(_ueFAT, &Rpath[0], owner, group);
		}
	  #endif
	}

	return(Ret);
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
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

mode_t umask(mode_t mask)
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
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int chmod(const char *path, mode_t mode)
{
int  Drv;											/* Drive # of the file / directory				*/
int  Fidx;											/* FS stack index where the file is located		*/
int  Ret;											/* Return value									*/
char Rpath[SYS_CALL_MAX_PATH+1];

	Ret = -1;

	Drv = RealPath(&Rpath[0], path, &Fidx);
	if (Drv < 0) {									/* Invalid drive or /dev						*/
		strcpy(&Rpath[0], " :");					/* Make sure to use an invalid name				*/
		REENT_ERRNO_ = ENOENT;
	} 
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (Fidx == MULTI_FATFS_IDX) {
			Ret = SYS_CHMOD_(_FatFS, &Rpath[0], mode);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (Fidx == MULTI_FULLFAT_IDX) {
			Ret = SYS_CHMOD_(_FullFAT, &Rpath[0], mode);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (Fidx == MULTI_UEFAT_IDX) {
			Ret = SYS_CHMOD_(_ueFAT, &Rpath[0], mode);
		}
	  #endif
	}

	return(Ret);
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
/*		        In option, N: can be pre-pended with the FS stack i.e.								*/
/*		             N:<FS> where <FS> is the file system stack to use for mounting					*/
/*				     <FS> is case insensitive and can be FATFS, FULLFAT, UEFAT, etc					*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Non-standard system call mount() function.													*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int mount(const char *type, void *dir, int flags, void *data)
{
char  DrvNmb[3];									/* Drive # string as required by all FS mount()	*/
int   Drv;											/* Device number to mount						*/
int   Err;											/* Error tracking								*/
int   ii;											/* General purpose								*/
char *Ptr;											/* "data" argument is the logical drive #		*/
int   MntRes;;										/* Result when calling the FS stack mount()		*/
int   Select;										/* Selected FS stack specified in "data"		*/

	Drv = 0;
	Err = 0;
	Ptr = (char *)data;

	if ((0 != strcasecmp(type, FS_TYPE_NAME_AUTO))	/* Check for a valid file system types			*/
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_FAT12))
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_FAT16))
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_FAT32))
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_EXFAT))) {
		Err = EINVAL;								/* Invalid parameter							*/
	}
													/* Double check the validity of the drive #		*/
	if (Err == 0) {									/* Must be "n" or "n:" where 'n' is a digit		*/
		if (!isdigit((int)Ptr[0])) {				/* First char must be a digit					*/
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

	DrvNmb[0] = Ptr[0];
	DrvNmb[1] = ':';
	DrvNmb[2] = '\0';

	Select = -1;
	if ((Err == 0)									/* Check for a selected FS stack				*/
	&&  (Ptr[1] == ':')) {
		if (0 == strcasecmp(&Ptr[2], "FATFS")) {	/* If the FS stack is not included, is OK as	*/
			Select = MULTI_FATFS_IDX;				/* value for MULTI_????_IDX is -1 (not selected)*/
		}
		else if (0 == strcasecmp(&Ptr[2], "FULLFAT")) {
			Select = MULTI_FULLFAT_IDX;
		}
		else if (0 == strcasecmp(&Ptr[2], "UEFAT")) {
			Select = MULTI_UEFAT_IDX;
		}
	}

	MntRes = -1;
	if (Err == 0) {
		if (Select >= 0) {							/* Selected FS stack to use to mount			*/
		  #if ((SYS_CALL_MULTI_FATFS) != 0)
			if (Select == MULTI_FATFS_IDX) {
				MntRes = SYS_MOUNT_(_FatFS, type, dir, flags, &DrvNmb[0]);
			}
		  #endif
		  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
			if (Select == MULTI_FULLFAT_IDX) {
				MntRes = SYS_MOUNT_(_FullFAT, type, dir, flags, &DrvNmb[0]);
			}
		  #endif
		  #if ((SYS_CALL_MULTI_UEFAT) != 0)
			if (Select == MULTI_UEFAT_IDX) {
				MntRes = SYS_MOUNT_(_ueFAT, type, dir, flags, &DrvNmb[0]);
			}
		  #endif
			g_SysMnt[Drv].Fsys = Select;			/* Not valid until .MntPoint[0] != '\0'			*/
		}

		else if (0 == strcasecmp(type, FS_TYPE_NAME_AUTO)) {/* Auto type, try all FS stacks until OK*/
		  #if ((SYS_CALL_MULTI_FATFS) != 0)
			if (MntRes != 0) {
				MntRes = SYS_MOUNT_(_FatFS, type, dir, flags, &DrvNmb[0]);
				g_SysMnt[Drv].Fsys = MULTI_FATFS_IDX;
			}
		  #endif

		  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
			if (MntRes != 0) {
				MntRes = SYS_MOUNT_(_FullFAT,  type, dir, flags, &DrvNmb[0]);
				g_SysMnt[Drv].Fsys = MULTI_FULLFAT_IDX;
			}
		  #endif

		  #if ((SYS_CALL_MULTI_UEFAT) != 0)
			if (MntRes != 0) {
				MntRes = SYS_MOUNT_(_ueFAT, type, dir, flags, &DrvNmb[0]);
				g_SysMnt[Drv].Fsys = MULTI_UEFAT_IDX;
			}
		  #endif
		}

		else if (0 == strcasecmp(type, FS_TYPE_NAME_EXFAT)) {	/* exFAT is only FatFS				*/
		  #if ((SYS_CALL_MULTI_FATFS) != 0)
			if (MntRes != 0) {
				MntRes = SYS_MOUNT_(_FatFS, type, dir, flags, &DrvNmb[0]);
				g_SysMnt[Drv].Fsys = MULTI_FATFS_IDX;
			}
		  #endif
		}

		else {												/* non-exFAT formatting, all FS are		*/
		  #if ((SYS_CALL_MULTI_FATFS) != 0)					/* able to format FAT16 & FAT32			*/
			if (MntRes != 0) {
				MntRes = SYS_MOUNT_(_FatFS, type, dir, flags, &DrvNmb[0]);
				g_SysMnt[Drv].Fsys = MULTI_FATFS_IDX;
			}
		  #endif

		  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
			if (MntRes != 0) {
				MntRes = SYS_MOUNT_(_FullFAT,  type, dir, flags, &DrvNmb[0]);
				g_SysMnt[Drv].Fsys = MULTI_FULLFAT_IDX;
			}
		  #endif

		  #if ((SYS_CALL_MULTI_UEFAT) != 0)
			if (MntRes != 0) {
				MntRes = SYS_MOUNT_(_ueFAT, type, dir, flags, &DrvNmb[0]);
				g_SysMnt[Drv].Fsys = MULTI_UEFAT_IDX;
			}
		  #endif
		}
	}

	if (MntRes == 0) {								/* Successful mount, memo the mount point		*/
		strcpy(&g_SysMnt[Drv].MntPoint[0], (char *)dir);
	}

	if (Err != 0) {
		REENT_ERRNO_ = Err;
	}

	return(MntRes);
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

int unmount(const char *dir, int flags)
{
int Drv;											/* Device number to mount						*/
int Err;											/* Value to set errno to						*/
int ii;												/* General purpose								*/
int jj;												/* General purpose								*/
int Len;											/* String length of the argument type			*/
int Ret;											/* Return value									*/

	Drv = -1;
	Err =  0;
	Ret =  0;

	Len = 0;										/* Find the drive of the mount point			*/
	for (ii=0 ; ii<(int)((sizeof(g_SysMnt)/sizeof(g_SysMnt[0]))) ; ii++) {
		jj = strlen(&(g_SysMnt[ii].MntPoint[0]));
		if ((jj != 0)
		&&  (jj > Len)
		&&  (0 == strncmp(&(g_SysMnt[ii].MntPoint[0]), dir, jj))) {
			Drv = ii;
			Len = jj;
		}
	}

	if (Drv < 0) {									/* Invalid drive or is a /dev or unknown mount	*/
		Ret = -1;
		Err = EINVAL;
	}

	if ((Ret == 0)
	&&  (g_SysMnt[Drv].MntPoint[0] == '\0')) {		/* Drv is the FS descriptor to use				*/
		Ret = -1;
		Err = ENODEV;								/* Must be mounted to be unmounted				*/
	}

	if (Ret == 0) {
													/* Close all files that are open on that drive	*/
		for (ii=3 ; ii<(int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])) ; ii++) {	/* Skip stdio		*/
			if ((g_SysFile[ii].InUse != 0)
			&&  (g_SysFile[ii].Fsys == Drv)) {
			  #if ((SYS_CALL_MULTI_FATFS) != 0)
				if (Drv == MULTI_FATFS_IDX) {
					SYS_CLOSE_(_FatFS, __getreent(), g_SysFile[ii].Fdsc);
				}
			  #endif
			  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
				if (Drv == MULTI_FULLFAT_IDX) {
					SYS_CLOSE_(_FullFAT, __getreent(), g_SysFile[ii].Fdsc);
				}
			  #endif
			  #if ((SYS_CALL_MULTI_UEFAT) != 0)
				if (Drv == MULTI_UEFAT_IDX) {
					SYS_CLOSE_(_ueFAT, __getreent(), g_SysFile[ii].Fdsc);
				}
			  #endif
			}
		}
													/* Close all dirs that are open on that drive	*/
		for (ii=0 ; ii<(int)(sizeof(g_SysDir)/sizeof(g_SysDir[0])) ; ii++) {
			if ((g_SysDir[ii].Ddsc != (DIR_t *)NULL)
			&&  (g_SysDir[ii].Fsys == Drv)) {
			  #if ((SYS_CALL_MULTI_FATFS) != 0)
				if (Drv == MULTI_FATFS_IDX) {
					SYS_CLOSEDIR_(_FatFS, g_SysDir[ii].Ddsc);
				}
			  #endif
			  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
				if (Drv == MULTI_FULLFAT_IDX) {
					SYS_CLOSEDIR_(_FullFAT, g_SysDir[ii].Ddsc);
				}
			  #endif
			  #if ((SYS_CALL_MULTI_UEFAT) != 0)
				if (Drv == MULTI_UEFAT_IDX) {
					SYS_CLOSEDIR_(_ueFAT, g_SysDir[ii].Ddsc);
				}
			  #endif
				g_SysDir[ii].Ddsc = (DIR_t *)NULL;
			}
		}

		ii = g_SysMnt[Drv].Fsys;					/* Use the FS stack index to select the call	*/
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (ii == MULTI_FATFS_IDX) {
			Ret = SYS_UNMOUNT_(_FatFS, dir, flags);
		}
	  #endif
	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (ii == MULTI_FULLFAT_IDX) {
			Ret = SYS_UNMOUNT_(_FullFAT, dir, flags);
		}
	  #endif
	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (ii == MULTI_UEFAT_IDX) {
			Ret = SYS_UNMOUNT_(_ueFAT, dir, flags);
		}
	  #endif

		if (Ret == 0) {
			g_SysMnt[Drv].MntPoint[0] = '\0';
		}
	}

	if (Err != 0) {
		REENT_ERRNO_ = Err;
	}

	return(Ret);
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
/*		        In option, N: can be pre-pended with the FS stack i.e.								*/
/*		             N:<FS> where <FS> is the file system stack to use for formatting				*/
/*				     <FS> is case insensitive and can be FATFS, FULLFAT, UEFAT, etc					*/
/*																									*/
/* RETURN VALUE:																					*/
/*		==  0 : success																				*/
/*		== -1 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*		Non-standard function.																		*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int mkfs(const char *type, void *data)
{
char  DrvNmb[3];									/* Drive # string as required by all FS mkfs()	*/
int   Drv;											/* Device number to format						*/
int   Err;											/* Error tracking								*/
char *Ptr;											/* "data" argument is the logical drive #		*/
int   MkfsRes;;										/* Result when calling the FS stack mkfs()		*/
int   Select;										/* Selected FS stack specified in "data"		*/

	Drv = 0;
	Err = 0;
	Ptr = (char *)data;

	if ((0 != strcasecmp(type, FS_TYPE_NAME_AUTO))	/* Check for a valid file system types			*/
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_FAT12))
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_FAT16))
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_FAT32))
	&&  (0 != strcasecmp(type, FS_TYPE_NAME_EXFAT))) {
		Err = EINVAL;								/* Invalid parameter							*/
	}
													/* Double check the validity of the drive #		*/
	if (Err == 0) {									/* Must be "n" or "n:" where 'n' is a digit		*/
		if (!isdigit((int)Ptr[0])) {				/* First char must be a digit					*/
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

	if (Err == 0) {									/* If the drive is mounted, unmount it			*/
		if (g_SysMnt[Drv].MntPoint[0] != '\0') {
			unmount(&g_SysMnt[Drv].MntPoint[0], 0);
		}
	}

	DrvNmb[0] = Ptr[0];
	DrvNmb[1] = ':';
	DrvNmb[2] = '\0';

	Select = -1;
	if ((Err == 0)									/* Check for a selected FS stack				*/
	&&  (Ptr[1] == ':')) {
		if (0 == strcasecmp(&Ptr[2], "FATFS")) {	/* If the FS stack is not included, is OK as	*/
			Select = MULTI_FATFS_IDX;				/* value for MULTI_????_IDX is -1 (not selected)*/
		}
		else if (0 == strcasecmp(&Ptr[2], "FULLFAT")) {
			Select = MULTI_FULLFAT_IDX;
		}
		else if (0 == strcasecmp(&Ptr[2], "UEFAT")) {
			Select = MULTI_UEFAT_IDX;
		}
	}

	MkfsRes = -1;
	if (Err == 0) {
		if (Select >= 0) {							/* Selected FS stack to use to format			*/
		  #if ((SYS_CALL_MULTI_FATFS) != 0)
			if (Select == MULTI_FATFS_IDX) {
				MkfsRes = SYS_MKFS_(_FatFS, type, &DrvNmb[0]);
			}
		  #endif
		  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
			if (Select == MULTI_FULLFAT_IDX) {
				MkfsRes = SYS_MKFS_(_FullFAT, type, &DrvNmb[0]);
			}
		  #endif
		  #if ((SYS_CALL_MULTI_UEFAT) != 0)
			if (Select == MULTI_UEFAT_IDX) {
				MkfsRes = SYS_MKFS_(_ueFAT, type,  &DrvNmb[0]);
			}
		  #endif
		}

		else if (0 == strcasecmp(type, FS_TYPE_NAME_AUTO)) {/* Auto type, try all FS stacks until OK*/
		  #if ((SYS_CALL_MULTI_FATFS) != 0)
			if (MkfsRes != 0) {
				MkfsRes = SYS_MKFS_(_FatFS, type, &DrvNmb[0]);
			}
		  #endif

		  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
			if (MkfsRes != 0) {
				MkfsRes = SYS_MKFS_(_FullFAT,  type, &DrvNmb[0]);
			}
		  #endif

		  #if ((SYS_CALL_MULTI_UEFAT) != 0)
			if (MkfsRes != 0) {
				MkfsRes = SYS_MKFS_(_ueFAT, type, &DrvNmb[0]);
			}
		  #endif
		}

		else if (0 == strcasecmp(type, FS_TYPE_NAME_EXFAT)) {	/* exFAT is only FatFS				*/
		  #if ((SYS_CALL_MULTI_FATFS) != 0)
			if (MkfsRes != 0) {
				MkfsRes = SYS_MKFS_(_FatFS, type, &DrvNmb[0]);
			}
		  #endif
		}

		else {												/* non-exFAT formatting, all FS are		*/
		  #if ((SYS_CALL_MULTI_FATFS) != 0)					/* able to format FAT16 & FAT32			*/
			if (MkfsRes != 0) {
				MkfsRes = SYS_MKFS_(_FatFS, type, &DrvNmb[0]);
			}
		  #endif

		  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
			if (MkfsRes != 0) {
				MkfsRes = SYS_MKFS_(_FullFAT,  type, &DrvNmb[0]);
			}
		  #endif

		  #if ((SYS_CALL_MULTI_UEFAT) != 0)
			if (MkfsRes != 0) {
				MkfsRes = SYS_MKFS_(_ueFAT, type, &DrvNmb[0]);
			}
		  #endif
		}
	}

	if (MkfsRes != 0) {										/* To make sure, as required, to return	*/
		MkfsRes = -1;										/* -ve when there was an error			*/
	}

	return(MkfsRes);
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
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

DIR_t *opendir(const char *path)
{
int     dd;											/* Directory descriptor							*/
int     Drv;										/* Drive # of the directory to open				*/
int     Err;										/* Error tracking								*/
int     Fidx;
DIR_t  *Ret;
char    Rpath[SYS_CALL_MAX_PATH+1];					/* Real path using logical drive #				*/

	Err  = 0;
	Fidx = -1;										/* To remove compiler warning					*/
	Ret  = (DIR_t *)NULL;

	Drv = RealPath(&Rpath[0], path, &Fidx);
	if (Drv < -1) {									/* Is a /dev									*/
		Err = EINVAL;
	}
	else {
		if (0 != MTXlock(g_GlbMtx, -1)) {			/* Need global protection: modifying .IsOpen	*/
			Err = EBUSY;							/* Although infinte wait, check in case			*/
		}											/* deadlock protection is enable				*/
		else {
		  #if ((SYS_CALL_MULTI_FATFS) != 0)
				if (Ret == (DIR_t *)NULL) {
				if ((Fidx == MULTI_FATFS_IDX)
				||  (Drv == -1)) {					/* Needed if ls /dsk on mlount point /dsk/mmc	*/
					Ret = SYS_OPENDIR_(_FatFS, path);
				}
			}
		  #endif

		  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
			if (Ret == (DIR_t *)NULL) {
				if ((Fidx == MULTI_FULLFAT_IDX)
				||  (Drv == -1)) {					/* Needed if ls /dsk on mlount point /dsk/mmc	*/
					Ret = SYS_OPENDIR_(_FullFAT, path);
				}
			}
		  #endif

		  #if ((SYS_CALL_MULTI_UEFAT) != 0)
			if (Ret == (DIR_t *)NULL) {
				if ((Fidx == MULTI_UEFAT_IDX)
				||  (Drv == -1)) {					/* Needed if ls /dsk on mlount point /dsk/mmc	*/
					Ret = SYS_OPENDIR_(_ueFAT, path);
				}
			}
		  #endif

			if (Ret == (DIR_t *)NULL) {				/* Invalid directory							*/
				Err = EINVAL;
			}

			if (Err == 0) {							/* When successful opening, memo info			*/
				dd = (int)(sizeof(g_SysDir)/sizeof(g_SysDir[0]));
				for (dd=0 ; dd<(int)(sizeof(g_SysDir)/sizeof(g_SysDir[0])) ; dd++) {
					if (g_SysDir[dd].Ddsc == (DIR_t *)NULL) {/* Is unused, keep processing with it	*/
						break;						/* Not using local lock as loop & check is		*/
					}								/* very quick									*/
				}							

				if (dd >= (int)(sizeof(g_SysDir)/sizeof(g_SysDir[0]))) {	/* All entries are used	*/
					Err = ENFILE;					/* Maximum # of file reached					*/
					Ret = (DIR_t *)NULL;
				}
				else {								/* Found an unused directory descriptor			*/
					g_SysDir[dd].Ddsc  = Ret;
					g_SysDir[dd].Fsys  = g_SysMnt[Drv].Fsys;
					Ret                = (DIR_t *)&g_SysDir[dd];
				}
			}

			MTXunlock(g_GlbMtx);
		}
	}

	if (Err != 0) {									/* Set errno upon encountering an error			*/
		REENT_ERRNO_ = Err;
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
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int closedir(DIR_t *dirp)
{
int       Ret;
SysDir_t *MyDir;

	Ret   = -1;
	MyDir = (SysDir_t  *)dirp;

  #if ((SYS_CALL_MULTI_FATFS) != 0)
	if (MyDir->Fsys == MULTI_FATFS_IDX) {
		Ret = SYS_CLOSEDIR_(_FatFS, MyDir->Ddsc);
	}
  #endif

  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
	if (MyDir->Fsys == MULTI_FULLFAT_IDX) {
		Ret = SYS_CLOSEDIR_(_FullFAT, MyDir->Ddsc);
	}
  #endif

  #if ((SYS_CALL_MULTI_UEFAT) != 0)
	if (MyDir->Fsys == MULTI_UEFAT_IDX) {
		Ret = SYS_CLOSEDIR_(_ueFAT, MyDir->Ddsc);
	}
  #endif

	if (Ret == 0) {
		MyDir->Ddsc = (DIR_t *)NULL;
	}

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
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

struct dirent *readdir(DIR_t *dirp)
{
SysDir_t      *MyDir;
struct dirent *Ret;

	Ret   = (struct dirent *)NULL;
	MyDir = (SysDir_t  *)dirp;

  #if ((SYS_CALL_MULTI_FATFS) != 0)
	if (MyDir->Fsys == MULTI_FATFS_IDX) {
		Ret = SYS_READDIR_(_FatFS, MyDir->Ddsc);
	}
  #endif

  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
	if (MyDir->Fsys == MULTI_FULLFAT_IDX) {
		Ret = SYS_READDIR_(_FullFAT, MyDir->Ddsc);
	}
  #endif

  #if ((SYS_CALL_MULTI_UEFAT) != 0)
	if (MyDir->Fsys == MULTI_UEFAT_IDX) {
		Ret = SYS_READDIR_(_ueFAT, MyDir->Ddsc);
	}
  #endif

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
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

long telldir(DIR_t *dirp)
{
SysDir_t *MyDir;
long      Ret;

	Ret   = -1;
	MyDir = (SysDir_t  *)dirp;

  #if ((SYS_CALL_MULTI_FATFS) != 0)
	if (MyDir->Fsys == MULTI_FATFS_IDX) {
		Ret = SYS_TELLDIR_(_FatFS, MyDir->Ddsc);
	}
  #endif

  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
	if (MyDir->Fsys == MULTI_FULLFAT_IDX) {
		Ret = SYS_TELLDIR_(_FullFAT, MyDir->Ddsc);
	}
  #endif

  #if ((SYS_CALL_MULTI_UEFAT) != 0)
	if (MyDir->Fsys == MULTI_UEFAT_IDX) {
		Ret = SYS_TELLDIR_(_ueFAT, MyDir->Ddsc);
	}
  #endif

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
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void seekdir(DIR_t *dirp, long loc)
{
SysDir_t *MyDir;

	MyDir = (SysDir_t  *)dirp;

  #if ((SYS_CALL_MULTI_FATFS) != 0)
	if (MyDir->Fsys == MULTI_FATFS_IDX) {
		SYS_SEEKDIR_(_FatFS, MyDir->Ddsc, loc);
	}
  #endif

  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
	if (MyDir->Fsys == MULTI_FULLFAT_IDX) {
		SYS_SEEKDIR_(_FullFAT, MyDir->Ddsc, loc);
	}
  #endif

  #if ((SYS_CALL_MULTI_UEFAT) != 0)
	if (MyDir->Fsys == MULTI_UEFAT_IDX) {
		SYS_SEEKDIR_(_ueFAT, MyDir->Ddsc, loc);
	}
  #endif

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
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void rewinddir(DIR_t *dirp)
{
SysDir_t *MyDir;

	MyDir = (SysDir_t  *)dirp;

  #if ((SYS_CALL_MULTI_FATFS) != 0)
	if (MyDir->Fsys == MULTI_FATFS_IDX) {
		SYS_REWINDDIR_(_FatFS, MyDir->Ddsc);
	}
  #endif

  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
	if (MyDir->Fsys == MULTI_FULLFAT_IDX) {
		SYS_REWINDDIR_(_FullFAT, MyDir->Ddsc);
	}
  #endif

  #if ((SYS_CALL_MULTI_UEFAT) != 0)
	if (MyDir->Fsys == MULTI_UEFAT_IDX) {
		SYS_REWINDDIR_(_ueFAT, MyDir->Ddsc);
	}
  #endif

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

int SC_ISATTY_(struct _reent *reent, int fd)
{
	return((fd>=0 && (fd<3)));
}

/* ------------------------------------------------------------------------------------------------ */
/* devctl - device control																			*/
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

int devctl(int fd, const int *cfg)
{
int Err;											/* Value to set errno to						*/
int Ret;

	Err =  0;
	Ret = -1;

	if ((fd < 0)									/* Make sure the file descriptor is valid		*/
	||  (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {
		Err = EBADF;
	}
	else if (g_SysFile[fd].InUse == 0) {			/* Make sure the device is open					*/
		Err = EINVAL;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FATFS_IDX) {
			Ret = SYS_DEVCTL_(_FatFS, g_SysFile[fd].Fdsc, cfg);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_FULLFAT_IDX) {
			Ret = SYS_DEVCTL_(_FullFAT, g_SysFile[fd].Fdsc, cfg);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (g_SysFile[fd].Fsys == MULTI_UEFAT_IDX) {
			Ret = SYS_DEVCTL_(_ueFAT, g_SysFile[fd].Fdsc, cfg);
		}
	  #endif
	}

	if (Err != 0) {
		REENT_ERRNO_ = Err;
	}

	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* GetKey() : non-standard but ubiquitous 															*/
/*																									*/
/* Return:																							*/
/*         == 0 : no char available from stdin														*/
/*         != 0 : char obtained from stdin															*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int GetKey(void)
{
char ccc;

	ccc = (char)0;
	if (1 != uart_recv(G_UartDevIn, &ccc, -1)) {
		ccc = (char)0;
	}

	return(ccc);
}

/* ------------------------------------------------------------------------------------------------ */
/* media access re-mapping																			*/
/* ------------------------------------------------------------------------------------------------ */

int64_t media_size(int Drv)
{
int64_t Size;

	Size = (int64_t)-1;								/* Assume invalid drive #						*/

	if ((Drv >= 0)
	&&  (Drv < (int)sizeof(g_SysMnt)/(int)sizeof(g_SysMnt[0]))) {
		if (g_SysMnt[Drv].MntPoint[0] != '\0') {
		  #if ((SYS_CALL_MULTI_FATFS) != 0)
			if (g_SysMnt[Drv].Fsys == MULTI_FATFS_IDX) {
				Size = media_size_FatFS(Drv);
			}
		  #endif

		  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
			if (g_SysMnt[Drv].Fsys == MULTI_FULLFAT_IDX) {
				Size = media_size_FullFAT(Drv);			
			}
		  #endif

		  #if ((SYS_CALL_MULTI_UEFAT) != 0)
			if (g_SysMnt[Drv].Fsys == MULTI_UEFAT_IDX) {
				Size = media_size_ueFAT(Drv);
			}
		  #endif
		}	
	}
	return(Size);
}

/* ------------------------------------------------------------------------------------------------ */

int32_t media_blksize(int Drv)
{
int32_t Size;

	Size = (int32_t)-1;								/* Assume invalid drive #						*/

	if ((Drv >= 0)
	&&  (Drv < (int)sizeof(g_SysMnt)/(int)sizeof(g_SysMnt[0]))) {
		if (g_SysMnt[Drv].MntPoint[0] != '\0') {
		  #if ((SYS_CALL_MULTI_FATFS) != 0)
			if (g_SysMnt[Drv].Fsys == MULTI_FATFS_IDX) {
				Size = media_blksize_FatFS(Drv);
			}
		  #endif

		  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
			if (g_SysMnt[Drv].Fsys == MULTI_FULLFAT_IDX) {
				Size = media_blksize_FullFAT(Drv);			
			}
		  #endif

		  #if ((SYS_CALL_MULTI_UEFAT) != 0)
			if (g_SysMnt[Drv].Fsys == MULTI_UEFAT_IDX) {
				Size = media_blksize_ueFAT(Drv);
			}
		  #endif
		}	
	}

	return(Size);
}

/* ------------------------------------------------------------------------------------------------ */

int32_t media_sectsize(int Drv)
{
int32_t Size;

	Size = (int32_t)-1;								/* Assume invalid drive #						*/

	if ((Drv >= 0)
	&&  (Drv < (int)sizeof(g_SysMnt)/(int)sizeof(g_SysMnt[0]))) {
		if (g_SysMnt[Drv].MntPoint[0] != '\0') {
		  #if ((SYS_CALL_MULTI_FATFS) != 0)
			if (g_SysMnt[Drv].Fsys == MULTI_FATFS_IDX) {
				Size = media_sectsize_FatFS(Drv);
			}
		  #endif

		  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
			if (g_SysMnt[Drv].Fsys == MULTI_FULLFAT_IDX) {
				Size = media_sectsize_FullFAT(Drv);			
			}
		  #endif

		  #if ((SYS_CALL_MULTI_UEFAT) != 0)
			if (g_SysMnt[Drv].Fsys == MULTI_UEFAT_IDX) {
				Size = media_sectsize_ueFAT(Drv);
			}
		  #endif
		}	
	}

	return(Size);
}

/* ------------------------------------------------------------------------------------------------ */

const char *media_info(int Drv)
{
const char *Info;

	Info = (const char *)NULL;						/* Assume invalid drive #						*/

	if ((Drv >= 0)
	&&  (Drv < (int)sizeof(g_SysMnt)/(int)sizeof(g_SysMnt[0]))) {
		if (g_SysMnt[Drv].MntPoint[0] != '\0') {	/* The drive is mounted, use the mounting FS	*/
		  #if ((SYS_CALL_MULTI_FATFS) != 0)
			if (g_SysFile[Drv].Fsys == MULTI_FATFS_IDX) {
				Info = media_info_FatFS(Drv);
			}
		  #endif

		  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
			if (g_SysFile[Drv].Fsys == MULTI_FULLFAT_IDX) {
				Info = media_info_FullFAT(Drv);			
			}
		  #endif

		  #if ((SYS_CALL_MULTI_UEFAT) != 0)
			if (g_SysFile[Drv].Fsys == MULTI_UEFAT_IDX) {
				Info = media_info_ueFAT(Drv);
			}
		  #endif
		}
		else {										/* The drive hasn't been mounted, try FSes		*/
		  #if ((SYS_CALL_MULTI_FATFS) != 0)
			if (Info == (const char *)NULL) {
				Info = media_info_FatFS(Drv);
			}
		  #endif

		  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
			if (Info == (const char *)NULL) {
				Info = media_info_FullFAT(Drv);
			}
		  #endif

		  #if ((SYS_CALL_MULTI_UEFAT) != 0)
			if (Info == (const char *)NULL) {
				Info = media_info_ueFAT(Drv);
			}
		  #endif
		}
	}

	return(Info);
}

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

int _sys_ensure(FILEHANDLE fh)
{
int Err;											/* Value to set errno to						*/
int Ret;

	Err =  0;
	Ret = -1;

	if ((fh < 0)									/* Make sure the file descriptor is valid		*/
	||  (fh >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {
		Err = EBADF;
	}
	else if (g_SysFile[fh].InUse == 0) {			/* Make sure the device is open					*/
		Err = EINVAL;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (g_SysFile[fh].Fsys == MULTI_FATFS_IDX) {
			Ret = SYS_SYS_ENSURE_(_FatFS, g_SysFile[fh].Fdsc);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (g_SysFile[fh].Fsys == MULTI_FULLFAT_IDX) {
			Ret = SYS_SYS_ENSURE_(_FullFAT, g_SysFile[fh].Fdsc);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (g_SysFile[fh].Fsys == MULTI_UEFAT_IDX) {
			Ret = SYS_SYS_ENSURE_(_ueFAT, g_SysFile[fh].Fdsc);
		}
	  #endif
	}

	if (Err != 0) {
		REENT_ERRNO_ = Err;
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

long _sys_flen(FILEHANDLE fh)
{
int  Err;											/* Value to set errno to						*/
long Ret;

	Err =  0;
	Ret = -1;

	if ((fh < 0)									/* Make sure the file descriptor is valid		*/
	||  (fh >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])))) {
		Err = EBADF;
	}
	else if (g_SysFile[fh].InUse == 0) {			/* Make sure the device is open					*/
		Err = EINVAL;
	}
	else {
	  #if ((SYS_CALL_MULTI_FATFS) != 0)
		if (g_SysFile[fh].Fsys == MULTI_FATFS_IDX) {
			Ret = SYS_SYS_FLEN_(_FatFS, g_SysFile[fh].Fdsc);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_FULLFAT) != 0)
		if (g_SysFile[fh].Fsys == MULTI_FULLFAT_IDX) {
			Ret = SYS_SYS_FLEN_(_FullFAT, g_SysFile[fh].Fdsc);
		}
	  #endif

	  #if ((SYS_CALL_MULTI_UEFAT) != 0)
		if (g_SysFile[fh].Fsys == MULTI_UEFAT_IDX) {
			Ret = SYS_SYS_FLEN_(_ueFAT, g_SysFile[fh].Fdsc);
		}
	  #endif
	}

	if (Err != 0) {
		REENT_ERRNO_ = Err;
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
/*		int IARflush(int fd);																		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int IARflush(int fd)
{
	return((fd<3) ? 0 : -1);						/* When fd -ve return OK (is a flush all)		*/
}													/* Always OK with stdio							*/

#endif

/* EOF */
