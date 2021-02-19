/* ------------------------------------------------------------------------------------------------ */
/* FILE :		Abassi_ueFAT.c																		*/
/*																									*/
/* CONTENTS :																						*/
/*				Contains the I/F with Abassi for the mutex access protection for ueFAT				*/
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
/*	$Revision: 1.6 $																				*/
/*	$Date: 2019/01/10 18:06:18 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "fat_opts.h"								/* Needed for fct prototypes but also for the	*/
													/* correct Abassi.h / mAbassi.h / uAbassi.h		*/
/* ------------------------------------------------------------------------------------------------ */

#if ((OX_RUNTIME) == 0)
  #error "ueFAT requires run-time service creation. Set OS_RUNTIME to non-zero"
#endif

/* ------------------------------------------------------------------------------------------------ */

static MTX_t *g_MyMtx = (MTX_t *)NULL;				/* Holds the mutex that was deleted				*/

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    fl_init_locks																		*/
/*																									*/
/* fl_init_locks - init the mutex																	*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void fl_init_locks(void);																	*/
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

void fl_init_locks(void)
{
	if (g_MyMtx == (MTX_t *)NULL) {					/* Init only once								*/
		g_MyMtx = MTXopen("ueFAT");
	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION:    fl_lock																				*/
/*																									*/
/* fl_lock - lock the mutex																			*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void fl_lock(void);																			*/
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

void fl_lock(void)
{
	if (0 != MTXlock(g_MyMtx, -1)) {				/* Although infinte wait, check in case			*/
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
/* FUNCTION:    fl_unlock																			*/
/*																									*/
/* fl_unlock - unlock the mutex																		*/
/*																									*/
/* SYNOPSIS:																						*/
/*		void fl_unlock(void);																		*/
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

void fl_unlock(void)
{
	MTXunlock(g_MyMtx);
	return;
}

/* EOF */
