/* ------------------------------------------------------------------------------------------------ */
/* FILE :		Abassi_FatFS.c																		*/
/*																									*/
/* CONTENTS :																						*/
/*				Original FatFS syscall.c Modified to work with Abassi								*/
/*				Contains the I/F with Abassi for the mutex access protection & memory allocation	*/
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
/*	$Revision: 1.13 $																				*/
/*	$Date: 2019/01/10 18:06:18 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "ff.h"										/* Needed for fct prototypes but also for the	*/
													/* correct Abassi.h / mAbassi.h / uAbassi.h		*/
#if _FS_REENTRANT									/* None of these needed if _FS_REENTRANT false	*/

/* ------------------------------------------------------------------------------------------------ */

#if ((OX_RUNTIME) == 0)
  #error "FatFS requires run-time service creation. Set OS_RUNTIME to non-zero"
#endif

/* ------------------------------------------------------------------------------------------------ */

static MTX_t *g_Parking[_VOLUMES];					/* Holds the mutexes that were deleted			*/
static int    g_IsInitPark=0;						/* Flags to remember if parking was initialized	*/

static const char g_Names[][8] = {
                                 "FatFS-0"
                               #if ((_VOLUMES) > 1)
                                 ,"FatFS-1"
                               #endif 
                               #if ((_VOLUMES) > 2)
                                 ,"FatFS-2"
                               #endif 
                               #if ((_VOLUMES) > 3)
                                 ,"FatFS-3"
                               #endif 
                               #if ((_VOLUMES) > 4)
                                 ,"FatFS-4"
                               #endif 
                               #if ((_VOLUMES) > 5)
                                 ,"FatFS-5"
                               #endif 
                               #if ((_VOLUMES) > 6)
                                 ,"FatFS-6"
                               #endif 
                               #if ((_VOLUMES) > 7)
                                 ,"FatFS-7"
                               #endif 
                               };

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    ff_cre_syncobj																		*/
/*																									*/
/* ff_cre_syncobj - create a mutex																	*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int ff_cre_syncobj(BYTE vol, _SYNC_t *sobj);												*/
/*																									*/
/* ARGUMENTS:																						*/
/*		vol   : drive (there is 1 mutex per drive)													*/
/*		_sobj : pointer to the created mutex														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 1 : success																				*/
/*		== 0 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/*		if g_Parking[vol] == NULL																	*/
/*			g_Parking[vol] = MTXopen()																*/
/* ------------------------------------------------------------------------------------------------ */

int ff_cre_syncobj(BYTE vol, _SYNC_t *sobj)
{
int ii;												/* Loop counter and index						*/

	if (g_IsInitPark == 0) {						/* No need for critical region if initialized	*/
		if (0 != MTXlock(G_OSmutex, -1)) {			/* Although infinite wait, check in case		*/
			return(0);								/* deadlock detection is used					*/
		} 
		if (g_IsInitPark == 0) {					/* Re-check as it was done before protection	*/
			for (ii=0 ; ii<_VOLUMES ; ii++) {		/* Init with all NULLs (no mutex in the parking)*/
				g_Parking[ii] = (MTX_t *)NULL;
			}
			g_IsInitPark = 1;
		}
		MTXunlock(G_OSmutex);
	}

	if (g_Parking[vol] == (MTX_t *)NULL) {			/* No mutex has been created for this volume	*/
		g_Parking[vol] = MTXopen(&g_Names[vol][0]);	/* Open a new one								*/
	}

	*sobj = g_Parking[vol];							/* Return the mutex descriptor					*/

	return((*sobj != (MTX_t *)NULL));				/* TRUE if opened, FALSE if not opened			*/
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    ff_del_syncobj																		*/
/*																									*/
/* ff_del_syncobj - delete a mutex																	*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int ff_del_syncobj(_SYNC_t sobj);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		_sobj : pointer to the mutex to delete														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 1 : success																				*/
/*		== 0 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/*		return(MTX->Value == 1)																		*/
/* ------------------------------------------------------------------------------------------------ */

int ff_del_syncobj(_SYNC_t sobj)
{													/* The mutex value should be 1 otherwise there	*/
	return(sobj->Value == 1);						/* is still a task accessing the volume			*/
}													/* Do nothing else. Mutex held in parking lot	*/

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    ff_req_grant																		*/
/*																									*/
/* ff_req_grant - lock a mutex																		*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int ff_req_grant(_SYNC_t sobj);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		_sobj : pointer to the mutex to lock														*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 1 : success																				*/
/*		== 0 : error																				*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/*		return(MTX->Value == 1)																		*/
/* ------------------------------------------------------------------------------------------------ */

int ff_req_grant(_SYNC_t sobj)
{													/* Abassi returns 0 upon success.				*/
	return(!MTXlock(sobj, _FS_TIMEOUT));			/* Need to return TRUE (!=0) upon success		*/
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    ff_rel_grant																		*/
/*																									*/
/* ff_rel_grant - unlock a mutex																	*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int ff_rel_grant(_SYNC_t sobj);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		_sobj : pointer to the mutex to unlock														*/
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

void ff_rel_grant(_SYNC_t sobj)
{
	MTXunlock(sobj);

	return;
}
#endif

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */

#if _USE_LFN == 3										/* LFN with a working buffer on the heap	*/

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    ff_memalloc																			*/
/*																									*/
/* ff_memalloc - allocate memory																	*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void *ff_memalloc(UINT msize);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		msize : number of bytes to allocate															*/
/*																									*/
/* RETURN VALUE:																					*/
/*		!= NULL : allocated memory area																*/
/*		== NULL : out of memory																		*/
/*																									*/
/* DESCRIPTION:																						*/
/*																									*/
/* NOTE:																							*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

void *ff_memalloc(UINT msize)
{
void *Ptr;

  #if ((OX_LIB_REENT_PROTECT) == 0)					/* If lib re-entrance protection used no meed	*/
	if (0 != MTXlock(G_OSmutex, -1)) {				/* Although infinte wait, check in case			*/
		return((void *)NULL);						/* deadlock detection is used					*/
	}
  #endif

	Ptr = malloc((size_t)msize);

  #if ((OX_LIB_REENT_PROTECT) == 0)
	MTXunlock(G_OSmutex);
  #endif

	return(Ptr);
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    ff_memfree																			*/
/*																									*/
/* ff_memfree - free a previously allocated memory block											*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void ff_memfree(void *mblock);																*/
/*																									*/
/* ARGUMENTS:																						*/
/*		mblock : pointer to the memory block to free												*/
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

void ff_memfree(void *mblock)
{
  #if ((OX_LIB_REENT_PROTECT) == 0)					/* If lib re-entrance protection used no meed	*/
	if (0 != MTXlock(G_OSmutex, -1)) {				/* Although infinte wait, check in case			*/
	  #if ((OX_OUT_OF_MEM) != 0)					/* deadlock detection is used					*/
		OSintOff();
		for(;;);
	  #else
		return;
	  #endif
	}
  #endif

	free(mblock);

  #if ((OX_LIB_REENT_PROTECT) == 0)
	MTXunlock(G_OSmutex);
  #endif

	return;
}
#endif

/* EOF */
