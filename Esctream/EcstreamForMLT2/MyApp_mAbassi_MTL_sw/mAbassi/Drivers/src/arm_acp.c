/* ------------------------------------------------------------------------------------------------ */
/* FILE :		arm_acp.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Simple interface to enable ACP on memory transfers on the ARM processors			*/
/*																									*/
/*																									*/
/* Copyright (c) 2017-2019, Code-Time Technologies Inc. All rights reserved.						*/
/*																									*/
/* Redistribution and use in source and binary forms, with or without								*/
/* modification, are permitted provided that the following conditions are met:						*/
/*																									*/
/* 1. Redistributions of source code must retain the above copyright notice, this					*/
/*    list of conditions and the following disclaimer.												*/
/* 2. Redistributions in binary form must reproduce the above copyright notice,						*/
/*    this list of conditions and the following disclaimer in the documentation						*/
/*    and/or other materials provided with the distribution.										*/
/*																									*/
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND					*/
/* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED					*/
/* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE							*/
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR					*/
/* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES					*/
/* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;						*/
/* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND						*/
/* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT						*/
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS					*/
/* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.										*/
/*																									*/
/*																									*/
/*	$Revision: 1.6 $																				*/
/*	$Date: 2019/01/10 18:07:05 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* NOTES:																							*/
/*																									*/
/* LIMITATIONS:																						*/
/*																									*/
/* NOT YET SUPPORTED:																				*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "arm_acp.h"

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: acp_enable																				*/
/*																									*/
/* acp_enable - enable the use of ACP on memory transfers											*/
/*																									*/
/* SYNOPSIS:																						*/
/*		uint32_t acp_enable(int VID, int MID, inr Page, in IsRead)									*/
/*																									*/
/* ARGUMENTS:																						*/
/*		VID    : virtual ID																			*/
/*		         when -ve, enable the dynamic page													*/
/*		MID    : Master ID																			*/
/*		         -ve : disable the corresponding virtual ID	/ IsRead is taken into account			*/
/*		Page   : memory page to dynamicaly apply ACP on												*/
/*		         -ve : disable all / IsRead is taken into account									*/
/*		IsRead : == 0 : enable ACP on memory writes													*/
/*		         != 0 : enable ACP on memory reads													*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0xFFFFFFFF : error / not supported														*/
/*		!= 0xFFFFFFFF : 2 MSbit of the ACP remapped page 											*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Cyclone V & Arria V																				*/
#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)

uint32_t acp_enable(int VID, int MID, int Page, int IsRead)
{
int                ii;								/* General purpose								*/
volatile uint32_t *Reg;								/* Pointer to the Register to update			*/
uint32_t           RetVal;							/* Return value									*/
uint32_t           Tmp32;							/* Temporary 32 bit								*/

	Reg    = (uint32_t *)0xFF707000;				/* Base address of all ACP mapper registers		*/
	Reg   += (IsRead == 0);							/* Write is Reg[ii] & read is Reg[ii+1]			*/
	RetVal = 0x00000000;							/* Assume will be a disable reguest				*/
	Tmp32  = 0x00000000;							/* Assume will be a disable reguest				*/

	if (Page < 0) {									/* Disable all									*/
		for (ii=0*2 ; ii<5*2 ; ii++) {
			Reg[ii] = Tmp32;						/* Virtual ID 3,4,5,6,Dynamic					*/
			while(Tmp32 != (Reg[12+ii] & 0x8FFF31F0));
		}

		Reg[10] = Tmp32;							/* Dynamic requires a different mask			*/
		while(Tmp32 != (Reg[12] & 0x000031F0));
	
		Tmp32 = 0x80040010;							/* Virtual ID 2 has a different reset value		*/
		Reg[0] = Tmp32;
		while(Tmp32 != (Reg[12] & 0x8FFF31F0));
	}
	else if (VID < 0) {								/* Dynamic Virtual ID re-map					*/
		if (MID >= 0) {								/* -ve MID is disable (value to set is 0		*/
			Tmp32   = (Page << 12)
			        | 0x000001F0;
			RetVal = 0x80000000;					/* Selected Page always remapped at 0x80000000	*/
		}
		Reg[10] = Tmp32;
		while(Tmp32 != (Reg[22] & 0x000031F0));
	}
	else if ((VID>=2) && (VID<=6)) {
		VID      = (VID-2)
		         * 2;
		if (MID >= 0) {								/* -ve MID is disable (value to set is 0		*/
			Tmp32    = 0x00000000
			         | (MID <<16)
			         | (Page << 12)
		    	     | 0x000001F0;
			RetVal = 0x80000000;					/* Selected Page always remapped at 0x80000000	*/
		}
		Reg[VID] = Tmp32;
		while(Tmp32 != (Reg[12+VID] & 0x8FFF31F0));
	}
	else {
		RetVal = 0xFFFFFFFF;
	}

	return(RetVal);
}

/* ------------------------------------------------------------------------------------------------ */
/* All others: report unsupported																	*/
#else

uint32_t acp_enable(int VID, int MID, int Page, int IsRead)
{
	return(0xFFFFFFFF);
}

#endif

/* EOF */
