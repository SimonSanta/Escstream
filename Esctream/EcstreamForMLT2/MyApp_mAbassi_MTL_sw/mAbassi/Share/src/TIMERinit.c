/* ------------------------------------------------------------------------------------------------ */
/* FILE :		TIMERinit.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Initialization of the timer used by the RTOS										*/
/*																									*/
/*																									*/
/* Copyright (c) 2017-2019, Code-Time Technologies Inc. All rights reserved.						*/
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
/*	$Date: 2019/01/10 18:07:16 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#if defined(_STANDALONE_)
  #include "mAbassi.h"
#elif defined(_UABASSI_)							/* It is the same include file in all cases.	*/
  #include "mAbassi.h"								/* There is a substitution during the release 	*/
#elif defined (OS_N_CORE)							/* This file is the same for Abassi, mAbassi,	*/
  #include "mAbassi.h"								/* and uAbassi, so Code Time uses these checks	*/
#else												/* to not have to keep 3 quasi identical		*/
  #include "mAbassi.h"								/* copies of this file							*/
#endif

#include "Platform.h"								/* Everything about the target platform is here	*/
#include "HWinfo.h"									/* Everything about the target board is here	*/

/* ------------------------------------------------------------------------------------------------ */
/* A9 processors use the private timer																*/

#if  ((((OS_PLATFORM) & 0x00FFFFFF) == 0x0000AA10)													\
 ||   (((OS_PLATFORM) & 0x00FFFFFF) == 0x0000AAC5)													\
 ||   (((OS_PLATFORM) & 0x00FFFFFF) == 0x00007020)													\
 ||   (((OS_PLATFORM) & 0x00FFFFFF) == 0x0000FEE6))

void TIMERinit(unsigned int Reload)
{

volatile unsigned int *TimReg;

	TimReg    = COREgetPeriph();					/* Private timer is at base of Peripherals		*/
	TimReg   += 0x600/sizeof(unsigned int);
	TimReg[2] = 0;									/* Disable timer & interrupts					*/
	TimReg[0] = Reload;								/* Private timer load value						*/
	TimReg[1] = Reload;								/* Start the count at reload value				*/
													/* Private timer control register				*/
	TimReg[2] = (0<<8)								/* Prescaler = /1								*/
	          | (1<<2)								/* Interrupt enable								*/
	          | (1<<1)								/* Auto-reload									*/
	          | (1<<0);								/* Timer enable									*/

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* UltraScale+ A53 use TTC #3																		*/

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00007753)	/* Xilinx Ultrascale+ (A53 processors)			*/

static volatile uint32_t TimDum;

void TIMERinit(unsigned int Reload)
{

volatile uint32_t *TimReg;

	TimReg = (volatile uint32_t *)0xFF140000;		/* Using TTC3, timer/clock #0					*/

	TimReg[0x0C/4] = 0x21;							/* Out disable, disable counter					*/
	TimReg[0x00/4] = 0;								/* Reset the clock control						*/
	TimReg[0x60/4] = 0;								/* Reset IER									*/
	TimDum         = TimReg[0x54/4];				/* Reset ISR (is clronrd)						*/
	TimReg[0x0C/4] = 0x10;							/* Reset the counter/timer						*/

	TimReg[0x00/4] = 0x00;							/* Internal clock, positive edge				*/
	TimReg[0x24/4] = Reload;
	TimReg[0x60/4] = 0x01;							/* Interval interrupt enable					*/
	TimReg[0x0C/4] = 0x06;							/* Enable timer									*/

	return;
}

#else
  #error "Unsupported Platform"
#endif

/* EOF */
