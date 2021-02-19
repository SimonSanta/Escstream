/* ------------------------------------------------------------------------------------------------ */
/* FILE :		WebApp.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Webserver application defintions													*/
/*				includes lwip & DHCP client task information.										*/
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
/*	$Revision: 1.20 $																				*/
/*	$Date: 2019/01/10 18:07:13 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __WEBAPP_H
#define __WEBAPP_H

#include <string.h>

#ifndef OS_CMSIS_RTOS
 #if !defined(__ABASSI_H__) && !defined(__MABASSI_H__) && !defined(__UABASSI_H__)
  #if defined(_STANDALONE_)						 	/* It is the same include file in all cases.	*/
    #include "mAbassi.h"								/* There is a substitution during the release 	*/
  #elif defined(_UABASSI_)							/* This file is the same for Abassi, mAbassi,	*/
    #include "mAbassi.h"							/* and uAbassi, stamdalone so Code Time uses	*/
  #elif defined (OS_N_CORE)							/* these checks to not have to keep 4 quasi		*/
    #include "mAbassi.h"							/* identical copies of this file				*/
  #else
    #include "mAbassi.h"
  #endif
 #endif
#else
  #include "cmsis_os.h"
#endif

#ifdef __UABASSI_H__
  #include "Xtra_uAbassi.h"							/* For Events and Mailbox emulation				*/
#endif

#include "SysCall.h"								/* System call layer definitions				*/
#include "Platform.h"								/* Everything about the target platform is here	*/
#include "HWinfo.h"									/* Everything about the target board is here	*/
#include "WebServer.h"

#if (((OS_PLATFORM) & 0x00FFFFFF) == 0x0000AAC5)		\
 || (((OS_PLATFORM) & 0x00FFFFFF) == 0x0000AA10)
  #include "alt_gpio.h"
  #include "dw_ethernet.h"
  #include "dw_i2c.h"
  #include "dw_sdmmc.h"
  #include "dw_uart.h"
#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00007020)		\
   || (((OS_PLATFORM) & 0x00FFFFFF) == 0x00007753)
  #include "cd_i2c.h"
  #include "cd_uart.h"
  #include "xlx_ethernet.h"
  #include "xlx_gpio.h"
  #include "xlx_sdmmc.h"
#elif (((OS_PLATFORM) & 0x00FFFF00) == 0x0032F100)	/* All STM32F1xx devices						*/
  #include "stm32_eval.h"
  #include "arm_comm.h"
#elif (((OS_PLATFORM) & 0x00FFFF00) == 0x0032F200)
  #define UART_FILTER		(UART_PIN_PACK | UART_FILT_OUT_LF_CRLF | UART_FILT_IN_CR_LF | UART_ECHO)
  #include "stm32_uart.h"
  #include "stm32f2xx.h"
  extern void ETH_IRQHandler(void);					/* Ethernet I/F interrupt handler				*/
  extern void EXTI15_10_IRQHandler(void);			/* Ethernet I/F interrupt handler				*/
 #if ((OS_PLATFORM) == 0x0032F207)					/* STM3220G Evaluation board					*/
  #include "stm322xg_eval_lcd.h"
  #include "stm322xg_eval_ioe.h"
  #include "stm32f2x7_eth_bsp.h"
  extern void SD_ProcessDMAIRQ(void);				/* micro SD I/F interrupt handler				*/
  extern void SD_ProcessIRQSrc(void);				/* micro SD I/F interrupt handler				*/
 #endif
#elif (((OS_PLATFORM) & 0x00FFFF00) == 0x0032F400)
  #define UART_FILTER		(UART_PIN_PACK | UART_FILT_OUT_LF_CRLF | UART_FILT_IN_CR_LF | UART_ECHO)
  #include "stm32_uart.h"
  #include "stm32f4xx.h"
  #include "stm32f4xx_rtc.h"
  extern void ETH_IRQHandler(void);					/* Ethernet I/F interrupt handler				*/
  extern void EXTI15_10_IRQHandler(void);			/* Ethernet I/F interrupt handler				*/
 #if ((OS_PLATFORM) == 0x0032F407)
  #include "stm32f4_discovery.h"
  #include "stm32f4x7_eth_bsp.h"
  #include "stm32f4_discovery_sdio_sd.h"
 #elif ((OS_PLATFORM) == 0x1032F407)				/* Olimex STM-P407								*/
  #include "stm32_p407.h"
  #include "drv_glcd_cnfg.h"
  #include "drv_glcd.h"
  #include "glcd_ll_st.h"
  extern FontType_t Terminal_9_12_6;
 #endif
#elif (((OS_PLATFORM) & 0x00FF0000) == 0x003C0000)	/* NXP LPCxxxx devices							*/
 #if ((OS_PLATFORM) == 0x003C1124)					/* NGX Blue Board LPC11U24						*/
  #include "system_LPC11Uxx.h"
  #include "glcd_api.h"

 #elif ((OS_PLATFORM) == 0x003C1227)				/* Olimex LPC1227-STK							*/
  #include "lpc12xx_sysctrl.h"
  #include "lpc12xx_iocon.h"

 #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x003C1766)
  #define UART_FILTER		(UART_FILT_OUT_LF_CRLF | UART_FILT_IN_CR_LF)
  #include "lpc_uart.h"
  #include "drv_glcd_cnfg.h"
  #include "drv_glcd.h"
  #include "glcd_ll.h"
  extern FontType_t Terminal_9_12_6;
 #endif
#endif

#include "ethernetif.h"
#include "lwip/api.h"
#include "lwip/arch.h"
#include "lwip/def.h"
#include "lwip/opt.h"
#include "lwip/dhcp.h"
#include "tcp.h"
#include "tcpip.h"
#include "netif/etharp.h"
#include "lwipopts.h"

#ifdef __cplusplus
 extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------ */

#ifndef WEBS_PRIO
 #ifndef osCMSIS
  #define WEBS_PRIO					(OX_PRIO_MIN-1)	/* Priority of the HTTP WebServer task			*/
 #else
  #define WEBS_PRIO					osPriorityLow
 #endif
#endif

#ifndef WEBS_STACKSIZE
  #define WEBS_STACKSIZE			8192			/* Stack size of the HTTP WebServer task		*/
#endif

/* ------------------------------------------------------------------------------------------------ */

#if BYTE_ORDER == BIG_ENDIAN
 #define IP4_INT32(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))
#else
 #define IP4_INT32(a,b,c,d) (((d)<<24)|((c)<<16)|((b)<<8)|(a))
#endif

#ifndef IP_ADDR0
  #define IP_ADDR0        192						/* IP Address									*/
#endif
#ifndef IP_ADDR1
  #define IP_ADDR1        168						/* Lands in G_IPnetDefIP located in the file	*/
#endif
#ifndef IP_ADDR2
  #define IP_ADDR2          1						/* NetAddr.c									*/
#endif
#ifndef IP_ADDR3
  #define IP_ADDR3         10						/* Currently set in main()						*/
#endif

#ifndef NETMASK_ADDR0
  #define NETMASK_ADDR0   255						/* Net Mask										*/
#endif
#ifndef NETMASK_ADDR1
  #define NETMASK_ADDR1   255						/* Lands in G_IPnetDefNM located in the file	*/
#endif
#ifndef NETMASK_ADDR2
  #define NETMASK_ADDR2   255						/* NetAddr.c									*/
#endif
#ifndef NETMASK_ADDR3
  #define NETMASK_ADDR3     0						/* Currently set in main()						*/
#endif

#ifndef GATEWAY_ADDR0
  #define GATEWAY_ADDR0   192						/* Gateway address								*/
#endif
#ifndef GATEWAY_ADDR1
  #define GATEWAY_ADDR1   168						/* Lands in G_IPnetDefGW located in the file	*/
#endif
#ifndef GATEWAY_ADDR2
  #define GATEWAY_ADDR2     1						/* NetAddr.c									*/
#endif
#ifndef GATEWAY_ADDR3
  #define GATEWAY_ADDR3     1						/* Currently set in main()						*/
#endif

#define MAC0_ADDR0   0x02							/* MAC address MSByte for EMAC device #0		*/
#define MAC0_ADDR1   0x00							/* If available from hardware, modify the		*/
#define MAC0_ADDR2   0x00							/* the function GetMACaddr() located in the		*/
#define MAC0_ADDR3   0x00							/* file NetAddr.c								*/
#define MAC0_ADDR4   0x00
#define MAC0_ADDR5   0x00							/* MAC address LSByte							*/

#define MAC1_ADDR0   0x06							/* MAC address MSByte for EMAC device #1		*/
#define MAC1_ADDR1   0x00							/* If available from hardware, modify the		*/
#define MAC1_ADDR2   0x00							/* the function GetMACaddr() located in the		*/
#define MAC1_ADDR3   0x00							/* file NetAddr.c								*/
#define MAC1_ADDR4   0x00
#define MAC1_ADDR5   0x00							/* MAC address LSByte							*/

#define MAC2_ADDR0   0x0A							/* MAC address MSByte for EMAC device #2		*/
#define MAC2_ADDR1   0x00							/* If available from hardware, modify the		*/
#define MAC2_ADDR2   0x00							/* the function GetMACaddr() located in the		*/
#define MAC2_ADDR3   0x00							/* file NetAddr.c								*/
#define MAC2_ADDR4   0x00
#define MAC2_ADDR5   0x00							/* MAC address LSByte							*/

#define MAC3_ADDR0   0x0E							/* MAC address MSByte for EMAC device #2		*/
#define MAC3_ADDR1   0x00							/* If available from hardware, modify the		*/
#define MAC3_ADDR2   0x00							/* the function GetMACaddr() located in the		*/
#define MAC3_ADDR3   0x00							/* file NetAddr.c								*/
#define MAC3_ADDR4   0x00
#define MAC3_ADDR5   0x00							/* MAC address LSByte							*/

#define MII_MODE

/* ------------------------------------------------------------------------------------------------ */

#ifndef TEST_RXPERF
  #define TEST_RXPERF	0
#endif
#ifndef TEST_TXPERF
  #define TEST_TXPERF	0
#endif

/* ------------------------------------------------------------------------------------------------ */

#if ((OX_LIB_REENT_PROTECT) <= 0)					/* If library not protected against reentrance	*/
 #ifndef osCMSIS									/* Mutex to protect stdio mutex operations		*/
  #define MTXLOCK_ALLOC()		MTXlock(G_OSmutex, -1)
  #define MTXUNLOCK_ALLOC()		MTXunlock(G_OSmutex)
  #define MTXLOCK_STDIO()		MTXlock(G_OSmutex, -1)
  #define MTXUNLOCK_STDIO()		MTXunlock(G_OSmutex)
 #else
  #define MTXLOCK_ALLOC()		osMutexWait(G_OSmutex, osWaitForever)
  #define MTXUNLOCK_ALLOC()		osMutexRelease(G_OSmutex)
  #define MTXLOCK_STDIO()		osMutexWait(G_OSmutex, osWaitForever)
  #define MTXUNLOCK_STDIO()		osMutexRelease(G_OSmutex)
 #endif
#else												/* Lib is safe, no need for extra protection	*/
  #define MTXLOCK_ALLOC()		do {int _=0;_=_;} while(0)
  #define MTXUNLOCK_ALLOC()		do {int _=0;_=_;} while(0)
  #define MTXLOCK_STDIO()		do {int _=0;_=_;} while(0)
  #define MTXUNLOCK_STDIO()		do {int _=0;_=_;} while(0)
#endif

/* ------------------------------------------------------------------------------------------------ */

extern u32_t G_IPnetDefGW;							/* Default static default gateway address		*/
extern u32_t G_IPnetDefIP;							/* Default static default IP address			*/
extern u32_t G_IPnetDefNM;							/* Default net mask								*/
extern int   G_IPnetStatic;							/* If using static or dynamic IP address		*/

#if ((OX_EVENTS) == 0)
  extern volatile int G_WebEvents;
#endif

/* ------------------------------------------------------------------------------------------------ */

extern void GetMACaddr(int Dev, u8_t MACaddr[]);
#ifdef osCMSIS
  void LwIP_DHCP_task(const void *Arg);
#else
  void LwIP_DHCP_task(void);
#endif
void LwIP_Init(void);
void NetAddr_Init(void);
void PrtAddr(char *Str, u32_t Addr);

/* ------------------------------------------------------------------------------------------------ */
/* File system specifics																			*/
/* ------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------ */
/* In-memory file system																			*/

#if (((OS_DEMO) ==  10) || ((OS_DEMO) ==  11) || ((OS_DEMO) ==  110) || ((OS_DEMO) ==  111)			\
 ||  ((OS_DEMO) == -10) || ((OS_DEMO) == -11) || ((OS_DEMO) == -110) || ((OS_DEMO) == -111))
  #include "fs.h"									/* In-memory FS defintion file					*/
  typedef struct fs_file	*FILE_DSC_t;

  #define F_INIT_FDSC(x)						((x)=NULL)
  #define F_MNT()                               0
  #define F_OPEN(F_Dsc, F_Name)					(NULL != ((F_Dsc)=fs_open(F_Name)))
  #define F_CLOSE(F_Dsc)						fs_close(F_Dsc)
  #define F_READ(F_Dsc, Buf, BufSize, Nread)														\
						if (-1 == (*(Nread)=fs_read((F_Dsc), (char *)Buf, (BufSize)))) {			\
							*(Nread) = 0;															\
						}
  #define F_PATCH_OPEN(F_Dsc)					do {(F_Dsc)->index = 0;} while(0)
#endif

/* ------------------------------------------------------------------------------------------------ */
/* FatFS directly used																				*/

#if (((OS_DEMO) ==  12) || ((OS_DEMO) ==  13) || ((OS_DEMO) ==  112) || ((OS_DEMO) ==  113)			\
 ||  ((OS_DEMO) == -12) || ((OS_DEMO) == -13) || ((OS_DEMO) == -112) || ((OS_DEMO) == -113))
  #include "ff.h"									/* FatFS defintion file							*/
  typedef FIL FILE_DSC_t;

  #define F_INIT_FDSC(x)
  #define F_MNT()								f_mount(&g_FileSys, "0", 1)
  #define F_OPEN(F_Dsc, F_Name)					(FR_OK == (f_open(&(F_Dsc),(F_Name), FA_READ)))
  #define F_CLOSE(F_Dsc)						f_close(&(F_Dsc))
  #define F_READ(F_Dsc, Buf, BufSize, Nrd)  	f_read(&(F_Dsc),(void *)(Buf),(BufSize),(UINT *)(Nrd))
  #define F_PATCH_OPEN(F_Dsc)					do{int _=0;_=_;}while(0)
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Uses System Layer call for standard UNIX style I/O API											*/

#if (((((OS_DEMO) >=  14) && ((OS_DEMO) <=   19)) ||  ((OS_DEMO) >=  114) && ((OS_DEMO) <=  119))	\
 ||  ((((OS_DEMO) <= -14) && ((OS_DEMO) >= -119)) ||  ((OS_DEMO) <= -114) && ((OS_DEMO) >= -119)))
  #include "SysCall.h"								/* File systen accessed with system call layer	*/
  typedef FILE *FILE_DSC_t;

  #define F_INIT_FDSC(x)						((x)=NULL)
  #define F_MNT()								mount(FS_TYPE_NAME_AUTO, "/", 0 , "0:")
  #define F_OPEN(F_Dsc, F_Name)					(NULL != ((F_Dsc) = fopen((F_Name), "r")))
  #define F_CLOSE(F_Dsc)						fclose((F_Dsc))
  #define F_READ(F_Dsc, Buf, BufSize, Nread)	(*(Nread))=fread((void *)(Buf),1,(BufSize),(F_Dsc))
  #define F_PATCH_OPEN(F_Dsc)					do{int _=0;_=_;}while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
