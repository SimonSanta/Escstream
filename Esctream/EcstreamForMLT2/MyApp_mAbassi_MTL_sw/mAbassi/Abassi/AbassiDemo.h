/* ------------------------------------------------------------------------------------------------ */
/* FILE :		AbassiDemo.h																		*/
/*																									*/
/* CONTENS :																						*/
/*				Contains the build options for the different example code							*/
/*																									*/
/*																									*/
/* Copyright (c) 2011-2019, Code-Time Technologies Inc. All rights reserved.						*/
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
/*  $Revision: 1.77 $																				*/
/*	$Date: 2019/01/10 18:06:18 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __ABASSIDEMO_H__
#define __ABASSIDEMO_H__ 1

/* ------------------------------------------------------------------------------------------------ */
/* Override the definitions in Abassi.h																*/

#define OS_DEF_IN_MAKE 1

/* ------------------------------------------------------------------------------------------------ */
/* Select the task's stack size according to the processor & sometimes tool chain					*/

#ifndef TC_TSK_STACK
  #if defined (__PIC32MX__)
	#define TC_TSK_STACK	2048


  #elif defined(__KEIL__) && defined(__C51__)
	#define TC_TSK_STACK	1024


  #elif defined(SDCC_z80)
	#define TC_TSK_STACK	256


  #elif  defined(__ICC430__)  									\
   || (defined(__GNUC__) && defined(__MSP430__))				\
   || (defined(__TI_COMPILER_VERSION__) && defined(__MSP430__))
  #include "msp430.h"							/* Need to know the device type						*/
  #if defined( __msp430x54x) || defined(__msp430x16x)
	#define TC_TSK_STACK	784//512
  #else
	#define TC_TSK_STACK	80
  #endif


  #elif defined(__TI_TMS470_V7M3__) && defined(omap4460)
	#define TC_TSK_STACK	2048


  #elif (defined(__TI_COMPILER_VERSION__) &&defined(__TMS320C28XX__))
	#define TC_TSK_STACK	200


  #elif defined(__ICCAVR__)										\
   || (defined(__GNUC__) && defined(__AVR__))
	#define TC_TSK_STACK	160


  #elif defined(LPC_P1227) || defined(LPC11U24)
	#define TC_TSK_STACK	384


  #elif defined(LPC1343)
	#define TC_TSK_STACK	376


  #elif (defined(__CC_ARM) && defined(__TARGET_ARCH_7_A))
	#define TC_TSK_STACK	2048


  #elif (defined(__GNUC__) && defined(__ARM_ARCH_7A__))
	#define TC_TSK_STACK	2048

  #elif defined(__TI_TMS470_V7A8__)
	#define TC_TSK_STACK	2048


  #else
	#define TC_TSK_STACK	1024

  #endif
#endif

#if ((defined(__CC_ARM) && defined(__TARGET_ARCH_7_A))												\
 ||  (defined(__GNUC__) && defined(__ARM_ARCH_7A__)))
  #define LARGE_MEM		1
#else
  #define LARGE_MEM		0
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Almost all demos use the same set-up																*/
/* ------------------------------------------------------------------------------------------------ */

#if (((OS_DEMO >= 0) && (OS_DEMO <= 8) && ((LARGE_MEM) == 0)))
	#define OS_ALLOC_SIZE		0				/* When !=0, RTOS supplied OSalloc					*/
	#define OS_COOPERATIVE		0				/* When !=0, the  kernel is in cooperative mode		*/
	#define OS_EVENTS			1				/* If event flags are supported						*/
	#define OS_FCFS				0				/* Allow the use of 1st come 1st serve semaphore	*/
  #ifdef __ABASSI_H__							/* mAbassi & uAbassi don't need Idle anymore		*/
	#define OS_IDLE_STACK		TC_TSK_STACK	/* If IdleTask supplied & if so, stack size			*/
  #else
	#define OS_IDLE_STACK		0
  #endif
	#define OS_LOGGING_TYPE		0				/* Type of logging to use							*/
	#define OS_MAILBOX			1				/* If mailboxes are used							*/
  #ifndef OS_MAX_PEND_RQST
	#define OS_MAX_PEND_RQST	32U				/* Maximum number of sempahores posted in ISRs		*/
  #endif
  #if defined(__KEIL__) && defined(__C51__)
	#define OS_MIN_STACK_USE	1				/* If the kernel minimizes its stack usage			*/
  #else
	#define OS_MIN_STACK_USE	0				/* If the kernel minimizes its stack usage			*/
  #endif
	#define OS_MTX_DEADLOCK		0				/* This test validates this							*/
	#define OS_MTX_INVERSION	1				/* To enable protection against priority inversion	*/
	#define OS_NAMES			-1				/* != 0 when named Tasks / Semaphores / Mailboxes	*/
  #ifndef OS_NESTED_INTS
	#define OS_NESTED_INTS		1				/* If operating with nested interrupts				*/
  #endif
	#define OS_PRIO_CHANGE		1				/* If a task priority can be changed at run time	*/
	#define OS_PRIO_MIN			10				/* Max priority, Idle = OS_PRIO_MIN, AdameEve = 0	*/
	#define OS_PRIO_SAME		1				/* Support multiple tasks with the same priority	*/
	#define OS_ROUND_ROBIN		-1000000		/* Use round-robin, value specifies period in uS	*/
	#define OS_RUNTIME			1				/* If create Task / Semaphore / Mailbox at run time	*/
	#define OS_SEARCH_ALGO		0				/* If using a fast search							*/
	#define OS_STARVE_PRIO		-3				/* Priority threshold for starving protection		*/
  #if defined(__KEIL__) && defined(__C51__)
	#define OS_STARVE_RUN_MAX	-40				/* Maximum Timer Tick for starving protection		*/
	#define OS_STARVE_WAIT_MAX	-100			/* Maximum time on hold for starving protection		*/
  #else
	#define OS_STARVE_RUN_MAX	-200			/* Maximum Timer Tick for starving protection		*/
	#define OS_STARVE_WAIT_MAX	-500			/* Maximum time on hold for starving protection		*/
  #endif
  #if (defined(__TI_COMPILER_VERSION__) &&defined(__TMS320C28XX__))
	#define OS_STATIC_BUF_MBX	0				/* when OS_STATIC_MBOX != 0, # of buffer element	*/
	#define OS_STATIC_MBX		7				/* If !=0 how many mailboxes						*/
  #else											/* For the C28X we'll use malloc()					*/
	#define OS_STATIC_BUF_MBX	320				/* when OS_STATIC_MBOX != 0, # of buffer element	*/
	#define OS_STATIC_MBX		7				/* If !=0 how many mailboxes						*/
  #endif
  #ifdef OS_UART_ISR
   #if ((OS_UART_ISR) != 0)
	#undef  OS_STATIC_BUF_MBX
	#define OS_STATIC_BUF_MBX	(320+514)		/* when OS_STATIC_MBOX != 0, # of buffer element	*/
	#undef  OS_STATIC_MBX
	#define OS_STATIC_MBX		(7+2)			/* If !=0 how many mailboxes						*/
   #endif
  #endif
	#define OS_STATIC_NAME		0				/* If named mailboxes/semaphore/task, size in char	*/
  #ifdef UART_ISR
	#define OS_STATIC_SEM		6				/* If !=0 how many semaphores and mutexes			*/
  #else
	#define OS_STATIC_SEM		4				/* If !=0 how many semaphores and mutexes			*/
  #endif
	#define OS_STATIC_STACK		((TC_TSK_STACK)*(OS_STATIC_TASK))
	#define OS_STATIC_TASK		6				/* If !=0 how many tasks (excluding A&E and Idle)	*/
	#define OS_STATIC_TIM_SRV	0				/* If !=0 how many timer services					*/
	#define OS_TASK_SUSPEND		1				/* If a task can suspend another one				*/
	#define OS_TIMEOUT			1				/* !=0 enables blocking timeout						*/
	#define OS_TIMER_CB			1				/* !=0 gives the timer callback period				*/
	#define OS_TIMER_SRV		0				/* !=0 includes the timer services					*/
  #if defined(__KEIL__) && defined(__C51__)
	#define OS_TIMER_US			50000			/* !=0 enables timer & specifies the period in uS	*/
  #else
	#define OS_TIMER_US			10000			/* !=0 enables timer & specifies the period in uS	*/
  #endif
	#define OS_USE_TASK_ARG		1				/* If tasks have arguments							*/

  #if defined(LPC_P1227) || defined(LPC11U24)	/* We only have 6 K / 8K, so cut some stuff			*/
	#undef  OS_STATIC_BUF_MBX
	#define OS_STATIC_BUF_MBX 	10
	#undef  OS_STATIC_MBX
	#define	OS_STATIC_MBX		2
  #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* FatFS shell Demo settings																		*/
/* lwIP Demo settings																				*/
/* ------------------------------------------------------------------------------------------------ */

#if ((((OS_DEMO) >= 9) && ((OS_DEMO) <= 99)) || ((OS_DEMO) >= 9900)									\
 || ((OS_DEMO >= 0) && ((LARGE_MEM) != 0)))

	#define OS_ALLOC_SIZE		0				/* When !=0, RTOS supplied OSalloc					*/
	#define OS_COOPERATIVE		0				/* When !=0, the  kernel is in cooperative mode		*/
	#define OS_EVENTS			1				/* If event flags are supported						*/
	#define OS_FCFS				0				/* Allow the use of 1st come 1st serve semaphore	*/
  #ifdef __ABASSI_H__							/* mAbassi & uAbassi don't need Idle anymore		*/
	#define OS_IDLE_STACK		0//TC_TSK_STACK	/* If IdleTask supplied & if so, stack size			*/
  #else
	#define OS_IDLE_STACK		0
  #endif
	#define OS_LOGGING_TYPE		0				/* Type of logging to use							*/
	#define OS_MAILBOX			1				/* If mailboxes are used							*/
  #ifndef OS_MAX_PEND_RQST
	#define OS_MAX_PEND_RQST	32U				/* Maximum number of sempahores posted in ISRs		*/
  #endif
	#define OS_MIN_STACK_USE	0				/* If the kernel minimizes its stack usage			*/
	#define OS_MTX_DEADLOCK		0				/* This test validates this							*/
	#define OS_MTX_INVERSION	1				/* To enable protection against priority inversion	*/
	#define OS_NAMES			-1				/* != 0 when named Tasks / Semaphores / Mailboxes	*/
  #ifndef OS_NESTED_INTS
	#define OS_NESTED_INTS		1				/* If operating with nested interrupts				*/
  #endif
	#define OS_PRIO_CHANGE		1				/* If a task priority can be changed at run time	*/
	#define OS_PRIO_MIN			10				/* Max priority, Idle = OS_PRIO_MIN, AdameEve = 0	*/
	#define OS_PRIO_SAME		1				/* Support multiple tasks with the same priority	*/
	#define OS_ROUND_ROBIN		-1000000		/* Use round-robin, value specifies period in uS	*/
	#define OS_RUNTIME			1				/* If create Task / Semaphore / Mailbox at run time	*/
	#define OS_SEARCH_ALGO		0				/* If using a fast search							*/
  #if ((OS_DEMO) == 9)
	#define OS_STARVE_PRIO		0				/* Priority threshold for starving protection		*/
	#define OS_STARVE_RUN_MAX	0				/* Maximum Timer Tick for starving protection		*/
	#define OS_STARVE_WAIT_MAX	0				/* Maximum time on hold for starving protection		*/
  #else
	#define OS_STARVE_PRIO		-3				/* Priority threshold for starving protection		*/
	#define OS_STARVE_RUN_MAX	-200			/* Maximum Timer Tick for starving protection		*/
	#define OS_STARVE_WAIT_MAX	-500			/* Maximum time on hold for starving protection		*/
  #endif
	#define OS_STATIC_BUF_MBX	0				/* when OS_STATIC_MBOX != 0, # of buffer element	*/
	#define OS_STATIC_MBX		0				/* If !=0 how many mailboxes						*/
	#define OS_STATIC_NAME		0				/* If named mailboxes/semaphore/task, size in char	*/
	#define OS_STATIC_SEM		0				/* If !=0 how many semaphores and mutexes			*/
	#define OS_STATIC_STACK		0
	#define OS_STATIC_TASK		0				/* If !=0 how many tasks (excluding A&E and Idle)	*/
	#define OS_STATIC_TIM_SRV	0				/* If !=0 how many timer services					*/
	#define OS_TASK_SUSPEND		1				/* If a task can suspend another one				*/
	#define OS_TIMEOUT			1				/* !=0 enables blocking timeout						*/
	#define OS_TIMER_CB			1				/* !=0 gives the timer callback period				*/
	#define OS_TIMER_SRV		0				/* !=0 includes the timer services					*/
  #if (((OS_DEMO >= 0) && (OS_DEMO <= 8) && ((LARGE_MEM) != 0)))	/* Keep same timer us 			*/
	#define OS_TIMER_US			10000			/* !=0 enables timer & specifies the period in uS	*/
  #elif (((OS_DEMO) == 50) || ((OS_DEMO) == -50))
	#define OS_TIMER_US			1000
  #else
	#define OS_TIMER_US			5000			/* !=0 enables timer & specifies the period in uS	*/
  #endif
	#define OS_USE_TASK_ARG		1				/* If tasks have arguments							*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* FatFS shell Demo settings with CMSIS API															*/
/* lwIP Demo settings																				*/
/* ------------------------------------------------------------------------------------------------ */

#if (((OS_DEMO) >= 110) && ((OS_DEMO) <= 129))
	#define OS_CMSIS_RTOS		1
	#define OS_ALLOC_SIZE		0				/* When !=0, RTOS supplied OSalloc					*/
	#define OS_COOPERATIVE		0				/* When !=0, the  kernel is in cooperative mode		*/
	#define OS_EVENTS			1				/* If event flags are supported						*/
	#define OS_FCFS				0				/* Allow the use of 1st come 1st serve semaphore	*/
  #ifdef __ABASSI_H__							/* mAbassi & uAbassi don't need Idle anymore		*/
	#define OS_IDLE_STACK		TC_TSK_STACK	/* If IdleTask supplied & if so, stack size			*/
  #else
	#define OS_IDLE_STACK		0
  #endif
	#define OS_LOGGING_TYPE		0				/* Type of logging to use							*/
	#define OS_MAILBOX			1				/* If mailboxes are used							*/
  #ifndef OS_MAX_PEND_RQST
	#define OS_MAX_PEND_RQST	32U				/* Maximum number of sempahores posted in ISRs		*/
  #endif
	#define OS_MIN_STACK_USE	0				/* If the kernel minimizes its stack usage			*/
	#define OS_MTX_DEADLOCK		0				/* This test validates this							*/
	#define OS_MTX_INVERSION	0				/* To enable protection against priority inversion	*/
	#define OS_NAMES			1				/* != 0 when named Tasks / Semaphores / Mailboxes	*/
  #ifndef OS_NESTED_INTS
	#define OS_NESTED_INTS		1				/* If operating with nested interrupts				*/
  #endif
	#define OS_PRIO_CHANGE		1				/* If a task priority can be changed at run time	*/
	#define OS_PRIO_MIN			6				/* Max priority, Idle = OS_PRIO_MIN, AdameEve = 0	*/
	#define OS_PRIO_SAME		1				/* Support multiple tasks with the same priority	*/
	#define OS_ROUND_ROBIN		0				/* Use round-robin, value specifies period in uS	*/
	#define OS_RUNTIME			0				/* If create Task / Semaphore / Mailbox at run time	*/
	#define OS_SEARCH_ALGO		0				/* If using a fast search							*/
	#define OS_STARVE_PRIO		-3				/* Priority threshold for starving protection		*/
	#define OS_STARVE_RUN_MAX	-200			/* Maximum Timer Tick for starving protection		*/
	#define OS_STARVE_WAIT_MAX	-500			/* Maximum time on hold for starving protection		*/
	#define OS_STATIC_BUF_MBX	0				/* when OS_STATIC_MBOX != 0, # of buffer element	*/
	#define OS_STATIC_MBX		0				/* If !=0 how many mailboxes						*/
	#define OS_STATIC_NAME		0				/* If named mailboxes/semaphore/task, size in char	*/
	#define OS_STATIC_SEM		0				/* If !=0 how many semaphores and mutexes			*/
	#define OS_STATIC_STACK		0
	#define OS_STATIC_TASK		0				/* If !=0 how many tasks (excluding A&E and Idle)	*/
	#define OS_STATIC_TIM_SRV	0				/* If !=0 how many timer services					*/
	#define OS_TASK_SUSPEND		1				/* If a task can suspend another one				*/
	#define OS_TIMEOUT			1				/* !=0 enables blocking timeout						*/
	#define OS_TIMER_CB			1				/* !=0 gives the timer callback period				*/
	#define OS_TIMER_SRV		1				/* !=0 includes the timer services					*/
	#define OS_TIMER_US			5000			/* !=0 enables timer & specifies the period in uS	*/
	#define OS_USE_TASK_ARG		1				/* If tasks have arguments							*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* IP-USB-FAT Demo settings																			*/
/* ------------------------------------------------------------------------------------------------ */

#if (OS_DEMO == 1000)
	#undef  TC_TSK_STACK
	#define TC_TSK_STACK 		2048			/* Dedicated platform demo							*/

	#define OS_ALLOC_SIZE		0				/* When !=0, RTOS supplied OSalloc					*/
	#define OS_COOPERATIVE		0				/* When !=0, the  kernel is in cooperative mode		*/
	#define OS_EVENTS			1				/* If event flags are supported						*/
	#define OS_FCFS				0				/* Allow the use of 1st come 1st serve semaphore	*/
	#define OS_IDLE_STACK		TC_TSK_STACK	/* If IdleTask supplied & if so, stack size			*/
	#define OS_LOGGING_TYPE		0				/* Type of logging to use							*/
	#define OS_MAILBOX			1				/* If mailboxes are used							*/
  #ifndef OS_MAX_PEND_RQST
	#define OS_MAX_PEND_RQST	32U				/* Maximum number of sempahores posted in ISRs		*/
  #endif
	#define OS_MIN_STACK_USE	0				/* If the kernel minimizes its stack usage			*/
	#define OS_MTX_DEADLOCK		0				/* This test validates this							*/
	#define OS_MTX_INVERSION	0				/* To enable protection against priority inversion	*/
	#define OS_NAMES			1				/* != 0 when named Tasks / Semaphores / Mailboxes	*/
  #ifndef OS_NESTED_INTS
	#define OS_NESTED_INTS		1				/* If operating with nested interrupts				*/
  #endif
	#define OS_PRIO_CHANGE		0				/* If a task priority can be changed at run time	*/
	#define OS_PRIO_MIN			32				/* Max priority, Idle = OS_PRIO_MIN, AdameEve = 0	*/
	#define OS_PRIO_SAME		1				/* Support multiple tasks with the same priority	*/
	#define OS_ROUND_ROBIN		-1000000		/* Use round-robin, value specifies period in uS	*/
	#define OS_RUNTIME			1				/* If create Task / Semaphore / Mailbox at run time	*/
	#define OS_SEARCH_ALGO		0				/* If using a fast search							*/
	#define OS_STARVE_PRIO		0				/* Priority threshold for starving protection		*/
	#define OS_STARVE_RUN_MAX	0				/* Maximum Timer Tick for starving protection		*/
	#define OS_STARVE_WAIT_MAX	0				/* Maximum time on hold for starving protection		*/
	#define OS_STATIC_BUF_MBX	16				/* when OS_STATIC_MBOX != 0, # of buffer element	*/
	#define OS_STATIC_MBX		10				/* If !=0 how many mailboxes						*/
	#define OS_STATIC_NAME		2048			/* If named mailboxes/semaphore/task, size in char	*/
	#define OS_STATIC_SEM		32				/* If !=0 how many semaphores and mutexes			*/
	#define OS_STATIC_STACK		((TC_TSK_STACK)*(OS_STATIC_TASK))
	#define OS_STATIC_TASK		18				/* If !=0 how many tasks (excluding A&E and Idle)	*/
	#define OS_STATIC_TIM_SRV	0				/* If !=0 how many timer services					*/
	#define OS_TASK_SUSPEND		0				/* If a task can suspend another one				*/
	#define OS_TIMEOUT			1				/* !=0 enables blocking timeout						*/
	#define OS_TIMER_CB			0				/* !=0 gives the timer callback period				*/
	#define OS_TIMER_SRV		0				/* !=0 includes the timer services					*/
	#define OS_TIMER_US			10000			/* !=0 enables timer & specifies the period in uS	*/
	#define OS_USE_TASK_ARG		0				/* If tasks have arguments							*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Almost all demos use the same set-up																*/
/* ------------------------------------------------------------------------------------------------ */

#if ((OS_DEMO) == 333)
  #undef  TC_TSK_STACK
  #define TC_TSK_STACK 176
	#define OS_ALLOC_SIZE		0				/* When !=0, RTOS supplied OSalloc					*/
	#define OS_COOPERATIVE		0				/* When !=0, the  kernel is in cooperative mode		*/
	#define OS_EVENTS			1				/* If event flags are supported						*/
	#define OS_FCFS				0				/* Allow the use of 1st come 1st serve semaphore	*/
	#define OS_IDLE_STACK		0				/* If IdleTask supplied & if so, stack size			*/
	#define OS_LOGGING_TYPE		0				/* Type of logging to use							*/
	#define OS_MAILBOX			1				/* If mailboxes are used							*/
  #ifndef OS_MAX_PEND_RQST
	#define OS_MAX_PEND_RQST	8U				/* Maximum number of sempahores posted in ISRs		*/
  #endif
	#define OS_MIN_STACK_USE	1				/* If the kernel minimizes its stack usage			*/
	#define OS_MTX_DEADLOCK		0				/* This test validates this							*/
	#define OS_MTX_INVERSION	1				/* To enable protection against priority inversion	*/
	#define OS_NAMES			-1				/* != 0 when named Tasks / Semaphores / Mailboxes	*/
  #ifndef OS_NESTED_INTS
	#define OS_NESTED_INTS		1				/* If operating with nested interrupts				*/
  #endif
	#define OS_PRIO_CHANGE		1				/* If a task priority can be changed at run time	*/
	#define OS_PRIO_MIN			10				/* Max priority, Idle = OS_PRIO_MIN, AdameEve = 0	*/
	#define OS_PRIO_SAME		1				/* Support multiple tasks with the same priority	*/
	#define OS_ROUND_ROBIN		-1000000		/* Use round-robin, value specifies period in uS	*/
	#define OS_RUNTIME			1				/* If create Task / Semaphore / Mailbox at run time	*/
	#define OS_SEARCH_ALGO		0				/* If using a fast search							*/
	#define OS_STARVE_PRIO		-3				/* Priority threshold for starving protection		*/
	#define OS_STARVE_RUN_MAX	-200			/* Maximum Timer Tick for starving protection		*/
	#define OS_STARVE_WAIT_MAX	-500			/* Maximum time on hold for starving protection		*/
	#define OS_STATIC_BUF_MBX	136				/* when OS_STATIC_MBOX != 0, # of buffer element	*/
	#define OS_STATIC_MBX		1				/* If !=0 how many mailboxes						*/
	#define OS_STATIC_NAME		0				/* If named mailboxes/semaphore/task, size in char	*/
	#define OS_STATIC_SEM		3				/* If !=0 how many semaphores and mutexes			*/
	#define OS_STATIC_STACK		((TC_TSK_STACK)*(OS_STATIC_TASK))
	#define OS_STATIC_TASK		6				/* If !=0 how many tasks (excluding A&E and Idle)	*/
	#define OS_STATIC_TIM_SRV	0				/* If !=0 how many timer services					*/
	#define OS_TASK_SUSPEND		1				/* If a task can suspend another one				*/
	#define OS_TIMEOUT			1				/* !=0 enables blocking timeout						*/
	#define OS_TIMER_CB			1				/* !=0 gives the timer callback period				*/
	#define OS_TIMER_SRV		0				/* !=0 includes the timer services					*/
	#define OS_TIMER_US			10000			/* !=0 enables timer & specifies the period in uS	*/
	#define OS_USE_TASK_ARG		1				/* If tasks have arguments							*/

  #if defined(LPC_P1227) || defined(LPC11U24)	/* We only have 6 K / 8K, so cut some stuff			*/
	#undef  OS_STATIC_BUF_MBX
	#define OS_STATIC_BUF_MBX 	10
	#undef  OS_STATIC_MBX
	#define	OS_STATIC_MBX		2
  #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* To create evaluation objects																		*/
/* ------------------------------------------------------------------------------------------------ */

#if ((OS_DEMO) < 0)
	#define OS_ALLOC_SIZE		0				/* When !=0, RTOS supplied OSalloc					*/
	#define OS_COOPERATIVE		0				/* When !=0, the  kernel is in cooperative mode		*/
	#define OS_EVENTS			1				/* If event flags are supported						*/
	#define OS_FCFS				1				/* Allow the use of 1st come 1st serve semaphore	*/
  #ifdef __ABASSI_H__							/* mAbassi & uAbassi don't need Idle anymore		*/
	#define OS_IDLE_STACK		TC_TSK_STACK	/* If IdleTask supplied & if so, stack size			*/
  #else
	#define OS_IDLE_STACK		0
  #endif
	#define OS_LOGGING_TYPE		0				/* Type of logging to use							*/
	#define OS_MAILBOX			1				/* If mailboxes are used							*/
  #ifndef OS_MAX_PEND_RQST
	#define OS_MAX_PEND_RQST	128U			/* Maximum number of sempahores posted in ISRs		*/
  #endif
	#define OS_MIN_STACK_USE	0				/* If the kernel minimizes its stack usage			*/
	#define OS_MTX_DEADLOCK		0				/* This test validates this							*/
	#define OS_MTX_INVERSION	1				/* To enable protection against priority inversion	*/
	#define OS_NAMES			-1				/* != 0 when named Tasks / Semaphores / Mailboxes	*/
  #ifndef OS_NESTED_INTS
	#define OS_NESTED_INTS		1				/* If operating with nested interrupts				*/
  #endif
	#define OS_PRIO_CHANGE		1				/* If a task priority can be changed at run time	*/
  #ifdef OS_CMSIS_RTOS
	#define OS_PRIO_MIN			6				/* Max priority, Idle = OS_PRIO_MIN, AdameEve = 0	*/
  #else
	#define OS_PRIO_MIN			30				/* Max priority, Idle = OS_PRIO_MIN, AdameEve = 0	*/
  #endif
	#define OS_PRIO_SAME		1				/* Support multiple tasks with the same priority	*/
	#define OS_ROUND_ROBIN		-100000			/* Use round-robin, value specifies period in uS	*/
  #ifdef OS_CMSIS_RTOS
	#define OS_RUNTIME			0				/* If create Task / Semaphore / Mailbox at run time	*/
  #else
	#define OS_RUNTIME			1				/* If create Task / Semaphore / Mailbox at run time	*/
  #endif
	#define OS_SEARCH_ALGO		0				/* If using a fast search							*/
  #ifdef OS_CMSIS_RTOS
	#define OS_STARVE_PRIO		-6				/* Priority threshold for starving protection		*/
  #else
	#define OS_STARVE_PRIO		-30				/* Priority threshold for starving protection		*/
  #endif
	#define OS_STARVE_RUN_MAX	-100			/* Maximum Timer Tick for starving protection		*/
	#define OS_STARVE_WAIT_MAX	-100			/* Maximum time on hold for starving protection		*/
	#define OS_STATIC_BUF_MBX	0				/* when OS_STATIC_MBOX != 0, # of buffer element	*/
	#define OS_STATIC_MBX		0				/* If !=0 how many mailboxes						*/
	#define OS_STATIC_NAME		0				/* If named mailboxes/semaphore/task, size in char	*/
	#define OS_STATIC_SEM		0				/* If !=0 how many semaphores and mutexes			*/
	#define OS_STATIC_STACK		0
	#define OS_STATIC_TASK		0				/* If !=0 how many tasks (excluding A&E and Idle)	*/
	#define OS_STATIC_TIM_SRV	0				/* If !=0 how many timer services					*/
	#define OS_TASK_SUSPEND		1				/* If a task can suspend another one				*/
	#define OS_TIMEOUT			1				/* !=0 enables blocking timeout						*/
	#define OS_TIMER_CB			1				/* !=0 gives the timer callback period				*/
	#define OS_TIMER_SRV		1				/* !=0 includes the timer services					*/
	#define OS_TIMER_US			10000			/* !=0 enables timer & specifies the period in uS	*/
	#define OS_USE_TASK_ARG		1				/* If tasks have arguments							*/
#endif

/* ------------------------------------------------------------------------------------------------ */

  #ifdef OS_STACK_CHECK
	#undef OS_STACK_CHECK
  #endif
  #define OS_STACK_CHECK		1

  #ifdef OS_OUT_OF_MEM
	#undef OS_OUT_OF_MEM
  #endif
  #define OS_OUT_OF_MEM			1

  #ifdef __UABASSI_H__
	#undef OS_MAX_PEND_RQST
	#undef OS_NESTED_INTS
	#undef OS_PRIO_MIN
  #endif

#endif

/* EOF */
