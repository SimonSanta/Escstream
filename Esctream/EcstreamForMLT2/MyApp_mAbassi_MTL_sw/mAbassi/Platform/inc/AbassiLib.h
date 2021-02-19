/* ------------------------------------------------------------------------------------------------ */
/* FILE :		AbassiLib.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Configuration build options for the libraries										*/
/*																									*/
/* IMPORTANT:																						*/
/*				For the RTOS supplied in a library:													*/
/*				Although there are a few build options definitions enclosed between					*/
/*				#ifndef - #endif, ONLY OS_TIMER_US can be externally set.							*/
/*				DO NOT SET any of the others.														*/
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
/*	$Revision: 1.12 $																				*/
/*	$Date: 2019/02/10 16:54:19 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __ABASSILIB_H__
#define __ABASSILIB_H__		1

#define OS_DEF_IN_MAKE 		1					/* To inform mAbaasi to not use Abassi.h defintions	*/

#ifdef __UABASSI_H__							/* uAbassi needs very few build options				*/
 #ifndef OS_MAX_PEND_RQST
   #define OS_MAX_PEND_RQST	4096U				/* ISR request queue size							*/
 #endif
 #ifndef OS_TIMER_US							/* Conditional to set app specific					*/
   #define OS_TIMER_US		10000				/* When !=0 enable & timer vaalue in uS				*/
 #endif

#else

 #define OS_ALLOC_SIZE		0					/* Dynamic memory allocation done with malloc()		*/
 #define OS_COOPERATIVE		0					/* Don't emulate a cooperative RTOS					*/
 #define OS_EVENTS			1					/* If event flags are supported						*/
 #define OS_FCFS			1					/* Allow the use of 1st come 1st served unblocking	*/
 #define OS_GROUP			-1					/* Group supprted with memory allocation			*/
 #ifndef OS_IDLE_STACK
  #ifdef __ABASSI_H__
   #define OS_IDLE_STACK	1024				/* If IdleTask supplied & if so, stack size			*/
  #else
   #define OS_IDLE_STACK	0					/* mAbassi does not need an Idle Task				*/
  #endif
 #endif
 #define OS_LOGGING_TYPE	0					/* No logging facilities							*/
 #define OS_MAILBOX			1					/* Mailboxes are supported							*/
 #ifndef OS_MAX_PEND_RQST
   #define OS_MAX_PEND_RQST	4096U				/* ISR request queue size							*/
 #endif
 #define OS_MBXPUT_ISR		1					/* Mailbox report in ISRs if full/empty				*/
 #define OS_MEM_BLOCK		1					/* Memory block supported with memory allocation	*/
 #define OS_MIN_STACK_USE	0					/* Not minimizing the stack usage					*/
 #define OS_MTX_DEADLOCK	1					/* Detect mutex deadlock condition					*/
 #define OS_MTX_INVERSION	1					/* Priority inhertance is enabled					*/
 #define OS_MTX_OWN_UNLOCK	0					/* Any tasks can unlock a mutex						*/
 #define OS_NAMES			1					/* Use names with local copies in descriptors		*/
 #ifndef OS_NESTED_INTS
   #define OS_NESTED_INTS	1					/* Nested is a superset of non-nested				*/
 #endif
 #define OS_OUT_OF_MEM		1					/* Detect out of memory / ISR queue full  condition	*/
 #ifndef OS_PERF_MON							/* NOT USER SETTABLE - USE WHEN BUILING ELIBRARIES	*/
   #define OS_PERF_MON		0					/* Performance monitoring not provided				*/
 #endif
 #define OS_PRE_CONTEXT		0					/* Hook-up before context switch not supported		*/
 #define OS_PRIO_CHANGE		1					/* Task priorities can change during run time		*/
 #define OS_PRIO_MIN		30					/* Max priority, Idle = OS_PRIO_MIN, AdameEve = 0	*/
 #define OS_PRIO_SAME		1					/* Multiple tasks can have the same priority		*/
 #define OS_ROUND_ROBIN		-100000				/* Asymmetric round-robin, default run: 100 ticks	*/
 #define OS_RUNTIME			1					/* Run-time service creation						*/
 #define OS_SEARCH_ALGO		0					/* Basic linear search								*/
 #define OS_STACK_CHECK		1					/* Check stack overruns								*/
 #define OS_STARVE_PRIO		-(OS_PRIO_MIN)		/* Default strvation threshold priority				*/
 #define OS_STARVE_RUN_MAX	-100				/* Maximum Timer Tick for starving protection		*/
 #define OS_STARVE_WAIT_MAX	-100				/* Maximum time on hold for starving protection		*/
 #define OS_STATIC_BUF_MBLK	0
 #define OS_STATIC_BUF_MBX	0					/* when OS_STATIC_MBOX != 0, # of buffer element	*/
 #define OS_STATIC_MBLK		0
 #define OS_STATIC_MBX		0					/* Not using static meilbox descriptors				*/
 #define OS_STATIC_NAME		0					/* Using mem alloc for name copying					*/
 #define OS_STATIC_SEM		0					/* If !=0 how many semaphores and mutexes			*/
 #define OS_STATIC_STACK	0					/* Not using static semaphores descriptors			*/
 #define OS_STATIC_TASK		0					/* Not using static task descriptors				*/
 #define OS_STATIC_TIM_SRV	0					/* Not using static timner service descriptors		*/
 #define OS_TASK_SUSPEND	1					/* Tasks can suspend other ones						*/
 #define OS_TIMEOUT			1					/* Enables blocking timeout							*/
 #define OS_TIMER_CB		1					/* RTOS timer calls TIMcallBack() at every tick		*/
 #define OS_TIMER_SRV		1					/* Includes the timer services						*/
 #ifndef OS_TIMER_US							/* Conditional to set app specific					*/
   #define OS_TIMER_US		10000				/* When !=0 enable & timer vaalue in uS				*/
 #endif
 #define OS_USE_TASK_ARG	1					/* Tasks can access arguments						*/
 #define OS_WAIT_ABORT		1					/* Abort of task blocking supported					*/
												/* ------------------------------------------------ */
												/* Multi-core specific build options				*/
 #ifndef OS_MP_TYPE
   #define OS_MP_TYPE		5U					/* For multi-core use BMP packed					*/
 #endif
 #define OS_TIM_TICK_MULTI	1					/* For mAbassi, use multi-core interrups			*/
 #ifndef OS_START_STACK
   #define OS_START_STACK	8192
 #endif

 #define OS_SYS_CALL		1					/* System Call layer supported						*/
#endif

#endif
/* EOF */
