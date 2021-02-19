/* ------------------------------------------------------------------------------------------------ */
/* FILE :		CGI_SSI.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Code for the lwIP webserver															*/
/*				This file contains the CGI, POST and SSI functions and variables					*/
/*				Yes, a bit clubbered but the same file is used for all ports						*/
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
/*	$Revision: 1.40 $																				*/
/*	$Date: 2019/01/10 18:07:14 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* How this works...																				*/
/* This file contains all the CGI & SSI supported by all Abassi ports. This code is used with the	*/
/* the web pages located in the fs directory.  The web pages are also the same across all ports.	*/
/* The whole demo depends heavily on the SSI variables to enable the followings:					*/
/* 		- Top bottons redirecting to speficif pages													*/
/*		- Controls what is displayed on each pages													*/
/*		- how many LEDs, Buttons, Switches, LCD lines etc the traget platform has					*/
/*																									*/
/* Top buttons / pages are enable with the follwing variable name construction:						*/
/*		SHOW_????_PAGE																				*/
/*		is set to "NO" or "YES", and ???? can be:													*/
/*							BUT  : Push buttons page												*/
/*							I2CC : Control of all I2C devices										*/
/*							I2CS : Status of all I2C devices										*/
/*							LCD  : LCD set-up (when not on a I2C bus)								*/
/*							LED  : Led control page													*/
/*							SW   : Switches page													*/
/* 							RTC  : Real Time Clock set-up page (When not on I2C bus)				*/
/*																									*/
/* What is displayed in each page is controlled by another SSI variable construction:				*/
/*		SHOW_?????																					*/
/*		is set to "NO" or "YES", and ???? can be:													*/
/*							ACC  : show accelerometer related information 							*/
/*							ADC  : show the Analog to digital converters(s) information				*/
/*							LCD  : show the LCD information											*/
/*							RTC  : show the real-time clokc information								*/
/*							TEMP : show the temperature sensor information							*/
/*																									*/
/* Finally, things like LCD, LED, Swithes, Button are provided with extra informattion to set-up	*/
/* how many line the LCD has, the number of LEDs, buttons, switches etc.  In all cases, ? is a		*/
/* digit from 0 to 9																				*/
/* The device that are controlled are set to "NO" or "YES"											*/
/*							LCD_HAS_LINE_? 															*/
/*							LED?_PRESENT															*/
/* The devices with status report are set to "ON" or "OFF" or "---" (the # does not exist)			*/
/*							BUT?																	*/
/*							SW?																		*/
/* or they have custom strings, e.g. the variable DATE_LOCAL that holds the current date/time		*/
/*																									*/
/* All SSI non-constant variables are updated by dedicated code and it's the webserver itslef that	*/
/* calls the updater functions.  All updaters are called when a new page is sent to the client		*/
/*																									*/
/* All CGIs are also handled with dedicated code and it's also the webserver that calls the 		*/
/* appropriate function according to the the POST token.											*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "WebApp.h"
	
/* ------------------------------------------------------------------------------------------------ */

#ifndef HOST_NAME									/* This is from Platform.h. Check in case		*/
  #define HOST_NAME "unknown target"				/* the application does not reply on it			*/
#endif

/* ------------------------------------------------------------------------------------------------ */

#define NIBBLING(x)	( (((x)/10)<<4)      + ((x)%10) )	/* To convert RTC <--> numerical values		*/
#define UNNIBBLE(x) (((((x)>>4)&0xF)*10) + ((x)&0xF))

#ifndef STM_RTC_USE_XTAL
  #define STM_RTC_USE_XTAL	0						/* Set to != 0 when 32.768 kHz is available		*/
#endif												/* Was not tested with xtal						*/

static volatile int g_Dummy;

/* ------------------------------------------------------------------------------------------------ */

#if defined(MY_BUTTON_0) || defined(MY_SWITCH_0)
  static const char g_SwitchState[2][4] = {"OFF", "ON"};
#endif

#ifdef ADC_TASK_PRIO								/* If the ADC reading done in a low prio task	*/
  int32_t ADCdata[16];
#endif

#ifdef osCMSIS
  extern int G_TaskLedFlashOff;
 #if ((I2C_USE_ADC) != 0)
  #ifdef ADC_TASK_PRIO
  void LwIP_ADC_task(const void *Arg);
  osThreadDef(LwIP_ADC_task, osPriorityLow, 1, 4096);
  #endif
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* RTOS task update dynamic page																	*/
/* ------------------------------------------------------------------------------------------------ */
#define STRCAT(s)		do {																		\
							int _len = strlen(s);													\
							if ((_len+1) < SizeOut) {												\
								strcat(&BufOut[0], s);												\
								SizeOut -= _len;													\
							}																		\
						} while(0)																	\

static char *WebPageRTOS(char *BufOut, int SizeOut)
{
#ifdef osCMSIS
  if (SizeOut > 1024) {
	BufOut[0] = '\0';
	STRCAT("<br>--------------------------------------------------------------------<br>");
	STRCAT("RTOS task info is not available with CMSIS RTOS API");
	STRCAT("<br>--------------------------------------------------------------------<br>");
  }
  return(&BufOut[strlen(BufOut)]);

#elif ((OX_NAMES) == 0)
	BufOut[0] = '\0';
	STRCAT("<br>--------------------------------------------------------------------<br>");
	STRCAT("RTOS task info is not available when OS_NAMES == 0");
	STRCAT("<br>--------------------------------------------------------------------<br>");
	return(&BufOut[strlen(BufOut)]);

#else

int             CoreN;
char            DispCnt[10];
int             ii;
int             jj;
int             Len;
int             LenTmp;
TSK_t          *TskList;
TSK_t          *TskRun[OX_N_CORE];

extern TSK_t  *g_TaskList;
static char    State[4][8] = {"Ready  ",
                              "Running",
                              "Suspend",
                              "Blocked"};

  #ifndef __ABASSI_H__
	for (ii=0 ; ii<OX_N_CORE ; ii++) {
		TskRun[ii] = G_OStaskNow[ii];
	}
  #else
	TskRun[0] = G_OStaskNow;
  #endif

	BufOut[0] = '\0';

  #if ((OX_N_CORE) == 1)
   #if ((OX_STACK_CHECK) != 0)
	STRCAT ("<pre><br>Name            State  Prio  Stack");
	STRCAT ("<br>----------------------------------<br>");
   #else
	STRCAT ("<pre><br>Name            State  Prio");
	STRCAT ("<br>---------------------------<br>");
   #endif
  #else
   #if ((OX_STACK_CHECK) != 0)
	STRCAT ("<pre><br>Name            State  Prio  Stack  Core");
	STRCAT ("<br>----------------------------------------<br>");
   #else
	STRCAT ("<pre><br>Name            State  Prio  Core");
	STRCAT ("<br>---------------------------------<br>");
   #endif
  #endif
	Len = strlen(BufOut);


	for (ii=0 ; ii<=OX_PRIO_MIN; ii++) {
		TskList = g_TaskList;
		while(TskList != (TSK_t *)NULL) {
			if (TskList->Prio == ii) {
				jj    = TSKstate(TskList);
				CoreN = OX_N_CORE;
				if	    (jj > 0) { jj = 3; }
				else if (jj < 0) { jj = 2; }
				else {
					for (CoreN=0 ; CoreN<OX_N_CORE ; CoreN++) {
						if (TskList == TskRun[CoreN]) {
							jj = 1;
							break;
						}
					}
				}
			  #if ((OX_STACK_CHECK) != 0)
				{ int kk;
					kk = TSKstkChk(TskList, 0);
					if (kk < 0) {
						strcpy(&DispCnt[0], "    N/A");
					}
					else {
						sprintf(&DispCnt[0], "%7d", TSKstkChk(TskList, 0));
					}
				}
			  #else
				DispCnt[0] = '\0';
			  #endif

				if (SizeOut > 512) {					/* Much larger than real					*/
				  #if ((OX_N_CORE) == 1)
					LenTmp = sprintf(&BufOut[Len], "\r\n%-15s%s  %2d %s",
					                 TskList->TskName, &State[jj][0], TskList->Prio, &DispCnt[0]);
				  #else
					LenTmp = sprintf(&BufOut[Len], "\r\n%-15s%s  %2d %s    %c",
					                 TskList->TskName, &State[jj][0], TskList->Prio, &DispCnt[0],
				    	             (CoreN == OX_N_CORE) ? ' ' : '0'+CoreN);
				  #endif
					SizeOut -= LenTmp;
					Len     += LenTmp;
				}
			}
			TskList = TskList->TaskNext;
		}
	}
	if (SizeOut > 512) {								/* Much larger than real					*/
		LenTmp = 0;
	  #if ((OX_N_CORE) == 1)
	   #if ((OX_STACK_CHECK) != 0)
		LenTmp = sprintf(&BufOut[Len], "<br><br>----------------------------------\n\n");
	   #else
		LenTmp = sprintf(&BufOut[Len], "<br><br>---------------------------\n\n");
	   #endif
	  #else
	   #if ((OX_STACK_CHECK) != 0)
		LenTmp = sprintf(&BufOut[Len], "<br><br>----------------------------------------\n\n");
	   #else
		LenTmp = sprintf(&BufOut[Len], "<br><br>---------------------------------\n\n");
	   #endif
	  #endif
		SizeOut -= LenTmp;
		Len     += LenTmp;
	}

	if (SizeOut > 512) {						/* Much larger than real					*/
		LenTmp   = sprintf(&BufOut[Len], "RTOS Timer Tick %10u\n\n", G_OStimCnt);
		SizeOut -= LenTmp;
		Len     += LenTmp;
	}

	return(&BufOut[Len]);

#endif
}

/* ------------------------------------------------------------------------------------------------ */
/* Send text to the LCD display																		*/
/* POST: already split in parameters and values														*/
/* Is attached to the POST dispatcher with POSTnewHandler()											*/
/* ------------------------------------------------------------------------------------------------ */

const char *LCD_POST_Handler(int NumParam, char *Params[], char *Values[])
{
  int   ii;											/* Loop counter									*/
  int   jj;											/* Line index									*/
  char *Lines[10];
  char  Names[] = "line0";

	memset(&Lines[0], 0, sizeof(Lines));

	for (ii=0 ; ii<NumParam ; ii++) {				/* Analyze all parameters						*/
		for (jj=0 ; jj<10 ; jj++) {					/* We have to compare with "line0" -> "line9"	*/
			Names[4] = '0' + jj;					/* Do char substitution to keep code compact	*/
			if (0 == strcmp(Params[ii] , Names)){	/* Matching line #, memo the string				*/
				Lines[jj] = Values[ii];
			}
		}
	}

	for (ii=0 ; ii<(MY_LCD_NMB_LINE) ; ii++) {
		MY_LCD_CLRLINE(ii);
		if (Lines[ii] != NULL) {
			MY_LCD_LINE(ii, Lines[ii]);
		}
	}

  #if ((I2C_USE_LCD) != 0)
	return("/I2C.shtml");							/* Web page to send back to the client			*/

  #elif ((MY_LCD_NMB_LINE) > 0)
	return("/LCD.shtml");							/* Web page to send back to the client			*/

  #else
	return(NULL);

  #endif
}

/* ------------------------------------------------------------------------------------------------ */
/* Set the RTC																						*/
/* POST: already split in parameters and values														*/
/* Is attached to the POST dispatcher with POSTnewHandler()											*/
/* ------------------------------------------------------------------------------------------------ */

const char *RTC_POST_Handler(int NumParam, char *Params[], char *Values[])
{
#if (((OS_PLATFORM) == 0x00032F407) || ((I2C_USE_RTC) != 0))
char Buffer[33];									/* Holds the data to send to the RTC			*/
int  Date[6];										/* Hold the date numbers from the user			*/
int  ii;											/* Loop counter									*/
int  dw;											/* For computing day of the week				*/
int  d;												/* Day for computing day of the week			*/
int  m;												/* Month for computing day of the week			*/
int  y;												/* Year for computing day of the week			*/
#if ((OS_PLATFORM) == 0x00032F407)
  RTC_DateTypeDef DateRTC;
  RTC_TimeTypeDef TimeRTC;
#endif

	memset(&Date[0], 0, sizeof(Date));				/* Assign default values in case a CGI is not	*/
	Date[3] = 1;									/* in the list									*/
	Date[4] = 1;
	Date[5] = 2000;

	for (ii=0 ; ii<NumParam ; ii++) {
		if ((0 == strcmp(Params[ii] , "sec"))
		&&  (Values[ii][0] != '\0')) {
			Date[0] = strtol(Values[ii], NULL, 10);
		}
		if ((0 == strcmp(Params[ii] , "min"))
		&&  (Values[ii][0] != '\0')) {
			Date[1] = strtol(Values[ii], NULL, 10);
		}
		if ((0 == strcmp(Params[ii] , "hrs"))
		&&  (Values[ii][0] != '\0')) {
			Date[2] = strtol(Values[ii], NULL, 10);
		}
		if ((0 == strcmp(Params[ii] , "day"))
		&&  (Values[ii][0] != '\0')) {
			Date[3] = strtol(Values[ii], NULL, 10);
		}
		if ((0 == strcmp(Params[ii] , "mth"))
		&&  (Values[ii][0] != '\0')) {
			Date[4] = strtol(Values[ii], NULL, 10);
		}
		if ((0 == strcmp(Params[ii] , "yr"))
		&&  (Values[ii][0] != '\0')) {
			Date[5] = strtol(Values[ii], NULL, 10);
		}
	}

	y  = Date[5];									/* Compute what day of the week this is			*/
	m  = Date[4];
	d  = Date[3];
	d += (m < 3)
	   ? y--
	   : y-2;
	dw = ((23*m/9+d+4+y/4-y/100+y/400)%7);

	memset(&Buffer[0], 0, sizeof(Buffer));

  #if ((OS_PLATFORM) == 0x00032F407)
	DateRTC.RTC_WeekDay = dw+1;

	TimeRTC.RTC_Seconds = Date[0];
	TimeRTC.RTC_Minutes = Date[1];
	TimeRTC.RTC_Hours   = Date[2];
	DateRTC.RTC_Date    = Date[3];
	DateRTC.RTC_Month   = Date[4];
	DateRTC.RTC_Year    = Date[5]-2000;

	RTC_SetTime(RTC_Format_BIN, &TimeRTC);
	RTC_SetDate(RTC_Format_BIN, &DateRTC);

	return("/RTC.shtml");							/* Web page to send back to the client			*/

  #elif ((I2C_USE_RTC) != 0)

   #if ((I2C_RTC_TYPE) == 1339)
	Buffer[1] = (char)NIBBLING(Date[0]);			/* Seconds										*/
	Buffer[2] = (char)NIBBLING(Date[1]);			/* Minutes										*/
	Buffer[3] = (char)NIBBLING(Date[2]);			/* Hours										*/
	Buffer[4] = (char)(dw+1);						/* Day of the Week (RTC is 1 -> 7)				*/
	Buffer[5] = (char)NIBBLING(Date[3]);			/* Date											*/
	Buffer[6] = (char)NIBBLING(Date[4]);			/* Month										*/
	Buffer[7] = (char)NIBBLING(Date[5]-2000);		/* Year (year 00 is 2000)						*/

	Buffer[0] = 0x00;								/* We use the I2C start/stop to take snapshot	*/
	i2c_send(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[0], 8);	/* Buffer[0] is register address		*/

   #elif ((I2C_RTC_TYPE) == 4182)
	Buffer[1] = (char)NIBBLING(Date[0]);			/* Seconds										*/
	Buffer[2] = (char)NIBBLING(Date[1]);			/* Minutes										*/
	Buffer[3] = (char)NIBBLING(Date[2]);			/* Hours										*/
	Buffer[4] = (char)(dw+1);						/* Day of the Week (RTC is 1 -> 7)				*/
	Buffer[5] = (char)NIBBLING(Date[3]);			/* Date											*/
	Buffer[6] = (char)NIBBLING(Date[4]);			/* Month										*/
	Buffer[7] = (char)NIBBLING(Date[5]-2000);		/* Year (year 00 is 2000)						*/

	Buffer[0] = 0x01;								/* We use the I2C start/stop to take snapshot	*/
	i2c_send(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[0], 31);
   #endif

	return("/I2C.shtml");							/* Web page to send back to the client			*/

  #else
	return(NULL);
  #endif

#else
	return(NULL);

#endif
}

/* ------------------------------------------------------------------------------------------------ */
/* Control the on-board LEDs																		*/
/* CGI: already split in parameters and values														*/
/* ------------------------------------------------------------------------------------------------ */

const char *LEDS_CGI_Handler(int NumParam, char *Params[], char *Values[])
{
#ifndef MY_LED_0
  NumParam = NumParam;
  Params   = Params;
  Values   = Values;
#else
int ii;												/* Loop counter and index						*/
#ifndef osCMSIS
  TSK_t *TaskLed;
#endif

  #ifdef osCMSIS
	G_TaskLedFlashOff = 1;
  #else
	TaskLed = TSKgetID("LED Flash");
	if (TaskLed != (TSK_t *)NULL) {
		TSKsuspend(TaskLed);						/* LED N is controlled by the LEDFlash Task		*/
	}
  #endif											/* So suspend the task and then turn LED N off	*/

  #ifdef MY_LED_0
	MY_LED_OFF(MY_LED_0);							/* Turn off all the LEDS first					*/
  #endif
  #ifdef MY_LED_1
	MY_LED_OFF(MY_LED_1);
  #endif
  #ifdef MY_LED_2
	MY_LED_OFF(MY_LED_2);
  #endif
  #ifdef MY_LED_3
	MY_LED_OFF(MY_LED_3);
  #endif
  #ifdef MY_LED_4
	MY_LED_OFF(MY_LED_4);
  #endif
  #ifdef MY_LED_5
	MY_LED_OFF(MY_LED_5);
  #endif
  #ifdef MY_LED_6
	MY_LED_OFF(MY_LED_6);
  #endif
  #ifdef MY_LED_7
	MY_LED_OFF(MY_LED_7);
  #endif
  #ifdef MY_LED_8
	MY_LED_OFF(MY_LED_8);
  #endif
  #ifdef MY_LED_9
	MY_LED_OFF(MY_LED_9);
  #endif

	for (ii=0 ; ii<NumParam ; ii++) {				/* Get param: "GET /leds.cgi?led=1&led=2..."	*/
		if (0 == strcmp(Params[ii] , "led")){
			if (0 == strcmp(Values[ii], "0")) { 
			  #ifdef MY_LED_0
				MY_LED_ON(MY_LED_0);
			  #endif
			}
			if (0 == strcmp(Values[ii], "1")) { 
			  #ifdef MY_LED_1
				MY_LED_ON(MY_LED_1);
			  #endif
			}
			if (0 == strcmp(Values[ii], "2")) { 
			  #ifdef MY_LED_2
				MY_LED_ON(MY_LED_2);
			  #endif
			}
			if (0 == strcmp(Values[ii], "3")) { 
			  #ifdef MY_LED_3
				MY_LED_ON(MY_LED_3);
			  #endif
			}
			if (0 == strcmp(Values[ii], "4")) { 
			  #ifdef MY_LED_4
				MY_LED_ON(MY_LED_4);
			  #endif
			}
			if (0 == strcmp(Values[ii], "5")) { 
			  #ifdef MY_LED_5
				MY_LED_ON(MY_LED_5);
			  #endif
			}
			if (0 == strcmp(Values[ii], "6")) { 
			  #ifdef MY_LED_6
				MY_LED_ON(MY_LED_6);
			  #endif
			}
			if (0 == strcmp(Values[ii], "7")) { 
			  #ifdef MY_LED_7
				MY_LED_ON(MY_LED_7);
			  #endif
			}
			if (0 == strcmp(Values[ii], "8")) { 
			  #ifdef MY_LED_8
				MY_LED_ON(MY_LED_8);
			  #endif
			}
			if (0 == strcmp(Values[ii], "9")) { 
			  #ifdef MY_LED_9
				MY_LED_ON(MY_LED_9);
			  #endif
			}
		  #ifdef MY_LED_FLASH_S
			if (0 == strcmp(Values[ii], MY_LED_FLASH_S)) { 
				MY_LED_ON(MY_LED_FLASH);
			  #ifdef osCMSIS
				G_TaskLedFlashOff = 0;
			  #else
				if (TaskLed != (TSK_t *)NULL) {
					TSKresume(TaskLed);
				}
			  #endif
			}
		  #endif
		}
	}
#endif
	return("/LED.shtml");							/* Web page to send back to the client			*/
}

/* ------------------------------------------------------------------------------------------------ */
/* ADC LT2497 reader																				*/
/* ------------------------------------------------------------------------------------------------ */

#if ((I2C_USE_ADC) != 0)
 #if (((I2C_ADC_TYPE)&0xFFFFFF) == 0x2497)

int LT2497read(int Address, int32_t *Data)
{
char    Buffer[3];
int     ii;
int32_t MyData;
int     RetVal;

	RetVal = 0;
	for (ii=0 ; ii<8 ; ii++) {
		Buffer[0] = 0xA0							/* 1 / 0 / EN=1 / SGL=0 / ODD=0 				*/
		          |  (char)(0x07 & ii);				/* A2 / A1 / A0									*/
		RetVal    |= i2c_send(I2C_ADC_DEVICE, Address, &Buffer[0], 1);

	  #ifdef osCMSIS								/* ADC takes around 150 ms to convert			*/
		osDelay(175);								/* Perform a Start / Read / Start conversion	*/
	  #else
		TSKsleep(OS_MS_TO_TICK(175));				/* ADC takes around 150 ms to convert			*/
	  #endif										/* Perform a Start / Read / Start conversion	*/

		RetVal   |= i2c_send_recv(I2C_ADC_DEVICE, Address, &Buffer[0], 1, &Buffer[0], 3);

		MyData  = (((unsigned int)(Buffer[0]))<<16)
		        | (((unsigned int)(Buffer[1]))<<8)
		        |  ((unsigned int)(Buffer[2]));
		if (MyData == 0x00C00000) {					/* Max +ve is > max 2's complement positive		*/
			MyData -= 1;							/* Print it a single bit less positive			*/
		}
		MyData  ^= 0x00800000;						/* Sign is inverse of 2's complement			*/
		if ((MyData & 0x00800000) != 0) {			/* Sign extend from 24 bits to 32 bits			*/
			MyData |= 0xFF000000;
		}
		Data[ii] = MyData;
	}
	return(RetVal);
}
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* ADC data update Task																				*/
/* This task is used because reading 16 ADC values takes more than 2 second and it gives the		*/
/* impression the Ethernet link is down																*/
/* ------------------------------------------------------------------------------------------------ */

#if ((I2C_USE_ADC) != 0)
 #ifdef ADC_TASK_PRIO
   #ifdef osCMSIS
void LwIP_ADC_task(const void *Arg)
   #else
void ADCtask(void)
   #endif
{

	for (;;) {
	  #if (((I2C_ADC_TYPE)&0xFFFFFF) == 0x2497)
	   #if (((I2C_ADC_TYPE)&0xFF000000) == 0x00000000)
		LT2497read(I2C_ADC_ADDR, &ADCdata[0]);
	   #elif (((I2C_ADC_TYPE)&0xFF000000) == 0x02000000)
		LT2497read(I2C_ADC_ADDR_0, &ADCdata[0]);
		LT2497read(I2C_ADC_ADDR_1, &ADCdata[8]);
	   #else
		TSKselfSusp();
       #endif
	  #else
		TSKselfSusp();
	  #endif
	}
}
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Read the ADC and insert the text in the TEMP0 & TEMP 1 variables									*/
/* This function is attached to SSIupdate() with SSInewUpdate()										*/
/* If an ADC task has been created, it gets the data from ADCdata[]									*/
/* Other wise, it reads the ADC directly															*/
/* ------------------------------------------------------------------------------------------------ */

void ADCupdate(void)
{
#if ((I2C_USE_ADC) != 0)
#ifndef ADC_TASK_PRIO
  int32_t Data[8];
  int     ii;
#endif
char    VarVal[64];

  #if (((I2C_ADC_TYPE)&0xFFFFFF) == 0x2497)
	#if (((I2C_ADC_TYPE)&0xFF000000) == 0x02000000)
	 #ifdef ADC_TASK_PRIO
		sprintf(&VarVal[0], "PCI:      %6.1lfmA",
		        1000.0*0.15*(((double)ADCdata[0])/4194304.0)/0.01);
		SSIsetVar("ADC_0", &VarVal[0]);
		sprintf(&VarVal[0], "FMA:      %6.1lfmA",
		        1000.0*0.15*(((double)ADCdata[1])/4194304.0)/0.01);
		SSIsetVar("ADC_1", &VarVal[0]);
		sprintf(&VarVal[0], "FMB:      %6.1lfmA",
		        1000.0*0.15*(((double)ADCdata[2])/4194304.0)/0.01);
		SSIsetVar("ADC_2", &VarVal[0]);
		sprintf(&VarVal[0], "FMAVAD:   %6.1lfmA",
		        1000.0*0.15*(((double)ADCdata[3])/4194304.0)/0.001);
		SSIsetVar("ADC_3", &VarVal[0]);
		sprintf(&VarVal[0], "FMBVAD:   %6.1lfmA",
		         1000.0*0.15*(((double)ADCdata[4])/4194304.0)/0.001);
		SSIsetVar("ADC_4", &VarVal[0]);
		sprintf(&VarVal[0], "2.5V:     %6.1lfmA",
		         1000.0*0.15*(((double)ADCdata[8])/4194304.0)/0.001);
		SSIsetVar("ADC_5", &VarVal[0]);
		sprintf(&VarVal[0], "HLVDD:    %6.1lfmA",
		         1000.0*0.15*(((double)ADCdata[9])/4194304.0)/0.001);
		SSIsetVar("ADC_6", &VarVal[0]);
		sprintf(&VarVal[0], "HLVDDQ:   %6.1lfmA",
		         1000.0*0.15*(((double)ADCdata[10])/4194304.0)/0.001);
		SSIsetVar("ADC_7", &VarVal[0]);
		sprintf(&VarVal[0], "HPSVDD:   %6.1lfmA",
		         1000.0*0.15*(((double)ADCdata[11])/4194304.0)/0.001);
		SSIsetVar("ADC_8", &VarVal[0]);
		sprintf(&VarVal[0], "1.8V:     %6.1lfmA",
		         1000.0*0.15*(((double)ADCdata[12])/4194304.0)/0.00025);
		SSIsetVar("ADC_9", &VarVal[0]);
		sprintf(&VarVal[0], "1.0V:     %6.1lfmA",
		         1000.0*0.15*(((double)ADCdata[13])/4194304.0)/0.00025);
		SSIsetVar("ADC_10", &VarVal[0]);
		sprintf(&VarVal[0], "0.95V:    %6.1lfmA",
		         1000.0*0.15*(((double)ADCdata[14])/4194304.0)/0.001);
		SSIsetVar("ADC_11", &VarVal[0]);
		sprintf(&VarVal[0], "HPS0.95V: %6.1lfmA",
		         1000.0*0.15*(((double)ADCdata[15])/4194304.0)/0.001);
		SSIsetVar("ADC_12", &VarVal[0]);

     #else

	  ii = LT2497read(I2C_ADC_ADDR_0, &Data[0]);
	  if (ii == 0) {
		sprintf(&VarVal[0], "PCI:      %6.1lfmA",1000.0*0.15*(((double)Data[0])/4194304.0)/0.01);
		SSIsetVar("ADC_0", &VarVal[0]);
	  }
	  else {
		SSIsetVar("ADC_0", "(UNAVAIL / I2C read error)");
	  }
	  if (ii == 0) {
		sprintf(&VarVal[0], "FMA:      %6.1lfmA",1000.0*0.15*(((double)Data[1])/4194304.0)/0.01);
		SSIsetVar("ADC_1", &VarVal[0]);
	  }
	  else {
		SSIsetVar("ADC_1", "(UNAVAIL / I2C read error)");
	  }
	  if (ii == 0) {
		sprintf(&VarVal[0], "FMB:      %6.1lfmA",1000.0*0.15*(((double)Data[2])/4194304.0)/0.01);
		SSIsetVar("ADC_2", &VarVal[0]);
	  }
	  else {
		SSIsetVar("ADC_2", "(UNAVAIL / I2C read error)");
	  }
	  if (ii == 0) {
		sprintf(&VarVal[0], "FMAVAD:   %6.1lfmA",1000.0*0.15*(((double)Data[3])/4194304.0)/0.001);
		SSIsetVar("ADC_3", &VarVal[0]);
	  }
	  else {
		SSIsetVar("ADC_3", "(UNAVAIL / I2C read error)");
	  }
	  if (ii == 0) {
		sprintf(&VarVal[0], "FMBVAD:   %6.1lfmA",1000.0*0.15*(((double)Data[4])/4194304.0)/0.001);
		SSIsetVar("ADC_4", &VarVal[0]);
	  }
	  else {
		SSIsetVar("ADC_4", "(UNAVAIL / I2C read error)");
	  }

	  ii = LT2497read(I2C_ADC_ADDR_1, &Data[0]);
	  if (ii == 0) {
		sprintf(&VarVal[0], "2.5V:     %6.1lfmA",1000.0*0.15*(((double)Data[0])/4194304.0)/0.001);
		SSIsetVar("ADC_5", &VarVal[0]);
	  }
	  else {
		SSIsetVar("ADC_5", "(UNAVAIL / I2C read error)");
	  }
	  if (ii == 0) {
		sprintf(&VarVal[0], "HLVDD:    %6.1lfmA",1000.0*0.15*(((double)Data[1])/4194304.0)/0.001);
		SSIsetVar("ADC_6", &VarVal[0]);
	  }
	  else {
		SSIsetVar("ADC_6", "(UNAVAIL / I2C read error)");
	  }
	  if (ii == 0) {
		sprintf(&VarVal[0], "HLVDDQ:   %6.1lfmA",1000.0*0.15*(((double)Data[2])/4194304.0)/0.001);
		SSIsetVar("ADC_7", &VarVal[0]);
	  }
	  else {
		SSIsetVar("ADC_7", "(UNAVAIL / I2C read error)");
	  }
	  if (ii == 0) {
		sprintf(&VarVal[0], "HPSVDD:   %6.1lfmA",1000.0*0.15*(((double)Data[3])/4194304.0)/0.001);
		SSIsetVar("ADC_8", &VarVal[0]);
	  }
	  else {
		SSIsetVar("ADC_8", "(UNAVAIL / I2C read error)");
	  }
	  if (ii == 0) {
		sprintf(&VarVal[0], "1.8V:     %6.1lfmA",1000.0*0.15*(((double)Data[4])/4194304.0)/0.00025);
		SSIsetVar("ADC_9", &VarVal[0]);
	  }
	  else {
		SSIsetVar("ADC_9", "(UNAVAIL / I2C read error)");
	  }
	  if (ii == 0) {
		sprintf(&VarVal[0], "1.0V:     %6.1lfmA",1000.0*0.15*(((double)Data[5])/4194304.0)/0.00025);
		SSIsetVar("ADC_10", &VarVal[0]);
	  }
	  else {
		SSIsetVar("ADC_10", "(UNAVAIL / I2C read error)");
	  }
	  if (ii == 0) {
		sprintf(&VarVal[0], "0.95V:    %6.1lfmA",1000.0*0.15*(((double)Data[6])/4194304.0)/0.001);
		SSIsetVar("ADC_11", &VarVal[0]);
	  }
	  else {
		SSIsetVar("ADC_11", "(UNAVAIL / I2C read error)");
	  }
	  if (ii == 0) {
		sprintf(&VarVal[0], "HPS0.95V: %6.1lfmA",1000.0*0.15*(((double)Data[7])/4194304.0)/0.001);
		SSIsetVar("ADC_12", &VarVal[0]);
	  }
	  else {
		SSIsetVar("ADC_12", "(UNAVAIL / I2C read error)");
	  }
	 #endif
	#endif
  #endif
#endif
	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Read the MAX1619 and insert the text in the TEMP0 variable										*/
/* This function is attached to SSIupdate() with SSInewUpdate()										*/
/* ------------------------------------------------------------------------------------------------ */

void MAX1619update(void)
{
#if ((I2C_USE_MAX1619) != 0)
char Buffer[32];									/* Holds the data read/written from/to			*/
int  ii;
char VarVal[64];
													/* Note MAC1619 can only read a single byte		*/
	Buffer[0] = 0x00;								/* Read local temperature (RLTS)				*/
	ii = i2c_send_recv(I2C_MAX1619_DEVICE, I2C_MAX1619_ADDR, &Buffer[0], 1, &Buffer[0], 1);
	sprintf(&VarVal[0], "Board: %3.1lf", 32.0+(9.0*((double)Buffer[0])/5.0));
	if (ii == 0) {									/* Page display F, MAX1619 reports in C			*/
		SSIsetVar("TEMP_1", &VarVal[0]);
	}
	else {
		SSIsetVar("TEMP_1", "(UNAVAIL / I2C read error)");
	}

	Buffer[0] = 0x01;								/* Read remote temperature (RRTE)				*/
	ii = i2c_send_recv(I2C_MAX1619_DEVICE, I2C_MAX1619_ADDR, &Buffer[0], 1, &Buffer[0], 1);
	sprintf(&VarVal[0], "FPGA:  %3.1lf", 32.0+(9.0*((double)Buffer[0])/5.0));
	if (ii == 0) {									/* Page display F, MAX1619 reports in C			*/
		SSIsetVar("TEMP_0", &VarVal[0]);
	}
	else {
		SSIsetVar("TEMP_0", "(UNAVAIL / I2C read error)");
	}
#endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Read the RTC and insert the text in the DATE_LOCAL variable										*/
/* This function is attached to SSIupdate() with SSInewUpdate()										*/
/* ------------------------------------------------------------------------------------------------ */

void RTCupdate(void)
{

#if ((OS_PLATFORM) == 0x00032F407)
  RTC_DateTypeDef Date;
  char            RTCstr[64];
  RTC_TimeTypeDef Time;
  static const char g_DayOfWeek[7][4]   = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char g_MonthNames[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	RTC_GetTime(RTC_Format_BIN, &Time);
	RTC_GetDate(RTC_Format_BIN, &Date);

	sprintf(&RTCstr[0], "%s, %02d-%s-%04d  %02d:%02d:%02d", &g_DayOfWeek[Date.RTC_WeekDay-1][0],
	                 Date.RTC_Date, &g_MonthNames[Date.RTC_Month-1][0], 2000+(int)Date.RTC_Year,
		            Time.RTC_Hours, Time.RTC_Minutes, Time.RTC_Seconds);
	SSIsetVar("DATE_LOCAL", &RTCstr[0]);

#elif ((I2C_USE_RTC) != 0)
  char  Buffer[32];									/* Holds the data read from the RTC				*/
  int   Date[7];									/* Date in textual format						*/
  int   ii;											/* General purpose								*/
  char  VarVal[64];
  static const char g_DayOfWeek[7][4]   = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char g_MonthNames[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  #if ((I2C_RTC_TYPE) == 1339)
	Buffer[0] = 0x00;								/* Request to read from register 0x00			*/
	ii = i2c_send_recv(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[0], 1, &Buffer[0], 7);
	if (ii == 0) {
		Date[0] = (int)UNNIBBLE(Buffer[0]);			/* Seconds										*/
		Date[1] = (int)UNNIBBLE(Buffer[1]);			/* Minutes										*/
		Date[2] = (int)UNNIBBLE(Buffer[2]);			/* Hours										*/
		Date[6] = (int)UNNIBBLE(Buffer[3])-1;		/* Day of the Week (RTC is 1 -> 7)				*/
		Date[3] = (int)UNNIBBLE(Buffer[4]);			/* Date											*/
		Date[4] = (int)UNNIBBLE(Buffer[5])-1;		/* Month (RTC is 1 -> 12)						*/
		Date[5] = (int)UNNIBBLE(Buffer[6])+2000;	/* Year (RTC is 00 -> 99)						*/
		sprintf(&VarVal[0], "%s, %02d-%s-%04d  %02d:%02d:%02d", &g_DayOfWeek[Date[6]][0],Date[3], 
		                 &g_MonthNames[Date[4]][0], Date[5], Date[2], Date[1], Date[0]);
		SSIsetVar("DATE_LOCAL", &VarVal[0]);
	}
	else {
		SSIsetVar("DATE_LOCAL", "(UNAVAIL / I2C read error)");
	}
  #elif ((I2C_RTC_TYPE) == 4182)
	Buffer[0]  = 0x0C;								/* Clear HT bit: is set when back from Battery	*/
	ii         = i2c_send(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[0], 1);
	ii        |= i2c_recv(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[1], 1);
	Buffer[1] &= 0xBF;
	ii        |= i2c_send(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[0], 2);

	Buffer[0] = 0x01;								/* Request to start reading from second reg		*/
	ii       |= i2c_send(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[0], 1);
	ii       |= i2c_recv(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[0], 7);

	if (ii == 0) {
		Buffer[0] &= 0x7F;							/* Remove the controls bits						*/
		Buffer[2] &= 0x3F;							/* Remove the Century bits						*/

		Date[0] = (int)UNNIBBLE(Buffer[0]);			/* Seconds										*/
		Date[1] = (int)UNNIBBLE(Buffer[1]);			/* Minutes										*/
		Date[2] = (int)UNNIBBLE(Buffer[2]);			/* Hours										*/
		Date[6] = (int)UNNIBBLE(Buffer[3])-1;		/* Day of the Week (RTC is 1 -> 7)				*/
		Date[3] = (int)UNNIBBLE(Buffer[4]);			/* Date											*/
		Date[4] = (int)UNNIBBLE(Buffer[5])-1;		/* Month (RTC is 1 -> 12)						*/
		Date[5] = (int)UNNIBBLE(Buffer[6])+2000;	/* Year (RTC is 00 -> 99)						*/
		sprintf(&VarVal[0], "%s, %02d-%s-%04d  %02d:%02d:%02d", &g_DayOfWeek[Date[6]][0],Date[3], 
		                 &g_MonthNames[Date[4]][0], Date[5], Date[2], Date[1], Date[0]);
		SSIsetVar("DATE_LOCAL", &VarVal[0]);
	}
	else {
		SSIsetVar("DATE_LOCAL", "(UNAVAIL / I2C read error)");
	}
  #endif
#endif

	return;	
}

/* ------------------------------------------------------------------------------------------------ */
/* Read the G-sensor and insert the text in the DATE_LOCAL variable									*/
/* This function is attached to SSIupdate() with SSInewUpdate()										*/
/* ------------------------------------------------------------------------------------------------ */

void XL345update(void)
{
#if ((I2C_USE_XL345) != 0)
char Buffer[7];
int  RetVal;
char VarVal[64];
int  Xaxis;
int  Yaxis;
int  Zaxis;


	Buffer[0]  = XL345_REG_POWER_CTL;
	Buffer[1]  = XL345_MEASURE;						/* Start the measurements						*/

	RetVal     = i2c_send(I2C_XL345_DEVICE, I2C_XL345_ADDR, &Buffer[0], 2);

  #ifdef osCMSIS
	osDelay(100);									/* Wait for the measurement to be done			*/
  #else
	TSKsleep(OS_MS_TO_TICK(100));					/* Wait for the measurement to be done			*/
  #endif

	do {											/* Check to make sure is done					*/
		Buffer[0] = XL345_REG_INT_SOURCE;
		RetVal = i2c_send_recv(I2C_XL345_DEVICE, I2C_XL345_ADDR, &Buffer[0], 1, &Buffer[0], 1);
	} while (((XL345_DATAREADY & (int)Buffer[0]) == 0)
	  &&      (RetVal == 0));

	Buffer[0]  = 0x32;								/* Read the data; *4 because 0.004g/bit			*/
	RetVal |= i2c_send_recv(I2C_XL345_DEVICE, I2C_XL345_ADDR, &Buffer[0], 1, &Buffer[0], 6);

	if (RetVal == 0) {
		Xaxis = 4*((((int)(signed char)Buffer[1])<<8) + (0xFF & ((int)Buffer[0])));
		Yaxis = 4*((((int)(signed char)Buffer[3])<<8) + (0xFF & ((int)Buffer[2])));
		Zaxis = 4*((((int)(signed char)Buffer[5])<<8) + (0xFF & ((int)Buffer[4])));
		sprintf(&VarVal[0], "%5dmg", Xaxis);
		SSIsetVar("ACCEL_X", &VarVal[0]);
		sprintf(&VarVal[0], "%5dmg", Yaxis);
		SSIsetVar("ACCEL_Y", &VarVal[0]);
		sprintf(&VarVal[0], "%5dmg", Zaxis);
		SSIsetVar("ACCEL_Z", &VarVal[0]);
	}
	else {
		SSIsetVar("ACCEL_X", "---   ");
		SSIsetVar("ACCEL_Y", "---   ");
		SSIsetVar("ACCEL_Z", "---   ");
	}
	Buffer[0]  = XL345_REG_POWER_CTL;
	Buffer[1]  = XL345_STANDBY;						/* Stop the measurements						*/
	RetVal |= i2c_send(I2C_XL345_DEVICE, I2C_XL345_ADDR, &Buffer[0], 2);

#endif
	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Update the variables BUT0 --> BUT9 to "ON" or "OFF"												*/
/* Push Button value read of zero is ON, non-zero is OFF											*/
/* This function is attached to SSIupdate() with SSInewUpdate()										*/
/* ------------------------------------------------------------------------------------------------ */

void ButtonUpdate(void)
{

  #if defined(MY_BUTTON_0)
	SSIsetVar("BUT0", &g_SwitchState[0 == BUT_GET(MY_BUTTON_0)][0]);
  #endif

  #if defined(MY_BUTTON_1)
	SSIsetVar("BUT1", &g_SwitchState[0 == BUT_GET(MY_BUTTON_1)][0]);
  #endif

  #if defined(MY_BUTTON_2)
	SSIsetVar("BUT2", &g_SwitchState[0 == BUT_GET(MY_BUTTON_2)][0]);
  #endif

  #if defined(MY_BUTTON_3)
	SSIsetVar("BUT3", &g_SwitchState[0 == BUT_GET(MY_BUTTON_3)][0]);
  #endif

  #if defined(MY_BUTTON_4)
	SSIsetVar("BUT4", &g_SwitchState[0 == BUT_GET(MY_BUTTON_4)][0]);
  #endif

  #if defined(MY_BUTTON_5)
	SSIsetVar("BUT5", &g_SwitchState[0 == BUT_GET(MY_BUTTON_5)][0]);
  #endif

  #if defined(MY_BUTTON_6)
	SSIsetVar("BUT6", &g_SwitchState[0 == BUT_GET(MY_BUTTON_6)][0]);
  #endif

  #if defined(MY_BUTTON_7)
	SSIsetVar("BUT7", &g_SwitchState[0 == BUT_GET(MY_BUTTON_7)][0]);
  #endif

  #if defined(MY_BUTTON_8)
	SSIsetVar("BUT8", &g_SwitchState[0 == BUT_GET(MY_BUTTON_8)][0]);
  #endif

  #if defined(MY_BUTTON_9)
	SSIsetVar("BUT9", &g_SwitchState[0 == BUT_GET(MY_BUTTON_9)][0]);
  #endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Update the variables SW0 --> SW9 to "ON" or "OFF"												*/
/* Switches value read of zero is ON, non-zero is OFF												*/
/* This function is attached to SSIupdate() with SSInewUpdate()										*/
/* ------------------------------------------------------------------------------------------------ */

void SwitchUpdate(void)
{

  #if defined(MY_SWITCH_0)
	SSIsetVar("SW0", &g_SwitchState[0 == SW_GET(MY_SWITCH_0)][0]);
  #endif

  #if defined(MY_SWITCH_1)
	SSIsetVar("SW1", &g_SwitchState[0 == SW_GET(MY_SWITCH_1)][0]);
  #endif

  #if defined(MY_SWITCH_2)
	SSIsetVar("SW2", &g_SwitchState[0 == SW_GET(MY_SWITCH_2)][0]);
  #endif

  #if defined(MY_SWITCH_3)
	SSIsetVar("SW3", &g_SwitchState[0 == SW_GET(MY_SWITCH_3)][0]);
  #endif

  #if defined(MY_SWITCH_4)
	SSIsetVar("SW4", &g_SwitchState[0 == SW_GET(MY_SWITCH_4)][0]);
  #endif

  #if defined(MY_SWITCH_5)
	SSIsetVar("SW5", &g_SwitchState[0 == SW_GET(MY_SWITCH_5)][0]);
  #endif

  #if defined(MY_SWITCH_6)
	SSIsetVar("SW6", &g_SwitchState[0 == SW_GET(MY_SWITCH_6)][0]);
  #endif

  #if defined(MY_SWITCH_7)
	SSIsetVar("SW7", &g_SwitchState[0 == SW_GET(MY_SWITCH_7)][0]);
  #endif

  #if defined(MY_SWITCH_8)
	SSIsetVar("SW8", &g_SwitchState[0 == SW_GET(MY_SWITCH_8)][0]);
  #endif

  #if defined(MY_SWITCH_9)
	SSIsetVar("SW9", &g_SwitchState[0 == SW_GET(MY_SWITCH_9)][0]);
  #endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Send text to the LCD																				*/
/* ------------------------------------------------------------------------------------------------ */

void LCDputs(char *Line_1, char *Line_2)
{
#if ((I2C_USE_LCD) != 0)
int Len;											/* Length of the string to send					*/

//static const char LCDclr[]     =  {0xFE, 0x51};	/* Clear display command						*/
static const char LCDlin[2][3] = {{0xFE, 0x45, 0x00},	/* Set cursor at beginning of line 1		*/
                                  {0xFE, 0x45, 0x40}};	/* Set cursor at beginning of line 2		*/

  #if ((I2C_LCD_TYPE) == 2163)
   #if 0
	i2c_send(I2C_LCD_DEVICE, I2C_LCD_ADDR, &LCDclr[0], 2);	/* Clear the LCD						*/

   #ifdef osCMSIS
	osDelay(20);									/* LCD very slow between transactions			*/
   #else
	TSKsleep(OS_MS_TO_MIN_TICK(2,2));				/* LCD very slow between transactions			*/
   #endif
  #endif

	if (Line_1 != NULL) {
		Len = strlen(Line_1);						/* Check if the string is empty					*/
		if (Len > 0) {								/* Non-empty string								*/
			if (Len > 16) {							/* The Cyclone V display line are 16 chars long	*/
				Len = 16;
			}
			i2c_send(I2C_LCD_DEVICE, I2C_LCD_ADDR, &LCDlin[0][0], 3);/* Go to the correct line		*/
		  #ifdef osCMSIS
			osDelay(20);							/* Need ~10us, use the smallest valid timeout	*/
		  #else
			TSKsleep(OS_MS_TO_MIN_TICK(2,2));
			TSKsleep(2);							/* Need ~10us, use the smallest valid timeout	*/
		  #endif
   	 		i2c_send(I2C_LCD_DEVICE, I2C_LCD_ADDR, Line_1, Len);	/* Send the string to display	*/
		  #ifdef osCMSIS
			osDelay(20);
		  #else
			TSKsleep(OS_MS_TO_MIN_TICK(2,2));
		  #endif
		}
	}

	if (Line_2 != NULL) {
		Len = strlen(Line_2);						/* Check if the string is empty					*/
		if (Len > 0) {								/* Non-empty string								*/
			if (Len > 16) {							/* The Cyclone V display line are 16 chars long	*/
				Len = 16;
			}
			i2c_send(I2C_LCD_DEVICE, I2C_LCD_ADDR, &LCDlin[1][0], 3);/* Go to the correct line		*/
		  #ifdef osCMSIS
			osDelay(20);							/* Need ~10us, use the smallest valid timeout	*/
		  #else
			TSKsleep(OS_MS_TO_MIN_TICK(2,2));
			TSKsleep(2);							/* Need ~10us, use the smallest valid timeout	*/
		  #endif
	   	 	i2c_send(I2C_LCD_DEVICE, I2C_LCD_ADDR, Line_2, Len);	/* Send the string to display	*/
		  #ifdef osCMSIS
			osDelay(20);
		  #else
			TSKsleep(OS_MS_TO_MIN_TICK(2,2));
		  #endif
		}
	}
  #endif
#endif
	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Set-up all CGI handlers																			*/
/* ------------------------------------------------------------------------------------------------ */

void CGIinit(void)
{ 

	CGInewHandler("/leds.cgi", LEDS_CGI_Handler);

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Set-up all the POST handlers																		*/
/* ------------------------------------------------------------------------------------------------ */

void POSTinit(void)
{ 
  #if ((MY_LCD_NMB_LINE) > 0)
	POSTnewHandler("/lcd.cgi",  LCD_POST_Handler);
  #endif
  #if ((I2C_USE_RTC) != 0) || ((OS_PLATFORM) == 0x032F407)
	POSTnewHandler("/rtc.cgi",  RTC_POST_Handler);
  #endif

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Set-up the MAX1619 temperature monitor															*/
/* ------------------------------------------------------------------------------------------------ */

int MAX1619init(void)
{
#if ((I2C_USE_MAX1619) != 0)
char Buffer[32];									/* Holds the data read/written from/to			*/
int  RetVal;										/* Return value : bus error						*/

	RetVal = 0;
	Buffer[0] = 0x09;								/* Write configuration byte (WCA) reg #0x09		*/
	Buffer[1] = 0x80;								/* Mask Alert interrupt							*/
	RetVal |= i2c_send(I2C_MAX1619_DEVICE, I2C_MAX1619_ADDR, &Buffer[0], 2);

	Buffer[0] = 0x0A;								/* Write conversion rate byte (WCRW) reg #0x0A	*/
	Buffer[1] = 0x07;								/* Maximum conversion rate: 8 Hz				*/
	RetVal |= i2c_send(I2C_MAX1619_DEVICE, I2C_MAX1619_ADDR, &Buffer[0], 2);

	Buffer[0] = 0x0E;								/* Write remote T low  (WCLN) reg #0x0E			*/
	Buffer[1] = 0x7F;								/* Set to lowest: not interested with Alters	*/
	RetVal |= i2c_send(I2C_MAX1619_DEVICE, I2C_MAX1619_ADDR, &Buffer[0], 2);

	Buffer[0] = 0x0D;								/* Write remote T high (WCHA) reg #0x0D			*/
	Buffer[1] = 0x80;								/* Set to highest: not interested with Alters	*/
	RetVal |= i2c_send(I2C_MAX1619_DEVICE, I2C_MAX1619_ADDR, &Buffer[0], 2);

	return(RetVal);
#else
	return(0);
#endif
}

/* ------------------------------------------------------------------------------------------------ */
/* ADXL345 accelerometer init																		*/
/* ------------------------------------------------------------------------------------------------ */

int XL345init(void)
{
#if ((I2C_USE_XL345) != 0)
char Buffer[2];
int  RetVal;

	Buffer[0]  = XL345_REG_DATA_FORMAT;
	Buffer[1]  = (XL345_RANGE_2G)					/* +- 2G range, 10 bit resolution				*/
	           | (XL345_FULL_RESOLUTION);			/* This delivers results of 4mg per bit			*/
	RetVal     = i2c_send(I2C_XL345_DEVICE, I2C_XL345_ADDR, &Buffer[0], 2);

	Buffer[0]  = XL345_REG_BW_RATE;
	Buffer[1]  = XL345_RATE_12_5;					/* Conversion rate is 12.5 Hz (once every 80ms)	*/
	RetVal    |= i2c_send(I2C_XL345_DEVICE, I2C_XL345_ADDR, &Buffer[0], 2);

	Buffer[0]  = XL345_REG_INT_ENABLE;
	Buffer[1]  = XL345_DATAREADY;					/* Enable the interrupt for data ready			*/
	RetVal    |= i2c_send(I2C_XL345_DEVICE, I2C_XL345_ADDR, &Buffer[0], 2);

	Buffer[0]  = XL345_REG_POWER_CTL;
	Buffer[1]  = XL345_STANDBY;						/* Stop the measurements						*/
	RetVal    |= i2c_send(I2C_XL345_DEVICE, I2C_XL345_ADDR, &Buffer[0], 2);

 	return(RetVal);
#else
	return(0);
#endif
}

/* ------------------------------------------------------------------------------------------------ */
/* Set-up all server variables and variables update handlers										*/
/* ------------------------------------------------------------------------------------------------ */

void SSIinit(void)
{
char DemoVal[4];								/* Temp to convert from number to string			*/
int  RTCpresent;								/* RTC non-standard on some platform e.g. Zedboard	*/
#if ((OS_PLATFORM) == 0x00032F407)
  RTC_InitTypeDef RTCsetup;
#endif
												/* ------------------------------------------------ */
	RTCpresent = 0;								/* I2C peripheral devices init						*/
												/* ------------------------------------------------ */
	g_Dummy = RTCpresent;						/* To remove compiler warning when not used			*/

	if (0 != MAX1619init()) {
		puts("ERROR cannot initialize the MAX1619 Tsensor on I2C bus");
	}

	if (0 != XL345init()) {
		puts("ERROR: cannot initialize ADXL345 accelerometer on the I2C bus");
	}
												/* ------------------------------------------------ */
	SSInewVar("REMOTE_HOST", HOST_NAME, NULL);	/* Host name variable								*/

	SSInewVar("WebPageRTOS", NULL, &WebPageRTOS);

	SSInewVar("DATE_LOCAL", " ", NULL);

  #ifdef __ABASSI_H__							/* Abassi RTOS type									*/
	SSInewVar("RTOS_ID", "Abassi", NULL);
  #endif
  #ifdef __MABASSI_H__
	SSInewVar("RTOS_ID", "mAbassi", NULL);
  #endif
  #ifdef __UABASSI_H__
	SSInewVar("RTOS_ID", "&micro;Abassi", NULL);
  #endif
												/* ------------------------------------------------ */

	sprintf(&DemoVal[0], "%d", (OS_DEMO<0) ? -OS_DEMO : OS_DEMO);	/* Demo number variable			*/
	SSInewVar("OS_DEMO", &DemoVal[0], NULL);
												/* ------------------------------------------------ */
												/* LED page SET-UP									*/
												/* ------------------------------------------------ */
  #ifdef MY_LED_0
	SSInewVar("SHOW_LED_PAGE", "YES", NULL);
  #else
	SSInewVar("SHOW_LED_PAGE", "NO",  NULL);
  #endif

  #ifdef MY_LED_0
   #if defined(MY_LED_0)
	SSInewVar("LED0_PRESENT", "YES", NULL);
   #else
	SSInewVar("LED0_PRESENT", "NO",  NULL);
   #endif
   #if defined(MY_LED_1)
	SSInewVar("LED1_PRESENT", "YES", NULL);
   #else
	SSInewVar("LED1_PRESENT", "NO",  NULL);
   #endif
   #if defined(MY_LED_2)
	SSInewVar("LED2_PRESENT", "YES", NULL);
   #else
	SSInewVar("LED2_PRESENT", "NO",  NULL);
   #endif
   #if defined(MY_LED_3)
	SSInewVar("LED3_PRESENT", "YES", NULL);
   #else
	SSInewVar("LED3_PRESENT", "NO",  NULL);
   #endif
   #if defined(MY_LED_4)
	SSInewVar("LED4_PRESENT", "YES", NULL);
   #else
	SSInewVar("LED4_PRESENT", "NO",  NULL);
   #endif
   #if defined(MY_LED_5)
	SSInewVar("LED5_PRESENT", "YES", NULL);
   #else
	SSInewVar("LED5_PRESENT", "NO",  NULL);
   #endif
   #if defined(MY_LED_6)
	SSInewVar("LED6_PRESENT", "YES", NULL);
   #else
	SSInewVar("LED6_PRESENT", "NO",  NULL);
   #endif
   #if defined(MY_LED_7)
	SSInewVar("LED7_PRESENT", "YES", NULL);
   #else
	SSInewVar("LED7_PRESENT", "NO",  NULL);
   #endif
   #if defined(MY_LED_8)
	SSInewVar("LED8_PRESENT", "YES", NULL);
   #else
	SSInewVar("LED8_PRESENT", "NO",  NULL);
   #endif
   #if defined(MY_LED_9)
	SSInewVar("LED9_PRESENT", "YES", NULL);
   #else
	SSInewVar("LED9_PRESENT", "NO",  NULL);
   #endif
  #endif
												/* ------------------------------------------------ */
												/* Buttons page set-up								*/
												/* ------------------------------------------------ */
  #ifdef MY_BUTTON_0
	SSInewVar("SHOW_BUT_PAGE", "YES", NULL);
	SSInewUpdate(&ButtonUpdate);				/* Add the button refresh handler to the system		*/
  #else
	SSInewVar("SHOW_BUT_PAGE", "NO",  NULL);
  #endif

  #ifdef MY_BUTTON_0
	SSInewVar("BUT0",  "--", NULL);
	SSInewVar("BUT1",  "--", NULL);
	SSInewVar("BUT2",  "--", NULL);
	SSInewVar("BUT3",  "--", NULL);
	SSInewVar("BUT4",  "--", NULL);
	SSInewVar("BUT5",  "--", NULL);
	SSInewVar("BUT6",  "--", NULL);
	SSInewVar("BUT7",  "--", NULL);
	SSInewVar("BUT8",  "--", NULL);
	SSInewVar("BUT9",  "--", NULL);
  #endif
												/* ------------------------------------------------ */
												/* Switches page set-up								*/
												/* ------------------------------------------------ */
  #ifdef MY_SWITCH_0
	SSInewVar("SHOW_SW_PAGE", "YES", NULL);
	SSInewUpdate(&SwitchUpdate);				/* Add the switch refresh handler to the system		*/
  #else
	SSInewVar("SHOW_SW_PAGE", "NO",  NULL);
  #endif

  #ifdef MY_SWITCH_0
	SSInewVar("SW0",  "--", NULL);
	SSInewVar("SW1",  "--", NULL);
	SSInewVar("SW2",  "--", NULL);
	SSInewVar("SW3",  "--", NULL);
	SSInewVar("SW4",  "--", NULL);
	SSInewVar("SW5",  "--", NULL);
	SSInewVar("SW6",  "--", NULL);
	SSInewVar("SW7",  "--", NULL);
	SSInewVar("SW8",  "--", NULL);
	SSInewVar("SW9",  "--", NULL);
  #endif
												/* ------------------------------------------------ */
												/* LCD page SET-UP									*/
												/* ------------------------------------------------ */

  #if ((MY_LCD_NMB_LINE) <= 0)					/* When LCD present on the board					*/
	SSInewVar("SHOW_LCD_PAGE", "NO",   NULL);	/* No LCD											*/
	SSInewVar("SHOW_LCD",      "NO",   NULL);
  #else											/* There's a LCD									*/
   #if ((I2C_USE_LCD) != 0)						/* If LCD is on I2C, set-up is in I2C control page	*/
	SSInewVar("SHOW_LCD_PAGE", "NO",   NULL);
	SSInewVar("SHOW_LCD",      "YES",  NULL);
   #else										/* If LCD not on I2C, set-up is in LCD control page	*/
	SSInewVar("SHOW_LCD_PAGE", "YES",  NULL);
	SSInewVar("SHOW_LCD",      "NO",   NULL);
   #endif

   #if ((MY_LCD_NMB_LINE) > 0)
	SSInewVar("LCD_HAS_LINE_0", "YES", NULL);
   #else
	SSInewVar("LCD_HAS_LINE_0", "NO",  NULL);
   #endif
   #if ((MY_LCD_NMB_LINE) > 1)
	SSInewVar("LCD_HAS_LINE_1", "YES", NULL);
   #else
	SSInewVar("LCD_HAS_LINE_1", "NO",  NULL);
   #endif
   #if ((MY_LCD_NMB_LINE) > 2)
	SSInewVar("LCD_HAS_LINE_2", "YES", NULL);
   #else
	SSInewVar("LCD_HAS_LINE_2", "NO",  NULL);
   #endif
   #if ((MY_LCD_NMB_LINE) > 3)
	SSInewVar("LCD_HAS_LINE_3", "YES", NULL);
   #else
	SSInewVar("LCD_HAS_LINE_3", "NO",  NULL);
   #endif
   #if ((MY_LCD_NMB_LINE) > 4)
	SSInewVar("LCD_HAS_LINE_4", "YES", NULL);
   #else
	SSInewVar("LCD_HAS_LINE_4", "NO",  NULL);
   #endif
   #if ((MY_LCD_NMB_LINE) > 5)
	SSInewVar("LCD_HAS_LINE_5", "YES", NULL);
   #else
	SSInewVar("LCD_HAS_LINE_5", "NO",  NULL);
   #endif
   #if ((MY_LCD_NMB_LINE) > 6)
	SSInewVar("LCD_HAS_LINE_6", "YES", NULL);
   #else
	SSInewVar("LCD_HAS_LINE_6", "NO",  NULL);
   #endif
   #if ((MY_LCD_NMB_LINE) > 7)
	SSInewVar("LCD_HAS_LINE_7", "YES", NULL);
   #else
	SSInewVar("LCD_HAS_LINE_7", "NO",  NULL);
   #endif
   #if ((MY_LCD_NMB_LINE) > 8)
	SSInewVar("LCD_HAS_LINE_8", "YES", NULL);
   #else
	SSInewVar("LCD_HAS_LINE_8", "NO",  NULL);
   #endif
   #if ((MY_LCD_NMB_LINE) > 9)
	SSInewVar("LCD_HAS_LINE_9", "YES", NULL);
   #else
	SSInewVar("LCD_HAS_LINE_9", "NO",  NULL);
   #endif
  #endif
												/* ------------------------------------------------ */
												/* Temperature page set-up							*/
												/* ------------------------------------------------ */
  #if ((I2C_USE_MAX1619)!= 0)
	SSInewVar("SHOW_TEMP", "YES", NULL);
	SSInewUpdate(&MAX1619update);				/* Add the temperature sensor update handler		*/
  #else
	SSInewVar("SHOW_TEMP", "NO", NULL);
  #endif

  #if ((I2C_USE_MAX1619) != 0)
	SSInewVar("TEMP_0", "0.0", NULL);
	SSInewVar("TEMP_1", "0.0", NULL);

	SSInewVar("SHOW_TEMP_0", "YES", NULL);
	SSInewVar("SHOW_TEMP_1", "YES", NULL);
	SSInewVar("SHOW_TEMP_2", "NO",  NULL);
	SSInewVar("SHOW_TEMP_3", "NO",  NULL);
	SSInewVar("SHOW_TEMP_4", "NO",  NULL);
	SSInewVar("SHOW_TEMP_5", "NO",  NULL);
	SSInewVar("SHOW_TEMP_6", "NO",  NULL);
	SSInewVar("SHOW_TEMP_7", "NO",  NULL);
	SSInewVar("SHOW_TEMP_8", "NO",  NULL);
	SSInewVar("SHOW_TEMP_9", "NO",  NULL);
  #endif
												/* ------------------------------------------------ */
												/* ADC page set-up									*/
												/* ------------------------------------------------ */
  #if ((I2C_USE_ADC) != 0)
	SSInewVar("SHOW_ADC", "YES", NULL);
	SSInewUpdate(&ADCupdate);					/* Add the ADC handler								*/
  #else
	SSInewVar("SHOW_ADC", "NO",  NULL);
  #endif

  #if ((I2C_USE_ADC) != 0)
   #if ((I2C_ADC_TYPE) == 0x2002497)
	SSInewVar("ADC_0",  "---", NULL);
	SSInewVar("ADC_1",  "---", NULL);
	SSInewVar("ADC_2",  "---", NULL);
	SSInewVar("ADC_3",  "---", NULL);
	SSInewVar("ADC_4",  "---", NULL);
	SSInewVar("ADC_5",  "---", NULL);
	SSInewVar("ADC_6",  "---", NULL);
	SSInewVar("ADC_7",  "---", NULL);
	SSInewVar("ADC_8",  "---", NULL);
	SSInewVar("ADC_9",  "---", NULL);
	SSInewVar("ADC_10", "---", NULL);
	SSInewVar("ADC_11", "---", NULL);
	SSInewVar("ADC_12", "---", NULL);

	SSInewVar("SHOW_ADC_0",  "YES",  NULL);
	SSInewVar("SHOW_ADC_1",  "YES", NULL);
	SSInewVar("SHOW_ADC_2",  "YES", NULL);
	SSInewVar("SHOW_ADC_3",  "YES", NULL);
	SSInewVar("SHOW_ADC_4",  "YES", NULL);
	SSInewVar("SHOW_ADC_5",  "YES", NULL);
	SSInewVar("SHOW_ADC_6",  "YES", NULL);
	SSInewVar("SHOW_ADC_7",  "YES", NULL);
	SSInewVar("SHOW_ADC_8",  "YES", NULL);
	SSInewVar("SHOW_ADC_9",  "YES", NULL);
	SSInewVar("SHOW_ADC_10", "YES", NULL);
	SSInewVar("SHOW_ADC_11", "YES", NULL);
	SSInewVar("SHOW_ADC_12", "YES", NULL);
	SSInewVar("SHOW_ADC_13", "NO",  NULL);
	SSInewVar("SHOW_ADC_14", "NO",  NULL);
	SSInewVar("SHOW_ADC_15", "NO",  NULL);
   #endif

   #ifdef ADC_TASK_PRIO
	memset(&ADCdata[0], 0, sizeof(ADCdata));
	#ifdef osCMSIS
	 osThreadCreate(osThread(LwIP_ADC_task), NULL);
	#else
	 TSKcreate("ADC update", ADC_TASK_PRIO, 4096, ADCtask, 1);
    #endif
   #endif
  #endif
												/* ------------------------------------------------ */
												/* Accelerometer (G-sensor) page set-up				*/
												/* ------------------------------------------------ */
  #if ((I2C_USE_XL345) != 0)
	SSInewVar("SHOW_ACC", "YES", NULL);
	SSInewUpdate(&XL345update);					/* Add the ADXL345 Gsensor update handler			*/
  #else
	SSInewVar("SHOW_ACC", "NO",  NULL);
  #endif

  #if ((I2C_USE_XL345) != 0)
	SSInewVar("ACCEL_X", "---   ", NULL);
	SSInewVar("ACCEL_Y", "---   ", NULL);
	SSInewVar("ACCEL_Z", "---   ", NULL);
  #endif
												/* ------------------------------------------------ */
												/* RTC page set-up									*/
												/* ------------------------------------------------ */
  #if ((OS_PLATFORM) == 0x00032F407)			/* 32F407 has an internal RTC						*/
	SSInewVar("SHOW_RTC_PAGE",  "YES", NULL);
	SSInewVar("SHOW_RTC",       "YES", NULL);	/* In I2C control page & Task list, if used			*/
	SSInewUpdate(&RTCupdate);					/* Add the real time clock update handler			*/
	RTCpresent = 1;
  #elif ((I2C_USE_RTC) != 0)
   #if (((OS_PLATFORM) == 0x00007020)																\
    ||  ((OS_PLATFORM) == 0x00007753))			/* NON-standard: RTC on Pmod JA1 (check if there)	*/
	if (0 == i2c_recv(I2C_RTC_DEVICE, I2C_RTC_ADDR, &DemoVal[0], 1)) {	/* Read to see if there		*/
		SSInewVar("SHOW_RTC",       "YES", NULL);
		SSInewUpdate(&RTCupdate);				/* Add the real time clock update handler			*/
		RTCpresent = 1;
	}
	else {
		SSInewVar("SHOW_RTC_PAGE",  "NO",  NULL);
		SSInewVar("SHOW_RTC",       "NO",  NULL);
	}
   #else
  #if ((I2C_USE_RTC) == 0)
	SSInewVar("SHOW_RTC_PAGE",  "YES",  NULL);	/* Set-up in I2C control page						*/
  #else
	SSInewVar("SHOW_RTC",       "YES", NULL);
  #endif
	SSInewUpdate(&RTCupdate);					/* Add the real time clock update handler			*/
   #endif
  #else
	SSInewVar("SHOW_RTC_PAGE",  "NO",  NULL);
	SSInewVar("SHOW_RTC",       "NO",  NULL);
  #endif
												/* ------------------------------------------------ */
												/* I2C control page set-up							*/
  #if ((I2C_IN_USE) != 0)						/* ------------------------------------------------ */
	if ((RTCpresent != 0)						/* RTC useable when on board or has been detected	*/
	||  ((I2C_USE_LCD) != 0)) {
		SSInewVar("SHOW_I2CC_PAGE", "YES", NULL);
	}
	else {
		SSInewVar("SHOW_I2CC_PAGE", "NO",  NULL);
	}
												/* ------------------------------------------------ */
												/* I2C status page set-up							*/
												/* ------------------------------------------------ */
   	if ((RTCpresent != 0)						/* RTC useable when on board or has been detected	*/
	||  ((I2C_USE_MAX1619)!= 0)
	||  ((I2C_USE_XL345) != 0)
	||  ((I2C_USE_ADC) != 0)) {
		SSInewVar("SHOW_I2CS_PAGE", "YES", NULL);
	}
	else {
		SSInewVar("SHOW_I2CS_PAGE", "NO",  NULL);
	}
  #endif  
												/* ------------------------------------------------ */
												/* STM RTC set-up									*/
												/* ------------------------------------------------ */
  #if ((OS_PLATFORM) == 0x032F407)
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	PWR_BackupAccessCmd(ENABLE);
   #if ((STM_RTC_USE_XTAL) == 0)
	RCC_LSICmd(ENABLE);
	while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
	RCC_RTCCLKCmd(ENABLE);
	RTC_WaitForSynchro();
	RTCsetup.RTC_AsynchPrediv = 127;
	RTCsetup.RTC_SynchPrediv  = 249;
   #elif defined (RTC_CLOCK_SOURCE_LSE)
	RCC_LSEConfig(RCC_LSE_ON);
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
	RCC_RTCCLKCmd(ENABLE);
	RTC_WaitForSynchro();
	RTCsetup.RTC_AsynchPrediv = 127;
	RTCsetup.RTC_SynchPrediv  = 255;
   #endif
	RTCsetup.RTC_HourFormat   = RTC_HourFormat_24;
	RTC_Init(&RTCsetup);
	RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
  #endif

	return;
}

/* EOF */
