/* ------------------------------------------------------------------------------------------------ */
/* FILE :        sys_arch.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*               Layer between lwIP and Abassi														*/
/*																									*/
/*																									*/
/* Copyright (c) 2013-2018, Code-Time Technologies Inc. All rights reserved.						*/
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
/*  $Revision: 1.15 $																				*/
/*	$Date: 2018/11/02 19:24:27 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"

#ifndef SYS_MBX_HOLD
  #define SYS_MBX_HOLD		16					/* Maximum number of "freed" mailboxes				*/
#endif
#ifndef SYS_MTX_HOLD
  #define SYS_MTX_HOLD		16					/* Maximum number of "freed" mutexes				*/
#endif
#ifndef SYS_SEM_HOLD
  #define SYS_SEM_HOLD		16					/* Maximum number of "freed" semaphores				*/
#endif
#ifndef LWIP_MAX_TASK
  #define LWIP_MAX_TASK		16					/* Maximum # of tasks created with sys_thread_new()	*/
#endif

struct _MboxParking {							/* Structure to hold freed mailboxes & related info	*/
	sys_mbox_t Mbox;							/* Mailbox that was "freed"							*/
	int        Size;							/* Size of the freed mailbox						*/
};

struct _TaskFctArg {							/* Structure to pass the task argument in function	*/
	lwip_thread_fn Fct;							/* Function attached to the task					*/
	void          *Arg;							/* Task argument to pass to the function			*/
};

static struct _MboxParking g_MbxParking[SYS_MBX_HOLD];
static sys_mutex_t         g_MtxParking[SYS_MTX_HOLD];
static sys_sem_t           g_SemParking[SYS_SEM_HOLD];
static struct _TaskFctArg  g_TaskFctArg[LWIP_MAX_TASK];

#if ((MEM_LIBC_MALLOC) != 0)
  static MTX_t              *g_LWIPmutex;
#endif

#if ((OX_N_CORE) > 1)	
  static int               g_ParkSpin;			/* To protect access to the parking lots			*/
#endif

static void TaskTrampoline(void);				/* Used to set argument as lwip_thread_fn() needs	*/

#if ((OX_N_CORE) > 1)							/* Macros for short time mutual exclusion			*/
  #define PROT_START()		OSintOff(); CORElock(LWIP_SPINLOCK, &g_ParkSpin, 0, 1+COREgetID())
  #define PROT_END(x)		do {COREunlock(LWIP_SPINLOCK, &g_ParkSpin, 0); OSintBack(x);}while(0)
#else											/* The spinlock is not needed on a single core		*/
  #define PROT_START()		OSintOff()
  #define PROT_END(x)		OSintBack(x)
#endif

#if (((OX_N_CORE) > 1) && ((MEM_LIBC_MALLOC) == 0))
  static volatile int g_MySpinLock;				/* The spinlock variable							*/
  static volatile int g_MySpinGot;				/* When I got the spin lock (needed for reentrance	*/
  static volatile int g_MySpinIsr[OX_N_CORE];	/* ISR enable/disable state of the core				*/
#endif
static int g_IdxMbxNames;
static int g_IdxMtxNames;
static int g_IdxSemNames;

static const char g_Names[][8] = {
                                   "lwIP-0"
                                  ,"lwIP-1"
                                  ,"lwIP-2"
                                  ,"lwIP-3"
                                  ,"lwIP-4"
                                  ,"lwIP-5"
                                  ,"lwIP-6"
                                  ,"lwIP-7"
								#if (((SYS_MBX_HOLD)>8)||((SYS_MTX_HOLD)>8)||((SYS_SEM_HOLD)>8))
                                  ,"lwIP-8"
                                  ,"lwIP-9"
                                  ,"lwIP-10"
                                  ,"lwIP-11"
                                  ,"lwIP-12"
                                  ,"lwIP-13"
                                  ,"lwIP-14"
                                  ,"lwIP-15"
								#endif
								#if (((SYS_MBX_HOLD)>16)||((SYS_MTX_HOLD)>16)||((SYS_SEM_HOLD)>16))
                                  ,"lwIP-16"
                                  ,"lwIP-17"
                                  ,"lwIP-18"
                                  ,"lwIP-19"
                                  ,"lwIP-20"
                                  ,"lwIP-21"
                                  ,"lwIP-22"
                                  ,"lwIP-23"
                                  ,"lwIP-24"
                                  ,"lwIP-25"
                                  ,"lwIP-26"
                                  ,"lwIP-27"
                                  ,"lwIP-28"
                                  ,"lwIP-29"
                                  ,"lwIP-30"
                                  ,"lwIP-31"
								#endif
								#if (((SYS_MBX_HOLD)>32)||((SYS_MTX_HOLD)>32)||((SYS_SEM_HOLD)>32))
                                  ,"lwIP-32"
                                  ,"lwIP-33"
                                  ,"lwIP-34"
                                  ,"lwIP-35"
                                  ,"lwIP-36"
                                  ,"lwIP-37"
                                  ,"lwIP-38"
                                  ,"lwIP-39"
                                  ,"lwIP-40"
                                  ,"lwIP-41"
                                  ,"lwIP-42"
                                  ,"lwIP-43"
                                  ,"lwIP-44"
                                  ,"lwIP-45"
                                  ,"lwIP-46"
                                  ,"lwIP-47"
								#endif
								#if (((SYS_MBX_HOLD)>48)||((SYS_MTX_HOLD)>48)||((SYS_SEM_HOLD)>48))
                                  ,"lwIP-48"
                                  ,"lwIP-49"
                                  ,"lwIP-50"
                                  ,"lwIP-51"
                                  ,"lwIP-52"
                                  ,"lwIP-53"
                                  ,"lwIP-54"
                                  ,"lwIP-55"
                                  ,"lwIP-56"
                                  ,"lwIP-57"
                                  ,"lwIP-58"
                                  ,"lwIP-59"
                                  ,"lwIP-60"
                                  ,"lwIP-61"
                                  ,"lwIP-62"
                                  ,"lwIP-63"
								#endif
                                 };

#if (((SYS_MBX_HOLD)>64)||((SYS_MTX_HOLD)>64)||((SYS_SEM_HOLD)>64))
  #error "Add more names in the g_Names[] array"
#endif

/*--------------------------------------------------------------------------------------------------*/
/*
  Creates an empty mailbox.
*/
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
int        ii;									/* Loop counter										*/
int        IsrState;							/* State of ISRs (enable/disable) before disabling	*/
sys_mbox_t MailBox;

	MailBox = (sys_mbox_t)NULL;					/* Assume there is no matching "freed" mailboxes	*/

	IsrState = PROT_START();					/* Make sure only 1 task plays with the parking lot	*/

	for (ii=0 ; ii<SYS_MBX_HOLD ; ii++) {		/* Check if the same mailbox size was freed			*/
		if ((g_MbxParking[ii].Mbox != (sys_mbox_t)NULL)
		&&  (g_MbxParking[ii].Size == size)) {	/* If yes, then take the freed one instead of new	*/
			MailBox               = g_MbxParking[ii].Mbox;
			g_MbxParking[ii].Mbox = (sys_mbox_t)NULL;
		  
			MailBox->RdIdx        = 0;			/* Make sure the mailbox is empty					*/
			MailBox->WrtIdx       = MailBox->Size;	/* Should defintitely be but let's play safe	*/
			break;
		}
	}

	PROT_END(IsrState);

	if (MailBox == (sys_mbox_t)NULL) {			/* No such mailbox was freed						*/
		MailBox = (sys_mbox_t)MBXopen(&g_Names[g_IdxMbxNames++][0], size);
	}

	*mbox = MailBox;

  #if ((SYS_STATS) != 0)
	if (MailBox == (sys_mbox_t)NULL) {
		STATS_INC(sys.mbox.err);				/* Should be protected. Not, to keep best real-time	*/
	}
	else {
		STATS_INC(sys.mbox.used);
	}
  #endif

	return((MailBox == NULL) ? (ERR_MEM) : (ERR_OK));
}

/*--------------------------------------------------------------------------------------------------*/
/*
  Deallocates a mailbox. If there are messages still present in the
  mailbox when the mailbox is deallocated, it is an indication of a
  programming error in lwIP and the developer should be notified.
*/
void sys_mbox_free(sys_mbox_t *mbox)
{
int        ii;									/* Loop counter										*/
int        IsrState;							/* State of ISRs (enable/disable) before disabling	*/
sys_mbox_t MailBox;								/* New mailbox (either MBXopen or from parking lot)	*/

	STATS_DEC(sys.mbox.used);					/* Should be protected. Not, to keep best real-time	*/

	MailBox = *mbox;							/* Local may help speed-up the code					*/

	IsrState = PROT_START();					/* Make sure only 1 task plays with the parking lot	*/

	for (ii=0 ; ii<SYS_MBX_HOLD ; ii++) {		/* Find an unused entry to memo that mailbox		*/
		if (g_MbxParking[ii].Mbox == (sys_mbox_t)NULL) {
			g_MbxParking[ii].Mbox = MailBox;
			g_MbxParking[ii].Size = MailBox->Size;
			*mbox                  = (sys_mbox_t)NULL;
			break;
		}
	}

	PROT_END(IsrState);
												/* No more room to hold deleted mailbox				*/
	LWIP_ERROR("lwIP (sys_arch.c) - Out room to hold mailboxes", (ii<SYS_MBX_HOLD), do{}while(0); );

	if (ii >= SYS_MBX_HOLD) {					/* Stop here as this is a memory leak				*/
		(void)OSintOff();						/* This should be found during development			*/
		for(;;);
	}

	return;
}

/*--------------------------------------------------------------------------------------------------*/
/*
   Posts the "msg" to the mailbox.
*/
void sys_mbox_post(sys_mbox_t *mbox, void *data)
{
	MBXput(*mbox, (intptr_t)data, -1);

	return;
}


/*--------------------------------------------------------------------------------------------------*/
/*
   Try to post the "msg" to the mailbox.
*/
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
err_t result;

	result = ERR_OK;							/* Assume the mailbox is not full					*/
	if (0 != MBXput(*mbox, (intptr_t)msg, 0)) {
		result = ERR_MEM;						/* Mailbox is full, return info abour the error		*/
		STATS_INC(sys.mbox.err);				/* Should be protected. Not, to keep best real-time	*/
	}

   return(result);
}

/*--------------------------------------------------------------------------------------------------*/
/*
  Blocks the thread until a message arrives in the mailbox, but does
  not block the thread longer than "timeout" milliseconds (similar to
  the sys_arch_sem_wait() function). The "msg" argument is a result
  parameter that is set by the function (i.e., by doing "*msg =
  ptr"). The "msg" parameter maybe NULL to indicate that the message
  should be dropped.

  The return values are the same as for the sys_arch_sem_wait() function:
  Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
  timeout.

  Note that a function with a similar name, sys_mbox_fetch(), is
  implemented by lwIP.
*/
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
u32_t Elapse;

	Elapse = (u32_t)G_OStimCnt;					/* Current Abassi internal time						*/
	if (0 == MBXget(*mbox, (intptr_t *)msg, (timeout == 0) ? -1 : OS_MS_TO_TICK(timeout))) {
		Elapse = (((u32_t)G_OStimCnt-Elapse)*1000)
		       / OS_TICK_PER_SEC;				/* Compute elapsed time and convert in milliseconds	*/
	}
	else {
		Elapse = SYS_ARCH_TIMEOUT;				/* Did not get it, report the error					*/
		*msg   = NULL;							/* Make sure there is an invalid meesage there		*/
	}

	return(Elapse);
}

/*--------------------------------------------------------------------------------------------------*/
/*
  Similar to sys_arch_mbox_fetch, but if message is not ready immediately, we'll
  return with SYS_MBOX_EMPTY.  On success, 0 is returned.
*/
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
u32_t RetVal;

	RetVal = ERR_OK;
	if (0 != MBXget(*mbox, (intptr_t *)msg, 0)) {
		*msg   = NULL;							/* Make sure there is an invalid meesage there		*/
		RetVal = SYS_MBOX_EMPTY;				/* Report the mailbox is empty						*/
	}

	return(RetVal);
}

/*--------------------------------------------------------------------------------------------------*/
/*
  Creates and returns a new semaphore. The "count" argument specifies
  the initial state of the semaphore.
*/
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
int       ii;									/* Loop counter										*/
int       IsrState;								/* State of ISRs (enable/disable) before disabling	*/
sys_sem_t Sema;									/* New semaphore (either SEMopen or parking lot)	*/

	Sema = (sys_sem_t)NULL;

	IsrState = PROT_START();					/* Make sure only 1 task plays with the parking lot	*/

	for (ii=0 ; ii<SYS_SEM_HOLD ; ii++) {		/* Check if a semaphore was freed					*/
		if (g_SemParking[ii] != (sys_sem_t)NULL) {/* If yes, then take the freed one instead of new	*/
			Sema             = g_SemParking[ii];
			g_SemParking[ii] = (sys_sem_t)NULL;
			break;
		}
	}

	PROT_END(IsrState);

	if (Sema == (sys_sem_t)NULL) {				/* No semaphore was freed							*/
		Sema = SEMopen(&g_Names[g_IdxSemNames++][0]);
	}

	Sema->Value = (int)count;					/* Set the initial count as requested				*/

	*sem = Sema;

  #if ((SYS_STATS) != 0)
	if (Sema == (sys_sem_t)NULL) {
		STATS_INC(sys.sem.err);					/* Should be protected. Not, to keep best real-time	*/
	}
	else {
		STATS_INC(sys.sem.used);
	}
  #endif

	LWIP_ERROR("lwIP (sys_arch.c) - Out of semaphores", (Sema != NULL), do{}while(0); );

	return((Sema == NULL) ? (ERR_MEM) : (ERR_OK));
}

/*--------------------------------------------------------------------------------------------------*/
/* Used in version #2																				*/

sys_sem_t *LWIP_NETCONN_THREAD_SEM_GET(void)
{
static sys_sem_t sem = NULL;

	if (sem == NULL) { 
		sys_sem_new(&sem, 0);
	}
	return(&sem);
}

/*--------------------------------------------------------------------------------------------------*/
/*
  Blocks the thread while waiting for the semaphore to be
  signaled. If the "timeout" argument is non-zero, the thread should
  only be blocked for the specified time (measured in
  milliseconds).

  If the timeout argument is non-zero, the return value is the number of
  milliseconds spent waiting for the semaphore to be signaled. If the
  semaphore wasn't signaled within the specified time, the return value is
  SYS_ARCH_TIMEOUT. If the thread didn't have to wait for the semaphore
  (i.e., it was already signaled), the function may return zero.

  Notice that lwIP implements a function with a similar name,
  sys_sem_wait(), that uses the sys_arch_sem_wait() function.
*/
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
u32_t Elapse;

	Elapse = (u32_t)G_OStimCnt;					/* Current Abassi internal time						*/
	if(0 == SEMwait((SEM_t *)*sem, (timeout == 0) ? -1 : OS_MS_TO_TICK((int)timeout))) {
		Elapse = (((u32_t)G_OStimCnt-Elapse)*1000)
		       / OS_TICK_PER_SEC;				/* Compute elapsed time and convert in milliseconds	*/
	}
	else {
		Elapse = SYS_ARCH_TIMEOUT;				/* Did not get it, report the error					*/
	}

	return(Elapse);
}

/*--------------------------------------------------------------------------------------------------*/
/*
  Signals a semaphore
*/
void sys_sem_signal(sys_sem_t *sem)
{
	SEMpost((SEM_t *)*sem);

	return;
}

/*--------------------------------------------------------------------------------------------------*/
/*
  Deallocates a semaphore
*/
void sys_sem_free(sys_sem_t *sem)
{
int ii;											/* Loop counter										*/
int IsrState;									/* State of ISRs (enable/disable) before disabling	*/

	STATS_DEC(sys.sem.used);					/* Should be protected. Not, to keep best real-time	*/

	IsrState = PROT_START();					/* Make sure only 1 task plays with the parking lot*/

	for (ii=0 ; ii<SYS_SEM_HOLD ; ii++) {		/* Find an unused entry to memo that semaphore		*/
		if (g_SemParking[ii] == (sys_sem_t)NULL) {
			g_SemParking[ii] = *sem;
			break;
		}
	}

	PROT_END(IsrState);
												/* No more room to hold deleted semaphores			*/
	LWIP_ERROR("lwIP (sys_arch.c) - Out room to hold semaphores", (ii<SYS_SEM_HOLD), do{}while(0); );

	if (ii >= SYS_SEM_HOLD) {					/* Stop here as this is a memory leak				*/
		(void)OSintOff();						/* This should be found during development			*/
		for(;;);
	}

	return;
}

/*--------------------------------------------------------------------------------------------------*/
/** Create a new mutex
 * @param mutex pointer to the mutex to create
 * @return a new mutex */
err_t sys_mutex_new(sys_mutex_t *mutex)
{
int         ii;									/* Loop counter										*/
int         IsrState;							/* State of ISRs (enable/disable) before disabling	*/
sys_mutex_t Mtx;								/* New mutex (either MTXopen or from parking lot)	*/

	Mtx = (sys_sem_t)NULL;

	IsrState = PROT_START();					/* Make sure only 1 task plays with the parking lot	*/

	for (ii=0 ; ii<SYS_MTX_HOLD ; ii++) {		/* Check if a mutex was freed						*/
		if (g_MtxParking[ii] != (sys_mutex_t)NULL) {/* If yes, then take the freed instead of new	*/
			Mtx              = g_MtxParking[ii];
			g_MtxParking[ii] = (sys_mutex_t)NULL;
			break;
		}
	}

	PROT_END(IsrState);

	if (Mtx == (sys_mutex_t)NULL) {				/* No mutex was freed								*/
		Mtx = MTXopen(&g_Names[g_IdxMtxNames++][0]);
	}

	*mutex = Mtx;

  #if ((LWIP_DEBUG) != 0)
	if (Mtx == NULL) {
		puts("ERROR: lwIP (sys_arch.c) - Out of mutexes");
	}
  #endif

  #if ((SYS_STATS) != 0)
	if (Mtx == (sys_mutex_t)NULL) {
		STATS_INC(sys.mutex.err);				/* Should be protected. Not, to keep best real-time	*/
	}
	else {
		STATS_INC(sys.mutex.used);
	}
  #endif

	LWIP_ERROR("lwIP (sys_arch.c) - Out of mutexes", (Mtx != NULL), do{}while(0); );

	return((Mtx == NULL) ? (ERR_MEM) : (ERR_OK));
}

/*--------------------------------------------------------------------------------------------------*/
/** Lock a mutex
 * @param mutex the mutex to lock */
void sys_mutex_lock(sys_mutex_t *mutex)
{
	MTXlock((MTX_t *)*mutex, -1);

	return;
}

/*--------------------------------------------------------------------------------------------------*/
/** Unlock a mutex
 * @param mutex the mutex to unlock */
void sys_mutex_unlock(sys_mutex_t *mutex)
{
	MTXunlock((MTX_t *)*mutex);

	return;
}

/*--------------------------------------------------------------------------------------------------*/
/** Delete a mutex
 * @param mutex the mutex to delete */
void sys_mutex_free(sys_mutex_t *mutex) 
{
int ii;											/* Loop counter										*/
int IsrState;									/* State of ISRs (enable/disable) before disabling	*/

	STATS_DEC(sys.mutex.used);					/* Should be protected. Not, to keep best real-time	*/

	(*mutex)->Value = 1;						/* Make sure the mutex is at its initial state		*/

	IsrState = PROT_START();					/* Make sure only 1 task plays with the arking lot	*/

	for (ii=0 ; ii<SYS_MTX_HOLD ; ii++) {		/* Find an unused entry to memo that semaphore		*/
		if (g_MtxParking[ii] == (sys_mutex_t)NULL) {
			g_MtxParking[ii] = *mutex;
			break;
		}
	}

	PROT_END(IsrState);
												/* No more room to hold deleted mutex				*/
	LWIP_ERROR("lwIP (sys_arch.c) - Out room to hold mutexes", (ii<SYS_MTX_HOLD), do{}while(0); );

	if (ii >= SYS_MTX_HOLD) {					/* Stop here as this is a memory leak				*/
		(void)OSintOff();						/* This should be found during development			*/
		for(;;);
	}

	return;
}

/*--------------------------------------------------------------------------------------------------*/
/*
  Initialize sys arch
*/
void sys_init(void)
{
int ii;											/* Loop counter										*/
	
	for (ii=0 ; ii<SYS_MBX_HOLD ; ii++) {		/* No mailboxes freed yet							*/
		g_MbxParking[ii].Mbox = (sys_mbox_t)NULL;
	}
	for (ii=0 ; ii<SYS_MTX_HOLD ; ii++) {		/* No mutexes freed yet								*/
		g_MtxParking[ii] = (sys_mutex_t)NULL;
	}
	for (ii=0 ; ii<SYS_SEM_HOLD ; ii++) {		/* No semaphores freed yet							*/
		g_SemParking[ii] = (sys_sem_t)NULL;
	}
	for (ii=0 ; ii<LWIP_MAX_TASK ; ii++) {
		g_TaskFctArg[ii].Fct = (lwip_thread_fn)NULL;
	}

	g_IdxMbxNames=0;
	g_IdxMtxNames=0;
	g_IdxSemNames=0;

  #if ((MEM_LIBC_MALLOC) != 0)
	g_LWIPmutex = MTXopen("LWIP Park");			/* Get mutex to protect the parking array accesses	*/
  #endif

  #if (((OX_N_CORE) > 1) && ((MEM_LIBC_MALLOC) == 0))
	g_MySpinGot  = 0;
	g_MySpinLock = 0;
  #endif

  #if ((SYS_STATS) != 0)
	memset(&lwip_stats.sys, 0, sizeof(lwip_stats.sys));
  #endif

	return;
}

/*--------------------------------------------------------------------------------------------------*/
/*
  Starts a new thread with priority "prio" that will begin its execution in the
  function "thread()". The "arg" argument will be passed as an argument to the
  thread() function. The id of the new thread is returned. Both the id and
  the priority are system dependent.
*/
sys_thread_t sys_thread_new(const char *name,lwip_thread_fn thread,void *arg,int stacksize,int prio)
{
int          ii;								/* Loop counter										*/
int          IsrState;							/* State of ISRs (enable/disable) before disabling	*/
sys_thread_t Task;								/* Created task decriptor							*/

												/* Create thread in suspended mode to set argument	*/
	Task = (sys_thread_t)TSKcreate(name, prio, stacksize, &TaskTrampoline, 0);

	LWIP_ERROR("lwIP (sys_arch.c) - Out of threads", (Task != NULL), do{}while(0); );

	if (Task == NULL) {							/* Out of thread, stop here are NULL is not defined	*/
		(void)OSintOff();						/* as an error condition for sys_thrtead_new()		*/
		for(;;);								/* This should be found during development			*/
	}

	IsrState = PROT_START();					/* Make sure only 1 task plays with the parking lot	*/

	for (ii=0 ; ii<LWIP_MAX_TASK ; ii++) {
		if (g_TaskFctArg[ii].Fct == (lwip_thread_fn)NULL) {
			g_TaskFctArg[ii].Fct = thread;
			g_TaskFctArg[ii].Arg = arg;
			break;
		}
	}

	PROT_END(IsrState);

	LWIP_ERROR("lwIP (sys_arch.c) - Out room to hold threads", (ii<LWIP_MAX_TASK),  do{}while(0););

	if (ii>= LWIP_MAX_TASK) {					/* Stop here as this is a memory leak				*/
		(void)OSintOff();						/* This should be found during development			*/
		for(;;);
	}
	TSKsetArg(Task, &g_TaskFctArg[ii]);			/* Set the task function and argument				*/

  #ifdef LWIP_ABASSI_BMP_CORE_2_USE				/* If lwIP is assigned to one core					*/
	TSKsetCore(Task, LWIP_ABASSI_BMP_CORE_2_USE);
  #endif

	TSKresume(Task);							/* It can now be running							*/

	return(Task);
}

/*--------------------------------------------------------------------------------------------------*/
/* Intermediate function used to pass the task argument in the function argument					*/
/*--------------------------------------------------------------------------------------------------*/

void TaskTrampoline(void)
{
struct _TaskFctArg *MyFctArg;					/* My function and argument							*/

	MyFctArg = (void *)TSKgetArg();				/* Get the Fct pointer and function argument		*/

	for(;;) {									/* Abassi doc states return is same as suspend		*/
		MyFctArg->Fct(MyFctArg->Arg);			/* Call the task function							*/
		TSKselfSusp();							/* If returning, suspend the task as documented		*/
	}
}

/*--------------------------------------------------------------------------------------------------*/
/*
  This optional function does a "fast" critical region protection and returns
  the previous protection level. This function is only called during very short
  critical regions. An embedded system which supports ISR-based drivers might
  want to implement this function by disabling interrupts. Task-based systems
  might want to implement this by using a mutex or disabling tasking. This
  function should support recursive calls from the same task or interrupt. In
  other words, sys_arch_protect() could be called while already protected. In
  that case the return value indicates that it is already protected.

  sys_arch_protect() is only required if your port is supporting an operating
  system.
*/

/* NOTE: the normal way would be to disable the interrupts and in the case of multi-core to also 	*/
/*       use a spinlock to protect the resource access across the different cores. But 				*/
/*       sys_arch_protect() and sys_arch_unprotect() are used to protect the lwIP memory management	*/
/*		 which can use malloc().																	*/
/*       Because some libraries can be protected against re-entrance by a mutex, if the interrupts	*/
/*       are disabled when the task blocks on the mutex re-entrance protection would trigger a bad	*/
/*       situation.  The blocking means a task switch will happen when the interrupts are disable.	*/
/*		 This would most likely mean the application would become frozen. That's why a mutex is		*/
/*       used instead of enabling/disabling ISRs.													*/

sys_prot_t sys_arch_protect(void)
{

  #if ((MEM_LIBC_MALLOC) != 0)
	MTXlock(g_LWIPmutex, -1);
	return((sys_prot_t)0);

  #else
   #if ((OX_N_CORE) == 1)
	return((sys_prot_t)OSintOff());

   #else
	int IsrState;
	int MyCoreID;
	IsrState = OSintOff();
	MyCoreID = COREgetID();
	CORElock(LWIP_SPINLOCK, &g_MySpinLock, 0, 1+MyCoreID);
	if (g_MySpinGot == 0) {
		g_MySpinGot = 1;
		g_MySpinIsr[MyCoreID] = IsrState;
		return((sys_prot_t)1);
	}
	else {
		return((sys_prot_t)0);
	}
   #endif
  #endif
}

/*--------------------------------------------------------------------------------------------------*/
/*
  This optional function does a "fast" set of critical region protection to the
  value specified by pval. See the documentation for sys_arch_protect() for
  more information. This function is only required if your port is supporting
  an operating system.
*/

void sys_arch_unprotect(sys_prot_t pval)
{

  #if ((MEM_LIBC_MALLOC) != 0)
	pval = pval;								/* To remove compiler warning						*/
	MTXunlock(g_LWIPmutex);

  #else
   #if ((OX_N_CORE) == 1)
	OSintBack((int)pval);

   #else
	if (pval != 0) {							/* When !=0, is the first time was called			*/
		g_MySpinGot = 0;						/* This requires us to release the spinlock			*/
		COREunlock(LWIP_SPINLOCK, &g_MySpinLock, 0);
		OSintBack((int)g_MySpinIsr[COREgetID()]);	/* Put ISR back as when protect called 1st time	*/
	}
   #endif
  #endif

	return;
}

/*--------------------------------------------------------------------------------------------------*/
/*
 * Prints an assertion messages and aborts execution.
 */
void sys_assert( const char *msg )
{
	puts(msg);
	(void)OSintOff();
	for(;;);
}

/*--------------------------------------------------------------------------------------------------*/
/*
 * Prints an assertion messages and aborts execution.
 */

u32_t sys_now(void)
{                                                    /* When RTOS tick not exact ms, we need to add    */
#if ((((OX_TIMER_US)/1000)*1000) != (OX_TIMER_US))    /* extra MSbits to the count for a precise    */
    static unsigned int Prev   = 0;                    /* conversion. It is stated sys_now() is only    */
    static uint64_t     Offset = 0;                    /* used for time difference so we know it will    */
    unsigned int Now;                        /* always be used first to read a ref and then    */
    int          ISRstate;                    /* will be called again sooner than 2^32 ticks    */
    
    ISRstate = PROT_START();
    Now = (unsigned int)G_OStimCnt;                /* To add extra MSBits we need to work with        */
    if (Prev > Now) {                                /* unsigned variables                            */
        Offset += 0x100000000ULL;                    /* When a rool-=over occurred, increment the    */
    }                                                /* MSBits to add                                */
    Prev = Now;                                    /* Memo for the next call                        */
    PROT_END(ISRstate);
    
    return((u32_t)(((Offset+(uint64_t)Now) * (OS_TIMER_US)) / 1000LL));
#else                                                /* OS_TIMER_US is an exact multiple of 1 ms        */
    return(((u32_t)G_OStimCnt) * ((OS_TIMER_US)/1000));
    
#endif
}


/* EOF */
