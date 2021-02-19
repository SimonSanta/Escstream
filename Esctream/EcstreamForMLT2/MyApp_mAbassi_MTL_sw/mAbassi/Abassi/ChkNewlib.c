/* ------------------------------------------------------------------------------------------------ */
/* FILE :		ChkNewlib.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				For lib based releases, to check if the NewLib version match with build				*/
/*																									*/
/*																									*/
/* Copyright (c) 2018-2019, Code-Time Technologies Inc. All rights reserved.						*/
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
/*	$Revision: 1.3 $																				*/
/*	$Date: 2019/01/10 18:06:18 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* INFO:																							*/
/*		ChkNewlib.c is part of the Abassi library.  If Absssi is used in source form, there is		*/
/*		no need to check the version of Newlib becausde all code generated uses the same version	*/
/*		of Newlib.																					*/
/*		The need to check the version of the Newlib is to make sure the version used to build		*/
/*		the application is the same as the one used to build the Abassi library.  It sometimes		*/
/*		happens data structures are different between versions and mismatches will make the Abassi	*/
/*		library code incompatible with the application.  This is mainly for library re-entrance		*/
/*		protection.																					*/
/*																									*/
/*		DO NOT INCLUDE ChkNewlib.c in your application as it will overload the one in the			*/
/*		library and always report OK. To check if the versions match, add this code in your			*/
/*		application:																				*/
/*																									*/
/*		#include <_newlib_version.h>																*/
/*		extern int ChkNewlib(const char *InUse);													*/
/*		if (0 == ChkNewlib(_NEWLIB_VERSION)) {														*/
/*			---> LIBARY MISMATCH																	*/
/*		}																							*/
/*																									*/
/*		A macro could have been defined in Abassi include file but the file _newlib_version.h has	*/
/*		not always been there and an application with Abassi source built with older Newlib			*/
/*		version, although without version mismatch issues, would fail due to the file				*/
/*		_newlib_version.h not found error.															*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include <_newlib_version.h>
#include <string.h>

/* ------------------------------------------------------------------------------------------------ */
/* FUNCTION: ChkNewlib																				*/
/*																									*/
/* ChkNewlib - check if version of NewLib used in the mAbassi library is the same as the app		*/
/*																									*/
/* SYNOPSIS:																						*/
/*		int ChkNewlib(const char *InUse);															*/
/*																									*/
/* ARGUMENTS:																						*/
/*		InUse : Newline version (string: _NEWLIB_VERSION) use by the application					*/
/*																									*/
/* RETURN VALUE:																					*/
/*		== 0 : different																			*/
/*		!= 0 : same																					*/
/*																									*/
/* IMPLEMENTATION:																					*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

int ChkNewlib(const char *InUse)
{
	return(0 == strcmp(_NEWLIB_VERSION, InUse));
}

/* EOF */
