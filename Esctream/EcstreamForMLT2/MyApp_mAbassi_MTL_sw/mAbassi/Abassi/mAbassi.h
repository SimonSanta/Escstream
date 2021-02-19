/* ------------------------------------------------------------------------------------------------ */
/* FILE :		mAbassi.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				All typedef, definitions and function prototypes for the							*/
/*				multi-core mAbassi RTOS																*/
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
/*  $Revision: 1.128 $																				*/
/*	$Date: 2019/02/10 16:53:40 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __MABASSI_H__							/* To protect against re-include					*/
#define __MABASSI_H__

#ifdef __cplusplus
  extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>

#if defined(OS_DEF_IN_INC)						/* Defines are in the AbassiDef.h include file		*/
  #include "AbassiDef.h"
#elif defined(OS_DEMO)							/* File use by Code Time Technologies for the demos	*/
 #if ((OS_DEMO) < 0)							/* -ve Demo values are used to build the libraries	*/
  #include "AbassiLib.h"
 #else											/* Regular demos									*/
  #include "AbassiDemo.h"						/* All demo RTOS build configurations are there		*/
 #endif
#elif defined(OS_TEST_HOOK)						/* File use by Code Time Technologies for testing	*/
  #include "AbassiTestCfg.h"					/* All test RTOS build configurations are there		*/

/* ------------------------------------------------------------------------------------------------ */
												/* Define these in the makefile of set the values	*/
#elif !defined(OS_DEF_IN_MAKE)					/* ------------------------------------------------ */
												/* BUILD OPTIONS THAT MUST DE DEFINED				*/
												/* ------------------------------------------------ */
	#define OS_ALLOC_SIZE		0				/* When !=0, RTOS supplied OSalloc					*/
	#define OS_COOPERATIVE		0				/* When !=0, the  kernel is in cooperative mode		*/
	#define OS_EVENTS			0				/* != 0 when event flags are supported				*/
	#define OS_FCFS				0				/* Allow the use of 1st come 1st serve services		*/
	#define OS_LOGGING_TYPE		0				/* Type of logging to use							*/
	#define OS_MAILBOX			0				/* != 0 when mailboxes are used						*/
	#define OS_MAX_PEND_RQST	0U				/* Maximum number of requests performed in ISRs		*/
	#define OS_MEM_BLOCK		0				/* If the block memory pool part of the build		*/
	#define OS_MIN_STACK_USE	0				/* If the kernel minimizes its stack usage			*/
	#define OS_MTX_DEADLOCK		0				/* != 0 to enable mutex deadlock protection			*/
	#define OS_MTX_INVERSION	0				/* >0 priority inheritance, <0 priority ceiling		*/
	#define OS_NAMES			0				/* != 0 when named Tasks / Semaphores / Mailboxes	*/
	#define OS_PRIO_CHANGE		0				/* If a task priority can be changed at run time	*/
	#define OS_PRIO_MIN			0				/* Max priority, Idle = OS_PRIO_MIN, AdameEve = 0	*/
	#define OS_PRIO_SAME		0				/* Support multiple tasks with the same priority	*/
	#define OS_ROUND_ROBIN		0				/* Use round-robin, value specifies period in uS	*/
	#define OS_RUNTIME			0				/* If create Task / Semaphore / Mailbox at run time	*/
	#define OS_SEARCH_ALGO		0				/* If using a fast search							*/
	#define OS_STARVE_PRIO		0				/* Priority threshold for starving protection		*/
	#define OS_STARVE_RUN_MAX	0				/* Maximum Timer Tick for starving protection		*/
	#define OS_STARVE_WAIT_MAX	0				/* Maximum time on hold for starving protection		*/
	#define OS_STATIC_BUF_MBX	0				/* When OS_STATIC_MBX != 0, # of buffer elements	*/
	#define OS_STATIC_MBX		0				/* If !=0 how many mailboxes						*/
	#define OS_STATIC_NAME		0				/* If named mailboxes/semaphore/task, size in char	*/
	#define OS_STATIC_SEM		0				/* If !=0 how many semaphores and mutexes			*/
	#define OS_STATIC_STACK		0				/* if !=0 number of bytes for all stacks			*/
	#define OS_STATIC_TASK		0				/* If !=0 how many tasks (excluding A&E and Idle)	*/
	#define OS_STATIC_TIM_SRV	0				/* If !=0 how many timer services					*/
	#define OS_TASK_SUSPEND		0				/* If a task can suspend another one				*/
	#define OS_TIMEOUT			0				/* !=0 enables blocking timeout						*/
	#define OS_TIMER_CB			0				/* !=0 gives the timer callback period				*/
	#define OS_TIMER_SRV		0				/* !=0 includes the timer services					*/
	#define OS_TIMER_US			0				/* !=0 enables timer & specifies the period in uS	*/
	#define OS_USE_TASK_ARG		0				/* If tasks have arguments							*/
	#define OS_WAIT_ABORT		0				/* If XXXabort() components are included			*/
												/* ------------------------------------------------ */
												/* OPTIONAL OPTIONS SET AS IF NOT DEFINED			*/
												/* ------------------------------------------------ */
	#define OS_C_PLUS_PLUS		0				/* When !=0, C++ abstraction layer compatibility	*/
	#define OS_GROUP			0				/* If groups of trigger are supported				*/
	#define OS_GRP_XTRA_FIELD	0
	#define OS_HASH_ALL			0U
	#define OS_HASH_MBLK		0U
	#define OS_HASH_MBX			0U
	#define OS_HASH_MUTEX		0U
	#define OS_HASH_SEMA		0U
	#define OS_HASH_TASK		0U
	#define OS_HASH_TIMSRV		0U
	#define OS_IDLE_STACK		0				/* If IdleTask supplied & if so, stack size			*/
	#define OS_MBX_XTRA_FIELD	0
	#define OS_MBLK_XTRA_FIELD	0
	#define OS_MBXPUT_ISR		0
	#define OS_MTX_OWN_UNLOCK	0				/* !=0 only allows ISR and owner to unlock a mutex	*/
	#define OS_NESTED_INTS		0				/* != 0 operating with nested interrupts			*/
	#define OS_OUT_OF_MEM		0				/* If trapping out of memory conditions				*/
	#define OS_PERF_MON			0				/* If performance monitoring is included			*/
	#define OS_PRE_CONTEXT		0				/* If the callback before OScontext is needed		*/
	#define OS_SEM_XTRA_FIELD	0
	#define OS_STACK_CHECK		0				/* Set to != for checking stack collisions			*/
	#define OS_STATIC_BUF_MBLK	0				/* When OS_STATIC_MBLK != 0, # of memory bytes		*/
	#define OS_STATIC_MBLK		0				/* If !=0 how many block memory descriptors			*/
	#define OS_SYS_CALL			0
	#define OS_TASK_XTRA_FIELD	0				/* If adding extra scratch area in task descriptors	*/
	#define OS_TIM_XTRA_FIELD	0
												/* ------------------------------------------------ */
												/* MULTICORE OPTIONS THAT MUST BE DEFINED			*/
												/* ------------------------------------------------ */
	#define OS_MP_TYPE			0U
	#define OS_SPINLOCK_BASE	0
	#define OS_START_STACK		0
	#define OS_N_CORE			1

#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifndef OSputchar
  #define OSputchar(x)	(putchar(x))			/* Character output for ASCII logging				*/
#endif

/* ------------------------------------------------------------------------------------------------	*/
/* ------------------------------------------------------------------------------------------------	*/
/* ------------------------------------------------------------------------------------------------	*/
/* Compiler & Target processor specifics											   				*/
/* Many defines could be set to default for the most common value but MISRA does not accept #undef	*/
/* So we use an overload definition (OX_nnn) instead of the original (OS_nnn)						*/

/* ------------------------------------------------------------------------------------------------ */
/* ARM CORTEX A9 / CCS																				*/
/* ------------------------------------------------------------------------------------------------ */

#if (defined(__TI_TMS470_V4__)																		\
 ||  defined(__TI_TMS470_V5__)																		\
 ||  defined(__TI_TMS470_V6__)																		\
 ||  defined(__TI_TMS470_V7A8__)																	\
 ||  defined(__TI_TMS470_V7R4__))
	#define OX_AE_STACK_SIZE		0			/* A&E stack is main()'s, no user overload			*/
	#if (__TI_EABI_ASSEMBLER)
		#define OS_BSS_ZERO			1			/* This ABI zeroes the BSS at start-up				*/
	#else
		#define OS_BSS_ZERO			0			/* TIEABI & TI ARM9 ABI don't zero BSS at start-up	*/
	#endif
	#define OX_MEMSET_LIB			1			/* The function call is smaller than in-line		*/
	#ifndef OS_N_INTERRUPTS
		#define OX_N_INTERRUPTS		1024		/* Set 1024 as it's the maximum the GIC supports	*/
	#endif
	#define OX_NESTED_INTS			0			/* The ARM9 nests FIQ over IRQ: RTOS is only IRQ	*/
	#define OX_STACK_GROWTH			(-1)		/* Stack grows toward lower memory addresses		*/
	#define OX_USE_STRLIB			0			/* strcpy() under some optimization only works with	*/
												/* strings that are 4 byte aligned. So use in-line	*/
	typedef int64_t OSalign_t;					/* Data type that will use the alignment size		*/
	#define OX_PM_ISR_ENABLE(_IsrN)	GICenable(_IsrN, 128, 1)
	#define OX_CACHE_LSIZE			32			/* Cache lines are 32 bytes							*/

	extern void                   COREcacheON(void);
	extern volatile unsigned int *COREgetPeriph(void);
	extern void                   DCacheFlushRange(const void *Addr, int Len);
	extern void                   DCacheInvalRange(const void *Addr, int Len);
	extern void                  *MMUlog2Phy(const void *Addr);
	extern void                  *MMUphy2Log(const void *Addr);
	extern void                   GICenable(int IntNmb, int Prio, int Edge);
	extern void                   GICinit(void);
	extern void                   DATAabort_Handler(void);
	extern void                   FIQ_Handler(void);
	extern void                   PFabort_Handler(void);
	extern void                   SWI_Handler(int SWInmb);

	#if ((OS_N_CORE) == 1) || (((OS_MP_TYPE) & 6U) == 0U)
		#define OX_INT_CTRL_INIT()																	\
			GICinit()
	#else
		#define OX_INT_CTRL_INIT()																	\
			do {																					\
				int ij;																				\
				for (ij=0 ; ij<OX_N_CORE ; ij++) {													\
					OSisrInstall(ij, &COREwakeISR);													\
				}																					\
				GICinit();																			\
			} while(0)
	#endif

/* ------------------------------------------------------------------------------------------------ */
/* ARM CORTEX A9 / ARM CC																			*/
/* MUST BE BEFORE GCC BECAUSE KEIL DEFINES __GNUC__													*/
/* ------------------------------------------------------------------------------------------------ */

#elif (defined(__CC_ARM) && defined(__TARGET_ARCH_7_A))
	#define OX_AE_STACK_SIZE		0			/* A&E stack is main()'s, no user overload			*/
	#define OS_BSS_ZERO				1			/* ARM CC zeroes the BSS at start-up				*/
	#define OX_MEMSET_LIB			1			/* The function call is smaller than in-line		*/
	#ifndef OS_N_INTERRUPTS
		#define OX_N_INTERRUPTS		1024		/* Set 1024 as it's the maximum the GIC supports	*/
	#endif
	#define OX_NESTED_INTS			0			/* The ARM9 nests FIQ over IRQ: RTOS is only IRQ	*/
	#define OX_STACK_GROWTH			(-1)		/* Stack grows toward lower memory addresses		*/
	#define OX_USE_STRLIB			1			/* The function call is more efficient than inline	*/

	typedef int64_t OSalign_t;					/* Data type that will use the alignment size		*/
	#define OX_PM_ISR_ENABLE(_IsrN)	GICenable(_IsrN, 128, 1)
	#define OX_CACHE_LSIZE			32			/* Cache lines are 32 bytes							*/

	extern void                   COREcacheON(void);
	extern volatile unsigned int *COREgetPeriph(void);
	extern void                   DCacheFlushRange(const void *Addr, int Len);
	extern void                   DCacheInvalRange(const void *Addr, int Len);
	extern void                  *MMUlog2Phy(const void *Addr);
	extern void                  *MMUphy2Log(const void *Addr);
	extern void                   GICenable(int IntNmb, int Prio, int Edge);
	extern void                   GICinit(void);
	extern void                   DATAabort_Handler(void);
	extern void                   FIQ_Handler(void);
	extern void                   PFabort_Handler(void);
	extern void                   SWI_Handler(int SWInmb);

	#if ((OS_N_CORE) == 1) || (((OS_MP_TYPE) & 6U) == 0U)
		#define OX_INT_CTRL_INIT()																	\
			GICinit()
	#else
		#define OX_INT_CTRL_INIT()																	\
			do {																					\
				int ij;																				\
				for (ij=0 ; ij<OX_N_CORE ; ij++) {													\
					OSisrInstall(ij, &COREwakeISR);													\
				}																					\
				GICinit();																			\
			} while(0)
	#endif

/* ------------------------------------------------------------------------------------------------ */
/* ARM CORTEX A53 / GCC																				*/
/* ------------------------------------------------------------------------------------------------ */

#elif (defined(__GNUC__) && ((__ARM_ARCH) == 8))													\
   || (defined(__GNUC__) && defined(__aarch64__))
	#define OX_AE_STACK_SIZE		0			/* Most ports don't need a stack for Adam & Eve		*/
	#define OS_BSS_ZERO				1			/* GNU zeroes the BSS at start-up					*/
	#define OX_MEMSET_LIB			1			/* The function call is more efficient than inline	*/
	#ifndef OS_N_INTERRUPTS
		#define OX_N_INTERRUPTS		1024		/* Set 1024 as it's the maximum the GIC supports	*/
	#endif
	#define OX_NESTED_INTS			0			/* The ARM9 nests FIQ over IRQ: RTOS is only IRQ	*/
	#define OX_STACK_GROWTH			(-1)		/* Stack grows toward lower memory addresses		*/
	#define OX_USE_STRLIB			1			/* The function call is more efficient than inline	*/

	typedef __int128 OSalign_t;					/* Data type that will use the alignment size		*/
	#define OX_PM_ISR_ENABLE(_IsrN)	GICenable(_IsrN, 128, 1)
	#define OX_CACHE_LSIZE			64			/* Cache lines are 64 bytes							*/

	extern void                   COREcacheON(void);
	extern volatile unsigned int *COREgetPeriph(void);
	extern void                   DCacheFlushRange(const void *Addr, int Len);
	extern void                   DCacheInvalRange(const void *Addr, int Len);
	extern void                  *MMUlog2Phy(const void *Addr);
	extern void                  *MMUphy2Log(const void *Addr);
	extern void                   GICenable(int IntNmb, int Prio, int Edge);
	extern void                   GICinit(void);

	extern void FIQ_Handler(void);
	extern void SError_Handler(uint32_t Syndrome, void *Ret, uint32_t Pstate, int Level);
	extern void Sync_Handler(uint32_t Syndrome, void *Ret, uint32_t Pstate, int Level);

	#if ((OS_N_CORE) == 1) || (((OS_MP_TYPE) & 6U) == 0U)
		#define OX_INT_CTRL_INIT()																	\
			GICinit()
	#else
		#define OX_INT_CTRL_INIT()																	\
			do {																					\
				int ij;																				\
				for (ij=0 ; ij<OX_N_CORE ; ij++) {													\
					OSisrInstall(ij, &COREwakeISR);													\
				}																					\
				GICinit();																			\
			} while(0)
	#endif
	#ifdef OS_PLATFORM
		#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00007753)
			#define OX_TIM_TICK_ACK()	 do {														\
											volatile uint32_t _dum;									\
											_dum = *((volatile uint32_t *)0xFF140054);				\
											_dum = _dum;											\
										} while(0)
		#endif
	#endif

/* ------------------------------------------------------------------------------------------------ */
/* ARM CORTEX A9 / GCC																				*/
/* MUST BE AFTER KEIL ARMCC BECAUSE KEIL ALSO DEFINES __GNUC__										*/
/* ------------------------------------------------------------------------------------------------ */

#elif (defined(__GNUC__) && defined(__ARM_ARCH_7A__))
	#define OX_AE_STACK_SIZE		0			/* Most ports don't need a stack for Adam & Eve		*/
	#define OS_BSS_ZERO				1			/* GNU zeroes the BSS at start-up					*/
	#define OX_MEMSET_LIB			1			/* The function call is more efficient than inline	*/
	#ifndef OS_N_INTERRUPTS
		#define OX_N_INTERRUPTS		1024		/* Set 1024 as it's the maximum the GIC supports	*/
	#endif
	#define OX_NESTED_INTS			0			/* The ARM9 nests FIQ over IRQ: RTOS is only IRQ	*/
	#define OX_STACK_GROWTH			(-1)		/* Stack grows toward lower memory addresses		*/
	#define OX_USE_STRLIB			1			/* The function call is more efficient than inline	*/

	typedef int64_t OSalign_t;					/* Data type that will use the alignment size		*/
	#define OX_PM_ISR_ENABLE(_IsrN)	GICenable(_IsrN, 128, 1)
	#define OX_CACHE_LSIZE			32			/* Cache lines are 32 bytes							*/

	extern void                   COREcacheON(void);
	extern volatile unsigned int *COREgetPeriph(void);
	extern void                   DCacheFlushRange(const void *Addr, int Len);
	extern void                   DCacheInvalRange(const void *Addr, int Len);
	extern void                  *MMUlog2Phy(const void *Addr);
	extern void                  *MMUphy2Log(const void *Addr);
	extern void                   GICenable(int IntNmb, int Prio, int Edge);
	extern void                   GICinit(void);
	extern void                   DATAabort_Handler(void);
	extern void                   FIQ_Handler(void);
	extern void                   PFabort_Handler(void);
	extern void                   SWI_Handler(int SWInmb);

	#if ((OS_N_CORE) == 1) || (((OS_MP_TYPE) & 6U) == 0U)
		#define OX_INT_CTRL_INIT()																	\
			GICinit()
	#else
		#define OX_INT_CTRL_INIT()																	\
			do {																					\
				int ij;																				\
				for (ij=0 ; ij<OX_N_CORE ; ij++) {													\
					OSisrInstall(ij, &COREwakeISR);													\
				}																					\
				GICinit();																			\
			} while(0)
	#endif

/* ------------------------------------------------------------------------------------------------ */
/* ARM CORTEX M3 / CCS																				*/
/* ------------------------------------------------------------------------------------------------ */

#elif defined(__TI_TMS470_V7M3__)
	#define OX_AE_STACK_SIZE		0			/* A&E stack is main()'s, no user overload			*/
	#if (__TI_EABI_ASSEMBLER)
		#define OS_BSS_ZERO			1			/* This ABI zeroes the BSS at start-up				*/
	#else
		#define OS_BSS_ZERO			0			/* TI ARM9 ABI doesn't zero BSS at start-up			*/
	#endif										/* TIABI does not support the Cortex M3				*/
	#define OX_ISR_TBL_OFFSET		1			/* We need to include SysTick (interrupt # == -1)	*/
	#define OX_MEMSET_LIB			0			/* The function call same as in-line, keep in-line	*/
	#ifndef OS_N_INTERRUPTS
		#define OX_N_INTERRUPTS		241			/* NVIC has 256 sources of interrupt, drop 15 1st	*/
	#endif
	#define OX_NESTED_INTS			1			/* The ARM has 8 nested (NVIC) interrupt levels		*/
	#define OX_STACK_GROWTH			(-1)		/* Stack grows toward lower memory addresses		*/
	#define OX_USE_STRLIB			1			/* The function call is smaller than in-line		*/

	#ifndef OS_PLATFORM
		#ifdef omap4460
			#define OS_PLATFORM 			0x4460
		#endif
	#endif

	#ifndef OS_PLATFORM
		#error "Abassi: The build option OS_PLATFORM hasn't been defined"
		#error "        SMP M3 must have the target platform defined:"
		#error "		== 0xXX004460 : OMAP4460"
	#else
		#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x4460)
			#define OX_INT_CTRL_INIT()	do {														\
											NVIC_enableIRQ(3);										\
											OSisrInstall(3, &COREwakeISR);							\
										} while(0)
		#else
			#error "Abassi: The build option OS_PLATFORM is set to an invalid platform"
		#endif
	#endif

	typedef int64_t OSalign_t;					/* Data type that will use the alignment size		*/
	#define OX_PM_ISR_ENABLE(_IsrN)	OS_NVIC_ON(_IsrN)

	#define OS_NVIC_OFF(_IntN) 	do {																\
									int _NN = _IntN;												\
									((((volatile uint32_t *)0xE000E180)[(_NN)/32])					\
	                          		                                        = 1U << ((_NN)&0x1F));	\
								} while(0)
	#define OS_NVIC_ON(_IntN) 	do {																\
									int _NN = _IntN;												\
									((((volatile uint32_t *)0xE000E100)[(_NN)/32])					\
									                                        = 1U << ((_NN)&0x1F));	\
								} while(0)
												/* Functions unique to CCS / M3						*/
	extern void BusFault_Handler   (void);
	extern void DebugMon_Handler   (void);
	extern void HardFault_Handler  (void);
	extern void MemManage_Handler  (void);
	extern void NMI_Handler        (void);
	extern void NVIC_enableIRQ     (int IntNmb);
	extern void SVC_Handler        (void);
	extern void UsageFault_Handler (void);

	#ifdef __MABASSI_C__
		extern void COREinit(int CoreID);		/* Only used in mAbassi.c							*/
	#endif

	#define OX_XTRA_CORE_START(__CoreID)															\
		do { 																						\
			COREinit(__CoreID);																		\
		} while(0)

/* ------------------------------------------------------------------------------------------------ */
/* UNSUPPORTED PLATFORM																				*/
/* ------------------------------------------------------------------------------------------------ */

#else
	#error "mAbassi: Unsupported processor/compiler"

#endif

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* LIBRARIES RE-ENTRANCE PROTECTION:																*/
/*            Libraries typically uses the same re-entrance protection for all platforms			*/
/*																									*/

#ifndef	OX_INT_CTRL_INIT
	#define OX_INT_CTRL_INIT()		OX_DO_NOTHING()
#endif

/* ------------------------------------------------------------------------------------------------ */
/* TI/CCS - LIBRARY RE-ENTRANCE PROTECTION															*/
/* ------------------------------------------------------------------------------------------------ */

#if defined(__TI_COMPILER_VERSION__)

	#ifndef OX_DOING_BSS_0
		#define OX_DOING_BSS_0()	OX_DO_NOTHING()
	#endif

	#ifndef OS_CCS_REENT
		#define OS_CCS_REENT 		1
	#endif

	#if ((OS_CCS_REENT) != 0)

		#define OX_LIB_REENT_PROTECT	(OS_CCS_REENT)

		#if (defined(__ABASSI_C__) || defined(__MABASSI_C__) || defined(__UABASSI_C__))
			#include <_lock.h>					/* Install the protection for multi-threading on	*/
			void OS_CCSlock(void);				/* Abassi mutex protection for the library			*/
			void OS_CCSunlock(void);			/* Abassi mutex protection for the library			*/
			int G_OSmtxOK = 0;					/* Set to 0 in case BSS not zeroed					*/
		#else
			extern int G_OSmtxOK;
		#endif

		#define OX_DEVICE_PREINIT()																	\
			do {																					\
				G_OSmtxOK = 1;																		\
				_register_lock(&OS_CCSlock);														\
				_register_unlock(&OS_CCSunlock);													\
				OX_INT_CTRL_INIT();																	\
			} while(0)

		#define OX_XTRA_FCT																			\
			static void OS_CCSlockNone(void) { return; }											\
			int _system_pre_init(void) {															\
				OX_DOING_BSS_0();																	\
				_register_lock(&OS_CCSlockNone);													\
				_register_unlock(&OS_CCSlockNone);													\
				return(1);																			\
			}																						\
			void OS_CCSlock(void) {																	\
				if (COREreentOK() != 0) {															\
					MTXsafeLock(G_OSmutex);															\
				}																					\
				return;																				\
			}																						\
			void OS_CCSunlock(void) {																\
				if (COREreentOK() != 0) {															\
					MTXunlock(G_OSmutex);															\
				}																					\
				return;																				\
			}

	#else										/* No library protection							*/
		#if (defined(__ABASSI_C__) || defined(__MABASSI_C__) || defined(__UABASSI_C__))
			#include <_lock.h>
		#endif

		#define OX_XTRA_FCT																			\
			static void OS_CCSnolock(void) { return; }												\
			int _system_pre_init(void) {															\
				OX_DOING_BSS_0();																	\
				_register_lock(&OS_CCSnolock);														\
				_register_unlock(&OS_CCSnolock);													\
				return(1);																			\
			}

	#endif

/* ------------------------------------------------------------------------------------------------ */
/* KEIL - LIBRARY RE-ENTRANCE PROTECTION															*/
/*        MUST BE BEFORE GCC NEWLIB BECAUSE KEIL DEFINES __GNUC__									*/
/* ------------------------------------------------------------------------------------------------ */

#elif defined(__CC_ARM)

	#ifndef OS_KEIL_REENT
		#define OS_KEIL_REENT	0
	#endif
												/* MicroLib doesn't support re-entrance protection	*/
	#if (((OS_KEIL_REENT) != 0) && !defined(__MICROLIB))
		#define OX_LIB_REENT_PROTECT		(OS_KEIL_REENT)

		#ifdef OX_TASK_XTRA_FIELD_1
			#error "With library re-entrance protection enabled, 1st entry in XTRA_FIELD is"
			#error "reserved.  Use a higher OX_TASK_XTRA_FIELD_n"
		#else
			#define OX_TASK_XTRA_FIELD_1	1
		#endif

		extern char __libspace_start[96];		/* Default libspace area							*/

		#if (defined(__ABASSI_C__) || defined(__MABASSI_C__) || defined(__UABASSI_C__))
			int G_OSmtxOK;						/* No need to set to zero as BSS is zeroed			*/
		#else
			extern int G_OSmtxOK;
		#endif

		#if ((OS_KEIL_REENT) > 0)
			#if (defined(__ABASSI_C__) || defined(__MABASSI_C__) || defined(__UABASSI_C__))
		  	  #define OX_TASK_XTRA(_TskDsc, _Fct)													\
				do {																				\
					if (_Fct != NULL) {																\
						void *_Reent;																\
						_Reent = OSalloc(96);														\
						 (_TskDsc)->XtraData[0] = (intptr_t)_Reent;									\
						memset(_Reent, 0, 96);														\
					}																				\
				} while(0)
			#else								/* When used outside of Abassi.c by TSK_SETUP()		*/
		  	  #define OX_TASK_XTRA(_TskDsc, _Fct)	/* there is no need to check for A&E		*/	\
				do {																				\
					void *_Reent;																	\
					_Reent = OSalloc(96);															\
					 (_TskDsc)->XtraData[0] = (intptr_t)_Reent;										\
					memset(_Reent, 0, 96);															\
				} while(0)
			#endif

			#define TSKmultThrd(_TskDsc)	OX_DO_NOTHING()

		#else
			#define TSKmultThrd(_TskDsc)															\
				do {																				\
					void *_Reent;																	\
					_Reent = OSalloc(96);															\
					 (_TskDsc)->XtraData[0] = (intptr_t)_Reent;										\
					memset(_Reent, 0, 96);															\
				} while(0)
		#endif

		#define OX_DEVICE_PREINIT()																	\
			do {																					\
				G_OSmtxOK = 1;																		\
				OX_INT_CTRL_INIT();																	\
			} while(0)

		#define OX_XTRA_FCT																			\
			int _mutex_initialize(SEM_t *Sema) {													\
				Sema = Sema;																		\
				return(1);																			\
			}																						\
			void _mutex_acquire(SEM_t *Sema) {														\
				Sema = Sema;																		\
				if (COREreentOK() != 0) {															\
					MTXsafeLock(G_OSmutex);															\
				}																					\
			}																						\
			void _mutex_release(SEM_t *Sema) {														\
				Sema = Sema;																		\
				if (COREreentOK() != 0) {															\
					MTXunlock(G_OSmutex);															\
				}																					\
			}																						\
			void _mutex_free(SEM_t *Sema) {															\
				Sema = Sema;																		\
				return;																				\
			}																						\
			void *__user_perproc_libspace(void) {													\
				return((void *)&__libspace_start[0]);												\
			}																						\
			void *__user_perthread_libspace(void) { 												\
				void *Ptr;																			\
				Ptr = (void *)&__libspace_start[0];													\
				if (TSKmyID() != (TSK_t *)NULL) {													\
					if (TSKmyID()->XtraData[0] != (intptr_t)0) {									\
					    Ptr = (void *)TSKmyID()->XtraData[0];										\
					}																				\
				}																					\
				return(Ptr);																		\
			}
	#endif

/* ------------------------------------------------------------------------------------------------ */
/* GCC - NEWLIB RE-ENTRANCE PROTECTION																*/
/*       MUST BE AFTER KEIL ARM CC BECAUSE KEIL ALSO DEFINES __GNUC__								*/
/* ------------------------------------------------------------------------------------------------ */

#elif defined(__GNUC__)

	#ifndef OS_NEWLIB_REENT
		#define OS_NEWLIB_REENT		0
	#endif

	#if ((OS_NEWLIB_REENT) != 0)
		#include <reent.h>
		struct _reent *__getreent(void);

		#define OX_LIB_REENT_PROTECT		(OS_NEWLIB_REENT)

		#ifdef OS_CMSIS_RTOS
	      #if ((OS_CMSIS_RTOS) != 0)
			#define OS_NEWLIB_MUTEX_SINGLE -1	/* >0 :  G_OSmtxGCC	 ---   <0 : G_OSmutex 			*/
		  #else									/* ==0: individual mutexes created & destroyed		*/
			#define OS_NEWLIB_MUTEX_SINGLE	1
		  #endif
	      #if ((OS_CMSIS_RTOS)!= 0) && ((OS_NEWLIB_MUTEX_SINGLE) >= 0)
			#error "With CMSIS, OS_NEWLIB_MUTEX_SINGLE must be set to a negative value (<0)"
	      #endif								/* Due to MTXopen() not available + NO runtime		*/
		#endif

	    #ifndef OS_NEWLIB_MUTEX_SINGLE			/* If Newlib uses individual or global mutex		*/
			#define OS_NEWLIB_MUTEX_SINGLE	0	/* >0 :  G_OSmtxGCC	 ---   <0 : G_OSmutex 			*/
	    #endif									/* ==0: individual mutexes							*/

		#ifndef OS_NEWLIB_SHARE_STDIO			/* If stdio same across all tasks with reent protect*/
			#define OS_NEWLIB_SHARE_STDIO	1	/* Set to 2 if __sf_fake_??? are not found			*/
		#endif

		#ifndef OS_NEWLIB_REDEF
			#define OS_NEWLIB_REDEF			0
		#endif

		#ifdef OX_TASK_XTRA_FIELD_1
			#error "With library re-entrance protection enabled, 1st entry in XTRA_FIELD is"
			#error "reserved.  Use a higher OX_TASK_XTRA_FIELD_n"
		#else
			#define OX_TASK_XTRA_FIELD_1	1
		#endif

		#if ((OS_NEWLIB_SHARE_STDIO) != 0)		/* Sharing the same stdio							*/
			#define OX_NEWLIB_STDIO_COPY(_Tsk)	do {												\
				((struct _reent *)((_Tsk)->XtraData[0]))->_stdin     = _global_impure_ptr->_stdin;	\
				((struct _reent *)((_Tsk)->XtraData[0]))->_stdout    = _global_impure_ptr->_stdout;	\
				((struct _reent *)((_Tsk)->XtraData[0]))->_stderr    = _global_impure_ptr->_stderr;	\
				((struct _reent *)((_Tsk)->XtraData[0]))->__sdidinit = 1;							\
												} while(0)
		#else									/* Not sharing the same stdio						*/
			#define OX_NEWLIB_STDIO_COPY(_Tsk)	OX_DO_NOTHING()
		#endif

	 	#if ((defined(_REENT_SMALL) 																\
		 &&  (defined(__ABASSI_C__) || defined(__MABASSI_C__) || defined(__UABASSI_C__)))			\
		 &&  ((OS_NEWLIB_SHARE_STDIO) > 1))
			const struct __sFILE_fake __sf_fake_stdin  = {_NULL, 0, 0, 0, 0, {_NULL, 0}, 0, _NULL};
			const struct __sFILE_fake __sf_fake_stdout = {_NULL, 0, 0, 0, 0, {_NULL, 0}, 0, _NULL};
			const struct __sFILE_fake __sf_fake_stderr = {_NULL, 0, 0, 0, 0, {_NULL, 0}, 0, _NULL};
		#endif

		#if ((OS_NEWLIB_REENT) > 0)				/* Reentrance protection for all tasks				*/
			#if (defined(__ABASSI_C__) || defined(__MABASSI_C__) || defined(__UABASSI_C__))
				#define OX_TASK_XTRA(_TskDsc, _Fct)													\
					do {																			\
						if ((_Fct) != NULL) {	/* Not A&E: allocate / init / memo _reent 		*/	\
							struct _reent *_Reent;													\
							_Reent = (struct _reent *)OSalloc(sizeof(struct _reent));				\
							_REENT_INIT_PTR(_Reent);												\
							(_TskDsc)->XtraData[0] = (intptr_t)_Reent;								\
							OX_NEWLIB_STDIO_COPY(_TskDsc);											\
						}																			\
						else {					/* A&E uses the default data		 			*/	\
							(_TskDsc)->XtraData[0] = (intptr_t)_global_impure_ptr;					\
						}																			\
					} while(0)
			#else
		 		#define OX_TASK_XTRA(_TskDsc, _Fct)	/* There is no need to check for A&E		*/	\
					do {																			\
						struct _reent *_Reent;														\
						_Reent = (struct _reent *)OSalloc(sizeof(struct _reent));					\
						_REENT_INIT_PTR(_Reent);													\
						(_TskDsc)->XtraData[0] = (intptr_t)_Reent;									\
						OX_NEWLIB_STDIO_COPY(_TskDsc);												\
					} while(0)
			#endif
												/* Always define it when REENT PROTECT ENABLE		*/
			#define TSKmultThrd(_TskDsc)		OX_DO_NOTHING()

	    #else									/* Reentrance protection on selected tasks			*/
			#define OX_TASK_XTRA(_TskDsc, _Fct)														\
				do {																				\
					(_TskDsc)->XtraData[0] = (intptr_t)_global_impure_ptr;							\
				} while(0)

			#define TSKmultThrd(_TskDsc)															\
				do {																				\
					struct _reent *_Reent;															\
					_Reent = (struct _reent *)OSalloc(sizeof(struct _reent));						\
					_REENT_INIT_PTR(_Reent);														\
					(_TskDsc)->XtraData[0] = (intptr_t)_Reent;										\
					OX_NEWLIB_STDIO_COPY(_TskDsc);													\
				} while(0)

	    #endif
												/* For backward compatibility						*/
		#define OS_TASK_NEWLIB_TASK_REENT(_TskDsc)	TSKmultThrd(_TskDsc)

	    #if (defined(__ABASSI_C__) || defined(__MABASSI_C__) || defined(__UABASSI_C__))
			#include <envlock.h>				/* Includes needed with reentrance protection		*/
			#include <locale.h>
			#include <malloc.h>
			int G_OSmtxOK;						/* No need to set to 0 as BSS is zeroed				*/

		  #ifndef __gthr_generic_h				/* Depending on how libs were built, these typedef	*/
			typedef void *__gthread_key_t;		/* may or not exist if gthr_generic.h included		*/
			typedef void *__gthread_once_t;		/* __gthr_generic_h is defined in gthr_generic.h	*/
			typedef struct __gthread_mutex_s { void *__p; } __gthread_mutex_t;
			typedef struct __gthread_recursive_mutex_s { void *__p; } __gthread_recursive_mutex_t;
		  #endif

		#else
			extern int G_OSmtxOK;
	    #endif

	    #if ((OS_NEWLIB_MUTEX_SINGLE) == 0)		/* Individual mutexes inited & destroyed			*/
			#define OX_XTRA_VAR_AND_DEF MTX_t *G_OSmtxPark;											\
										MTX_t *G_OSmtxGCC;											\
										MTX_t *G_OSmtxGCCalloc;										\
										MTX_t *G_OSmtxGCCenv;										\
										MTX_t *G_OSmtxGCCtz;
			#define OX_SEM_PARKING		1		/* With init & destroy, need unused mutex parking 	*/
			#define NL_MUTEX			((*Mtx == NULL) ? G_OSmtxGCC : *Mtx)
			#define NL_MUTX_GLOB		(G_OSmtxGCC)
			#define NL_MTX_ALLOC		(G_OSmtxGCCalloc)
			#define NL_MTX_CHK			do {														\
											if (*Mtx == NULL) {										\
												__gthread_recursive_mutex_t *mutex = (void *)Mtx;	\
												NL_MTX_INIT();										\
											}														\
										} while (0);
			#define NL_MTX_ENV			(G_OSmtxGCCenv)
			#define NL_MTX_TZ			(G_OSmtxGCCtz)
			#define NL_MTX_DESTROY()	do {														\
											if (mutex->__p != NULL) {								\
												MTXsafeLock(G_OSmtxGCC);							\
												((MTX_t *)(mutex->__p))->ParkNext = G_OSmtxPark;	\
												G_OSmtxPark = (MTX_t *)mutex->__p;					\
												MTXunlock(G_OSmtxGCC);								\
											}														\
										} while(0)
			#define NL_MTX_INIT()		do {														\
											if (G_OSmtxOK != 0) {									\
												MTXsafeLock(G_OSmtxGCC);							\
												if (G_OSmtxPark == (MTX_t *)NULL) {					\
													mutex->__p = MTXopen(NULL);						\
												}													\
												else {												\
													G_OSmtxPark->Value = 1;							\
													mutex->__p  = G_OSmtxPark;						\
													G_OSmtxPark = G_OSmtxPark->ParkNext;			\
												}													\
												MTXunlock(G_OSmtxGCC);								\
											}														\
											else {													\
												mutex->__p = NULL;									\
											}														\
										} while(0)
			#define NL_MTX_MTHR_INIT()	do {														\
										    G_OSmtxPark=NULL;										\
											G_OSmtxGCC      = MTXopen("GCC mtx");					\
											G_OSmtxGCCalloc = MTXopen("GCC alloc");					\
											G_OSmtxGCCenv   = MTXopen("GCC env");					\
											G_OSmtxGCCtz    = MTXopen("GCC tz");					\
										} while(0)

	    #elif ((OS_NEWLIB_MUTEX_SINGLE) < 0)	/* Using only G_OSmutex								*/
			#define NL_MUTEX			(G_OSmutex)
			#define NL_MUTX_GLOB		(G_OSmutex)
			#define NL_MTX_ALLOC		(G_OSmutex)
			#define NL_MTX_CHK			OX_DO_NOTHING()
			#define NL_MTX_ENV			(G_OSmutex)
			#define NL_MTX_TZ			(G_OSmutex)
			#define NL_MTX_DESTROY()	OX_DO_NOTHING()
			#define NL_MTX_INIT()		do {mutex->__p=((G_OSmtxOK==0)?NULL:G_OSmutex);} while(0)
			#define NL_MTX_MTHR_INIT()	OX_DO_NOTHING()

	    #else									/* Using only G_OSmtxGCC							*/
			#define OX_XTRA_VAR_AND_DEF MTX_t *G_OSmtxGCC;
			#define NL_MUTEX			(G_OSmtxGCC)
			#define NL_MUTX_GLOB		(G_OSmtxGCC)
			#define NL_MTX_ALLOC		(G_OSmtxGCC)
			#define NL_MTX_CHK			OX_DO_NOTHING()
			#define NL_MTX_ENV			(G_OSmtxGCC)
			#define NL_MTX_TZ			(G_OSmtxGCC)
			#define NL_MTX_DESTROY()	OX_DO_NOTHING()
			#define NL_MTX_INIT()		do {mutex->__p=(G_OSmtxOK==0)?NULL:G_OSmtxGCC;} while(0)
			#define NL_MTX_MTHR_INIT()	do {G_OSmtxGCC=MTXopen("GCC mutex");} while(0)
	    #endif

	    #define OX_DEVICE_PREINIT()																	\
			do {																					\
				NL_MTX_MTHR_INIT();																	\
				G_OSmtxOK = 1;																		\
				OX_INT_CTRL_INIT();																	\
			} while(0)
											/* __generic_gxx_getspecific() not included due to		*/
											/* changes of function prototype across lib versions	*/
	    #define OX_XTRA_FCT																			\
			int __generic_gxx_active_p(void) {														\
				return(G_OSmtxOK != 0);		/* G_OSmtxOK is a good indicator for Abassi active	*/	\
			}																						\
			int __generic_gxx_once(__gthread_once_t *once, void(*func)(void)) {						\
				return(-1);					/* Not supported									*/	\
			}																						\
			int __generic_gxx_key_create(__gthread_key_t *keyp, void(*dtor)(void *)) {				\
				return(-1);					/* Not supported									*/	\
			}																						\
			int __generic_gxx_key_delete(__gthread_key_t key) {										\
				return(-1);					/* Not supported									*/	\
			}																						\
			int __generic_gxx_setspecific(__gthread_key_t key, const void *ptr) {					\
				return(-1);					/* Not supported									*/	\
			}																						\
			int __generic_gxx_mutex_destroy(__gthread_mutex_t *mutex) {								\
				NL_MTX_DESTROY();			/* Code specific for single or multiple mutexes		*/	\
				return(0);																			\
			}																						\
			void __generic_gxx_recursive_mutex_init_function(__gthread_recursive_mutex_t *mutex) {	\
				NL_MTX_INIT();				/* Code specific for single or multiple mutexes		*/	\
				return;																				\
			}																						\
			int __generic_gxx_recursive_mutex_destroy(__gthread_recursive_mutex_t *mutex) {			\
				return(__generic_gxx_mutex_destroy((__gthread_mutex_t *)mutex));					\
			}																						\
			void __generic_gxx_mutex_init_function(__gthread_mutex_t *mutex) {						\
				__generic_gxx_recursive_mutex_init_function((__gthread_recursive_mutex_t *)mutex);	\
				return;																				\
			}																						\
			static int __my_gxx_lock(MTX_t **Mtx, int TimeOut) {									\
				int Ret = 1;				/* When can't lock mutex, report error				*/	\
				if (0 != COREreentOK()) {	/* Code specific for single or multiple mutexes		*/	\
					NL_MTX_CHK;				/* stdio mutxes are created before entering main()	*/	\
					if ((0 == OSisrInfo()) && ((TimeOut) != 0)) {									\
						MTXsafeLock(NL_MUTEX);	/* Here, ISRs are disabled and Timeout == -1	*/	\
						Ret = 0;			/* Must use the safe locking						*/	\
					}																				\
					else {					/* ISR on or Timeout == 0, regular lock				*/	\
						Ret = (0 != MTXlock(NL_MUTEX, (TimeOut)));									\
					}																				\
				}																					\
				return(Ret);																		\
			}																						\
			static int __my_gxx_unlock(MTX_t **Mtx) {												\
				if (0 != COREreentOK()) {															\
					MTXunlock(NL_MUTEX);	/* Code specific for single or multiple mutexes		*/	\
				}																					\
				return(0);																			\
			}																						\
			int __generic_gxx_mutex_lock(__gthread_mutex_t *mutex) {								\
				return(__my_gxx_lock((MTX_t **)mutex, -1));											\
			}																						\
			int __generic_gxx_mutex_trylock(__gthread_mutex_t *mutex) {								\
				return(__my_gxx_lock((MTX_t **)mutex, 0));											\
			}																						\
			int __generic_gxx_mutex_unlock(__gthread_mutex_t *mutex) {								\
				return(__my_gxx_unlock((MTX_t **)mutex));											\
			}																						\
			int __generic_gxx_recursive_mutex_lock(__gthread_recursive_mutex_t *mutex) {			\
				return(__my_gxx_lock((MTX_t **)mutex, -1));											\
			}																						\
			int __generic_gxx_recursive_mutex_trylock(__gthread_recursive_mutex_t *mutex) {			\
				return(__my_gxx_lock((MTX_t **)mutex, 0));											\
			}																						\
			int __generic_gxx_recursive_mutex_unlock(__gthread_recursive_mutex_t *mutex) {			\
				return(__my_gxx_unlock((MTX_t **)mutex));											\
			}																						\
			void __malloc_lock(struct _reent *reent) {												\
				if (COREreentOK() != 0) {															\
					MTXsafeLock(NL_MTX_ALLOC);														\
				}																					\
			}																						\
			void __malloc_unlock(struct _reent *reent) {											\
				if (COREreentOK() != 0) {															\
					MTXunlock(NL_MTX_ALLOC);														\
				}																					\
			}																						\
			void __env_lock(struct _reent *reent) {													\
				if (COREreentOK() != 0) {															\
					MTXsafeLock(NL_MTX_ENV);														\
				}																					\
			}																						\
			void __env_unlock(struct _reent *reent) {												\
				if (COREreentOK() != 0) {															\
					MTXunlock(NL_MTX_ENV);															\
				}																					\
			}																						\
			void __tz_lock(void) {																	\
				if (COREreentOK() != 0) {															\
					MTXsafeLock(NL_MTX_TZ);															\
				}																					\
			}																						\
			void __tz_unlock(void) {																\
				if (COREreentOK() != 0) {															\
					MTXunlock(NL_MTX_TZ);															\
				}																					\
			}																						\
			struct _reent *__getreent(void) {														\
				return((G_OSmtxOK == 0)																\
				      ? _impure_ptr																	\
				      : ((struct _reent *)(TSKmyID()->XtraData[0])));								\
			}
												/* For Newlib not built with __DYNAMIC_REENT__		*/
		#if (((OS_N_CORE) > 1) && ((OS_NEWLIB_REDEF) != 0))
		  #include <sys/config.h>
		  #ifndef __DYNAMIC_REENT__				/* Make sure is not built with __DYNAMIC_REENT__	*/
			#include <errno.h>					/* Must include them to make sure #undef works		*/
			#include <getopt.h>					/* and if included after mAbassi.h, will not undo	*/
			#include <iconv.h>					/* the defines done here							*/
			#include <malloc.h>					/* many were included at the top of the file		*/
			#include <signal.h>
			#include <time.h>
			#include <wchar.h>
			#include <sys/file.h>
			#include <sys/stat.h>
			#include <sys/unistd.h>

			#ifdef getc
			  #undef getc
			#endif
			#ifdef getchar
			  #undef getchar
			#endif
			#ifdef getchar_unlocked
			  #undef getchar_unlocked
			#endif
			#ifdef getwc
			  #undef getwc
			#endif
			#ifdef getwchar
			  #undef getwchar
			#endif
			#ifdef putc
			  #undef putc
			#endif
			#ifdef putchar
			  #undef putchar
			#endif
			#ifdef putchar_unlocked
			  #undef putchar_unlocked
			#endif
			#ifdef putwc
			  #undef putwc
			#endif
			#ifdef putwchar
			  #undef putwchar
			#endif
			#ifdef errno
			  #undef  errno
			#endif
			#define errno                  __errno_r (__getreent())
			#ifdef  stdin
			  #undef  stdin
			#endif
			#define stdin                  _stdin_r (__getreent())
			#ifdef  stdout
			  #undef  stdout
			#endif
			#define stdout                 _stdout_r(__getreent())
			#ifdef  stderr
			  #undef  stderr
			#endif
			#define stderr                 _stderr_r(__getreent())

			#define _getopt_long_only(...)  __getopt_long_only_r  (__getreent(), ## __VA_ARGS__)
			#define _getopt_long(...)       __getopt_long_r       (__getreent(), ## __VA_ARGS__)
			#define _getopt(...)            __getopt_r            (__getreent(), ## __VA_ARGS__)
			#define _srget(...)             __srget_r             (__getreent(), ## __VA_ARGS__)
			#define _swbuf(...)             __swbuf_r             (__getreent(), ## __VA_ARGS__)
			#define asiprintf(...)          _asiprintf_r          (__getreent(), ## __VA_ARGS__)
			#define asniprintf(...)         _asniprintf_r         (__getreent(), ## __VA_ARGS__)
			#define asnprintf(...)          _asnprintf_r          (__getreent(), ## __VA_ARGS__)
			#define asprintf(...)           _asprintf_r           (__getreent(), ## __VA_ARGS__)
			#define atoi(...)               _atoi_r               (__getreent(), ## __VA_ARGS__)
			#define atol(...)               _atol_r               (__getreent(), ## __VA_ARGS__)
			#define atoll(...)              _atoll_r              (__getreent(), ## __VA_ARGS__)
			#define calloc(...)             _calloc_r             (__getreent(), ## __VA_ARGS__)
			#define close(...)              _close_r              (__getreent(), ## __VA_ARGS__)
			#define diprintf(...)           _diprintf_r           (__getreent(), ## __VA_ARGS__)
			#define dprintf(...)            _dprintf_r            (__getreent(), ## __VA_ARGS__)
			#define drand48(...)            _drand48_r            (__getreent(), ## __VA_ARGS__)
			#define dtoa(...)               _dtoa_r               (__getreent(), ## __VA_ARGS__)
			#define erand48(...)            _erand48_r            (__getreent(), ## __VA_ARGS__)
			#define execve(...)             _execve_r             (__getreent(), ## __VA_ARGS__)
			#define fclose(...)             _fclose_r             (__getreent(), ## __VA_ARGS__)
			#define fcloseall(...)          _fcloseall_r          (__getreent(), ## __VA_ARGS__)
			#define fcntl(...)              _fcntl_r              (__getreent(), ## __VA_ARGS__)
			#define fdopen64(...)           _fdopen64_r           (__getreent(), ## __VA_ARGS__)
			#define fdopen(...)             _fdopen_r             (__getreent(), ## __VA_ARGS__)
			#define fflush(...)             _fflush_r             (__getreent(), ## __VA_ARGS__)
			#define fgetc(...)              _fgetc_r              (__getreent(), ## __VA_ARGS__)
			#define fgetpos64(...)          _fgetpos64_r          (__getreent(), ## __VA_ARGS__)
			#define fgetpos(...)            _fgetpos_r            (__getreent(), ## __VA_ARGS__)
			#define fgets(...)              _fgets_r              (__getreent(), ## __VA_ARGS__)
			#define fgetwc(...)             _fgetwc_r             (__getreent(), ## __VA_ARGS__)
			#define fgetws(...)             _fgetws_r             (__getreent(), ## __VA_ARGS__)
			#define _findenv(...)           _findenv_r            (__getreent(), ## __VA_ARGS__)
			#define fiprintf(...)           _fiprintf_r           (__getreent(), ## __VA_ARGS__)
			#define fiscanf(...)            _fiscanf_r            (__getreent(), ## __VA_ARGS__)
			#define fmemopen(...)           _fmemopen_r           (__getreent(), ## __VA_ARGS__)
			#define fopen64(...)            _fopen64_r            (__getreent(), ## __VA_ARGS__)
			#define fopen(...)              _fopen_r              (__getreent(), ## __VA_ARGS__)
			#define fopencookie(...)        _fopencookie_r        (__getreent(), ## __VA_ARGS__)
			#define fork(...)               _fork_r               (__getreent(), ## __VA_ARGS__)
			#define fprintf(...)            _fprintf_r            (__getreent(), ## __VA_ARGS__)
			#define fpurge(...)             _fpurge_r             (__getreent(), ## __VA_ARGS__)
			#define fputc(...)              _fputc_r              (__getreent(), ## __VA_ARGS__)
			#define fputs(...)              _fputs_r              (__getreent(), ## __VA_ARGS__)
			#define fputwc(...)             _fputwc_r             (__getreent(), ## __VA_ARGS__)
			#define fputws(...)             _fputws_r             (__getreent(), ## __VA_ARGS__)
			#define fread(...)              _fread_r              (__getreent(), ## __VA_ARGS__)
			#define free(...)               _free_r               (__getreent(), ## __VA_ARGS__)
			#define freopen64(...)          _freopen64_r          (__getreent(), ## __VA_ARGS__)
			#define freopen(...)            _freopen_r            (__getreent(), ## __VA_ARGS__)
			#define fscanf(...)             _fscanf_r             (__getreent(), ## __VA_ARGS__)
			#define fseek(...)              _fseek_r              (__getreent(), ## __VA_ARGS__)
			#define fseeko64(...)           _fseeko64_r           (__getreent(), ## __VA_ARGS__)
			#define fseeko(...)             _fseeko_r             (__getreent(), ## __VA_ARGS__)
			#define fsetpos64(...)          _fsetpos64_r          (__getreent(), ## __VA_ARGS__)
			#define fsetpos(...)            _fsetpos_r            (__getreent(), ## __VA_ARGS__)
			#define fstat(...)              _fstat_r              (__getreent(), ## __VA_ARGS__)
			#define fstat64(...)            _fstat64_r            (__getreent(), ## __VA_ARGS__)
			#define ftell(...)              _ftell_r              (__getreent(), ## __VA_ARGS__)
			#define ftello64(...)           _ftello64_r           (__getreent(), ## __VA_ARGS__)
			#define ftello(...)             _ftello_r             (__getreent(), ## __VA_ARGS__)
			#define funopen(...)            _funopen_r            (__getreent(), ## __VA_ARGS__)
			#define fwide(...)              _fwide_r              (__getreent(), ## __VA_ARGS__)
			#define fwprintf(...)           _fwprintf_r           (__getreent(), ## __VA_ARGS__)
			#define fwrite(...)             _fwrite_r             (__getreent(), ## __VA_ARGS__)
			#define fwscanf(...)            _fwscanf_r            (__getreent(), ## __VA_ARGS__)
			#define getc(...)               _getc_r               (__getreent(), ## __VA_ARGS__)
			#define getc_unlocked(...)      _getc_unlocked_r      (__getreent(), ## __VA_ARGS__)
			#define getchar(...)            _getchar_r            (__getreent(), ## __VA_ARGS__)
			#define getchar_unlocked(...)   _getchar_unlocked_r   (__getreent(), ## __VA_ARGS__)
			#define getenv(...)             _getenv_r             (__getreent(), ## __VA_ARGS__)
			#define getpid(...)             _getpid_r             (__getreent(), ## __VA_ARGS__)
			#define gets(...)               _gets_r               (__getreent(), ## __VA_ARGS__)
			#define getwc(...)              _getwc_r              (__getreent(), ## __VA_ARGS__)
			#define getwchar(...)           _getwchar_r           (__getreent(), ## __VA_ARGS__)
			#define iconv_close(...)        _iconv_close_r        (__getreent(), ## __VA_ARGS__)
			#define iconv_open(...)         _iconv_open_r         (__getreent(), ## __VA_ARGS__)
			#define iconv(...)              _iconv_r              (__getreent(), ## __VA_ARGS__)
			#define iprintf(...)            _iprintf_r            (__getreent(), ## __VA_ARGS__)
			#define iscanf(...)             _iscanf_r             (__getreent(), ## __VA_ARGS__)
			#define isatty(...)             _isatty_r             (__getreent(), ## __VA_ARGS__)
			#define jrand48(...)            _jrand48_r            (__getreent(), ## __VA_ARGS__)
			#define kill(...)               _kill_r               (__getreent(), ## __VA_ARGS__)
			#define l64a(...)               _l64a_r               (__getreent(), ## __VA_ARGS__)
			#define lcong48(...)            _lcong48_r            (__getreent(), ## __VA_ARGS__)
			#define link(...)               _link_r               (__getreent(), ## __VA_ARGS__)
			#define localeconv(...)         _localeconv_r         (__getreent(), ## __VA_ARGS__)
			#define lrand48(...)            _lrand48_r            (__getreent(), ## __VA_ARGS__)
			#define lseek(...)              _lseek_r              (__getreent(), ## __VA_ARGS__)
			#define lseek64(...)            _lseek64_r            (__getreent(), ## __VA_ARGS__)
			#define malloc(...)             _malloc_r             (__getreent(), ## __VA_ARGS__)
			#define malloc_usable_size(...) _malloc_usable_size_r (__getreent(), ## __VA_ARGS__)
			#define malloc_stats(...)       _malloc_stats_r       (__getreent(), ## __VA_ARGS__)
			#define malloc_trim(...)        _malloc_trim_r        (__getreent(), ## __VA_ARGS__)
			#define mallopt(...)            _mallopt_r            (__getreent(), ## __VA_ARGS__)
			#define mblen(...)              _mblen_r              (__getreent(), ## __VA_ARGS__)
			#define mbrtowc(...)            _mbrtowc_r            (__getreent(), ## __VA_ARGS__)
			#define mbsnrtowcs(...)         _mbsnrtowcs_r         (__getreent(), ## __VA_ARGS__)
			#define mbsrtowcs(...)          _mbsrtowcs_r          (__getreent(), ## __VA_ARGS__)
			#define mbstowcs(...)           _mbstowcs_r           (__getreent(), ## __VA_ARGS__)
			#define mbtowc(...)             _mbtowc_r             (__getreent(), ## __VA_ARGS__)
			#define memalign(...)           _memalign_r           (__getreent(), ## __VA_ARGS__)
			#define mkdir(...)              _mkdir_r              (__getreent(), ## __VA_ARGS__)
			#define mkdtemp(...)            _mkdtemp_r            (__getreent(), ## __VA_ARGS__)
			#define mkostemp(...)           _mkostemp_r           (__getreent(), ## __VA_ARGS__)
			#define mkostemps(...)          _mkostemps_r          (__getreent(), ## __VA_ARGS__)
			#define mkstemp(...)            _mkstemp_r            (__getreent(), ## __VA_ARGS__)
			#define mkstemps(...)           _mkstemps_r           (__getreent(), ## __VA_ARGS__)
			#define mktemp(...)             _mktemp_r             (__getreent(), ## __VA_ARGS__)
			#define mrand48(...)            _mrand48_r            (__getreent(), ## __VA_ARGS__)
			#define mstats(...)             _mstats_r             (__getreent(), ## __VA_ARGS__)
			#define nrand48(...)            _nrand48_r            (__getreent(), ## __VA_ARGS__)
			#define open(...)               _open_r               (__getreent(), ## __VA_ARGS__)
			#define open64(...)             _open64_r             (__getreent(), ## __VA_ARGS__)
			#define open_memstream(...)     _open_memstream_r     (__getreent(), ## __VA_ARGS__)
			#define open_wmemstream(...)    _open_wmemstream_r    (__getreent(), ## __VA_ARGS__)
			#define perror(...)             _perror_r             (__getreent(), ## __VA_ARGS__)
			#define printf(...)             _printf_r             (__getreent(), ## __VA_ARGS__)
			#define putc(...)               _putc_r               (__getreent(), ## __VA_ARGS__)
			#define putc_unlocked(...)      _putc_unlocked_r      (__getreent(), ## __VA_ARGS__)
			#define putchar(...)            _putchar_r            (__getreent(), ## __VA_ARGS__)
			#define putchar_unlocked(...)   _putchar_unlocked_r   (__getreent(), ## __VA_ARGS__)
			#define putenv(...)             _putenv_r             (__getreent(), ## __VA_ARGS__)
			#define puts(...)               _puts_r               (__getreent(), ## __VA_ARGS__)
			#define putwc(...)              _putwc_r              (__getreent(), ## __VA_ARGS__)
			#define putwchar(...)           _putwchar_r           (__getreent(), ## __VA_ARGS__)
			#define pvalloc(...)            _pvalloc_r            (__getreent(), ## __VA_ARGS__)
			#define raise(...)              _raise_r              (__getreent(), ## __VA_ARGS__)
			#define read(...)               _read_r               (__getreent(), ## __VA_ARGS__)
			#define realloc(...)            _realloc_r            (__getreent(), ## __VA_ARGS__)
			#define reallocf(...)           _reallocf_r           (__getreent(), ## __VA_ARGS__)
			#define remove(...)             _remove_r             (__getreent(), ## __VA_ARGS__)
			#define rename(...)             _rename_r             (__getreent(), ## __VA_ARGS__)
			#define rewind(...)             _rewind_r             (__getreent(), ## __VA_ARGS__)
			#define scanf(...)              _scanf_r              (__getreent(), ## __VA_ARGS__)
			#define sbrk(...)               _sbrk_r               (__getreent(), ## __VA_ARGS__)
			#define seed48(...)             _seed48_r             (__getreent(), ## __VA_ARGS__)
			#define setenv(...)             _setenv_r             (__getreent(), ## __VA_ARGS__)
			#define setlocale(...)          _setlocale_r          (__getreent(), ## __VA_ARGS__)
			#define signal(...)             _signal_r             (__getreent(), ## __VA_ARGS__)
			#define siprintf(...)           _siprintf_r           (__getreent(), ## __VA_ARGS__)
			#define siscanf(...)            _siscanf_r            (__getreent(), ## __VA_ARGS__)
			#define sniprintf(...)          _sniprintf_r          (__getreent(), ## __VA_ARGS__)
			#define snprintf(...)           _snprintf_r           (__getreent(), ## __VA_ARGS__)
			#define sprintf(...)            _sprintf_r            (__getreent(), ## __VA_ARGS__)
			#define srand48(...)            _srand48_r            (__getreent(), ## __VA_ARGS__)
			#define sscanf(...)             _sscanf_r             (__getreent(), ## __VA_ARGS__)
			#define stat(...)               _stat_r               (__getreent(), ## __VA_ARGS__)
			#define stat64(...)             _stat64_r             (__getreent(), ## __VA_ARGS__)
			#define strdup(...)             _strdup_r             (__getreent(), ## __VA_ARGS__)
			#define strndup(...)            _strndup_r            (__getreent(), ## __VA_ARGS__)
			#define strtod(...)             _strtod_r             (__getreent(), ## __VA_ARGS__)
			#define strtol(...)             _strtol_r             (__getreent(), ## __VA_ARGS__)
			#define strtoll(...)            _strtoll_r            (__getreent(), ## __VA_ARGS__)
			#define strtoul(...)            _strtoul_r            (__getreent(), ## __VA_ARGS__)
			#define strtoull(...)           _strtoull_r           (__getreent(), ## __VA_ARGS__)
			#define swprintf(...)           _swprintf_r           (__getreent(), ## __VA_ARGS__)
			#define swscanf(...)            _swscanf_r            (__getreent(), ## __VA_ARGS__)
			#define system(...)             _system_r             (__getreent(), ## __VA_ARGS__)
			#define tempnam(...)            _tempnam_r            (__getreent(), ## __VA_ARGS__)
			#define times(...)              _times_r              (__getreent(), ## __VA_ARGS__)
			#define tmpfile64(...)          _tmpfile64_r          (__getreent(), ## __VA_ARGS__)
			#define tmpfile(...)            _tmpfile_r            (__getreent(), ## __VA_ARGS__)
			#define tmpnam(...)             _tmpnam_r             (__getreent(), ## __VA_ARGS__)
			#define tzset(...)              _tzset_r              (__getreent(), ## __VA_ARGS__)
			#define ungetc(...)             _ungetc_r             (__getreent(), ## __VA_ARGS__)
			#define ungetwc(...)            _ungetwc_r            (__getreent(), ## __VA_ARGS__)
			#define unlink(...)             _unlink_r             (__getreent(), ## __VA_ARGS__)
			#define unsetenv(...)           _unsetenv_r           (__getreent(), ## __VA_ARGS__)
			#define unsetenv(...)           _unsetenv_r           (__getreent(), ## __VA_ARGS__)
			#define valloc(...)             _valloc_r             (__getreent(), ## __VA_ARGS__)
			#define vasniprintf(...)        _vasniprintf_r        (__getreent(), ## __VA_ARGS__)
			#define vasnprintf(...)         _vasnprintf_r         (__getreent(), ## __VA_ARGS__)
			#define vasprintf(...)          _vasprintf_r          (__getreent(), ## __VA_ARGS__)
			#define vdiprintf(...)          _vdiprintf_r          (__getreent(), ## __VA_ARGS__)
			#define vdprintf(...)           _vdprintf_r           (__getreent(), ## __VA_ARGS__)
			#define vfiprintf(...)          _vfiprintf_r          (__getreent(), ## __VA_ARGS__)
			#define vfiscanf(...)           _vfiscanf_r           (__getreent(), ## __VA_ARGS__)
			#define vfprintf(...)           _vfprintf_r           (__getreent(), ## __VA_ARGS__)
			#define vfscanf(...)            _vfscanf_r            (__getreent(), ## __VA_ARGS__)
			#define vfwprintf(...)          _vfwprintf_r          (__getreent(), ## __VA_ARGS__)
			#define vfwscanf(...)           _vfwscanf_r           (__getreent(), ## __VA_ARGS__)
			#define viprintf(...)           _viprintf_r           (__getreent(), ## __VA_ARGS__)
			#define viscanf(...)            _viscanf_r            (__getreent(), ## __VA_ARGS__)
			#define vprintf(...)            _vprintf_r            (__getreent(), ## __VA_ARGS__)
			#define vscanf(...)             _vscanf_r             (__getreent(), ## __VA_ARGS__)
			#define vsiprintf(...)          _vsiprintf_r          (__getreent(), ## __VA_ARGS__)
			#define vsiscanf(...)           _vsiscanf_r           (__getreent(), ## __VA_ARGS__)
			#define vsniprintf(...)         _vsniprintf_r         (__getreent(), ## __VA_ARGS__)
			#define vsnprintf(...)          _vsnprintf_r          (__getreent(), ## __VA_ARGS__)
			#define vsprintf(...)           _vsprintf_r           (__getreent(), ## __VA_ARGS__)
			#define vsscanf(...)            _vsscanf_r            (__getreent(), ## __VA_ARGS__)
			#define vswprintf(...)          _vswprintf_r          (__getreent(), ## __VA_ARGS__)
			#define vswscanf(...)           _vswscanf_r           (__getreent(), ## __VA_ARGS__)
			#define vwprintf(...)           _vwprintf_r           (__getreent(), ## __VA_ARGS__)
			#define vwscanf(...)            _vwscanf_r            (__getreent(), ## __VA_ARGS__)
			#define wait(...)               _wait_r               (__getreent(), ## __VA_ARGS__)
			#define wcrtomb(...)            _wcrtomb_r            (__getreent(), ## __VA_ARGS__)
			#define wcsdup(...)             _wcsdup_r             (__getreent(), ## __VA_ARGS__)
			#define wcsnrtombs(...)         _wcsnrtombs_r         (__getreent(), ## __VA_ARGS__)
			#define wcsrtombs(...)          _wcsrtombs_r          (__getreent(), ## __VA_ARGS__)
			#define wcstod(...)             _wcstod_r             (__getreent(), ## __VA_ARGS__)
			#define wcstof(...)             _wcstof_r             (__getreent(), ## __VA_ARGS__)
			#define wcstol(...)             _wcstol_r             (__getreent(), ## __VA_ARGS__)
			#define wcstoll(...)            _wcstoll_r            (__getreent(), ## __VA_ARGS__)
			#define wcstombs(...)           _wcstombs_r           (__getreent(), ## __VA_ARGS__)
			#define wcstoul(...)            _wcstoul_r            (__getreent(), ## __VA_ARGS__)
			#define wcstoull(...)           _wcstoull_r           (__getreent(), ## __VA_ARGS__)
			#define wctomb(...)             _wctomb_r             (__getreent(), ## __VA_ARGS__)
			#define wprintf(...)            _wprintf_r            (__getreent(), ## __VA_ARGS__)
			#define write(...)              _write_r              (__getreent(), ## __VA_ARGS__)
			#define wscanf(...)             _wscanf_r             (__getreent(), ## __VA_ARGS__)
			#define asctime(...)            asctime_r             (__getreent(), ## __VA_ARGS__)
			#define ctime(...)              ctime_r               (__getreent(), ## __VA_ARGS__)
			#define getdate(...)            getdate_r             (__getreent(), ## __VA_ARGS__)
			#define getlogin(...)           getlogin_r            (__getreent(), ## __VA_ARGS__)
			#define gmtime(...)             gmtime_r              (__getreent(), ## __VA_ARGS__)
			#define rand(...)               rand_r                (__getreent(), ## __VA_ARGS__)
			#define strerror(...)           strerror_r            (__getreent(), ## __VA_ARGS__)
			#define strtok(...)             strtok_r              (__getreent(), ## __VA_ARGS__)
			#define ttyname(...)            ttyname_r             (__getreent(), ## __VA_ARGS__)
		  #endif
		#endif

	#else										/* GCC: internal error without this do nothing		*/
		#define OX_TASK_XTRA(_TskDsc, _Fct)		/* TC007B-hook #751 -O3 */							\
			OX_DO_NOTHING()
	#endif

/* ------------------------------------------------------------------------------------------------ */
/* IAR - LIBRARY RE-ENTRANCE PROTECTION																*/
/* ------------------------------------------------------------------------------------------------ */

#elif defined(__IAR_SYSTEMS_ICC__)

	#ifndef OS_IAR_MTHREAD
		#define OS_IAR_MTHREAD	0
	#endif

  	#if ((OS_IAR_MTHREAD) != 0)					/* Library multi-threading protection				*/
		#include <yvals.h>
		#include <DLib_Threads.h>

		#define OX_LIB_REENT_PROTECT	(OS_IAR_MTHREAD)
		#define OX_SEM_PARKING			1		/* Needed due to alloc / free of mutexes			*/
		#ifdef OX_TASK_XTRA_FIELD_1
			#error "With library re-entrance protection enabled, 1st entry in XTRA_FIELD is"
			#error "reserved.  Use a higher OX_TASK_XTRA_FIELD_n"
		#else
			#define OX_TASK_XTRA_FIELD_1	1
		#endif

		#if (defined(__ABASSI_C__) || defined(__MABASSI_C__) || defined(__UABASSI_C__))
			int G_OSmtxOK;						/* No need to set to zero as BSS is zeroed			*/
		#else
			extern int G_OSmtxOK;
		#endif

		#define OX_DEVICE_PREINIT()																	\
			do {																					\
				G_OSmtxOK = 1;																		\
				OX_INT_CTRL_INIT();																	\
			} while(0)
												/* Multi-thread protect changed in version 8.11.3	*/
		#if (defined(__ARM_PROFILE_M__) && (__VER__ >= 8011001))
			#if defined(__ARM_PROFILE_M__)		/* Provision when more target processors changed	*/
				#define OS_IAR_REENT_ACCESS_FCT		__aeabi_read_tp
				#define OS_IAR_REENT_SECTION		"__iar_tls$$DATA"
			#else
				#error "IAR Reentrance - must define the access function"
			#endif

			#if (defined(__ABASSI_C__) || defined(__MABASSI_C__) || defined(__UABASSI_C__))
				#pragma section=OS_IAR_REENT_SECTION
			#endif

			#define OX_XTRA_FCT																		\
				void * OS_IAR_REENT_ACCESS_FCT(void)												\
				{																					\
				void *MTptr;																		\
					MTptr = (void *)__section_begin(OS_IAR_REENT_SECTION);							\
					if (TSKmyID() != (TSK_t *)NULL) {												\
						if (TSKmyID()->XtraData[0] != (intptr_t)0) {								\
						    MTptr = (void *)TSKmyID()->XtraData[0];									\
						}																			\
					}																				\
					return(MTptr);																	\
				}

			#if ((OS_IAR_MTHREAD) > 0)			/* Multi-thread protection on all tasks				*/
				#if (defined(__ABASSI_C__) || defined(__MABASSI_C__) || defined(__UABASSI_C__))
					#define OX_TASK_XTRA(_TskDsc,_Fct) 												\
						do {																		\
							if ((_Fct) != NULL) {													\
							    void *MTptr;														\
								MTptr = OSalloc(__iar_tls_size());									\
								__iar_tls_init(MTptr);												\
								(_TskDsc)->XtraData[0] = (intptr_t)MTptr;							\
							}																		\
						} while(0)
				#else								/* When used outside of Abassi.c by TSK_SETUP()	*/
			  		#define OX_TASK_XTRA(_TskDsc, _Fct)	/* there is no need to check for A&E	*/	\
						do {																		\
					    	void *MTptr;															\
							MTptr = OSalloc(__iar_tls_size());										\
							__iar_tls_init(MTptr);													\
							(_TskDsc)->XtraData[0] = (intptr_t)MTptr;								\
						} while(0)
				#endif

				#define TSKmultThrd(_TskDsc)	OX_DO_NOTHING()

			#else
				#define TSKmultThrd(_TskDsc)														\
					do {																			\
						void *MTptr;																\
						MTptr = OSalloc(__iar_tls_size());											\
						__iar_tls_init(MTptr);														\
						(_TskDsc)->XtraData[0] = (intptr_t)MTptr;									\
					} while(0)

			#endif
												/* ARM pre-version 8.11.1 or non-ARM				*/
		#elif ((_DLIB_THREAD_SUPPORT) < 2)		/* ==2 is mutexes only, ==3 is mutexes + TLS		*/
			#error "Multi-thread protection is not supported by the current DLib"
												/* When 0 & 1, there are no multi-thread support	*/
		#else
			#if ((_DLIB_THREAD_SUPPORT) == 3)	/* TLS are supported								*/
				#define OX_XTRA_FCT																	\
					void _DLIB_TLS_MEMORY *__iar_dlib_perthread_access(void _DLIB_TLS_MEMORY *Symb)	\
					{																				\
					char _DLIB_TLS_MEMORY *MTptr;													\
						MTptr = (char _DLIB_TLS_MEMORY *)__segment_begin("__DLIB_PERTHREAD");		\
						if (TSKmyID() != (TSK_t *)NULL) {											\
							if (TSKmyID()->XtraData[0] != (intptr_t)0) {							\
							    MTptr = (void *)TSKmyID()->XtraData[0];								\
							}																		\
						}																			\
						MTptr += __IAR_DLIB_PERTHREAD_SYMBOL_OFFSET(Symb);							\
						return((void _DLIB_TLS_MEMORY *)MTptr);										\
					}

				#if ((OS_IAR_MTHREAD) > 0)		/* Multi-thread protection on all tasks				*/
					#if (defined(__ABASSI_C__) || defined(__MABASSI_C__) || defined(__UABASSI_C__))
						#define OX_TASK_XTRA(_TskDsc,_Fct) 											\
							do {																	\
								if ((_Fct) != NULL) {												\
								    void *MTptr;													\
									MTptr = OSalloc(__IAR_DLIB_PERTHREAD_SIZE);						\
									__iar_dlib_perthread_initialize(MTptr);							\
									(_TskDsc)->XtraData[0] = (intptr_t)MTptr;						\
								}																	\
							} while(0)
					#else							/* When used outside of Abassi.c by TSK_SETUP()	*/
				  		#define OX_TASK_XTRA(_TskDsc, _Fct)	/* there is no need to check for A&E*/	\
							do {																	\
						    	void *MTptr;														\
								MTptr = OSalloc(__IAR_DLIB_PERTHREAD_SIZE);							\
								__iar_dlib_perthread_initialize(MTptr);								\
								(_TskDsc)->XtraData[0] = (intptr_t)MTptr;							\
							} while(0)
					#endif

					#define TSKmultThrd(_TskDsc)	OX_DO_NOTHING()

				#else
					#define TSKmultThrd(_TskDsc)													\
						do {																		\
							void *MTptr;															\
							MTptr = OSalloc(__IAR_DLIB_PERTHREAD_SIZE);								\
							__iar_dlib_perthread_initialize(MTptr);									\
							(_TskDsc)->XtraData[0] = (intptr_t)MTptr;								\
						} while(0)
				#endif
			#endif
		#endif
	#endif

/* ------------------------------------------------------------------------------------------------ */
/* END OF LIBRARY RE-ENTRANCE PROTECTION															*/
/* ------------------------------------------------------------------------------------------------ */

#endif

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* CMSIS EXTRA SET-UP																				*/

#ifdef OS_CMSIS_RTOS							/* Special stuff for CMSIS V3.0						*/

	#ifndef OX_DEVICE_INIT_XTRA
		#define OX_DEVICE_INIT_XTRA()	OX_DO_NOTHING()
	#endif

	#if (defined(__ABASSI_C__) || defined(__MABASSI_C__) || defined(__UABASSI_C__))
		int G_OSstarted = 0;					/* CMSIS needs to know if kernel started			*/
	#else
		extern int G_OSstarted;
	#endif

	#define OX_DEVICE_INIT() 																		\
		do {																						\
			OX_DEVICE_INIT_XTRA();																	\
			G_OSstarted = 1;																		\
			TSKsetPrio(TSKmyID(), 3);																\
		} while(0)
#endif

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* LEFT OVER WHEN LIBRARY REENTRANCE PROTECTION AND/OR CMSIS NOT USED								*/

#if !defined(OX_DEVICE_PREINIT)
	#define OX_DEVICE_PREINIT()																		\
		do {																						\
			OX_INT_CTRL_INIT();																		\
		} while(0)
#endif

#ifndef OS_CMSIS_RTOS							/* CMSIS deals with its own OX_DEVICE_INIT()		*/
  #if defined(OX_DEVICE_INIT_XTRA)
	#define OX_DEVICE_INIT()	OX_DEVICE_INIT_XTRA()
  #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Place-holder on how to add internal task extra fields											*/
/* ------------------------------------------------------------------------------------------------ */

#if 0
#ifdef OS_NEW_FEATURE
  #define OX_NEW_FEATURE		(OS_NEW_FEATURE)
#else
  #define OX_NEW_FEATURE		0
#endif

#if ((OX_NEW_FEATURE) != 0)
  #ifndef OX_TASK_XTRA_FIELD_1
	#define OX_TASK_XTRA_FIELD_1	1
	#defien OS_NEW_FEATURE_FIELD	0
  #elif !defined(OX_TASK_XTRA_FIELD_2)
	#define OX_TASK_XTRA_FIELD_2	(1+(OX_TASK_XTRA_FIELD_1))
	#define OS_NEW_FEATURE_FIELD	(OX_TASK_XTRA_FIELD_1)
  #elif !defined(OX_TASK_XTRA_FIELD_3)
	#define OX_TASK_XTRA_FIELD_3	(1+(OX_TASK_XTRA_FIELD_2))
	#define OS_NEW_FEATURE_FIELD	(OX_TASK_XTRA_FIELD_2)
  #elif !defined(OX_TASK_XTRA_FIELD_4)
	#define OX_TASK_XTRA_FIELD_4	(1+(OX_TASK_XTRA_FIELD_3))
	#define OS_NEW_FEATURE_FIELD	(OX_TASK_XTRA_FIELD_3)
  #elif !defined(OX_TASK_XTRA_FIELD_5)
	#define OX_TASK_XTRA_FIELD_5	(1+(OX_TASK_XTRA_FIELD_4)
	#define OS_NEW_FEATURE_FIELD	(OX_TASK_XTRA_FIELD_4)
  #else
	#error "Add more OX_TASK_XTRA_FIELD_N"
  #endif
#endif
#endif
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* DO NOT MODIFY ANYTHING BELOW THIS LINE UNLESS YOU ARE SURE YOU KNOW WHAT YOU ARE DOING			*/
/* DO NOT MODIFY ANYTHING BELOW THIS LINE UNLESS YOU ARE SURE YOU KNOW WHAT YOU ARE DOING			*/
/* DO NOT MODIFY ANYTHING BELOW THIS LINE UNLESS YOU ARE SURE YOU KNOW WHAT YOU ARE DOING			*/
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------ */
/* Build options validation																			*/
/* ------------------------------------------------------------------------------------------------ */

#ifdef OS_N_CORE								/* Do these check to simplifies user's I/F			*/
  #if ((OS_N_CORE) == 1)
	#ifndef OS_MP_TYPE
	  #define OS_MP_TYPE		0U
	#endif
	#ifndef OS_START_STACK
	  #define OS_START_STACK	0
	#endif
  #endif
#endif

#ifndef OS_ALLOC_SIZE
  #error "Abassi: The build option OS_ALLOC_SIZE hasn't been defined"
#elif ((OS_ALLOC_SIZE) < 0)
  #error "Abassi: The build option OS_ALLOC_SIZE has a negative value"  
  #error "        >= 0 as defines the number bytes allocated for the OS memory pool"
  #error "        == 0 if malloc() is preferred from OSalloc()"
#endif
#ifndef OS_COOPERATIVE
  #error "Abassi: The build option OS_COOPERATIVE hasn't been defined"
#endif
#ifndef OS_EVENTS
  #error "Abassi: The build option OS_EVENTS hasn't been defined"
#endif
#ifndef OS_FCFS
  #error "Abassi: The build option OS_FCFS hasn't been defined"
#endif
#ifdef OS_IDLE_STACK
 #if ((OS_IDLE_STACK) < 0)
  #error "Abassi: The build option OS_IDLE_STACK has a negative value"
  #error "        >= 0 defines the number bytes allocated for the Idle Task stack"
  #error "        == 0 the application supplied its own Idle Task"
 #endif
#endif
#ifndef OS_LOGGING_TYPE
  #error "Abassi: The build option OS_LOGGING_TYPE hasn't been defined"
#elif ((OS_LOGGING_TYPE) < 0)
  #error "Abassi: The build option OS_LOGGING_TYPE is negative"
  #error "       = 0 disable logging"
  #error "       = 1 specifies to perform direct formatted print to OSputChar()"
  #error "       > 1 specifies the buffer size (in # of messages) to accumulate past logging messages"
#endif
#ifndef OS_MAILBOX
  #error "Abassi: The build option OS_MAILBOX hasn't been defined"
#endif
#ifndef OS_MAX_PEND_RQST
  #error "Abassi: The build option OS_MAX_PEND_RQST hasn't been defined"
#elif ((OS_MAX_PEND_RQST) < 2U)
  #error "Abassi: The build option OS_MAX_PEND_RQST must be > 1"
  #error "        It defines the size of the queue to absorb kernel requests during interrupts"
#endif
#ifndef OS_MIN_STACK_USE
  #error "Abassi: The build option OS_MIN_STACK_USE hasn't been defined"
#endif
#ifndef OS_MP_TYPE
  #error "Abassi: The build option OS_MP_TYPE hasn't been defined"
#elif ((OS_MP_TYPE) > 5U)
  #error "Abassi: Invalid build option OS_MP_TYPE value (must be between 0 and 5)"
#endif
#ifndef OS_MTX_DEADLOCK
  #error "Abassi: The build option OS_MTX_DEADLOCK hasn't been defined"
#endif
#ifndef OS_MTX_INVERSION
  #error "Abassi: The build option OS_MTX_INVERSION hasn't been defined"
#endif
#ifndef OS_N_CORE
  #error "Abassi: The build option OS_N_CORE hasn't been defined"
#elif ((OS_N_CORE) < 1)
  #error "Abassi: The build option OS_N_CORE must be >= 1"
#endif
#ifndef OS_NAMES
  #error "Abassi: The build option OS_NAMES hasn't been defined"
#endif
#ifndef OS_PRIO_CHANGE
  #error "Abassi: The build option OS_PRIO_CHANGE hasn't been defined"
#endif
#ifndef OS_PRIO_MIN
  #error "Abassi: The build option OS_PRIO_MIN hasn't been defined"
#elif ((OS_PRIO_MIN) < 0)
  #error "Abassi: The build option OS_PRIO_MIN is negative"
  #error "        The highest priority value is 0"
  #error "        The lowest priority must have a value > 0"
#endif
#ifndef OS_PRIO_SAME
  #error "Abassi: The build option OS_PRIO_SAME hasn't been defined"
#endif
#ifndef OS_ROUND_ROBIN
  #error "Abassi: The build option OS_ROUND_ROBIN hasn't been defined"
#endif
#ifndef OS_RUNTIME
  #error "Abassi: The build option OS_RUNTIME hasn't been defined"
#endif
#ifndef OS_SEARCH_ALGO
  #error "Abassi: The build option OS_SEARCH_ALGO hasn't been defined"
#endif
#ifndef OS_START_STACK
  #error "Abassi: The build option OS_START_STACK hasn't been defined"
#elif ((OS_START_STACK) < 0)
  #error "Abassi: The build option OS_START_STACK is negative"
  #error "        Must be >= 0 as it defines the stack for each Core Idle tasks"
  #error "        Can be set to zero when OS_N_CORE == 1"
#endif
#ifndef OS_STARVE_PRIO
  #error "Abassi: The build option OS_STARVE_PRIO hasn't been defined"
#endif
#ifndef OS_STARVE_RUN_MAX
  #error "Abassi: The build option OS_STARVE_RUN_MAX hasn't been defined"
#endif
#ifndef OS_STARVE_WAIT_MAX
  #error "Abassi: The build option OS_STARVE_WAIT_MAX hasn't been defined"
#endif
#ifdef OS_STATIC_BUF_MBLK
 #if ((OS_STATIC_BUF_MBLK) < 0)
  #error "Abassi: The build option OS_STATIC_BUF_MBLK is negative"
  #error "        Must be >= 0 as it defines the buffer memory to allocate"
  #error "        If set to 0, memory is obtained through OSalloc()"
 #endif
#endif
#ifdef OS_STATIC_MBLK
 #if ((OS_STATIC_MBLK) < 0)
  #error "Abassi: The build option OS_STATIC_MBLK is negative"
  #error "        Must be >= 0 as it defines the number memory block descriptors to allocate"
  #error "        If set to 0, memory is obtained through OSalloc()"
 #endif
#endif
#ifndef OS_STATIC_BUF_MBX
  #error "Abassi: The build option OS_STATIC_BUF_MBX hasn't been defined"
#elif ((OS_STATIC_BUF_MBX) < 0)
  #error "Abassi: The build option OS_STATIC_BUF_MBX is negative"
  #error "        This defines the number of mailbox elements to allocate"
  #error "        It must be greater or equal to the sum of all mailbox buffer size"
  #error "        If set to 0, memory is obtained through OSalloc()"
#endif
#ifndef OS_STATIC_MBX
  #error "Abassi: The build option OS_STATIC_MBX hasn't been defined"
#elif ((OS_STATIC_MBX) < 0)
  #error "Abassi: The build option OS_STATIC_MBX is negative"
  #error "        Must be >= 0 as it defines the number mailbox descriptors to allocate"
  #error "        If set to 0, memory is obtained through OSalloc()"
#endif
#if ((OS_STATIC_MBX) == 0) && ((OS_STATIC_BUF_MBX) > 0)
  #error "Abassi: The build option OS_STATIC_BUF_MBX is non-zero when OS_STATIC_MBX` is zero"
  #error "        OS_STATIC_BUF_MBX takes into account the number of mailboxes in the application"
  #error "        to allocate the correct amount of memory"
  #error "        Set OS_STATIC_MBX to the number of mailboxes in the application"
#endif
#ifndef OS_STATIC_NAME
  #error "Abassi: The build option OS_STATIC_NAME hasn't been defined"
#elif ((OS_STATIC_NAME) < 0)
  #error "Abassi: The build option OS_STATIC_NAME is negative"
  #error "        Must be >= 0 as it defines the number of bytes allocated for the names"
  #error "        If set to 0, memory is obtained through OSalloc()"
#endif
#ifndef OS_STATIC_SEM
  #error "Abassi: The build option OS_STATIC_SEM hasn't been defined"
#elif ((OS_STATIC_SEM) < 0)
  #error "Abassi: The build option OS_STATIC_SEM is negative"
  #error "        Must be >= 0 as it defines the number mailbox descriptors to allocate"
  #error "        If set to 0, memory is obtained through OSalloc()"
#endif
#ifndef OS_STATIC_STACK
  #error "Abassi: The build option OS_STATIC_STACK hasn't been defined"
#elif ((OS_STATIC_STACK) < 0)
  #error "Abassi: The build option OS_STATIC_STACK is negative"
  #error "        Must be >=0 as it defines the total number of bytes allocated for all task stacks"
  #error "        If set to 0, memory is obtained through OSalloc()"
#endif
#ifndef OS_STATIC_TASK
  #error "Abassi: The build option OS_STATIC_TASK hasn't been defined"
#elif ((OS_STATIC_TASK) < 0)
  #error "Abassi: The build option OS_STATIC_TASK is negative"
  #error "        Must be >= 0 as it defines the number task descriptors to allocate"
  #error "        If set to 0, memory is obtained through OSalloc()"
#endif
#ifndef OS_STATIC_TIM_SRV
  #error "Abassi: The build option OS_STATIC_TIM_SRV hasn't been defined"
#elif ((OS_STATIC_TIM_SRV) < 0)
  #error "Abassi: The build option OS_STATIC_TIM_SRV is negative"
  #error "        Must be >= 0 as it defines the number timer service descriptors to allocate"
  #error "        If set to 0, memory is obtained through OSalloc()"
#endif
#ifndef OS_TASK_SUSPEND
  #error "Abassi: The build option OS_TASK_SUSPEND hasn't been defined"
#endif
#ifndef OS_TIMEOUT
  #error "Abassi: The build option OS_TIMEOUT hasn't been defined"
#endif
#ifndef OS_TIMER_CB
  #error "Abassi: The build option OS_TIMER_CB hasn't been defined"
#elif ((OS_TIMER_CB) < 0)
  #error "Abassi: The build option OS_TIMER_CB is negative"
  #error "        Must be >= 0 as it defines the rate the timer tick calls the callback function"
  #error "        If set to 0, the timer tick does not call the callback function"
#endif
#ifndef OS_TIMER_SRV
  #error "Abassi: The build option OS_TIMER_SRV hasn't been defined"
#endif
#ifndef OS_TIMER_US
  #error "Abassi: The build option OS_TIMER_US hasn't been defined"
#elif ((OS_TIMER_US) < 0)
  #error "Abassi: The build option OS_TIMER_US is negative"
  #error "        Must be positive (>0) as it defines the period of the timer tick"
#endif
#ifndef OS_USE_TASK_ARG
  #error "Abassi: The build option OS_USE_TASK_ARG hasn't been defined"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Internal overloading																				*/
/* ------------------------------------------------------------------------------------------------ */

#if (OS_COOPERATIVE)							/* When the kernel operates in a cooperative mode	*/
	#define OX_ROUND_ROBIN		0				/* Round robin cannot exist							*/
	#define OX_SEARCH_ALGO		(-1)			/* The search algorithm must use the linked list	*/
#endif

#define OX_DO_NOTHING()			for(;0;)

/* ------------------------------------------------------------------------------------------------ */
/* Definition mapping RTOS timer tick <--> human time												*/
/* ------------------------------------------------------------------------------------------------ */

#if ((OS_TIMER_US) == 0)
  #if ((OS_ROUND_ROBIN) && ((OS_COOPERATIVE)==0))	/* OS_COOPERATIVE overloads OS_ROUND_ROBIN		*/
	#error "Abassi: The build option OS_ROUND_ROBIN is true, but OS_TIMER_US is false"
	#error "        This means round robin is activated but the timer is not enable"
  #endif
  #if (OS_STARVE_WAIT_MAX)
	#error "Abassi: The build option OS_STARVE_WAIT_MAX is true, but OS_TIMER_US is false"
	#error "        This means starvation priority boosting is activated but the timer is not enable"
  #endif
  #if ((OS_TIMEOUT) > 0)
	#error "Abassi: The build option OS_TIMEOUT is true, but OS_TIMER_US is false"
	#error "        This means timeouts are activated but the timer is not enable"
  #endif
  #if (OS_TIMER_CB)
	#error "Abassi: The build option OS_TIMER_CB is true, but OS_TIMER_US is false"
	#error "        This means a timer callback function is supplied but the timer is not enable"
  #endif
  #if (OS_TIMER_SRV)
	#error "Abassi: The build option OS_TIMER_SRV is true, but OS_TIMER_US is false"
	#error "        This means the timer services are enable but the timer is not enable"
  #endif
#endif


#if ((OS_STATIC_TASK) && ((OS_RUNTIME)==0))
  #error "Abassi: The build option OS_STATIC_TASK is true when OS_RUNTIME is false"
  #error "        OS_STATIC_TASK can only be used if OS_RUNTIME is true"
#endif
#if ((OS_STATIC_STACK) && ((OS_RUNTIME)==0))
  #error "Abassi: The build option OS_STATIC_STACK is true when OS_RUNTIME is false"
  #error "        OS_STATIC_STACK can only be used if OS_RUNTIME is true"
#endif
#if ((OS_STATIC_SEM) && ((OS_RUNTIME)==0))
  #error "Abassi: The build option OS_STATIC_SEM is true when OS_RUNTIME is false"
  #error "        OS_STATIC_SEM can only be used if OS_RUNTIME is true"
#endif
#if ((OS_STATIC_MBX) && ((OS_RUNTIME)==0) && (OS_MAILBOX))
  #error "Abassi: The build option OS_STATIC_MBX is true when OS_RUNTIME is false"
  #error "        OS_STATIC_MBX can only be used if OS_RUNTIME is true"
#endif
#if ((OS_STATIC_BUF_MBX) && ((OS_RUNTIME)==0) && (OS_MAILBOX))
  #error "Abassi: The build option OS_STATIC_BUF_MBX is true when OS_RUNTIME is false"
  #error "        OS_STATIC_BUF_MBX can only be used if OS_RUNTIME is true"
#endif
#ifdef OS_MEM_BLOCK
  #ifndef OS_STATIC_MBLK
	#error "Abassi: The build option OS_MEM_BLOCK is defined and OS_STATIC_MBLK is not"
	#error "        OS_STATIC_MBLK must be defined when OS_MEM_BLOCK is defined"
  #endif
  #ifndef OS_STATIC_BUF_MBLK
	#error "Abassi: The build option OS_MEM_BLOCK is defined and OS_STATIC_BUF_MBLK is not"
	#error "        OS_STATIC_BUF_MBLK must be defined when OS_MEM_BLOCK is defined"
  #endif
 #if (OS_MEM_BLOCK) && ((OS_RUNTIME)==0)
  #error "Abassi: The build option OS_MEM_BLOCK is true when OS_RUNTIME is false"
  #error "        Memory block can only be created at run time, set OS_RUNTIME to non-zero"
 #endif
 #if ((OS_STATIC_MBLK) && ((OS_RUNTIME)==0) && (OS_MEM_BLOCK))
  #error "Abassi: The build option OS_STATIC_MBLK is true when OS_RUNTIME is false"
  #error "        OS_STATIC_MBLK can only be used if OS_RUNTIME is true"
 #endif
 #if ((OS_STATIC_BUF_MBLK) && ((OS_RUNTIME)==0) && (OS_MEM_BLOCK))
  #error "Abassi: The build option OS_STATIC_BUF_MBLK is true when OS_RUNTIME is false"
  #error "        OS_STATIC_BUF_MBLK can only be used if OS_RUNTIME is true"
 #endif
#endif
#if ((OS_STATIC_TIM_SRV) && ((OS_RUNTIME)==0) && (OS_TIMER_SRV))
  #error "Abassi: The build option OS_STATIC_SEM is true when OS_RUNTIME is false"
  #error "        OS_STATIC_SEM can only be used if OS_RUNTIME is true"
#endif


#if ((OS_LOGGING_TYPE)==1)						/* For ASCII dump, need named descriptors			*/
  #define OX_NAMES				-1				/* So override the user supplied definition			*/
#endif


#if ((OS_TASK_SUSPEND) || (OS_MTX_DEADLOCK) || (OS_MTX_INVERSION))
  #define OX_HANDLE_MTX			1				/* These 3 build options require info about mutex	*/
#else											/* So override the user supplied definition			*/
  #define OX_HANDLE_MTX			0
#endif

#ifndef OS_IDLE_STACK
  #define OS_IDLE_STACK			0				/* Is now optional 									*/
#endif

#if ((OS_MTX_INVERSION) || (OS_STARVE_WAIT_MAX))
  #define OX_PRIO_CHANGE		1				/* These 2 build options require changing task prio	*/
#endif											/* So override the user supplied definition			*/

#ifndef OS_MTX_OWN_UNLOCK						/* Optional build option, set to false if not def'd	*/
  #define OS_MTX_OWN_UNLOCK		0				/* This restricts mutex unlocking to owner only		*/
#endif

#if ((OS_MAILBOX)==0)							/* Mailboxes are not part of the build				*/
  #define OX_STATIC_MBX			0				/* Override the user supplied definitions			*/
  #define OX_STATIC_BUF_MBX 	0
#endif

#ifndef OS_STATIC_BUF_MBLK						/* Optional build option, set to false if not def'd	*/
  #define OS_STATIC_BUF_MBLK	0
#endif
#ifndef OS_STATIC_MBLK							/* Optional build option, set to false if not def'd	*/
  #define OS_STATIC_MBLK		0
#endif
#ifndef OS_MEM_BLOCK							/* Optional build option, set to false if not def'd	*/
  #define OS_MEM_BLOCK			0
#endif
#if ((OS_MEM_BLOCK)==0)							/* Memory Blocks are not part of the build			*/
  #define OX_STATIC_MBLK		0				/* Override the user supplied definitions			*/
  #define OX_STATIC_BUF_MBLK 	0
#endif

#ifdef OX_NAMES									/* Names are not part of the build					*/
 #if ((OX_NAMES) <= 0)
  #define OX_STATIC_NAME		0				/* Names not supported, no need to reserve memory	*/
 #endif
#else
 #if ((OS_NAMES) <= 0)
  #define OX_STATIC_NAME		0				/* Names not supported, no need to reserve memory	*/
 #endif
#endif

#ifndef OS_HASH_ALL
  #define OS_HASH_ALL			0U
#else
  #if (((OS_HASH_ALL) & ((OS_HASH_ALL)-1U)) != 0U)
	#error "Abassi: The build option OS_HASH_ALL must be a power of 2"
  #else
   #if ((OS_HASH_ALL) != 0U)
	#define OX_HASH_MBX			(OS_HASH_ALL)
	#define OX_HASH_MBLK		(OS_HASH_ALL)
	#define OX_HASH_MUTEX		(OS_HASH_ALL)
	#define OX_HASH_SEMA		(OS_HASH_ALL)
	#define OX_HASH_TIMSRV		(OS_HASH_ALL)
	#define OX_HASH_TASK		(OS_HASH_ALL)
   #endif
  #endif
#endif
#ifndef OS_HASH_MBLK
  #define OS_HASH_MBLK			0U
#else
  #if (((OS_HASH_MBLK) & ((OS_HASH_MBLK)-1U)) != 0U)
	#error "Abassi: The build option OS_HASH_MBLK must be a power of 2"
  #endif
#endif
#ifndef OS_HASH_MBX
  #define OS_HASH_MBX			0U
#else
  #if (((OS_HASH_MBX) & ((OS_HASH_MBX)-1U)) != 0U)
	#error "Abassi: The build option OS_HASH_MBX must be a power of 2"
  #endif
#endif
#ifndef OS_HASH_MUTEX
  #define OS_HASH_MUTEX			0U
#else
  #if (((OS_HASH_MUTEX) & ((OS_HASH_MUTEX)-1U)) != 0U)
	#error "Abassi: The build option OS_HASH_MUTEX must be a power of 2"
  #endif
#endif
#ifndef OS_HASH_SEMA
  #define OS_HASH_SEMA			0U
#else
  #if (((OS_HASH_SEMA) & ((OS_HASH_SEMA)-1U)) != 0U)
	#error "Abassi: The build option OS_HASH_SEMA must be a power of 2"
  #endif
#endif
#ifndef OS_HASH_TASK
  #define OS_HASH_TASK			0U
#else
  #if (((OS_HASH_TASK) & ((OS_HASH_TASK)-1U)) != 0U)
	#error "Abassi: The build option OS_HASH_TASK must be a power of 2"
  #endif
#endif
#ifndef OS_HASH_TIMSRV
  #define OS_HASH_TIMSRV		0U
#else
  #if (((OS_HASH_TIMSRV) & ((OS_HASH_TIMSRV)-1U)) != 0U)
	#error "Abassi: The build option OS_HASH_TIMSRV must be a power of 2"
  #endif
#endif

#ifndef OX_GRP_PARKING
  #define OX_GRP_PARKING		0
#endif
#ifndef OX_MBLK_PARKING
  #define OX_MBLK_PARKING		0
#endif
#ifndef OX_MBX_PARKING
  #define OX_MBX_PARKING		0
#endif
#ifndef OX_SEM_PARKING
  #define OX_SEM_PARKING		0
#endif
#ifndef OX_TASK_PARKING
  #define OX_TASK_PARKING		0
#endif
#ifndef OX_TIM_PARKING
  #define OX_TIM_PARKING		0
#endif

#if ((OS_STARVE_WAIT_MAX) && ((OS_STARVE_PRIO) == (OS_PRIO_MIN)))
  #define OX_STARVE_WAIT_MAX	0				/* When set to min prio, is turned OFF				*/
#else
  #define OX_STARVE_WAIT_MAX	(OS_STARVE_WAIT_MAX)
#endif

#if ((OX_STARVE_WAIT_MAX) || ((OS_TIMER_US) && ((OS_ROUND_ROBIN) && ((OS_COOPERATIVE)==0))))
  #define OX_PRIO_SAME			1				/* Starvation protection requires same prio tasks	*/
#endif

#if (OX_STARVE_WAIT_MAX)
  #if ((OS_STARVE_PRIO) < 0)
	#if (((-(OS_STARVE_PRIO)) > (OS_PRIO_MIN)) || ((-(OS_STARVE_PRIO)) < 0))
	  #error "Abassi: The build option OS_STARVE_PRIO is out of range"
	  #error "        Must be bounded between 0 and OS_PRIO_MIN"
	#endif
  #else
	#if (((OS_STARVE_PRIO) > (OS_PRIO_MIN)) || ((OS_STARVE_PRIO) < 0))
	  #error "Abassi: The build option OS_STARVE_PRIO is out of range"
	  #error "        Must be bounded between 0 and OS_PRIO_MIN"
	#endif
  #endif
  #if ((OS_STARVE_RUN_MAX) == 0)
	#error "Abassi: With starvation protection enabled, OS_STARVE_RUN_MAX cannot be set to zero"
	#error "        Starvation protection is enabled because OS_STARVE_WAIT_MAX is non-zero"
  #endif
#else
	#define OX_STARVE_PRIO		0				/* Starvation protect is not part of the build		*/
	#define OX_STARVE_RUN_MAX	0				/* Override the user supplied definitions			*/
#endif

#ifndef OS_TIM_TICK_MULTI
  #define OS_TIM_TICK_MULTI		0
#endif

#if ((OS_TIMER_SRV) == 0)						/* Timer services are not part of the build			*/
  #define OX_STATIC_TIM_SRV 	0				/* Override the user supplied definitions			*/
#endif

#ifndef OS_TRACE_BACK
  #define OS_TRACE_BACK			0
#endif

#ifndef OS_WAIT_ABORT
  #define OS_WAIT_ABORT			0
#endif

#if ((((OS_MP_TYPE) & 6U) && ((OS_N_CORE) > 1)) && (OS_COOPERATIVE))
  #error "Abassi: Cooperative mode is not supported on multi-core"
  #error "        set the build option OS_COOPERATIVE to 0"
#endif

#if (OS_MIN_STACK_USE)							/* Control of the kernel stack usage				*/
  #define OX_STATIC 			static 
#else
  #define OX_STATIC
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Overloading of build definitions																	*/
/* All build options are remapped to OX_XXX to avoid confusion when using the tokens				*/
/* From now on, all build options are OX_XXX														*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef OS_AE_STACK_SIZE
  #define OS_AE_STACK_SIZE		0
#endif
#ifndef OX_AE_STACK_SIZE
  #define OX_AE_STACK_SIZE		(OS_AE_STACK_SIZE)
#endif
#ifndef OX_ALLOC_SIZE
  #define OX_ALLOC_SIZE			(OS_ALLOC_SIZE)
#endif
#ifndef OX_BSS_ZERO
  #define OX_BSS_ZERO			(OS_BSS_ZERO)
#endif
#ifndef OS_C_PLUS_PLUS
  #define OX_C_PLUS_PLUS		0
#endif
#ifndef OX_C_PLUS_PLUS
  #define OX_C_PLUS_PLUS		(OS_C_PLUS_PLUS)
#endif
#if (OX_C_PLUS_PLUS)
 #ifndef OX_DO_STATIC_INIT
  #define OX_DO_STATIC_INIT		0
 #endif
#endif
#ifndef OX_CACHE_LSIZE
  #define OX_CACHE_LSIZE		1
#endif
#ifndef OX_COOPERATIVE
  #define OX_COOPERATIVE		(OS_COOPERATIVE)
#endif
#ifndef OX_EVENTS
  #define OX_EVENTS				(OS_EVENTS)
#endif
#ifndef OX_FCFS
  #define OX_FCFS				(OS_FCFS)
#endif
#ifndef OS_GROUP
  #define OS_GROUP				0
#endif
#ifndef OX_GROUP
  #define OX_GROUP				(OS_GROUP)
#endif
#ifndef OX_HASH_ALL
  #define OX_HASH_ALL			(OS_HASH_ALL)
#endif
#ifndef OX_HASH_MBLK
  #define OX_HASH_MBLK			(OS_HASH_MBLK)
#endif
#ifndef OX_HASH_MBX
  #define OX_HASH_MBX			(OS_HASH_MBX)
#endif
#ifndef OX_HASH_MUTEX
  #define OX_HASH_MUTEX			(OS_HASH_MUTEX)
#endif
#ifndef OX_HASH_SEMA
  #define OX_HASH_SEMA			(OS_HASH_SEMA)
#endif
#ifndef OX_HASH_TASK
  #define OX_HASH_TASK			(OS_HASH_TASK)
#endif
#ifndef OX_HASH_TIMSRV
  #define OX_HASH_TIMSRV		(OS_HASH_TIMSRV)
#endif
#ifndef OX_IDLE_STACK
  #define OX_IDLE_STACK			(OS_IDLE_STACK)
#endif
#ifndef OX_INT_1_READ
  #define OX_INT_1_READ			1
#endif
#ifndef OX_ISR_TBL_OFFSET
  #define OX_ISR_TBL_OFFSET		0
#endif
#ifndef OX_LIB_REENT_PROTECT
  #define OX_LIB_REENT_PROTECT	0
#endif
#ifndef OX_TRACE_BACK
  #define OX_TRACE_BACK			(OS_TRACE_BACK)
#endif
#ifndef OX_LOGGING_TYPE
  #define OX_LOGGING_TYPE		(OS_LOGGING_TYPE)
#endif
#ifndef OX_MAILBOX
  #define OX_MAILBOX			(OS_MAILBOX)
#endif
#ifndef OX_MAX_PEND_RQST
  #define OX_MAX_PEND_RQST		(OS_MAX_PEND_RQST)
#endif
#ifndef OS_MBXPUT_ISR
  #define OS_MBXPUT_ISR			0
#endif
#ifndef OX_MBXPUT_ISR
  #define OX_MBXPUT_ISR			(OS_MBXPUT_ISR)
#endif
#ifndef OX_MEM_BLOCK
  #define OX_MEM_BLOCK			(OS_MEM_BLOCK)
#endif
#ifndef OX_MIN_STACK_USE
  #define OX_MIN_STACK_USE		(OS_MIN_STACK_USE)
#endif
#ifndef OX_MP_TYPE
  #if ((OS_N_CORE) == 1)							/* Single core, neither SMP nor BMP				*/
	#define OX_MP_TYPE			0U
  #else
	#define OX_MP_TYPE			(OS_MP_TYPE)
  #endif
#endif
#ifndef OX_MTX_DEADLOCK
  #define OX_MTX_DEADLOCK		(OS_MTX_DEADLOCK)
#endif
#ifndef OX_MTX_INVERSION
  #define OX_MTX_INVERSION		(OS_MTX_INVERSION)
#endif
#ifndef OX_MTX_OWN_UNLOCK
  #define OX_MTX_OWN_UNLOCK		(OS_MTX_OWN_UNLOCK)
#endif

#if (((OX_MTX_INVERSION) > 999) || ((OX_MTX_INVERSION) < -999))
  #define OX_PRIO_INV_ON_OFF	1
#else
  #define OX_PRIO_INV_ON_OFF	0
#endif
#ifndef OX_N_CORE
  #if (((OX_MP_TYPE) & 6U) == 0U)					/* When not SMP/BMP, then is a single core		*/
	#define OX_N_CORE			1
  #else
	#define OX_N_CORE			(OS_N_CORE)
  #endif
#endif
#ifndef OX_N_INTERRUPTS
  #define OX_N_INTERRUPTS		(OS_N_INTERRUPTS)
#endif
#ifndef OX_NESTED_INTS
 #ifndef OS_NESTED_INTS
  #error "Abassi: The build option OS_NESTED_INTS hasn't been defined"
 #endif
  #define OX_NESTED_INTS		(OS_NESTED_INTS)
#endif
#ifndef OX_NAMES
  #define OX_NAMES				(OS_NAMES)
#endif
#ifndef OS_OUT_OF_MEM
  #define OS_OUT_OF_MEM			0
#endif
#ifndef OX_OUT_OF_MEM								/* Optional build option						*/
  #define OX_OUT_OF_MEM			(OS_OUT_OF_MEM)
#endif
#ifndef OS_PERF_MON
  #define OS_PERF_MON			0
#endif
#ifndef OX_PERF_MON
  #define OX_PERF_MON			(OS_PERF_MON)
#endif
#ifndef OX_PM_TYPE									/* If not specified use 64 bit integer			*/
  #define OX_PM_TYPE			int64_t
#endif
#if ((OX_PERF_MON) == 0x7FFFFFFF)					/* Timer used but not perf mon					*/
   typedef OSpm_t  OSperfMon_t;						/* The precision is target specific				*/
#else
 #if ((OX_PERF_MON) > 0)							/* If using the RTOS timer tick					*/
  typedef int OSperfMon_t;							/* RTOS timer tick is always an int				*/
  #define OX_PERF_MON_DIV		(OX_PERF_MON)
 #elif ((OX_PERF_MON) <0)							/* Using a peripheral timer						*/
  typedef OX_PM_TYPE OSperfMon_t;					/* The precision is target specific				*/
  #define OX_PERF_MON_DIV		(-(OX_PERF_MON))
 #else
  #define OX_PERF_MON_DIV		(1)					/* Not used, in case of ...						*/
 #endif
#endif
#ifndef OS_PRE_CONTEXT
  #define OS_PRE_CONTEXT		0
#endif
#ifndef OX_PRE_CONTEXT
  #define OX_PRE_CONTEXT		(OS_PRE_CONTEXT)
#endif
#ifndef OX_PRIO_CHANGE
  #define OX_PRIO_CHANGE		(OS_PRIO_CHANGE)
#endif
#ifndef OX_PRIO_MIN
  #define OX_PRIO_MIN			(OS_PRIO_MIN)
#endif
#ifndef OX_PRIO_SAME
  #if ((OX_N_CORE) == 1)							/* When single core, use user set-up			*/
    #define OX_PRIO_SAME		(OS_PRIO_SAME)
  #else
    #define OX_PRIO_SAME		1					/* SMP/BMP it must be enabled					*/
  #endif
#endif
#ifndef OX_RUNTIME
  #define OX_RUNTIME			(OS_RUNTIME)
#endif
#ifndef OX_SEARCH_ALGO
  #define OX_SEARCH_ALGO		(OS_SEARCH_ALGO)
#endif
#ifndef OS_SPINLOCK_BASE
  #define OS_SPINLOCK_BASE		0
#endif
#ifndef OX_SPINLOCK_BASE
  #define OX_SPINLOCK_BASE		(OS_SPINLOCK_BASE)
#endif
#ifndef OS_STACK_CHECK								/* Optional build option						*/
  #define OS_STACK_CHECK		0
#endif
#ifndef OX_STACK_CHECK
  #define OX_STACK_CHECK		(OS_STACK_CHECK)
#endif
#ifndef OX_START_STACK
  #define OX_START_STACK		(OS_START_STACK)
#endif
#ifndef OX_STARVE_PRIO
  #define OX_STARVE_PRIO		(OS_STARVE_PRIO)
#endif
#ifndef OX_STARVE_RUN_MAX
  #define OX_STARVE_RUN_MAX		(OS_STARVE_RUN_MAX)
#endif
#ifndef OX_STATIC_BUF_MBX
  #define OX_STATIC_BUF_MBX		(OS_STATIC_BUF_MBX)
#endif
#ifndef OX_STATIC_MBX
  #define OX_STATIC_MBX			(OS_STATIC_MBX)
#endif
#ifndef OX_STATIC_BUF_MBLK
  #define OX_STATIC_BUF_MBLK	(OS_STATIC_BUF_MBLK)
#endif
#ifndef OX_STATIC_MBLK
  #define OX_STATIC_MBLK		(OS_STATIC_MBLK)
#endif
#ifndef OX_STATIC_NAME
  #define OX_STATIC_NAME		(OS_STATIC_NAME)
#endif
#ifndef OX_STATIC_SEM
  #define OX_STATIC_SEM			(OS_STATIC_SEM)
#endif
#ifndef OX_STATIC_STACK
  #define OX_STATIC_STACK		(OS_STATIC_STACK)
#endif
#ifndef OX_STATIC_TASK
  #define OX_STATIC_TASK		(OS_STATIC_TASK)
#endif
#ifndef OX_STATIC_TIM_SRV
  #define OX_STATIC_TIM_SRV		(OS_STATIC_TIM_SRV)
#endif
#ifndef OX_TASK_SUSPEND
  #define OX_TASK_SUSPEND		(OS_TASK_SUSPEND)
#endif

#ifndef OS_GRP_XTRA_FIELD
  #define OS_GRP_XTRA_FIELD			(0)
#else
  #if ((OS_GRP_XTRA_FIELD) < 0)
	#error "OS_GRP_XTRA_FIELD must be non-negative"
  #endif
#endif
#define OX_GRP_XTRA_MY_IDX			 (OS_GRP_XTRA_FIELD)
#define OX_GRP_1ST_XTRA_FIELD		 (0)			/* Defined for compatibility with Task			*/
#if defined(OX_GRP_XTRA_FIELD_5)
  #define OX_GRP_XTRA_FIELD			((OS_GRP_XTRA_FIELD)+5)
#elif defined(OX_GRP_XTRA_FIELD_4)
  #define OX_GRP_XTRA_FIELD			((OS_GRP_XTRA_FIELD)+4)
#elif defined(OX_GRP_XTRA_FIELD_3)
  #define OX_GRP_XTRA_FIELD			((OS_GRP_XTRA_FIELD)+3)
#elif defined(OX_GRP_XTRA_FIELD_2)
  #define OX_GRP_XTRA_FIELD			((OS_GRP_XTRA_FIELD)+2)
#elif defined(OX_GRP_XTRA_FIELD_1)
  #define OX_GRP_XTRA_FIELD			((OS_GRP_XTRA_FIELD)+1)
#else
  #define OX_GRP_XTRA_FIELD			 (OS_GRP_XTRA_FIELD)
#endif

#ifndef OS_MBLK_XTRA_FIELD
  #define OS_MBLK_XTRA_FIELD		(0)
#else
  #if ((OS_MBLK_XTRA_FIELD) < 0)
	#error "OS_MBLK_XTRA_FIELD must be non-negative"
  #endif
#endif
#define OX_MBLK_XTRA_MY_IDX			 (OS_MBLK_XTRA_FIELD)
#define OX_MBLK_1ST_XTRA_FIELD		 (0)			/* Defined for compatibility with Task			*/
#if defined(OX_MBLK_XTRA_FIELD_5)
  #define OX_MBLK_XTRA_FIELD		((OS_MBLK_XTRA_FIELD)+5)
#elif defined(OX_MBLK_XTRA_FIELD_4)
  #define OX_MBLK_XTRA_FIELD		((OS_MBLK_XTRA_FIELD)+4)
#elif defined(OX_MBLK_XTRA_FIELD_3)
  #define OX_MBLK_XTRA_FIELD		((OS_MBLK_XTRA_FIELD)+3)
#elif defined(OX_MBLK_XTRA_FIELD_2)
  #define OX_MBLK_XTRA_FIELD		((OS_MBLK_XTRA_FIELD)+2)
#elif defined(OX_MBLK_XTRA_FIELD_1)
  #define OX_MBLK_XTRA_FIELD		((OS_MBLK_XTRA_FIELD)+1)
#else
  #define OX_MBLK_XTRA_FIELD		 (OS_MBLK_XTRA_FIELD)
#endif

#ifndef OS_MBX_XTRA_FIELD
  #define OS_MBX_XTRA_FIELD			(0)
#else
  #if ((OS_MBX_XTRA_FIELD) < 0)
	#error "OS_MBX_XTRA_FIELD must be non-negative"
  #endif
#endif
#define OX_MBX_XTRA_MY_IDX			 (OS_MBX_XTRA_FIELD)
#define OX_MBX_1ST_XTRA_FIELD		 (0)			/* Defined for compatibility with Task			*/
#if defined(OX_MBX_XTRA_FIELD_5)
  #define OX_MBX_XTRA_FIELD			((OS_MBX_XTRA_FIELD)+5)
#elif defined(OX_MBX_XTRA_FIELD_4)
  #define OX_MBX_XTRA_FIELD			((OS_MBX_XTRA_FIELD)+4)
#elif defined(OX_MBX_XTRA_FIELD_3)
  #define OX_MBX_XTRA_FIELD			((OS_MBX_XTRA_FIELD)+3)
#elif defined(OX_MBX_XTRA_FIELD_2)
  #define OX_MBX_XTRA_FIELD			((OS_MBX_XTRA_FIELD)+2)
#elif defined(OX_MBX_XTRA_FIELD_1)
  #define OX_MBX_XTRA_FIELD			((OS_MBX_XTRA_FIELD)+1)
#else
  #define OX_MBX_XTRA_FIELD			 (OS_MBX_XTRA_FIELD)
#endif

#ifndef OS_SEM_XTRA_FIELD
  #define OS_SEM_XTRA_FIELD			(0)
#else
  #if ((OS_SEM_XTRA_FIELD) < 0)
	#error "OS_SEM_XTRA_FIELD must be non-negative"
  #endif
#endif
#define OX_SEM_XTRA_MY_IDX			 (OS_SEM_XTRA_FIELD)
#define OX_SEM_1ST_XTRA_FIELD		 (0)			/* Defined for compatibility with Task			*/
#if defined(OX_SEM_XTRA_FIELD_5)
  #define OX_SEM_XTRA_FIELD			((OS_SEM_XTRA_FIELD)+5)
#elif defined(OX_SEM_XTRA_FIELD_4)
  #define OX_SEM_XTRA_FIELD			((OS_SEM_XTRA_FIELD)+4)
#elif defined(OX_SEM_XTRA_FIELD_3)
  #define OX_SEM_XTRA_FIELD			((OS_SEM_XTRA_FIELD)+3)
#elif defined(OX_SEM_XTRA_FIELD_2)
  #define OX_SEM_XTRA_FIELD			((OS_SEM_XTRA_FIELD)+2)
#elif defined(OX_SEM_XTRA_FIELD_1)
  #define OX_SEM_XTRA_FIELD			((OS_SEM_XTRA_FIELD)+1)
#else
  #define OX_SEM_XTRA_FIELD			 (OS_SEM_XTRA_FIELD)
#endif

#ifndef OS_TIM_XTRA_FIELD
  #define OS_TIM_XTRA_FIELD			(0)
#else
  #if ((OS_TIM_XTRA_FIELD) < 0)
	#error "OS_TIM_XTRA_FIELD must be non-negative"
  #endif
#endif
#define OX_TIM_XTRA_MY_IDX			 (OS_TIM_XTRA_FIELD)
#define OX_TIM_1ST_XTRA_FIELD		 (0)			/* Defined for compatibility with Task			*/
#if defined(OX_TIM_XTRA_FIELD_5)
  #define OX_TIM_XTRA_FIELD			((OS_TIM_XTRA_FIELD)+5)
#elif defined(OX_TIM_XTRA_FIELD_4)
  #define OX_TIM_XTRA_FIELD			((OS_TIM_XTRA_FIELD)+4)
#elif defined(OX_TIM_XTRA_FIELD_3)
  #define OX_TIM_XTRA_FIELD			((OS_TIM_XTRA_FIELD)+3)
#elif defined(OX_TIM_XTRA_FIELD_2)
  #define OX_TIM_XTRA_FIELD			((OS_TIM_XTRA_FIELD)+2)
#elif defined(OX_TIM_XTRA_FIELD_1)
  #define OX_TIM_XTRA_FIELD			((OS_TIM_XTRA_FIELD)+1)
#else
  #define OX_TIM_XTRA_FIELD			 (OS_TIM_XTRA_FIELD)
#endif

#if defined(OX_TASK_XTRA_FIELD_5)					/* Task different: low index may be internal	*/
  #define OX_TASK_1ST_XTRA_FIELD	(OX_TASK_XTRA_FIELD_5)
#elif defined(OX_TASK_XTRA_FIELD_4)
  #define OX_TASK_1ST_XTRA_FIELD	(OX_TASK_XTRA_FIELD_4)
#elif defined(OX_TASK_XTRA_FIELD_3)
  #define OX_TASK_1ST_XTRA_FIELD	(OX_TASK_XTRA_FIELD_3)
#elif defined(OX_TASK_XTRA_FIELD_2)
  #define OX_TASK_1ST_XTRA_FIELD	(OX_TASK_XTRA_FIELD_2)
#elif defined(OX_TASK_XTRA_FIELD_1)
  #define OX_TASK_1ST_XTRA_FIELD	(OX_TASK_XTRA_FIELD_1)
#else
  #define OX_TASK_1ST_XTRA_FIELD	(0)
#endif
#ifndef OS_TASK_XTRA_FIELD
  #define OS_TASK_XTRA_FIELD		(0)
#else
  #if ((OS_TASK_XTRA_FIELD) < 0)
	#error "OS_TASK_XTRA_FIELD must be non-negative"
  #endif
#endif
#define OX_TASK_XTRA_FIELD			((OX_TASK_1ST_XTRA_FIELD)+(OS_TASK_XTRA_FIELD))

#ifdef OS_TIM_TICK_ACK
 #ifndef OX_TIM_TICK_ACK
  #define OX_TIM_TICK_ACK()		OS_TIM_TICK_ACK()
 #endif
#endif
#ifndef OX_TIMEOUT
  #define OX_TIMEOUT			(OS_TIMEOUT)
#endif
#ifndef OX_TIMER_CB
  #define OX_TIMER_CB			(OS_TIMER_CB)
#endif
#ifndef OX_TIM_TICK_MULTI
  #define OX_TIM_TICK_MULTI		(OS_TIM_TICK_MULTI)
#endif
#ifndef OX_TIMER_SRV
  #define OX_TIMER_SRV			(OS_TIMER_SRV)
#endif
#ifndef OX_TIMER_US
  #define OX_TIMER_US			(OS_TIMER_US)
#endif
#ifndef OX_USE_INT_REGISTER
  #define OX_USE_INT_REGISTER	0
#endif
#ifndef OX_USE_TASK_ARG
  #define OX_USE_TASK_ARG		(OS_USE_TASK_ARG)
#endif
#ifndef OX_WAIT_ABORT
  #define OX_WAIT_ABORT			(OS_WAIT_ABORT)
#endif
#ifndef OX_DO_STATIC_INIT
  #define OX_DO_STATIC_INIT		1
#endif
#ifndef OX_STR_CONST
  typedef const char OSstrCnst_t;
#endif

#ifndef REENT_POST
  #define REENT_POST
#endif

#ifndef OS_SYS_CALL
  #define OS_SYS_CALL			0
#endif
#ifndef OX_SYS_CALL
  #define OX_SYS_CALL			(OS_SYS_CALL)
#endif
#if ((OX_SYS_CALL) != 0)
  #ifndef OX_SYS_CALL_SIZE
    #define OX_SYS_CALL_SIZE	2				/* SysCall used, when size not set, most lib use 2	*/
  #endif
#endif
#ifndef OX_SYS_CALL_SIZE
  #define OX_SYS_CALL_SIZE		0
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Defintion mapping RTOS timer tick <--> human time												*/
/* ------------------------------------------------------------------------------------------------ */

#if ((OX_TIMER_US) != 0)						/* If the timer is used, a few human definitions   	*/
  #define OS_TICK_PER_SEC	((int)(((((long)(OX_TIMER_US))/2)+     1000000L ) /((long)(OX_TIMER_US))))

  #define ZZ_MS_TO_TICK(x)	((int)(((((long)(OX_TIMER_US))/2)+((x)*   1000L ))/((long)(OX_TIMER_US))))
  #define ZZ_SEC_TO_TICK(x) ((int)(((((long)(OX_TIMER_US))/2)+((x)*1000000LL))/((long)(OX_TIMER_US))))
  #define OS_MS_TO_TICK(xx)				((((signed)(xx))<0)?-1:(ZZ_MS_TO_TICK(xx)))
  #define OS_SEC_TO_TICK(xx)			((((signed)(xx))<0)?-1:(ZZ_SEC_TO_TICK(xx)))
  #define OS_HMS_TO_TICK(h,m,s)			(OS_SEC_TO_TICK(((h)*60+(m))*60+(s)))

  #define OS_TICK_TO_MIN_TICK(x,m)		(((((signed)(x))<0)||(((signed)(m))<0))?-1:(				\
										(((signed)(x))>((signed)(m)))?(x):(m)))
  #define OS_MS_TO_MIN_TICK(x,m)		(OS_TICK_TO_MIN_TICK(OS_MS_TO_TICK(x),m))

  #define OS_MS_EXP(m)					(OS_TICK_EXP(OS_MS_TO_TICK(m)))
  #define OS_TICK_EXP(x)				(G_OStimCnt+(x))
  #define OS_MS_TO_MIN_TICK_EXP(x,m)	(OS_TICK_TO_MIN_TICK_EXP(OS_MS_TO_TICK(x),m))
  #define OS_TICK_TO_MIN_TICK_EXP(x,m)	(G_OStimCnt+(OS_TICK_TO_MIN_TICK(x,m)))

  #define OS_HAS_TIMEDOUT(x)			((G_OStimCnt-((int)(x)))>=0)

  #define OS_TICK_EXPIRY(xx,mm)			(OS_MS_TO_MIN_TICK_EXP(xx,mm))	/* LEGACY!!!				*/
																		/* Should use OS_TICK_EXP()	*/
  #if ((OS_ROUND_ROBIN) && ((OS_COOPERATIVE)==0))	/* Round Robin value must be >= Timer us		*/
	#if ((OS_ROUND_ROBIN) < 0)					/* Need to have the absolute value					*/
	  #define OS_RR_ABS 		(0-(OS_ROUND_ROBIN))
	#else
	  #define OS_RR_ABS 		(OS_ROUND_ROBIN)
	#endif
    #if ((OS_RR_ABS) < (OX_TIMER_US))			/* Force it == if too small							*/
	  #if ((OS_ROUND_ROBIN) > 0)
		#define OX_ROUND_ROBIN	(OX_TIMER_US)
	  #else
		#define OX_ROUND_ROBIN	(0-(OX_TIMER_US))
	  #endif
	  #define OX_RR_ABS 		(OX_ROUND_ROBIN)
	#endif										/* Number of ticks in a round robin time slice		*/
	#ifndef OX_RR_ABS
	  #define OX_RR_ABS			(OS_RR_ABS)
	#endif
	#define OX_RR_TICK			((OX_RR_ABS)/(OX_TIMER_US))
  #endif
#endif

#ifndef OX_ROUND_ROBIN
  #define OX_ROUND_ROBIN		(OS_ROUND_ROBIN)
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Data types & definitions																			*/
/* ------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------	*/
/* Semaphore descriptor								*/

typedef struct _SEM_t {							/* Semaphore descriptor								*/
	struct _TSK_t *Blocked;						/* Linked list of tasks blocked by this semaphore	*/
	struct _TSK_t *MtxOwner;					/* Owner / locker of the mutex (when it is a mutex)	*/
  #if (OX_HANDLE_MTX)
	struct _SEM_t *MtxNext;						/* The next Mutex owned by the same task			*/
  #endif
  #if (OX_NAMES)
	const char    *SemName;						/* Name of the semaphore							*/
	struct _SEM_t *SemaNext;					/* Next semaphore in system (when searching name)	*/
  #endif
	int   Value;								/* Semaphore counter								*/
  #if (OX_FCFS)									/* When 1st come 1st served enable					*/
	int IsFCFS;									/* This indicates if it's a FCFS semaphore/mutex	*/
  #endif
  #if ((OX_MTX_INVERSION) < 0)					/* For priority ceiling, we need to attach a		*/
	int CeilPrio;								/* priority to the mutex							*/
  #endif
  #if (OX_PRIO_INV_ON_OFF)
	int PrioInvOn;
  #endif
  #if ((OX_MTX_OWN_UNLOCK) < 0)					/* When checking if the unlocker of a mutex is the	*/
	int IgnoreOwn;								/* owner (locker) of the mutex						*/
  #endif
  #if (OX_GROUP)								/* When supporting groups, this is the pointer to	*/
	struct _GRP_t *GrpOwner;					/* the group descriptor this semaphore is part of	*/
   #if (OX_C_PLUS_PLUS)
    void *CppSemRef[2];                         /* Ptr to associated AbassiCPP::semaphore_t object  */
   #endif
  #endif
  #if (OX_SEM_PARKING)							/* When "destroy semaphore/mutex" is needed			*/
	struct _SEM_t *ParkNext; 					/* Keep the "destroyed" mutex in a parking lot		*/
  #endif
  #if (OX_SEM_XTRA_FIELD)
	intptr_t XtraData[OX_SEM_XTRA_FIELD];
  #endif
} SEM_t;

/* ------------------------------------------------	*/
/* Mutex descriptor									*/

typedef SEM_t MTX_t;							/* Mutexes are same data type as semaphores			*/
												/* Would have prefer to use .field=XXX but a few	*/
#if (OX_DO_STATIC_INIT)							/* compilers do not support it. So has to revert	*/
 #if ((OX_RUNTIME) >= 0)						/* to the old K&R initialization scheme				*/
  #if (OX_HANDLE_MTX)
	#define INIT_SEM_MTXNEXT			,(SEM_t *)NULL
  #else
	#define INIT_SEM_MTXNEXT
  #endif
  #if (OX_NAMES)
	#define INIT_SEM_NAME(_Name)		,(_Name)
	#define INIT_SEM_SEMANEXT			,(SEM_t *)NULL
  #else
	#define INIT_SEM_NAME(_Name)
	#define INIT_SEM_SEMANEXT
  #endif
  #if (OX_FCFS)
	#define INIT_SEM_ISFCFS				,0
  #else
	#define INIT_SEM_ISFCFS
  #endif
  #if ((OX_MTX_INVERSION) < 0)
	#define INIT_SEM_CEILPRIO			,0
	#define INIT_MTX_CEILPRIO			,(OX_PRIO_MIN)
  #else
	#define INIT_SEM_CEILPRIO
	#define INIT_MTX_CEILPRIO
  #endif
  #if (OX_PRIO_INV_ON_OFF)
	#define INIT_SEM_PRIO_INV_ON		,1
  #else
	#define INIT_SEM_PRIO_INV_ON
  #endif
  #if ((OX_MTX_OWN_UNLOCK) < 0)
	#define INIT_SEM_IGNORE_OWN			,0
  #else
	#define INIT_SEM_IGNORE_OWN
  #endif
  #if (OX_GROUP)
	#define INIT_SEM_GRP_OWNER			,(struct _GRP_t *) NULL
   #if (OX_C_PLUS_PLUS)
    #define INIT_SEM_CPP_SEM_REF		,{((void *)NULL), ((void *)NULL)}
   #else
	#define INIT_SEM_CPP_SEM_REF
   #endif
  #else
	#define INIT_SEM_GRP_OWNER
	#define INIT_SEM_CPP_SEM_REF
  #endif
  #if (OX_SEM_PARKING)
	#define INIT_SEM_PARKING			,((void *)NULL)
  #else
	#define INIT_SEM_PARKING
  #endif
  #if ((OX_SEM_XTRA_FIELD) != 0)
   #if ((OX_SEM_XTRA_FIELD)   == 1)
	#define INIT_SEM_XTRADATA			,{((intptr_t)0)}
   #elif ((OX_SEM_XTRA_FIELD) == 2)
	#define INIT_SEM_XTRADATA			,{((intptr_t)0),((intptr_t)0)}
   #elif ((OX_SEM_XTRA_FIELD) == 3)
	#define INIT_SEM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_SEM_XTRA_FIELD) == 4)
	#define INIT_SEM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_SEM_XTRA_FIELD) == 5)
	#define INIT_SEM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0)}
   #elif ((OX_SEM_XTRA_FIELD) == 6)
	#define INIT_SEM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0)}
   #elif ((OX_SEM_XTRA_FIELD) == 7)
	#define INIT_SEM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_SEM_XTRA_FIELD) == 8)
	#define INIT_SEM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_SEM_XTRA_FIELD) == 9)
	#define INIT_SEM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
	                                      ((intptr_t)0)}
   #elif ((OX_SEM_XTRA_FIELD) == 10)
	#define INIT_SEM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0)}
   #else
	#error "Too many OS_SEM_XTRA_FIELD requested; mAbassi.h must be updated / modified"
   #endif
  #else
	#define INIT_SEM_XTRADATA
  #endif

  #define SEM_STATIC(__Var, __Name)		\
   static SEM_t __Var##__ = {			\
	 (TSK_t *)NULL						\
	,(TSK_t *)NULL						\
	INIT_SEM_MTXNEXT					\
	INIT_SEM_NAME(__Name)				\
	INIT_SEM_SEMANEXT					\
	,0									\
	INIT_SEM_ISFCFS						\
	INIT_SEM_CEILPRIO					\
	INIT_SEM_PRIO_INV_ON				\
	INIT_SEM_IGNORE_OWN					\
	INIT_SEM_GRP_OWNER					\
	INIT_SEM_CPP_SEM_REF				\
	INIT_SEM_PARKING					\
	INIT_SEM_XTRADATA					\
  };									\
  SEM_t *(__Var) = &(__Var##__)

  #define MTX_STATIC(__Var, __Name)		\
   static MTX_t __Var##__ = {			\
	 (TSK_t *)NULL						\
	,(TSK_t *)NULL						\
	INIT_SEM_MTXNEXT					\
	INIT_SEM_NAME(__Name)				\
	INIT_SEM_SEMANEXT					\
	,1									\
	INIT_SEM_ISFCFS						\
	INIT_MTX_CEILPRIO					\
	INIT_SEM_PRIO_INV_ON				\
	INIT_SEM_IGNORE_OWN					\
	INIT_SEM_GRP_OWNER					\
	INIT_SEM_CPP_SEM_REF				\
	INIT_SEM_PARKING					\
	INIT_SEM_XTRADATA					\
  };									\
  MTX_t *(__Var) = &(__Var##__)
 #endif
#endif

#ifndef SEM_STATIC								/* This seems the best way to report unavailable	*/
  #define SEM_STATIC(_a, _b)	SEM_STATIC_is_unavailable	/* because can't use #error in here		*/
#endif
#ifndef MTX_STATIC
  #define MTX_STATIC(_a, _b)	MTX_STATIC_is_unavailable
#endif

/* ------------------------------------------------ */
/* Timer Services descriptor						*/

#if (OX_TIMER_SRV)

typedef void(*OStimerFct_t)(intptr_t Arg);		/* Function attached when Ops == TIM_CALL_FCT		*/

  typedef struct _TIM_t {
	struct _TIM_t *TmNext;						/* Next timer descriptor in the linked list			*/
	struct _TIM_t *TmPrev;						/* Previous timer descriptor in the linked list		*/
  #if (OX_NAMES)
	const char    *TimName;						/* Name of a timer service							*/
	struct _TIM_t *TimNext;						/* Next timer in the list (to search name)			*/
  #endif
	void          *Ptr;							/* Ptr to fct, data or RTOS component				*/
	void (*FctPtr)(intptr_t Arg);				/* *Ptr to fct call upon timeout					*/
	intptr_t       Arg;							/* When operation needs an argument					*/
	intptr_t       ArgOri;						/* Original value of ARG for TIMrestart()			*/
	unsigned int   Ops;							/* Operation to perform								*/
	int            Period;						/* When != 0, period of a periodic timer			*/
	int            Expiry;						/* Needed when the timer is re-started				*/
	int            ExpOri;						/* Needed when the timer is re-started after freeze	*/
	int            TickDone;					/* Tick value for timeout							*/
	int            ToAdd;						/* Increment, decrement to add to argument			*/
	int            Paused;						/* > 0 - no callback / < 0 -countdown stopped		*/
  #if (OX_TIM_PARKING)							/* When "destroy timer" is needed					*/
	struct _TIM_t *ParkNext; 					/* Keep the "destroyed" in a parking lot			*/
  #endif
  #if (OX_TIM_XTRA_FIELD)
	intptr_t XtraData[OX_TIM_XTRA_FIELD];
  #endif
  } TIM_t;

 #if (OX_DO_STATIC_INIT)
  #if ((OX_RUNTIME) >= 0)
   #if (OX_NAMES)
	 #define INIT_TIM_NAME(_Name)		,(_Name)
	 #define INIT_TIM_TIMNEXT			,(TIM_t *)NULL
   #else
	 #define INIT_TIM_NAME(_Name)
	 #define INIT_TIM_TIMNEXT
   #endif
  #if (OX_TIM_PARKING)
	#define INIT_TIM_PARKING			,((void *)NULL)
  #else
	#define INIT_TIM_PARKING
  #endif
  #if ((OX_TIM_XTRA_FIELD) != 0)
   #if ((OX_TIM_XTRA_FIELD)   == 1)
	#define INIT_TIM_XTRADATA			,{((intptr_t)0)}
   #elif ((OX_TIM_XTRA_FIELD) == 2)
	#define INIT_TIM_XTRADATA			,{((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TIM_XTRA_FIELD) == 3)
	#define INIT_TIM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TIM_XTRA_FIELD) == 4)
	#define INIT_TIM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TIM_XTRA_FIELD) == 5)
	#define INIT_TIM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0)}
   #elif ((OX_TIM_XTRA_FIELD) == 6)
	#define INIT_TIM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TIM_XTRA_FIELD) == 7)
	#define INIT_TIM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TIM_XTRA_FIELD) == 8)
	#define INIT_TIM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TIM_XTRA_FIELD) == 9)
	#define INIT_TIM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
	                                      ((intptr_t)0)}
   #elif ((OX_TIM_XTRA_FIELD) == 10)
	#define INIT_TIM_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0)}
   #else
	#error "Too many OS_TIM_XTRA_FIELD requested; mAbassi.h must be updated / modified"
   #endif
  #else
	#define INIT_TIM_XTRADATA
  #endif

   #define TIM_STATIC(__Var, __Name)	\
    static TIM_t __Var##__ = {			\
	   (TIM_t *)NULL					\
	 , (TIM_t *)NULL					\
	 INIT_TIM_NAME(__Name)				\
	 INIT_TIM_TIMNEXT					\
	 , (void *)NULL						\
	 , (OStimerFct_t)NULL				\
	 , (intptr_t)0						\
	 , (intptr_t)0						\
	 , 0U								\
	 , 0								\
	 , 0								\
	 , 0								\
	 , 0								\
	 , 0								\
	 , 0								\
	 INIT_TIM_PARKING					\
	 INIT_TIM_XTRADATA					\
    };									\
    TIM_t *(__Var) = &(__Var##__)
  #endif
 #endif
#endif
#ifndef TIM_STATIC								/* This seems the best way to report unavailable	*/
  #define TIM_STATIC(_a, _b)	TIM_STATIC_is_unavailable	/* because can't use #error in here		*/
#endif


/* ------------------------------------------------	*/
/* Task descriptor									*/

typedef struct _TSK_t {							/* Process descriptor								*/
	void       *Stack;							/* Stack pointer when not running (context save)	*/
  #if (OX_TASK_XTRA_FIELD)						/* Some ports require custom information in here	*/
	intptr_t    XtraData[OX_TASK_XTRA_FIELD];
  #endif
	SEM_t       Priv;							/* Private semaphore								*/
	SEM_t      *Blocker;						/* Semaphore blocking this task						*/
	struct _TSK_t *Next;						/* Next task blocked/ready to run					*/
	struct _TSK_t *Prev;						/* Previous task blocked/ready to run				*/
  #if (OX_PRIO_SAME)
	struct _TSK_t *Last;						/* Last ready with same priority: eliminates a scan	*/
  #endif
  #if (OX_NAMES)
	const char    *TskName;						/* Name of the task									*/
	struct _TSK_t *TaskNext;					/* Next task in the list (to search name)			*/
  #endif
  #if (OX_HANDLE_MTX)
	SEM_t         *MyMutex;						/* Linked list of all mutex I'm the owner			*/
  #endif 
  #if (OX_USE_TASK_ARG)
	void          *TaskArg;						/* Pointer to the argument of the task				*/
  #endif
  #if ((OX_SEARCH_ALGO) < 0)					/* When using link lists instead of array			*/
	struct _TSK_t *PrioNext;					/* Task at a lower priority							*/
	struct _TSK_t *PrioPrev;					/* Task at a higher priority						*/
  #endif
  #if (OX_STARVE_WAIT_MAX)
	struct _TSK_t *StrvNext;					/* Next in linked list of ready to run task			*/
	struct _TSK_t *StrvPrev;					/* Previous in linked list of ready to run task		*/
	struct _TSK_t *StrvLast;					/* Last in the linked list of ready to run			*/
	volatile int   StrvState;					/* State of the protection for this task			*/
	int            StrvPrio;					/* Priority before starvation priority promotion	*/
  #endif
  #if ((OX_STARVE_PRIO) < 0)
	int  StrvPrioMax;							/* Maximum priority under starvation				*/
  #endif
  #if ((OX_STARVE_RUN_MAX) < 0)
	int StrvRunMax;								/* Maximum run-time under starvation				*/
  #endif
  #if ((OX_STARVE_WAIT_MAX) < 0)
	int StrvWaitMax;							/* Maximum time at same priority under starvation	*/
  #endif
  #if ((OX_TIMEOUT) > 0)
	struct _TSK_t *ToutNext;					/* Next task in the timeout linked list				*/
	struct _TSK_t *ToutPrev;					/* Previous task in the timeout linked list			*/
	int            TickTime;					/* RTOS counter tick value for timeout				*/
  #endif
  #if ((OX_TASK_SUSPEND) || ((OX_TIMEOUT) > 0) || (OX_GROUP) || (OX_WAIT_ABORT))
	int  PrevRetVal;							/* Return value before getting suspended			*/
  #endif
	int         Prio;							/* Current priority of the task						*/
  #if (OX_TASK_SUSPEND)
	unsigned int SuspRqst;						/* Request to suspend the task (Boolean)			*/
  #endif
  #if (OX_ROUND_ROBIN)
	unsigned int RRcnt;							/* This counts how many timer ticks the task has	*/
	         int RRtick;						/* been running, RRtick is last timer tick update	*/
	#if ((OX_ROUND_ROBIN) < 0)
	  unsigned int RRmax;						/* Variable round-robin time slice duration			*/
	#endif										/* Round-robin maximum run-time slice duration		*/
  #endif
  #if (OX_COOPERATIVE)
	int Yield;									/* Request to yield (3 states)						*/
  #endif
  #if (OX_MTX_INVERSION)						/* To eliminate priority inversion, need to			*/
	volatile int OriPrio;						/* temporary increase the priority of the task		*/
  #endif										/* Need volatile as one port has problems without	*/
  #if (OX_EVENTS)
	unsigned int EventAcc;						/* Accumulation of the events bits					*/
	unsigned int EventRX;						/* Event bits received								*/
	unsigned int EventAND;						/* AND mask for the events to unblock				*/
	unsigned int EventOR;						/* OR mask for the events to unblock				*/
  #endif
  #if (OX_GROUP)								/* When supporting groups							*/
	struct _GRP_t *GrpBlkOn;					/* Group this task is blocked on when non-NULL		*/
	void         (* GrpCBfct)(intptr_t CBarg);	/* Callback function								*/
	intptr_t       GrpCBarg;					/* Argument to the callback function				*/
  #endif										/* It controls if the queue is read or not			*/
  #if (OX_MAILBOX)
	intptr_t       MboxMsg;						/* Message to write when blocked on a full mailbox	*/
  #endif
  #if (OX_MEM_BLOCK)
	OSalign_t     *MblkBuf;						/* Buffer to return when blocked on an empty pool	*/
  #endif
  #if (OX_STACK_CHECK)							/* When stack checking, memo stack boundaries		*/
	int32_t *StackLow;							/* Low memory address of the stack					*/
	int32_t *StackHigh;							/* High memory address of the stack					*/
  #endif
  #if ((OX_N_CORE) > 1)
	int MyCore;									/* Core # (-1: not running) speeds load balancing	*/
  #endif
  #if (((OX_MP_TYPE) & 4U) != 0U)
	int TargetCore;								/* With BMP, needs to hold the target core info		*/
  #endif
  #if ((OX_PERF_MON) != 0)
   #define OX_PERF_MON_FIRST	PMcumul[0]		/* Define needed to zero all PM stats				*/
   #define PMcoreRun PMcumul					/* Backward compatibility for older versions		*/
	OSperfMon_t PMcumul[OX_N_CORE];				/* Cumulative time spend on a core					*/
	OSperfMon_t PMstartTick;					/* Tick counter when the measurement was started	*/
	OSperfMon_t PMlastTick;						/* Tick counter when the measurement was stopped	*/
												/* Latency: time from unblocked (kernel) to running	*/
	OSperfMon_t PMlatentLast;					/* Last run latency time							*/
	OSperfMon_t PMlatentMin;					/* Minimum latency time								*/
	OSperfMon_t PMlatentMax;					/* Maximum latency time								*/
	OSperfMon_t PMlatentAvg;					/* Average latency time								*/
	OSperfMon_t PMlatentStrt;					/* Time snapshot when the task was unblocked		*/
	int         PMlatentMinOK;					/* To init the minimum								*/
	int         PMlatentOK;						/* Latency not valid when running task restarted	*/
												/* Alive: total time span from unblocked -> blocked	*/
	OSperfMon_t PMaliveLast;					/* Last run alive time 								*/
	OSperfMon_t PMaliveMin;						/* Minimum alive time								*/
	OSperfMon_t PMaliveMax;						/* Maximum alive time								*/
	OSperfMon_t PMaliveAvg;						/* Average alive time								*/
	OSperfMon_t PMaliveStrt;					/* Time snapshot when the task started running		*/
												/* Run: time the task is running, using CPU			*/
	OSperfMon_t PMrunLast;						/* Last run CPU usage time 							*/
	OSperfMon_t PMrunMin;						/* Minimum CPU usage time							*/
	OSperfMon_t PMrunMax;						/* Maximum CPU usage time							*/
	OSperfMon_t PMrunAvg;						/* Average CPU usage time							*/
	OSperfMon_t PMrunStrt;						/* Time snapshot when task was back running			*/
	OSperfMon_t PMrunCum;						/* Cumulative run time for a slice					*/
	int         PMrunOK;						/* Slice run not valid when running task restarted	*/
												/* Preemption: time span between preempt --> running*/
	OSperfMon_t PMpreemLast;					/* Last run preemption time 						*/
	OSperfMon_t PMpreemMin;						/* Minimum preemption time							*/
	OSperfMon_t PMpreemMax;						/* Maximum latency time								*/
	OSperfMon_t PMpreemAvg;						/* Average latency time								*/
	OSperfMon_t PMpreemStrt;					/* Time snapshot when the task was unblocked		*/
	uint32_t    PMpreemCnt;						/* # of times a task got preempted					*/

	uint32_t    PMblkCnt;						/* # of times a task got blocked					*/
	uint32_t    PMsemBlkCnt;					/* # of times a task got blocked on semaphores		*/
	uint32_t    PMmtxBlkCnt;					/* # of times a task got blocked on mutexes			*/

   #if (OX_EVENTS)
	uint32_t    PMevtBlkCnt;					/* # of times a task got blocked on events			*/
   #endif
   #if (OX_GROUP)
	uint32_t    PMgrpBlkCnt;					/* # of times a task got blocked on groups			*/
   #endif
   #if (OX_MAILBOX)
	uint32_t    PMmbxBlkCnt;					/* # of times a task got blocked on mailboxes		*/
   #endif
   #if (OX_MEM_BLOCK)
	uint32_t    PMmblkBlkCnt;					/* # of times a task got blocked on memory blocks	*/
   #endif
   #if (OX_STARVE_WAIT_MAX)
	uint32_t    PMstrvCnt;						/* # of times task was under starvation protect		*/
	uint32_t    PMstrvRun;						/* # of times task ran under starvation protect		*/
	uint32_t    PMstrvRunMax;					/* Idem PMstrvRun except ran max time				*/
   #endif
   #if (OX_MTX_INVERSION)
	uint32_t    PMinvertCnt;					/* # of times task's prio raised under prio invert	*/
   #endif

	int          PMminOK;						/* Instead of using initial INT_MAX for mins		*/
   #define OX_PERF_MON_LAST		PMminOK			/* Define needed to zero all PM stats				*/
												/* The followings cannot be part of the zeroing		*/
	volatile int PMupdating;					/* If stats are being updated						*/
	volatile int PMsnapshot;					/* Flags needed when taking a snap shot of  stats	*/
	unsigned int PMcontrol;						/* Collection state: see PerfMon.c for details		*/
	  #define OX_PM_CTRL_PEND		(0)			/* When task becomes unblocked						*/
	  #define OX_PM_CTRL_STOP		(1U<<0)		/* The capturing is stopped							*/
	  #define OX_PM_CTRL_RESTART	(1U<<1)		/* Restart new run sequence							*/
	  #define OX_PM_CTRL_ARMED		(1U<<2)		/* The task is waiting to be running the first time	*/
	  #define OX_PM_CTRL_RUN		(1U<<3)		/* Is currently in a valid run sequence				*/

  #endif
  #if (OX_C_PLUS_PLUS)
	void *CppClass;								/* To hold the task class							*/
  #endif
  #if ((OX_SYS_CALL) && (OX_SYS_CALL_SIZE))		/* When using optional system call layer			*/
	intptr_t SysCall[OX_SYS_CALL_SIZE];
  #endif
  #if (OX_TASK_PARKING)							/* When "destroy task" is needed					*/
	struct _TSK_t *ParkNext; 					/* Keep the "destroyed" in a parking lot			*/
  #endif
} TSK_t;

#if (OX_DO_STATIC_INIT)
 #if ((OX_RUNTIME) >= 0)
  typedef struct {								/* When Data ptr are smaller size than function	ptr */
	TSK_t Tsk;									/* then we can't re-use a data pointer to hold the	*/
	void (*TskFct)(void);						/* address of the function attached to the task in	*/
  } _sTSK_t;									/* anything in the task descriptor. e.g MSP430X		*/

  #if (OX_TASK_XTRA_FIELD)
   #if ((OX_TASK_XTRA_FIELD)   == 1)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 2)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 3)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 4)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 5)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 6)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 7)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 8)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 9)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
	                                      ((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 10)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 11)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 12)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 13)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
	                                      ((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 14)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 15)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_TASK_XTRA_FIELD) == 16)
	#define INIT_TASK_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #else
	#error "Too many OS_TASK_XTRA_FIELD requested; mAbassi.h must be updated / modified"
   #endif
  #else
	#define INIT_TASK_XTRADATA
  #endif
  #if (OX_PRIO_SAME)
	#define INIT_TASK_LAST				,(TSK_t *)NULL
  #else
	#define INIT_TASK_LAST
  #endif
  #if (OX_NAMES)
	#define INIT_TASK_NAME(_Name)		,(_Name)
	#define INIT_TASK_TASKNEXT			,(TSK_t *)NULL
  #else
	#define INIT_TASK_NAME(_Name)
	#define INIT_TASK_TASKNEXT
  #endif
  #if (OX_HANDLE_MTX)
	#define INIT_TASK_MYMUTEX			,(SEM_t *)NULL
  #else
	#define INIT_TASK_MYMUTEX
  #endif 
  #if (OX_USE_TASK_ARG)
	#define INIT_TASK_TASKARG			,(void *)NULL
  #else
	#define INIT_TASK_TASKARG
  #endif
  #if ((OX_SEARCH_ALGO) < 0)					/* When using link list instead of array			*/
	#define INIT_TASK_PRIONEXT			,(TSK_t *)NULL
	#define INIT_TASK_PRIOPREV			,(TSK_t *)NULL
  #else
	#define INIT_TASK_PRIONEXT
	#define INIT_TASK_PRIOPREV
  #endif
  #if (OX_STARVE_WAIT_MAX)
	#define INIT_TASK_STARVNEXT			,(TSK_t *)NULL
	#define INIT_TASK_STARVPREV			,(TSK_t *)NULL
	#define INIT_TASK_STARVLAST			,(TSK_t *)NULL
	#define INIT_TASK_STARVSTATE		,0
	#define INIT_TASK_STARVPRIO(_Prio)	,(_Prio)
	#if ((OX_STARVE_PRIO) < 0)
	  #define INIT_TASK_STARVPRIOMAX	,-(OX_STARVE_PRIO)
	#else
	  #define INIT_TASK_STARVPRIOMAX
	#endif
	#if ((OX_STARVE_RUN_MAX) < 0)
	  #define INIT_TASK_STARVRUNMAX		,-(OX_STARVE_RUN_MAX)
	#else
	  #define INIT_TASK_STARVRUNMAX
	#endif
	#if ((OX_STARVE_WAIT_MAX) < 0)
	  #define INIT_TASK_STARVWAITMAX	,-(OX_STARVE_WAIT_MAX)
	#else
	  #define INIT_TASK_STARVWAITMAX
	#endif
  #else
	#define INIT_TASK_STARVNEXT
	#define INIT_TASK_STARVPREV
	#define INIT_TASK_STARVLAST
	#define INIT_TASK_STARVSTATE
	#define INIT_TASK_STARVPRIO(_Prio)
	#define INIT_TASK_STARVPRIOMAX
	#define INIT_TASK_STARVRUNMAX
	#define INIT_TASK_STARVWAITMAX
  #endif
  #if ((OX_TIMEOUT) > 0)
	#define INIT_TASK_TOUTNEXT			,(TSK_t *)NULL
	#define INIT_TASK_TOUTPREV			,(TSK_t *)NULL
	#define INIT_TASK_TICKTIME			,0
  #else
	#define INIT_TASK_TOUTNEXT
	#define INIT_TASK_TOUTPREV
	#define INIT_TASK_TICKTIME
  #endif
  #if ((OX_TASK_SUSPEND) || ((OX_TIMEOUT) > 0) || (OX_GROUP) || (OX_WAIT_ABORT))
	#define INIT_TASK_PREVRETVAL		,0
  #else
	#define INIT_TASK_PREVRETVAL
  #endif
  #if (OX_TASK_SUSPEND)
    #define INIT_TASK_SUSPRQST			,0U
  #else
	#define INIT_TASK_SUSPRQST
  #endif
  #if (OX_ROUND_ROBIN)
	#define INIT_TASK_RRCNT				,0U
	#define INIT_TASK_RRTICK			,0U
	#if ((OX_ROUND_ROBIN) < 0)
		#define INIT_TASK_RRMAX			,(unsigned int)(OX_RR_TICK)
	#else
		#define INIT_TASK_RRMAX
	#endif
  #else
	#define INIT_TASK_RRCNT
	#define INIT_TASK_RRTICK
	#define INIT_TASK_RRMAX
  #endif
  #if (OX_COOPERATIVE)
	#define	INIT_TASK_YIELD				,0
  #else
	#define	INIT_TASK_YIELD
  #endif
  #if (OX_MTX_INVERSION)
	#define INIT_TASK_ORIPRIO(_Prio)	,(_Prio)
  #else
	#define INIT_TASK_ORIPRIO(_Prio)
  #endif
  #if (OX_EVENTS)
	#define INIT_TASK_EVENTRX			,0U
	#define INIT_TASK_EVENTAND			,0U
	#define INIT_TASK_EVENTOR			,0U
	#define INIT_TASK_EVENTACC			,0U
  #else
	#define INIT_TASK_EVENTRX
	#define INIT_TASK_EVENTAND
	#define INIT_TASK_EVENTOR
	#define INIT_TASK_EVENTACC
  #endif
  #if (OX_GROUP)
	#define INIT_TASK_GRP_BLK_ON		,(struct _GRP_t *) NULL
	#define INIT_TACK_GRP_CB_FCT		,(void *)NULL
	#define INIT_TASK_GRP_CB_ARG		,(intptr_t)0
  #else
	#define INIT_TASK_GRP_BLK_ON
	#define INIT_TACK_GRP_CB_FCT
	#define INIT_TASK_GRP_CB_ARG
  #endif
  #if (OX_MAILBOX)
	#define INIT_TASK_MBOXMSG			,(intptr_t)0
  #else
	#define INIT_TASK_MBOXMSG
  #endif
  #if (OX_MEM_BLOCK)
	#define INIT_TASK_MBLKBUF			,(OSalign_t *)NULL
  #else
	#define INIT_TASK_MBLKBUF
  #endif
  #if (OX_STACK_CHECK)
	#define INIT_TASK_STACKLOW(x)		,(int32_t *)(void *)&(x)[0]
	#define INIT_TASK_STACKHIGH(x,y)	,((int32_t *)&(x))+(((y)/sizeof(int32_t))-1)
  #else
	#define INIT_TASK_STACKLOW(x)
	#define INIT_TASK_STACKHIGH(x,y)
  #endif
  #if ((OX_N_CORE) > 1)
	#define INIT_TASK_MY_CORE			,-1
  #else
	#define INIT_TASK_MY_CORE
  #endif
  #if (((OX_MP_TYPE) & 4U) != 0U)
	#define INIT_TASK_TARGET_CORE		,-1
  #else
	#define INIT_TASK_TARGET_CORE
  #endif

  #if (OX_PERF_MON)
	#if    ((OX_N_CORE) == 1)
	  #define	INIT_TASK_PM_RUN_CORE	{((OSperfMon_t)0)}
	#elif  ((OX_N_CORE) == 2)
	  #define	INIT_TASK_PM_RUN_CORE	{((OSperfMon_t)0),((OSperfMon_t)0)}
	#elif  ((OX_N_CORE) == 3)
	  #define	INIT_TASK_PM_RUN_CORE	{((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0)}
	#elif  ((OX_N_CORE) == 4)
	  #define	INIT_TASK_PM_RUN_CORE	{((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0),		\
										 ((OSperfMon_t)0)}
	#elif  ((OX_N_CORE) == 5)
	  #define	INIT_TASK_PM_RUN_CORE	{((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0),		\
										 ((OSperfMon_t)0),((OSperfMon_t)0)}
	#elif  ((OX_N_CORE) == 6)
	  #define	INIT_TASK_PM_RUN_CORE	{((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0),		\
										 ((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0)}
	#elif  ((OX_N_CORE) == 7)
	  #define	INIT_TASK_PM_RUN_CORE	{((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0),		\
										 ((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0),		\
										 ((OSperfMon_t)0)}
	#elif  ((OX_N_CORE) == 8)
	  #define	INIT_TASK_PM_RUN_CORE	{((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0),		\
										 ((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0),		\
										 ((OSperfMon_t)0),((OSperfMon_t)0)}
	#elif  ((OX_N_CORE) == 9)
	  #define	INIT_TASK_PM_RUN_CORE	{((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0),		\
										 ((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0),		\
										 ((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0)}
	#else
		#error "Abassi: Too many cores required"
	#endif
	#define INIT_TASK_PM_1				,INIT_TASK_PM_RUN_CORE,((OSperfMon_t)0),((OSperfMon_t)0)	\
										,((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0)			\
										,((OSperfMon_t)0),((OSperfMon_t)0)							\
										,((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0)			\
										,((OSperfMon_t)0),((OSperfMon_t)0)							\
										,((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0)			\
										,((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0)			\
										,((OSperfMon_t)0),((OSperfMon_t)0),((uint32_t)0)			\
										,((OSperfMon_t)0),((OSperfMon_t)0),((OSperfMon_t)0)			\
										,((uint32_t)0),   ((uint32_t)0),   ((uint32_t)0)
   #if (OX_EVENTS)
	#define	INIT_TASK_PM_EVT			,((uint32_t)0)
   #else
	#define INIT_TASK_PM_EVT
   #endif
   #if (OX_GROUP)
	#define	INIT_TASK_PM_GRP			,((uint32_t)0)
   #else
	#define INIT_TASK_PM_GRP
   #endif
   #if (OX_MAILBOX)
	#define	INIT_TASK_PM_MBX			,((uint32_t)0)
   #else
	#define INIT_TASK_PM_MBX
   #endif
   #if (OX_MEM_BLOCK)
	#define	INIT_TASK_PM_MBLK			,((uint32_t)0)
   #else
	#define INIT_TASK_PM_MBLK
   #endif
   #if (OX_STARVE_WAIT_MAX)
	#define	INIT_TASK_PM_STRV			,((uint32_t)0),((uint32_t)0,((uint32_t)0
   #else
	#define INIT_TASK_PM_STRV
   #endif
   #if (OX_MTX_INVERSION)
	#define	INIT_TASK_PM_INV			,((uint32_t)0)
   #else
	#define INIT_TASK_PM_INV
   #endif
	#define INIT_TASK_PM_2				,((uint32_t)0),   ((uint32_t)0)

  #else
	#define INIT_TASK_PM_1
	#define INIT_TASK_PM_EVT
	#define INIT_TASK_PM_GRP
	#define INIT_TASK_PM_MBX
	#define INIT_TASK_PM_MBLK
	#define INIT_TASK_PM_STRV
	#define INIT_TASK_PM_INV
	#define INIT_TASK_PM_INV
	#define INIT_TASK_PM_2
  #endif
  #if (OX_C_PLUS_PLUS)
	#define INIT_TASK_CPP_CLASS			,((void *)NULL)
  #else
	#define INIT_TASK_CPP_CLASS
  #endif
  #if ((OX_SYS_CALL) && (OX_SYS_CALL_SIZE))
   #if ((OX_SYS_CALL_SIZE) == 1)
	#define INIT_TASK_SYS_CALL			,{((intptr_t)0)}
   #elif ((OX_SYS_CALL_SIZE) == 2)
	#define INIT_TASK_SYS_CALL			,{((intptr_t)0) ,((intptr_t)0)}
   #elif ((OX_SYS_CALL_SIZE) == 3)
	#define INIT_TASK_SYS_CALL			,{((intptr_t)0) ,((intptr_t)0) ,((intptr_t)0)}
   #elif ((OX_SYS_CALL_SIZE) == 4)
	#define INIT_TASK_SYS_CALL			,{((intptr_t)0) ,((intptr_t)0) ,((intptr_t)0) ,((intptr_t)0)}
   #else
	#error "OS_SYS_CALL : add more defines in INIT_TASK_SYS_CALL"
   #endif
  #else
	#define INIT_TASK_SYS_CALL
  #endif
  #if (OX_TASK_PARKING)
	#define INIT_TASK_PARKING			,((void *)NULL)
  #else
	#define INIT_TASK_PARKING
  #endif

												/* We put stacksize in TSK_t.Priv.Value				*/
  #define TSK_STATIC(__Tsk, __Name, __Prio, __StackSize, __Fct)										\
   static OSalign_t __Tsk##__stack[((__StackSize)+sizeof(OSalign_t)-1)/sizeof(OSalign_t)];			\
   static _sTSK_t __Tsk##_s_ = {		\
   {									\
   (void *)&__Tsk##__stack[0]			\
   INIT_TASK_XTRADATA					\
   ,{									\
	 (TSK_t *)&__Tsk##_s_.Tsk			\
	,(TSK_t *)NULL						\
	INIT_SEM_MTXNEXT					\
	INIT_SEM_NAME(&G_OSprivName[0])		\
	INIT_SEM_SEMANEXT					\
	,((int)sizeof(OSalign_t)*(((__StackSize)+(int)sizeof(OSalign_t)-1)/(int)sizeof(OSalign_t)))		\
	INIT_SEM_ISFCFS						\
	INIT_SEM_CEILPRIO					\
	INIT_SEM_PRIO_INV_ON				\
	INIT_SEM_IGNORE_OWN					\
	INIT_SEM_GRP_OWNER					\
	INIT_SEM_CPP_SEM_REF				\
	INIT_SEM_PARKING					\
	INIT_SEM_XTRADATA					\
   }									\
   ,(SEM_t *)&__Tsk##_s_.Tsk.Priv		\
   ,(TSK_t *)NULL						\
   ,(TSK_t *)NULL						\
   INIT_TASK_LAST						\
   INIT_TASK_NAME(__Name)				\
   INIT_TASK_TASKNEXT					\
   INIT_TASK_MYMUTEX					\
   INIT_TASK_TASKARG					\
   INIT_TASK_PRIONEXT					\
   INIT_TASK_PRIOPREV					\
   INIT_TASK_STARVNEXT					\
   INIT_TASK_STARVPREV					\
   INIT_TASK_STARVLAST					\
   INIT_TASK_STARVSTATE					\
   INIT_TASK_STARVPRIO(__Prio)			\
   INIT_TASK_STARVPRIOMAX				\
   INIT_TASK_STARVRUNMAX				\
   INIT_TASK_STARVWAITMAX				\
   INIT_TASK_TOUTNEXT					\
   INIT_TASK_TOUTPREV					\
   INIT_TASK_TICKTIME					\
   INIT_TASK_PREVRETVAL					\
   ,(__Prio)							\
   INIT_TASK_SUSPRQST					\
   INIT_TASK_RRCNT						\
   INIT_TASK_RRTICK						\
   INIT_TASK_RRMAX						\
   INIT_TASK_YIELD						\
   INIT_TASK_ORIPRIO(__Prio)			\
   INIT_TASK_EVENTACC					\
   INIT_TASK_EVENTRX					\
   INIT_TASK_EVENTAND					\
   INIT_TASK_EVENTOR					\
   INIT_TASK_GRP_BLK_ON					\
   INIT_TACK_GRP_CB_FCT					\
   INIT_TASK_GRP_CB_ARG					\
   INIT_TASK_MBOXMSG					\
   INIT_TASK_MBLKBUF					\
   INIT_TASK_STACKLOW(__Tsk##__stack)	\
   INIT_TASK_STACKHIGH((__Tsk##__stack), (__StackSize))	\
   INIT_TASK_MY_CORE					\
   INIT_TASK_TARGET_CORE				\
   INIT_TASK_PM_1						\
   INIT_TASK_PM_EVT						\
   INIT_TASK_PM_GRP						\
   INIT_TASK_PM_MBX						\
   INIT_TASK_PM_MBLK					\
   INIT_TASK_PM_STRV					\
   INIT_TASK_PM_INV						\
   INIT_TASK_PM_2						\
   INIT_TASK_CPP_CLASS					\
   INIT_TASK_SYS_CALL					\
   INIT_TASK_PARKING					\
   },									\
   (__Fct)								\
  };									\
  TSK_t *(__Tsk) = &__Tsk##_s_.Tsk

  #if (OX_STACK_CHECK)
   #define TSK_SC_SETUP(__Tsk)												\
	do {																	\
	  int      _ii;															\
	  int      _jj;															\
	  int32_t *Stack32;														\
		_jj     = (__Tsk)->StackHigh-(__Tsk)->StackLow;						\
		Stack32 = (int32_t *)((__Tsk)->StackLow);							\
		for (_ii=0 ; _ii<=_jj ; _ii++) {									\
			Stack32[_ii] = OX_STACK_MAGIC;									\
		}																	\
	} while(0)
  #else
   #define TSK_SC_SETUP(__Tsk)			do{__Tsk=__Tsk;} while(0)
  #endif

  #ifdef OX_TASK_XTRA
   #define TSK_XTRA_SETUP(_Tsk_, _Fct_)	OX_TASK_XTRA(_Tsk_, _Fct_)
  #else
   #define TSK_XTRA_SETUP(_Tsk_, _Fct_)	do{_Tsk_=_Tsk_;} while(0)
  #endif

  #ifndef __CC_ARM									/* Keil at -O2 & -O3 generates bad code			*/
   #define TSK_SETUP(_Tsk, _State)												\
	do {																		\
	  void (*Fct)(void);														\
		Fct = ((_sTSK_t *)(_Tsk))->TskFct;										\
		TSK_SC_SETUP(_Tsk);														\
		(_Tsk)->Stack      = OSstack((_Tsk)->Stack, (_Tsk)->Priv.Value, Fct);	\
		(_Tsk)->Priv.Value = 0;													\
		TSK_XTRA_SETUP(_Tsk, Fct);												\
		if ((_State) != 0) {													\
			(void)Abassi((ABASSI_POST), (void *)&((_Tsk)->Priv), (intptr_t)0);	\
		}																		\
	} while(0)
  #else
   #define TSK_SETUP(_Tsk, _State)												\
	do {																		\
	  void (*Fct)(void);														\
		Fct = ((_sTSK_t *)(_Tsk))->TskFct;										\
		TSK_SC_SETUP(_Tsk);														\
		(_Tsk)->Stack	     = OSstack((_Tsk)->Stack, (_Tsk)->Priv.Value, Fct);	\
		(_Tsk)->Priv.Value   = ((int)(_Tsk)->Next);		/* repair */			\
		(_Tsk)->Priv.Blocked = (_Tsk);					/* repair */			\
		TSK_XTRA_SETUP(_Tsk, Fct);												\
		if ((_State) != 0) {													\
			(void)Abassi((ABASSI_POST), (void *)&((_Tsk)->Priv), (intptr_t)0);	\
		}																		\
	} while(0)
  #endif
 #endif
#endif


#ifndef TSK_STATIC								/* This seems the best way to report unavailable	*/
  #define TSK_STATIC(_a, _b, _c, _d, _e)	TSK_STATIC_is_unavailable	/* can't use #error in here	*/
  #define TSK_SETUP(_a, _b)
#endif

/* ------------------------------------------------	*/
/* Mailbox descriptor								*/

#if (OX_MAILBOX)

  typedef struct _MBX_t {
	SEM_t     MboxSema;							/* Semaphore when no buffer in the mailbox			*/
	intptr_t *Buffer;							/* Holds the queue / mailbox elements				*/
  #if (OX_NAMES)								/* Name of the queue /  mailbox						*/
	const char    *MbxName;
	struct _MBX_t *MbxNext;
  #endif
	int       Size;								/* Number of elements in the mailbox				*/
	int       RdIdx;							/* Read index										*/
	int       WrtIdx;							/* Write index										*/
  #if ((OX_MBXPUT_ISR) != 0)					/* If MBXput needs valid return when called in ISR	*/
	int       PutInISR;							/* If MBXput need retval in ISR on this mailbox		*/
	int       Count;							/* Number of messages in the mailbox				*/
   #if ((OX_N_CORE) > 1)
	int       MbxSpin;							/* Spinlock needed when mailbox writing in ISR		*/
   #endif										/* but only for multi-core							*/
  #endif
  #if (OX_GROUP)								/* When supporting groups, this is the pointer to	*/
	struct _GRP_t *GrpOwner;					/* the group descriptor this mailbox is part of		*/
  #endif
  #if (OX_MBX_PARKING)							/* When "destroy mailbox" is needed					*/
	struct _MBX_t *ParkNext; 					/* Keep the "destroyed" in a parking lot			*/
  #endif
  #if (OX_MBX_XTRA_FIELD)
	intptr_t XtraData[OX_MBX_XTRA_FIELD];
  #endif
  } MBX_t;

 #if (OX_DO_STATIC_INIT)
  #if ((OX_RUNTIME) >= 0)
   #if (OX_NAMES)
	 #define INIT_MBX_NAME(_Name)		,(_Name)
	 #define INIT_MBX_MBXNEXT			,(MBX_t *)NULL
   #else
	 #define INIT_MBX_NAME(_Name)
	 #define INIT_MBX_MBXNEXT
   #endif
   #if ((OX_MBXPUT_ISR) != 0)
	#define INIT_MBX_PUTINISR			, 0
	#define INIT_MBX_COUNT				, 0
    #if ((OX_N_CORE) > 1)
	 #define INIT_MBX_SPIN				, 0
    #else
	 #define INIT_MBX_SPIN
    #endif
   #else
	#define INIT_MBX_PUTINISR
	#define INIT_MBX_COUNT
	#define INIT_MBX_SPIN
   #endif
   #if (OX_GROUP)
	 #define INIT_MBX_GRP_OWNER			, (struct _GRP_t *) NULL
   #else
	 #define INIT_MBX_GRP_OWNER
   #endif
  #if (OX_MBX_PARKING)
	#define INIT_MBX_PARKING			,((void *)NULL)
  #else
	#define INIT_MBX_PARKING
  #endif
  #if ((OX_MBX_XTRA_FIELD) != 0)
   #if ((OX_MBX_XTRA_FIELD)   == 1)
	#define INIT_MBX_XTRADATA			,{((intptr_t)0)}
   #elif ((OX_MBX_XTRA_FIELD) == 2)
	#define INIT_MBX_XTRADATA			,{((intptr_t)0),((intptr_t)0)}
   #elif ((OX_MBX_XTRA_FIELD) == 3)
	#define INIT_MBX_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_MBX_XTRA_FIELD) == 4)
	#define INIT_MBX_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_MBX_XTRA_FIELD) == 5)
	#define INIT_MBX_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0)}
   #elif ((OX_MBX_XTRA_FIELD) == 6)
	#define INIT_MBX_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0)}
   #elif ((OX_MBX_XTRA_FIELD) == 7)
	#define INIT_MBX_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_MBX_XTRA_FIELD) == 8)
	#define INIT_MBX_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0)}
   #elif ((OX_MBX_XTRA_FIELD) == 9)
	#define INIT_MBX_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
	                                      ((intptr_t)0)}
   #elif ((OX_MBX_XTRA_FIELD) == 10)
	#define INIT_MBX_XTRADATA			,{((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0),((intptr_t)0),((intptr_t)0),	\
										  ((intptr_t)0),((intptr_t)0)}
   #else
	#error "Too many OS_MBX_XTRA_FIELD requested; mAbassi.h must be updated / modified"
   #endif
  #else
	#define INIT_MBX_XTRADATA
  #endif

   #define MBX_STATIC(__Var, __Name, __Size)	\
    static intptr_t __Var##__buf[(__Size)+1];	\
    MBX_t __Var##__ = {					\
    {									\
	  (TSK_t *)NULL						\
	 ,(TSK_t *)NULL						\
	 INIT_SEM_MTXNEXT					\
	 INIT_SEM_NAME(__Name)				\
	 INIT_SEM_SEMANEXT					\
	 ,0									\
	 INIT_SEM_ISFCFS					\
	 INIT_SEM_CEILPRIO					\
	 INIT_SEM_PRIO_INV_ON				\
	 INIT_SEM_IGNORE_OWN				\
	 INIT_SEM_GRP_OWNER					\
	 INIT_SEM_CPP_SEM_REF				\
	 INIT_SEM_PARKING					\
	 INIT_SEM_XTRADATA					\
    },									\
    &(__Var##__buf[0])					\
    INIT_MBX_NAME(__Name)				\
    INIT_MBX_MBXNEXT					\
    ,(__Size)							\
    ,0									\
    ,(__Size)							\
	INIT_MBX_PUTINISR					\
	INIT_MBX_COUNT						\
	INIT_MBX_SPIN						\
    INIT_MBX_GRP_OWNER					\
	INIT_MBX_PARKING					\
	INIT_MBX_XTRADATA					\
    };									\
    MBX_t *(__Var) = &(__Var##__)
  #endif
 #endif
#endif
#ifndef MBX_STATIC								/* This seems the best way to report unavailable	*/
  #define MBX_STATIC(_a, _b, _c)	MBX_STATIC_is_unavailable		/* because can't use #error here*/
#endif

/* ------------------------------------------------	*/
/* Memory block descriptor							*/

#if (OX_MEM_BLOCK)

  typedef struct _MBLK_t {
	SEM_t      MblkSema;						/* Semaphore when no buffers are left in the pool	*/
	OSalign_t *Block;							/* Holds the head of the memory block linked list	*/
  #if ((OX_N_CORE) > 1)
	int        MblkSpin;						/* Spinlock needed because ISR vs background		*/
  #endif										/* On single core, no need for this spinlock		*/
  #if (OX_NAMES)
	const char     *MblkName;					/* Name of the memory block							*/
	struct _MBLK_t *MblkNext;					/* Next memory block in the application				*/
  #endif
  #if (OX_MBLK_PARKING)							/* When "destroy memory block" is needed			*/
	struct _MBLK_t *ParkNext; 					/* Keep the "destroyed" in a parking lot			*/
  #endif
  #if (OX_MBLK_XTRA_FIELD)
	intptr_t XtraData[OX_MBLK_XTRA_FIELD];
  #endif
  } MBLK_t;

#endif

/* ------------------------------------------------	*/
/* Group descriptor									*/

#if (OX_GROUP)

  typedef struct _GRP_t {
	struct _GRP_t *GrpNext;						/* Next trigger group descriptor					*/
	struct _GRP_t *GrpPrev;						/* Previous trigger group descriptor				*/
	struct _GRP_t *GrpHead;						/* Head of the linked list (group desc to use)		*/
	TSK_t         *TaskOwner;					/* Task descriptor when blocked on this group		*/
	int            TrigCnt;						/* Counter of how many valid triggers are available	*/
	void          *Trigger;						/* Pointer to the trigger descriptor				*/
	int            TrigType;					/* Type of trigger									*/
	void (* CBfct)(intptr_t CBarg);				/* Call Back function								*/
  #if (OX_C_PLUS_PLUS)
	void          *CppGrpRef;					/* Pointer to associated AbassiCPP::group_t object	*/
  #endif
  #if (OX_GRP_PARKING)							/* When "destroy group" is needed					*/
	struct _GRP_t *ParkNext; 					/* Keep the "destroyed" in a parking lot			*/
  #endif
  #if (OX_GRP_XTRA_FIELD)
	intptr_t XtraData[OX_GRP_XTRA_FIELD];
  #endif
  } GRP_t;

#endif

/* ------------------------------------------------	*/
/* Kernel requests bit masks (must fit in 16 bits)	*/
												/* MISRA has issues with (1U<<XX)					*/
#define ABASSI_BIN         (0x0001U)			/* Binary semaphore & extra tag to some commands	*/
#define ABASSI_WAIT        (0x0002U)			/* Regular semaphore waiting / mutex unlocking		*/
#define ABASSI_MTX         (0x0004U)			/* Mutex priority inheritance checking				*/
#define ABASSI_POST        (0x0008U)			/* Semaphore posting / mutex unlocking				*/
#define ABASSI_TOUT_RM     (0x0010U)			/* Remove from timeout linked list					*/
#define ABASSI_TOUT_ADD    (0x0020U)			/* Add in the timeout list							*/
#define ABASSI_TOUT_CHK    (0x0040U)			/* Check timer expiry's								*/
#define ABASSI_ADD_BLK     (0x0080U)			/* Add the task to the blocked list					*/
#define ABASSI_ADD_RUN     (0x0100U)			/* Add the task to the ready to run list			*/
#define ABASSI_NEW_PRIO    (0x0200U)			/* Set the task to a new priority					*/
#define ABASSI_EVENT       (0x0400U)			/* Event operation									*/
#define ABASSI_MBX_PUT     (0x0800U)			/* Put a mailbox message							*/
#define ABASSI_MBX_GET     (0x1000U)			/* Get a mailbox message							*/
#define ABASSI_RESUSP      (0x2000U)			/* Suspend or resume a task							*/
#define ABASSI_STARVE      (0x4000U)			/* Process starving (prio aging) protection			*/
#define ABASSI_TIM_SRV     (0x8000U)			/* Operation on timer services						*/
#define ABASSI_LOG         (0xFFFFU)			/* Only used to send logging message				*/

#define ABASSI_MBLK        (0x0180U)
#define ABASSI_MBLK_MASK   (~1)					/* Mask for any of the memory operations			*/
#define ABASSI_MBLK_GET	   (0x0180U)			/* Combines ABASSI_ADD_BLK & ABASSI_ADD_RUN			*/
#define ABASSI_MBLK_PUT	   (0x0181U)			/* This one + ABASSI_BIN							*/

#define ABASSI_GRP         (0xC000U)
#define ABASSI_GRP_MASK    (~3)					/* Mask for any of the group operations				*/
#define ABASSI_GRP_RM      (0xC000U)			/* Combines ABASSI_STARVE & ABASSI_TIM_SRV			*/
#define ABASSI_GRP_ADD     (0xC001U)			/* This one + ABASSI_BIN							*/
#define ABASSI_GRP_WAIT    (0xC002U)			/* This one + ABASSI_WAIT							*/
#define ABASSI_GRP_DETACH  (0xC003U)			/* This one + ABASSI_WAIT + ABASSI_BIN				*/
#define ABASSI_GRP_RQST    (ABASSI_STARVE)		/* For special handling when group request			*/

#define ABASSI_TOUT_TSK    (0xB000U)			/* Update the time out a task is blocked on			*/

#define ABASSI_ABRT        (0xA000U)
#define ABASSI_ABRT_MASK   (~7)					/* Mask for any abort operations					*/
#define ABASSI_ABRT_EVT    (0xA001U)			/* Abort waiting of a task blocked on its events	*/
#define ABASSI_ABRT_MBX    (0xA002U)			/* Abort waiting of all tasks blocked on a mailbox	*/
#define ABASSI_ABRT_MTX    (0xA003U)			/* Abort waiting of all tasks blocked on a mutex	*/
#define ABASSI_ABRT_SEM    (0xA004U)			/* Abort waiting of all tasks blocked on a semaphore*/

#define ABASSI_TIM_RM      (ABASSI_BIN)			/* Added to timer service request to remove a timer	*/
#define ABASSI_TIM_ADD     (ABASSI_MTX)			/* Added to timer service request to add a timer	*/
#define ABASSI_TIM_FRZ     (ABASSI_WAIT)		/* Added to timer service request to freeze a timer	*/
#define ABASSI_TOUT_RM0    (ABASSI_BIN)			/* Remove from timeout list + zero TimedOut			*/
#define ABASSI_NEW_O_PRIO  (ABASSI_BIN)			/* New prio + set original							*/
#define ABASSI_SUSPEND     (ABASSI_BIN)			/* For ABASSI_RESUSP this means to suspend			*/

/* ------------------------------------------------	*/
/* Global variables									*/

#if ((OX_N_CORE) > 1)
  extern volatile        int G_CoreIdleDone[OX_N_CORE];
#endif
extern volatile unsigned int G_OSisrState[7][OX_N_CORE];	/* ISR state for each core				*/
extern volatile unsigned int G_OSstate;			/* Kernel state: !=0 is Core#+1 using the kernel	*/

#if ((OX_N_INTERRUPTS) > 0)
  extern        void (*G_OSisrTbl[OX_N_INTERRUPTS])(void) REENT_POST;	/* Table used by ISRdispatch*/
#else
  extern        void (*G_OSisrTbl[1])(void) REENT_POST;	/* Imported/sized in ASM file, dummy size	*/
#endif
extern OSstrCnst_t     G_OSprivName[5];				/* Name for a task's private semaphore			*/
extern OSstrCnst_t     G_OSnoName[4];				/* Name for un-named components					*/
extern          TSK_t *G_OStaskNow[OX_N_CORE];		/* Currently running tasks on each core			*/
extern volatile int    G_OStimCnt;					/* RTOS time base counter tick					*/
extern          MTX_t *G_OSmutex;				    /* OS needs a mutex for some service creation	*/

/* ------------------------------------------------	*/
/* Atomic function prototypes & other components	*/

extern int      Abassi      (unsigned int Ops, void *Ptr, intptr_t Arg) REENT_POST;
#if ((OX_IDLE_STACK) != 0)
  extern TSK_t *TSKidle;
  extern void   IdleTask	(void) REENT_POST;
#endif
#if (OX_MAILBOX)
 #if (OX_RUNTIME)
  extern MBX_t *MBX__open   (const char *Name, int MbxSize, unsigned int IsFCFS) REENT_POST;
 #endif
  extern int MBXget			(MBX_t *Mbx,     intptr_t *Msg, int Tout) REENT_POST;
  extern int MBXput			(MBX_t *Mailbox, intptr_t  Msg, int Tout) REENT_POST;
  #define MBXavail(_Mbx)	((((_Mbx)->RdIdx -(_Mbx)->WrtIdx) <= 0)									\
							? -((_Mbx)->RdIdx-(_Mbx)->WrtIdx)										\
							: ((_Mbx)->Size+1-((_Mbx)->RdIdx -(_Mbx)->WrtIdx)))
  #define MBXused(_Mbx)		((((_Mbx)->RdIdx-(_Mbx)->WrtIdx) <= 0)									\
							? ((_Mbx)->Size+((_Mbx)->RdIdx-(_Mbx)->WrtIdx))							\
							: (((_Mbx)->RdIdx-(_Mbx)->WrtIdx)-1))
#endif
#if ((OX_PERF_MON) != 0)
  extern OSperfMon_t OSperfMonTimeGet	(void);
  extern void        OSperfMonTimeInit	(void);

  #if ((OX_PERF_MON) != 0x7FFFFFFFL)
	#if ((OX_PERF_MON) > 0)
	  #define OX_PERF_MON_TIME()		(G_OStimCnt)
	#else
	  #define OX_PERF_MON_TIME()		OSperfMonTimeGet()
	#endif
  #endif
#endif
#if ((OX_PRE_CONTEXT) != 0)
  extern void OSpreContext(TSK_t *TaskNext, TSK_t *TaskNow);
#endif

extern int  COREreentOK(void);
extern int  COREgetID  (void) REENT_POST;
extern void CORElock   (int LockNmb, volatile void *Address, int WaitFor, int WrtVal) REENT_POST;
extern void COREunlock (int LockNmb, volatile void *Address, int WrtVal) REENT_POST;
extern void SPINlock(void);
extern int  SPINtrylock(void);
extern void SPINunlock(void);

#if ((OX_N_CORE) != 1)
  extern TSK_t *COREtaskNow(void) REENT_POST;
#else
  #define COREtaskNow()			G_OStaskNow[0]
#endif
#if ((OX_MEM_BLOCK) && (OX_RUNTIME))
  extern MBLK_t *MBLK__open	(const char *Name, int Nblock, int BlkSize, int MblkIsFCFS) REENT_POST;
  extern void   *MBLKalloc  (MBLK_t *MemBlk, int TimOut) REENT_POST;
#endif
extern void    *OSalloc     (size_t MemSize) REENT_POST;
#if ((OX_ALLOC_SIZE) != 0)
  extern int G_OSallocIdxPool;
  #define OSallocAvail()	(G_OSallocIdxPool * sizeof(OSalign_t))
#endif
extern void     OScontext   (TSK_t *NewTask, TSK_t **TaskNowPtr) REENT_POST;
extern void     OSinvalidISR(void) REENT_POST;
extern int      OSisrInfo   (void) REENT_POST;
extern void    *OSstack     (void *StackBase, int StackSize, void (*TskFct)(void) REENT_POST)
                             REENT_POST;
extern void     OSstart     (void);
#ifndef OSnoTask
  extern void   OSnoTask    (void) REENT_POST;
#endif

extern  int			       OSintCtrl(int State);/* Function that deals with both dint() and eint()	*/
#define OSdint()		  (OSintCtrl(0))		/* Could split into 2 functions but only a few		*/
#define OSeint(x)	((void)OSintCtrl(x))		/* CPU cycles are added on OSdint(), not OSeint()	*/

extern void OSintBack(int ISRstate);
extern int  OSintOff(void);
extern void OSintOn(void);

#if (OX_RUNTIME)
  extern SEM_t *SEM__open   (const char *Name, unsigned int Type) REENT_POST;
  extern TSK_t *TSKcreate   (const char *Name, int TskPrio,int StackSize,void (*Fct)(void) REENT_POST,
                             int State, ...) REENT_POST;
#endif
extern void     TIMcallBack (void) REENT_POST;
#if ((OX_NAMES) && (OX_RUNTIME))
  extern TSK_t *TSKgetID	(const char *Name) REENT_POST;
#endif
extern void     TSKstart    (void (*TaskFct)(void)) REENT_POST;

												/* Core Idle tasks. Declared here in case the		*/
extern void COREstart0 (void) REENT_POST;		/* application supplies one or more. It sets the	*/
extern void COREstart1 (void) REENT_POST;		/* function prototype for the application			*/
extern void COREstart2 (void) REENT_POST;
extern void COREstart3 (void) REENT_POST;
extern void COREstart4 (void) REENT_POST;
extern void COREstart5 (void) REENT_POST;
extern void COREstart6 (void) REENT_POST;
extern void COREstart7 (void) REENT_POST;
extern void COREstart8 (void) REENT_POST;
extern void COREstart9 (void) REENT_POST;
extern void COREstart10(void) REENT_POST;
extern void COREstart11(void) REENT_POST;
extern void COREstart12(void) REENT_POST;
extern void COREstart13(void) REENT_POST;
extern void COREstart14(void) REENT_POST;
extern void COREstart15(void) REENT_POST;
extern void COREstart16(void) REENT_POST;
extern void COREstart17(void) REENT_POST;
extern void COREstart18(void) REENT_POST;
extern void COREstart19(void) REENT_POST;
extern void COREstart20(void) REENT_POST;
extern void COREstart21(void) REENT_POST;
extern void COREstart22(void) REENT_POST;
extern void COREstart23(void) REENT_POST;
extern void COREstart24(void) REENT_POST;
extern void COREstart25(void) REENT_POST;
extern void COREstart26(void) REENT_POST;
extern void COREstart27(void) REENT_POST;
extern void COREstart28(void) REENT_POST;
extern void COREstart29(void) REENT_POST;
extern void COREstart30(void) REENT_POST;
extern void COREstart31(void) REENT_POST;
#if ((OX_N_CORE) > 32)
	#error "Abassi: Too many cores required"
	#error "        Contact Code Time Technologies to increase the maximum"
#endif

/* ------------------------------------------------	*/
/* Function defines									*/

#if (((OX_MP_TYPE) & 6U) != 0U)					/* These are only usable here. If they were used by	*/
  extern unsigned int  COREisInISR   (void) REENT_POST;
#else
  #define COREisInISR()		G_OSisrState[OX_ISR_STATE_IN_ISR][0]
#endif

#if ((OX_N_INTERRUPTS) != 0)
  #define OSisrInstall(Nmb, Fct)	(G_OSisrTbl[(Nmb)+(OX_ISR_TBL_OFFSET)]=(Fct))
#endif

typedef void TSKfct_t(void);						/* Useful for C++ when testing					*/
extern  void OSshell(void);

/* ------------------------------------------------	*/
/* Events related macros							*/

#if (OX_EVENTS)
  #define EVTget()					(TSKmyID()->EventRX)
  #define EVTgetAcc()				(TSKmyID()->EventAcc)
  #define EVTreset()				(TSKmyID()->EventRX=0U)
  #define EVTresetAcc()				(TSKmyID()->EventAcc=0U)
  #define EVTset(_Tsk, _Bits)		((void)Abassi((ABASSI_EVENT),(void *)(_Tsk),(intptr_t)(_Bits)))
  #define EVTwait(_And,_Or,_Tout)	(TSKmyID()->Priv.Value = 0,										\
									 TSKmyID()->EventOR    = (_Or),									\
									 TSKmyID()->EventAND   = ((_And) == 0U)							\
									                       ? (_Or)									\
									                       : (_And),								\
									 Abassi(((ABASSI_WAIT)|(ABASSI_EVENT)), 						\
										    (void *)&(TSKmyID()->Priv), (intptr_t)(_Tout)))
#endif


/* ------------------------------------------------	*/
/* Group related macros								*/

#if (OX_GROUP)

  #define OS_GRP_SEM		0						/* Trigger types attached to a GRP_t			*/
  #define OS_GRP_SEM_BIN	1
  #define OS_GRP_MBX		2

  GRP_t *GroupAdd     (GRP_t *GrpHead, void *Service, void (* CallBack)(intptr_t CBarg), int Type);
  int    GroupRm      (GRP_t *Grp2rm, int RmAll);
  int    GRPwait      (GRP_t *Group, int TimeOut, int All);

 #if (OX_MAILBOX)
  #define GRPaddMBX(_Grp,_Mbx,_Fct)		GroupAdd((_Grp),(void *)(_Mbx),(void (*)(intptr_t))(_Fct),	\
                                                 OS_GRP_MBX)
  #define GRPdscMBX(_Mbx)				( ((_Mbx)->GrpOwner != (GRP_t*)NULL)						\
										 ? ((_Mbx)->GrpOwner->GrpHead) : (GRP_t *)NULL)
  #define GRPrmMBX(_Mbx)				GroupRm((_Mbx)->GrpOwner, 0)
 #endif

  #define GRPaddSEM(_Grp,_Sem,_Fct)		GroupAdd((_Grp),(void *)(_Sem),(void (*)(intptr_t))(_Fct),	\
                                                 OS_GRP_SEM)
  #define GRPaddSEMbin(_Grp,_Sem,_Fct) 	GroupAdd((_Grp),(void *)(_Sem),(void (*)(intptr_t))(_Fct),	\
                                                 OS_GRP_SEM_BIN)
  #define GRPdscSEM(_Sema)				( ((_Sema)->GrpOwner != (GRP_t*)NULL)						\
										 ? ((_Sema)->GrpOwner->GrpHead) : (GRP_t *)NULL)
  #define GRPrmSEM(_Sema)				GroupRm((_Sema)->GrpOwner, 0)
  #define GRPrmAll(_Grp)				GroupRm((_Grp), 1)

#endif


/* ------------------------------------------------	*/
/* Mailboxes related macros							*/

#if (OX_MAILBOX)
  #if (OX_RUNTIME)
	#define MBXopen(_Name, _Size)		(MBX__open((_Name), (_Size), 0U))
   #if (OX_FCFS)
	#define MBXopenFCFS(_Name, _Size)	(MBX__open((_Name), (_Size), 1U))
   #endif
  #endif

  #if (OX_FCFS)
	#define MBXsetFCFS(_Mbox)			((_Mbox)->MboxSema.IsFCFS=1)
	#define MBXnotFCFS(_Mbox)			((_Mbox)->MboxSema.IsFCFS=0)
  #endif

  #if ((OX_MBXPUT_ISR) != 0)
	#define MBXputInISR(_Mbox)			((_Mbox)->PutInISR=1)
  #endif
#endif


/* ------------------------------------------------	*/
/* Memory block related macros						*/

#if (OX_MEM_BLOCK)
	#define MBLKfree(__Mblk, _Buf)		(Abassi(ABASSI_MBLK_PUT, (__Mblk), (intptr_t)(_Buf)))

  #if (OX_RUNTIME)
	#define MBLKopen(_Nm, _Nbk, _Sz)	(MBLK__open((_Nm), (_Nbk), (_Sz), 0U))
   #if (OX_FCFS)
	#define MBLKopenFCFS(_Nm,_Nbk,_Sz)	(MBLK__open((_Nm), (_Nbk), (_Sz), 1U))
   #endif
  #endif

  #if (OX_FCFS)
	#define MBLKsetFCFS(_Mblk)			((_Mblk)->MblkSema.IsFCFS=1)
	#define MBLKnotFCFS(_Mblk)			((_Mblk)->MblkSema.IsFCFS=0)
  #endif

#endif


/* ------------------------------------------------	*/
/* Mutexes related macros							*/

#if (OX_RUNTIME)
  #define MTXopen(_Name)		(SEM__open((_Name), 1U))
#endif
#define MTXowner(_Mtx)			((_Mtx)->MtxOwner)
#define MTXlock(_Mtx, _Tout)	(Abassi(((ABASSI_WAIT)|(ABASSI_MTX)|(ABASSI_BIN)), (void *)(_Mtx),  \
								        (intptr_t)(_Tout)))
#define MTXunlock(_Mtx)			(Abassi(((ABASSI_POST)|(ABASSI_MTX)), (void *)(_Mtx), (intptr_t)0))
#if (OX_FCFS)
 #if (OX_RUNTIME)
  #define MTXopenFCFS(_Name)	(SEM__open((_Name), 3U))
 #endif
  #define MTXsetFCFS(_Mtx)		((_Mtx)->IsFCFS=1)
  #define MTXnotFCFS(_Mtx)		((_Mtx)->IsFCFS=0)
#endif
#if ((OX_MTX_INVERSION) < 0)
  #define MTXgetCeilPrio(_Mtx)			(((volatile MTX_t *)(_Mtx))->CeilPrio)
  #define MTXsetCeilPrio(_Mtx, _Prio)	((_Mtx)->CeilPrio=(_Prio))
#endif
#if (OX_PRIO_INV_ON_OFF)
  #define MTXprioInvOff(_Mtx)	((_Mtx)->PrioInvOn=0)
  #define MTXprioInvOn(_Mtx)	((_Mtx)->PrioInvOn=1)
  #define MTXgetPrioInv(_Mtx)	((_Mtx)->PrioInvOn)
#endif
#if ((OX_MTX_OWN_UNLOCK) < 0)
  #define MTXcheckOwn(_Mtx)		((_Mtx)->IgnoreOwn=0)
  #define MTXignoreOwn(_Mtx)	((_Mtx)->IgnoreOwn=1)
  #define MTXisChkOwn(_Mtx)		((_Mtx)->IgnoreOwn==0)
#endif
													/* Internal use: for ISRs disabled at start-up	*/
#define MTXsafeLock(_Mtx)		do {int _=0;_=_; } while (0 != MTXlock(_Mtx, -(OSisrInfo()!=0)))


/* ------------------------------------------------	*/
/* Semaphores related macros						*/

#if (OX_RUNTIME)
  #define SEMopen(_Name)		(SEM__open((_Name), 0U))
#endif
#define SEMpost(_Sem)			((void)Abassi((ABASSI_POST), (void *)(_Sem), (intptr_t)0))
#define SEMreset(_Sem)			SEMwaitBin(_Sem, 0)
#define SEMwait(_Sem, _Tout)	(Abassi((ABASSI_WAIT), (void *)(_Sem), (intptr_t)(_Tout)))
#define SEMwaitBin(_Sem, _Tout)	(Abassi(((ABASSI_WAIT)|(ABASSI_BIN)), (void *)(_Sem),				\
								        (intptr_t)(_Tout)))
#if (OX_FCFS)
 #if (OX_RUNTIME)
  #define SEMopenFCFS(_Name)	(SEM__open((_Name), 2U))
 #endif
  #define SEMsetFCFS(_Sema)		((_Sema)->IsFCFS=1)
  #define SEMnotFCFS(_Sema)		((_Sema)->IsFCFS=0)
#endif


/* ------------------------------------------------	*/
/* Timer Services related macros					*/

#if (OX_TIMER_SRV)
  #if (OX_RUNTIME)
	extern  TIM_t *TIMopen(const char *Name) REENT_POST;
  #endif

	#define TIM_CALL_FCT				((0xFFFFU) & ~(ABASSI_TIM_SRV))
 	#define TIM_WRITE_DATA				((0xEEEEU) & ~(ABASSI_TIM_SRV))

	#define TIMarg(_Timer, _Arg)		((void)((_Timer)->Arg   = (intptr_t)(_Arg)))
	#define TIMtoAdd(_Timer, _ToAdd)	((void)((_Timer)->ToAdd = (int)(_ToAdd)))
	#define TIMkill(_Timer)				((void)Abassi(((ABASSI_TIM_SRV)|(ABASSI_TIM_RM)),			\
										              (void *)(_Timer), (intptr_t)0))
	#define TIMleft(_Timer)				(((_Timer)->TmPrev != (TIM_t *)NULL)						\
										        ? ((_Timer)->TickDone - G_OStimCnt)					\
										        : (((_Timer)->Paused < 0)							\
										           ? (_Timer)->Expiry								\
										           : -1))
	#define TIMadj(_Timer, _Xtra)		do {														\
										  int _ii;													\
										  int _jj;													\
										  int _xx = _Xtra;											\
											_ii = OSintOff();										\
											_jj = (_Timer)->Paused;									\
											(_Timer)->Paused |= 1;									\
											if ((_Timer)->TmPrev != (TIM_t *)NULL) {				\
												TIMfreeze(_Timer);									\
												Abassi(((ABASSI_TIM_SRV)|(ABASSI_TIM_ADD)),			\
												       (void *)( _Timer),							\
					             				       (intptr_t)((_Timer)->Expiry+_xx));			\
												(_Timer)->Paused = _jj;								\
											}														\
											else {													\
												if ((_Timer)->Paused < 0) {							\
													if (((_Timer)->Expiry+_xx) > 0) {				\
														(_Timer)->Expiry += _xx;					\
													}												\
													else {											\
														(_Timer)->Expiry = 1;						\
													}												\
												}													\
											}														\
											OSintBack(_ii);											\
										} while (0)
	#define TIMfreeze(_Timer)			((void)Abassi(((ABASSI_TIM_SRV)|(ABASSI_TIM_FRZ)),			\
										              (void *)(_Timer), (intptr_t)0))
	#define TIMpause(_Timer)			((void)((_Timer)->Paused |= 1))
	#define TIMperiod(_Timer, _Period)	((void)((_Timer)->Period = (_Period)))
	#define TIMresume(_Timer)			do {														\
											if ((_Timer)->Paused > 0) {								\
												(_Timer)->Paused = 0;								\
											}														\
											else if ((_Timer)->Paused < 0) {						\
												Abassi(((ABASSI_TIM_SRV)|(ABASSI_TIM_ADD)),			\
												       (void *)(_Timer),							\
					             				       (intptr_t)((_Timer)->Expiry));				\
											}														\
										} while (0)
	#define TIMrestart(_Timer)			do {														\
											if (((_Timer)->TmPrev != (TIM_t *)NULL)					\
											||  ((_Timer)->Paused < 0)) {							\
												(_Timer)->Arg = (_Timer)->ArgOri;					\
												((void)Abassi(((ABASSI_TIM_SRV)|(ABASSI_TIM_ADD)),	\
									              (void *)(_Timer), (intptr_t)((_Timer)->ExpOri)));	\
											}														\
										} while (0)
	#define TIMset(_Timer, _Tout)		do {														\
										  int _ii;													\
										  int _jj;													\
											_ii = OSintOff();										\
											_jj = (_Timer)->Paused;									\
											(_Timer)->Paused |= 1;									\
											if ((_Timer)->TmPrev != (TIM_t *)NULL) {				\
												TIMfreeze(_Timer);									\
												Abassi(((ABASSI_TIM_SRV)|(ABASSI_TIM_ADD)),			\
												       (void *)( _Timer),							\
					             				       (intptr_t)(_Tout));							\
												(_Timer)->Paused = _jj;								\
											}														\
											else {													\
												if ((_Timer)->Paused < 0) {							\
													if ((_Tout) > 0) {								\
														(_Timer)->Expiry = (_Tout);					\
													}												\
													else {											\
														(_Timer)->Expiry = 1;						\
													}												\
												}													\
											}														\
											OSintBack(_ii);											\
										} while (0)

	#define TIMdata(_Timer, _Addr, _Data, _ToAdd, _Expiry, _Period)									\
				do {																				\
					(_Timer)->Ops    = (TIM_WRITE_DATA) | (ABASSI_TIM_SRV);							\
					(_Timer)->Ptr    = (void *)(_Addr);												\
					(_Timer)->Arg    = (intptr_t)(_Data);											\
					(_Timer)->ArgOri = (intptr_t)(_Data);											\
					(_Timer)->ToAdd  = (int)(_ToAdd);												\
					(_Timer)->Expiry = (int)(_Expiry); 												\
					(_Timer)->ExpOri = (int)(_Expiry); 												\
					(_Timer)->Period = (int)(_Period); 												\
					(void)Abassi(((ABASSI_TIM_SRV)|(ABASSI_TIM_ADD)),(void *)( _Timer),				\
					             (intptr_t)(_Expiry));												\
				} while(0)

  #if (OX_EVENTS)
	#define TIMevt(_Timer, _Tsk, _Data, _Expiry, _Period)											\
				do {																				\
					(_Timer)->Ops    = (ABASSI_EVENT) | (ABASSI_TIM_SRV);							\
					(_Timer)->Ptr    = (void *)(_Tsk);												\
					(_Timer)->Arg    = (intptr_t)(_Data);											\
					(_Timer)->ArgOri = (intptr_t)(_Data);											\
					(_Timer)->ToAdd  = 0;															\
					(_Timer)->Expiry = (int)(_Expiry); 												\
					(_Timer)->ExpOri = (int)(_Expiry); 												\
					(_Timer)->Period = (int)(_Period);												\
					(void)Abassi(((ABASSI_TIM_SRV)|(ABASSI_TIM_ADD)), (void *)(_Timer),				\
					             (intptr_t)(_Expiry));												\
				} while(0)
  #endif

	#define TIMfct(_Timer, _Fct, _Arg, _ToAdd, _Expiry, _Period)									\
				do {																				\
					(_Timer)->Ops    = (TIM_CALL_FCT) | (ABASSI_TIM_SRV);							\
					(_Timer)->FctPtr = (_Fct);														\
					(_Timer)->Arg    = (intptr_t)(_Arg);											\
					(_Timer)->ArgOri = (intptr_t)(_Data);											\
					(_Timer)->ToAdd  = (int)(_ToAdd);												\
					(_Timer)->Expiry = (int)(_Expiry); 												\
					(_Timer)->ExpOri = (int)(_Expiry); 												\
					(_Timer)->Period = (int)(_Period);												\
					(void)Abassi(((ABASSI_TIM_SRV)|(ABASSI_TIM_ADD)), (void *)(_Timer),				\
					             (intptr_t)(_Expiry));												\
				} while(0)

  #if (OX_MAILBOX)
	#define TIMmbx(_Timer, _Mbx, _Data, _ToAdd, _Expiry, _Period)									\
				do {																				\
					(_Timer)->Ops    = (ABASSI_MBX_PUT) | (ABASSI_TIM_SRV);							\
					(_Timer)->Ptr    = (void *)(_Mbx);												\
					(_Timer)->Arg    = (intptr_t)(_Data);											\
					(_Timer)->ArgOri = (intptr_t)(_Data);											\
					(_Timer)->ToAdd  = (int)(_ToAdd);												\
					(_Timer)->Expiry = (int)(_Expiry); 												\
					(_Timer)->ExpOri = (int)(_Expiry); 												\
					(_Timer)->Period = (int)(_Period);												\
					(void)Abassi(((ABASSI_TIM_SRV)|(ABASSI_TIM_ADD)), (void *)(_Timer),				\
					             (intptr_t)(_Expiry));												\
				} while(0)
  #endif

	#define TIMmtx(_Timer, _Mtx, _Expiry, _Period)													\
				do {																				\
					(_Timer)->Ops    = (ABASSI_POST) | (ABASSI_MTX) | (ABASSI_TIM_SRV);				\
					(_Timer)->Ptr    = (void *)(_Mtx);												\
					(_Timer)->Expiry = (int)(_Expiry); 												\
					(_Timer)->ExpOri = (int)(_Expiry); 												\
					(_Timer)->Period = (int)(_Period);												\
					(void)Abassi(((ABASSI_TIM_SRV)|(ABASSI_TIM_ADD)), (void *)(_Timer),				\
					             (intptr_t)(_Expiry));												\
				} while(0)

	#define TIMsem(_Timer, _Sem, _Expiry, _Period)													\
				do {																				\
					(_Timer)->Ops    = (ABASSI_POST) | (ABASSI_TIM_SRV);							\
					(_Timer)->Ptr    = (void *)(_Sem);												\
					(_Timer)->Expiry = (int)(_Expiry); 												\
					(_Timer)->ExpOri = (int)(_Expiry); 												\
					(_Timer)->Period = (int)(_Period);												\
					(void)Abassi(((ABASSI_TIM_SRV)|(ABASSI_TIM_ADD)), (void *)(_Timer),				\
					             (intptr_t)(_Expiry));												\
				} while(0)

#endif


/* ------------------------------------------------	*/
/* Tasks related macros								*/

#define TSKgetPrio(_Tsk)					(((volatile TSK_t *)(_Tsk))->Prio)
#define TSKmyID()							(COREtaskNow())

#if (((OX_MP_TYPE) & 4U) != 0)
  #define TSKgetCore(_Tsk, _CoreID)			((_Tsk)->TargetCore)
  #define TSKsetCore(_Tsk, _CoreID)			((_Tsk)->TargetCore = (_CoreID))
#endif

#if (OX_PRIO_CHANGE)
  #define TSKsetPrio(_Tsk,_New) ((void)Abassi(((ABASSI_NEW_PRIO)|(ABASSI_NEW_O_PRIO)),				\
								              (void *)(_Tsk), (intptr_t)(_New)))
#endif


#if (OX_STARVE_WAIT_MAX)
  #if ((OX_STARVE_PRIO) < 0)
	#define TSKgetStrvPrio(_Tsk)			(((volatile TSK_t *)(_Tsk))->StrvPrioMax)
	#define TSKsetStrvPrio(_Tsk, _Prio)		do {(_Tsk)->StrvPrioMax = (_Prio); 						\
											Abassi(ABASSI_STARVE, _Tsk, (intptr_t)0); } while(0)
  #else
	#define TSKgetStrvPrio(_Tsk)			(OX_STARVE_PRIO)
  #endif
  #if ((OX_STARVE_RUN_MAX) < 0)
	#define TSKgetStrvRunMax(_Tsk)			(((volatile TSK_t *)(_Tsk))->StrvRunMax)
	#define TSKsetStrvRunMax(_Tsk, _Cnt)	((_Tsk)->StrvRunMax  = (_Cnt))
  #else
	#define TSKgetStrvRunMax(_Tsk)			(OX_STARVE_RUN_MAX)
  #endif
  #if ((OX_STARVE_WAIT_MAX) < 0)
	#define TSKgetStrvWaitMax(_Tsk)			(((volatile TSK_t *)(_Tsk))->StrvWaitMax)
	#define TSKsetStrvWaitMax(_Tsk, _Cnt)	((_Tsk)->StrvWaitMax = (_Cnt))
  #else
	#define TSKgetStrvWaitMax(_Tsk)			(OX_STARVE_WAIT_MAX)
  #endif
#else
	#define TSKgetStrvPrio(_Tsk)			(OX_PRIO_MIN)
	#define TSKgetStrvRunMax(_Tsk)			0
	#define TSKgetStrvWaitMax(_Tsk)			0
#endif


#if (OX_TASK_SUSPEND)
  #define TSKresume(_Tsk)		((void)Abassi((ABASSI_RESUSP),  (void *)(_Tsk),  (intptr_t)0))
  #define TSKsuspend(_Tsk)		((void)Abassi(((ABASSI_RESUSP)|(ABASSI_SUSPEND)), (void *)(_Tsk),	\
								              (intptr_t)-1))
#else
  #define TSKresume(_Tsk)		((void)Abassi((ABASSI_POST), (void *)&((_Tsk)->Priv), (intptr_t)0))
#endif
#define TSKselfSusp()			do {																\
									TSK_t *Me = TSKmyID();											\
									((void)Abassi((ABASSI_WAIT), (void *)&(Me->Priv),(intptr_t)-1));\
								} while(0)

#if (OX_TIMER_US)
  extern void TIMtick(void) REENT_POST;
  #if ((OX_ROUND_ROBIN) < 0)
	#define TSKgetRR(_Tsk)		(((volatile TSK_t *)(_Tsk))->RRmax)
	#define TSKsetRR(_Tsk,_Tim)	((_Tsk)->RRmax = (_Tim))
  #else
   #ifdef OX_RR_TICK
	#define TSKgetRR(_Tsk)		(OX_RR_TICK)
   #endif
  #endif
  #if ((OX_TIMEOUT) > 0)
	#define TSKsleep(_TimeOut)	 ((void)Abassi((ABASSI_WAIT), (void *)&(TSKmyID()->Priv),			\
								               (intptr_t)(_TimeOut)))
    #define TSKtout(_Tsk, _Tout)        Abassi((ABASSI_TOUT_TSK), (void *)(_Tsk), (intptr_t)_Tout)
	#define TSKtimeoutKill(_Tsk) ((void)Abassi((ABASSI_TOUT_TSK), (void *)(_Tsk), (intptr_t)0))
  #endif
#else
	#define TSKgetRR(_Tsk)		(0U)
#endif

#if (OX_COOPERATIVE)								/* TSKyield() is !=  when cooperative or not	*/
	#define TSKyield()			do { 																\
									TSKmyID()->Yield = 1;											\
									(void)Abassi(0U, NULL, (intptr_t)0);							\
								} while(0)
#else												/* Not coop, put the task at end of linked list	*/
	#define TSKyield()			do {																\
									TSK_t *MyTask;													\
									MyTask = TSKmyID();												\
									if (MyTask->Next != (TSK_t *)NULL) {							\
										(void)Abassi(ABASSI_ADD_RUN, (void *)MyTask, (intptr_t)0);	\
									}																\
								} while(0)
#endif

#if (OX_EVENTS)
  #define OX_EVT_NOTBLK(_tsk)	(0U == ((volatile TSK_t *)(_tsk))->EventAND)
#else
  #define OX_EVT_NOTBLK(_tsk)	(1)
#endif
#if ((OX_TIMEOUT) > 0)
  #define OX_TOUT_NOTBLK(_tsk)	((TSK_t *)NULL == ((volatile TSK_t *)(_tsk))->ToutPrev)
#else
  #define OX_TOUT_NOTBLK(_tsk)	(1)
#endif
#if (OX_GROUP)
  #define OX_GRP_NOTBLK(_tsk)	((GRP_t *)NULL == ((volatile TSK_t *)(_tsk))->GrpBlkOn)
#else
  #define OX_GRP_NOTBLK(_tsk)	(1)
#endif

#define TSKstate(_Tsk)			((SEM_t *)NULL == (((volatile TSK_t *)(_Tsk))->Blocker)				\
								? 0								/* 0 is running		*/				\
								: (((((volatile TSK_t *)(_Tsk))->Blocker							\
								                           == (&((volatile TSK_t *)(_Tsk))->Priv))	\
								  && (OX_TOUT_NOTBLK(_Tsk))											\
								  && (OX_GRP_NOTBLK(_Tsk))											\
								  && (OX_EVT_NOTBLK(_Tsk)))											\
								  ? -1			/* -ve is suspended, +ve is blocked */			 	\
								  :  1))

#define TSKisRdy(_Tsk)			((SEM_t *)NULL == ((volatile TSK_t *)(_Tsk))->Blocker)
#define TSKisBlk(_Tsk)			(TSKstate(_Tsk) > 0)
#define TSKisSusp(_Tsk)			((((volatile TSK_t *)(_Tsk))->Blocker								\
								                          == (&((volatile TSK_t *)(_Tsk))->Priv))	\
								    && (OX_TOUT_NOTBLK(_Tsk))										\
								    && (OX_GRP_NOTBLK(_Tsk))										\
								    && (OX_EVT_NOTBLK(_Tsk)) )

#if (OX_USE_TASK_ARG)
  #define TSKsetArg(_Tsk, _Arg)	((_Tsk)->TaskArg=((void *)(intptr_t)_Arg))
  #define TSKgetArg()			((intptr_t)(((volatile TSK_t *)(TSKmyID()))->TaskArg))
#endif

#if (OX_STACK_CHECK)
  #define OX_STACK_MAGIC		((int32_t)0xFAEEDA7AL)

  extern int TSKstkChk(TSK_t *Task, int IsFree) REENT_POST;

  #define TSKstkFree(_Tsk)		TSKstkChk((_Tsk), 1)
  #define TSKstkUsed(_Tsk)		TSKstkChk((_Tsk), 0)
#endif


/* ------------------------------------------------	*/
/* Performance monitoring related macros			*/

#if ((OX_PERF_MON) != 0)
  #if ((OX_N_CORE) > 1)
	#define PM_RST_STATE(__Tsk)	do {																\
									if (((__Tsk)->MyCore >= 0)		/* Idle task				*/	\
									||  ((__Tsk) == TSKmyID())) {	/* My task (I am  running)	*/	\
										(__Tsk)->PMrunStrt = (__Tsk)->PMstartTick;					\
										(__Tsk)->PMcontrol = OX_PM_CTRL_RUN;						\
									}																\
									else {															\
										(__Tsk)->PMcontrol = ((__Tsk)->Blocker != (SEM_t *)NULL)	\
										                   ? OX_PM_CTRL_PEND						\
										                   : OX_PM_CTRL_ARMED;						\
									}																\
								} while(0)
  #else
	#define PM_RST_STATE(__Tsk)	do {																\
									if ((__Tsk) == TSKmyID()) {		/* My task (I am  running)	*/	\
										(__Tsk)->PMrunStrt = (__Tsk)->PMstartTick;					\
										(__Tsk)->PMcontrol = OX_PM_CTRL_RUN;						\
									}																\
									else {															\
										(__Tsk)->PMcontrol = ((__Tsk)->Blocker != (SEM_t *)NULL)	\
										                   ? OX_PM_CTRL_PEND						\
										                   : OX_PM_CTRL_ARMED;						\
									}																\
								} while(0)
  #endif
  #define PMrestart(_Tsk)		do {																\
									PMstop(_Tsk);													\
									memset(&(_Tsk)->OX_PERF_MON_FIRST, 0,		 					\
									             (size_t)(((int)offsetof(TSK_t, OX_PERF_MON_LAST)	\
		                                                -  (int)offsetof(TSK_t, OX_PERF_MON_FIRST))	\
		                                                +  (int)sizeof((_Tsk)->OX_PERF_MON_LAST)));	\
									(_Tsk)->PMstartTick = OX_PERF_MON_TIME();						\
									(_Tsk)->PMlastTick  = (_Tsk)->PMstartTick;						\
									PM_RST_STATE(_Tsk);												\
								} while(0)
  #define PMstop(_Tsk)			do {																\
									int _Try;				/* Make sure to not wait forever	*/	\
									for (_Try=0 ; _Try<1000 ; _Try++) {								\
										(_Tsk)->PMsnapshot = 0;										\
										if (((_Tsk)->PMupdating == 0)								\
										&&  ((_Tsk)->PMsnapshot == 0)) {							\
											if ((_Tsk)->PMcontrol != (OX_PM_CTRL_STOP)) {			\
												(_Tsk)->PMcontrol  = (OX_PM_CTRL_STOP);				\
												(_Tsk)->PMlastTick = OX_PERF_MON_TIME();			\
											}														\
											if ((_Tsk)->PMsnapshot == 0) {							\
												break;												\
											}														\
										}															\
									}																\
								} while(0)
#endif

/* ------------------------------------------------	*/
/* Abort related macros								*/

#if (OX_WAIT_ABORT)
	#define MTXabort(_Mtx)		(void)Abassi(ABASSI_ABRT_MTX, (void *)(_Mtx), (intptr_t)0)
	#define SEMabort(_Sem)		(void)Abassi(ABASSI_ABRT_SEM, (void *)(_Sem), (intptr_t)0)
  #if (OX_MAILBOX)
	#define MBXabort(_Mbx)		(void)Abassi(ABASSI_ABRT_MBX, (void *)(_Mbx), (intptr_t)0)
  #endif
  #if (OX_EVENTS)
	#define EVTabort(_Tsk)		(void)Abassi(ABASSI_ABRT_EVT, (void *)(_Tsk), (intptr_t)0)
  #endif
#endif


/* ------------------------------------------------	*/
/* Logging related macros							*/

#if (OX_LOGGING_TYPE)
	#define LOG_ENABLE			0				/* Enable a specific event number					*/
	#define LOG_DISABLE			1				/* Enable a specific event number					*/
	#define LOG_ALL_ON			2				/* Enable the recording of all events				*/
	#define LOG_ALL_OFF			3				/* Disable the recording of all events				*/
	#define LOG_OFF				4				/* Turn off the recording (freeze it)				*/
	#define LOG_ON				5				/* Turn on the recording (Continue)					*/
	#define LOG_CONT			6				/* Start from scratch until OFF/buffer full			*/
	#define LOG_ONCE			7				/* Reset the recording: empty the buffer			*/
	#define LOG_DUMP_ALL		8				/* Dump all valid events from the buffer			*/
	#define LOG_DUMP_NEXT		9				/* Dump the next valid event from the buffer		*/
	#define LOG_GET_NEXT		10				/* Retrieve the next valid event from the buffer	*/

	#define LOGallOff()			((void)LOGctrl((LOG_ALL_OFF),   0U))
	#define LOGallOn()			((void)LOGctrl((LOG_ALL_ON),    0U))
	#define LOGcont()			((void)LOGctrl((LOG_CONT),      0U))
	#define LOGdis(_Nmb)		((void)LOGctrl((LOG_DISABLE),  (unsigned int)(_Nmb)))
	#define LOGdumpAll()		((void)LOGctrl((LOG_DUMP_ALL),  0U))
	#define LOGdumpNext()		((void)LOGctrl((LOG_DUMP_NEXT), 0U))
	#define LOGenb(_Nmb)		((void)LOGctrl((LOG_ENABLE),   (unsigned int)(_Nmb)))
	#define LOGgetNext()		      (LOGctrl((LOG_GET_NEXT),  0U))
	#define LOGoff()			((void)LOGctrl((LOG_OFF),       0U))
	#define LOGon()				((void)LOGctrl((LOG_ON),        0U))
	#define LOGonce()			((void)LOGctrl((LOG_ONCE),      0U))

	extern const char   		*LOGctrl(int Ctrl, unsigned int Opt) REENT_POST;

#else
	#define LOGallOff()
	#define LOGallOn()
	#define LOGcont();
	#define LOGdis(_Nmb)
	#define LOGdumpAll()
	#define LOGdumpNext()
	#define LOGenb(_Nmb)
	#define LOGgetNext()
	#define LOGoff()
	#define LOGon()
	#define LOGonce()

#endif


/* ------------------------------------------------	*/
/* All H/W spinlocks reg used by mAbassi & drivers	*/

#define KERNEL_SPINLOCK		((OX_SPINLOCK_BASE)+0)
#define MBLK_SPINLOCK		((OX_SPINLOCK_BASE)+1)
#define GIC_SPINLOCK		((OX_SPINLOCK_BASE)+2)
#define CMSIS_SPINLOCK		((OX_SPINLOCK_BASE)+3)
#define CACHE_SPINLOCK		((OX_SPINLOCK_BASE)+4)
#define LWIP_SPINLOCK		((OX_SPINLOCK_BASE)+5)
#define UART_SPINLOCK		((OX_SPINLOCK_BASE)+6)
#define MBX_SPINLOCK		((OX_SPINLOCK_BASE)+7)
#define TICK_SPINLOCK		((OX_SPINLOCK_BASE)+8)
#define DMA_SPINLOCK		((OX_SPINLOCK_BASE)+9)
#define DMAI_SPINLOCK		((OX_SPINLOCK_BASE)+10)
#define GPIO_SPINLOCK		((OX_SPINLOCK_BASE)+11)
#define I2C_SPINLOCK		((OX_SPINLOCK_BASE)+12)
#define QSPI_SPINLOCK		((OX_SPINLOCK_BASE)+13)
#define SPI_SPINLOCK		((OX_SPINLOCK_BASE)+14)
#define MMC_SPINLOCK		((OX_SPINLOCK_BASE)+15)
#define ETH_SPINLOCK		((OX_SPINLOCK_BASE)+16)
#ifdef __cplusplus
  }
#endif

#endif											/* The whole file is conditional					*/

/* EOF */
