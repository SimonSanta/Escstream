/* ------------------------------------------------------------------------------------------------ */
/* FILE :		Abassi_FullFAT.c																	*/
/*																									*/
/* CONTENTS :																						*/
/*				Mutex and timeout I/F for FullFAT													*/
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
/*	$Date: 2019/01/10 18:06:18 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "ff_safety.h"								/* Needed for fct prototypes but also for the	*/
													/* correct Abassi.h / mAbassi.h / uAbassi.h		*/
/* ------------------------------------------------------------------------------------------------ */

#if ((OX_RUNTIME) == 0)
  #error "FullFAT requires run-time service creation. Set OS_RUNTIME to non-zero"
#endif

/* ------------------------------------------------------------------------------------------------ */

#if ((OX_SEM_PARKING) != 0)							/* Abassi can be set for parking linked list	*/
  static MTX_t *g_Parking = NULL;					/* Holds the mutexes that were deleted			*/
#else
  typedef struct _MtxList {
	MTX_t *Mutex;
	struct _MtxList *ParkNext;
  } MtxList_t;
  static MtxList_t *g_Parking = NULL;				/* Holds the mutexes that were deleted			*/
#endif

static const char g_Names[][10] = {
                                 "FullFat-0",
                                 "FullFat-1",
                                 "FullFat-2",
                                 "FullFat-3",
                                 "FullFat-4",
                                 "FullFat-5",
                                 "FullFat-6",
                                 "FullFat-7",
                                 "FullFat-8",
                                 "FullFat-9"
                               };
static int g_IdxName=0;

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    FF_CreateSemaphore																	*/
/*																									*/
/* FF_CreateSemaphore - create a mutex																*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void *FF_CreateSemaphore(void);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*																									*/
/* RETURN VALUE:																					*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void *FF_CreateSemaphore(void)
{
void *Mutex;

	if (0 != MTXlock(G_OSmutex, -1)) {				/* Although infinte wait, check in case			*/
		return((void *)NULL);						/* deadlock detection is used					*/
	}

  #if ((OX_SEM_PARKING) != 0)
	if (g_Parking == NULL) {
		g_Parking = MTXopen(&g_Names[g_IdxName++][0]);
	}
  #else
	if (g_Parking == NULL) {
		g_Parking = OSalloc(sizeof(*g_Parking));
		if (g_Parking == NULL) {
			OSintOff();
			for(;;);
		}
		g_Parking->Mutex    = MTXopen(&g_Names[g_IdxName++][0]);
		g_Parking->ParkNext = NULL; 
	}
  #endif

	Mutex     = (void *)g_Parking;
	g_Parking = g_Parking->ParkNext;

	MTXunlock(G_OSmutex);

	return(Mutex);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    FF_DestroySemaphore																	*/
/*																									*/
/* FF_DestroySemaphore - delete a mutex																*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void FF_DestroySemaphore(void *pSemaphore);													*/
/*																									*/
/* ARGUMENTS:																						*/
/*																									*/
/* RETURN VALUE:																					*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void FF_DestroySemaphore(void *pSemaphore)
{
#if ((OX_SEM_PARKING) != 0)
  MTX_t *Mutex = (MTX_t *)pSemaphore;
#else
  MtxList_t *Mutex = (MtxList_t *)pSemaphore;
#endif

	if (0 != MTXlock(G_OSmutex, -1)) {				/* Although infinte wait, check in case			*/
	  #if ((OX_OUT_OF_MEM) != 0)					/* deadlock detection is used					*/
		OSintOff();
		for(;;);
	  #else
		return;
	  #endif
	}

	Mutex->ParkNext = g_Parking;
	g_Parking       = Mutex;

	MTXunlock(G_OSmutex);

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    FF_PendSemaphore																	*/
/*																									*/
/* FF_PendSemaphore - lock a mutex																	*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void FF_PendSemaphore(void *pSemaphore);													*/
/*																									*/
/* ARGUMENTS:																						*/
/*		pSemaphore : pointer to the mutex to lock													*/
/*																									*/
/* RETURN VALUE:																					*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void FF_PendSemaphore(void *pSemaphore)
{
#if ((OX_SEM_PARKING) != 0)
  MTX_t *Mutex = (MTX_t *)pSemaphore;
#else
  MTX_t *Mutex = ((MtxList_t *)pSemaphore)->Mutex;
#endif

	if (0 != MTXlock(Mutex, -1)) {					/* Although infinte wait, check in case			*/
	  #if ((OX_OUT_OF_MEM) != 0)					/* deadlock detection is used					*/
		OSintOff();
		for(;;);
	  #else
		return;
	  #endif
	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    FF_ReleaseSemaphore																	*/
/*																									*/
/* FF_ReleaseSemaphore - unlock a mutex																*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void FF_ReleaseSemaphore(void *pSemaphore);													*/
/*																									*/
/* ARGUMENTS:																						*/
/*		pSemaphore : pointer to the mutex to unlock													*/
/*																									*/
/* RETURN VALUE:																					*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void FF_ReleaseSemaphore(void *pSemaphore)
{
#if ((OX_SEM_PARKING) != 0)
  MTX_t *Mutex = (MTX_t *)pSemaphore;
#else
  MTX_t *Mutex = ((MtxList_t *)pSemaphore)->Mutex;
#endif

	MTXunlock(Mutex);

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    FF_Sleep																			*/
/*																									*/
/* FF_Sleep - go to sleep for a specified time														*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void FF_Sleep(FF_T_UINT32 TimeMs);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		TimeMs : sleep time in ms																	*/
/*																									*/
/* RETURN VALUE:																					*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		Uses TSKsleep() but also makes sure there are at least 2 timer ticks of sleep				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void FF_Sleep(FF_T_UINT32 TimeMs)
{

	TSKsleep(OS_MS_TO_MIN_TICK(TimeMs, 2));

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    FF_Yield																			*/
/*																									*/
/* FF_Yield - yield the CPU to another task															*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void FF_Yield(void);																		*/
/*																									*/
/* ARGUMENTS:																						*/
/*																									*/
/* RETURN VALUE:																					*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*		Not a true yield because TSKyield() only works on task involved in Round-Robin or when 		*/
/* 		Abassi is configured to emulate a cooperative RTOS											*/
/*		It puts the task to sleep for 100ms using TSKsleep() but also makes sure there are at		*/
/*		least 2 timer ticks of sleep																*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void FF_Yield(void)
{

	TSKsleep(OS_MS_TO_MIN_TICK(100, 2));

	return;
}

/* ------------------------------------------------------------------------------------------------ */

void *FFalloc(size_t Size)
{
void *Ret;


  #if ((OX_LIB_REENT_PROTECT) == 0)					/* If lib prot, a mute is already used			*/
	if (0 != MTXlock(G_OSmutex, -1)) {				/* Although infinte wait, check in case			*/
		return((void *)NULL);						/* deadlock detection is used					*/
	}
  #endif

	Ret = malloc(Size);

  #if ((OX_LIB_REENT_PROTECT) == 0)
	MTXunlock(G_OSmutex);
  #endif

	return(Ret);
}

/* ------------------------------------------------------------------------------------------------ */

void FFfree(void *Ptr)
{

  #if ((OX_LIB_REENT_PROTECT) == 0)					/* If lib prot, a mute is already used			*/
	if (0 != MTXlock(G_OSmutex, -1)) {				/* Although infinte wait, check in case			*/
	  #if ((OX_OUT_OF_MEM) != 0)					/* deadlock detection is used					*/
		OSintOff();
		for(;;);
	  #else
		return;
	  #endif
	}
  #endif

	free(Ptr);

  #if ((OX_LIB_REENT_PROTECT) == 0)
	MTXunlock(G_OSmutex);
  #endif

	return;
}
/* EOF */
