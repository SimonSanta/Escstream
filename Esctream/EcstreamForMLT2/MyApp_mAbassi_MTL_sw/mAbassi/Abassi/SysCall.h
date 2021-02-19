/* ------------------------------------------------------------------------------------------------ */
/* FILE :		SysCall.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				System Calls: non-standard function prototypes										*/
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
/*	$Revision: 1.13 $																				*/
/*	$Date: 2019/01/10 18:06:19 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#if (!defined(__ABASSI_H__)) && (!defined(__MABASSI_H__)) && (!defined(__UABASSI_H__))
  #if defined(_UABASSI_)						 	/* It is the same include file in all cases.	*/
	#include "mAbassi.h"							/* There is a substitution during the release 	*/
  #elif defined (OS_N_CORE)							/* This file is the same for Abassi, mAbassi,	*/
	#include "mAbassi.h"							/* and uAbassi, so Code Time uses these checks	*/
  #else												/* to not have to keep 3 quasi identical		*/
	#include "mAbassi.h"								/* copies of this file							*/
  #endif
#endif

#ifdef __cplusplus
 extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Do all the includes needed to define all the system calls implemented							*/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__CC_ARM)								/* ARM CC										*/
  #include <rt_sys.h>
  typedef int      dev_t;
  typedef int      gid_t;
  typedef int      ino_t;
  typedef int      mode_t;
  typedef int      nlink_t;
  typedef int      off_t;
  typedef int64_t _ssize_t;
  typedef int      uid_t;

#elif defined(__ICCARM__) 							/* IAR / CLIB								*/	\
   || defined(__ICCAVR32__)
  typedef int      dev_t;
  typedef int      gid_t;
  typedef int      ino_t;
  typedef int      mode_t;
  typedef int      nlink_t;
  typedef int      off_t;
  typedef int64_t _ssize_t;
  typedef int      uid_t;

#elif defined(__GNUC__)  							/* GCC / Newlib									*/
  #include <reent.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/stat.h>
  #include <sys/time.h>
  #include <sys/types.h>

#else
  #error "Unsupported library"

#endif

#ifndef SYS_CALL_MAX_PATH							/* Maximum sting length for full path & fname	*/
  #define SYS_CALL_MAX_PATH		512
#endif

/* ------------------------------------------------------------------------------------------------ */
/* These function prototypes are defined here as they are not standard system call functions or 	*/
/* they have different argument / return value than the standard system call functions				*/
/* ------------------------------------------------------------------------------------------------ */

void SysCallInit(void);
 
/* ------------------------------------------------------------------------------------------------ */
/* GetKey()																							*/
/*																									*/
/* return																							*/
/*	        == 0 : no characters available on stdin													*/
/*	        != 0 : character (cast to int) read from stdin											*/
/* ------------------------------------------------------------------------------------------------ */

extern int GetKey(void);


/* ------------------------------------------------------------------------------------------------ */
/* mkfs - make a file system, AKA formatting a device												*/
/*	return																							*/

int mkfs(const char *type, void *data);

/* ------------------------------------------------------------------------------------------------ */
/* mount - mounting operations (mount of un-mount) on a device										*/

extern int mount(const char *type, void *dir, int flags, void *data);
extern int unmount(const char *dir, int flags);

#if (!defined(MNT_RDONLY) && !defined(MNT_SYNCHRONOUS) && !defined(MNT_NOEXEC)						\
 &&  !defined(MNT_NOSUID) && !defined(MNT_NODEV) && !defined(MNT_UNION) && !defined(MNT_ASYNC)		\
 &&  !defined(MNT_UPDATE))
  #define MNT_RDONLY		0x0001
  #define MNT_SYNCHRONOUS	0x0002
  #define MNT_NOEXEC		0x0004
  #define MNT_NOSUID		0x0008
  #define MNT_NODEV			0x0010
  #define MNT_UNION			0x0020
  #define MNT_ASYNC			0x0020
  #define MNT_UPDATE		0x0040

#elif !(defined(MNT_RDONLY) && defined(MNT_SYNCHRONOUS) && defined(MNT_NOEXEC)						\
   &&   defined(MNT_NOSUID) && defined(MNT_NODEV) && defined(MNT_UNION) && defined(MNT_ASYNC)		\
   && defined(MNT_UPDATE))
  #error "Definitions missing for MNT_?????"

#endif


/* ------------------------------------------------------------------------------------------------ */

extern int chmod(const char *path, mode_t mode);
extern int devctl(int fd, const int *cfg);

/* ------------------------------------------------------------------------------------------------ */
/* File access modes																				*/
/* Is most likely not defined by the compiler for the target platform								*/

#ifndef S_IRWXU
  #define	S_IRWXU 	(S_IRUSR | S_IWUSR | S_IXUSR)
#endif
#ifndef S_IRUSR
  #define	S_IRUSR		((mode_t)000400)			/* Read permission, owner						*/
#endif
#ifndef S_IWUSR
  #define	S_IWUSR		((mode_t)000200)			/* Write permission, owner						*/
#endif
#ifndef S_IXUSR
  #define	S_IXUSR 	((mode_t)000100)			/* Execute/search permission, owner				*/
#endif
#ifndef S_IRWXG
  #define	S_IRWXG		(S_IRGRP | S_IWGRP | S_IXGRP)
#endif
#ifndef S_IRGRP
  #define	S_IRGRP		((mode_t)000040)			/* Read permission, group 						*/
#endif
#ifndef S_IWGRP
  #define	S_IWGRP		((mode_t)000020)			/* Write permission, group						*/
#endif
#ifndef S_IXGRP
  #define	S_IXGRP 	((mode_t)000010)			/* Execute/search permission, group				*/
#endif
#ifndef S_IRWXO
  #define	S_IRWXO		(S_IROTH | S_IWOTH | S_IXOTH)
#endif
#ifndef S_IROTH
  #define	S_IROTH		((mode_t)000004)			/* Read permission, other						*/
#endif
#ifndef S_IWOTH
  #define	S_IWOTH		((mode_t)000002)			/* Write permission, other						*/
#endif
#ifndef S_IXOTH
  #define	S_IXOTH 	((mode_t)000001)			/* Execute/search permission, other				*/
#endif

#ifndef S_IFMT
  #define	S_IFMT		((mode_t)017000)			/* Type of file (MASK)							*/
#endif
#ifndef S_IFIFO
  #define	S_IFIFO		((mode_t)001000)			/* FIFO											*/
#endif
#ifndef S_IFCHR
  #define	S_IFCHR		((mode_t)002000)			/* Character special							*/
#endif
#ifndef S_IFDIR
  #define	S_IFDIR		((mode_t)004000)			/* Directory									*/
#endif
#ifndef S_IFBLK
  #define	S_IFBLK		((mode_t)006000)			/* Block special								*/
#endif
#ifndef S_IFREG
  #define	S_IFREG		((mode_t)010000)			/* Regular										*/
#endif
#ifndef S_IFLNK
  #define	S_IFLNK		((mode_t)012000)			/* Symbolic link								*/
#endif
#ifndef S_IFSOCK
  #define	S_IFSOCK	((mode_t)014000)			/* Socket										*/
#endif

#ifndef O_RDONLY
  #define O_RDONLY		0
#endif
#ifndef O_WRONLY
  #define O_WRONLY		1
#endif
#ifndef O_RDWR
  #define O_RDWR		2
#endif
#ifndef O_APPEND
  #define O_APPEND		0x0008
#endif
#ifndef O_CREAT
  #define O_CREAT		0x0200
#endif
#ifndef O_TRUNC
  #define O_TRUNC		0x0400
#endif
#ifndef O_EXCL
  #define O_EXCL		0x0800
#endif
#ifndef O_SYNC
  #define O_SYNC		0x2000
#endif
#ifndef O_NONBLOCK
  #define O_NONBLOCK	0x4000
#endif
#ifndef O_NOCTTY
  #define O_NOCTTY		0x8000
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Directory stuff																					*/
 
typedef struct SysDir DIR_t;						/* typedef of an opaque structure				*/
													/* Allows to be independent of underlying FS	*/
#ifndef _IFMT
  #define _IFMT			0170000
#endif
#ifndef _IFDIR
  #define _IFDIR		0040000
#endif
#ifndef _IFCHR
  #define _IFCHR		0020000
#endif
#ifndef _IFBLK
  #define _IFBLK		0060000
#endif
#ifndef _IFREG
  #define _IFREG		0100000
#endif
#ifndef _IFLNK
  #define _IFLNK		0120000
#endif
#ifndef _IFSOCK
  #define _IFSOCK		0140000
#endif
#ifndef _IFIFO
  #define _IFIFO		0010000
#endif

#ifndef DT_UNKNOWN
  #define DT_UNKNOWN	000
#endif
#ifndef DT_FIFO
  #define DT_FIFO		001
#endif
#ifndef DT_CHR
  #define DT_CHR		002
#endif
#ifndef DT_DIR
  #define DT_DIR		004
#endif
#ifndef DT_BLK
  #define DT_BLK		006
#endif
#ifndef DT_REG
  #define DT_REG		010
#endif
#ifndef DT_LNK
  #define DT_LNK		012
#endif
#ifndef DT_SOCK
  #define DT_SOCK		014
#endif

struct dirent {
	int  d_ino;
	int  d_seekoff;
	int  d_reclen;
	int  d_type;
	int  d_drv;
	int  d_namlen;
	char d_name[SYS_CALL_MAX_PATH+1];
};

int            closedir(DIR_t *dirp);
long           telldir(DIR_t *dirp);
DIR_t         *opendir(const char *path);
struct dirent *readdir(DIR_t *dirp);
void           rewinddir(DIR_t *dirp);
void           seekdir(DIR_t *dirp, long loc);

/* ------------------------------------------------------------------------------------------------ */
/* statfs stuff																						*/

struct statfs {
	int64_t f_bsize;								/* Block size									*/
	int64_t f_blocks;								/* Total data blocks in the file system			*/
	int64_t f_bfree;								/* Number of free blocks in the file system		*/
	int     f_type;									/* Type of file system							*/
	int     f_flags;								/* Mounting flags								*/
	char    f_fstypename[16];						/* type of FS name (Must hold defines below)	*/
	char    f_mntonname[SYS_CALL_MAX_PATH+1];		/* Mount point									*/
	char    f_mntfromname[SYS_CALL_MAX_PATH+1];		/* Device name									*/
};

#define FS_TYPE_AUTO	0
#define FS_TYPE_NAME_AUTO	"AUTO"
#define FS_TYPE_FAT12	1
#define FS_TYPE_NAME_FAT12	"FAT12"
#define FS_TYPE_FAT16	2
#define FS_TYPE_NAME_FAT16	"FAT16"
#define FS_TYPE_FAT32	3
#define FS_TYPE_NAME_FAT32	"FAT32"
#define FS_TYPE_EXFAT	4
#define FS_TYPE_NAME_EXFAT	"exFAT"

extern int statfs(const char *path, struct statfs *buf);
extern int fstatfs(int fs, struct statfs *buf);

/* ------------------------------------------------------------------------------------------------ */
/* CC ARM & IAR CLIB  don't have the Unix system calls prototypes									*/

#if defined(__CC_ARM) || defined(__ICCARM__)    || defined(__ICCAVR32__)

struct	stat {
  dev_t		st_dev;
  ino_t		st_ino;
  mode_t	st_mode;
  nlink_t	st_nlink;
  uid_t		st_uid;
  gid_t		st_gid;
  dev_t		st_rdev;
  off_t		st_size;
  int       st_blksize;
  time_t	st_atime;
  time_t	st_mtime;
  time_t	st_ctime;
};

/* ---------------------------------------------------------------- */
/* Missing errno, set to same values of lwip to not have conflicts	*/

#define EACCES		13
#define EBADF		 9
#define EBUSY		16 
#define EEXIST		17
#define ENFILE		23
#define ENODEV		19
#define ENOENT		 2
#define ENOLCK		37
#define ENOSPC		28
#define ENOTDIR		20
#define ENOTEMPTY	39
#define ENXIO		 6
#define EROFS		30

#if defined(__ICCARM__) || defined(__ICCAVR32__)
  #define EINVAL	22
#endif

int      chdir(const char *path);
int      chmod(const char *path, mode_t mode);
int      chown(const char *path, uid_t owner, gid_t group);
int      close(int fd);
int      closedir(DIR_t *dirp);
int      dup(int fd);
int      fstat(int fd, struct stat *pstat);
int      fstatfs(int fs, struct statfs *buf);
char    *getcwd(char *buf, size_t size);
int      isatty(int fd);
off_t    lseek(int fd, off_t offset, int whence);
int      mkdir(const char *path, mode_t mode);
int      mkfs(const char *type, void *data);
int      mount(const char *type, void *dir, int flags, void *data);
int      open(const char *path, int flags, int mode);
DIR_t   *opendir(const char *path);
_ssize_t read(int fd, void *vbuf, size_t size);
struct dirent *readdir(DIR_t *dirp);
int      rename(const char *old, const char *anew);
void     rewinddir(DIR_t *dirp);
void     seekdir(DIR_t *dirp, long loc);
int      stat(const char *path, struct stat *pstat);
int      statfs(const char *path, struct statfs *buf);
long     telldir(DIR_t *dirp);
mode_t   umask(mode_t mask);
int      unlink(const char *path);
int      unmount(const char *dir, int flags);
_ssize_t write(int fd, const void *vbuf, size_t len);
#if defined(__CC_ARM)
  int _gettimeofday(void *tv, void *tz);
#endif

#endif

/* ------------------------------------------------------------------------------------------------ */

int64_t media_size(int Drv);
int32_t media_blksize(int Drv);
const char *media_info(int Drv);

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
