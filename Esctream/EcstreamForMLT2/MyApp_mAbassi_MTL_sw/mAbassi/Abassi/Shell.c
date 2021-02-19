/* ------------------------------------------------------------------------------------------------ */
/* FILE :		Shell.c																				*/
/*																									*/
/* CONTENTS :																						*/
/*				Shell to probe into / modify the RTOS services in an application					*/
/*																									*/
/*																									*/
/* Copyright (c) 2018-2019, Code-Time Technologies Inc. All rights reserved.						*/
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
/*	$Revision: 1.29 $																				*/
/*	$Date: 2019/02/10 16:53:39 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* NOTES:																							*/
/*		This shell must be running as a task:														*/
/*				TSKcreate("Shell", PRIO, STACKSIZE, &OShell, 1);									*/
/*																									*/
/*		Because stdin is the input device for the commands, the task priority of Shell should be	*/
/*		set to:																						*/
/*			Lowest priority - if the UART driver is configured for polling.							*/
/*			Lowest priority - if the UART driver has the build option UART_FULL_PROTECT != 0.		*/
/*			Any priority    - if the UART driver is blocking.										*/
/*																									*/
/*		Names can be enclosed within '' or "" to deal with spaces in the names.						*/
/*		A sub-shell (application specific) can be added to this RTOS shell.							*/
/*																									*/
/* BUILD OPTIONS:																					*/
/*		SHELL_CMD_LEN  - Maximum number of character the command line buffer can hold				*/
/*		                 It excludes the final '\0' so the real buffer size is SHELL_CMD_LEN + 1	*/
/*		SHELL_FILES    - When != 0 it includes the file system commands alike ls rm etc				*/
/*		SHELL_HISTORY  - When set to >0 it enable the command history and size the history buffer	*/
/*		                 to SHELL_HISTORY past commands.											*/
/*		SHELL_INPUT    - Select if gets(), or polling GetKey(), is used for reading the input		*/
/*		                     <= 0 : using gets()													*/
/*		                     >  0 : polling GetKey() where the SHELL_INPUT value is the sleep time	*/
/*		                            in ms when no char is available									*/
/*		                 If gets() is used and the library re-entrance protection is enable this 	*/
/*		                 can lock the system. Depending on what type of protection is activated a	*/
/*		                 single mutex can be used to to provide exclusive access to all types of	*/
/*		                 I/O. So if the shell is blocked waiting for an input char, it would lock	*/
/*		                 the mutex and not allow any other task sto print on stdout, write, or read	*/
/*                       media files or devices														*/
/*		SHELL_LOGIN    - Active login credential requirement (i.e. username and password)			*/
/*		                    <= 0 : no login required												*/
/*		                    == 1 : login required and there is no inactivity auto logout			*/
/*		                    >= 2 : login required with SHELL_LOGIN seconds inactivity auto logout	*/
/*		                    usernames, passwords, and access privileges are defined in the			*/
/*		                    g_Login[] data structure. Multiple usernames are supported.				*/
/*		SHELL_USE_SUB  - If attaching an application-specific sub-shell								*/
/*		                    == 0 : application specific sub-shell not used							*/
/*		                    != 0 : application specific sub-shell used								*/
/*		                 Refer to the file SubShell.c to see how to interface a sub-shell with		*/
/*		                 this shell.																*/
/*		                 Don't forget to add your own sub-shell file into the build.				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "Shell.h"

#ifndef SHELL_CMD_LEN
  #define SHELL_CMD_LEN		128						/* Max number of char a command line can hold	*/
#endif

#ifndef SHELL_FILES									/* If the file system command are supported		*/
  #define SHELL_FILES		0						/* == 0 : no file system support				*/
#endif												/* != 0 : file system commands included			*/
													/* Select how input is read						*/
#ifndef SHELL_HISTORY								/* If a command history buffer & edit is used	*/
  #define SHELL_HISTORY		0						/* <= 0 : no history buffer						*/
#endif												/* >  0 : number of last commands to hold		*/
													/* Select how input is read						*/
#ifndef SHELL_INPUT									/* <= 0 : use gets()							*/
  #define SHELL_INPUT		10						/* >  0 : use GetKey() with SHELL_INPUT ms 		*/
#endif												/* sleep when no char available					*/

#ifndef SHELL_LOGIN									/* To enable login accesses						*/
  #define SHELL_LOGIN		0						/* == 0 no username & passswd required			*/
#endif												/* == 1 username & passwd / g_Login[] no expiry	*/
													/* >= 2 username & passwd / g_Login[] #s expiry	*/

#ifndef SHELL_USE_SUB								/* If a sub-shell is used						*/
  #define SHELL_USE_SUB		0						/* == 0 : no sub-shell							*/
#endif												/* != 0 : SubShell() called (See SubShell.c)	*/

/* ------------------------------------------------------------------------------------------------ */

#if ((SHELL_FILES) != 0)
 #ifndef SYS_CALL_TTY_EOF
  #define SYS_CALL_TTY_EOF	4
 #endif
#endif

#if ((OX_NAMES) == 0)
  #error "Shell.c - OS_NAME must be set to a non-zero value"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* App definitions																					*/


#define SHELL_RES_NONE				 0
#define SHELL_RES_OK				 1
#define SHELL_RES_OK_NO_MSG			 2
#define SHELL_RES_ERR_NO_MSG		 3
#define SHELL_RES_PRT_HEX			 4
#define SHELL_RES_PRT_INTPTR		 5
#define SHELL_RES_UNK_CMD_ARG_0		 6
#define SHELL_RES_UNK_CMD_ARG_1		 7
#define SHELL_RES_UNK_CMD_ARG_2		 8
#define SHELL_RES_UNK_CMD_ARG_3		 9
#define SHELL_RES_UNK_CMD_ARG_4		10
#define SHELL_RES_UNK_CMD_ARG_5		11
#define SHELL_RES_UNK_OP_ARG_1		12
#define SHELL_RES_UNK_OP_ARG_2		13
#define SHELL_RES_UNK_OP_ARG_3		14
#define SHELL_RES_UNK_OP_ARG_4		15
#define SHELL_RES_UNK_OP_ARG_5		16
#define SHELL_RES_UNK_FLD_ARG_1		17
#define SHELL_RES_UNK_FLD_ARG_2		18
#define SHELL_RES_UNK_FLD_ARG_3		19
#define SHELL_RES_UNK_FLD_ARG_4		20
#define SHELL_RES_UNK_FLD_ARG_5		21
#define SHELL_RES_UNAVAIL_OP_ARG_0	22
#define SHELL_RES_UNAVAIL_OP_ARG_1	23
#define SHELL_RES_UNAVAIL_OP_ARG_2	24
#define SHELL_RES_UNAVAIL_OP_ARG_3	25
#define SHELL_RES_UNAVAIL_OP_ARG_4	26
#define SHELL_RES_UNAVAIL_OP_ARG_5	27
#define SHELL_RES_UNAVAIL_FLD_ARG_1	28
#define SHELL_RES_UNAVAIL_FLD_ARG_2	29
#define SHELL_RES_UNAVAIL_FLD_ARG_3	30
#define SHELL_RES_UNAVAIL_FLD_ARG_4	31
#define SHELL_RES_UNAVAIL_FLD_ARG_5	32
#define SHELL_RES_INVAL_VAL_ARG_1	33
#define SHELL_RES_INVAL_VAL_ARG_2	34
#define SHELL_RES_INVAL_VAL_ARG_3	35
#define SHELL_RES_INVAL_VAL_ARG_4	36
#define SHELL_RES_INVAL_VAL_ARG_5	37
#define SHELL_RES_NO_NAME_ARG_1		38
#define SHELL_RES_NO_NAME_ARG_2		39
#define SHELL_RES_NO_NAME_ARG_3		40
#define SHELL_RES_NO_NAME_ARG_4		41
#define SHELL_RES_NO_NAME_ARG_5		42
#define SHELL_RES_OUT_RANGE_ARG_1	43
#define SHELL_RES_OUT_RANGE_ARG_2	44
#define SHELL_RES_OUT_RANGE_ARG_3	45
#define SHELL_RES_OUT_RANGE_ARG_4	46
#define SHELL_RES_OUT_RANGE_ARG_5	47
#define SHELL_RES_NO_SERV			48
#define SHELL_RES_NOT_VALID			49
#define SHELL_RES_IS_RD_ONLY		50

#if defined(__ABASSI_H__)
  #define PROMPT       "Abassi> "
  #define PROMPT_FILES "Abassi [%s]> "
#elif defined(__MABASSI_H__)
  #define PROMPT       "mAbassi> "
  #define PROMPT_FILES "mAbassi [%s]> "
#else
  #define PROMPT       "uAbassi> "
  #define PROMPT_FILES "uAbassi [%s]> "
#endif

#define LEGEND_ALIGN				32				/* Where right fields (values) start			*/

#if ((SHELL_LOGIN) <= 0)							/* Macro to know if R) or RW access				*/
  #define SHELL_CHK_WRT(_Result)	(1)
#else
  #define SHELL_CHK_WRT(_Result)	(0 != ShellRdWrt(&(_Result)))
#endif
													/* Compile should reuse the format string		*/
													/* and it keeps all help with same indentation	*/
#define HLP_CMD(_cmd)				printf("       %s\n", _cmd)
#define HLP_DSC(_desc)				printf("            %s\n", _desc)
#define HLP_CMP(_comp)				printf("                - component %s\n", _comp)

/* ------------------------------------------------------------------------------------------------ */
/* Apps variables																					*/

typedef struct {
	char *Name;
	int (* FctPtr)(int argc, char *argv[]);
} Cmd_t;

static int  g_EchoOff;								/* != is to not echo the password				*/
static char g_StrIP[32];							/* String to print intptr_t in hex values		*/
static char g_StrIPn[32];							/* String to print intptr_t in hex values		*/
static char g_StrP[32];								/* String to print address values				*/
static char g_StrPn[32];							/* String to print address values				*/
static char g_StrV[32];								/* String to print int in hex values			*/
static char g_StrVn[32];							/* String to print int in hex values			*/
#if ((OX_PERF_MON) != 0)
  static char TmpStr1[64];
  static char TmpStr2[64];
#endif

#if ((SHELL_HISTORY) > 0)
  static char g_CmdHist[(SHELL_HISTORY)+1][(SHELL_CMD_LEN)+1];
  static int  g_HistNew;							/* Index of the newest history command line		*/
  static int  g_HistOld;							/* Index of the oldest history command line		*/
#endif

#if ((OX_MEM_BLOCK) != 0)
 #if ((OX_HASH_MBLK) == 0U)
  extern MBLK_t *g_MblkList;
 #else
  extern MBLK_t *g_MblkList[(OX_HASH_MBLK)+1U];
 #endif
#endif

#if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
 #if ((OX_HASH_MBX) == 0U)
  extern MBX_t *g_MboxList;
 #else
  extern MBX_t *g_MboxList[(OX_HASH_MBX)+1U];
 #endif
#endif

#if ((OX_HASH_MUTEX) == 0U)
  extern MTX_t	*g_MutexList;
#else
  extern MTX_t	*g_MutexList[(OX_HASH_MUTEX)+1U];
#endif

#if ((OX_HASH_SEMA) == 0U)
  extern SEM_t *g_SemaList;
#else
  extern SEM_t *g_SemaList[(OX_HASH_SEMA)+1U];
#endif

#if ((OX_HASH_TASK) == 0U)
  extern TSK_t *g_TaskList;
#else
  extern TSK_t *g_TaskList[(OX_HASH_TASK)+1U];
#endif

#if ((OX_TIMER_SRV) != 0)
 #if ((OX_HASH_TIMSRV) == 0U)
  extern TIM_t *g_TimerList;
 #else
  extern TIM_t *g_TimerList[(OX_HASH_TIMSRV)+1U];
 #endif
#endif

#if ((SHELL_LOGIN) > 0)
  typedef struct {
	char User[128];									/* Username										*/
	char Pswd[128];									/* Password										*/
	int  ReadOnly;									/* ==0 Read-Write access / !=0 Read-only access	*/
  } Login_t;
													/* NOTE for SH_USERNAME_# & SH_PASSWORD_#		*/
													/* When defined on the command line, use \" to	*/
													/* make sure the defines are strings:			*/
													/*		-DSH_USERNAME_0=\"NewUsername\"			*/
													/*		-DSH_PASSWORD_0=\"NewPassword\"			*/

  static Login_t g_Login[] = {
                              {"CodeTime",  "Abassi", 0}		/* Full RW access					*/
                             ,{"anonymous", "guest",  1}		/* Read only access					*/
                           #ifdef SH_USERNAME_0
							 ,{SH_USERNAME_0, SH_PASSWORD_0, SH_ROACCESS_0}
						   #endif
                           #ifdef SH_USERNAME_1
							 ,{SH_USERNAME_1, SH_PASSWORD_1, SH_ROACCESS_1}
						   #endif
                           #ifdef SH_USERNAME_2
							 ,{SH_USERNAME_2, SH_PASSWORD_2, SH_ROACCESS_2}
						   #endif
                           #ifdef SH_USERNAME_3
							 ,{SH_USERNAME_3, SH_PASSWORD_3, SH_ROACCESS_3}
						   #endif
                           #ifdef SH_USERNAME_4
							 ,{SH_USERNAME_4, SH_PASSWORD_4, SH_ROACCESS_4}
						   #endif
                           #ifdef SH_USERNAME_5
							 ,{SH_USERNAME_5, SH_PASSWORD_5, SH_ROACCESS_5}
						   #endif
                           #ifdef SH_USERNAME_6
							 ,{SH_USERNAME_6, SH_PASSWORD_6, SH_ROACCESS_6}
						   #endif
                           #ifdef SH_USERNAME_7
							 ,{SH_USERNAME_7, SH_PASSWORD_7, SH_ROACCESS_7}
						   #endif
                           #ifdef SH_USERNAME_8
							 ,{SH_USERNAME_8, SH_PASSWORD_8, SH_ROACCESS_8}
						   #endif
                           #ifdef SH_USERNAME_9
							 ,{SH_USERNAME_9, SH_PASSWORD_9, SH_ROACCESS_9}
						   #endif
                             };
  static int g_IsRdOnly;							/* != 0 is read-only access						*/
  static int g_IsLogin;								/* != 0 is currently logged in					*/
#endif												/* == 0 asks for valid username & password		*/

extern int G_UartDevIn;								/* SysCall input device # (for echo on/off)		*/
extern int uart_filt(int Dev, int Deb, int Filt);	/* Declare localy(too many #include ???_uart.h)	*/

/* ------------------------------------------------------------------------------------------------ */
/* Apps functions																					*/

static int CmdHelp    (int argc, char *argv[]);
static int CmdEdit    (int argc, char *argv[]);
static int CmdEvt     (int argc, char *argv[]);
static int CmdGrp     (int argc, char *argv[]);
static int CmdLog     (int argc, char *argv[]);
static int CmdMblk    (int argc, char *argv[]);
static int CmdMbx     (int argc, char *argv[]);
static int CmdMem     (int argc, char *argv[]);
static int CmdMtx     (int argc, char *argv[]);
static int CmdSem     (int argc, char *argv[]);
static int CmdSys     (int argc, char *argv[]);
static int CmdTask    (int argc, char *argv[]);
static int CmdTim     (int argc, char *argv[]);

#if ((SHELL_LOGIN) > 0)
static int CmdExit    (int argc, char *argv[]);
#endif

static Cmd_t CommandLst[] = {
	{ "help",    &CmdHelp		},
	{ "edit",	 &CmdEdit		},
	{ "evt",	 &CmdEvt		},
  #if ((SHELL_LOGIN) > 0)
	{ "exit",    &CmdExit		},
  #endif
	{ "grp",	 &CmdGrp		},
	{ "log",	 &CmdLog		},
	{ "mblk",	 &CmdMblk		},
	{ "mbx",	 &CmdMbx		},
	{ "mem",	 &CmdMem		},
	{ "mtx",	 &CmdMtx		},
	{ "sem",	 &CmdSem		},
	{ "sys",	 &CmdSys		},
	{ "task",	 &CmdTask		},
	{ "tim",	 &CmdTim		},
	{ "?",       &CmdHelp		}
};

#if (((OX_MAILBOX) != 0) && ((OX_GROUP) != 0) && !defined(__UABASSI_H__))
  static MBX_t *FindMBX(const char *Name);
#endif
static SEM_t *FindSEM(const char *Name);
static void   Legend(const char *Txt);
static int    PrtResult(char *Srv, int Result, int OSret, char *argv[], int Value, intptr_t IntPtr);
static void   ShellGetEdit(char *Str);

#if ((SHELL_FILES) != 0)
  static int  FileShell(int argc, char *argv[]);
#endif

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */

void OSshell(void)
{
int  Arg_C;											/* Number of tokens on the command line			*/
int  ii;											/* General purpose								*/
int  IsStr1;										/* Toggle to decode quoted ' strings			*/
int  IsStr2;										/* Toggle to decode quoted " strings			*/
int  jj;											/* General purpose								*/

static char *Arg_V[10];								/* Individual tokens from the command line		*/
static char  CmdLine[(SHELL_CMD_LEN)+1];			/* Command line typed by the user				*/
static char  SetV[3];								/* IAR has issues with setvbuf _IONULL			*/
#if ((SHELL_FILES) != 0)
  static char  MyDir[128];
#endif

/* ------------------------------------------------ */
/* Set buffering here: tasks inherit main's stdios	*/
/* when Newlib reent is used & stdio are shared		*/ 

	setvbuf(stdin,  &SetV[0], _IONBF, 1);			/* By default, Newlib library flush the I/O		*/
	setvbuf(stdout, &SetV[1], _IONBF, 1);			/* buffer when full or when new-line			*/
	setvbuf(stderr, &SetV[2], _IONBF, 1);			/* Done before OSstart() to have all tasks with	*/
													/* the same stdio set-up						*/

	if (sizeof(int) == 8) {							/* Define the printf width depending on the		*/
		strcpy(g_StrV,  "0x%16X");					/* data size of an int							*/
		strcpy(g_StrVn, "0x%16X\n");
	}
	else if (sizeof(int) == 4) {
		strcpy(g_StrV,  "0x%08X");
		strcpy(g_StrVn, "0x%08X\n");
	}
	else {
		strcpy(g_StrV,  "0x%04X");
		strcpy(g_StrVn, "0x%04X\n");
	}

	if (sizeof(intptr_t) == 8) {					/* Define the printf width depending on the		*/
		strcpy(g_StrIP,  "0x%16X");					/* data size of an intptr_t						*/
		strcpy(g_StrIPn, "0x%16X\n");
	}
	else if (sizeof(intptr_t) == 4) {
		strcpy(g_StrIP,  "0x%08X");
		strcpy(g_StrIPn, "0x%08X\n");
	}
	else {
		strcpy(g_StrIP,  "0x%04X");
		strcpy(g_StrIPn, "0x%04X\n");
	}

	if (sizeof(void *) == 8) {						/* Define the printf width depending on the		*/
		strcpy(g_StrP,  "%018p");					/* data size of a pointer						*/
		strcpy(g_StrPn, "%018p\n");
	}
	else if (sizeof(void *) == 4) {
		strcpy(g_StrP,  "%010p");
		strcpy(g_StrPn, "%010p\n");
	}
	else {
		strcpy(g_StrP,  "%06p");
		strcpy(g_StrPn, "%06p\n");
	}

	g_EchoOff = 0;

  #if ((SHELL_LOGIN) > 0)
	g_IsLogin = 0;									/* Not yet logged in							*/
  #endif

  #if ((SHELL_HISTORY) > 0)
	g_HistNew = 0;
	g_HistOld = 0;
  #endif

	uart_filt(G_UartDevIn, 0, (1U<<0));				/* Turn echo off when doing full edit			*/

	putchar('\n');

/* ------------------------------------------------ */
/* Processing loop									*/

	for (;;) {										/* Infinite loop								*/

	  #if ((SHELL_LOGIN) > 0)
		while (g_IsLogin == 0) {					/* Ask userane & password as long as not		*/
		  char User[256];							/* properly logged in							*/
		  char Passwd[256];

			do {
				printf("Username : ");				/* Get the username								*/
				ShellGets(&User[0], sizeof(User));
			} while (strlen(&User[0]) == 0);

			g_EchoOff = 1;
			 printf("Password : ");					/* Get the password								*/
			 ShellGets(&Passwd[0], sizeof(Passwd));
			g_EchoOff = 0;

			putchar('\n');
													/* Check for valid username / password			*/
			for (ii=0 ; ii<(sizeof(g_Login)/sizeof(g_Login[0])) ; ii++) {
				if ((0 == strcmp(&g_Login[ii].User[0], &User[0]))
				&&  (0 == strcmp(&g_Login[ii].Pswd[0], &Passwd[0]))) {
					g_IsLogin  = 1;					/* Found a match, move on						*/
					g_IsRdOnly = g_Login[ii].ReadOnly;	/* Memo if RO or RW access					*/
					break;;
				}
			}
		}
	  #endif

	  #if ((SHELL_FILES) == 0)
		printf(PROMPT);								/* Command line prompt							*/
	  #else
		if (NULL == getcwd(&MyDir[0], sizeof(MyDir))) {
			printf(PROMPT);
		}
		else {
			printf(PROMPT_FILES, &MyDir[0]);
		}
	  #endif

		ShellGetEdit(&CmdLine[0]);					/* Get the command line with editing			*/

		ii            = strlen(CmdLine);			/* Adding a space at the end simplifies a lot	*/
		CmdLine[ii]   = ' ';
		CmdLine[ii+1] = '\0';

		Arg_C  = 0;									/* Index filling argv[]							*/
		ii     = 0;									/* Index in CmdLine of the char been looked at	*/
		IsStr1 = 0;									/* If is dealing with a '						*/
		IsStr2 = 0;									/* If is dealing with a "						*/
		jj     = 0;									/* To shift command line when removing ' or "	*/

		while (CmdLine[ii] != '\0') {				/* Do as long not gone through whole cmd line	*/

			while (CmdLine[ii] == ' ') {			/* Skip initial white space before this token	*/
				ii++;
			}

			if (CmdLine[ii] == '\0') {				/* The whole command line was processed			*/
				break;
			}

			Arg_V[Arg_C++] = &CmdLine[ii];			/* Memo the beginning of this token				*/

			while ((CmdLine[ii] != '\0')			/* Absorb the token until space or EOL			*/
			&&     ((CmdLine[ii] != ' ') || IsStr1 || IsStr2)) {

				if (CmdLine[ii] == '\'') {			/* Toggle beginning / end of string				*/
					ii++;
					if (IsStr2 == 0) {				/* Keep ' if enclosed in "						*/
						ii--;
						jj = ii;
						while (CmdLine[jj] != '\0') {	/* Remove ' by shifting the command line	*/
							CmdLine[jj] = CmdLine[jj+1];
							jj++;
						}
						IsStr1 = !IsStr1;
					}

				}
				else if (CmdLine[ii] == '\"') {		/* Toggle beginning / end of string				*/
					ii++;
					if (IsStr1 == 0) {				/* Keep " if enclosed in '						*/
						ii--;
						jj = ii;
						while (CmdLine[jj] != '\0') {	/* Remove " by shifting the command line	*/
							CmdLine[jj] = CmdLine[jj+1];
							jj++;
						}
						IsStr2 = !IsStr2;
					}
				}
				else {								/* not 1, nor ", go to next char index			*/
					ii++;
				}
			}

			CmdLine[ii++] = '\0';					/* Terminate this argument						*/

			if (Arg_C >= sizeof(Arg_V)/sizeof(Arg_V[0]) ) {
				break;								/* Don't exceed the sizwe of the argv[] array	*/
			}
		}

		if (Arg_C != 0) {							/* Non-empty line								*/
			if (Arg_V[0][0] == '#') {				/* Commented line								*/
				Arg_C = 0;
			}
		}

		if (Arg_C != 0) {							/* It is not an empty line						*/
													/* Find if this is a one of my commands			*/
			for (ii=0 ; ii<(sizeof(CommandLst)/sizeof(CommandLst[0])); ii++) {
				if (0 == strcmp(CommandLst[ii].Name, &Arg_V[0][0])) {
					CommandLst[ii].FctPtr(Arg_C, Arg_V);
					break;
				}
			}
													/* When not one of my commands. check sub-shell	*/
			if (ii >= (sizeof(CommandLst)/sizeof(CommandLst[0]))) {
			  #if (((SHELL_USE_SUB) == 0) && ((SHELL_FILES) == 0))
				printf("ERROR: unknown command \"%s\"\n", Arg_V[0]);
			  #else
				jj = 1;
			   #if ((SHELL_FILES) != 0)
				if ((jj != 0)
				&&  (FileShell(Arg_C, Arg_V) != 99)) {
					jj = 0;							/* Command recognized, then no error			*/
				}
			   #endif
			   #if ((SHELL_USE_SUB) != 0)
				if ((jj != 0)
				&&  (SubShell(Arg_C, Arg_V) != 99)) {
					jj = 0;							/* Command recognized, then no error			*/
				}
			   #endif
				if (jj != 0) {
					printf("ERROR: unknown command \"%s\"\n", Arg_V[0]);
				}
			  #endif
			}
		}

	}
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */

static int CmdHelp(int argc, char *argv[])
{
int ii;												/* General purpose								*/
int Result;											/* Result of command (to select error to print)	*/

	Result = SHELL_RES_NONE;						/* Start with no error							*/

	if (argc < 0) {									/* Special value to print usage					*/
		puts("help : me");

		if (argc < -1) {							/* Print the help for all commands				*/
			puts("usage: help                       Show help for all commands");
			puts("       help cmd ...               Show help for specified commands");
			puts("       ?                          can be used instead of \"help\"");
		}
		Result = SHELL_RES_OK_NO_MSG;
	}

	else if (argc == 1) {							/* "help" alone on the command line				*/
		puts("usage: help                       Show basic help for all commands");
		puts("       help cmd ...               Show full help for specified commands\n");
		puts("List of commands:\n");
		for (ii=0 ; ii<(sizeof(CommandLst)/sizeof(CommandLst[0])) ; ii++) {
			if (CommandLst[ii].Name[0] != '?') {
				(void)CommandLst[ii].FctPtr(-1, NULL);
			}
		}

	  #if ((SHELL_FILES) != 0)						/* If file commands used, get it to do the same	*/
		FileShell(argc, argv);						/* with its own commands						*/
	  #endif

	  #if ((SHELL_USE_SUB) != 0)					/* If sub-shell used, get it to do the same		*/
		SubShell(argc, argv);						/* with its own commands						*/
	  #endif

	}

	else if (argc == 2) {							/* "help cmd" on the command line				*/
		Result = SHELL_RES_UNK_CMD_ARG_1;			/* Assume unknown command						*/
		for (ii=0 ; ii<(sizeof(CommandLst)/sizeof(CommandLst[0])) ; ii++) {
			if (0 == strcmp(argv[1], CommandLst[ii].Name)) {
				(void)CommandLst[ii].FctPtr(-2, NULL);
				Result = SHELL_RES_NONE;
				break;
			}
		}
													/* If we are here, command not found			*/
	  #if ((SHELL_FILES) != 0)						/* When file commands used see if it knows about*/
		if ((Result == SHELL_RES_UNK_CMD_ARG_1)		/* that command									*/
		&&  (0 == FileShell(argc, argv))) {
			Result = SHELL_RES_NONE;				/* yes, sub-shell printed halp so we are done	*/
		}											/* with success									*/
	  #endif
													/* If we are here, command not found			*/
	  #if ((SHELL_USE_SUB) != 0)					/* When sub-shell used see if it knows about	*/
		if ((Result == SHELL_RES_UNK_CMD_ARG_1)		/* that command									*/
		&&  (0 == SubShell(argc, argv))) {
			Result = SHELL_RES_NONE;				/* yes, sub-shell printed halp so we are done	*/
		}											/* with success									*/
	  #endif

	}

	else {											/* Print usage in case of error					*/
		puts("usage: help                       Show basic help for all commands");
		puts("       help cmd ...               Show full help for the specified command\n");
		Result = SHELL_RES_ERR_NO_MSG;
	}

	return(PrtResult("help", Result, 0, argv, 0, (intptr_t)0));
}

/* ------------------------------------------------------------------------------------------------ */
/* Event command : edit																				*/
/*																									*/
/* Is not a real command, only a help for the control keys											*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdEdit(int argc, char *argv[])
{
	if (argc == -1) {								/* Special value to print usage					*/
		puts("edit : command line control characters");
	}
	else {
		puts("usage:");
		puts("        # : at the start of line makes the line a comment");
		puts("       ^B : backward 1 character (Left arrow key on VT100)");
		puts("       ^F : forward 1 character (Right arrow key on VT100)");
		puts("       ^A : beginning of the line");
		puts("       ^E : end of the line");
	  #if ((SHELL_HISTORY) > 0)
		puts("       ^P : back in time in the history (Up arrow key on VT100)");
		puts("       ^N : forward in time in the history (Down arrow key on VT100)");
	  #endif
	}

	return(0);
}

/* ------------------------------------------------------------------------------------------------ */
/* Event command : evt																				*/
/*																									*/
/* Use the run-time help for a complete description of all the commands supported					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdEvt(int argc, char *argv[])
{
#if defined(__UABASSI_H__)
	puts("evt  : events emulated, no debug available");
	return(-1);
#elif ((OX_EVENTS) == 0)
	puts("evt  : events - not supported (OS_EVENTS == 0)");
	return(-1);
#else
char    *Cptr;										/* Used to know if number is malformed			*/
int      ii;										/* General purpose								*/
int	     OSret;										/* Return value from OS service call			*/
int      Result;									/* Result of command (to select error to print)	*/
char    *Serv = "EVT";								/* Service name									*/
TSK_t    TaskNow;									/* Snapshot of the one to display / manipulate	*/
TSK_t   *TaskScan;									/* To traverse Abassi internal linked list		*/
int      Value;										/* Numerical value specified on command line	*/

	OSret    = 0;									/* OS return is OK								*/
	Result   = SHELL_RES_NONE;						/* Start with no error							*/
	Value    = 0;									/* Remove compiler warning						*/

	if (argc < 0) {									/* Special value to print usage					*/
		puts("evt  : event operations");

		if (argc < -1) {
			puts("usage:");
		  #if ((OX_WAIT_ABORT) != 0)
			HLP_CMD("evt <TaskName> abort");
			  HLP_DSC("abort the blocking on its events of the task <TaskName>");
			  HLP_CMP("EVTabort()");
		  #endif
			HLP_CMD("evt <TaskName> set #");
			  HLP_DSC("set the event flags # of the task <TaskName>");
			  HLP_CMP("EVTset()");
		}
		Result = SHELL_RES_OK_NO_MSG;
	}
	else if ((argc != 4)
  #if ((OX_WAIT_ABORT) != 0)
	&&  (argc !=  3)
  #endif
	) {
	  #if ((OX_WAIT_ABORT) != 0)
		puts("usage: evt TaskName abort|{set #}");
	  #else
		puts("usage: evt TaskName set #");
	  #endif
		Result = SHELL_RES_ERR_NO_MSG;
	}

	TaskScan = (TSK_t *)NULL;
	if (Result == SHELL_RES_NONE) {
		TaskScan = TSKgetID(argv[1]);
		if (TaskScan != (TSK_t *)NULL) {			/* Grab a copy to minimize possible changes		*/
			ii = OSintOff();						/* when displaying the information				*/
			memmove(&TaskNow, TaskScan, sizeof(TaskNow));
 			OSintBack(ii);
		}
		else {			/* No tasks with the name specified				*/
			Serv   = "task";
			Result = SHELL_RES_NO_NAME_ARG_1;
		}
	}

	if ((Result == SHELL_RES_NONE)
	&&  (argc == 3)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/
		Result  = SHELL_RES_OK;						/* Assume printing OK result					*/
	  #if ((OX_WAIT_ABORT) != 0)
		if (0 == strcmp(argv[2], "abort")) {		/* CMD: evt TaskName abort						*/
			EVTabort(TaskScan);
		}
		else {										/* The operation specified is not "abort"		*/
			Result = SHELL_RES_UNK_OP_ARG_2;
		}
	  #else
		Result = SHELL_RES_UNAVAIL_OP_ARG_2;
	  #endif
	}

	if ((Result == SHELL_RES_NONE)
	&&  (argc == 4)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/
		if (0 == strcmp(argv[2], "set")) {			/* CMD: evt TskName set #						*/
			Value = (int)strtoll(argv[3], &Cptr, 0);
			if (*Cptr == '\0') {
				EVTset(TaskScan, Value);
			}
			else {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		}
		else {										/* The operation specified is not "set"			*/
			Result = SHELL_RES_UNK_OP_ARG_2;
		}
	}

	return(PrtResult(Serv, Result, OSret, argv, Value, (intptr_t)0));

#endif
}

/* ------------------------------------------------------------------------------------------------ */
/* Logout command : exit																			*/
/*																									*/
/* Use the run-time help for a complete description of all the commands supported					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#if ((SHELL_LOGIN) > 0)
static int CmdExit(int argc, char *argv[])
{
	if (argc < 0) {									/* Special value to print usage					*/
		puts("exit : exit from the shell");
		if (argc < -1) {
			puts("usage:");
			HLP_CMD("exit");
		}
	}
	else {
		g_IsLogin = 0;
	}

	return(0);
}
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Group commands : group																			*/
/*																									*/
/* grp TaskName																						*/
/*		dump the info on the group TaskName is blocked on											*/
/*																									*/
/* grp TaskName rmAll																				*/
/*		remove all group entries TaskName is blocked on												*/
/*																									*/
/* grp MbxName rmMBX												(OS_MAILBOX != 0)				*/
/*		remove MbxName from the group it is part of													*/
/*																									*/
/* grp SemName rmSEM																				*/
/*		remove SemName from the group it is part of													*/
/*																									*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdGrp(int argc, char *argv[])
{
#if ((OX_GROUP) == 0)
	puts("grp  : groups - not supported (OS_GROUP == 0)");
	return(-1);
#else

char     *Cptr;										/* Used to know if number is malformed			*/
GRP_t    GrpNow;									/* Snapshot of the one to display / manipulate	*/
GRP_t   *GrpScan;									/* To traverse the group linked list			*/
int      ii;										/* General purpose								*/
int	     OSret;										/* Return value from OS service call			*/
int      Result;									/* Result of command (to select error to print)	*/
SEM_t   *SemNow;									/* Semaphore being processed					*/
char    *Serv = "GRP";								/* Service name									*/
TSK_t   *TaskNow;									/* Task being processed							*/
intptr_t Value;										/* Numerical value specified on command line	*/
#if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
  MBX_t  *MbxNow;									/* Mailbox being processed						*/
#endif

	OSret   = 0;									/* OS return is OK								*/
	Result  = SHELL_RES_NONE;						/* Start with no error							*/

	if (argc < 0) {									/* Special value to print usage					*/
		puts("grp  : group information / processing");

		if (argc < -1) {
			puts("usage:");
			HLP_CMD("grp <TaskName>");
			  HLP_DSC("show all the services in the group used by the task <TaskName>");
			HLP_CMD("grp <TaskName> rm all");
			  HLP_DSC("remove all services from the group used by the task <TaskName>");
			  HLP_CMP("GRPrmALL()");
		  #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
			HLP_CMD("grp <MbxName> rm mbx");
			  HLP_DSC("remove the mailbox <MbxName> from the group it's part of");
			  HLP_CMP("GRPrmMBX()");
		  #endif
			HLP_CMD("grp <SemName> rm sem");
			  HLP_DSC("remove the semaphore <SemName> from the group it's part of");
			  HLP_CMP("GRPrmSEM()");
		  #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
			HLP_CMD("grp <MbxName> mbx callback #");
			  HLP_DSC("set the mailbox <MbxName>'s group callback function to #");
			  HLP_CMP("- none -");
		  #endif
			HLP_CMD("grp <SemName> sem callback #");
			  HLP_DSC("set the semaphore <SemName>'s group callback function to #");
			  HLP_CMP("- none -");
		}
		Result = SHELL_RES_OK_NO_MSG;
	}
	else if ((argc != 2)
	     &&  (argc != 4)
	     &&  (argc != 5)) {
		puts("usage: grp Name");
	  #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
		puts("       grp Name rm all|mbx|sem");
		puts("       grp Name mbx|sem callback #");
	  #else
		puts("       grp Name rm all|sem");
		puts("       grp Name sem callback #");
	  #endif
		Result = SHELL_RES_ERR_NO_MSG;
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 2)) {
		TaskNow = TSKgetID(argv[1]);
		if (TaskNow == (TSK_t*) NULL) {				/* CMD: grp TaskName							*/
			Serv   = "task";
			Result = SHELL_RES_NO_NAME_ARG_1;
		}
		else {
			GrpScan = TaskNow->GrpBlkOn;
			if (GrpScan == (GRP_t *)NULL) {
				puts("** None **");
			}
			else {
				puts("Group info:");
				do {
					ii = OSintOff();				/* Grab a copy to minimize possible changes		*/
					memmove(&GrpNow, GrpScan, sizeof(GrpNow));
					OSintBack(ii);					/* when displaying the information				*/

					Legend("Trigger type:");
					if (GrpNow.TrigType == OS_GRP_SEM) {
						printf("Semaphore %s\n", ((SEM_t *)(GrpNow.Trigger))->SemName);
					}
				  #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
					else if (GrpNow.TrigType == OS_GRP_MBX) {
						printf("Mailbox %s\n",  ((MBX_t *)(GrpNow.Trigger))->MbxName);
					}
				  #endif
					else {
						printf("Binary Semaphore %s\n", ((SEM_t *)(GrpNow.Trigger))->SemName);
					}
					Legend("Call back function:");
					printf(g_StrPn, GrpNow.CBfct);

					Legend("Trigger count:");
					printf("%d\n", GrpNow.TrigCnt);

					GrpScan = GrpScan->GrpNext;
				} while (GrpScan != (GRP_t *)NULL);
			}
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 4)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/
		Result  = SHELL_RES_OK;						/* Assume printing OK result					*/
		if (0 == strcmp(argv[2], "rm")) {
			if (0 == strcmp(argv[3], "all")) {		/* CMD: grp TaskName rm all						*/
				TaskNow = TSKgetID(argv[1]);
				if (TaskNow != (TSK_t *)NULL) {
					OSret  = GRPrmAll(TaskNow->GrpBlkOn);
				}
				else {								/* No tasks with the name specified				*/
					Result = SHELL_RES_NO_NAME_ARG_1;
					Serv   = "task";
				}
			}
			else if (0 == strcmp(argv[3], "mbx")) {	/* CMD: grp MbxName rm mbx						*/
			  #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
				MbxNow = FindMBX(argv[1]);
				if (MbxNow != (MBX_t *)NULL) {
					OSret  = GRPrmMBX(MbxNow);
				}
				else {								/* No mailboxes with the name specified			*/
					Result = SHELL_RES_NO_NAME_ARG_1;
					Serv   = "mailbox";
				}
			  #else
				Result = SHELL_RES_UNAVAIL_OP_ARG_3;
			  #endif
			}
			else if (0 == strcmp(argv[3], "sem")) {	/* CMD: grp SemName rm sem						*/
				SemNow = FindSEM(argv[1]);
				if (SemNow != (SEM_t *)NULL) {
					OSret  = GRPrmSEM(SemNow);
				}
				else {								/* No sempahores with the name specified		*/
					Serv   = "semaphore";
					Result = SHELL_RES_NO_NAME_ARG_1;
				}
			}
			else {
			Result = SHELL_RES_UNK_OP_ARG_3;		/* not all, mbx nor sem							*/
			}
		}
		else {
			Result = SHELL_RES_UNK_OP_ARG_2;		/* not rm										*/
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 5)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/
		if (0 == strcmp(argv[3], "callback")) {		/* grp name ??? callback #						*/
			if (0 == strcmp(argv[2], "mbx")) {		/* grp name mbx callback #						*/
			  #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
				MbxNow = FindMBX(argv[1]);
				if (MbxNow != (MBX_t *)NULL) {
					Value = strtoull(argv[4], &Cptr, 0);
					if (*Cptr == '\0') {
						MbxNow->GrpOwner->CBfct = (void (*)(intptr_t)) Value;
					}
					else {
						Result = SHELL_RES_INVAL_VAL_ARG_4;
					}
				}
				else {								/* No mailboxes with the name specified			*/
					Serv   = "mailbox";
					Result = SHELL_RES_NO_NAME_ARG_1;
				}
			  #else
				Result = SHELL_RES_UNAVAIL_OP_ARG_2;
			  #endif
			}
			else if (0 == strcmp(argv[2], "sem")) {	/* grp name sem callback #						*/
				SemNow = FindSEM(argv[1]);
				if (SemNow != (SEM_t *)NULL) {
					Value = strtoull(argv[4], &Cptr, 0);
					if (*Cptr == '\0') {
						SemNow->GrpOwner->CBfct = (void (*)(intptr_t)) Value;
					}
					else {
						Result = SHELL_RES_INVAL_VAL_ARG_4;
					}
				}
				else {								/* No semaphore with the name specified			*/
					Serv   = "semaphore";
					Result = SHELL_RES_NO_NAME_ARG_1;
				}
			}
			else {									/* not mbx or sem								*/
				Result = SHELL_RES_UNK_OP_ARG_3;
			}
		}
		else {										/* not callback									*/
			Result = SHELL_RES_UNK_OP_ARG_2;
		}
	}

	return(PrtResult(Serv, Result, OSret, argv, 0, (intptr_t)0));

#endif
}

/* ------------------------------------------------------------------------------------------------ */
/* Log facilities command : log																		*/
/*																									*/
/* Use the run-time help for a complete description of all the commands supported					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdLog(int argc, char *argv[])
{
#if ((OX_LOGGING_TYPE) <= 0)
	puts("log  : logging - not supported (OS_LOGGING_TYPE <= 0)");
	return(-1);
#else
char        *Cptr;									/* Used to know if number is malformed			*/
int          LogCmd;								/* CTRLog() command								*/
unsigned int LogOpt;								/* CTRLlog() option								*/
int          Result;								/* Result of command (to select error to print)	*/
char        *Serv = "LOG";							/* Service name									*/
int          Value;									/* Numerical value specified on command line	*/

	LogOpt = 0;
	Result = SHELL_RES_NONE;
	Value  = 0;

	if (argc < 0) {									/* Special value to print usage					*/
		puts("log  : log facilities");
		if (argc < -1) {
			puts("usage:");
			HLP_CMD("log off|on");
			  HLP_DSC("off : stop the logging facilities");
			  HLP_CMP("LOGoff()");
			  HLP_DSC("on  :start the logging facilities");
			  HLP_CMP("LOGon()");
			HLP_CMD("log disable|enable #");
			  HLP_DSC("disable : disable message number # in the filter");
			  HLP_CMP("LOGdis()");
			  HLP_DSC("enable  : enable  message number # in the filter");
			  HLP_CMP("LOGenb()");
			HLP_CMD("log all off|on");
			  HLP_DSC("off : disable all messages numbers in the filter");
			  HLP_CMP("LOGallOff()");
			  HLP_DSC("on  : enable  all messages numbers in the filter");
			  HLP_CMP("LOGallOn()");
		  #if ((OX_LOGGING_TYPE) > 1)
			HLP_CMD("log cont");
			  HLP_DSC("empty the log buffer and start continuous recording");
			  HLP_CMP("LOGcont()");
			HLP_CMD("log dump all|next");
			  HLP_DSC("all  : stop recoding and show log buffer");
			  HLP_CMP("LOGdumpAll()");
			  HLP_DSC("next : stop recoding and show and delete oldest entry");
			  HLP_CMP("LOGdumpNext()");
			HLP_CMD("log once");
			  HLP_DSC("empty the log buffer and start recoding until log buffer full");
			  HLP_CMP("LOGonce()");
		  #endif
		}
		Result = SHELL_RES_OK_NO_MSG;
	}
	else if ((argc < 2)
	     ||  (argc > 3)) {
		puts("usage: log cmd [<op>|#]");
		Result = SHELL_RES_ERR_NO_MSG;
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 2)) {
		Result = SHELL_RES_OK_NO_MSG;
		if (0 == strcmp(argv[1], "off")) { 			/* CMD: log off									*/
			LogCmd = LOG_OFF;
		}
		else if (0 == strcmp(argv[1], "on")) { 		/* CMD: log on									*/
			LogCmd = LOG_ON;
		}
		else if (0 == strcmp(argv[1], "cont")) { 	/* CMD: log cont								*/
		  #if ((OX_LOGGING_TYPE) > 1)
			LogCmd = LOG_CONT;
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_1;
		  #endif
		}
		else if (0 == strcmp(argv[1], "once")) { 	/* CMD: log once								*/
		  #if ((OX_LOGGING_TYPE) > 1)
			LogCmd = LOG_ONCE;
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_1;
		  #endif
		}
		else {
			Result = SHELL_RES_UNK_CMD_ARG_1;
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 3)) {

		Result = SHELL_RES_OK_NO_MSG;
		if ((0 == strcmp(argv[1], "disable")) 		/* log disable|enable need a #					*/
		||  (0 == strcmp(argv[1], "enable"))) {
			Value = strtol(argv[2], &Cptr, 0);
			if (*Cptr != '\0') {
				Result = SHELL_RES_INVAL_VAL_ARG_2;
			}
			else if (Value < 0) {
				puts("Value must be >= 0");
				Result = SHELL_RES_ERR_NO_MSG;
			}
			LogOpt = (unsigned int)Value;
 		}
		if (0 == strcmp(argv[1], "disable")) { 		/* CMD: log disable #							*/
			LogCmd = LOG_DISABLE;
		}
		else if (0 == strcmp(argv[1], "enable")) {	/* CMD: log enable #							*/
			LogCmd = LOG_ENABLE;
		}
		else if (0 == strcmp(argv[1], "all")) {
			if (0 == strcmp(argv[2], "off")) { 		/* CMD: log all off								*/
				LogCmd = LOG_ALL_OFF;
			}
			else if (0 == strcmp(argv[2], "on")) { 	/* CMD: log all on								*/
				LogCmd = LOG_ALL_ON;
			}
			else {
				Result = SHELL_RES_UNK_OP_ARG_2;
			}
		}
		else if (0 == strcmp(argv[1], "dump")) {
			if (0 == strcmp(argv[2], "all")) { 		/* CMD: log dump all							*/
			  #if ((OX_LOGGING_TYPE) > 1)
				LogCmd = LOG_DUMP_ALL;
			  #else
				Result = SHELL_RES_UNAVAIL_OP_ARG_2;
			  #endif
			}
			else if (0 == strcmp(argv[2], "next")) {/* CMD: log dump next							*/
			  #if ((OX_LOGGING_TYPE) > 1)
				LogCmd = LOG_DUMP_NEXT;
			  #else
				Result = SHELL_RES_UNAVAIL_OP_ARG_2;
			  #endif
			}
			else {
				Result = SHELL_RES_UNAVAIL_OP_ARG_2;
			}
		}
		else {
			Result = SHELL_RES_UNK_OP_ARG_1;
		}
	}

	if ((Result == SHELL_RES_OK_NO_MSG)
	&&  (argc > 0)) {
		LOGctrl(LogCmd, LogOpt);
	}

	return(PrtResult(Serv, Result, 0, argv, 0, (intptr_t)0));

#endif
}

/* ------------------------------------------------------------------------------------------------ */
/* Memeory block command : mblk																		*/
/*																									*/
/* Use the run-time help for a complete description of all the commands supported					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdMblk(int argc, char *argv[])
{
#if ((OX_MEM_BLOCK) == 0)
	puts("mblk : memory blocks - not supported (OS_MEM_BLOCK == 0)");
	return(-1);
#else
int        Cnt;
int        ii;
OSalign_t *MblkPtr;
MBLK_t    *MblkScan;								/* To traverse Abassi internal linked list		*/
int        Result;									/* Result of command (to select error to print)	*/
char      *Serv = "MBLK";							/* Service name									*/
int        Value;

	Result = SHELL_RES_NONE;						/* Start with no error							*/
	Value  = 0;

	if (argc < 0) {									/* Special value to print usage					*/
		puts("mblk : memory blocks information");

		if (argc < -1) {
			puts("usage:");
			HLP_CMD("mblk");
			  HLP_DSC("show all the memory blocks in the application + # free block");
		}
		Result = SHELL_RES_OK_NO_MSG;
	}
	else if (argc > 1) {
		puts("usage: mblk");
		Result = SHELL_RES_ERR_NO_MSG;
	}

	if ((argc == 1)									/* Dump all memeory blocks & # of free blocks	*/
	&&  (Result == SHELL_RES_NONE)) {
		Value = 1;
		for (ii=0 ; ii<((OX_HASH_MBLK)+1U) ; ii++) {/* Don't use MBLKopen() because it will create	*/
		  #if ((OX_HASH_MBLK) != 0U)				/* one when the mailbox doesn't exist			*/
			MblkScan = g_MblkList[ii];
		  #else
			MblkScan = g_MblkList;
		  #endif
			while (MblkScan != (MBLK_t *)NULL) {	/* Traverse the list of memeory blocks			*/
				MblkPtr = MblkScan->Block;
				for (Cnt=0 ; MblkPtr != (OSalign_t *)NULL ; Cnt++) {
					MblkPtr = *(OSalign_t **)MblkPtr;
				}
				printf("[address:");
				printf(g_StrP, MblkScan);
				printf("] (free:%3d) ", Cnt);
				printf("%s\n", MblkScan->MblkName);
				Value = 0;
				
				MblkScan = MblkScan->MblkNext;
			}
		}

		if (Value != 0) {							/* No memory blocks found						*/
			Serv   = "mem blocks";
			Result = SHELL_RES_NO_SERV;
		}
	}

	return(PrtResult(Serv, Result, 0, argv, 0, (intptr_t)0));
#endif
}

/* ------------------------------------------------------------------------------------------------ */
/* Mailbox command : mbx																			*/
/*																									*/
/* Use the run-time help for a complete description of all the commands supported					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdMbx(int argc, char *argv[])
{
#if defined(__UABASSI_H__)
	puts("mbx  : mailboxes emulated, no debug available");
	return(-1);
#elif ((OX_MAILBOX) == 0)
	puts("mbx  : mailboxes - not supported (OS_MAILBOX == 0)");
	return(-1);
#else
char    *Cptr;										/* Used to know if number is malformed			*/
int      ii;										/* General purpose								*/
MBX_t    MbxNow;									/* Snapshot of the one to display / manipulate	*/
MBX_t   *MbxScan;									/* To traverse Abassi internal linked list		*/
int	     OSret;										/* Return value from OS service call			*/
int      Result;									/* Result of command (to select error to print)	*/
char    *Serv = "MBX";								/* Service name									*/
TSK_t   *TaskScan;									/* To traverse Abassi internal linked list		*/
intptr_t Value;										/* Numerical value specified on command line	*/
intptr_t Value2;
intptr_t ValMbx;									/* value read from . written to a mailbox		*/
#if (OX_GROUP)
   GRP_t *GrpScan;
#endif

	OSret  = 0;										/* OS return is OK								*/
	Result = SHELL_RES_NONE;						/* Start with no error							*/
	Value  = 0;										/* Remove compiler warning						*/

	if (argc < 0) {									/* Special value to print usage					*/
		puts("mbx  : mailbox information / processing");

		if (argc < -1) {
			puts("usage:");
			HLP_CMD("mbx");
			  HLP_DSC("show all the mailboxes in the application");
			HLP_CMD("mbx ##");
			  HLP_DSC("dump the mailbox descriptor field memory offsets");
			HLP_CMD("mbx <MbxName>");
			  HLP_DSC("dump info on the mailbox <MbxName>");
			HLP_CMD("mbx <MbxName> dump");
			  HLP_DSC("dump the contents of the queue of the mailbox <MbxName>");
			HLP_CMD("mbx <MbxName> get");
			  HLP_DSC("retrieve a message from the mailbox <MbxName> with a timeout of 0");
			  HLP_CMP("MBXget()");
			HLP_CMD("mbx <MbxName> get #");
			  HLP_DSC("retrieve a message from the mailbox MailBox with a timeout of #");
			  HLP_CMP("MBXget()");
			HLP_CMD("mbx <MbxName> put #");
			  HLP_DSC("insert the message # in the mailbox MailBox with a timeout of 0");
			  HLP_CMP("MBXput()");
			HLP_CMD("mbx <MbxName> put # <Tout>");
			  HLP_DSC("insert the message # in the mailbox MailBox with a timeout of <Tout>");
			  HLP_CMP("MBXput()");
		  #if ((OX_WAIT_ABORT) != 0)
			HLP_CMD("mbx <MbxName> abort");
			  HLP_DSC("abort the blocking of all tasks on the mailbox <MbxName>");
			  HLP_CMP("MBXabort()");
		  #endif
		  #if ((OX_FCFS) != 0)
			HLP_CMD("mbx <MbxName> order FCFS");
			  HLP_DSC("set First-Come-First-Served unblocking order");
			  HLP_CMP("MBXsetFCFS()");
			HLP_CMD("mbx <MbxName> order prio");
			  HLP_DSC("set priority unblocking order");
			  HLP_CMP("MBXnotFCFS()");
		  #endif
		}
		Result = SHELL_RES_OK_NO_MSG;
	}
	else if (argc > 4) {
		puts("usage: mbx [MbxName [op | {property [#]]} ]");
		Result = SHELL_RES_ERR_NO_MSG;
	}

	MbxScan = (MBX_t *)NULL;
	if (Result == SHELL_RES_NONE) {	
		if ((argc > 1)
		&&  (argv[1][0] == '0')
		&&  (argv[1][1] == 'x')) {
			MbxScan = (MBX_t *)(uintptr_t)strtoul(argv[1], &Cptr, 16);
			if (*Cptr != '\0') {
				MbxScan = (MBX_t *)NULL;
			}
		}
		else {
			for (ii=0 ; ii<((OX_HASH_MBX)+1U) ; ii++) {	/* Don't use MBXopen() because will create	*/
			  #if ((OX_HASH_MBX) != 0U)				/* one when the mailbox doesn't exist			*/
				MbxScan = g_MboxList[ii];
			  #else
				MbxScan = g_MboxList;
			  #endif
				while (MbxScan != (MBX_t *)NULL) {	/* Traverse the list of mailboxes				*/
					if (argc == 1) {
						printf("[address:");
						printf(g_StrP, MbxScan);
						printf("] ");
						printf("%s\n", MbxScan->MbxName);
						Value = 1;
					}
					else {
						if (0 == strcmp(argv[1], MbxScan->MbxName)) {
							break;					/* Done											*/
						}
					}
					MbxScan = MbxScan->MbxNext;
				}
			}
		}
		if ((argc == 1)
		&&  (Value == 0)) {
			puts("No mailboxes in the application");
			Result = SHELL_RES_OK;
		}
	}

	if ((Result == SHELL_RES_NONE)
	&&  ((argc > 2)
	  ||   ((argc == 2) && (0 != strcmp("##", argv[1]))))) {
		if (MbxScan != (MBX_t *)NULL) {
			ii = OSintOff();
			memmove(&MbxNow, MbxScan, sizeof(MbxNow));
			OSintBack(ii);
		}
		else {
			Serv   = "mailbox";
			Result = SHELL_RES_NO_NAME_ARG_1;
		}
	}


	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 2)) {								/* CMD: mbx MbxName								*/
		if (0 == strcmp(argv[1], "##")) {			/* CMD: mbx ##									*/
			Legend("MboxSema:");
			printf("%d (sem ## for offsets)\n", (int)offsetof(MBX_t, MboxSema));
			Legend("Buffer:");
			printf("%d\n", (int)offsetof(MBX_t, Buffer));
		  #if (OX_NAMES)							/* Name of the queue /  mailbox					*/
			Legend("MbxName:");
			printf("%d\n", (int)offsetof(MBX_t, MbxName));
			Legend("MbxNext:");
			printf("%d\n", (int)offsetof(MBX_t, MbxNext));
		  #endif
			Legend("Size:");
			printf("%d\n", (int)offsetof(MBX_t, Size));
			Legend("RdIdx:");
			printf("%d\n", (int)offsetof(MBX_t, RdIdx));
			Legend("WrtIdx:");
			printf("%d\n", (int)offsetof(MBX_t, WrtIdx));
		  #if ((OX_MBXPUT_ISR) != 0)				/* If MBXput needs valid ret when called in ISR	*/
			Legend("PutInISR:");
			printf("%d\n", (int)offsetof(MBX_t, PutInISR));
			Legend("Count:");
			printf("%d\n", (int)offsetof(MBX_t, Count));
		   #if ((OX_N_CORE) > 1)
			Legend("MbxSpin:");
			printf("%d\n", (int)offsetof(MBX_t, MbxSpin));
		   #endif									/* but only for multi-core						*/
		  #endif
		  #if (OX_GROUP)
			Legend("GrpOwner:");
			printf("%d\n", (int)offsetof(MBX_t, GrpOwner));
		  #endif
		  #if (OX_MBX_PARKING)						/* When "destroy mailbox" is needed				*/
			Legend("ParkNext:");
			printf("%d\n", (int)offsetof(MBX_t, ParkNext));
		  #endif
		  #if (OX_MBX_XTRA_FIELD)
			Legend("XtraData:");
			printf("%d\n", (int)offsetof(MBX_t,XtraData ));
		  #endif
		}
		else {
			printf("Info on mailbox %s:\n\n", argv[1]);

			Legend("Mailbox name:");
			printf("%s\n", MbxScan->MbxName);

			Legend("Descriptor address:");
			printf(g_StrPn, MbxScan);

			Legend("Mailbox size:");
			printf("%d\n", MbxNow.Size);

			Legend("Mailbox used:");
			printf("%d\n", MBXused(&MbxNow));

			Legend("Mailbox free:");
			printf("%d\n", MBXavail(&MbxNow));

			Legend("Tasks blocked on it:");
			TaskScan = MbxNow.MboxSema.Blocked;
			if (TaskScan == (TSK_t *)NULL) {
				puts("** None **");
			}
			else {
				ii = 0;
				while(TaskScan != (TSK_t *)NULL) {	/* Traverse the list of tasks					*/
					if (ii != 0) {					/* There is always A&E so no need for info		*/
						Legend(" ");				/* about no task in the application				*/
					}
					printf("%s\n", TaskScan->TskName);
					TaskScan = TaskScan->Next;
					ii++;
				}
			}

		  #if ((OX_FCFS) != 0)
			ii = MbxNow.MboxSema.IsFCFS;
		  #else
			ii = 0;
		  #endif
			Legend("Unblocking order:");
			printf("%s\n", (ii==0) ? "Priority" : "First Come First Served");

		  #if (OX_GROUP)
			Legend("Member of group:");
			if (MbxNow.GrpOwner == (GRP_t *)NULL) {
				puts("** None **");
			}
			else {
				GrpScan = MbxNow.GrpOwner;
				while (GrpScan->GrpPrev != GrpScan) {
					GrpScan = GrpScan->GrpPrev;
				}
				printf(g_StrPn, GrpScan);
				Legend("Mailbox sub-group:");
				printf(g_StrPn, MbxNow.GrpOwner);
				Legend("Task blocked on group:");
				if (MbxNow.GrpOwner->TaskOwner == (TSK_t *)NULL) {
					puts("** None **");
				}
				else {
					printf("%s\n", MbxNow.GrpOwner->TaskOwner->TskName);
				}
			}
		  #endif
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 3)) {
		Result  = SHELL_RES_OK;
		if (0 == strcmp(argv[2], "dump")) {			/* CMD: mbx MbxName dump						*/
			printf("Contents of mailbox %s\n", argv[1]);
			Value = 0;
			ii    = MbxNow.RdIdx;
			if (--ii < 0) {
				ii = MbxNow.Size;
			}
			while (ii != MbxNow.WrtIdx) {
				printf(g_StrIPn, MbxNow.Buffer[ii]);
				if (--ii < 0) {
					ii = MbxNow.Size;
				}
				Value = 1;
			}

			if (Value == 0) {
				puts("** Empty **");
			}
			Result = SHELL_RES_NONE;			/* No result printing							*/
		}
		else if (0 == strcmp(argv[2], "get")) {	/* CMD: mbx MbxName get							*/
			if (SHELL_CHK_WRT(Result)) {		/* Writing so make sure read-write access login	*/
				ii  = MBXget(MbxScan, &ValMbx, 0);
				if (ii != 0) {					/* MBXget reports != 0 when mailbox is empty	*/
					puts("Done : ** Empty **");
				}
				else {
					Result = SHELL_RES_PRT_INTPTR;
				}
			}
		}
		else if (0 == strcmp(argv[2], "abort")) {/* CMD: mbx MbxName abort						*/
		  #if ((OX_WAIT_ABORT) != 0)
			if (SHELL_CHK_WRT(Result)) {		/* Writing so make sure read-write access login	*/
				MBXabort(MbxScan);
			}
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}
		else {									/* not "mbx MbxName dump"						*/
			Result = SHELL_RES_UNK_OP_ARG_2;	/*     "mbx MbxName get"						*/
		}										/*     "mbx MbxName abort"						*/
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 4)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/
		Result  = SHELL_RES_OK;						/* Assume printing OK result					*/

		Value = strtoll(argv[3], &Cptr, 0);

		if (0 == strcmp(argv[2], "put")) {			/* CMD: mbx MbxName put #						*/
			if (*Cptr == '\0') {
				OSret = MBXput(MbxScan, Value, 0);
			}
			else {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		}
		else if (0 == strcmp(argv[2], "get")) {		/* CMD: mbx MbxName get #Tout					*/
			if (*Cptr == '\0') {
				ii  = MBXget(MbxScan, &ValMbx, (int)Value);
				if (ii != 0) {						/* MBXget() reports != 0 when mailbox is empty	*/
					puts("Done : ** timeout **");
					Result = SHELL_RES_OK_NO_MSG;
				}
				else {
					Result = SHELL_RES_PRT_INTPTR;
				}
			}
			else {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		}
		else if (0 == strcmp(argv[2], "order")) {
			if (0 == strcmp(argv[3], "FCFS")) {		/* CMD: mbx MbxName order FCFS					*/
			  #if ((OX_FCFS) != 0)
				MBXsetFCFS(MbxScan);
			  #else
				Result = SHELL_RES_UNAVAIL_OP_ARG_2;
			  #endif
			}
			else if (0 == strcmp(argv[3], "prio")) {/* CMD: mbx MbxName order prio					*/
			  #if ((OX_FCFS) != 0)
				MBXnotFCFS(MbxScan);
			  #else
				Result = SHELL_RES_UNAVAIL_OP_ARG_2;
			  #endif
			}
			else {
				Result = SHELL_RES_UNK_OP_ARG_3;
			}
		}
		else {										/* The operation specified is not valid			*/
			Result = SHELL_RES_UNK_OP_ARG_2;
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 5)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/
		Result  = SHELL_RES_OK;						/* Assume printing OK result					*/

		Value = strtoll(argv[3], &Cptr, 0);
		if (*Cptr == '\0') {
			Value2 = strtoll(argv[4], &Cptr, 0);
			if (*Cptr != '\0') {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		}
		else {
			Result = SHELL_RES_INVAL_VAL_ARG_3;
		}

		if (0 == strcmp(argv[2], "put")) {			/* CMD: mbx MbxName put # #Tout					*/
			if (Result == SHELL_RES_OK) {
				OSret = MBXput(MbxScan, Value, (int)Value2);
			}
		}
		else {										/* The operation specified is not valid			*/
			Result = SHELL_RES_UNK_OP_ARG_2;
		}
	}

	return(PrtResult(Serv, Result, OSret, argv, Value, ValMbx));

#endif
}

/* ------------------------------------------------------------------------------------------------ */
/* Memory access command : mem																		*/
/*																									*/
/* Use the run-time help for a complete description of all the commands supported					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdMem(int argc, char *argv[])
{
volatile uint32_t *Address;							/* Address specified on the command line		*/
char     *Cptr;										/* Used to know if number is malformed			*/
int       ii;										/* General purpose								*/
int       Result;									/* Result of command (to select error to print)	*/
char     *Serv = "MEM";								/* Service name									*/
uint32_t  Value;									/* Numerical value specified on command line	*/

static char ValAsc[128];

	Result = SHELL_RES_NONE;						/* Start with no error							*/

	if (argc < 0) {									/* Special value to print usage					*/
		puts("mem  : memory information / setting");

		if (argc < -1) {
			puts("usage:");
			HLP_CMD("mem dump Address [size]");
			  HLP_DSC("Dump 32 bit memory from Address");
			  HLP_DSC("If size specified, dump size words");
			  HLP_DSC("If size not specified, dump 1 word");
			HLP_CMD("mem set  Address [#]");
			  HLP_DSC("Set 32 bit memory at Address");
			  HLP_DSC("If # specified, write word # at address");
			  HLP_DSC("If # not specified, prompt until non-numerical or empty");
		}
		Result = SHELL_RES_OK_NO_MSG;
	}
	else if ((argc < 3)
	||  (argc > 4)) {
		puts("usage: mem [{dump Addr [size]]} | {[set Addr [#]}");
		Result = SHELL_RES_ERR_NO_MSG;
	}


	Address = (uint32_t *)NULL;						/* Remove compiler warning						*/
	if ((Result == SHELL_RES_NONE)
	&&  (argc >=  3)) {
		Address = (void *)(intptr_t)strtoull(argv[2], &Cptr, 0);
		if (*Cptr != '\0') {
			Result = SHELL_RES_INVAL_VAL_ARG_2;
		}
	}

	Value = 0;										/* Remove compiler warning						*/
	if ((Result == SHELL_RES_NONE)
	&&  (argc == 4)) {
		Value = strtoll(argv[3], &Cptr, 0);
		if (*Cptr != '\0') {
			Result = SHELL_RES_INVAL_VAL_ARG_3;
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 3)) {
		if (0 == strcmp(argv[1], "dump")) {			/* CMD: mem dump address						*/
			printf(g_StrP, Address);
			printf(": ");
			printf(g_StrVn, *Address);
		}
		else if (0 == strcmp(argv[1], "set")) {		/* CMD: mem set address							*/
			if (SHELL_CHK_WRT(Result)) {			/* Writing so make sure read-write access login	*/
				do {
					printf(g_StrP, Address);
					printf(" [now:");
					printf(g_StrP, *Address);
					printf("] ? ");
					ShellGets(&ValAsc[0], sizeof(ValAsc));	/* Get the value from the user			*/

					Cptr  = &ValAsc[0];
					Value = 0;
					while (*Cptr != '\0') {			/* To trap if empty or spaces					*/
						if (*Cptr++ != ' ') {
							Value = 1;
							break;
						}
					}
					if (Value != 0) {
						Value = strtoll(&ValAsc[0], &Cptr, 0);
						if (*Cptr == ' ') {
							*Cptr = '\0';
						}
						if (*Cptr == '\0') {
							*Address = Value;
							 Address++;
						}
					}
					else {
						 Cptr = &ValAsc[0];
						*Cptr = ' ';
					}
				} while (*Cptr == '\0');
			}
		}
		else {
			Result = SHELL_RES_UNK_CMD_ARG_2;
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 4)) {
		if (0 == strcmp(argv[1], "dump")) {			/* CMD: mem dump address Ntimes					*/
			for (ii=0 ; ii<Value ; ii++) {
				printf(g_StrP, Address);
				printf(": ");
				printf(g_StrVn, *Address);
				Address++;
			}
		}
		else if (0 == strcmp(argv[1], "set")) {		/* CMD: mem set address #						*/
			if (SHELL_CHK_WRT(Result)) {			/* Writing so make sure read-write access login	*/
				*Address = Value;
				puts("Done");
			}
		}
		else {
			Result = SHELL_RES_UNK_CMD_ARG_2;
		}
	}

	return(PrtResult(Serv, Result, 0, argv, 0, 0));

}

/* ------------------------------------------------------------------------------------------------ */
/* Mutex command : mtx																				*/
/*																									*/
/* Use the run-time help for a complete description of all the commands supported					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdMtx(int argc, char *argv[])
{
char    *Cptr;										/* Used to know if number is malformed			*/
int      ii;										/* General purpose								*/
MTX_t    MtxNow;									/* Snapshot of the one to display / manipulate	*/
MTX_t   *MtxScan;									/* To traverse Abassi internal linked list		*/
int      OSret;										/* Return value from OS service call			*/
int      Result;									/* Result of command (to select error to print)	*/
char    *Serv = "MTX";								/* Service name									*/
TSK_t   *TaskScan;									/* To traverse Abassi internal linked list		*/
intptr_t Value;										/* Numerical value specified on command line	*/


	OSret  = 0;										/* OS return is OK								*/
	Result = SHELL_RES_NONE;						/* Start with no error							*/
	Value  = 0;										/* Remove compiler warning						*/

	if (argc < 0) {									/* Special value to print usage					*/
		puts("mtx  : mutex information / processing");

		if (argc < -1) {
			puts("usage:");
			HLP_CMD("mtx");
			  HLP_DSC("show all the mutexes in the application");
			HLP_CMD("mtx ##");
			  HLP_DSC("dump the mutex descriptor field memory offsets");
			HLP_CMD("mtx <MtxName>");
			  HLP_DSC("dump info on the mutex <MtxName>");
			HLP_CMD("mtx <MtxName> lock");
			  HLP_DSC("lock the mutex <MtxName> with a timeout of 0");
			  HLP_CMP("MTXlock()");
			HLP_CMD("mtx <MtxName> lock #");
			  HLP_DSC("lock the mutex <MtxName> with a timeout of #");
			  HLP_CMP("MTXlock()");
			HLP_CMD("mtx <MtxName> unlock");
			  HLP_DSC("unlock the mutex <MtxName>");
			  HLP_CMP("MTXunlock()");
		    HLP_CMD("mtx <MtxName> value #");
			  HLP_DSC("set the value of the mutex <MtxName> to #");
			  HLP_CMP("- none - direct field update");
		  #if ((OX_WAIT_ABORT) != 0)
			HLP_CMD("mtx <MtxName> abort");
			  HLP_DSC("abort the blocking of all tasks on the mutex <MtxName>");
			  HLP_CMP("MTXabort()");
		  #endif
		  #if ((OX_FCFS) != 0)
			HLP_CMD("mtx <MtxName> order FCFS");
			  HLP_DSC("set First-Come-First-Served unblocking order for the mutex <MtxName>");
			  HLP_CMP("MTXsetFCFS()");
			HLP_CMD("mtx <MtxName> order prio");
			  HLP_DSC("set priority unblocking order for the mutex <MtxName>");
			  HLP_CMP("MTXnotFCFS()");
		  #endif
		  #if ((OX_MTX_OWN_UNLOCK) < 0)
		    HLP_CMD("mtx <MtxName> unlk owner");
		      HLP_DSC("restrict the unlocking of the mutex <MtxName> to the owner only");
		      HLP_CMP("MTXchkOwn()");
		    HLP_CMD("mtx <MtxName> unlk all");
		      HLP_DSC("the unlocking of the mutex <MtxName> can be done by all tasks");
		      HLP_CMP("MTXignoreOwn()");
		  #endif
		   #if ((OX_MTX_INVERSION) < 0)
		    HLP_CMD("mtx <MtxName> ceil #");
		      HLP_DSC("set the priority ceiling value of the mutex <MtxName> to #");
		      HLP_CMP("MTXsetPrioCeil()");
		  #endif
		  #if ((OX_PRIO_INV_ON_OFF) != 0)
		    HLP_CMD("mtx <MtxName> invers off");
			  HLP_DSC("turn off priority inversion protection on the mutex <MtxName>");
			  HLP_CMP("MTXprioInvOff()");
		    HLP_CMD("mtx <MtxName> invers on");
			  HLP_DSC("turn on  priority inversion protection on the mutex <MtxName>");
			  HLP_CMP("MTXprioInvOn()");
		  #endif
		}
		Result = SHELL_RES_OK_NO_MSG;
	}
	else if (argc > 4) {
		puts("usage: mtx [[MtxName] [<op>|{<property> [#]} ]]");
		Result = SHELL_RES_ERR_NO_MSG;
	}

	MtxScan = (MTX_t *)NULL;						/* To remove compiler warning					*/
	if (Result == SHELL_RES_NONE) {	
		if ((argc > 1)
		&&  (argv[1][0] == '0')
		&&  (argv[1][1] == 'x')) {
			MtxScan = (MTX_t *)(uintptr_t)strtoul(argv[1], &Cptr, 16);
			if (*Cptr != '\0') {
				MtxScan = (MTX_t *)NULL;
			}
		}
		else {
			for (ii=0 ; ii<((OX_HASH_MUTEX)+1U) ; ii++) {
			  #if ((OX_HASH_MUTEX) != 0U)			/* Don't use MTXopen() because it will create	*/
				MtxScan = g_MutexList[ii];			/* one when the mutex doesn't exist				*/
			  #else
				MtxScan = g_MutexList;
			  #endif
				while (MtxScan != (MTX_t *)NULL) {	/* Traverse the list of mutexes					*/
					if (argc == 1) {				/* Request to dump all the info					*/
						printf("[address:");
						printf(g_StrP, MtxScan);
						printf("] ");
						printf("%s\n", MtxScan->SemName);
					}
					else {							/* Command on the mutex MtxName					*/
						if (0 == strcmp(argv[1], MtxScan->SemName)) {
							break;					/* Done											*/
						}
					}
					MtxScan = MtxScan->SemaNext;
				}
			}
		}											/* G_OSmutex always exists, no need to check	*/
	}												/* for no mutexes in the application			*/

	if ((Result == SHELL_RES_NONE)
	&&  ((argc > 2)
	  ||   ((argc == 2) && (0 != strcmp("##", argv[1]))))) {
		if (MtxScan != (MTX_t *)NULL) {
			ii = OSintOff();
			memmove(&MtxNow, MtxScan, sizeof(MtxNow));
			OSintBack(ii);
		}
		else {
			Serv   = "mutex";
			Result = SHELL_RES_NO_NAME_ARG_1;
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 2)) {								/* CMD: mtx MtxName								*/

		if (0 == strcmp(argv[1], "##")) {			/* CMD: mtx ##									*/
			Legend("Blocked:");
			printf("%d\n", (int)offsetof(SEM_t, Blocked));
			Legend("MtxOwner:");
			printf("%d\n", (int)offsetof(SEM_t, MtxOwner));
		  #if (OX_HANDLE_MTX)
			Legend("MtxNext:");
			printf("%d\n", (int)offsetof(SEM_t, MtxNext));
		  #endif
		  #if (OX_NAMES)
			Legend("SemName:");
			printf("%d\n", (int)offsetof(SEM_t, SemName));
			Legend("SemaMext:");
			printf("%d\n", (int)offsetof(SEM_t, SemaNext));
		  #endif
			Legend("Value:");
			printf("%d\n", (int)offsetof(SEM_t, Value));
		  #if (OX_FCFS)
			Legend("IsFCFS:");
			printf("%d\n", (int)offsetof(SEM_t, IsFCFS));
		  #endif
		  #if ((OX_MTX_INVERSION) < 0)
			Legend("CeilPrio:");
			printf("%d\n", (int)offsetof(SEM_t, CeilPrio));
		  #endif
		  #if (OX_PRIO_INV_ON_OFF)
			int PrioInvOn;
			Legend("PrioInvOn:");
			printf("%d\n", (int)offsetof(SEM_t, PrioInvOn));
		  #endif
		  #if ((OX_MTX_OWN_UNLOCK) < 0)
			Legend("IgnoreOwn:");
			printf("%d\n", (int)offsetof(SEM_t, IgnoreOwn));
		  #endif
		  #if (OX_GROUP)
			Legend("GrpOwner:");
			printf("%d\n", (int)offsetof(SEM_t, GrpOwner));
		   #if (OX_C_PLUS_PLUS)
			Legend("CppSemRef:");
			printf("%d\n", (int)offsetof(SEM_t, CppSemRef));
		   #endif
		  #endif
		  #if (OX_SEM_PARKING)
			Legend("ParkNext:");
			printf("%d\n", (int)offsetof(SEM_t, ParkNext));
		  #endif
		  #if (OX_SEM_XTRA_FIELD)
			intptr_t XtraData[OX_SEM_XTRA_FIELD];
			Legend("XtraData:");
			printf("%d\n", (int)offsetof(SEM_t, XtraData));
		  #endif
		}
		else {										/* CMD: mtx MtxName								*/
			printf("Info on mutex %s:\n\n", argv[1]);

			Legend("Mutex name:");
			printf("%s\n", MtxScan->SemName);

			Legend("Descriptor address:");
			printf(g_StrPn, MtxScan);

			Legend("Count value:");
			printf("%d\n", MtxNow.Value);

			Legend("Mutex locker:");
			if (MtxNow.MtxOwner == (TSK_t *)NULL) {
				printf("** None **\n");
			}
			else {
				printf("%s\n", MtxNow.MtxOwner->TskName);
			}

			Legend("Tasks blocked on it:");
			TaskScan = MtxNow.Blocked;
			if (TaskScan == (TSK_t *)NULL) {
				puts("** None **");
			}
			else {
				ii = 0;
				while(TaskScan != (TSK_t *)NULL) {	/* Traverse the list of tasks					*/
					if (ii != 0) {
						Legend(" ");
					}
					printf("%s\n", TaskScan->TskName);
					TaskScan = TaskScan->Next;
					ii++;
				}
			}

		  #if ((OX_FCFS) != 0)
			ii = MtxNow.IsFCFS;
		  #else
			ii = 0;
		  #endif
			Legend("Unblocking order:");
			printf("%s\n", (ii==0) ? "Priority" : "First Come First Served");

			Legend("Priority inversion protect:");
		  #if ((OX_MTX_INVERSION) == 0)
			printf("** None **\n");
		  #else
		   #if ((OX_MTX_INVERSION) < 0)
			printf("Ceiling\n");

			Legend("Ceiling priority:");
			printf("%d\n", MtxNow.CeilPrio);
		   #else
			printf("Inheritance\n");
		   #endif
		   #if (OX_PRIO_INV_ON_OFF)
			Legend("Prio inversion:");
			printf("%s\n", (MtxNow.PrioInvOn == 0) ? "Disable" : "Enable");
		   #endif
		  #endif

		  #if ((OX_MTX_OWN_UNLOCK) < 0)
			ii = MtxNow.IgnoreOwn;
		  #else
			ii = 1;
		  #endif
			Legend("Mutex unlocker:");
			printf("%s\n", (ii == 0) ? "Owner only" : "All tasks");
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 3)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/

		Result  = SHELL_RES_OK;						/* Assume printing OK result					*/
		if (0 == strcmp(argv[2], "unlock")) {		/* CMD: mtx MtxName unlock						*/
			OSret = MTXunlock(MtxScan);
		}
		else if (0 == strcmp(argv[2], "lock")) {	/* CMD: mtx MtxName lock						*/
			OSret = MTXlock(MtxScan, 0);
		}
		else if (0 == strcmp(argv[2], "abort")) {	/* CMD: mtx MtxName abort						*/
		  #if ((OX_WAIT_ABORT) != 0)
			MTXabort(MtxScan);
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}
		else {
			Result = SHELL_RES_UNK_OP_ARG_2;
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 4)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/
		Result  = SHELL_RES_OK;						/* Assume printing OK result					*/

		Value = strtoll(argv[3], &Cptr, 0);
		if (0 == strcmp(argv[2], "lock")) {			/* CMD: mtx MtxName lock #Tout					*/
			if (*Cptr == '\0') {
				ii = MTXlock(MtxScan, Value);
				if (ii != 0) {						/* MTXlock reports != 0 when timeout			*/
					puts("Done : ** timeout **");
					Result = SHELL_RES_OK_NO_MSG;
				}
			}
			else {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		}
		else if (0 == strcmp(argv[2], "ceil")) {	/* CMD: mtx MtxName CeilPrio #					*/
		  #if ((OX_MTX_INVERSION) < 0)
			if (*Cptr == '\0') {
				if (Value >= 0) {
					MtxScan->CeilPrio = Value;
				}
				else {
					Result = SHELL_RES_OUT_RANGE_ARG_3;
				}
			}
			else {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}
		else if (0 == strcmp(argv[2], "invers")) {	/* CMD: mtx MtxName invers ???					*/
		  #if ((OX_PRIO_INV_ON_OFF) != 0)
			if (0 == strcmp(argv[3], "off")) {		/* CMD: mtx MtxName invers off					*/
				MTXprioInvOff(MtxScan);
			}
			else if (0 == strcmp(argv[3], "on")) {	/* CMD: mtx MtxName invers on					*/
				MTXprioInvOff(MtxScan);
			}
			else {
				Result = SHELL_RES_UNK_FLD_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}
		else if (0 == strcmp(argv[2], "unlk")) {	/* CMD: mtx MtxName unlk ???					*/
		  #if ((OX_MTX_OWN_UNLOCK) < 0)
			if (0 == strcmp(argv[3], "owner")) {	/* CMD: mtx MtxName unlk owner					*/
				MTXprioInvOff(MtxScan);
			}
			else if (0 == strcmp(argv[3], "all")) {	/* CMD: mtx MtxName unlk all					*/
				MTXprioInvOff(MtxScan);
			}
			else {
				Result = SHELL_RES_UNK_FLD_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}
		else if (0 == strcmp(argv[2], "order")) {	/* CMD: mtx MtxName order ???					*/
		  #if ((OX_FCFS) != 0)
			if (0 == strcmp(argv[3], "prio")) {		/* CMD: mtx MtxName order prio					*/
				MTXnotFCFS(MtxScan);
			}
			else if (0 == strcmp(argv[3], "FCFS")) {/* CMD: mtx MtxName order FCFS					*/
				MTXsetFCFS(MtxScan);
			}
			else {
				Result = SHELL_RES_UNK_FLD_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}
		else if (0 == strcmp(argv[2], "value")) {	/* CMD: mtx MtxName value #						*/
			if (*Cptr == '\0') {
				MtxScan->Value = Value;
			}
			else {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		}
		else {
			Result = SHELL_RES_UNK_OP_ARG_2;
		}
	}

	return(PrtResult(Serv, Result, OSret, argv, Value, (intptr_t)0));

}

/* ------------------------------------------------------------------------------------------------ */
/* Semaphore command : sem																			*/
/*																									*/
/* Use the run-time help for a complete description of all the commands supported					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdSem(int argc, char *argv[])
{
char    *Cptr;										/* Used to know if number is malformed			*/
int      ii;										/* General purpose								*/
int	     OSret;										/* Return value from OS service call			*/
int      Result;									/* Result of command (to select error to print)	*/
SEM_t    SemNow;									/* Snapshot of the one to display / manipulate	*/
SEM_t   *SemScan;									/* To traverse Abassi internal linked list		*/
char    *Serv = "SEM";								/* Service name									*/
TSK_t   *TaskScan;									/* To traverse Abassi internal linked list		*/
long     Value;										/* Numerical value specified on command line	*/
#if (OX_GROUP)
   GRP_t *GrpScan;
#endif
	OSret  = 0;										/* OS return is OK								*/
	Result = SHELL_RES_NONE;						/* Start with no error							*/
	Value  = 0;										/* Remove compiler warning						*/

	if (argc < 0) {									/* Special value to print usage					*/
		puts("sem  : semaphore information / processing");

		if (argc < -1) {
			puts("usage:");
			HLP_CMD("sem");
			  HLP_DSC("show all the semaphores in the application");
			HLP_CMD("sem ##");
			  HLP_DSC("dump the semaphore descriptor field memory offsets");
			HLP_CMD("sem <SemName>");
			  HLP_DSC("dump info on the semaphore <SemName>");
			HLP_CMD("sem <SemName> post");
			  HLP_DSC("post the semaphore <SemName>");
			  HLP_CMP("SEMpost()");
			HLP_CMD("sem <SemName> wait");
			  HLP_DSC("wait on the semaphore <SemName> with a timeout of 0");
			  HLP_CMP("SEMwait()");
			HLP_CMD("sem <SemName> wait #");
			  HLP_DSC("wait on the semaphore <SemName> with a timeout of #");
			  HLP_CMP("SemWait()");
		    HLP_CMD("sem <SemName> reset");
			  HLP_DSC("reset the count of the semaphore <SemName>");
			  HLP_CMP("SEMreset()");
		    HLP_CMD("sem <SemName> value #");
			  HLP_DSC("set the count of the semaphore <SemName> to #");
			  HLP_CMP("- none - direct field update");
		  #if ((OX_WAIT_ABORT) != 0)
			HLP_CMD("sem <SemName> abort");
			  HLP_DSC("abort the blocking of all tasks on the semaphore <SemName>");
			  HLP_CMP("SEMabort()");
		  #endif
		  #if ((OX_FCFS) != 0)
			HLP_CMD("sem <SemName> order FCFS");
			  HLP_DSC("set First-Come-First-Served unblocking order for semaphore <SemName>");
			  HLP_CMP("SEMsetFCFS()");
			HLP_CMD("sem <SemName> order prio");
			  HLP_DSC("set priority unblocking order for the semaphore <SemName>");
			  HLP_CMP("SEMnotFCFS()");
		  #endif
		}
		Result = SHELL_RES_OK_NO_MSG;
	}
	else if (argc > 4) {
		puts("usage: sem [[SemName] [<op>|{<property> [#]} ]]");
		Result = SHELL_RES_ERR_NO_MSG;
	}

	SemScan = (SEM_t *)NULL;						/* To remove compiler warning					*/
	if (Result == SHELL_RES_NONE) {	
		if ((argc > 1)
		&&  (argv[1][0] == '0')
		&&  (argv[1][1] == 'x')) {
			SemScan = (SEM_t *)(uintptr_t)strtoul(argv[1], &Cptr, 16);
			if (*Cptr != '\0') {
				SemScan = (SEM_t *)NULL;
			}
		}
		else {
			for (ii=0 ; ii<((OX_HASH_SEMA)+1U) ; ii++) {
			  #if ((OX_HASH_SEMA) != 0U)			/* Don't use MTXopen() because it will create	*/
				SemScan = g_SemaList[ii];			/* one when the mutex doesn't exist				*/
			  #else
				SemScan = g_SemaList;
			  #endif
				while (SemScan != (MTX_t *)NULL) {	/* Traverse the list of mutexes					*/
					if (argc == 1) {				/* Request to dump all the info					*/
						printf("[address:");
						printf(g_StrP, SemScan);
						printf("] ");
						printf("%s\n", SemScan->SemName);
					}
					else {							/* Command on the mutex MtxName					*/
						if (0 == strcmp(argv[1], SemScan->SemName)) {
							break;					/* Done											*/
						}
					}
					SemScan = SemScan->SemaNext;
				}
			}
		}											/* G_OSmutex always exists, no need to check	*/
	}												/* for no mutexes in the application			*/

	if ((Result == SHELL_RES_NONE)
	&&  ((argc > 2)
	  ||   ((argc == 2) && (0 != strcmp("##", argv[1]))))) {
		SemScan = FindSEM(argv[1]);
		if (SemScan != (SEM_t *)NULL) {				/* Grab a copy to minimize possible changes		*/
			ii = OSintOff();						/* when displaying the information				*/
			memmove(&SemNow, SemScan, sizeof(SemNow));
			OSintBack(ii);
		}
		else {
			Serv   = "semaphore";
			Result = SHELL_RES_NO_NAME_ARG_1;
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 2)) {								/* CMD: sem ##|SemName							*/

		if (0 == strcmp(argv[1], "##")) {			/* CMD: sem ##									*/
			Legend("Blocked:");
			printf("%d\n", (int)offsetof(SEM_t, Blocked));
			Legend("MtxOwner:");
			printf("%d\n", (int)offsetof(SEM_t, MtxOwner));
		  #if (OX_HANDLE_MTX)
			Legend("MtxNext:");
			printf("%d\n", (int)offsetof(SEM_t, MtxNext));
		  #endif
		  #if (OX_NAMES)
			Legend("SemName:");
			printf("%d\n", (int)offsetof(SEM_t, SemName));
			Legend("SemaMext:");
			printf("%d\n", (int)offsetof(SEM_t, SemaNext));
		  #endif
			Legend("Value:");
			printf("%d\n", (int)offsetof(SEM_t, Value));
		  #if (OX_FCFS)
			Legend("IsFCFS:");
			printf("%d\n", (int)offsetof(SEM_t, IsFCFS));
		  #endif
		  #if ((OX_MTX_INVERSION) < 0)
			Legend("CeilPrio:");
			printf("%d\n", (int)offsetof(SEM_t, CeilPrio));
		  #endif
		  #if (OX_PRIO_INV_ON_OFF)
			int PrioInvOn;
			Legend("PrioInvOn:");
			printf("%d\n", (int)offsetof(SEM_t, PrioInvOn));
		  #endif
		  #if ((OX_MTX_OWN_UNLOCK) < 0)
			Legend("IgnoreOwn:");
			printf("%d\n", (int)offsetof(SEM_t, IgnoreOwn));
		  #endif
		  #if (OX_GROUP)
			Legend("GrpOwner:");
			printf("%d\n", (int)offsetof(SEM_t, GrpOwner));
		   #if (OX_C_PLUS_PLUS)
			Legend("CppSemRef:");
			printf("%d\n", (int)offsetof(SEM_t, CppSemRef));
		   #endif
		  #endif
		  #if (OX_SEM_PARKING)
			Legend("ParkNext:");
			printf("%d\n", (int)offsetof(SEM_t, ParkNext));
		  #endif
		  #if (OX_SEM_XTRA_FIELD)
			intptr_t XtraData[OX_SEM_XTRA_FIELD];
			Legend("XtraData:");
			printf("%d\n", (int)offsetof(SEM_t, XtraData));
		  #endif
		}
		else {										/* CMD: sem SemName								*/
			printf("Info on semaphore %s:\n\n", argv[1]);

			Legend("Semaphore name:");
			printf("%s\n", SemScan->SemName);

			Legend("Descriptor address:");
			printf(g_StrPn, SemScan);

			Legend("Count value:");
			printf("%d\n", SemNow.Value);

			Legend("Tasks blocked on it:");
			TaskScan = SemNow.Blocked;
			if (TaskScan == (TSK_t *)NULL) {
				puts("** None **");
			}
			else {
				ii = 0;
				while(TaskScan != (TSK_t *)NULL) {	/* Traverse the list of tasks					*/
					if (ii != 0) {
						Legend(" ");
					}
					printf("%s\n", TaskScan->TskName);
					TaskScan = TaskScan->Next;
					ii++;
				}
			}

		  #if ((OX_FCFS) != 0)
			ii = SemNow.IsFCFS;
		  #else
			ii = 0;
		  #endif
			Legend("Unblocking order:");
			printf("%s\n", (ii==0) ? "Priority" : "First Come First Served");

		  #if (OX_GROUP)
			Legend("Member of group:");
			if (SemNow.GrpOwner == (GRP_t *)NULL) {
				puts("** None **");
			}
			else {
				GrpScan = SemNow.GrpOwner;
				while (GrpScan->GrpPrev != GrpScan) {
					GrpScan = GrpScan->GrpPrev;
				}
				printf(g_StrPn, GrpScan);
				Legend("Semaphore sub-group:");
				printf(g_StrPn, SemNow.GrpOwner);
				Legend("Task blocked on group:");
				if (SemNow.GrpOwner->TaskOwner == (TSK_t *)NULL) {
					puts("** None **");
				}
				else {
					printf("%s\n", SemNow.GrpOwner->TaskOwner->TskName);
				}
			}
		  #endif
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 3)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/

		Result  = SHELL_RES_OK;						/* Assume printing OK result					*/
		if (0 == strcmp(argv[2], "post")) {			/* CMD: sem SemName post						*/
			SEMpost(SemScan);
		}
		else if (0 == strcmp(argv[2], "lock")) {	/* CMD: sem SemName wait						*/
			OSret = SEMwait(SemScan, 0);
		}
		else if (0 == strcmp(argv[2], "abort")) {	/* CMD: sem SemName abort						*/
		  #if ((OX_WAIT_ABORT) != 0)
			SEMabort(SemScan);
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}
		else {
			Result = SHELL_RES_UNK_OP_ARG_2;
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 4)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/
		Result  = SHELL_RES_OK;						/* Assume printing OK result					*/

		Value = strtoll(argv[3], &Cptr, 0);
		if (0 == strcmp(argv[2], "wait")) {			/* CMD: sem SemName wait #Tout					*/
			if (*Cptr == '\0') {
				ii = SEMwait(SemScan, Value);
				if (ii != 0) {						/* SEMlock reports != 0 when timeout			*/
					puts("Done : ** timeout **");
					Result = SHELL_RES_OK_NO_MSG;
				}
			}
			else {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		}
		else if (0 == strcmp(argv[2], "order")) {	/* CMD: sem SemName order ???					*/
		  #if ((OX_FCFS) != 0)
			if (0 == strcmp(argv[3], "prio")) {		/* CMD: sem SemName order prio					*/
				SEMnotFCFS(SemScan);
			}
			else if (0 == strcmp(argv[3], "FCFS")) {/* CMD: sem SemName order FCFS					*/
				SEMsetFCFS(SemScan);
			}
			else {
				Result = SHELL_RES_UNK_FLD_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}
		else if (0 == strcmp(argv[2], "value")) {	/* CMD: sem SemName value #						*/
			if (*Cptr == '\0') {
				SemScan->Value = Value;
			}
			else {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		}
		else {
			Result = SHELL_RES_UNK_OP_ARG_2;
		}
	}

	return(PrtResult(Serv, Result, OSret, argv, Value, (intptr_t)0));

}

/* ------------------------------------------------------------------------------------------------ */
/* System information : sys																			*/
/*																									*/
/* sys																								*/
/*		Dump all system related information															*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdSys(int argc, char *argv[])
{
int	     OSret;										/* Return value from OS service call			*/
int      Result;									/* Result of command (to select error to print)	*/
char    *Serv = "SYS";								/* Service name									*/
#if (defined(__MABASSI_H__) || defined(__UABASSI_H__))
  int ii;											/* General purpose								*/
#endif

	OSret  = 0;										/* OS return is OK								*/
	Result = SHELL_RES_NONE;						/* Start with no error							*/

	if (argc < 0) {									/* Special value to print usage					*/
		puts("sys  : system information");

		if (argc < -1) {
			puts("usage:");
			HLP_CMD("sys");
			HLP_DSC("dump all system information");
		}
		Result = SHELL_RES_OK_NO_MSG;
	}
	else if (argc != 1) {
		puts("usage: sys");
		Result = SHELL_RES_ERR_NO_MSG;
	}


	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 1)) {
	  #if (defined(__MABASSI_H__) || defined(__UABASSI_H__))
		for(ii=0 ; ii<OX_N_CORE ; ii++) {
			if (ii == 0) {
				Legend("Running tasks:");
			}
			else {
				Legend(" ");
			}
			printf("Core %d: %s\n", ii, G_OStaskNow[ii]->TskName);
			Legend(" ");
			printf("  Kernel in ISR: %s\n", (G_OSisrState[0][ii] == 0) ? "no" : "yes");
			Legend(" ");
			printf("  Kernel ISR requests: %s\n", (G_OSisrState[1][ii] != 0) ? "no" : "yes");
			Legend(" ");
			printf("  Kernel forced entry: %s\n", (G_OSisrState[2][ii] == 0) ? "no" : "yes");
			Legend(" ");
			printf("  Kernel running: %s\n", (G_OSisrState[3][ii] == 0) ? "no" : "yes");
		}
	  #endif

		Legend("ISRs");
	  #if ((OS_N_CORE) > 1)
		printf("%s on core #%d\n", (OSisrInfo()==0) ? "diabled" : "enabled", COREgetID());
	  #else
		printf("%s\n", (OSisrInfo()==0) ? "diabled" : "enabled");
	  #endif

	  #if ((OX_TIMER_US) != 0)
		Legend("OS timer tick count:");
		printf("%d\n", G_OStimCnt);
	  #endif

	  #if ((OX_ALLOC_SIZE) != 0)
		Legend("OSalloc unused");
		printf("%d bytes\n", OSallocAvail());
	  #endif

	}

	return(PrtResult(Serv, Result, OSret, argv, 0, (intptr_t)0));
}

/* ------------------------------------------------------------------------------------------------ */
/* task command : task																				*/
/*																									*/
/* Use the run-time help for a complete description of all the commands supported					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdTask(int argc, char *argv[])
{
char    *Cptr;										/* Used to know if number is malformed			*/
int      ii;										/* General purpose								*/
int	     OSret;										/* Return value from OS service call			*/
int      Result;									/* Result of command (to select error to print)	*/
SEM_t   *SemScan;									/* To know sem if for what service				*/
char    *Serv = "TSK";								/* Service name									*/
TSK_t    TaskNow;									/* Snapshot of the one to display / manipulate	*/
TSK_t   *TaskScan;									/* To traverse Abassi internal linked list		*/
int      Value;										/* Numerical value specified on command line	*/

#if (OX_HANDLE_MTX)
  MTX_t   *MtxNow;									/* Snapshot of the one to display / manipulate	*/
#endif
#if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
  MBX_t   *MbxScan;									/* To know sem if for what service				*/
#endif
#if (OX_STACK_CHECK)
  uint64_t LL_1;
  uint64_t LL_2;
#endif
#if ((OX_PERF_MON) != 0)
  double             Dtmp;
  int                jj;
  unsigned long long MaxVal;
  OSperfMon_t        PMdiff;						/* Time diference								*/
  OSperfMon_t        PMtimeNow;
 #if ((OX_N_CORE) > 1)
  OSperfMon_t RunTotal;
  OSperfMon_t RunTmp;
 #endif
#endif

	OSret  = 0;										/* OS return is OK								*/
	Result = SHELL_RES_NONE;						/* Assume no result printout					*/
	Value  = 0;										/* To remove compiler warning					*/

	if (argc < 0) {									/* Special value to print usage					*/
		puts("task : task information / processing");

		if (argc < -1) {
			puts("usage:");
			HLP_CMD("task");
			  HLP_DSC("show all the tasks in the application");
			HLP_CMD("task <TaskName>");
			  HLP_DSC("dump info on the task <TaskName>");
		  #if ((OX_TASK_SUSPEND) != 0)
			HLP_CMD("task <TaskName> resume");
			  HLP_DSC("resume the task <TaskName>");
			  HLP_CMP("TSKresume()");
		  #endif
		  #if ((OX_TASK_SUSPEND) != 0)
			HLP_CMD("task <TaskName> suspend");
			  HLP_DSC("suspend the task <TaskName>");
			  HLP_CMP("TSKsuspend()");
		  #endif
		  #if (OX_USE_TASK_ARG)
			HLP_CMD("task <TaskName> arg #");
			  HLP_DSC("set the task argument of the task <TaskName> to #");
			  HLP_CMP("TSKsetArg()");
		  #endif
		  #if ((OX_PRIO_CHANGE) != 0)
			HLP_CMD("task <TaskName> prio #");
			  HLP_DSC("set the priority of the task <TaskName> to #");
			  HLP_CMP("TSKsetPrio()");
		  #endif
		  #if ((OX_TIMEOUT) > 0)
			HLP_CMD("task <TaskName> tout kill");
			  HLP_DSC("abort the timed blocking of the task <TaskName>");
			  HLP_CMP("TSKtimeoutKill()");
			HLP_CMD("task <TaskName> set tout #");
			  HLP_DSC("set the timed blocking of the task <TaskName> #");
			  HLP_CMP("TSKtout()");
		  #endif
		  #if (((OX_EVENTS) != 0) && !defined(__UABASSI_H__))
			HLP_CMD("task <TaskName> evt acc #");
			  HLP_DSC("set the cummulative event flags of the task <TaskName> to #");
			  HLP_CMP("- none - direct field update");
			HLP_CMD("task <TaskName> evt and #");
			  HLP_DSC("set the AND event flags mask of the task <TaskName> to #");
			  HLP_CMP("- none - direct field update");
			HLP_CMD("task <TaskName> evt or #");
			  HLP_DSC("set the OR  event flags mask of the task <TaskName> to #");
			  HLP_CMP("- none - direct field update");
			HLP_CMD("task <TaskName> evt rx #");
			  HLP_DSC("set the received event flags of the task <TaskName> to #");
			  HLP_CMP("- none - direct field update");
		  #endif
		  #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
			HLP_CMD("task <TaskName> mbox msg #");
			  HLP_DSC("set the pending mailbox message of the task <TaskName> to #");
			  HLP_CMP("- none - direct field update");
		  #endif
		  #if ((OX_ROUND_ROBIN) < 0)
			HLP_CMD("task <TaskName> RR off");
			  HLP_DSC("turn off round robin operation for the task <TaskName>");
			  HLP_CMP("TSKsetRR()");
		  #endif
		  #if (OX_ROUND_ROBIN)
			HLP_CMD("task <TaskName> RR count #");
			  HLP_DSC("set the current round robin time left of the task <TaskName> to #");
			  HLP_CMP("- none - direct field update");
		  #endif
		  #if ((OX_ROUND_ROBIN) < 0)
			HLP_CMD("task <TaskName> RR slice #");
			  HLP_DSC("set the current round robin slice time of the task <TaskName> to #");
			  HLP_CMP("TSKsetRR()");
		  #endif
		  #if ((OX_STARVE_PRIO) < 0)
			HLP_CMD("task <TaskName> strv off");
			  HLP_DSC("turn off starvation protection for the task <TaskName>");
			  HLP_CMP("TSKsetStrvPrio()");
		  #endif
		  #if ((OX_STARVE_PRIO) < 0)
			HLP_CMD("task <TaskName> strv prio #");
			  HLP_DSC("set max prio starvation protection of the task <TaskName> to #");
			  HLP_CMP("TSKsetStrvPrio()");
		  #endif
		  #if ((OX_STARVE_RUN_MAX) < 0)
			HLP_CMD("task <TaskName> strv run #");
			  HLP_DSC("set max starvation protection run time of the task <TaskName> to #");
			  HLP_CMP("TSKsetStrvRunMax()");
		  #endif
		  #if ((OX_STARVE_WAIT_MAX) < 0)
			HLP_CMD("task <TaskName> strv wait #");
			  HLP_DSC("set max  starvation protection wait time of the task <TaskName> to #");
			  HLP_CMP("TSKsetStrvWaitMax()");
		  #endif
		  #if ((OX_MP_TYPE) & 4U)
			HLP_CMD("task <TaskName> set core #");
			  HLP_DSC("set the target core of the task <TaskName> to core #");
			  HLP_CMP("TSKsetCore()");
			HLP_CMD("task <TaskName> set any core");
			  HLP_DSC("set the task <TaskName> to run on any core ");
			  HLP_CMP("TSKsetCore()");
		  #endif
		  #if ((OX_PERF_MON) != 0)
			HLP_CMD("task <TaskName> stat");
			  HLP_DSC("print all performance montoring stats");
			HLP_CMD("task <TaskName> stat restart");
			  HLP_DSC("restart the stats collection of a task");
			  HLP_CMP("PMrestart()");
			HLP_CMD("task <TaskName> stat stop");
			  HLP_DSC("stop the stats collection of a task");
			  HLP_CMP("PMstop()");
			HLP_CMD("task * stat restart");
			  HLP_DSC("restart the stats collection of all tasks");
			  HLP_CMP("PMrestart()");
			HLP_CMD("task * stat stop");
			  HLP_DSC("stop the stats collection of all tasks");
			  HLP_CMP("PMstop()");
		  #endif
		}
		Result = SHELL_RES_OK_NO_MSG;
	}
	else if (argc > 5) {
		puts("usage: task [[TaskName] [<op>|{<property> [#]} ]]");
		Result = SHELL_RES_ERR_NO_MSG;
	}

  #if ((OX_PERF_MON) != 0)
	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 4)									/* CMD: task * stat restart						*/
	&&  (0 == strcmp(argv[1], "*"))
	&&  (0 == strcmp(argv[2], "stat"))
	&&  ((0 == strcmp(argv[3], "restart"))
	  || (0 == strcmp(argv[3], "stop")))) {
		jj = strcmp(argv[3], "stop");
		for (ii=0 ; ii<((OX_HASH_TASK)+1U) ; ii++) {
		  #if ((OX_HASH_TASK) != 0U)
			TaskScan = g_TaskList[ii];
		  #else
			TaskScan = g_TaskList;
		  #endif
			while (TaskScan != (TSK_t *)NULL) {	/* Traverse the list of tasks					*/
				if (jj == 0) {
					PMstop(TaskScan);
				}
				else {
					PMrestart(TaskScan);
				}
				TaskScan = TaskScan->TaskNext;
			}
		}											/* A&E always exists, no need to check for no	*/
		Result = SHELL_RES_OK;						/* tasks in the application						*/
	}
  #endif

	TaskScan = (TSK_t *)NULL;						/* To remove compiler warning					*/
  #if ((OX_PERF_MON) != 0)
	PMtimeNow = OX_PERF_MON_TIME();					/* Idem											*/
  #endif
	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 1)) {								/* CMD: task									*/
		for (ii=0 ; ii<((OX_HASH_TASK)+1U) ; ii++) {
		  #if ((OX_HASH_TASK) != 0U)
			TaskScan = g_TaskList[ii];
		  #else
			TaskScan = g_TaskList;
		  #endif
			while (TaskScan != (TSK_t *)NULL) {		/* Traverse the list of tasks					*/
				printf("[address:");
				printf(g_StrP, TaskScan);
				printf("] ");
			 #if ((OX_PERF_MON) != 0)
				for (jj=16384 ; jj>0; jj--) {		/* Use a local copy to not have weird stats		*/
					TaskScan->PMsnapshot = 0;		/* due to update during copying					*/
					if ((TaskScan->PMupdating == 0)
					&&  (TaskScan->PMsnapshot == 0)) {
						memmove(&TaskNow, TaskScan, sizeof(TaskNow));
						PMtimeNow = OX_PERF_MON_TIME();
						if (TaskScan->PMsnapshot == 0) {
							break;
						}
					}
				}
				if (jj <= 0) {						/* If <= couldn't take a static copy			*/
					memmove(&TaskNow, TaskScan, sizeof(TaskNow));
					PMtimeNow = OX_PERF_MON_TIME();
				}

				RunTotal = 0;						/* Print the total CPU usage (sum on all cores	*/
				for (jj=0 ; jj<(OX_N_CORE) ; jj++) {
					RunTotal += TaskNow.PMcumul[jj];
				}
				if ((TaskScan == TSKmyID())			/* It's me, I am running						*/
			  #if ((OX_N_CORE) > 1)
				||  (TaskNow.MyCore >= 0)			/* Me or runnign on another core				*/
			  #endif
				) {
					RunTotal += (PMtimeNow			/* Add time since it started running			*/
					          - TaskNow.PMrunStrt) / OX_PERF_MON_DIV;
				}
				PMdiff = (((TaskNow.PMcontrol != (OX_PM_CTRL_STOP))
				          ? PMtimeNow				/* Total time spanned since restart				*/
				          : TaskNow.PMlastTick)		/* When stopped, use the time spanned between	*/
				          - TaskNow.PMstartTick)	/* restart and stopping							*/
				       / OX_PERF_MON_DIV;
				if (PMdiff == 0) {
					printf("[%5.1lf%% CPU] ", 0.0);
				}
				else {
					Dtmp = 100.0*((double)RunTotal/(double)PMdiff);
					if (Dtmp < 0.0) {				/* Sometimes -0.0% is printed					*/
						Dtmp = 0.0;
					}
					printf("[%5.1lf%% CPU] ", Dtmp);
				}
			  #endif

				printf("%s\n", TaskScan->TskName);

				TaskScan = TaskScan->TaskNext;
			}
		}											/* A&E always exists, no need to check for no	*/
		Result = SHELL_RES_OK;						/* tasks in the application						*/
	}


	if ((Result == SHELL_RES_NONE)
	&&  (argc >= 2)) {
		if ((argv[1][0] == '0')
		&&  (argv[1][1] == 'x')) {
			TaskScan = (TSK_t *)(uintptr_t)strtoul(argv[1], &Cptr, 16);
			if (*Cptr == '\0') {
				ii = OSintOff();					/* when displaying the information				*/
				memmove(&TaskNow, TaskScan, sizeof(TaskNow));
				OSintBack(ii);
			}
			else {
				Serv   = "task";
				Result = SHELL_RES_NO_NAME_ARG_1;
			}
		}
		else {
			TaskScan = TSKgetID(argv[1]);
			if (TaskScan != (TSK_t *)NULL) {		/* Grab a copy to minimize possible changes		*/
				ii = OSintOff();					/* when displaying the information				*/
				memmove(&TaskNow, TaskScan, sizeof(TaskNow));
				OSintBack(ii);
			}
			else {
				Serv   = "task";
				Result = SHELL_RES_NO_NAME_ARG_1;
			}
		}
	}

	if ((Result == SHELL_RES_NONE)
	&&  (argc == 2)) {
		printf("Info on task %s:\n\n", argv[1]);

		Legend("Task name:");
		printf("%s\n", TaskScan->TskName);

		Legend("Descriptor address:");
		printf(g_StrPn, TaskScan);

		Legend("Private Semaphore address:");
		printf(g_StrPn, &TaskScan->Priv);

		Legend("Current stack:");
		if (TaskNow.Stack == (void *)NULL) {
			printf("A&E - not yet preempted\n");
		}
		else {
			printf(g_StrPn, TaskNow.Stack);
		}
		Legend("Priority:");
		printf("%d\n", TaskNow.Prio);

	  #if ((OX_MTX_INVERSION) != 0)
		Legend("Prio before MTX protection:");
		if (TaskNow.OriPrio != TaskNow.Prio) {
			printf("%d\n", TaskNow.OriPrio);
		}
		else {
			printf("No priority change\n");
		}
	  #endif

		Legend("State:");
		if (TSKisSusp(TaskScan)) {					/* Can't use &TaskNow for suspended				*/
			printf("Suspended\n");
		}
		else if (TSKisRdy(&TaskNow)) {
			puts("Ready");
		}
		else if (TSKisBlk(&TaskNow)) {
			puts("Blocked");
			Legend("Blocker address:");
			printf(g_StrPn, TaskNow.Blocker);

			SemScan = (SEM_t *)NULL;
			Legend("Blocked on:");
			if (TaskNow.Blocker->MtxOwner != (TSK_t *)NULL) {
				printf("%s (Mutex) ", TaskNow.Blocker->SemName);
				SemScan = (SEM_t *)1;
			}
			if ((SemScan == (SEM_t *)NULL)
			&&  (TaskNow.Blocker == &TaskScan->Priv)) {
				printf("Task\'s private mutex\n");
				SemScan = (SEM_t *)1;
			}

			if (SemScan == (SEM_t *)NULL) {
			  #if ((OX_HASH_SEMA) != 0U)
				for (ii=0 ; ii<((OX_HASH_SEMA)+1U) ; ii++) {
					SemScan = g_SemaList[ii];
			  #else
				{
					SemScan = g_SemaList;
			  #endif
					while (SemScan != (SEM_t *)NULL) {
						if (TaskNow.Blocker == SemScan) {
							break;
						}
						SemScan = SemScan->SemaNext;
					}
				}
				if (SemScan != (SEM_t *)NULL) {
					printf("%s (Sema)\n", SemScan->SemName);
				}
			}
		  #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
			if (SemScan == (SEM_t *)NULL) {
				for (ii=0 ; ii<((OX_HASH_MBX)+1U) ; ii++) {
				  #if ((OX_HASH_MBX) != 0U)
					MbxScan = g_MboxList[ii];
				  #else
					MbxScan = g_MboxList;
				  #endif
					while (MbxScan != (MBX_t *)NULL) {
						if (TaskNow.Blocker == &MbxScan->MboxSema) {
							break;
						}
						MbxScan = MbxScan->MbxNext;
					}
				}
				if (MbxScan != (MBX_t *)NULL) {
					printf("%s (Mailbox)\n", MbxScan->MbxName);
				}
			}
		  #endif
		}
		else {
			puts("Suspended");
		}

		if (TaskNow.Blocker != (SEM_t *) NULL) {
		  #if ((OX_TIMEOUT) > 0)
			Legend("Blocking will timeout in:");
			if (TaskNow.ToutPrev == (TSK_t*)NULL) {
				puts("infinite");
			}
			else {
				printf("%d ticks\n", TaskNow.TickTime-G_OStimCnt);
			}
		  #endif
		}

	  #if (OX_TASK_SUSPEND)
		Legend("Pending suspension:");
		printf("%s\n", TaskNow.SuspRqst == 0 ? "No" : "Yes");
	  #endif

	  #if (OX_HANDLE_MTX)
		Legend("My mutexes:");
		MtxNow = TaskNow.MyMutex;
		if (MtxNow == (MTX_t *)NULL) {
			puts("** None **");
		}
		else {
			ii = 0;
			while(MtxNow != (MTX_t *)NULL) {
				if (ii != 0) {
					Legend(" ");
				}
				printf("%s\n", MtxNow->SemName);
				MtxNow = MtxNow->MtxNext;
				ii++;
			}
		}
	  #endif

	  #if (OX_USE_TASK_ARG)
		Legend("Task argument pointer:");
		printf(g_StrPn, TaskNow.TaskArg);
	  #endif

	  #if (((OX_EVENTS) != 0) && !defined(__UABASSI_H__))
		Legend("Events AND mask:");
		printf(g_StrVn, TaskNow.EventAND);

		Legend("Events OR mask:");
		printf(g_StrVn, TaskNow.EventOR);

		Legend("Events accumulated:");
		printf(g_StrVn, TaskNow.EventAcc);

		Legend("Events received:");
		printf(g_StrVn, TaskNow.EventRX);
	  #endif

	  #if (OX_ROUND_ROBIN)
		Legend("Round Robin count done:");
		printf("%d\n", TaskNow.RRcnt);

		Legend("Round Robin last run tick:");
		printf("%d\n", TaskNow.RRtick);

		#if ((OX_ROUND_ROBIN) >= 0)
			ii = OX_ROUND_ROBIN;
		#else
			ii = TaskNow.RRmax;
		#endif

		Legend("Round Robin runtime:");
		if (ii > 0) {
			printf("%d ticks\n", ii);
		}
		else {
			printf("** no round robin **");
		}
	  #endif

	  #if ((OX_STARVE_WAIT_MAX) != 0)
			Legend("Starvation protection:");
	   #if ((OX_STARVE_PRIO) < 0)
		if ((TaskNow.Prio <= TaskNow.StrvPrioMax)
		||  (TaskNow.Prio >= (OX_PRIO_MIN))) {
			puts("Not protected");
			Legend("Starvation max prio:");
			printf("%d\n", TaskNow.StrvPrioMax);
		}
		else {
			puts("Protected");
			Legend("Starvation threshold prio:");
			printf("%d\n", TaskNow.StrvPrioMax);
		   #else
		if ((TaskNow.Prio <= OX_STARVE_PRIO) {
		||  (TaskNow.Prio >= (OX_PRIO_MIN+1))) {
			puts("Not Protected");
			Legend("Starvation prio threshold:");
			printf("%d\n", OX_STARVE_PRIO);
		}
		else {
			puts("Protected");
			Legend("Starvation prio threshold:");
			printf("%d\n", OX_STARVE_PRIO);
	   #endif

			Legend("Starvation state:");
			ii = TaskNow.StrvState;
			if (ii == 0) {
				puts("Blocked - not monitored");
			}
			else if ((ii == 1)
			||       (ii == 5)) {
				puts("Pending removal from linked list");
			}
			else if (ii == 2) {
				puts("Waiting to be running");
			}
			else if (ii == 3) {
				puts("Running under starvation protection");
			}
			else if (ii == 4) {
				puts("Priority got boosted");
			}

			if (ii != 0) {
				Legend("Prio before starvation protect:");
				printf("%d\n", TaskNow.StrvPrio);
			}

			Legend("Starvation original prio:");          
			if (TaskNow.StrvPrio != TaskNow.Prio) {
				printf("%d\n", TaskNow.StrvPrio);
			}
			else {
				printf("No priority change\n");
			}

		   #if ((OX_STARVE_RUN_MAX) >= 0)
			ii = OX_STARVE_RUN_MAX;
		   #else
			ii = TaskNow.StrvRunMax;
		   #endif
			Legend("Starvation max runtime:");   
			printf("%d ticks\n", ii);

		   #if ((OX_STARVE_WAIT_MAX) >= 0)
			ii = OX_STARVE_WAIT_MAX;
		   #else
			ii = TaskNow.StrvWaitMax;
		   #endif
			Legend("Starvation max wait time:");     
			printf("%d ticks\n", ii);
		}
	  #endif

	  #if ((OX_GROUP) != 0)
		Legend("Group task blocked:");
		if (TaskNow.GrpBlkOn == (GRP_t *)NULL) {
			printf("** None **\n");
		}
		else {
			printf(g_StrPn, TaskNow.GrpBlkOn);
		}
	  #endif

	  #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
		Legend("MBox message when blocked:");
		printf(g_StrIPn, TaskNow.MboxMsg);
	  #endif

	  #if (OX_MEM_BLOCK)
		Legend("Mem block buffer:");
		printf(g_StrPn, TaskNow.MblkBuf);
	  #endif

	  #if (OX_STACK_CHECK)
		LL_1 = sizeof(int32_t)
		     + ((uint64_t)(intptr_t)TaskNow.StackHigh)
		     - ((uint64_t)(intptr_t)TaskNow.StackLow);

			
		Legend("Stack size:");
		if (TaskNow.StackHigh == (int32_t *)NULL) {
			puts("** Unknown **");
		}
		else {
			printf("%lld bytes\n", (long long)LL_1);

			LL_2 = TSKstkUsed(&TaskNow); 
			Legend("Stack max used:");
			printf("%lld bytes\n", (long long)LL_2);

			Legend("Stack never unused:");
			printf("%lld bytes\n", (long long)(LL_1-LL_2));
		}
	  #endif

	  #if ((OX_MP_TYPE) & 4U)
		Legend("Target core:");
		if (TaskNow.TargetCore < 0) {
			puts("Any");
		}
		else {
			printf("core #%d\n", TaskNow.TargetCore);
		}
	  #endif
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 3)) {
		if (0 == strcmp(argv[2], "stat")) {
		  #if ((OX_PERF_MON) == 0)
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #else
			Result  = SHELL_RES_OK;

			for (jj=16384 ; jj>0; jj--) {			/* Use a local copy to not have weird stats		*/
				TaskScan->PMsnapshot = 0;			/* due to update during copying					*/
				if ((TaskScan->PMupdating == 0)
				&&  (TaskScan->PMsnapshot == 0)) {
					memmove(&TaskNow, TaskScan, sizeof(TaskNow));
					PMtimeNow = OX_PERF_MON_TIME();
					if (TaskScan->PMsnapshot == 0) {
						break;
					}
				}
			}
			if (jj <= 0) {							/* If <= couldn't take a static copy			*/
				memmove(&TaskNow, TaskScan, sizeof(TaskNow));
				PMtimeNow = OX_PERF_MON_TIME();
			}


			PMdiff = (((TaskNow.PMcontrol != (OX_PM_CTRL_STOP))
			          ? PMtimeNow					/* Total time spanned since restart				*/
			          : TaskNow.PMlastTick)			/* When stopped, use the time spanned between	*/
			          - TaskNow.PMstartTick)		/* restart and stopping							*/
			       / OX_PERF_MON_DIV;
			
		  #define PM_MAX_VAL(x)	do { if (MaxVal < (unsigned long long)(x)) {						\
							MaxVal = (unsigned long long)(x); } } while(0)

			MaxVal = (unsigned long long)0;			/* Find maximum value for miniman left space	*/
			for (ii=0 ; ii<(OX_N_CORE) ; ii++) {
				RunTotal = TaskNow.PMcumul[ii];
				if ((TaskScan == TSKmyID())			/* It's me, I am running						*/
			  #if ((OX_N_CORE) > 1)
				||  (TaskNow.MyCore >= 0)			/* Me or runnign on another core				*/
			  #endif
				) {
					RunTotal += (PMtimeNow			/* Add time since it started running			*/
					          -  TaskNow.PMrunStrt) / OX_PERF_MON_DIV;
				}
				PM_MAX_VAL(RunTotal);
			}
			PM_MAX_VAL(TaskNow.PMlatentLast);
			PM_MAX_VAL(TaskNow.PMlatentMin);
			PM_MAX_VAL(TaskNow.PMlatentMax);
			PM_MAX_VAL(TaskNow.PMlatentAvg);
			PM_MAX_VAL(TaskNow.PMaliveLast);
			PM_MAX_VAL(TaskNow.PMaliveMin);
			PM_MAX_VAL(TaskNow.PMaliveMax);
			PM_MAX_VAL(TaskNow.PMaliveAvg);
			PM_MAX_VAL(TaskNow.PMrunLast);
			PM_MAX_VAL(TaskNow.PMrunMin);
			PM_MAX_VAL(TaskNow.PMrunMax);
			PM_MAX_VAL(TaskNow.PMrunAvg);
			PM_MAX_VAL(TaskNow.PMpreemLast);
			PM_MAX_VAL(TaskNow.PMpreemMin);
			PM_MAX_VAL(TaskNow.PMpreemMax);
			PM_MAX_VAL(TaskNow.PMpreemAvg);
			PM_MAX_VAL(TaskNow.PMpreemCnt);
			PM_MAX_VAL(TaskNow.PMblkCnt);
			PM_MAX_VAL(TaskNow.PMsemBlkCnt);
			PM_MAX_VAL(TaskNow.PMmtxBlkCnt);
		  #if (((OX_EVENTS) != 0) && !defined(__UABASSI_H__))
			PM_MAX_VAL(TaskNow.PMevtBlkCnt);
		   #endif
		   #if (OX_GROUP)
			PM_MAX_VAL(TaskNow.PMgrpBlkCnt);
		   #endif
		   #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
			PM_MAX_VAL(TaskNow.PMmbxBlkCnt);
		   #endif
		   #if (OX_MEM_BLOCK)
			PM_MAX_VAL(TaskNow.PMmblkBlkCnt);
		   #endif
		   #if (OX_STARVE_WAIT_MAX)
			PM_MAX_VAL(TaskNow.PMstrvCnt);
			PM_MAX_VAL(TaskNow.PMstrvRun);
		   #endif
		   #if (OX_MTX_INVERSION)
			PM_MAX_VAL(TaskNow.PMinvertCnt);
		   #endif
			ii = 0;
			do {
				MaxVal = MaxVal / 10;
				ii++;
			} while (MaxVal != 0);

			sprintf(&TmpStr1[0], "%%%dllu", ii);	/* Create the format string to print values		*/

			Legend("Perf mon status:");				/* Show the status of the stat collection		*/
			if (TaskNow.PMcontrol == OX_PM_CTRL_PEND) {
				printf("Capturing - waiting for task unblocked\n");
			}
			else if (TaskNow.PMcontrol == (OX_PM_CTRL_STOP)) {
				printf("Stopped\n");
			}
			else if (TaskNow.PMcontrol == (OX_PM_CTRL_RESTART)) {
				printf("Restarting\n");
			}
			else if (TaskNow.PMcontrol == (OX_PM_CTRL_ARMED)) {
				printf("Capturing - task is unblocked\n");
			}
			else if (TaskNow.PMcontrol == (OX_PM_CTRL_RUN)) {
				if ((TaskScan == TSKmyID())
			  #if ((OX_N_CORE) > 1)
				||  (TaskNow.MyCore >= 0)
			  #endif
				) {
					printf("Capturing - task running\n");
				}
				else {
					printf("Capturing - task ready to run\n");
				}
			}

			RunTotal = (OSperfMon_t)0;
			for (ii=0 ; ii<(OX_N_CORE) ; ii++) {
				if ((OX_N_CORE) == 1) {
					sprintf(&TmpStr2[0], "Total run time:");
				}
				else {
					sprintf(&TmpStr2[0], "Run time - on core #%d:", ii);
				}
				Legend((const char  *)&TmpStr2);

				RunTmp = TaskNow.PMcumul[ii];
				if ((TaskScan == TSKmyID())			/* It's me, I am running						*/
			  #if ((OX_N_CORE) > 1)
				||  (TaskNow.MyCore >= 0)			/* Me or runnign on another core				*/
			  #endif
				) {
					RunTmp += (PMtimeNow
					        -  TaskNow.PMrunStrt) / OX_PERF_MON_DIV;
				}

				printf(&TmpStr1[0], (unsigned long long)RunTmp);
				if (PMdiff == 0) {
					printf(" [%5.1lf%% CPU]\n", 0.0);
				}
				else {
					printf(" [%5.1lf%% CPU]\n", 100.0*((double)RunTmp/(double)PMdiff));
				}
				RunTotal += RunTmp;
			}
			Legend("Run time - total:");
			printf(&TmpStr1[0], (unsigned long long)RunTotal);
			printf(" [%5.1lf%% CPU]\n", 100.0*((double)RunTotal)/((double)PMdiff));

			strcat(&TmpStr1[0], "\n");

			Legend("Run time - last slice:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMrunLast);

			Legend("Run time - minimum:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMrunMin);

			Legend("Run time - maximum:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMrunMax);

			Legend("Run time - average:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMrunAvg);


			Legend("Latency time - last slice:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMlatentLast);

			Legend("Latency time - minimum:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMlatentMin);

			Legend("Latency time - maximum:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMlatentMax);

			Legend("Latency time - average:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMlatentAvg);


			Legend("Alive time - last slice:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMaliveLast);

			Legend("Alive time = minimum:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMaliveMin);

			Legend("Alive time = maximum:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMaliveMax);

			Legend("Alive time = average:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMaliveAvg);


			Legend("Preemption time - last slice:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMpreemLast);

			Legend("Preemption time - minimum:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMpreemMin);

			Legend("Preemption time - maximum:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMpreemMax);

			Legend("Preemption time - average:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMpreemAvg);

			Legend("# of preemptions:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMpreemCnt);

			sprintf(&TmpStr2[0], "# of blockings%s:", (TaskNow.PMblkCnt == 0) ? " (see note)" : "");
			Legend(&TmpStr2[0]);
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMblkCnt);

			Legend("# of blockings on semaphore:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMsemBlkCnt);

			Legend("# of blockinsg on mutex:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMmtxBlkCnt);

		   #if (((OX_EVENTS) != 0) && !defined(__UABASSI_H__))
			Legend("# of blockings on events:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMevtBlkCnt);
		   #endif

		   #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
			Legend("# of blockings on mailbox:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMmbxBlkCnt);
		   #endif

		   #if (OX_GROUP)
			Legend("# of blockings on group:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMgrpBlkCnt);
		   #endif

		   #if (OX_MEM_BLOCK)
			Legend("# of blockings on  mem blk:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMmblkBlkCnt);
		   #endif

		   #if (OX_STARVE_WAIT_MAX)
			Legend("# of strv protections:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMstrvCnt);

			Legend("# of strv prot run:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMstrvRun);

			Legend("# of strv prot run max:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMstrvRunMax);
		   #endif

		   #if (OX_MTX_INVERSION)
			Legend("# of prio inversions:");
			printf(&TmpStr1[0], (unsigned long long)TaskNow.PMinvertCnt);
		   #endif

			if (TaskNow.PMblkCnt == 0) {
				if (TaskNow.PMlatentOK == 0) {
					printf("\nNote: Slice Run Time, Latency, and Alive not yet valid (all 0).\n");
				}
				else {
					printf("\nNote: Slice Run Time and Alive not yet valid (all 0).\n");
				}
				printf("      The task has never blocked.\n");
			}
		  #endif
		}
	}

	if ((Result == SHELL_RES_NONE)
	&&  (argc == 3)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/

		Result  = SHELL_RES_OK;						/* Assume printing OK result					*/
		if (0 == strcmp(argv[2], "resume")) {		/* CMD: task TaskName resume					*/
		  #if ((OX_TASK_SUSPEND) != 0)
			TSKresume(TaskScan);
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}
		else if (0 == strcmp(argv[2], "suspend")) {	/* CMD: task TaskName suspend					*/
		  #if ((OX_TASK_SUSPEND) != 0)
			TSKsuspend(TaskScan);
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}
		else {
			Result = SHELL_RES_UNK_OP_ARG_2;
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 4)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/
		Result  = SHELL_RES_OK;						/* Assume printing OK result					*/

		Value = strtoll(argv[3], &Cptr, 0);

		if (0 == strcmp(argv[2], "arg")) {			/* CMD: task TaskName arg #						*/
		  #if ((OX_USE_TASK_ARG) != 0)
			if (*Cptr == '\0') {
				TSKsetArg(TaskScan, Value);
			}
			else {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}
		else if (0 == strcmp(argv[2], "prio")) {	/* CMD: task TaskName prio #					*/
		  #if (((OX_TASK_SUSPEND) != 0) && ((OX_PRIO_CHANGE) != 0))
			if (*Cptr == '\0') {
				if ((Value >= 0)
				&&  (Value <= OX_PRIO_MIN)) {
					TSKsetPrio(TaskScan, Value);
				}
				else {
					Result = SHELL_RES_OUT_RANGE_ARG_3;
				}
			}
			else {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}
		else if (0 == strcmp(argv[2], "tout")) {	/* CMD: task TaskName tout kill					*/
		  #if ((OX_TIMEOUT) > 0)
			if (0 == strcmp(argv[3], "kill")) {
				TSKtimeoutKill(TaskScan);
			}
			else {
				Result = SHELL_RES_UNK_FLD_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNK_CMD_ARG_2;
		  #endif
		}
		else if (0 == strcmp(argv[2], "RR")) {		/* CMD: task TaskName RR off					*/
		  #if ((OX_ROUND_ROBIN) < 0)
			if (0 == strcmp(argv[3], "off")) {
				TSKsetRR(TaskScan,0);
			}
			else {
				Result = SHELL_RES_UNK_FLD_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNK_CMD_ARG_2;
		  #endif
		}
		else if (0 == strcmp(argv[2], "strv")) {	/* CMD: task TaskName strv off					*/
		  #if ((OX_STARVE_PRIO) < 0)
			if (0 == strcmp(argv[3], "off")) {
				TSKsetStrvPrio(TaskScan, OX_PRIO_MIN);
			}
			else {
				Result = SHELL_RES_UNK_FLD_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNK_OP_ARG_2;
		  #endif
		}
		else if (0 == strcmp(argv[2], "stat")) {	/* CMD: task TaskName stat xxxx					*/
		  #if ((OX_PERF_MON) == 0)
			Result = SHELL_RES_UNK_OP_ARG_2;
		  #else
			if (0 == strcmp(argv[3], "restart")) {
				PMrestart(TaskScan);
			}
			else if (0 == strcmp(argv[3], "stop")) {
				PMstop(TaskScan);
			}
			else {
				Result = SHELL_RES_UNK_FLD_ARG_3;
			}
		  #endif
		}
		else {
			Result = SHELL_RES_UNK_OP_ARG_2;
		}

	}
	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 5)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/
		Result  = SHELL_RES_OK;						/* Assume printing OK result					*/

		Value = strtoll(argv[4], &Cptr, 0);

		if (0 == strcmp(argv[2], "set")) {			/* CMD: task TaskName set ??? #					*/
			if (0 == strcmp(argv[3], "tout")) {		/* CMD: task Name set tout #					*/
			  #if ((OX_TIMEOUT) > 0)
				if (*Cptr == '\0') {
					TSKtout(TaskScan, Value);
				}
				else {
					Result = SHELL_RES_INVAL_VAL_ARG_4;
				}
			  #else
				Result = SHELL_RES_UNAVAIL_OP_ARG_2;
			  #endif
			}

			else if (0 == strcmp(argv[2], "Core")) {/* CMD: task set core #							*/
			  #if ((OX_MP_TYPE) & 4U)
				if ((Value >= 0)
				&&  (Value < OX_N_CORE)) {
					TSKsetCore(TaskScan, Value);
				}
				else {
					Result = SHELL_RES_OUT_RANGE_ARG_2;
				}
			  #else
				Result = SHELL_RES_UNAVAIL_OP_ARG_2;
			  #endif
			}
			else {
				Result = SHELL_RES_UNK_OP_ARG_3;
			}
		}

		else if (0 == strcmp(argv[2], "evt")) {
		  #if (((OX_EVENTS) != 0) && !defined(__UABASSI_H__))
			if (0 == strcmp(argv[3], "acc")) {		/* CMD: task evt acc #							*/
				if (*Cptr == '\0') {
					TaskScan->EventAcc = Value;
				}
				else {
					Result = SHELL_RES_INVAL_VAL_ARG_4;
				}
			}
			else if (0 == strcmp(argv[3], "and")) {	/* CMD: task evt and #							*/
				if (*Cptr == '\0') {
					TaskScan->EventAND = Value;
				}
				else {
					Result = SHELL_RES_INVAL_VAL_ARG_4;
				}
			}
			else if (0 == strcmp(argv[3], "or")) {	/* CMD: task evt or #							*/
				if (*Cptr == '\0') {
					TaskScan->EventRX = Value;
				}
				else {
					Result = SHELL_RES_INVAL_VAL_ARG_4;
				}
			}
			else if (0 == strcmp(argv[3], "rx")) {	/* CMD: task evt rx #							*/
				if (*Cptr == '\0') {
					TaskScan->EventRX = Value;
				}
				else {
					Result = SHELL_RES_INVAL_VAL_ARG_4;
				}
			}
			else {
				Result = SHELL_RES_UNK_OP_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}

		else if (0 == strcmp(argv[2], "mbx")) {
		  #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
			if (0 == strcmp(argv[3], "msg")) {
				if (*Cptr == '\0') {
					TaskScan->MboxMsg = Value;
				}
				else {
					Result = SHELL_RES_INVAL_VAL_ARG_4;
				}
			}
			else {
				Result = SHELL_RES_UNK_CMD_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}

		else if (0 == strcmp(argv[2], "RR")) {
		  #if ((OS_ROUND_ROBIN) != 0)
			if (0 == strcmp(argv[3], "slice")) {
			  #if ((OS_ROUND_ROBIN) < 0)
				if (*Cptr == '\0') {
					TSKsetRR(TaskScan, Value);
				}
				else {
					Result = SHELL_RES_INVAL_VAL_ARG_4;
				}
			  #else
				Result = SHELL_RES_UNAVAIL_OP_ARG_3;
			  #endif
			}
			if (0 == strcmp(argv[3], "count")) {
				if (*Cptr == '\0') {
					TaskScan->RRcnt = Value;
				}
				else {
					Result = SHELL_RES_INVAL_VAL_ARG_4;
				}
			}
			else {
				Result = SHELL_RES_UNK_OP_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNAVAIL_FLD_ARG_2;
		  #endif
		}
		else if (0 == strcmp(argv[2], "strv")) {
		  #if ((OX_STARVE_PRIO) < 0)
			if (0 == strcmp(argv[3], "prio")) {
				if (*Cptr == '\0') {
					if ((Value >= 0)
					&&  (Value <= OX_PRIO_MIN)) {
						TSKsetStrvPrio(TaskScan, Value);
					}
					else {
						Result = SHELL_RES_OUT_RANGE_ARG_4;
					}
				}
				else {
					Result = SHELL_RES_INVAL_VAL_ARG_4;
				}
			}
			else if (0 == strcmp(argv[3], "run")) {
				if (*Cptr == '\0') {
					TSKsetStrvRunMax(TaskScan, Value);
				}
				else {
					Result = SHELL_RES_INVAL_VAL_ARG_4;
				}
			}
			else if (0 == strcmp(argv[3], "wait")) {
				if (*Cptr == '\0') {
					TSKsetStrvWaitMax(TaskScan, Value);
				}
				else {
					Result = SHELL_RES_INVAL_VAL_ARG_4;
				}
			}
			else {
				Result = SHELL_RES_UNK_FLD_ARG_3;
			}
		  #else
			Result = SHELL_RES_UNAVAIL_OP_ARG_2;
		  #endif
		}
		else {
			Result = SHELL_RES_UNK_OP_ARG_2;
		}
	}

	return(PrtResult(Serv, Result, OSret, argv, Value, (intptr_t)0));
}

/* ------------------------------------------------------------------------------------------------ */
/* Timer services command : tim																		*/
/*																									*/
/* Use the run-time help for a complete description of all the commands supported					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdTim(int argc, char *argv[])
{
#if ((OX_TIMER_SRV) == 0)
	puts("tim  : timer services - not supported (OS_TIMER_SRV == 0)");
	return(-1);
#else
char    *Cptr;										/* Used to know if number is malformed			*/
int      ii;										/* General purpose								*/
unsigned int Ops;									/* Timer callback operation						*/
int	     OSret;										/* Return value from OS service call			*/
int      Result;									/* Result of command (to select error to print)	*/
char    *Serv = "TIM";								/* Service name									*/
TIM_t    TimNow;									/* Snapshot of the one to display / manipulate	*/
TIM_t   *TimScan;									/* To traverse Abassi internal linked list		*/
intptr_t Value;										/* Numerical value specified on command line	*/

	OSret  = 0;										/* OS return is OK								*/
	Result = SHELL_RES_NONE;						/* Start with no error							*/
	Value  = 0;

	if (argc < 0) {									/* Special value to print usage					*/
		puts("tim  : timer services information / processing");

		if (argc < -1) {
			puts("usage:");
			HLP_CMD("tim");
			  HLP_DSC("show all the timer services in the application");
			HLP_CMD("tim ##");
			  HLP_DSC("dump the timer descriptor field memory offsets");
			HLP_CMD("tim <TimName>");
			  HLP_DSC("dump info on the timer service TimName");
			HLP_CMD("tim <TimName> add #");
			  HLP_DSC("set callback argument increment of the timer service <TimName> to #");
			  HLP_CMP("TIMtoAdd()");
			HLP_CMD("tim <TimName> adj #");
			  HLP_DSC("adjust next time out of the timer service <TimName> to #");
			  HLP_CMP("TIMadj()");
			HLP_CMD("tim <TimName> arg #");
			  HLP_DSC("set the argument of the timer service <TimName> to #");
			  HLP_CMP("TIMarg()");
			HLP_CMD("tim <TimName> freeze");
			  HLP_DSC("freeze the timer service <TimName>");
			  HLP_CMP("TIMfreeze()");
			HLP_CMD("tim <TimName> kill");
			  HLP_DSC("stop and remove the timer service <TimName>");
			  HLP_CMP("TIMkill()");
			HLP_CMD("tim <TimName> pause");
			  HLP_DSC("pause the timer service <TimName>");
			  HLP_CMP("TIMpause()");
			HLP_CMD("tim <TimName> period #");
			  HLP_DSC("set the period of the timer service <TimName> to #");
			  HLP_CMP("TIMperiod()");
			HLP_CMD("tim <TimName> restart");
			  HLP_DSC("restart the timer service <TimName>");
			  HLP_CMP("TIMrestart()");
			HLP_CMD("tim <TimName> resume");
			  HLP_DSC("resumse the timer service <TimName>");
			  HLP_CMP("TIMresume()");
		  #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
			HLP_CMD("tim <TimName> callback type data|evt|fct|mbx|mtx|sem");
		  #else
			HLP_CMD("tim <TimName> callback type data|evt|fct|mtx|sem");
		  #endif
			  HLP_DSC("set the type of callback of the timer service <TimName>");
			  HLP_CMP("- none -");
			HLP_CMD("tim <TimName> callback fct #");
			  HLP_DSC("set the callback function address of the timer service <TimName>");
			  HLP_CMP("- none -");
			HLP_CMD("tim <TimName> callback arg #");
			  HLP_DSC("set the callback argument of the timer service <TimName>");
			  HLP_CMP("TIMarg()");
		}
		Result = SHELL_RES_OK_NO_MSG;
	}
	else if (argc > 5) {
		puts("usage: tim [[TimName] [<op>|{<property> [#]} ]]");
		Result = SHELL_RES_ERR_NO_MSG;
	}

	TimScan = (TIM_t *)NULL;

	if (Result == SHELL_RES_NONE) {					/* Dump all or find the one to work with		*/
		if ((argc > 1)
		&&  (argv[1][0] == '0')
		&&  (argv[1][1] == 'x')) {
			TimScan = (TIM_t *)(uintptr_t)strtoul(argv[1], &Cptr, 16);
			if (*Cptr != '\0') {
				TimScan = (TIM_t *)NULL;
			}
		}
		else {
			for (ii=0 ; ii<((OX_HASH_TIMSRV)+1U) ; ii++) {	/* Don't use TIMopen() because it will	*/
			  #if ((OX_HASH_TIMSRV) != 0U)			/* create one when the timer doesn't exist		*/
				TimScan = g_TimerList[ii];
			  #else
				TimScan = g_TimerList;
			  #endif
				while (TimScan != (TIM_t *)NULL) {	/* Traverse the list of tasks					*/
					if (argc == 1) {
						printf("[address:");
						printf(g_StrP, TimScan);
						printf("] ");
						printf("%s\n", TimScan->TimName);
						Value = 1;
					}
					else {
						if (0 == strcmp(argv[1], TimScan->TimName)) {
							break;
						}
					}
					TimScan = TimScan->TimNext;
				}
			}
		}
		if ((argc == 1)
		&&  (Value == 0)) {
			puts("No timer services in the application");
			Result = SHELL_RES_OK;
		}
	}

	if ((Result == SHELL_RES_NONE)
	&&  ((argc > 2)
	  ||   ((argc == 2) && (0 != strcmp("##", argv[1]))))) {
		if (TimScan != (TIM_t *)NULL) {
			ii = OSintOff();
			memmove(&TimNow, TimScan, sizeof(TimNow));
			OSintBack(ii);
		}
		else {
			Serv   = "timer";
			Result = SHELL_RES_NO_NAME_ARG_1;
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 2)) {								/* CMD: tim TimName								*/
		if (0 == strcmp(argv[1], "##")) {			/* CMD: mtx ##									*/
			Legend("TmNext:");
			printf("%d\n", (int)offsetof(TIM_t, TmNext));
			Legend("TmPrev:");
			printf("%d\n", (int)offsetof(TIM_t, TmPrev));
		  #if (OX_NAMES)
			Legend("TimName:");
			printf("%d\n", (int)offsetof(TIM_t, TimName));
			Legend("TimNext:");
			printf("%d\n", (int)offsetof(TIM_t, TimNext));
		  #endif
			Legend("Ptr:");
			printf("%d\n", (int)offsetof(TIM_t, Ptr));
			Legend("FctPtr:");
			printf("%d\n", (int)offsetof(TIM_t, FctPtr));
			Legend("Arg:");
			printf("%d\n", (int)offsetof(TIM_t, Arg));
			Legend("ArgOri:");
			printf("%d\n", (int)offsetof(TIM_t, ArgOri));
			Legend("Ops:");
			printf("%d\n", (int)offsetof(TIM_t, Ops));
			Legend("Period:");
			printf("%d\n", (int)offsetof(TIM_t, Period));
			Legend("Expiry:");
			printf("%d\n", (int)offsetof(TIM_t, Expiry));
			Legend("ExpOri:");
			printf("%d\n", (int)offsetof(TIM_t, ExpOri));
			Legend("TickDone:");
			printf("%d\n", (int)offsetof(TIM_t, TickDone));
			Legend("ToAdd:");
			printf("%d\n", (int)offsetof(TIM_t, ToAdd));
			Legend("Paused:");
			printf("%d\n", (int)offsetof(TIM_t, Paused));
		  #if (OX_TIM_PARKING)					/* When "destroy timer" is needed					*/
			Legend("ParkNext:");
			printf("%d\n", (int)offsetof(TIM_t, ParkNext));
		  #endif
		  #if (OX_TIM_XTRA_FIELD)
			Legend("XtraData:");
			printf("%d\n", (int)offsetof(TIM_t, XtraData));
		  #endif

		}
		else {
			printf("Info on timer %s:\n\n", argv[1]);

			Legend("Timer name:");
			printf("%s\n", TimScan->TimName);

			Legend("Descriptor address:");
			printf(g_StrPn, TimScan);

			ii  = 0;								/* Assume no ToAdd used in the timer			*/
			Ops = TimNow.Ops & ~(ABASSI_TIM_SRV);
			Legend("Service type:");
			if (Ops == (TIM_CALL_FCT)) {
				puts("Function call");
				Legend("Function address:");
				printf(g_StrPn, TimNow.FctPtr);
				Legend("Function argument:");
				printf(g_StrIPn, TimNow.Arg);
				ii = 1;
			}
			else if (Ops == (TIM_WRITE_DATA)) {
				puts("Data writing");
				Legend("Address to write:");
				printf(g_StrPn, TimNow.Ptr);
				Legend("Data to write:");
				printf(g_StrIPn, TimNow.Arg);
				ii = 1;
			}
		  #if (((OX_EVENTS) != 0) && !defined(__UABASSI_H__))
			else if (Ops == (ABASSI_EVENT)) {
				puts("Event set");
				Legend("Task Name");
				printf("%s\n", ((TSK_t *)(TimNow.Ptr))->TskName);
				Legend("Event to set:");
				printf(g_StrIPn, TimNow.Arg);
			}
		  #endif
		  #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
			else if (Ops == (ABASSI_MBX_PUT)) {
				puts("Mailbox put");
				Legend("Mailbox Name");
				printf("%s\n", ((MBX_t *)(TimNow.Ptr))->MbxName);
				Legend("Value to write:");
				printf(g_StrIPn, TimNow.Arg);
				ii = 1;
			}
		  #endif
			else if (Ops == (ABASSI_POST)) {
				puts("Semaphore posting");
				Legend("Semaphore Name");
				printf("%s\n", ((SEM_t *)(TimNow.Ptr))->SemName);
			}
			else {
				puts("Mutex unlock");
				Legend("Mutex Name");
				printf("%s\n", ((MTX_t *)(TimNow.Ptr))->SemName);
			}

			Legend("Status");
			if (TimNow.Paused != 0) {
				printf("Paused\n");
			}
			else if (TimNow.TmPrev == (TIM_t *)NULL) {
				printf("Suspended\n");
			}
			else {
				printf("Active\n");
			}

			Legend("Period");
			printf("%d\n", TimNow.Period);

			if (ii != 0) {
				Legend("Adding to value");
				printf("%d\n", TimNow.ToAdd);
			}

			Legend("Expiry at tick");
			printf("%d\n", TimNow.TickDone);
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 3)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/
		Result  = SHELL_RES_OK;

		if (0 == strcmp(argv[2], "freeze")) {		/* CMD: tim TimName freeze						*/
			TIMfreeze(TimScan);
		}
		else if (0 == strcmp(argv[2], "kill")) {	/* CMD: tim TimName kill						*/
			TIMkill(TimScan);
		}
		else if (0 == strcmp(argv[2], "pause")) {	/* CMD: tim TimName pause						*/
			TIMpause(TimScan);
		}
		else if (0 == strcmp(argv[2], "restart")) {	/* CMD: tim TimName restart						*/
			TIMrestart(TimScan);
		}
		else if (0 == strcmp(argv[2], "resume")) {	/* CMD: tim TimName resume						*/
			TIMresume(TimScan);
		}
		else {
			Result = SHELL_RES_UNK_CMD_ARG_2;
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 4)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/
		Result  = SHELL_RES_OK;

		Value = strtoll(argv[3], &Cptr, 0);
		if (0 == strcmp(argv[2], "adj")) {			/* CMD: tim TimName adj #						*/
			if (*Cptr == '\0') {
				TIMadj(TimScan, Value);
			}
			else {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		}
		else if (0 == strcmp(argv[2], "arg")) {		/* CMD: tim TimName arg #						*/
			if (*Cptr == '\0') {
				TIMarg(TimScan, Value);
			}
			else {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		}
		else if (0 == strcmp(argv[2], "add")) {		/* CMD: tim TimName add #						*/
			if (*Cptr == '\0') {
				TIMtoAdd(TimScan, (int)Value);
			}
			else {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
		}
		else if (0 == strcmp(argv[2], "period")) {	/* CMD: tim TimName period #					*/
			if (*Cptr != '\0') {
				Result = SHELL_RES_INVAL_VAL_ARG_3;
			}
			else if ((int)Value <= 0) {
				Result = SHELL_RES_OUT_RANGE_ARG_2;
			}
			else {
				TIMperiod(TimScan, (int)Value);
			}
		}
		else {
			Result = SHELL_RES_UNK_FLD_ARG_2;
		}
	}

	if ((Result == SHELL_RES_NONE)					/* -------------------------------------------- */
	&&  (argc == 5)
	&&  (SHELL_CHK_WRT(Result))) {					/* Writing so make sure read-write access login	*/
		Result  = SHELL_RES_OK;

		if (0 == strcmp(argv[2], "callback")) {
			Value = strtoll(argv[4], &Cptr, 0);		
			if (0 == strcmp(argv[3], "fct")) {		/* CMD: tim TimName callback fct #				*/
				if (*Cptr == '\0') {
					TimNow.Ptr = (void *)Value;
				}
				else {
					Result = SHELL_RES_INVAL_VAL_ARG_4;
				}
			}
			else if (0 == strcmp(argv[3], "arg")) 	{/* CMD: tim TimName callback arg #				*/
				if (*Cptr != '\0') {
					Result = SHELL_RES_INVAL_VAL_ARG_4;
				}
				else if (Value <= 0) {
					Result = SHELL_RES_OUT_RANGE_ARG_4;
				}
				else {
					TimNow.Arg = Value;
				}
			}
			else if (0 == strcmp(argv[3], "type")) {
				if (0 == strcmp(argv[4], "data")) {	/* CMD: tim TimName type data					*/
					TimNow.Ops = TIM_WRITE_DATA;
				}
				else if (0 == strcmp(argv[4], "evt")) {	/* CMD: tim TimName type evt				*/
					TimNow.Ops = ABASSI_EVENT;
				}
				else if (0 == strcmp(argv[4], "fct")) {	/* CMD: tim TimName type fct				*/
					TimNow.Ops = TIM_CALL_FCT;
				}
				else if (0 == strcmp(argv[4], "mbx")) {	/* CMD: tim TimName type mbx				*/
				  #if (((OX_MAILBOX) != 0) && !defined(__UABASSI_H__))
					TimNow.Ops = ABASSI_MBX_PUT;
				  #else
					Result = SHELL_RES_UNAVAIL_FLD_ARG_4;
				  #endif
				}
				else if (0 == strcmp(argv[4], "mtx")) {	/* CMD: tim TimName type mtx				*/
					TimNow.Ops = (ABASSI_POST) | (ABASSI_MTX);
				}
				else if (0 == strcmp(argv[4], "sem")) {	/* CMD: tim TimName type sem				*/
					TimNow.Ops =ABASSI_POST ;
				}
				else {
					Result = SHELL_RES_UNK_FLD_ARG_4;
				}
			}
			else {
				Result = SHELL_RES_UNK_FLD_ARG_3;
			}
		}
		else {
			Result = SHELL_RES_UNK_OP_ARG_2;
		}
	}

	return(PrtResult(Serv, Result, OSret, argv, Value, (intptr_t)0));

#endif
}

/* ------------------------------------------------------------------------------------------------ */
/* Utilities																						*/
/* ------------------------------------------------------------------------------------------------ */

static void Legend(const char *Txt)
{
int Nchar;

	Nchar = printf(Txt);
	for ( ; Nchar<LEGEND_ALIGN ; Nchar++) {
		putchar(' ');
	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */

static int PrtResult(char *Srv, int Result, int OSret, char *argv[], int Value, intptr_t IntPtr)
{
int RetVal;

	RetVal = 0;

	if (OSret != 0) {
		printf("%s%s() failed (OS reported %d)\n", Srv, argv[2], OSret);
		RetVal = 1;
		Result = SHELL_RES_NONE;
	}

	switch(Result) {
		case SHELL_RES_NONE:
		case SHELL_RES_OK_NO_MSG:
			break;
		case SHELL_RES_OK:
			puts("Done");
			break;
		case SHELL_RES_ERR_NO_MSG:
			RetVal = 1;
			break;
		case SHELL_RES_PRT_HEX:
			printf("Done : ");
			printf(g_StrVn, Value);
			break;
		case SHELL_RES_PRT_INTPTR:
			printf("Done : ");
			printf(g_StrIPn, IntPtr);
			break;
		case SHELL_RES_UNK_CMD_ARG_0:
		case SHELL_RES_UNK_CMD_ARG_1:
		case SHELL_RES_UNK_CMD_ARG_2:
		case SHELL_RES_UNK_CMD_ARG_3:
		case SHELL_RES_UNK_CMD_ARG_4:
		case SHELL_RES_UNK_CMD_ARG_5:
			printf("\nERROR : unknown command \"%s\"\n", argv[Result-(SHELL_RES_UNK_CMD_ARG_0)]);
			RetVal = 1;
			break;
		case SHELL_RES_UNK_FLD_ARG_1:
		case SHELL_RES_UNK_FLD_ARG_2:
		case SHELL_RES_UNK_FLD_ARG_3:
		case SHELL_RES_UNK_FLD_ARG_4:
		case SHELL_RES_UNK_FLD_ARG_5:
			printf("\nERROR : unknown field \"%s\"\n", argv[Result+1-(SHELL_RES_UNK_FLD_ARG_1)]);
			RetVal = 1;
			break;
		case SHELL_RES_UNK_OP_ARG_1:
		case SHELL_RES_UNK_OP_ARG_2:
		case SHELL_RES_UNK_OP_ARG_3:
		case SHELL_RES_UNK_OP_ARG_4:
		case SHELL_RES_UNK_OP_ARG_5:
			printf("\nERROR : unknown operation \"%s\"\n", argv[Result+1-(SHELL_RES_UNK_OP_ARG_1)]);
			RetVal = 1;
			break;

		case SHELL_RES_UNAVAIL_OP_ARG_0:
		case SHELL_RES_UNAVAIL_OP_ARG_1:
		case SHELL_RES_UNAVAIL_OP_ARG_2:
		case SHELL_RES_UNAVAIL_OP_ARG_3:
		case SHELL_RES_UNAVAIL_OP_ARG_4:
		case SHELL_RES_UNAVAIL_OP_ARG_5:
			printf("\nERROR : operation \"%s%s\" not available\n", Srv,
			                                       argv[Result-(SHELL_RES_UNAVAIL_OP_ARG_0)]);
			RetVal = 1;
			break;
		case SHELL_RES_UNAVAIL_FLD_ARG_1:
		case SHELL_RES_UNAVAIL_FLD_ARG_2:
		case SHELL_RES_UNAVAIL_FLD_ARG_3:
		case SHELL_RES_UNAVAIL_FLD_ARG_4:
		case SHELL_RES_UNAVAIL_FLD_ARG_5:
			printf("\nERROR : \"%s\" not available\n", argv[Result+1-(SHELL_RES_UNAVAIL_FLD_ARG_1)]);
			RetVal = 1;
			break;
		case SHELL_RES_INVAL_VAL_ARG_1:
		case SHELL_RES_INVAL_VAL_ARG_2:
		case SHELL_RES_INVAL_VAL_ARG_3:
		case SHELL_RES_INVAL_VAL_ARG_4:
		case SHELL_RES_INVAL_VAL_ARG_5:
			printf("\nERROR : invalid numerical value: \"%s\"\n\n",
			                                       argv[Result+1-(SHELL_RES_INVAL_VAL_ARG_1)]);
			RetVal = 1;
			break;
		case SHELL_RES_NO_NAME_ARG_1:
		case SHELL_RES_NO_NAME_ARG_2:
		case SHELL_RES_NO_NAME_ARG_3:
		case SHELL_RES_NO_NAME_ARG_4:
		case SHELL_RES_NO_NAME_ARG_5:
			printf("\nERROR : no %s named \"%s\" in the application\n\n", Srv,
			                                       argv[Result+1-(SHELL_RES_NO_NAME_ARG_1)]);
			RetVal = 1;
			break;
		case SHELL_RES_OUT_RANGE_ARG_1:
		case SHELL_RES_OUT_RANGE_ARG_2:
		case SHELL_RES_OUT_RANGE_ARG_3:
		case SHELL_RES_OUT_RANGE_ARG_4:
			printf("\nERROR : value \"%s\" is out of range\n\n", 
			                                       argv[Result+1-(SHELL_RES_OUT_RANGE_ARG_1)]);
			RetVal = 1;
			break;
		case SHELL_RES_NO_SERV:
			printf("No %s in the application\n", Srv);
			RetVal = 1;
			break;
		case SHELL_RES_NOT_VALID:
			puts("Not applicable");
			RetVal = 1;
			break;
		case SHELL_RES_IS_RD_ONLY:
			puts("Not performed - Read-only access");
			RetVal = 1;
			break;
		default:
			break;
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Get user stuff																					*/
/* Only used for login and non-command input														*/

void ShellGets(char *Str, int Len)
{
int ii;												/* Read index in the command line				*/
int jj;												/* Write index in the command line				*/
#if ((SHELL_LOGIN) > 1)
  int LastCtime;									/* Time when the Last char was received			*/
#endif

  #if ((SHELL_INPUT) <= 0)
	fgets(Str, Len, stdin);							/* Use fgets() instead of gets() to limit the	*/
  #else												/* number of character read						*/
   #if ((SHELL_LOGIN) > 1)							/* When auto-logout after inactivity the build	*/
	LastCtime = G_OStimCnt							/* option SHELL_LOGIN specified the # of second	*/
	          + OS_SEC_TO_TICK(SHELL_LOGIN);		/* of inacrtivity								*/
   #endif

	jj = 0;
	Str[0] = '\0';
	do {
		ii = GetKey();								/* Get the char									*/
		while (ii == 0) {							/* If no char available							*/
			TSKsleep(OS_MS_TO_MIN_TICK(SHELL_INPUT,2));		/* Sleep for the time specified			*/
			ii = GetKey();
		  #if ((SHELL_LOGIN) > 1)					/* When > 1, auto-logout after inactivity		*/
			if ((ii != 0)							/* A new char was typed so update the time		*/
			||  (g_IsLogin == 0)) {					/* when it was typed. No expiry when prompting	*/
				LastCtime = G_OStimCnt				/* for login									*/
				          + OS_SEC_TO_TICK(SHELL_LOGIN);
			}
			else if (OS_HAS_TIMEDOUT(LastCtime)) {
				printf("\nExiting shell due to inactivity\n");
				Str       = '\0';					/* Zap out the history							*/
				g_IsLogin = 0;						/* This will go back to the credential prompt	*/
				return;
			}
		  #endif
		}

		if (((char)ii == '\r')						/* When <CR> or <LF>, done reading the input	*/
		||  ((char)ii == '\n')) {
			ii = '\0';
		}
		if (ii == '\t') {							/* Replace tabs by space						*/
			ii = ' ';
		}
		if (jj < (Len-1)) {							/* Don't insert more than Str can hold			*/
			if ((char)ii == '\b') {					/* Backspace									*/
				if (jj != 0) {
					Str[--jj] = '\0';
					if (g_EchoOff == 0) {
						putchar('\b');
						putchar(' ');
						putchar('\b');
					}
				}
			}
			else {
				Str[jj++] = (char)ii;				/* Insert the char in the string				*/
				Str[jj]   = '\0';
				if (g_EchoOff == 0) {
					putchar((char)ii);
				}
			}
		}
	} while ((char)ii != '\0');
  #endif

	putchar('\n');

	return;
}

/* ------------------------------------------------------------------------------------------------ */


void ShellGetEdit(char *CmdStr)
{
int   CmdLast;										/* Index of the last character (the '\0'		*/
int   CmdPos;										/* Index of the insert position					*/
int   Done;											/* If we have a new command line (got \r or \f)	*/
int   ii;											/* General purpose								*/
int   jj;											/* General purpose								*/
int   kk;											/* General purpose								*/
int   StrEsc[4];									/* To hold escape sequence in analysis			*/
char *Str;											/* Pointer in the history buffer being filled	*/
#if ((SHELL_HISTORY) > 0)							/* of CmdStr if no history						*/
  int   HistEdit;									/* When history being edited (no insert yet)	*/
  int   HistIdx;									/* When ^P or ^N pressed, history idx selected	*/
  int   UpdStr;										/* If the history buffer (Str) must be updated	*/
#endif
#if ((SHELL_LOGIN) > 1)
  int LastCtime;									/* Time when the Last char was received			*/
#endif

  #if ((SHELL_HISTORY) <= 0)						/* Not using an history buffer					*/
	Str = CmdStr;									/* Command line edited directly in the argument	*/
  #else
	ii = g_HistNew;									/* History buffer used, then the new command	*/
	if (--ii < 0) {									/* is edited directly in the history buffer		*/
		ii = SHELL_HISTORY;							/* Go to the next (forward in time) location	*/
	}
	if (ii == g_HistOld) {							/* When the history buffer is full				*/
		if (--g_HistOld < 0) {						/* Set oldest entry index to the one that is	*/
			g_HistOld = SHELL_HISTORY;				/* after in time								*/
		}
	}

	Str = &g_CmdHist[g_HistNew][0];					/* Location to edit the command line			*/

	HistEdit = g_HistNew;
	HistIdx  = g_HistNew;							/* Right now we display this one				*/
	UpdStr   = 0;									/* No need to update the display yet			*/
  #endif

   #if ((SHELL_LOGIN) > 1)							/* When auto-logout after inactivity the build	*/
	LastCtime = G_OStimCnt							/* option SHELL_LOGIN specified the # of second	*/
	          + OS_SEC_TO_TICK(SHELL_LOGIN);		/* of inacrtivity								*/
   #endif

	CmdLast   = 0;									/* Command line last index is 0					*/
	CmdPos    = 0;									/* Current insert position is 0					*/
	Done      = 0;									/* Not done reading the command line			*/
	*Str      = '\0';								/* Empty command								*/
	StrEsc[0] = '\0';

	do {
		if (StrEsc[0] != '\0') {					/* If != '\0' we've encountered an escape		*/
			ii = StrEsc[0];							/* sequence that is anot one of the arry keys	*/
			memmove(&StrEsc[0], &StrEsc[1], sizeof(StrEsc)-1);	/* So use what we've analysed and	*/
		}											/* did not insert in the command line			*/
		else {										/* Not an escape sequence, regular input		*/
		  #if ((SHELL_INPUT) <= 0)					/* Wait for input using system call layer		*/
			ii = getchar();							/* Get the char									*/
		  #else										/* Wait for input not using system call layer	*/
			ii = GetKey();							/* Get the char									*/
			while (ii == 0) {						/* If no char available							*/
				TSKsleep(OS_MS_TO_MIN_TICK(SHELL_INPUT,2));		/* Sleep for the time specified		*/
				ii = GetKey();						/* Try getting a char again						*/
			  #if ((SHELL_LOGIN) > 1)				/* When > 1, auto-logout after inactivity		*/
				if ((ii != 0)						/* A new char was typed so update the time		*/
				||  (g_IsLogin == 0)) {				/* when it was typed. No expiry when prompting	*/
					LastCtime = G_OStimCnt			/* for login									*/
					          + OS_SEC_TO_TICK(SHELL_LOGIN);
				}
				else if (OS_HAS_TIMEDOUT(LastCtime)) {
					printf("\nExiting shell due to inactivity\n");
					CmdStr[0] = '\0';				/* Return empty command							*/
					Str       = '\0';				/* Zap out the history							*/
					g_IsLogin = 0;					/* This will go back to the credential prompt	*/
					return;
				}									/* command and the processiong is the same  as	*/
			  #endif								/*  if the user had	typed "logout"				*/
			}
		  #endif

			if (ii == 27) {							/* The character read is an ESC, start looking	*/
				jj = OS_TICK_PER_SEC;				/* for a VT100 arrow key escape sequence		*/
				memset(&StrEsc[0], 0, sizeof(StrEsc));
				for (kk=0 ; kk<3 ; kk++) {			/* <ESC> '[' then 'A' or 'B' or 'C' or 'D'		*/
					StrEsc[kk] = ii;				/* Memo in StrEsc[] in case is not arrow key	*/
					if ((kk == 0)					/* 1 loop always valid (1st char is <ESC>)		*/
					||  ((kk == 1) && (ii == '['))) {	/* Second loop valis if char is '['			*/
						ii = GetKey();				/* Hopefully it's a key sequence and we'll get	*/
						while ((ii == 0)			/* the rest of the sequence within 1 second		*/
						&&     (jj > 0)) {			/* So grab new chart when available				*/
							jj -= OS_MS_TO_MIN_TICK(SHELL_INPUT,2);
							TSKsleep(OS_MS_TO_MIN_TICK(SHELL_INPUT,2));
							ii = GetKey();
						}
					}
					else {							/* The escape sequence is not <ESC> '[', abort	*/
						break;
					}
				}
							
				if (kk == 2) {						/* When kk == 2, we got <ESC> '[', no timeout	*/
					if ((ii >= 'A')					/* Now check for 'A' or 'B' or 'C' or 'D'		*/
					&&  (ii <= 'D')) {
						if (ii == 'A') {			/* Up key, remppaed to ^P						*/
							ii = 16;
						}
						else if (ii == 'B') {		/* Down key, remapped to ^N						*/
							ii = 14;
						}
						else if (ii == 'C') {		/* Right key, remapped to ^F					*/
							ii = 6;
						}
						else {						/* Left key, remapped to ^B						*/
							ii = 2;
						}
						StrEsc[1] = '\0';			/* This invalidate memorized escape sequence	*/
					}								/* See the memmove() a bit below				*/
					else {							/* Is not 'A' -> 'D', then let the escape 		*/
						ii = StrEsc[0];				/* sequence go through							*/
					}
				}
				else {								/* Is not an arrow key escape sequence, then	*/
					ii = StrEsc[0];					/* let the escape sequence go through			*/
				}
				memmove(&StrEsc[0], &StrEsc[1], sizeof(StrEsc)-1);
			}
		}

		switch (ii){								/* -------------------------------------------- */
		case 6:										/* ^F go forward one character					*/
			if (CmdPos < CmdLast) {
			  #if ((SHELL_HISTORY) > 0)
				putchar(g_CmdHist[HistEdit][CmdPos++]);	/* Write char at current cursor position	*/
			  #else
				putchar(Str[CmdPos++]);
			  #endif
			}
			break;
													/* -------------------------------------------- */
		case 2:										/* ^B go backward one character					*/
			if (CmdPos > 0) {
				putchar('\b');						/* Write a baskspace character					*/
				CmdPos--;
			}
			break;
													/* -------------------------------------------- */
		case 1:										/* ^A go to the beginning of the line			*/
			for( ; CmdPos>0 ; CmdPos--) {			/* Wrtie as many backspace char to move the		*/
				putchar('\b');						/* under the first char of the command line		*/
			}
			break;
													/* -------------------------------------------- */
		case 5:										/* ^E go to the end of the line					*/
			for( ; CmdPos<CmdLast ; CmdPos++) {
			  #if ((SHELL_HISTORY) > 0)
				putchar(g_CmdHist[HistEdit][CmdPos]);	/* Write all the characters starting at the	*/
			  #else									/* current cursor position						*/
				putchar(Str[CmdPos]);
			  #endif
			}
			break;
													/* -------------------------------------------- */
		case 14:									/* ^N - page down, fwd in time in the history	*/
		case 16:									/* ^P - page up, back in time in the history	*/
		  #if ((SHELL_HISTORY) > 0)
			kk = HistIdx;
			if (ii == 16) {							/* Back in time									*/
				if (HistIdx != g_HistOld) {			/* There are some older commands				*/
					if (++HistIdx > (SHELL_HISTORY)) {	/* Roll over in circular history buffer		*/
						HistIdx = 0;
					}
				}
			}
			else {									/* Forward in time								*/
				if (HistIdx != g_HistNew) {			/* There are some newer commands				*/
					if (--HistIdx < 0) {			/* Go forward in time							*/
						HistIdx = SHELL_HISTORY;	/* Roll over in circular history buffer			*/
					}								/* Go forward, including gHistNew as this is	*/
				}									/* the location currently being updated			*/
			}
			if (kk != HistIdx) {					/* When != we have to refresh the screen		*/
				for ( ; CmdPos>0 ; CmdPos--) {		/* Go at the beginning of currently displayed	*/
					putchar('\b');					/* command line									*/
				}
				printf("%s", &g_CmdHist[HistIdx][0]);
				jj = CmdLast
				   - strlen(&g_CmdHist[HistIdx][0]);
				if (jj > 0) {						/* Updated is shorter than the one overwritten	*/
					for (ii=0 ; ii<jj ; ii++) {	/* Overwrite with spaces the remainder			*/
						putchar(' ');
					}
					for (ii=0 ; ii<jj ; ii++) {	/* Go back to the end of the new command line	*/
						putchar('\b');
					}
				}
				CmdPos   = strlen(&g_CmdHist[HistIdx][0]);
				CmdLast  = CmdPos;					/* Update the command position and last char	*/
				HistEdit = HistIdx;					/* This is the command line displayed			*/

				UpdStr = 1;							/* When it won;t be a movement char, the		*/
			}										/* target history buffer string must be updated	*/
		  #endif
			break;
													/* -------------------------------------------- */
		default:									/* Non-movement characters						*/
		  #if ((SHELL_HISTORY) > 0)					/* Is a command from the history we are now		*/
			if (UpdStr != 0) {						/* manipulating, copy in destination			*/
				UpdStr = 0;							/* Don't copy if is the same					*/
				if (&Str[0] != &g_CmdHist[HistIdx][0]) {
					strcpy(&Str[0], &g_CmdHist[HistIdx][0]);
				}
				HistEdit = g_HistNew;
			}
		  #endif

			switch(ii) {							/* -------------------------------------------- */
			case '\r':								/* When <CR> or <LF>, done reading the input	*/
			case '\n':
				Done = 1;
				break;
													/* -------------------------------------------- */
			case 8:									/* ^H (BS) - delete previous character			*/
				if (CmdPos > 0) {
					CmdPos--;
					memmove(&Str[CmdPos], &Str[CmdPos+1], CmdLast-CmdPos);
					printf("\b%s ", &Str[CmdPos]);
					for (ii=CmdPos ; ii<CmdLast ; ii++) {
						putchar('\b');
					}
					CmdLast--;
				}
			break;

													/* -------------------------------------------- */
			default:								/* Non <CR>, <LF>, <BS>, insert the char		*/
				if (CmdLast < (SHELL_CMD_LEN)) {	/* Don't insert if the command line is full		*/
					if (ii == '\t') {				/* Remap tabs by spaces							*/
						ii = ' ';
					}
					if (ii >= 32) {					/* Don't insert control chars					*/
						if (CmdPos == CmdLast) {	/* At the end of the line						*/
							Str[CmdLast++] = ii;
							Str[CmdLast]   = '\0';
							CmdPos++;
							putchar(ii);
						}
						else {						/* Not at the end of the line					*/
							CmdLast++;				/* Shift right the chars after the cursor		*/
							memmove(&Str[CmdPos+1], &Str[CmdPos], CmdLast-CmdPos);
							Str[CmdPos] = ii;		/* Insert eh new char							*/
							printf("%s", &Str[CmdPos]);	/* Refresh display right of cursor location	*/

							for (ii=1 ; ii<(CmdLast-CmdPos) ; ii++) {
								putchar('\b');		/* Bring the sursor back to where it was		*/
							}
							CmdPos++;				/* Move the position one character right		*/
						}
					}
				}
				else {								/* Command line string full						*/
					putchar(7);						/* Ring the bell								*/
				}
				break;
			}
			break;
		}
	} while (Done == 0);

	putchar('\n');

  #if ((SHELL_HISTORY) > 0)
	strcpy(CmdStr, &g_CmdHist[g_HistNew][0]);		/* Put the final command into argument			*/

	while((*Str == ' ')								/* if it's an empty command line, don;t update	*/
	||    (*Str == '\t')) {							/* it in the history							*/
		Str++;
	}
	if (*Str != '\0') {
		if (--g_HistNew < 0) {						/* Make room for the new command				*/
			g_HistNew = SHELL_HISTORY;
		}
	}
  #endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* RETURN:																							*/
/*		== 0 : access is read-only																	*/
/*		!= 0 : access is full read-write															*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int ShellRdWrt(int *Result)
{
  #if ((SHELL_LOGIN) > 0)
	if (g_IsRdOnly != 0) {
		if (Result != NULL) {
			*Result = SHELL_RES_IS_RD_ONLY;
		}
		return(0);
	}
  #endif

	return(1);
}

/* ------------------------------------------------------------------------------------------------ */

#if (((OX_MAILBOX) != 0) && ((OX_GROUP) != 0) && !defined(__UABASSI_H__))

static MBX_t *FindMBX(const char *Name)
{
char  *Cptr;
int    ii;
MBX_t *MbxScan;

	if ((Name[0] == '0')
	&&  (Name[1] == 'x')) {
		MbxScan = (MBX_t *)(uintptr_t)strtoul(Name, &Cptr, 16);
		if (*Cptr == '\0') {
			return(MbxScan);
		}
	}
	else {
		MbxScan = (MBX_t *)NULL;
		for (ii=0 ; ii<((OX_HASH_MBX)+1U) ; ii++) {	/* Don't use MBXopen() because it will create	*/
		  #if ((OX_HASH_MBX) != 0U)					/* one when the mailbox doesn't exist			*/
			MbxScan = g_MboxList[ii];
		  #else
			MbxScan = g_MboxList;
		  #endif
			while (MbxScan != (MBX_t *)NULL) {		/* Traverse the list of mailboxes				*/
				if (0 == strcmp(Name, MbxScan->MbxName)) {
					return(MbxScan);
				}
				MbxScan = MbxScan->MbxNext;
			}
		}
	}
	return((MBX_t *)NULL);
}
#endif

/* ------------------------------------------------------------------------------------------------ */

static SEM_t *FindSEM(const char *Name)
{
char  *Cptr;
int    ii;
SEM_t *SemScan;

	if ((Name[0] == '0')
	&&  (Name[1] == 'x')) {
		SemScan = (SEM_t *)(uintptr_t)strtoul(Name, &Cptr, 16);
		if (*Cptr == '\0') {
			return(SemScan);
		}
	}
	else {
		SemScan  = (SEM_t *)NULL;
		for (ii=0 ; ii<((OX_HASH_SEMA)+1U) ; ii++) {/* Don't use SEMopen() bacuse it will create	*/
		  #if ((OX_HASH_SEMA) != 0U)				/* one when the semaphore doesn't exist			*/
			SemScan = g_SemaList[ii];
		  #else
			SemScan = g_SemaList;
		  #endif
			while (SemScan != (SEM_t *)NULL) {		/* Traverse the list of semaphores				*/
				if (0 == strcmp(Name, SemScan->SemName)) {
					return(SemScan);
				}
				SemScan = SemScan->SemaNext;
			}
		}
	}
	return((SEM_t *)NULL);
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* File commands																					*/
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */

#if ((SHELL_FILES) != 0)

static int FileCat(int argc, char *argv[]);
static int FileCd(int argc, char *argv[]);
static int FileChmod(int argc, char *argv[]);
static int FileCp(int argc, char *argv[]);
static int FileDu(int argc, char *argv[]);
static int FileErrno(int argc, char *argv[]);
static int FileFmt(int argc, char *argv[]);
static int FileHelp(int argc, char *argv[]);
static int FileLs(int argc, char *argv[]);
static int FileMkdir(int argc, char *argv[]);
static int FileMnt(int argc, char *argv[]);
static int FileMv(int argc, char *argv[]);
static int FilePerf(int argc, char *argv[]);
static int FilePwd(int argc, char *argv[]);
static int FileRm(int argc, char *argv[]);
static int FileRmdir(int argc, char *argv[]);
static int FileUmnt(int argc, char *argv[]);

static Cmd_t g_FileCommand[] = {					/* help & ? MUST REMAIN THE FIRST 2 ENTRIES		*/
	{ "help",   &FileHelp	},
	{ "?",      &FileHelp	},

	{ "cat",	&FileCat	},
	{ "cd",		&FileCd		},
	{ "chmod",	&FileChmod	},
	{ "cp",		&FileCp		},
	{ "du",		&FileDu		},
	{ "errno",	&FileErrno	},
	{ "fmt",	&FileFmt	},
	{ "ls",		&FileLs		},
	{ "mkdir",	&FileMkdir	},
	{ "mnt",	&FileMnt	},
	{ "mv",		&FileMv		},
	{ "perf",	&FilePerf	},
	{ "pwd",	&FilePwd	},
	{ "rm",		&FileRm		},
	{ "rmdir",	&FileRmdir	},
	{ "umnt",	&FileUmnt	}
};

													/* Align on cache lines if cached transfers		*/
static char g_Buffer[16384] __attribute__ ((aligned (OX_CACHE_LSIZE)));	/* Buffer used for all  I/O	*/

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    FileShell																			*/
/*																									*/
/* FileShell - add-on to the RTOS monitor/debug shell												*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int FileShell(int argc, char *argv[]);														*/
/*																									*/
/* ARGUMENTS:																						*/
/*		argc : number of entries in argv[]															*/
/*		argv : array of strings of each command line token											*/
/*		       argv[0] is the command string														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		int : == 0 no error																			*/
/*		      != 0 error																			*/
/*																									*/
/* DESCRIPTION:																						*/
/*		See DESCRIPTION in the header																*/
/*																									*/
/* **** DO NOT modify the code in this function ****												*/
/* ------------------------------------------------------------------------------------------------ */

int FileShell(int argc, char *argv[])
{
int ii;
int RetVal;

	RetVal = 99;
	if (argc != 0) {								/* It is not an empty line						*/
		for (ii=0 ; ii<(sizeof(g_FileCommand)/sizeof(g_FileCommand[0])); ii++) {
			if (0 == strcmp(g_FileCommand[ii].Name, argv[0])) {
				printf("\n");
				RetVal = g_FileCommand[ii].FctPtr(argc, argv);
				break;
			}
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* help command : help or ?																			*/
/*																									*/
/* There is no need to modify this command. To add help about a command, the command  must	 		*/
/* be added in function of the command itself.														*/
/*																									*/
/* **** DO NOT modify the code in this function ****												*//* ------------------------------------------------------------------------------------------------ */

static int FileHelp(int argc, char *argv[])
{
int ii;												/* General purpose								*/
int RetVal;											/* Return value									*/

	RetVal = 0;										/* Assume everything is OK						*/

	if (argc == 1) {								/* Print short help on all cmd, skip "help" "?"	*/
		printf("\nFile commands:\n");
		for (ii=2 ; ii<(sizeof(g_FileCommand)/sizeof(g_FileCommand[0])) ; ii++) {
			(void)g_FileCommand[ii].FctPtr(-1, NULL);
		}
	}
	else if (argc == 2) {							/* Print the help for the specified command		*/
		for (ii=2 ; ii<(sizeof(g_FileCommand)/sizeof(g_FileCommand[0])) ; ii++) {
			if (0 == strcmp(argv[1], g_FileCommand[ii].Name)) {
				(void)g_FileCommand[ii].FctPtr(-2, NULL);
				return(0);
			}
		}
		RetVal = 99;
	}
	else {
		RetVal = 1;
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */ 
static int FileCat(int argc, char *argv[])
{
int   Fd;											/* File descriptor with open()					*/
FILE *Fdsc;											/* File descriptor with fopen()					*/
char *Fname;										/* Name of the file to cat						*/
int   ii;											/* General purpose								*/
int   IsEOF;										/* If eod of file is reached					*/
int   IsWrite;										/* If writing to the file						*/
int   Nrd;											/* Number of bytes read							*/
int   RetVal;										/* Return value									*/

	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("cat   : Redirect a file to stdout or redirect stdin to a file");
		return(0);
	}

	if (argc < 2) {									/* Needs 1 or 2 options on the command line		*/
		RetVal = 1;
	}

	IsWrite = 0;
	if (RetVal == 0) {
		if (argv[1][0] == '>') {					/* Is a write to a file							*/
			if (ShellRdWrt(NULL) == 0) {
				PrtResult(NULL, SHELL_RES_IS_RD_ONLY,0, NULL, 0, 0);
				return(1);
			}
			IsWrite = 1;
			if (argv[1][1] != '\0') {				/* If no space between '>' & file name			*/
				Fname = &argv[1][1];				/* The file name starts at the 2nd character	*/
				if (argc != 2) {					/* Can only have 2 tokens then					*/
					RetVal = 1;
				}
			}
			else {									/* Space between '>' & file name				*/
				Fname = argv[2];					/* The file name is the 3rd token				*/
				if (argc != 3) {					/* Can only have 3 tokens then					*/
					RetVal = 1;
				}
			}
		}
		else if (argv[1][0] == '<'){				/* In case using > for write					*/
			if (argv[1][1] != '\0') {				/* If no space between '>' & file name			*/
				Fname = &argv[1][1];				/* The file name starts at the 2nd character	*/
				if (argc != 2) {					/* Can only have 2 tokens then					*/
					RetVal = 1;
				}
			}
			else {									/* Space between '>' & file name				*/
				Fname = argv[2];					/* The file name is the 3rd token				*/
				if (argc != 3) {					/* Can only have 3 tokens then					*/
					RetVal = 1;
				}
			}
		}
		else {
			Fname = &argv[1][0];
			if (argc != 2) {
				RetVal = 1;
			}
		}
	}


	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: cat  source_file           Output the file contents on the screen");
		puts("       cat <target_file           Output the file contents on the screen");
		puts("       cat >target_file           Write from terminal to the file");
		puts("                                  Terminate with EOF<CR>");
		RetVal = 1;
	}

	if (RetVal == 0) {
		if (IsWrite == 0) {							/* Dumping a file to stdout						*/
			Fd = open(Fname, O_RDONLY, 0777);
			if (Fd < 0 ) {
				printf("ERROR: cannot open file %s\n", Fname);
				RetVal = 1;
			}
			else {
				do {								/* Dump the file contents to stdio until EOF	*/
					Nrd = read(Fd, &g_Buffer[0], sizeof(g_Buffer));
					if (Nrd < 0) {
						puts("ERROR: problems reading the file");
						RetVal = 1;
						Nrd    = 0;
					}
					for (ii=0 ; ii<Nrd ; ii++) {
						putchar(g_Buffer[ii]);
					}
				} while (Nrd == sizeof(g_Buffer));
				if (0 != close(Fd)) {
					puts("ERROR: closing the file");
				}
			}
		}
		else {										/* Writing to a file							*/
			if (RetVal == 0) {						/* Using fopen() etc and not open() becauee		*/
				Fdsc = fopen(Fname, "w");			/* each call to write() does the physical write	*/
				if (Fdsc == NULL) {					/* This takes time and could wear some devices	*/
					printf("ERROR: cannot open file %s\n", Fname);
					RetVal = 1;
				}
				else {
					IsEOF = 0;						/* Make sure it is buffered						*/
					setvbuf(Fdsc, &g_Buffer[0], _IOFBF, sizeof(g_Buffer));
					do {
						ii = getchar();
						if (ii == EOF) {
							IsEOF = 1;
						}
						else {
							if (ii == (char)((SYS_CALL_TTY_EOF) & 0x7FFFFFFF)) {
								IsEOF = 1;
							}
							else {
								fputc(ii, Fdsc);
							}
						}
					}
					while (IsEOF == 0);
					if (0 != fclose(Fdsc)) {
						puts("ERROR: closing the file");
					}
				}
			}
		}
	}

	if (RetVal == 0) {
		putchar('\n');
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FileCd(int argc, char *argv[])
{
int   RetVal;										/* Return value									*/


	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("cd    : Change directory");
		return(0);
	}

	if (argc != 2) {								/* Need a directory name or ..					*/
		RetVal = 1;
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: cd dir_name                Change directory to dir_name or up with dir_name=..");
		RetVal = 1;
	}

	if (RetVal == 0) {
		if (0 != chdir(argv[1])) {
			puts("ERROR: cannot change directory");
			RetVal = 1;
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FileChmod(int argc, char *argv[])
{
int  RetVal;										/* Return value									*/
mode_t NewMode;

	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("chmod : Change a file / directory access modes");
		return(0);
	}

	if (argc != 3) {								/* Need a mode + directory name or a file name	*/
		RetVal = 1;
	}

	NewMode = 0;
	if (0 == strcmp(argv[1], "-w")) {
		NewMode = S_IRUSR|S_IRGRP|S_IROTH;
	}
	else if (0 == strcmp(argv[1], "+w")) {
		NewMode = S_IRUSR|S_IRGRP|S_IROTH
		        | S_IWUSR|S_IWGRP|S_IWOTH;
	}
	else {
		RetVal = 1;
	}

	if (ShellRdWrt(NULL) == 0) {
		PrtResult(NULL, SHELL_RES_IS_RD_ONLY,0, NULL, 0, 0);
		return(1);
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: chmod -w file_name        Change a file / directory to read-only");
		puts("       chmod +w file_name        Change a file / directory to read-write");
		RetVal = 1;
	}

	if (RetVal == 0) {
       if (0 != chmod(argv[2], NewMode)) {
			puts("ERROR: cannot chmod the file");
		  #if (((OS_DEMO) == 22) || ((OS_DEMO) == -22))
			puts("       chmod() always returns an error with ueFAT");
			puts("       The error is expected (See SysCall_ueFAT.c for more info");
		  #endif

			RetVal = 1;
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FileCp(int argc, char *argv[])
{
int   FdDst;										/* Destination file descriptor					*/
int   FdSrc;										/* Source file descriptor						*/
int   Nrd;											/* Number of bytes read							*/
int   Nwrt;											/* Number of bytes written						*/
int   RetVal;										/* Return value									*/

	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("cp    : Copy a file");
		return(0);
	}

	if (argc != 3) {								/* Need a source and destination file			*/
		RetVal = 1;
	}

	if (ShellRdWrt(NULL) == 0) {
		PrtResult(NULL, SHELL_RES_IS_RD_ONLY,0, NULL, 0, 0);
		return(1);
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: cp src_file dst_file       Copy the file src_file to dst_file");
		RetVal = 1;
	}

	if (RetVal == 0) {
		FdSrc = open(argv[1], O_RDONLY, 0777);
		if (FdSrc < 0) {
			puts("ERROR: cannot open src file");
			RetVal = 1;
		}
	}

	if (RetVal == 0) {
		FdDst = open(argv[2], O_RDWR|O_CREAT, 0777);
		if (FdDst < 0) {
			puts("ERROR: cannot open dst file");
			RetVal = 1;
			close(FdSrc);
		}
	}

	if (RetVal == 0) {
		do {
			Nrd = read(FdSrc, &g_Buffer[0], sizeof(g_Buffer));
			if (Nrd < 0) {
				puts("ERROR: problems reading the file");
				RetVal = 1;
				Nrd    = 0;
				Nwrt   = 0;
			}
			Nwrt = write(FdDst, &g_Buffer[0], Nrd);
			if (Nwrt < 0) {
				puts("ERROR: problems writing the file");
				RetVal = 1;
				Nrd    = 0;
			}
			if (Nrd != Nwrt) {
				puts("ERROR: problems writing the file");
				Nrd = 0;
			}
		} while (Nrd == sizeof(g_Buffer));

		close(FdSrc);
		close(FdDst);
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FileDu(int argc, char *argv[])
{
int All;											/* If dumping all file systems					*/
int Drv;
int ErrNoBack;										/* Could be trying a lot of drive				*/
int ii;												/* General purpose								*/
int RetVal;											/* Return value									*/
int StatRet;
struct statfs FSstat;								/* Statistics returned by statfs()				*/

	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("du    : Show disk usage");
		return(0);
	}

	All = (argc == 1) ? 1 : 0;

	if (argc > 2) 	{								/* 1 or 2 arguments only						*/
		RetVal = 1;
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: du                         Show the disk usage");
		RetVal = 1;
	}

	if (RetVal == 0) {
		for (ii=0 ; ii<10 ; ii++) {					/* Check the first 10 file systems				*/
			if (All == 0) {							/* Dumping the specified file system			*/
				StatRet =  statfs(argv[1], &FSstat);
				ii      = 100000;
				Drv     = 0;
			}
			else {									/* Dumping all file systems						*/
				ErrNoBack = errno;
				StatRet   = fstatfs(ii, &FSstat);
				errno     = ErrNoBack;
				Drv       = ii;
			}

			if (StatRet == 0) {
				printf("Device    : \'%s\'\n", &(FSstat.f_mntfromname[0]));
				printf("Mount on  : \'%s\'\n", &(FSstat.f_mntonname[0]));
				printf("FS type   : %s\n", &(FSstat.f_fstypename[0]));
				printf("Access    : %s\n", (FSstat.f_flags & MNT_RDONLY) ? "RO" : "R/W");
				printf("Disk size :%12llu bytes\n", (unsigned long long int)
				                                    (FSstat.f_bsize * FSstat.f_blocks));
				printf("Disk free :%12llu bytes\n", (unsigned long long int)
				                                    (FSstat.f_bsize * FSstat.f_bfree));
				printf("Disk used :%12llu bytes\n", (unsigned long long int)
				                                   (FSstat.f_bsize*(FSstat.f_blocks-FSstat.f_bfree)));
				printf("Sector    :%12llu bytes\n", (unsigned long long int)FSstat.f_bsize);

			  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0x000007020)
				if (0 == strncmp("SDMMC", media_info(Drv), 5)) {
					puts("\nOn ZYNQ, the two next fields are invalid for the SD-MMC\n");
				}
			  #endif

				printf("DEV size  :%12llu bytes\n", (unsigned long long)media_size(Drv));
				printf("DEV blocks:%12u bytes\n", (unsigned int)media_blksize(Drv));
				printf("\n");
			}
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FileErrno(int argc, char *argv[])
{
int RetVal;

	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("errno : Read or reset errno");
		return(0);
	}

	if (argc > 2) 	{								/* 1 or 2 arguments only						*/
		RetVal = 1;
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: errno                      Show the current value of errno");
		puts("usage: errno 0                    Reset errno to 0");
		RetVal = 1;
	}

	if ((RetVal == 0)
	&&  (argc == 1)) {
		printf("Errno value %d\n", errno);
	}

	if ((RetVal == 0)
	&&  (argc == 2)) {
		if (0 == strcmp(argv[1], "0")) {
			errno = 0;
		}
		else {
			RetVal = 1;
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FileFmt(int argc, char *argv[])
{
const char *Ftype;									/* File system type								*/
int         RetVal;									/* Return value									*/

  #ifdef FILE_SYSTEM_TYPE							/* File system specified, use it				*/
	Ftype = FILE_SYSTEM_TYPE;						/* File system not specified, try cmd line		*/
  #else
	if (argc == 3) {
		if (0 == strcasecmp("FAT16", argv[2])) {
			Ftype = FS_TYPE_NAME_FAT16;
		}
		else if (0 == strcasecmp("exFAT", argv[2])) {
			Ftype = FS_TYPE_NAME_EXFAT;
		}
		else if (0 == strcasecmp("FAT32", argv[2])) {
			Ftype = FS_TYPE_NAME_FAT32;
		}
		else {
			Ftype = FS_TYPE_NAME_AUTO;
		}
		argc = 2;
	}
	else {
		Ftype = FS_TYPE_NAME_AUTO;
	}
  #endif

	if (ShellRdWrt(NULL) == 0) {
		PrtResult(NULL, SHELL_RES_IS_RD_ONLY,0, NULL, 0, 0);
		return(1);
	}

	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("fmt   : Format a drive");
		puts("        fmt # [FAT16|FAT32|exFAT]");
	  #if (((OS_DEMO) == 29) || ((OS_DEMO) == -29))
		puts("        Can also specify the FS stack e.g. fmt #:FatFS FAT32");
	  #endif
		return(0);
	}

	if ((argc < 2)									/* Need a volume name to format					*/
	||  (argc > 3)) {
		RetVal = 1;
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: fmt device                 Format the device");
		RetVal = 1;
	}

	if (RetVal == 0) {
	  #if ((VERBOSE) > 0) 
		puts("Formatting started");
	  #endif
	  #if ((((OS_DEMO) == 21) || ((OS_DEMO) == -21))												\
	   ||  (((OS_DEMO) == 22) || ((OS_DEMO) == -22)))
		puts("This will take a while due to the full device erasing");
	  #endif
		if (0 != mkfs(Ftype, argv[1])) {
			puts("ERROR: format of the drive failed");
		  #if ((OS_DEMO) == 21) 					/* FullFAT always return error for format		*/
			puts("       mkfs() always returns an error with FullFAT");
			puts("       The error is expected (see SysCall_FullFAT.c for more info)");
		  #endif
			RetVal = 1;
		}
	}

  #if ((VERBOSE) > 0) 
	if (RetVal == 0) {
		puts("Done");
	}
  #endif

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FileLs(int argc, char *argv[])
{
DIR_t         *Dinfo;
struct dirent *DirFile;
struct stat    Finfo;
char           Fname[SYS_CALL_MAX_PATH+1];
char           MyDir[SYS_CALL_MAX_PATH+1];
struct tm     *Time;
int            RetVal;								/* Return value									*/

	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("ls    : List the current directory contents");
		return(0);
	}

	if (argc > 2) {									/* No argument or directory						*/
		RetVal = 1;
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: ls                         List the directory contents");
		RetVal = 1;
	}

	if (RetVal == 0) {								/* Refresh the current directory path			*/
		if (argc == 1) {
			if (NULL == getcwd(&MyDir[0], sizeof(MyDir))) {
				puts("ERROR: cannot obtain current directory");
				RetVal = 1;
			}
		}
		else {
			strcpy(&MyDir[0], argv[1]);
		}
	}

	if (RetVal == 0) {								/* Open the dir to see if it's there			*/
		if (NULL == (Dinfo = opendir(&MyDir[0]))) {
			puts("ERROR: cannot open directory");
			RetVal = 1;
		}
	}

	if (RetVal == 0) {								/* Valid directory, read each entries and print	*/
		errno = 0;
		while(NULL != (DirFile = readdir(Dinfo))) {
			if (DirFile->d_type == DT_LNK) {		/* This a mount point or part of a mount point	*/
				printf("lrwx (%-9s mnt point)           - ", media_info(DirFile->d_drv));
			}
			else {
				strcpy(&Fname[0], &MyDir[0]);
				strcat(&Fname[0], "/");
				strcat(&Fname[0], &(DirFile->d_name[0]));

				stat(&Fname[0], &Finfo);

				putchar((Finfo.st_mode & _IFDIR)  ? 'd' : ' ');
				putchar((Finfo.st_mode & S_IRUSR) ? 'r' : '-');
				putchar((Finfo.st_mode & S_IWUSR) ? 'w' : '-');
				putchar((Finfo.st_mode & S_IXUSR) ? 'x' : '-');

				Time = localtime(&Finfo.st_mtime);
				if (Time != NULL) {					/* ARM CC returns NULL							*/
					printf(" (%04d.%02d.%02d %02d:%02d:%02d) ", Time->tm_year + 1900,
					                                            Time->tm_mon,
					                                            Time->tm_mday,
					                                            Time->tm_hour,
					                                            Time->tm_min,
					                                            Time->tm_sec);
				}
				printf(" %10lu ", Finfo.st_size);
			}
			puts(DirFile->d_name);
		}
		closedir(Dinfo);							/* Done, close the directory					*/
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FileMkdir(int argc, char *argv[])
{
int RetVal;											/* Return value									*/


	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("mkdir : Make a new directory");
		return(0);
	}

	if (argc != 2) {								/* Need the name of the directory to create		*/
		RetVal = 1;
	}

	if (ShellRdWrt(NULL) == 0) {
		PrtResult(NULL, SHELL_RES_IS_RD_ONLY,0, NULL, 0, 0);
		return(1);
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: mkdir dir_name             Make a new directory with the name dir_name");
		RetVal = 1;
	}

	if (RetVal == 0) {
		if (0 != mkdir(argv[1], 0777)) {
			puts("ERROR: cannot create directory");
			RetVal = 1;
		}
	}
	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FileMnt(int argc, char *argv[])
{
char  *Cptr;										/* TO validate strtol()							*/
int    Drv;											/* Drive number									*/
DIR_t *MyDir;
int    RetVal;										/* Return value									*/

	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("mnt   : Mount a drive to a mount point e.g. mnt 0 /");
		puts("        Multi-FS : can also specify FS  e.g. mnt 0:FatFS /");
		return(0);
	}

	if (argc != 3) {								/* Need the volume and the mount point			*/
		RetVal = 1;
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: mnt device MntPoint        Mount a file system device number");
	}

	if (RetVal == 0) {
		if (argv[2][0] != '/') {
			printf("ERROR: the mount point must always start with /\n");
			RetVal = 1;
		}
	}

	if (RetVal == 0) {
		Drv = strtol(argv[1], &Cptr, 10);
		if ((*Cptr != '\0')
		&&  (*Cptr != ':')):
			printf("ERROR: invalid number for drive # (%s)\n", argv[1]);
			RetVal = 1;				
		}
		else if (media_info(Drv) == NULL) {
			printf("ERROR: drive # %s out of range\n", argv[1]);
			RetVal = 1;				
		}
	}

	if (RetVal == 0) {
		if (0 != mount(FS_TYPE_NAME_AUTO, argv[2], 0, argv[1])) {
			printf("ERROR: cannot mount volume %s on %s\n", argv[1], argv[2]);
			RetVal = 1;
		}
	}

	if (RetVal == 0) {
		MyDir =  opendir(argv[2]);
		if (MyDir == NULL) {
			printf("ERROR: cannot read / dir on volume %s\n", argv[1]);
			RetVal = 1;
		}
		else {
			closedir(MyDir);
		}
	}


  #if ((VERBOSE) > 0)
	if (RetVal == 0) {
		puts("The volume is now mounted");
	}
  #endif

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FileMv(int argc, char *argv[])
{
int  RetVal;										/* Return value									*/


	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("mv    : Move / rename a file");
		return(0);
	}


	if (argc != 3) {								/* Need a source and destination name			*/
		RetVal = 1;
	}

	if (ShellRdWrt(NULL) == 0) {
		PrtResult(NULL, SHELL_RES_IS_RD_ONLY,0, NULL, 0, 0);
		return(1);
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: mv file_name new_name      Rename file_name to new_name");
		RetVal = 1;
	}

	if (RetVal == 0) {
		if (0 != rename(argv[1], argv[2])) {
			RetVal = 1;
		}
	}
	if (RetVal != 0) {
		puts("ERROR: renaming the file");
		RetVal = 1;
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FilePwd(int argc, char *argv[])
{
char MyDir[SYS_CALL_MAX_PATH+1];
int  RetVal;										/* Return value									*/


	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("pwd   : Show current directory");
		return(0);
	}

	if (argc != 1) {								/* Does not accept arguments					*/
		RetVal = 1;
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: pwd                        Print the current working directory");
		RetVal = 1;
	}

	if (RetVal == 0) {
		if (NULL == getcwd(&MyDir[0], sizeof(MyDir))) {
			puts("ERROR: cannot get directory information");
			RetVal = 1;
		}
	}

	if (RetVal == 0) {
		printf("Current directory: %s\n", &MyDir[0]);
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FilePerf(int argc, char *argv[])
{
int    BlkSize;
char  *Buffer;
char  *Cptr;
char   Data;
int    Fd;
int    Left;
int    Nrd;
int    Nwrt;
int    RetVal;										/* Return value									*/
int    Size;
int    StartTime;
double Time;
static const char Fname[] = "__PERF__";


	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("perf  : Throughput measurements");
		return(0);
	}

	if ((argc < 3)									/* Need the size of the transfers				*/
	||  (argc > 4)) {
		RetVal = 1;
	}

	if (ShellRdWrt(NULL) == 0) {
		PrtResult(NULL, SHELL_RES_IS_RD_ONLY,0, NULL, 0, 0);
		return(1);
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: perf                       Measure the read and write transfer rates");
		puts("       perf Nbytes BlkSize [Data] Nbytes : file size to write then read");
		puts("                                  BlkSize: block size to use");
		puts("                                  Data   : byte use to fill");
		RetVal = 1;
	}

	if (RetVal == 0) {
		Size = (int)strtoul(argv[1], &Cptr, 10);
		if ((*Cptr=='k')
		||  (*Cptr=='K')) {
			Size *= 1024;
		}
		if ((*Cptr=='m')
		||  (*Cptr=='M')) {
			Size *= 1024*1024;
		}
		BlkSize = (int)strtoul(argv[2], &Cptr, 10);
		if ((*Cptr=='k')
		||  (*Cptr=='K')) {
			BlkSize *= 1024;
		}
		if ((*Cptr=='m')
		||  (*Cptr=='M')) {
			BlkSize *= 1024*1024;
		}

		Data = 0x55;
		if (argc == 4) {
			Data = (int)strtoul(argv[3], &Cptr, 0);
		}

		Buffer  = &g_Buffer[0];
		if (BlkSize > sizeof(g_Buffer)) {
			Buffer = malloc(BlkSize);
			if (Buffer == (char *)NULL) {
				puts("ERROR: cannot allocate memory");
				RetVal = 1;
			}

		}
		memset(&Buffer[0], Data, BlkSize);
		TSKsleep(2);								/* Do this to make sure the test always same	*/
		if (RetVal == 0) {
			printf("%d bytes file using R/W block size of %d bytes\n", Size, BlkSize);
			Fd = open(Fname, O_RDWR|O_CREAT, 0777);
			if (Fd < 0) {
				printf("ERROR: cannot open file %s\n", Fname);
				RetVal = 1;
			}
			else {
				puts("Starting test");
				Left      = Size;
				StartTime = G_OStimCnt;
				do {
					Nwrt = Left;
					if (Nwrt > BlkSize) {
						Nwrt = BlkSize;
					}
					Left -= Nwrt;
					Nrd = write(Fd, &Buffer[0], Nwrt);
					if (Nrd < 0) {
						puts("ERROR: problems writing the file");
						RetVal = 1;
						Left   = 0U;
					}
					else {
						if (Nrd != Nwrt) {
							puts("ERROR: problems writing the file");
							RetVal = 1;
							Left   = 0U;;
						}
					}
				} while (Left != 0U);
				StartTime = G_OStimCnt
				          - StartTime;

				close(Fd);

				Time = ((double)StartTime)/(1000000.0/OX_TIMER_US);
				if (RetVal == 0) {
					if (Time != 0) {
						printf("[%7.3lfs] Write rate %9.3lf kB/s\n",
						       Time, ((double)Size/1000.0)/Time);
					}
					else {
						printf("[%7.3lfs] Write rate --------- kB/s (Less than 1 timer tick)\n",
						       Time);
					}
				}
			}
			Fd = open(Fname, O_RDONLY, 0777);
			if ((RetVal == 0)
			&&  (Fd < 0)) {
				printf("ERROR: cannot open file %s\n", Fname);
				RetVal = 1;
			}
			else {
				Left      = Size;
				StartTime = G_OStimCnt;
				do {
					Nrd = Left;
					if (Nrd > BlkSize) {
						Nrd = BlkSize;
					}
					Left -= Nrd;
					Nrd = read(Fd, &Buffer[0], BlkSize);
					if (Nrd < 0) {
						puts("ERROR: problems reading the file");
						RetVal = 1;
						Left   = 0U;
					}
				} while (Left != 0U);
				StartTime = G_OStimCnt
				          - StartTime;

				close(Fd);

				Time = ((double)StartTime)/(1000000.0/OX_TIMER_US);
				if (RetVal == 0) {
					if (Time != 0) {
						printf("[%7.3lfs] Read rate  %9.3lf kB/s\n",
						       Time, ((double)Size/1000.0)/Time);
					}
					else {
						printf("[%7.3lfs] Read rate  --------- kB/s (Less than 1 timer tick)\n",
						       Time);
					}
				}
			}
		}
		if ((BlkSize > sizeof(g_Buffer))
		&&  (Buffer != (char *)NULL)) {
			free(Buffer);
		}
	}

	if (RetVal == 0) {
		if (0 != unlink(Fname)) {
			puts("ERROR: cannot remove the file");
			RetVal = 1;
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FileRm(int argc, char *argv[])
{
int RetVal;											/* Return value									*/
struct stat Finfo;

	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("rm    : Remove / delete a file");
		return(0);
	}

	if (argc != 2) {								/* Need the file name to delete					*/
		RetVal = 1;
	}

	if (ShellRdWrt(NULL) == 0) {
		PrtResult(NULL, SHELL_RES_IS_RD_ONLY,0, NULL, 0, 0);
		return(1);
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: rm file_name               Delete the file file_name");
		RetVal = 1;
	}

	if (RetVal == 0) {
		RetVal = stat(argv[1], &Finfo);
		if (RetVal != 0) {
			puts("ERROR: Can't get stats of the file");
		}
	}

	if (RetVal == 0) {
		if (Finfo.st_mode & S_IFDIR) {
			puts("ERROR: this is a directory, use rmdir");
			RetVal = 1;
		}
	}

	if (RetVal == 0) {
		if (0 != unlink(argv[1])) {
			puts("ERROR: cannot remove the file");
			RetVal = 1;
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FileRmdir(int argc, char *argv[])
{
int RetVal;											/* Return value									*/
struct stat Stat;

	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("rmdir : Remove / delete a directory");
		return(0);
	}

	if (argc != 2) {								/* Need the directory name to delete	   		*/
		RetVal = 1;
	}

	if (ShellRdWrt(NULL) == 0) {
		PrtResult(NULL, SHELL_RES_IS_RD_ONLY,0, NULL, 0, 0);
		return(1);
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: rmdir dir_name             Delete the directory dirname");
		RetVal = 1;
	}

	if (RetVal == 0) {
		RetVal = stat(argv[1], &Stat);
		if (RetVal != 0) {
			puts("ERROR: Can't get stats of the file");
		}
	}

	if (RetVal == 0) {
		if (0 == (Stat.st_mode & S_IFDIR)) {
			puts("ERROR: this is not a directory, use rm");
			RetVal = 1;
		}
	}

	if (RetVal == 0) {
		if (0 != unlink(argv[1])) {
			puts("ERROR: cannot remove the directory");
			RetVal = 1;
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */

static int FileUmnt(int argc, char *argv[])
{
int RetVal;											/* Return value									*/

	RetVal = 0;										/* Assume everything is OK						*/
	if (argc < 0) {									/* Special value to print usage					*/
		puts("umnt  : Unmount a mount point");
		return(0);
	}

	if (argc != 2) {								/* Need he volume and the mount point			*/
		RetVal = 1;
	}

	if (RetVal != 0) {								/* Print usage in case of error					*/
		puts("usage: umnt device                Unmount a mount point in the file system");
		RetVal = 1;
	}

	if (RetVal == 0) {
		if (0 != unmount(argv[1], 0)) {
			printf("ERROR: cannot unmount volume %s\n", argv[1]);
			RetVal = 1;
		}
	}

  #if ((VERBOSE) > 0)
	if (RetVal == 0) {
		puts("The volume is now unmounted");
	}
  #endif

	return(RetVal);
}

#endif

/* EOF */
