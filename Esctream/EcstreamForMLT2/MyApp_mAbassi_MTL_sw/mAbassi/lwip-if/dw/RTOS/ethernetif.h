/* ------------------------------------------------------------------------------------------------ */
/* FILE :        ethernetif.h																		*/
/*																									*/
/* CONTENTS :																						*/
/*               Defintion file for the lwIP ethernet interface for the Arria 5/10 & Cyclone V		*/
/*																									*/
/*																									*/
/* Copyright (c) 2016-2018, Code-Time Technologies Inc. All rights reserved.						*/
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
/*  $Revision: 1.87 $																				*/
/*	$Date: 2018/08/13 17:25:07 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#include "lwip/err.h"
#include "lwip/netif.h"

#ifdef __cplusplus
 extern "C" {
#endif

extern err_t (*ethernet_init[5])(struct netif *netif);

extern void Emac0_IRQHandler(void);
extern void Emac1_IRQHandler(void);
extern void Emac2_IRQHandler(void);
extern void Emac3_IRQHandler(void);
extern void Emac4_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif 

/* EOF */
