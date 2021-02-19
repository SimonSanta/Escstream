/* ------------------------------------------------------------------------------------------------ */
/* FILE :        sys_arch.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*               All typedef, definitions and function prototypes for the							*/
/*               layer between lwIP and Abassi														*/
/*																									*/
/*																									*/
/* Copyright (c) 2013-2016, Code-Time Technologies Inc. All rights reserved.						*/
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
/*  $Revision: 1.5 $																				*/
/*	$Date: 2018/11/07 16:12:26 $																	*/
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
#ifndef __SYS_ARCH_H__
#define __SYS_ARCH_H__

#include "lwipopts.h"

#ifdef __UABASSI_H__							/* uAbassi does not support mailboxes, use add-on	*/
  #include "Xtra_uAbassi.h"
#endif

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef SYS_DEFAULT_THREAD_STACK_DEPTH
  #define SYS_DEFAULT_THREAD_STACK_DEPTH	1024
#endif

typedef MBX_t *sys_mbox_t;
typedef MTX_t *sys_mutex_t;
typedef SEM_t *sys_sem_t;
typedef TSK_t *sys_thread_t;

#define sys_mbox_set_invalid(x)  (*(x)  = (sys_mbox_t) NULL)
#define sys_mbox_valid(x)        (*(x) != (sys_mbox_t) NULL)
#define sys_mutex_set_invalid(x) (*(x)  = (sys_mutex_t)NULL)
#define sys_mutex_valid(x)       (*(x) != (sys_mutex_t)NULL)
#define sys_sem_set_invalid(x)   (*(x)  = (sys_sem_t)  NULL)
#define sys_sem_valid(x)         (*(x) != (sys_sem_t)  NULL)

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
