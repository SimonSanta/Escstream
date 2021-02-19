/* ------------------------------------------------------------------------------------------------ */
/* FILE :		SysCall_noFS.c																		*/
/*																									*/
/* CONTENTS :																						*/
/*				Unix system call layer with no underlying FS										*/
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

/* ------------------------------------------------------------------------------------------------ */

#ifndef SYS_CALL_N_FILE								/* Number of available file descriptors			*/
  #define SYS_CALL_N_FILE 		5					/* Excluding stdin, stdout & stderr				*/
#endif												/* If is set to <= 0, no /dev supported			*/

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
													/* Mutexes only used when /dev in use			*/
#ifndef SYS_CALL_MUTEX								/* -ve: individual descriptors not protected	*/
  #define SYS_CALL_MUTEX		0					/* ==0: descriptors share one mutex				*/
#endif												/* +ve: each descriptor has its own mutex		*/

#ifndef SYS_CALL_OS_MTX
  #define SYS_CALL_OS_MTX		0					/* If the global mutex is G_OSmutex or not		*/
#endif												/* != 0 is G_OSmutex / == 0 is local			*/

/* ------------------------------------------------------------------------------------------------ */
/* Internal definitions																				*/
/* ------------------------------------------------------------------------------------------------ */

#define SYS_CALL_DEV_USED	   (((SYS_CALL_DEV_I2C) != 0)											\
                             || ((SYS_CALL_DEV_SPI) != 0)											\
                             || ((SYS_CALL_DEV_TTY) != 0))

#if (((SYS_CALL_DEV_USED) != 0) && ((SYS_CALL_N_FILE) > 0))
  #define XXX_CALL_DEV_USED		(1)
#else
  #define XXX_CALL_DEV_USED		(0)
#endif

#define DEV_TYPE_I2C		(1)
#define DEV_TYPE_SPI		(2)
#define DEV_TYPE_TTY		(3)

#if ((SYS_CALL_MUTEX) >  0)	
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
/* Local variables																					*/
/* ------------------------------------------------------------------------------------------------ */

#if ((XXX_CALL_DEV_USED) != 0)						/* Only used with /dev							*/
  typedef struct {									/* Our own file descriptors						*/
	int    IsOpen;									/* If the file is open							*/
	int    DevNmb;
	int    DevType;
	int    omode;									/* File open access mode						*/
   #if (((SYS_CALL_TTY_EOF) != 0) && ((SYS_CALL_DEV_TTY) != 0))
	int IsEOF;										/* Flag to remember if EOF has been received	*/
   #endif
  #if ((SYS_CALL_MUTEX) >= 0)						/* Single / individual mutex(es) requested		*/
	MTX_t *MyMtx;									/* When single, all [] hold the same mutex		*/
  #endif
	char  Fname[SYS_CALL_MAX_PATH+1];
  } SysFile_t;

  static MTX_t    *g_GlbMtx;
  static SysFile_t g_SysFile[SYS_CALL_N_FILE];
#endif

extern int G_UartDevIn;								/* Must be supplied by the application because	*/
extern int G_UartDevOut;							/* the UART driver must know the device to use	*/
extern int G_UartDevErr;

/* ------------------------------------------------------------------------------------------------ */
/* These macros simplify the manipulation of the mutexes											*/
/* FIL_ : to protect a file descriptor																*/

#if ((SYS_CALL_MUTEX) <  0)							/* Minimal protection							*/
  #define FIL_MTXlock(x)	0
  #define FIL_MTXunlock(x)	do{int _=0;_=_;}while(0)

#else												/* Descriptors have a mutex (are shared)		*/
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
extern int uart_recv(int Dev, char *Buff, int Len);
extern int uart_send(int Dev, const char *Buff, int Len);

/* ------------------------------------------------------------------------------------------------ */
/* These macros make this file use-able by multiple libraries										*/

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
  int rename(const char *old, const char *anew) {
	return(_rename_r(__getreent(), old, anew));
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
#if ((XXX_CALL_DEV_USED) != 0)						/* Only used with /dev							*/
 #if ((SYS_CALL_MUTEX) >= 0)
  int ii;											/* General purpose								*/
 #endif

  #if ((SYS_CALL_OS_MTX) == 0)						/* The global mutex only required for /dev		*/
	g_GlbMtx = MTXopen("SysCall Glb");				/* Use a local mutex							*/
  #else
	g_GlbMtx = G_OSmutex;							/* Use G_OSmutex								*/
  #endif

	memset(&g_SysFile[0], 0, sizeof(g_SysFile)/sizeof(g_SysFile[0]));

  #if ((SYS_CALL_MUTEX) == 0)
	g_SysFile[0].MyMtx = MTXopen("SysCall Dsc");
	for (ii=1 ; ii<(int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])) ; ii++) {
		g_SysFile[ii].MyMtx = g_SysFile[0].MyMtx;
	}
  #elif ((SYS_CALL_MUTEX) >  0)	
	for (ii=0 ; ii<(int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])) ; ii++) {
		g_SysDir[ii].MyMtx = MTXopen(MTXopen(&g_FdscName[ii][0]);
	}
  #endif
#endif

	return;
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
  #if ((XXX_CALL_DEV_USED) == 0)					/* Only used with /dev							*/
	REENT_ERRNO = ENODEV;
	return(-1);

  #else
  int DevNmb;
  int DevType;
  int Err;
  int fd;

	Err = 0;
	if (0 != MTXlock(g_GlbMtx, -1)) {				/* Although infinte wait, check in case			*/
		Err = EBUSY;								/* deadlock protection is enable				*/
	}
	else {											/* Search a free file descriptor				*/
		for (fd=0 ; fd<(int)(sizeof(g_SysFile)/sizeof(g_SysFile[0])) ; fd++) {	/* Lower # is std	*/
			if (g_SysFile[fd].IsOpen == 0) {		/* as will be playing with .IsOpen				*/
				break;								/* This one is unused, keep processing with it	*/
			}
		}
		if (fd >= (int)(sizeof(g_SysFile)/sizeof(g_SysFile[0]))) {	/* All entries are used			*/
			Err = ENFILE;							/* Maximum # of files reached					*/
		}											/* Return error									*/

		if (Err == 0) {
			DevNmb  = 0;
			DevType = 0;

		   #if ((SYS_CALL_DEV_I2C) != 0)
			if (0 == strncasecmp(path, "/dev/i2c", 8)) {
				if ((!isdigit((int)path[8]))		/* Must be a single digit appended to /dev/i2c	*/
				||  (path[9] != '\0')) {
					Err = ENODEV;
				}
				else {
					 DevType = DEV_TYPE_I2C;
					 DevNmb  = path[8]-'0';
				}
			}
		   #endif

		   #if ((SYS_CALL_DEV_SPI) != 0)
			if (0 == strncasecmp(path, "/dev/spi", 8)) {
				if ((!isdigit((int)path[8]))		/* Must be a dual digit appended to /dev/spi	*/
				||  (!isdigit((int)path[9]))
				||  (path[10] != '\0')) {
					Err = ENODEV;
				}
				else {
					DevType = DEV_TYPE_SPI;
					DevNmb = ((path[8]-'0') * 16)	/* The device # is hex to simplify splitting	*/
					       +  (path[9]-'0');		/* controller number & slave number				*/
				}
			}
		   #endif

		   #if ((SYS_CALL_DEV_TTY) != 0)
			if (0 == strncasecmp(path, "/dev/tty", 8)) {
				if ((!isdigit((int)path[8]))		/* Must be a single digit appended to /dev/i2c	*/
				||  (path[9] != '\0')) {
					Err = ENODEV;
				}
				else {
					DevType = DEV_TYPE_TTY;
					DevNmb  = path[8]-'0';
				}
			}
		   #endif
		}

		if (Err == 0) {
			strcpy(&g_SysFile[fd].Fname[0], path);
			g_SysFile[fd].DevNmb  = DevNmb;
			g_SysFile[fd].DevType = DevType;
			g_SysFile[fd].omode   = mode;
			g_SysFile[fd].IsOpen  = 1;
		  #if (((SYS_CALL_TTY_EOF) != 0) && ((SYS_CALL_DEV_TTY) != 0))
			g_SysFile[fd].IsEOF   = 0;
		  #endif
			fd += 3;								/* Must skip stdin, stdout, stderr				*/
		}

		MTXunlock(g_GlbMtx);
	}

	if (Err != 0) {
		REENT_ERRNO = Err;
		fd          = -1;
	}
	return(fd);

  #endif
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
  #if ((XXX_CALL_DEV_USED) == 0)					/* Only used with /dev							*/
	REENT_ERRNO = ENODEV;
	return(-1);

  #else
	int Err;

	Err = -1;
	fd -=  3;
	if ((fd >= 0)
	&&  (fd < sizeof(g_SysFile)/sizeof(g_SysFile[0]))) {
		if (g_SysFile[fd].IsOpen != 0) {
			g_SysFile[fd].IsOpen = 0;
			Err = 0;
		}
	}

	return(Err);

  #endif
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
int Err;
int Ret;											/* Return value									*/
int UARTdev;										/* When writing to UART, uart device to access	*/
#if ((XXX_CALL_DEV_USED) != 0)
  int DevNmb  = 0;
  int DevType = 0;
#endif

	Err     =  0;
	Ret     =  0;
	UARTdev = -1;

	if (fd == 0) {									/* stdin										*/
		UARTdev = G_UartDevIn;
	}
	else if (fd == 1) {								/* stdout										*/
		UARTdev = G_UartDevOut;
	}
	else if (fd == 2) {								/* stderr										*/
		UARTdev = G_UartDevErr;
	}
	else {											/* No FS, anything else is an error				*/
	  #if ((XXX_CALL_DEV_USED) == 0)				/* Only used with /dev							*/
		Err = ENODEV;
		Ret = -1;
	  #else
		fd -= 3;
		if (fd < sizeof(g_SysFile)/sizeof(g_SysFile[0])) {

			if (0 != FIL_MTXlock(fd)) {				/* Protect the descriptor access				*/
				Err = EBUSY;						/* Although infinte wait, check in case			*/
			}										/* deadlock protection is enable				*/
			else if (g_SysFile[fd].IsOpen == 0) {	/* The file is not open, error					*/
				Err = EBADF;
			}

			if ((Err == 0)
			&&  (0 == (g_SysFile[fd].omode & 0444))) {	/* If the file was not open readable, error	*/
				Err = EBADF;
			}

			if (Err == 0) {							/* File open and read operation authorized		*/
				DevType = g_SysFile[fd].DevType;
				if (DevType != 0) {					/* When != 0 it is a /dev device				*/
					DevNmb = g_SysFile[fd].DevNmb;
					if (DevType == DEV_TYPE_TTY) {
						UARTdev = DevNmb;
					}
					Ret = 0;
				}
			}
			FIL_MTXunlock(fd);
		}
	  #endif
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
/*   - Error																						*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

_ssize_t SC_WRITE_(struct _reent *reent, int fd, const void *vbuf, size_t len)
{
int Err;
int Ret;											/* Return value									*/
int UARTdev;										/* When writing to UART, uart device to access	*/
#if ((XXX_CALL_DEV_USED) != 0)
  int DevNmb  = 0;
  int DevType = 0;
#endif

	Err     =  0;
	Ret     =  0;
	UARTdev = -1;

	if (fd == 0) {									/* stdin										*/
		UARTdev = G_UartDevIn;
	}
	else if (fd == 1) {								/* stdout										*/
		UARTdev = G_UartDevOut;
	}
	else if (fd == 2) {								/* stderr										*/
		UARTdev = G_UartDevErr;
	}
	else {											/* No FS, anything else is an error				*/
	  #if ((XXX_CALL_DEV_USED) == 0)				/* Only used with /dev							*/
		Err = ENODEV;
		Ret = -1;
	  #else
		fd -= 3;
		if (fd < sizeof(g_SysFile)/sizeof(g_SysFile[0])) {

			if (0 != FIL_MTXlock(fd)) {				/* Protect the descriptor access				*/
				Err = EBUSY;						/* Although infinte wait, check in case			*/
			}										/* deadlock protection is enable				*/
			else if (g_SysFile[fd].IsOpen == 0) {	/* The file is not open, error					*/
				Err = EBADF;
			}

			if ((Err == 0)
			&&  (0 == (g_SysFile[fd].omode & 0222))) {	/* If the file was not open writable, error	*/
				Err = EBADF;
			}

			if (Err == 0) {
				DevType = g_SysFile[fd].DevType;
				if (DevType != 0) {					/* When != 0 it is a /dev device				*/
					DevNmb = g_SysFile[fd].DevNmb;
					if (DevType == DEV_TYPE_TTY) {
						UARTdev = DevNmb;
					}
					Ret = 0;
				}
			}

			FIL_MTXunlock(fd);
		}
	  #endif
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
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

off_t SC_LSEEK_(struct _reent *reent, int fd, off_t offset, int whence)
{
	REENT_ERRNO = ENODEV;
	return((off_t)-1);
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

int SC_RENAME_(struct _reent *reent, const char *old, const char *anew)
{
	REENT_ERRNO = ENODEV;
	return(-1);
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

int SC_UNLINK_(struct _reent *reent, const char *path)
{
	REENT_ERRNO = ENODEV;
	return(-1);
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
	REENT_ERRNO_ = ENODEV;
	return(-1);
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
	REENT_ERRNO_ = ENODEV;
	return(NULL);
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
	REENT_ERRNO_ = ENODEV;
	return(-1);
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
	REENT_ERRNO = ENODEV;
	return(-1);
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
	REENT_ERRNO = ENODEV;
	return(-1);
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
	REENT_ERRNO = ENODEV;
	return(-1);
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
	REENT_ERRNO_ = ENODEV;
	return(-1);
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
	REENT_ERRNO_ = ENODEV;
	return(-1);
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
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int chown(const char *path, uid_t owner, gid_t group)
{
	return(0);
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
	REENT_ERRNO_ = ENODEV;
	return(0);
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
	REENT_ERRNO_ = ENODEV;
	return(-1);
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
/*		data  : for type == "FAT12", "n:" where n is a digit from 0 to _VOLUMES - 1					*/
/*		        for type == "FAT16", "n:" where n is a digit from 0 to _VOLUMES - 1					*/
/*		        for type == "FAT32", "n:" where n is a digit from 0 to _VOLUMES - 1					*/
/*		        for type == "exFAT", "n:" where n is a digit from 0 to _VOLUMES - 1					*/
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
	REENT_ERRNO_ = ENODEV;
	return(-1);
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
	REENT_ERRNO_ = ENODEV;
	return(-1);
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
/*		data  : for type == "FAT12", "n:" where n is a digit from 0 to _VOLUMES - 1					*/
/*		        for type == "FAT16", "n:" where n is a digit from 0 to _VOLUMES - 1					*/
/*		        for type == "FAT32", "n:" where n is a digit from 0 to _VOLUMES - 1					*/
/*		        for type == "exFAT", "n:" where n is a digit from 0 to _VOLUMES - 1					*/
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
	REENT_ERRNO_ = EBADF;
	return(-1);
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
	REENT_ERRNO_ = EBADF;
	return(NULL);	
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
	REENT_ERRNO_ = EBADF;
	return(-1);	
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
	REENT_ERRNO_ = EBADF;
	return(NULL);
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
	REENT_ERRNO_ = EBADF;
	return((long)-1);
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

int devctl(int fd, const int *cfg)
{
#if ((XXX_CALL_DEV_USED) != 0)
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

	  #if ((XXX_CALL_DEV_USED) != 0)
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
	return(-1);
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
	return((long)-1);
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

int IARflush(int fd)
{
	return((fd<3) ? 0 : -1);						/* When fd -ve return OK (is a flush all)		*/
}													/* Always OK with stdio							*/

#endif

/* EOF */
