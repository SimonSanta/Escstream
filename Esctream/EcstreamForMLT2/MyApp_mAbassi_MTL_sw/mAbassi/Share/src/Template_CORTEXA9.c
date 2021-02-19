/* ------------------------------------------------------------------------------------------------ */
/* FILE :		Template_CORTEXA9.c																	*/
/*																									*/
/* CONTENTS :																						*/
/*				Template to ease creating a new application											*/
/*				Serial port settings:																*/
/*							Baud Rate: 115200														*/
/*							Data bits:      8														*/
/*							Stop bits:      1														*/
/*							Parity:      none														*/
/*							Flow Ctrl:   none														*/
/*							Emulation: VT-100														*/
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
/*	$Revision: 1.8 $																				*/
/*	$Date: 2019/01/10 18:07:16 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "mAbassi.h"
#include "SysCall.h"
#include "Platform.h"								/* Everything about the target platform is here	*/
#include "HWinfo.h"

#if  ((((OS_PLATFORM) & 0x00FFFFFF) == 0x0000AA10)													\
 ||   (((OS_PLATFORM) & 0x00FFFFFF) == 0x0000AAC5))
  #include "dw_uart.h"
#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00007020)
  #include "cd_uart.h"
#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x0000FEE6)
  #include "imx_uart.h"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Defintions																						*/

#if ((OX_LIB_REENT_PROTECT) != 0)					/* If the library is protected against reentr	*/
  #define STDOUT_MTX_LOCK(x)	OX_DO_NOTHING()		/* then no need for a exclusive access mutex	*/
  #define STDOUT_MTX_UNLOCK(x)	OX_DO_NOTHING()		/* to protect printf & stdout.					*/
#else
  #define STDOUT_MTX_LOCK(x)	do { (x) = MTXopen("Disp");MTXlock(x,-1); } while(0)
  #define STDOUT_MTX_UNLOCK(x)	do { MTXunlock(x);                        } while(0)
#endif

/* ------------------------------------------------------------------------------------------------ */
/* App variables																					*/

int G_UartDevIn  = UART_DEV;						/* Needed by the system call layer				*/
int G_UartDevOut = UART_DEV;						/* Needed by the system call layer				*/
int G_UartDevErr = UART_DEV;						/* Needed by the system call layer				*/

void Fct1(void);
void Fct2(void);
void Fct3(void);
void Fct4(void);

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */

int main(void)
{
#if ((OX_LIB_REENT_PROTECT) == 0)
  MTX_t *Mutex;
#endif

/* ------------------------------------------------ */
/* Set buffering here: tasks inherit main's stdios	*/
/* when Newlib reent is used & stdio are shared		*/ 

	setvbuf(stdin,  NULL, _IONBF, 0);				/* By default, Newlib library flush the I/O		*/
	setvbuf(stdout, NULL, _IONBF, 0);				/* buffer when full or when new-line			*/
	setvbuf(stderr, NULL, _IONBF, 0);				/* Done before OSstart() to have all tasks with	*/
													/* the same stdio set-up						*/
/* ------------------------------------------------ */
/* Start mAbassi									*/

	OSstart();										/* Start mAbassi								*/

	SysCallInit();									/* Initialize the System Call layer				*/

	OSintOn();										/* Enable the interrupts						*/

/* ------------------------------------------------ */
/* SysTick Set-up									*/

	OSisrInstall(OS_TICK_INT, &TIMtick);			/* Install the RTOS timer tick function			*/
	TIMERinit(((((OS_TICK_SRC_FREQ/OS_TICK_PRESCL)/100000)*(OX_TIMER_US))/(10)));
	GICenable(OS_TICK_INT, 128, 1);					/* Priority == mid / Edge trigger				*/

/* ------------------------------------------------ */
/* UART set-up										*/
/* 8 bits, No Parity, 1 Stop						*/

   #if (0U == ((UART_LIST_DEVICE) & (1U << (UART_DEV))))
	#error "Selected UART device is not in UART_LIST_DEVICE"
   #endif
													/* UART_Q_SIZE are defined in Platform.h		*/
													/* See ??_uart.h for the UART_FILT etc defs		*/
	uart_init(UART_DEV, BAUDRATE, 8, 0, 10, UART_Q_SIZE_RX, UART_Q_SIZE_TX, UART_FILT_OUT_LF_CRLF);

   #if (((UART_LIST_DEVICE) & 0x01U) != 0U)			/* Install the UART interrupt handler			*/
	OSisrInstall(UART0_INT, &UARTintHndl_0);
	GICenable(-UART0_INT, 128, 0);					/* UART Must use level and not edge trigger		*/
   #endif											/* Int # -ve: the interrupt targets all cores	*/

   #if (((UART_LIST_DEVICE) & 0x02U) != 0U)			/* Install the UART interrupt handler			*/
	OSisrInstall(UART1_INT, &UARTintHndl_1);
	GICenable(-UART1_INT, 128, 0);					/* UART Must use level and not edge trigger		*/
   #endif											/* Int # -ve: the interrupt targets all cores	*/

   #if (((UART_LIST_DEVICE) & 0x04U) != 0U)			/* Install the UART interrupt handler			*/
	OSisrInstall(UART2_INT, &UARTintHndl_2);
	GICenable(-UART2_INT, 128, 0);					/* UART Must use level and not edge trigger		*/
   #endif											/* Int # -ve: the interrupt targets all cores	*/

   #if (((UART_LIST_DEVICE) & 0x08U) != 0U)			/* Install the UART interrupt handler			*/
	OSisrInstall(UART3_INT, &UARTintHndl_3);
	GICenable(-UART3_INT, 128, 0);					/* UART Must use level and not edge trigger		*/
   #endif											/* Int # -ve: the interrupt targets all cores	*/

/* ------------------------------------------------ */
/* Create a few tasks								*/

	printf("\n\n4 tasks are created and, including A&E, they all print Hello World\n");
	printf("The demo stops after that\n\n");

	TSKcreate("TSK 1", 1, 4096, &Fct1, 1);
	TSKcreate("TSK 2", 2, 4096, &Fct2, 1);
	TSKcreate("TSK 3", 3, 4096, &Fct3, 1);
	TSKcreate("TSK 4", 4, 4096, &Fct4, 1);

	STDOUT_MTX_LOCK(Mutex);							/* If needed, get exclusive access to stdout	*/

	printf("A&E  : Hello World\n");					/* Report we are alive and running				*/

	STDOUT_MTX_UNLOCK(Mutex);						/* Release the exclusive access					*/

	TSKselfSusp();
}

/* ------------------------------------------------------------------------------------------------ */
/* A few tasks that will also print "Hello World"													*/
/* ------------------------------------------------------------------------------------------------ */

void Fct1(void)
{
#if ((OX_LIB_REENT_PROTECT) == 0)
  MTX_t *Mutex;
#endif

	STDOUT_MTX_LOCK(Mutex);							/* If needed, get exclusive access to stdout	*/

	printf("Tsk1 : Hello World\n");					/* Report we are alive and running				*/

	STDOUT_MTX_UNLOCK(Mutex);						/* Release the exclusive access					*/

	TSKselfSusp();									/* And we can now disappear						*/
}

/* ------------------------------------------------------------------------------------------------ */

void Fct2(void)
{
#if ((OX_LIB_REENT_PROTECT) == 0)
  MTX_t *Mutex;
#endif

	STDOUT_MTX_LOCK(Mutex);							/* If needed, get exclusive access to stdout	*/

	printf("Tsk2 : Hello World\n");					/* Report we are alive and running				*/

	STDOUT_MTX_UNLOCK(Mutex);						/* Release the exclusive access					*/

	TSKselfSusp();									/* And we can now disappear						*/
}

/* ------------------------------------------------------------------------------------------------ */

void Fct3(void)
{
#if ((OX_LIB_REENT_PROTECT) == 0)
  MTX_t *Mutex;
#endif

	STDOUT_MTX_LOCK(Mutex);							/* If needed, get exclusive access to stdout	*/

	printf("Tsk3 : Hello World\n");					/* Report we are alive and running				*/

	STDOUT_MTX_UNLOCK(Mutex);						/* Release the exclusive access					*/

	TSKselfSusp();									/* And we can now disappear						*/
}

/* ------------------------------------------------------------------------------------------------ */

void Fct4(void)
{
#if ((OX_LIB_REENT_PROTECT) == 0)
  MTX_t *Mutex;
#endif

	STDOUT_MTX_LOCK(Mutex);							/* If needed, get exclusive access to stdout	*/

	printf("Tsk4 : Hello World\n");					/* Report we are alive and running				*/

	STDOUT_MTX_UNLOCK(Mutex);						/* Release the exclusive access					*/

	TSKselfSusp();									/* And we can now disappear						*/
}

/* ------------------------------------------------------------------------------------------------ */
/* RTOS timer callback function																		*/
/* ------------------------------------------------------------------------------------------------ */

void TIMcallBack(void)
{
	return;
}

/* EOF */
