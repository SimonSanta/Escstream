/* ------------------------------------------------------------------------------------------------ */
/* FILE :		SubShell.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Application specific Sub-Shell template												*/
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
/*	$Revision: 1.6 $																				*/
/*	$Date: 2019/01/10 18:06:19 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* Application specific add-on to the Abassi debug/monitor shell to extend the RTOS debugging		*/
/* capabilities.																					*/
/*																									*/
/* All application-specific debug/monitor commands must be add in the CommandLst[] array			*/
/* All command have the same API:																	*/
/*		int cmd(int argcc, char *argv[]);															*/
/*		argc & argv are the same as the standard "C" main() function arguments						*/
/*		the return valeu is 0 when success, and != 0 when error										*/
/* When argc == -1, a one-line help / usage must be printed on stdout, alike						*/
/*		cmdX : enable disable the X operation														*/
/* when argc < -1, a full help must be printed on stdout, alike										*/
/*		usage : cmdX on|off																			*/
/*		        on  : enable X operation															*/
/*		        off : disable X oiperation															*/
/*																									*/
/* *** When adding a new command, the new command information must be added in g_CommandLst[] 		*/
/*																									*/
/* 2 example commands are provided: sub1 (function: CmdSub1()) and sub2 (function: CmdSub2())		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "Shell.h"

/* ------------------------------------------------------------------------------------------------ */
/* Apps variables																					*/

typedef struct {
	char *Name;
	int (* FctPtr)(int argc, char *argv[]);
} Cmd_t;

/* ------------------------------------------------------------------------------------------------ */
/* Apps functions																					*/

static int CmdHelp    (int argc, char *argv[]);
static int CmdSub1    (int argc, char *argv[]);
static int CmdSub2    (int argc, char *argv[]);

static Cmd_t g_CommandLst[] = {						/* help & ? MUST REMAIN THE FIRST 2 ENTRIES		*/
	{ "help",    &CmdHelp		},					/* help command MUST be provided				*/
	{ "?",       &CmdHelp		},					/* help command MUST be provided				*/

	{ "sub1",    &CmdSub1		},					/* Application specific command					*/
	{ "sub2",    &CmdSub2		}					/* Application specific command					*/
};													/* Add more as needed							*/

/* ------------------------------------------------------------------------------------------------ */

static struct {										/* Used by the sub1 and sub2 examples			*/
	int Cnt;										/* You can get rid of this if sub1 and sub2		*/
	int Err;										/* are removed									*/
} SysVar = {9145,533};;

/* ------------------------------------------------------------------------------------------------ */
/* Sub command example : sub1																		*/
/*																									*/
/* sub1 Cnt      :  show the value of SysVar.Cnt													*/
/* sub1 Err      :  show the value of SysVar.Err													*/
/* sub1 Cnt #### :  set  the value of SysVar.Cnt													*/
/* sub1 Err #### :  set  the value of SysVar.Err													*/
/*																									*/
/* *** When adding a new command, the new command information must be added in g_CommandLst[] 		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdSub1(int argc, char *argv[])
{
char *Cptr;
int   RetVal;
int   Value;
static struct {
	int Cnt;
	int Err;
} SysVar = {1234,42};;


	RetVal = 0;
	if (argc < 0) {									/* Special value to print usage					*/
		puts("sub1 : app specific operation #1");

		if (argc < -1) {
			puts("usage:");
			puts("       sub1 Cnt         (show value of SysVar.Cnt)");
			puts("       sub1 Err         (show value of SysVar.Err)");
			puts("       sub1 Cnt value   (set  value of SysVar.Cnt");
			puts("       sub1 Err value   (set  value of SysVar.Err");
		}

		return(0);
	}

	if ((argc != 2)									/* Check the number of argument on the cmd line	*/
	&&  (argc != 3)) {
		puts("usage: sub1 option [value]");			/* When too many or not enoygh error			*/
		return(1); 
	}

	if (argc == 2) {								/* sub1 command with field but no value			*/
													/* CMDLINE		sub1 Cnt						*/
		if (0 == strcmp("Cnt", argv[1])) {			/* Check argv[1] for the field to show value	*/
			printf("SysVar.Cnt = %d\n", SysVar.Cnt);
		}	
													/* CMDLINE		sub1 Err						*/
		else if (0 == strcmp("Err", argv[1])) {
			printf("SysVar.Err = %d\n", SysVar.Err);
		}
		else {
			RetVal = 1;								/* Report error for unknown field				*/
			puts("Use \"sub1 Cnt\" or \"sub1 Err\"");
		}
	}
	else {											/* sub1 command with field and value			*/
													/* CMDLINE		sub1 Cnt ###					*/
													/* CMDLINE		sub1 Err ###					*/
		if (ShellRdWrt(NULL)) {						/* Write only if write access privilege			*/
			Value = strtol(argv[2], &Cptr, 0);		/* Convert the value specified					*/
			if (*Cptr != '\0') {					/* Value is argv[2]								*/
				RetVal = 1;
				printf("\nERROR : invalid value: %s\n\n", argv[3]);
			}
			else {									/* Valid numerical value						*/
													/* CMDLINE		sub1 Cnt ###					*/
				if (0 == strcmp("Cnt", argv[1])) {	/* Check argv[1] for the field to set value		*/
					SysVar.Cnt = Value;
				}	
													/* CMDLINE		sub1 Err ###					*/
				else if (0 == strcmp("Err", argv[1])) {
					SysVar.Err = Value;
				}
				else {
					RetVal = 1;						/* Report error for unknown field				*/
					puts("Use \"sub1 Cnt\" or \"sub1 Err\"");
				}
			}
		}
		else {
			RetVal = 1;								/* Report error when read-only access			*/
			puts("Not updated: read-only access");
		}
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* Sub command example : sub2																		*/
/*																									*/
/* sub2           :  show the value of SysVar.Cnt and SysVar.Err									*/
/* sub2 show      :  show the value of SysVar.Cnt and SysVar.Err									*/
/* sub2 show Cnt  :  show the value of SysVar.Cnt													*/
/* sub2 show Err  :  show the value of SysVar.Err													*/
/* sub2 reset     :  reset all SysVar entries														*/
/* sub2 reset Cnt :  reset SysVar.Cnt																*/
/* sub2 reset Err :  reset SysVar.Err																*/
/*																									*/
/* *** When adding a new command, the new command information must be added in g_CommandLst[] 		*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

static int CmdSub2(int argc, char *argv[])
{
int   RetVal;


	RetVal = 0;
	if (argc < 0) {									/* Special value to print usage					*/
		puts("sub2 : app specific operation #2");

		if (argc < -1) {
			puts("usage:");
			puts("       sub2             (show  value of SysVar.Cnt and SysVar.Err)");
			puts("       sub2 show        (show  value of SysVar.Cnt and SysVar.Err)");
			puts("       sub2 show Cnt    (show  value of SysVar.Cnt)");
			puts("       sub2 show Err    (show  value of SysVar.Err)");
			puts("       sub2 reset       (reset value of SysVar.Cnt and SysVar.Err");
			puts("       sub2 reset Cnt   (reset value of SysVar.Cnt");
			puts("       sub2 reset Err   (reset value of SysVar.Err");
		}

		return(0);
	}

	if (argc > 3) {									/* Check the number of argument on the cmd line	*/
		puts("usage: sub2 oper [field]");			/* When too many error							*/
		return(1); 
	}

	if (argc == 1) {								/* sub2 with no operation and no field			*/
													/* CMDLINE		sub								*/
		printf("SysVar.Cnt = %d\n", SysVar.Cnt);
		printf("SysVar.Err = %d\n", SysVar.Err);
	}
	else if (argc == 2) {							/* sub2 command with operation and no field		*/
		if (0 == strcmp("show", argv[1])) {			/* Show request									*/
													/* CMDLINE		sub2 show						*/
			printf("SysVar.Cnt = %d\n", SysVar.Cnt);
			printf("SysVar.Err = %d\n", SysVar.Err);
		}
		else if (0 == strcmp("reset", argv[1])) {	/* Reset request								*/
													/* CMDLINE		sub2 reset						*/
			if (ShellRdWrt(NULL)) {					/* Reset only if write access privilege			*/
				SysVar.Cnt = 0;
				SysVar.Err = 0;
			}
			else {
				RetVal = 1;
				puts("Not reset: read-only access");
			}
		}
		else {
			RetVal = 1;								/* Report error for unknown operation			*/
			puts("Use sub2 reset");
		}
	}
	else {											/* sub2 command with operation and field		*/
		if (0 == strcmp("show", argv[1])) {			/* Dump field value								*/
													/* CMDLINE		sub2 show  Cnt					*/
													/* CMDLINE		sub2 show  Err					*/
			if (0 == strcmp("Cnt", argv[2])) {	/* Check argv[2] for the field to reset				*/
				printf("SysVar.Cnt = %d\n", SysVar.Cnt);
			}
			else if (0 == strcmp("Err", argv[2])) {
				printf("SysVar.Err = %d\n", SysVar.Err);
			}
			else {
				RetVal = 1;							/* Report error for unkown field				*/
				puts("Use \"sub2 show Cnt\" or \"sub2 show Err\"");
			}
		}
		else if (0 == strcmp("reset", argv[1])) {	/* Reset a field								*/
													/* CMDLINE		sub2 reset Cnt					*/
													/* CMDLINE		sub2 reset Err					*/
			if (ShellRdWrt(NULL)) {					/* Reset only if write access privilege			*/
				if (0 == strcmp("Cnt", argv[2])) {	/* Check argv[2] for the field to reset			*/
					SysVar.Cnt = 0;
				}
				else if (0 == strcmp("Err", argv[2])) {
					SysVar.Err = 0;
				}
				else {
					RetVal = 1;						/* Report error for unkown field				*/
					puts("Use \"sub2 reset Cnt\" or \"sub2 reset Err\"");
				}
			}
			else {
				RetVal = 1;							/* Report error when read-only access			*/
				puts("Not reset: read-only access");
			}
		}
		else {
			RetVal = 1;								/* Report error for unknown operation			*/
			puts("Use \"sub2 show|reset Cnt|Errt\"");
		}
	}

	return(RetVal);
}


/* ------------------------------------------------------------------------------------------------ */
/* **** Always include the code below																*/
/* **** DO NOT modify the code below ****															*/
/* ------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    SubShell																			*/
/*																									*/
/* SubShell - add-on to the RTOS monitor/debug shell												*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int SubShell(int argc, char *argv[]);														*/
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

int SubShell(int argc, char *argv[])
{
int ii;
int RetVal;

	RetVal = 99;
	if (argc != 0) {								/* It is not an empty line						*/
		for (ii=0 ; ii<(sizeof(g_CommandLst)/sizeof(g_CommandLst[0])); ii++) {
			if (0 == strcmp(g_CommandLst[ii].Name, argv[0])) {
				printf("\n");
				RetVal = g_CommandLst[ii].FctPtr(argc, argv);
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

static int CmdHelp(int argc, char *argv[])
{
int ii;												/* General purpose								*/
int RetVal;											/* Return value									*/

	RetVal = 0;										/* Assume everything is OK						*/

	if (argc == 1) {								/* Print short help on all cmd, skip "help" "?"	*/
		printf("\nApplication commands:\n");
		for (ii=2 ; ii<(sizeof(g_CommandLst)/sizeof(g_CommandLst[0])) ; ii++) {
			(void)g_CommandLst[ii].FctPtr(-1, NULL);
		}
	}
	else if (argc == 2) {							/* Print the help for the specified command		*/
		for (ii=2 ; ii<(sizeof(g_CommandLst)/sizeof(g_CommandLst[0])) ; ii++) {
			if (0 == strcmp(argv[1], g_CommandLst[ii].Name)) {
				(void)g_CommandLst[ii].FctPtr(-2, NULL);
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

/* EOF */
