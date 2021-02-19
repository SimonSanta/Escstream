/* ------------------------------------------------------------------------------------------------ */
/* FILE :		WebServer.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Code for the lwIP WebServer with netconn / BSD sockets.								*/
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
/*	$Revision: 1.11 $																				*/
/*	$Date: 2019/01/10 18:07:13 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include <stdio.h>
#include <string.h>

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

#ifdef __UABASSI_H__
  #include "Xtra_uAbassi.h"							/* For Events and Mailbox emulation				*/
#endif

/* ------------------------------------------------------------------------------------------------ */

#ifdef __cplusplus
 extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------ */

typedef uint16_t    (*SSIhandler_t)(int Index, char *Insert, int InsertLen);
typedef const char *(*CGIhandler_t)(int NumParams, char *Param[], char *Value[]);

/* ------------------------------------------------------------------------------------------------ */
/* File system used (needed by the SSI #include directive)											*/

#define WEBS_FILESYS_NONE		0
#define WEBS_FILESYS_SYSCALL	1
#define WEBS_FILESYS_FATFS		2
#define WEBS_FILESYS_FULLFAT	3
#define WEBS_FILESYS_UEFAT		4

/* ------------------------------------------------------------------------------------------------ */

void  CGIinit(void);
char *CGIprocess(char *buf);
int   CGInewHandler(const char *Fname, const CGIhandler_t Handler);
void  http_server_init(void);
void  LCDputs(char *Line_1, char *Line_2);
void  POSTinit(void);
int   POSTnewHandler(const char *Fname, const CGIhandler_t Handler);
char *POSTprocess(char *uri, char *Data);
int   SSIexpand(char *buf, int Bsize, const char *uri);
char *SSIgetVar(const char *Name);
int   SSIsetVar(const char *Name, const char *Value);
void  SSIinit(void);
int   SSInewUpdate(void (*FctPtr)(void));
int   SSInewVar(const char *Name, char *Value, char *(*Command)(char *, int));
int   SSIvarUpdate(void);
void  WebServerInit(void);

#if ((OS_PLATFORM) == 0x1032F407)
  void ETH_BSP_Config(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

/* EOF */

