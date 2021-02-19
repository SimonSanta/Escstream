/* ------------------------------------------------------------------------------------------------ */
/* FILE :		SysCall_CY5.c																		*/
/*																									*/
/* CONTENTS :																						*/
/*				Hardware dependant Unix system call funtions										*/
/*				This file support Altera's SocFPGA chips											*/
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
/*	$Revision: 1.9 $																				*/
/*	$Date: 2019/01/10 18:07:12 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#if defined(_STANDALONE_)
  #include "mAbassi.h"
#elif defined(_UABASSI_)						 	/* It is the same include file in all cases.	*/
  #include "mAbassi.h"								/* There is a substitution during the release 	*/
#elif defined (OS_N_CORE)							/* This file is the same for Abassi, mAbassi,	*/
  #include "mAbassi.h"								/* and uAbassi, so Code Time uses these checks	*/
#else												/* to not have to keep 3 quasi identical		*/
  #include "mAbassi.h"								/* copies of this file							*/
#endif
#include "SysCall.h"
#include <time.h>

#include "Platform.h"
#include "HWinfo.h"
#include "dw_i2c.h"

#define UNNIBBLE(x)	(((((x)>>4)&0xF)*10) + ((x)&0xF))

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    time																				*/
/*																									*/
/* time - return time of the day using the hardware RTC												*/
/*																									*/
/* SYNOPSIS:																						*/
/*		time_t time(time_t *tptr);																	*/
/*																									*/
/* ARGUMENTS:																						*/
/*																									*/
/* RETURN VALUE:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

time_t time(time_t *tptr)

{
time_t    Ret;										/* Return value									*/
struct tm Time;

#if ((I2C_USE_RTC) != 0)
  char      Buffer[32];								/* Holds the data read from the RTC				*/
  int       ii;										/* General purpose								*/

	Time.tm_sec  = 0;								/* Use 2000.01.01 00:00:00 in case no RTC		*/
	Time.tm_min  = 0;
	Time.tm_hour = 0;
	Time.tm_mday = 1;
	Time.tm_mon  = 0;
	Time.tm_year = 2000-1900;
	Ret = mktime(&Time);
													/* -------------------------------------------- */
  #if ((I2C_RTC_TYPE) == 1339)						/* Maxim DS1339									*/
	Buffer[0] = 0x00;								/* Request to read from register 0x00			*/
	ii = i2c_send_recv(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[0], 1, &Buffer[0], 7);
	if (ii == 0) {
		Time.tm_sec  = (int)UNNIBBLE(Buffer[0]);
		Time.tm_min  = (int)UNNIBBLE(Buffer[1]);
		Time.tm_hour = (int)UNNIBBLE(Buffer[2]);
		Time.tm_mday = (int)UNNIBBLE(Buffer[4]);
		Time.tm_mon  = (int)UNNIBBLE(Buffer[5])-1;
		Time.tm_year = (int)UNNIBBLE(Buffer[6])
		             + (2000-1900);
	}
	else {
		errno = 111;
	}
													/* -------------------------------------------- */
  #elif ((I2C_RTC_TYPE) == 4182)					/* ST M41T82									*/
	Buffer[0]  = 0x0C;								/* Clear HT bit: is set when back from Battery	*/
	ii         = i2c_send(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[0], 1);
	ii        |= i2c_recv(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[1], 1);
	Buffer[1] &= 0xBF;
	ii        |= i2c_send(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[0], 2);

	Buffer[0] = 0x01;								/* Request to start reading from second reg	*/
	ii       |= i2c_send(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[0], 1);
	ii       |= i2c_recv(I2C_RTC_DEVICE, I2C_RTC_ADDR, &Buffer[0], 7);

	if (ii  == 0) {
		Buffer[0] &= 0x7F;							/* Remove the controls bits						*/
		Buffer[2] &= 0x3F;							/* Remove the Centuty bits						*/

		Time.tm_sec  = (int)UNNIBBLE(Buffer[0]);
		Time.tm_min  = (int)UNNIBBLE(Buffer[1]);
		Time.tm_hour = (int)UNNIBBLE(Buffer[2]);
		Time.tm_mday = (int)UNNIBBLE(Buffer[4]);
		Time.tm_mon  = (int)UNNIBBLE(Buffer[5])-1;
		Time.tm_year = (int)UNNIBBLE(Buffer[6])
		             + (2000-1900);
	}
	else {
		errno = 111;
	}
  #endif

#else												/* -------------------------------------------- */
	errno = 1111;									/* Unknown hardware								*/
#endif

	Ret = mktime(&Time);
	if (tptr != NULL) {
		*tptr  = Ret;
	}

	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    _gettimeofday																		*/
/*																									*/
/* *** DUMMY ***																					*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int _gettimeofday(struct timeval *tv, struct timezone *tz);									*/
/*																									*/
/* ARGUMENTS:																						*/
/*																									*/
/* RETURN VALUE:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
#if defined(__CC_ARM)
 int _gettimeofday(void *tv, void *tz)
#else
 int _gettimeofday(struct timeval *tv, struct timezone *tz)
#endif
{
	return(0);
}

/* EOF */
