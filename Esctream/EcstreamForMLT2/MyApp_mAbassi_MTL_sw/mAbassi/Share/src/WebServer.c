/* ------------------------------------------------------------------------------------------------ */
/* FILE :		WebServer.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Code for the generic Webserver.														*/
/*				This file contains all the web services management.									*/
/*																									*/
/*																									*/
/* Copyright (c) 2013-2019, Code-Time Technologies Inc. All rights reserved.						*/
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
/*	$Revision: 1.26 $																				*/
/*	$Date: 2019/01/10 18:07:16 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "WebServer.h"

#ifndef WEBS_FILESYS
  #define WEBS_FILESYS				WEBS_FILESYS_NONE
#endif

#if ((WEBS_FILESYS) == WEBS_FILESYS_SYSCALL)
  #include "SysCall.h"
#elif ((WEBS_FILESYS) == WEBS_FILESYS_FATFS)
  #include "ff.h"
#elif ((WEBS_FILESYS) == WEBS_FILESYS_FULLFAT)
  #include "ff_config.h"
#elif ((WEBS_FILESYS) == WEBS_FILESYS_UEFAT)
  #include "fat_opts.h"
#endif

/* ------------------------------------------------------------------------------------------------ */

#ifndef WEBS_DEBUG
  #define WEBS_DEBUG				0				/* Set to non-zero to print debug messages		*/
#endif

#ifndef WEBS_MAX_CGI
  #define WEBS_MAX_CGI				-1				/* Maximum number of different CGIs handlers	*/
#endif												/* <= 0 uses dynamic memory allocation			*/

#ifndef WEBS_MAX_POST
  #define WEBS_MAX_POST				-1				/* Maximum number of different POSTs handlers	*/
#endif												/* <= 0 uses dynamic memory allocation			*/

#ifndef WEBS_MAX_CGI_POST_PARAM
  #define WEBS_MAX_CGI_POST_PARAM	-1				/* Maximum number of Param=Value in a request	*/
#endif												/* <= 0 uses dynamic memory allocation			*/

#ifndef WEBS_MAX_SSI
  #define WEBS_MAX_SSI				-1				/* Maximum number of SSI update functions		*/
#endif												/* <= 0 uses dynamic memory allocation			*/

#ifndef WEBS_MAX_VARIABLE
  #define WEBS_MAX_VARIABLE			-1				/* Maximum number of server variables for SSI	*/
#endif												/* <= 0 uses dynamic memory allocation			*/

#if ((WEBS_MAX_VARIABLE) > 0)						/* When static variables are use, needs to size	*/
  #ifndef WEBS_VAR_NAME_SIZE						/* the char strigs hoding the variable name and	*/
	#define WEBS_VAR_NAME_SIZE		64				/* the value associated to the variable			*/
  #endif
  #ifndef WEBS_VAR_VAL_SIZE
	#define WEBS_VAR_VAL_SIZE		64
  #endif
#endif

/* ------------------------------------------------------------------------------------------------ */

#if ((OX_LIB_REENT_PROTECT) <= 0)					/* If library not protected against reentrance	*/
 #ifndef osCMSIS									/* Mutex to protect stdio mutex operations		*/
  #define WEBS_LOCK_ALLOC()		MTXlock(G_OSmutex, -1)
  #define WEBS_UNLOCK_ALLOC()	MTXunlock(G_OSmutex)
  #define WEBS_LOCK_STDIO()		MTXlock(G_OSmutex, -1)
  #define WEBS_UNLOCK_STDIO()	MTXunlock(G_OSmutex)
 #else
  #define WEBS_LOCK_ALLOC()		osMutexWait(G_OSmutex, osWaitForever)
  #define WEBS_UNLOCK_ALLOC()	osMutexRelease(G_OSmutex)
  #define WEBS_LOCK_STDIO()		osMutexWait(G_OSmutex, osWaitForever)
  #define WEBS_UNLOCK_STDIO()	osMutexRelease(G_OSmutex)
 #endif
#else												/* Lib is safe, no need for extra protection	*/
  #define WEBS_LOCK_ALLOC()		do {int _=0;_=_;} while(0)
  #define WEBS_UNLOCK_ALLOC()	do {int _=0;_=_;} while(0)
  #define WEBS_LOCK_STDIO()		do {int _=0;_=_;} while(0)
  #define WEBS_UNLOCK_STDIO()	do {int _=0;_=_;} while(0)
#endif

/* ------------------------------------------------------------------------------------------------ */

static int         CGIparse(char *String);
static int         ExecVar(const char *Name, char *BufOut, int SizeOut);
static int         SSIevalExpr(const char *Ptr);
static const char *SSIgetRHS(const char *Ptr, const char *LHS, int *Len);

/* ------------------------------------------------------------------------------------------------ */

#if ((WEBS_MAX_VARIABLE) > 0)
  struct LocalVar {									/* Local variable information holding			*/
	char VarName[WEBS_VAR_NAME_SIZE];				/* Name of the variable							*/
	char Value[WEBS_VAR_VAL_SIZE];					/* Value of the variable: always text			*/
	char *(*Command)(char *);    					/* Command associated when is an exec variable	*/
  };
#else
  struct LocalVar {									/* Local variable information holding			*/
	char *VarName;									/* Name of the variable							*/
	char *Value;									/* Value of the variable: always text			*/
	char *(*Command)(char *, int);					/* Command associated when is an exec variable	*/
  };
#endif

#if ((WEBS_MAX_CGI_POST_PARAM) > 0)
  char  *g_CGIparam[WEBS_MAX_CGI_POST_PARAM];		/* Parameters extracted from the request URI	*/
  char  *g_CGIvalue[WEBS_MAX_CGI_POST_PARAM];		/* Values for each of the extracted param		*/
#else
  char  **g_CGIparam;								/* Parameters extracted from the request URI	*/
  char  **g_CGIvalue;								/* Values for each of the extracted param		*/
  int    g_nCGIValPar;
#endif

typedef struct {
    const char  *Name;
    CGIhandler_t Handler;
} CGI_t;

#if ((WEBS_MAX_CGI) > 0)
  CGI_t  g_CGIspec[WEBS_MAX_CGI];					/* Specifications on the attached CGIs			*/
#else
  CGI_t  *g_CGIspec;								/* Specifications on the attached CGIs			*/
#endif
int    g_nCGIspec;									/* Number of CGIs attached						*/

#if ((WEBS_MAX_POST) > 0)
  CGI_t  g_POSTspec[WEBS_MAX_POST];					/* Specifications on the attached POSTs			*/
#else
  CGI_t  *g_POSTspec;								/* Specifications on the attached POSTs			*/
#endif
int    g_nPOSTspec;									/* Number of POSTs attached						*/

#if ((WEBS_MAX_SSI) > 0)
  void  (*g_SSIupdateFct[WEBS_MAX_SSI])(void);
#else
  void  (*(*g_SSIupdateFct))(void);
#endif
int    g_nSSIupd;									/* Number of update functons attached			*/

#if ((WEBS_MAX_VARIABLE) > 0)
  struct LocalVar g_MyVars[WEBS_MAX_VARIABLE];
#else
  struct LocalVar *g_MyVars;
  int              g_nMyVars;
#endif

char   g_UpTime[10];								/* UP_TIME variable								*/

/* ------------------------------------------------------------------------------------------------ */

void WebServerInit(void)
{
#if ((WEBS_MAX_VARIABLE) > 0)
  int ii;											/* General purpose								*/
#endif

	g_nCGIspec  = 0;								/* Number of CGIs handlers						*/
	g_nPOSTspec = 0;								/* Number of POSTs handlers						*/
	g_nSSIupd   = 0; 								/* Number of SSI updaters						*/

  #if ((WEBS_MAX_CGI_POST_PARAM) <= 0)
	g_CGIparam = NULL;
	g_CGIvalue = NULL;
	g_nCGIValPar = 0;
  #endif
  #if ((WEBS_MAX_CGI) <= 0)
	g_CGIspec = NULL;
  #endif
  #if ((WEBS_MAX_POST) <= 0)
	g_POSTspec = NULL;
  #endif
  #if ((WEBS_MAX_VARIABLE) > 0)
	for (ii=0 ; ii<WEBS_MAX_VARIABLE ; ii++) {
		g_MyVars[ii].VarName[0] = '\0';
	}
  #else
	g_MyVars  = NULL;
	g_nMyVars = 0;
  #endif

	SSInewVar("UP_TIME", "00:00:00", NULL);
													/* The following fct are supplied by the app	*/
	CGIinit();										/* Init the application CGIs					*/
	POSTinit();										/* Init the application POST verb handlers		*/
	SSIinit();										/* Init the application SSIs					*/

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Expand the SSIs																					*/
/*																									*/
/* - for the whole input page																		*/
/*   - find next "<!--#																				*/
/*   - memo start of SSI																			*/
/*   - copy input text from last end of SSI to start of SSI to output buffer						*/
/*   - find next "-->"																				*/
/*   - memo end of SSI																				*/
/*   - copy SSI text into output buffer																*/
/*   - process SSI command:																			*/
/*     #block       (nothing done)																	*/
/*     #config      (nothing done)																	*/
/*     #cookie      (nothing done)																	*/
/*     #cpt         (nothing done)																	*/
/*     #echo																						*/
/*     #endblock    (nothing done)																	*/
/*     #endloop     (nothing done)																	*/
/*     #exec        (only handle the non-standard #exec cmd="$var")									*/
/*     #exitloop    (nothing done)																	*/
/*     #flastmod    (nothing done)																	*/
/*     #fsize       (nothing done)																	*/
/*     #hitcount    (nothing done)																	*/
/*     #include																						*/
/*     #jdbc        (nothing done)																	*/
/*     #loop        (nothing done)																	*/
/*     #printenv    (nothing done)																	*/
/*     #servlet     (nothing done)																	*/
/*     #set         (nothing done)																	*/
/*     #if																							*/
/*     #elif																						*/
/*     #else																						*/
/*     #endif																						*/
/* ------------------------------------------------------------------------------------------------ */

int SSIexpand(char *Buffer, int Bsize, const char *BufIn)
{
const char     *Cptr;								/* General purpose char pointer					*/
const char     *CptrIn;								/* Analysis pointer in the input buffer			*/
int             ii;									/* General purpose								*/
int             Len;								/* Length of string to analyse,extract or copy	*/
int             IdxNow;								/* To track how many char uses in Buffer		*/
const char     *SSIend;								/* Pointer to the end of the SSI being handle	*/
const char     *SSIstart;							/* Pointer to the start of the SSI being handle	*/
uint32_t        TrueCond;
uint32_t        TrueNest;
uint32_t        TrueWas;
#if  ((WEBS_FILESYS) == WEBS_FILESYS_SYSCALL)
  int Fsize;										/* Number of bytes read from a file				*/
  int fd;
#elif ((WEBS_FILESYS) == WEBS_FILESYS_FATFS)
  int Fsize;										/* Number of bytes read from a file				*/
  FILE_DSC_t FdscSSI;
#endif
#if ((WEBS_DEBUG) != 0)
  static const char MsgFull[] = "WEBS  - Error - Dst buffer for SSI expansion too small, aborting";
#endif

	Bsize--;										/* -1 to take into account final '\0'			*/
	Buffer[0] = '\0';
	CptrIn    = BufIn;
	IdxNow    = 0;
	TrueCond  = 1U;									/* IF / ELIF / ELSE true or false condition		*/
	TrueNest  = 1U;
	TrueWas   = 0U;									/* False==0, True==1, Invalid==-1; 				*/
	if (CptrIn != (char *)NULL) {					/* Should never happen... plain safety			*/
		SSIend = CptrIn;
		do {

/* ------------------------------------------------ */
/* FIND AND ISOLATE THE NEXT SSI					*/

			CptrIn = strstr(CptrIn, "<!--#");		/* Find the beginning of the next SSI			*/
			if (CptrIn != (char *)NULL) {			/* Got one										*/
				SSIstart = CptrIn;					/* Memo the pointer of the start of this SSI	*/
				CptrIn  += 5;						/* Skip the SSI preamble						*/

				Len = ((int)(SSIstart-SSIend));		/* Copy everything since the end of the last	*/
				if (TrueCond & TrueNest) {			/* SSI. Is only done if the condition is true	*/
					if ((IdxNow+Len) < Bsize) {		/* and the condition is valid					*/
						memmove(&Buffer[IdxNow], SSIend, Len);
						IdxNow += Len;				/* Condition would be invalid e.g. if unknown	*/
					}								/* variable was used							*/
					else {
					  #if ((WEBS_DEBUG) != 0)
						puts(MsgFull);
					  #endif
						Buffer[IdxNow] = '\0';
						return(IdxNow);
					}
				}

				Cptr = strstr(CptrIn, "-->");		/* Find the end of this SSI						*/
				if (Cptr != (char *)NULL) {			/* Got it										*/
					SSIend = Cptr+3;				/* Memo the pointer to the end of the SSI		*/
				}

				Len = ((int)(SSIend-SSIstart));		/* Copy the SSI as is to the output buffer		*/
				if ((IdxNow+Len) < Bsize) {
					memmove(&Buffer[IdxNow], SSIstart, Len);
					IdxNow += Len;
				}
				else {
				  #if ((WEBS_DEBUG) != 0)
					puts(MsgFull);
				  #endif
					Buffer[IdxNow] = '\0';
					return(IdxNow);
				}

/* ------------------------------------------------ */
/* SSI PARSING & SUBSTITUTION						*/
													/* -------------------------------------------- */
													/* #block directive								*/
				if ((0 == strncmp(CptrIn, "block", 5))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/
				}
													/* -------------------------------------------- */
													/* #config directive							*/
				else if ((0 == strncmp(CptrIn, "config", 6))
				&&  (TrueCond & TrueNest)) {
					Cptr = SSIgetRHS(CptrIn, "timefmt=", &Len);
					if (Cptr != (char *)NULL) {
					}
					Cptr = SSIgetRHS(CptrIn, "sizefmt=", &Len);
					if (Cptr != (char *)NULL) {
					}
					Cptr = SSIgetRHS(CptrIn, "errmsg=", &Len);
					if (Cptr != (char *)NULL) {
					}
				}
													/* -------------------------------------------- */
													/* #cookie directive							*/
				else if ((0 == strncmp(CptrIn, "cookie", 6))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/
				}
													/* -------------------------------------------- */
													/* #cpt directive								*/
				else if ((0 == strncmp(CptrIn, "cpt", 3))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/
				}
													/* -------------------------------------------- */
													/* #echo directive								*/
				else if ((0 == strncmp(CptrIn, "echo", 4))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/
					Cptr = SSIgetRHS(CptrIn, "var=", &Len);
					if ((IdxNow+Len) >= Bsize) {
					  #if ((WEBS_DEBUG) != 0)
						puts(MsgFull);
					  #endif
						Buffer[IdxNow] = '\0';
						return(IdxNow);
					}
					if (Cptr != (char *)NULL) {
						memmove(&Buffer[IdxNow], Cptr, Len);/* Use the output buffer as temporary	*/
						Buffer[IdxNow+Len] = '\0';
						Cptr = SSIgetVar(&Buffer[IdxNow]);
						if (Cptr != (char *)NULL) {
							Len = strlen(Cptr);
							if ((IdxNow+Len) < Bsize) {
								memmove(&Buffer[IdxNow], Cptr, Len);
								IdxNow += Len;
							}
							else {
							  #if ((WEBS_DEBUG) != 0)
								puts(MsgFull);
							  #endif
								Buffer[IdxNow] = '\0';
								return(IdxNow);
							}
						}
					}
				}
													/* -------------------------------------------- */
													/* #endblock directive							*/
				else if ((0 == strncmp(CptrIn, "endblock", 8))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/
				}
													/* -------------------------------------------- */
													/* #endloop directive							*/
				else if ((0 == strncmp(CptrIn, "endloop", 7))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/
				}
													/* -------------------------------------------- */
													/* #exec directive								*/
				else if ((0 == strncmp(CptrIn, "exec", 4))
				&&      (TrueCond & TrueNest)) {
					Cptr = SSIgetRHS(CptrIn, "cgi=", &Len);
					if (Cptr == (char *)NULL) {
						Cptr = SSIgetRHS(CptrIn, "cmd=", &Len);
					}
					if ((IdxNow+Len) > Bsize) {
					  #if ((WEBS_DEBUG) != 0)
						puts(MsgFull);
					  #endif
						Buffer[IdxNow] = '\0';
						return(IdxNow);
					}
					if (Cptr != (char *)NULL) {
						memmove(&Buffer[IdxNow], Cptr, Len);/* Use the output buffer as temporary	*/
						Buffer[IdxNow+Len] = '\0';
						if (Cptr[0] == '$') {		/* **** NON-STANDARD **** : execute a variable	*/
							IdxNow += ExecVar(&Buffer[IdxNow], &Buffer[IdxNow], Bsize-IdxNow);
						}							/* for dynamic page creation	*/
					}		
				}
													/* -------------------------------------------- */
													/* #exitloop directive							*/
				else if ((0 == strncmp(CptrIn, "exitloop", 8))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/
				}
													/* -------------------------------------------- */
													/* #flastmod directive							*/
				else if ((0 == strncmp(CptrIn, "flastmod", 8))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/
					Cptr = SSIgetRHS(CptrIn, "virtual=", &Len);
					if (Cptr != (char *)NULL) {
					}
					Cptr = SSIgetRHS(CptrIn, "file=", &Len);
					if (Cptr != (char *)NULL) {
					}
				}
													/* -------------------------------------------- */
													/* #fsize directive								*/
				else if ((0 == strncmp(CptrIn, "fsize", 5))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/
					Cptr = SSIgetRHS(CptrIn, "virtual=", &Len);
					if (Cptr != (char *)NULL) {
					}
					Cptr = SSIgetRHS(CptrIn, "file=", &Len);
					if (Cptr != (char *)NULL) {
					}
				}
													/* -------------------------------------------- */
													/* #count directive								*/
				else if ((0 == strncmp(CptrIn, "hitcount", 8))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/
				}
													/* -------------------------------------------- */
													/* #include directive							*/
				else if ((0 == strncmp(CptrIn, "include", 7))
				&&      (TrueCond & TrueNest)) {
					Cptr = SSIgetRHS(CptrIn, "virtual=", &Len);
					if (Cptr == (char *)NULL) {
						Cptr = SSIgetRHS(CptrIn, "file=", &Len);
					}
					if (Cptr != (char *)NULL) {
						if ((IdxNow+Len) > Bsize) {
						  #if ((WEBS_DEBUG) != 0)
							puts(MsgFull);
						  #endif
							Buffer[IdxNow] = '\0';
							return(IdxNow);
						}
						memmove(&Buffer[IdxNow], Cptr, Len);/* Use the output buffer as temporary	*/
						IdxNow += Len;
						Buffer[IdxNow] = '\0';
					  #if ((WEBS_FILESYS) == WEBS_FILESYS_SYSCALL)
						fd = fopen(&Buffer[IdxNow], "r");
						if (fd >= 0) {
							do {					/* Copy the file contents in g_PageInBuf		*/
								if ((IdxNow+256) < Bsize) {
									Fsize = fread(&Buffer[IdxNow], 1, 256 ,fd)
									IdxNow += Fsize;
								}
								else {
								  #if ((WEBS_DEBUG) != 0)
									puts(MsgFull);
								  #endif
									fclose(fd);
									Buffer[IdxNow] = '\0';
									return(IdxNow);
								}
							} while (Fsize != 0);
						}
					  #elif ((WEBS_FILESYS) == WEBS_FILESYS_FATFS)
						if (FR_OK == f_open(FdscSSI, &Buffer[IdxNow], FA_READ))
							do {					/* Copy the file contents in g_PageInBuf		*/
								if ((IdxNow+256) < Bsize) {
									F_READ(FdscSSI, &Buffer[IdxNow], 256, &Fsize);
									f_read(FdscSSI, &Buffer[IdxNow], 256 ,(UINT *)(&Fsize))
									IdxNow += Fsize;
								}
								else {
								  #if ((WEBS_DEBUG) != 0)
									puts(MsgFull);
								  #endif
									f_close(FdscSSI);
									Buffer[IdxNow] = '\0';
									return(IdxNow);
								}
							} while (Fsize != 0);
							f_close(FdscSSI);
						}
					  #elif ((WEBS_FILESYS) == WEBS_FILESYS_FULLFAT)

					  #elif ((WEBS_FILESYS) == WEBS_FILESYS_UEFAT)

					  #endif
					}
				}
													/* -------------------------------------------- */
													/* #jdbc directive								*/
				else if ((0 == strncmp(CptrIn, "jdbc", 4))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/
				}
													/* -------------------------------------------- */
													/* #loop directive								*/
				else if ((0 == strncmp(CptrIn, "loop", 4))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/
				}
													/* -------------------------------------------- */
													/* #printenv directive							*/
				else if ((0 == strncmp(CptrIn, "printenv", 8))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/

				}
													/* -------------------------------------------- */
													/* #servlet directive							*/
				else if ((0 == strncmp(CptrIn, "servlet", 7))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true & valid	*/
				}
													/* -------------------------------------------- */
													/* #set directive								*/
				else if ((0 == strncmp(CptrIn, "set", 3))
				&&       (TrueCond & TrueNest)) {	/* Done only if the condition is true& valid	*/

				}
													/* -------------------------------------------- */
													/* #if directive								*/
				else if (0 == strncmp(CptrIn, "if", 2)) {
					if (TrueCond & TrueNest) {		/* When nesting must be in a true condition		*/
						TrueNest <<= 1;				/* Next nesting level							*/
						ii = SSIevalExpr(CptrIn+2);	/* Set current condition						*/
						if (ii < 0) {				/* Error, set everything false					*/
							TrueCond &= ~TrueNest;	/* This makes all elif / else also false		*/
							TrueWas  |=  TrueNest;
						}
						else if (ii != 0) {			/* Is True										*/
							TrueCond |=  TrueNest;
							TrueWas  |=  TrueNest;
						}
						else {						/* Is False										*/
							TrueCond &= ~TrueNest;
							TrueWas  &= ~TrueNest;
						}
					}
					else {							/* Previous nesting level is false				*/
						TrueNest <<= 1;				/* This makes all elif / else also false		*/
						TrueCond &= ~TrueNest;
						TrueWas  |=  TrueNest;
					}
				}
													/* -------------------------------------------- */
													/* #elif directive								*/
				else if (0 == strncmp(CptrIn, "elif", 4)) {
					TrueCond &= ~TrueNest;			/* Assume the condition is false				*/
					if ((TrueWas & TrueNest) == 0U) {/* If didn't have a true condition yet, check	*/
						ii = SSIevalExpr(CptrIn+4);
						if (ii < 0) {				/* Error, set everything false					*/
							TrueCond &= ~TrueNest;	/* This makes all elif / else also false		*/
							TrueWas  |= TrueNest;
						}
						else if (ii != 0) {			/* Condition True								*/
							TrueCond |= TrueNest;
							TrueWas  |= TrueNest;
						}
						else {						/* Condition False								*/
							TrueCond &= ~TrueNest;
						}
					}
				}
													/* -------------------------------------------- */
													/* #else directive								*/
				else if (0 == strncmp(CptrIn, "else", 4)) {
					if ((TrueWas & TrueNest) == 0U) {	/* Else is true if there was never a true	*/
						TrueCond |= TrueNest;		/* and all previous conditions were valid		*/
					}
					else {							/* Else is false if there was a true condition	*/
						TrueCond &= ~TrueNest;
					}
				}
													/* -------------------------------------------- */
													/* #endif directive								*/
				else if (0 == strncmp(CptrIn, "endif", 5)) {
					TrueNest >>= 1;					/* Down a nesting level							*/
				}
				CptrIn = SSIend;					/* We will now read past the SSI (after "-->")	*/
			}
		} while (CptrIn != (char *)NULL);			/* Loop as long as we find SSIs					*/

		Len = strlen(SSIend);
		if ((IdxNow+Len) < Bsize) {
			strcpy(&Buffer[IdxNow], SSIend);		/* Insert whatever is after the last SSI		*/
			IdxNow += Len;							/* And adjust the out pointer to return length	*/
		}
		else {
		  #if ((WEBS_DEBUG) != 0)
			puts(MsgFull);
		  #endif
			Buffer[IdxNow] = '\0';
			return(IdxNow);
		}
	}

	Buffer[IdxNow] = '\0';

	return(IdxNow);
}

/* ------------------------------------------------------------------------------------------------ */
/* Handle the CGIs																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

char *CGIprocess(char *Buf)
{
int   Cnt;
int   Found;										/* If has found a matching file name			*/
int   ii;											/* General purpose								*/
char *Param;
char *Space;
char *Ret;
char *URI;

	URI = (char *)NULL;								/* In case no associated handler(s) to the CGIs	*/
	if (g_nCGIspec != 0) {							/* Do nothing if no CGI have been set-up		*/
		Space = strchr(Buf, ' ');					/* Find the space after the CGI					*/
		if (Space != (char *)NULL) {
			*Space = '\0';							/* This terminates the CGI string				*/
		}
		Param = strchr(Buf, '?');					/* Isolate the base URI without the parameters	*/
		if (Param != (char *)NULL) {				/* URI has parameters, terminate the base URI	*/
			*Param = '\0';
			 Param++;
		}

		Cnt   = 0;
		Found = 0;
		for (ii=0; ii<g_nCGIspec; ii++) {			/* Search all CGIs that match the URI base		*/
			if (strcmp(Buf, g_CGIspec[ii].Name) == 0) {
				if (Found == 0) {					/* Found but will keep looping in case multiple	*/
					Cnt   = CGIparse(Param);		/* handlers use the same URI base				*/
					Found = 1;						/* Tag to not re-parse again					*/
				}
				Ret = (char *)g_CGIspec[ii].Handler(Cnt, &g_CGIparam[0], &g_CGIvalue[0]);
				if (Ret != (char *)NULL) {			/* When non-null, matching URI base & index		*/
					URI = Ret;						/* Matching URI									*/
					break;
				}
			}
		}

		if (URI == (char *)NULL) {					/* If no matching CGI, bring back original URI	*/
			if (Param != (char *)NULL) {
				 Param--;
				*Param = '?';
			}
			if (Space != (char *)NULL) {
				*Space = ' ';
			}
		}
	}

	return(URI);
}

/* ------------------------------------------------------------------------------------------------ */
/* Handle a POST request																			*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
char *POSTprocess(char *URI, char *Data)
{
int   Cnt;
char *Cptr;
int   Found;										/* If has found a matching file name			*/
int   ii;											/* General purpose								*/
int   Len;
char *Ret;
char *URIret;

	URIret = (char *)NULL;
	if (g_nPOSTspec != 0) {							/* Do nothing if no CGI have been set-up		*/
		Cptr = strstr(Data, "Content-Length:");		/* Find where data length is located			*/
		if (Cptr != NULL) {							/* Got it, so get the numerical value and		*/
			Cptr += strlen("Content-Length:");		/* go to the the beginning of the data			*/
			Len = (int)strtol(Cptr, NULL, 10);
			while (*Cptr++ != '\n');				/* Go to end of the "Content-Length nnn" line	*/
			ii = 0;
			do {
				if (*Cptr == '\0') {
					break;
				}

				if ((*Cptr != ' ')					/* Is a non-white character						*/
				&&  (*Cptr != '\t')
				&&  (*Cptr != '\r')
				&&  (*Cptr != '\n')) {
					ii = 1;							/* Tag is not an empty line						*/
				}
				if ((*Cptr == '\n')					/* Reached end of line							*/
				&&  (ii != 0)) {					/* And the line is not empty					*/
					*Cptr = ' ';					/* Remove the end of line						*/
					ii = 0;							/* Start again looking for an empty line		*/
				}
			} while(*Cptr++ != '\n');
			while ((*Cptr == '\n')
			||     (*Cptr == '\r')
			||     (*Cptr == '\t')
			||     (*Cptr == ' ')) {
				Cptr++;
			}
			Cptr[Len] = '\0';						/* Terminate the post definitions				*/
			Cnt       = 0;
			Found     = 0;
			for (ii=0; ii<g_nPOSTspec; ii++) {		/* Search all CGIs that match the URI base		*/
				if (strcmp(URI, g_POSTspec[ii].Name) == 0) {
					if (Found == 0) {				/* Found but will keep looping in case multiple	*/
						Cnt   = CGIparse(Cptr);		/* handlers use the same URI base				*/
						Found = 1;					/* Tag to not re-parse again					*/
					}
					Ret = (char *)g_POSTspec[ii].Handler(Cnt, &g_CGIparam[0], &g_CGIvalue[0]);
					if (Ret != (char *)NULL) {		/* When non-null, matching URI base & index		*/
						URIret = Ret;				/* Matching URI 								*/
						break;
					}
				}
			}
		}
	}

	return(URIret);
}

/* ------------------------------------------------------------------------------------------------ */
/* Parse the CGIs																					*/
/*																									*/
/* Extract the parameters & values																	*/
/* Simple parsing isolating Param and Value from:													*/
/* Param=Value&Param=Value&Param=Value																*/
/* The extracted info is deposited in the local globals g_CGIparam[] and g_CGIvalue[]				*/
/* ------------------------------------------------------------------------------------------------ */

static int CGIparse(char *String)
{
char *Equals;
char *Cdst;											/* Destination of the char copy					*/
char  Chold;
char *Csrc;											/* Source of the char copy						*/
int   ii;
int   Loop;


	Loop = 0;
	if ((String != (char *)NULL)					/* No parameters at all, BYE					*/
	&&  (String[0] != '\0')) {
	  #if ((WEBS_MAX_CGI_POST_PARAM) > 0)
		for (Loop=0; (Loop<WEBS_MAX_CGI_POST_PARAM) && (String!=NULL); Loop++) {
	  #else
		for (Loop=0; (String!=NULL); Loop++) {
			if (Loop >= g_nCGIValPar) {
				g_nCGIValPar++;
				WEBS_LOCK_ALLOC();
				g_CGIparam = realloc(g_CGIparam, g_nCGIValPar * sizeof(*g_CGIparam));
				g_CGIvalue = realloc(g_CGIvalue, g_nCGIValPar * sizeof(*g_CGIvalue));
				WEBS_UNLOCK_ALLOC();
				if ((g_CGIparam == NULL)
				||  (g_CGIvalue == NULL)) {
				  #if ((WEBS_DEBUG) != 0)
					WEBS_LOCK_STDIO();
					printf("WEBS  - Error - Out of memory for processing CGI/SSI param=value\n");
					WEBS_UNLOCK_STDIO();
				  #endif
					Loop = 0;
					break;
				}
			}
	  #endif
			g_CGIparam[Loop] = String;				/* Save the name of the parameter				*/
			Equals           = String;				/* Remember the start of this name=value String	*/
			String           = strchr(String, '&');	/* Find start of next name=val and split them	*/
			if (String != (char *)NULL) {
				*String = '\0';
				 String++;
			}
			else {									/* No parameter found							*/
				String = strchr(Equals, ' ');
				if (String != (char *)NULL) {
					*String = '\0';
				}
				String = (char *)NULL;				/* Force an exit of the Loop					*/
			}
			Equals = strchr(Equals, '=');			/* Find '=' in previous pair					*/
			if(Equals != (char *)NULL) {
				*Equals              = '\0';
				g_CGIvalue[Loop] = Equals + 1;
			}
			else {
				g_CGIvalue[Loop] = (char *)NULL;
			}
			if (g_CGIparam[Loop] != NULL) {
			}
		}

		for (ii=0 ; ii<2*Loop ; ii++) {				/* Replace the %nn by the real character		*/
			Csrc = (ii & 1)							/* Process the parameter or the the value		*/
			     ? g_CGIparam[ii/2]
			     : g_CGIvalue[ii/2];
			if (Csrc != (char *)NULL) {				/* Valid parameter / value						*/
				Cdst = Csrc;						/* Star at the beginning						*/
				while(*Csrc != '\0') {
					if (*Csrc == '+') {				/* Found a '+', replace by a space				*/
						*Csrc = ' ';
					}
					else if (*Csrc == '%') {		/* Found a '%'									*/
						Chold   = Csrc[3];
						Csrc[3] = '\0';
						Csrc[2] = (char)strtol(&Csrc[1], NULL, 16);
						Csrc[3] = Chold;
						Csrc   += 2;
					}
					
					*Cdst++ = *Csrc++;
				}
				*Cdst = '\0';
			Csrc = (ii & 1)							/* Process the parameter or the the value		*/
			     ? g_CGIparam[ii/2]
			     : g_CGIvalue[ii/2];
			}
		}
	}

	return(Loop);
}

/* ------------------------------------------------------------------------------------------------ */
/* Attach a CGI handler																				*/
/*																									*/
/* Memo in g_CGIspec[] the CGI name & function handler for that CGI									*/
/* ------------------------------------------------------------------------------------------------ */

int CGInewHandler(const char *Fname, const CGIhandler_t Handler)
{
int RetVal;

  #if ((WEBS_MAX_CGI) <= 0)
	WEBS_LOCK_ALLOC();
	g_CGIspec = realloc(g_CGIspec, (g_nCGIspec+1)*sizeof(*g_CGIspec));
	WEBS_UNLOCK_ALLOC();
	if (g_CGIspec != NULL) {
  #else
	if (g_nCGIspec < WEBS_MAX_CGI) {				/* Room left, memo the CGI specifications		*/
  #endif
		g_CGIspec[g_nCGIspec].Name    = Fname;
		g_CGIspec[g_nCGIspec].Handler = Handler;
		RetVal = g_nCGIspec;
		g_nCGIspec++;								/* One more CGI									*/
	}
	else {											/* No more room to memo the CGIs				*/
	  #if ((WEBS_DEBUG) != 0)
		WEBS_LOCK_STDIO();
		puts("WEBS  - Error - Too many CGI attached");
		WEBS_UNLOCK_STDIO();
	  #endif
		RetVal = -1;
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Attach a POST handler																			*/
/*																									*/
/* Memo in g_POSTspec[] the POST name & function handler for that POST verb							*/
/* ------------------------------------------------------------------------------------------------ */

int POSTnewHandler(const char *Fname, const CGIhandler_t Handler)
{
int RetVal;

  #if ((WEBS_MAX_POST) <= 0)
	WEBS_LOCK_ALLOC();
	g_POSTspec = realloc(g_POSTspec, (g_nPOSTspec+1)*sizeof(*g_POSTspec));
	WEBS_UNLOCK_ALLOC();
	if (g_POSTspec != NULL) {
  #else
	if (g_nPOSTspec < WEBS_MAX_POST) {				/* Room left, memo the CGI specifications		*/
  #endif
		g_POSTspec[g_nPOSTspec].Name    = Fname;
		g_POSTspec[g_nPOSTspec].Handler = Handler;
		RetVal = g_nPOSTspec++;						/* One more POST								*/
	}
	else {											/* No more room to memo the CGIs				*/
	  #if ((WEBS_DEBUG) != 0)
		WEBS_LOCK_STDIO();
		puts("WEBS  - Error - Too many POST attached");
		WEBS_UNLOCK_STDIO();
	  #endif
		RetVal = -1;
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Attach a new SSI variable update handler															*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SSInewUpdate(void (*FctPtr)(void))
{
int RetVal;
extern void RTCupdate(void);
  #if ((WEBS_MAX_SSI) <= 0)
	WEBS_LOCK_ALLOC();
	g_SSIupdateFct = realloc(g_SSIupdateFct, (g_nSSIupd+1)*sizeof(*g_SSIupdateFct));
	WEBS_UNLOCK_ALLOC();
	if (g_SSIupdateFct != NULL) {
  #else
	if (g_nSSIupd < WEBS_MAX_SSI) {
  #endif
		g_SSIupdateFct[g_nSSIupd++] = FctPtr;
		RetVal = 0;
	}
	else {
	  #if ((WEBS_DEBUG) != 0)
		WEBS_LOCK_STDIO();
		printf("WEBS  - Error - Too many SSI variable updaters");
		WEBS_UNLOCK_STDIO();
	  #endif
		RetVal = -1;
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Update the local variables																		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int SSIvarUpdate(void)
{
char           *Cptr;								/* Pointer to the SSI environment variable		*/
int             ii;									/* General purpose								*/
unsigned int    hh;									/* Hours										*/
unsigned int    mm;									/* Minutes										*/
unsigned int    ss;									/* Seconds										*/
unsigned int    Tick;								/* Snap shot of the RTOS timer tick counter		*/

	Cptr = SSIgetVar("UP_TIME");
	if (Cptr != NULL) {								/* Update the RTOS elapsed time variable		*/
		Tick = G_OStimCnt;							/* No need to check for OS_TIMER_US as lwIP		*/
		Tick = Tick / OS_TICK_PER_SEC;				/* requires the RTOS timer						*/
		Tick = Tick % 360000;						/* Roll over to 00:00:00 when reaching 99:59:59	*/
		hh   = Tick / 3600;
		Tick = Tick - (hh*3600);
		mm   = Tick / 60;
		ss   = Tick - (mm*60);

		sprintf(Cptr, "%02u:%02u:%02u", hh,mm,ss);
	}

	for (ii=0 ; ii<g_nSSIupd ; ii++) {				/* Call all SSI variable update handlers		*/
		g_SSIupdateFct[ii]();
	}

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* Declare a new variable																			*/
/*																									*/
/* Memo in g_MyVars[] the variable name, pointer to character string of that variable and a			*/
/* function pointer is this is an exec variable. The array g_MyVars[] is scanned to check if the	*/
/* variable already exists.  If it exists, the pointer to the string and function are updated.		*/
/* ------------------------------------------------------------------------------------------------ */

int SSInewVar(const char *Name, char *Value, char *(*Command)(char *, int))
{
int Exist;											/* If the variable already exists				*/
int ii;
int jj;
int LenName;
int LenVal;
int RetVal;

	Exist  = 0;
	RetVal = 0;

  #if ((WEBS_DEBUG) != 0)
	WEBS_LOCK_STDIO();
	printf("WEBS  - New Var - name:%s  Value:%s Cmd:%p\n", Name, (Value==NULL)?" ":Value, Command);
	WEBS_UNLOCK_STDIO();
  #endif

  #if ((WEBS_MAX_VARIABLE) > 0)
	jj = WEBS_MAX_VARIABLE;
  #else
	jj = g_nMyVars;
  #endif											/* First search if the variable already exists	*/
	for (ii=0 ; ii<jj  ; ii++) {
		if (0 == strcmp(Name, g_MyVars[ii].VarName)) {
			Exist = 1;
			break;									/* Found it, so it exists						*/
		}
	}
										
	LenName = strlen(Name)+1;
	LenVal  = 1;									/* Value can be NULL: e.g. attaching a function	*/
	if (Value != NULL) {
		LenVal = strlen(Value)+1;
	}

	if (Exist == 0) 	{							/* If the variable does not exist, add it		*/
	  #if ((WEBS_MAX_VARIABLE) > 0)
		LenName = sizeof(g_MyVars[0].VarName);
		LenVal  = sizeof(g_MyVars[0].Value);
		for (ii=0 ; ii<WEBS_MAX_VARIABLE ; ii++) {
			if (g_MyVars[ii].VarName[0] == '\0') {
				break;
			}
		}
	  #else
		g_nMyVars++;
		jj++;
		WEBS_LOCK_ALLOC();
		g_MyVars = realloc(g_MyVars, (g_nMyVars)*sizeof(*g_MyVars));
		WEBS_UNLOCK_ALLOC();
		if (g_MyVars == NULL) {
			ii = jj+1;								/* To report error								*/
			g_nMyVars--;
		}
		else {
			WEBS_LOCK_ALLOC();
			g_MyVars[ii].Value   = OSalloc((Value == NULL) ? 1 : LenVal);
			g_MyVars[ii].VarName = OSalloc(LenName);
			WEBS_UNLOCK_ALLOC();
			if ((g_MyVars[ii].Value == NULL)
			||  (g_MyVars[ii].VarName == NULL)) {
				ii = jj+1;							/* To report error								*/
				g_nMyVars--;
			}
		}
	  #endif
	}												/* Existing	variable will simply be updated		*/
	else {
	  #if ((WEBS_MAX_VARIABLE) > 0)
		strncpy(&g_MyVars[ii].Value[0], Value, sizeof(g_MyVars[0].Value));
		g_MyVars[ii].Value[sizeof(g_MyVars[0].Value)-1] = '\0';
	  #else
		jj = strlen(g_MyVars[ii].Value);
		if (strlen(Value) > jj) {
			WEBS_LOCK_ALLOC();
			g_MyVars[ii].Value = realloc(g_MyVars[ii].Value, jj+1);
			WEBS_UNLOCK_ALLOC();
		}
		jj = 0;
		if (g_MyVars[ii].Value == NULL) {
		  #if ((WEBS_DEBUG) != 0)
			WEBS_LOCK_STDIO();
			puts("WEBS  - Error - Out of memory");
			WEBS_UNLOCK_STDIO();
		  #endif
			RetVal = -1;
		}
	  #endif
	}

	if (RetVal == 0) {
		if (ii < jj) {
			strncpy(&g_MyVars[ii].VarName[0], Name, LenName);
			g_MyVars[ii].VarName[LenName-1] = '\0';
			if (Value == NULL) {
				g_MyVars[ii].Value[0] = '\0';
			}
			else {
				strncpy(&g_MyVars[ii].Value[0], Value, LenVal);
				g_MyVars[ii].Value[LenVal-1] = '\0';
			}
			g_MyVars[ii].Command = Command;
		}
		else {
		  #if ((WEBS_DEBUG) != 0)
			WEBS_LOCK_STDIO();
			puts("WEBS  - Error - too many variables");
			WEBS_UNLOCK_STDIO();
		  #endif
			RetVal = -1;
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Get the pointer to the value of a variable														*/
/*																									*/
/* Scan the array g_MyVars[] to find a matching name.  When the matching name is found, return the	*/
/* pointer to the character string attached to the variable as all variables are character strings	*/
/* unless it is a command variable, which is handle with ExecVar().									*/
/* ------------------------------------------------------------------------------------------------ */

char **SSIgetPtrVar(const char *Name)
{
int    ii;
int    jj;
int    Len;
char **RetVal;


	RetVal = (char **)NULL;

	if ((Name != (char *)NULL) 
	&&  (*Name != '\0')) {
	  #if ((WEBS_MAX_VARIABLE) > 0)
		jj = WEBS_MAX_VARIABLE;
	  #else
		jj = g_nMyVars;
	  #endif
		if (*Name == '$') {							/* Variable names have $ before them			*/
			Name++;
		}
		Len = strlen(Name);							/* Variable can be defined as ${name}			*/
		if (*Name == '{') {
			Name++;
			Len -= 2;
		}
													/* Scan g_MyVars[] to find a matching name		*/
		for(ii=0 ; ii<jj; ii++) {
			if (0 == strncmp(Name, g_MyVars[ii].VarName, Len)) {
				if (strlen(g_MyVars[ii].VarName) == Len) {
				  #if ((WEBS_MAX_VARIABLE) > 0)
					RetVal = (char **)&g_MyVars[ii].Value[0];
				  #else
					RetVal = &g_MyVars[ii].Value; 	/* The exact length is checked for this case:	*/
				  #endif
				}									/* var name #1: VAR_ABC							*/
			}										/* var name #2: VAR_ABCDEF						*/
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Get the value of a variable																		*/
/* ------------------------------------------------------------------------------------------------ */

char *SSIgetVar(const char *Name)
{
char  *RetVal;
char **VarPtr;

	RetVal = NULL;
	VarPtr = SSIgetPtrVar(Name);
	if (VarPtr != NULL) {
	  #if ((WEBS_MAX_VARIABLE) > 0)
		RetVal = (char *)VarPtr;
	  #else
		RetVal = *VarPtr;
	  #endif
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Set the value of a local variable																*/
/* ------------------------------------------------------------------------------------------------ */

int SSIsetVar(const char *Name , const char *Value)
{
int    RetVal;
char **VarPtr;

	RetVal = -1;
	VarPtr = SSIgetPtrVar(Name);

	if (VarPtr != NULL) {
	  #if ((WEBS_MAX_VARIABLE) > 0)
		char *Cptr;
		Cptr = (char *)VarPtr;
		strncpy(Cptr, Value, sizeof(g_MyVars[0].Value));
		Cptr[sizeof(g_MyVars[0].Value)-1] = '\0';
		RetVal = 0;
	  #else
		if (strlen(Value) > strlen(*VarPtr)) {
			WEBS_LOCK_ALLOC();
			*VarPtr = realloc(*VarPtr, strlen(Value)+1);
			WEBS_UNLOCK_ALLOC();
		}
		if (*VarPtr != NULL) {
			strcpy(*VarPtr, Value);
			RetVal = 0;
		}
	  #endif
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Get the right hand side of a SSI																	*/
/*																									*/
/* A SSI is always under the following format:														*/
/*        <!--#directive parameter="value"-->														*/
/* This function return a pointer to the character after the first " and the number of characters	*/
/* between the two "																				*/
/* ------------------------------------------------------------------------------------------------ */

static const char *SSIgetRHS(const char *Ptr, const char *LHS, int *Len)
{
const char *Cptr;
const char *RHS;

	Cptr = Ptr;
	*Len = 0;
	RHS  = (char *)NULL;

	if ((Cptr  != (char *)NULL)
	&&  (*Cptr != '\0')) {
		Cptr = strstr(Cptr, LHS);
		if (Cptr != (char *)NULL) {
			Cptr = strchr(Cptr, '\"');
		}
		if (Cptr != (char *)NULL) {
			Cptr++;
			RHS = Cptr;
		}
		if (Cptr != (char *)NULL) {
			Cptr = strchr(Cptr, '\"');
		}
		if (Cptr != (char *)NULL) {
			*Len = (int)(Cptr-RHS);			
		}
	}

	return(RHS);

}

/* ------------------------------------------------------------------------------------------------ */
/* Execute the function attached to a variable														*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int ExecVar(const char *Name, char *BufOut, int SizeOut)
{
char *Cptr;
int   ii;
int   jj;
int   Len;
	Len = 0;

	if ((Name != (char *)NULL)
	&&  (*Name != '\0')) {
	  #if ((WEBS_MAX_VARIABLE) > 0)
		jj = WEBS_MAX_VARIABLE;
	  #else
		jj = g_nMyVars;
	  #endif

		if (*Name == '$') {							/* Variable names have $ before them			*/
			Name++;
		}
		Len = strlen(Name);							/* Variable can be defined as ${name}			*/
		if (*Name == '{') {
			Name++;
			Len -= 2;
		}
													/* Scan g_MyVars[] to find a matching name		*/
		for(ii=0 ; ii<jj; ii++) {
			if (0 == strncmp(Name, g_MyVars[ii].VarName, Len)) {
				if (strlen(g_MyVars[ii].VarName) == Len) {
					if (g_MyVars[ii].Command != NULL) {
						Cptr = g_MyVars[ii].Command(BufOut, SizeOut);
						if (Cptr != NULL) {
							Len = Cptr - BufOut;
						}
						break;
					}
				}
			}
		}
	}
	return(Len);
}

/* ------------------------------------------------------------------------------------------------ */
/* Evaluate a SSI conditional expression															*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int SSIevalExpr(const char *Ptr)
{
const char *Cptr;
int   ii;
char  lhs[32];
char  op[32];
int   RetVal;
char  rhs[32];
const char *VarLeft;
const char *VarRight;

	RetVal = -1;									/* Assume this is not a valid text				*/
	Cptr   = strchr(Ptr, '\"');						/* Skip the first "								*/
	if ((Cptr != (char *)NULL)						/* Now, extract the left hand side 				*/
	&&  (*Cptr != '\0')) {
		Cptr++;
		while((*Cptr == ' ') && (*Cptr != '\0')) { 	/* Skip white spaces							*/
			Cptr++;
		}			
		ii = strcspn(Cptr, " !=><\"");				/* LHS ends when one of these is there			*/
		memmove(&lhs[0], Cptr, ii);					/* Memo the LHS									*/
		lhs[ii] = '\0';
		Cptr   += ii;								/* Be ready to extract the comparison			*/

		while((*Cptr == ' ') && (*Cptr != '\0')) { 	/* Skip white spaces							*/
			Cptr++;
		}			

		if (*Cptr == '\"') {						/* No conditional, so Boolean					*/
			VarLeft = SSIgetVar(&lhs[1]);
			if (VarLeft == (char *)NULL) {
				return(-1);
			}
			if (VarLeft[0] == '\0') {
				return(0);
			}
			return(1);
		}

		ii = strspn(Cptr, "!=><");					/* Find the first character of the comparison	*/
		memmove(&op, Cptr, ii);						/* This is the comparison operation				*/
		op[ii] = '\0';
		Cptr  += ii;

		while((*Cptr == ' ') && (*Cptr != '\0')) { 	/* Skip white spaces							*/
			Cptr++;
		}			

		ii = strcspn(Cptr, " \"");					/* Find the end of the end of the RHS			*/
		memmove(&rhs[0], Cptr, ii);					/* This is the right hand side					*/
		rhs[ii] = '\0';

		VarLeft = &lhs[0];							/* Assume the LHS is not a variable				*/
		if (lhs[0] == '$') {
			VarLeft = SSIgetVar(&lhs[1]);			/* Yes it is a variable, get its value			*/
			if (VarLeft == (char *)NULL) {			/* Variable not found, invalid condition		*/
				return(-1);
			}
		}
		VarRight = &rhs[0];							/* Assume the RHS is not a variable				*/
		RetVal   = 0;
		if (rhs[0] == '$') {
			VarRight = SSIgetVar(&rhs[1]);			/* Yes it is a variable, get its value			*/
			if (VarRight == (char *)NULL) {			/* Variable not found, invalid condition		*/
				return(-1);
			}
		}

													/* ------------------------------------------- */
		if (0 == strcmp(op, "!=")) {				/* != comparison								*/
			if (0 != strcmp(VarLeft, VarRight)) {
				RetVal = 1;
			}
		}
		else if (0 == strcmp(op, "=")) {			/* = comparison									*/
			if (0 == strcmp(VarLeft, VarRight)) {
				RetVal = 1;
			}
		}
		else if (0 == strcmp(op, ">=")) {			/* >= comparison								*/
			if (0 >= strcmp(VarLeft, VarRight)) {
				RetVal = 1;
			}
		}
		else if (0 == strcmp(op, ">")) {			/* > comparison									*/
			if (0 > strcmp(VarLeft, VarRight)) {
				RetVal = 1;
			}
		}
		else if (0 == strcmp(op, "<=")) {			/* <= comparison								*/
			if (0 <= strcmp(VarLeft, VarRight)) {
				RetVal = 1;
			}
		}
		else if (0 == strcmp(op, "<")) {			/* < comparison									*/
			if (0 < strcmp(VarLeft, VarRight)) {
				RetVal = 1;
			}
		}
		else {										/* Unknown comparison							*/
			RetVal = -1;							/* return invalid								*/
		}
	}

	return(RetVal);
}

/* EOF */
