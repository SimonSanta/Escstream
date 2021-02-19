/* ------------------------------------------------------------------------------------------------ */
/* FILE :		lwipopts.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Application specific definitions for lwIP											*/
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
/*	$Revision: 1.22 $																				*/
/*	$Date: 2019/01/10 18:07:14 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* NOTE:																							*/
/* This file contains many key lwIP defintions that allows to tune lwIP for the target platform		*/
/* and the application needs.  All defintions are in-between #ifndef ... #endif.					*/
/* The best practice is to use the maklefile / GUI to re-define the default value instead of		*/
/* changing the value in here.  Changing values in here means sfuture updates on the file will		*/
/* require a manual merge.																			*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

/* ------------------------------------------------------------------------------------------------ */
/* Target include file selection																	*/

#ifndef OS_CMSIS_RTOS
 #if defined(_STANDALONE_)
  #include "SAL.H"
 #elif !defined(__ABASSI_H__) && !defined(__MABASSI_H__) && !defined(__UABASSI_H__)
  #if defined(_UABASSI_)							/* This file is shared by all Code Time RTOSes	*/
	#include "mAbassi.h"							/* All these conditionals are here to pick the	*/
  #elif defined (OS_N_CORE)							/* the correct RTOS include file				*/
	#include "mAbassi.h"							/* In the release code, the files included are	*/
  #else												/* substituted at packaging time with the one	*/
	#include "mAbassi.h"								/* of the release								*/
  #endif
 #endif
#else
  #include "cmsis_os.h"
#endif

#ifdef __CC_ARM
  #include <errno.h>								/* lwIP Ver 2 socket.c needs this for ARM CC	*/
#endif

#ifndef _STANDALONE_
 #include "init.h"									/* Needed to extract the version				*/
 #include "sys_arch.h"								/* Needed for sys_sem_t data type				*/
#endif

#ifdef __cplusplus
 extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Standalone driver package does not have the cache enable and there are data alignment issues		*/

#ifdef _STANDALONE_
 #ifdef __CC_ARM									/* copy - standalone has cache off				*/
  #define SMEMCPY(dst,src,nmb)	do {volatile char *xsrc;											\
									volatile char *xdst;											\
									int kk;															\
									xsrc = (char *)src;												\
									xdst = (char *)dst;												\
									for(kk=0 ; kk<nmb ; kk++) {										\
										xdst[kk] = xsrc[kk];										\
									}																\
								} while(0)

  #define MEMCPY(a,b,c)			SMEMCPY(a,b,c);
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* lwiP defines its "internal" errno that sometimes conflict with from the compiler's				*/

#if defined(__CC_ARM) || defined(__ICCARM__)		/* ARM & IAR barely defines any errno values	*/
  #define LWIP_PROVIDE_ERRNO		(1)				/* lwIP can use its own errno definitions		*/
  #undef ENOMEM
  #undef EINVAL
  #undef EDOM
  #undef ERANGE
  #undef EILSEQ
  #undef ETIMEDOUT
#else
 #if ((LWIP_VERSION_MAJOR) == 2U)					/* These cannot be overloaded otherwise issues	*/
  #define LWIP_PROVIDE_ERRNO		(1)
 #endif
  #include <errno.h>								/* Can't #define LWIP_PROVIDE_ERRNO because it 	*/
  #define LWIP_TIMEVAL_PRIVATE		(0)				/* has defs conflicting with the system errno	*/
  #include <sys/time.h>
#endif

/* ------------------------------------------------------------------------------------------------ */
/* CPU, Multi-thread set-up																			*/

#ifndef NO_SYS										/* When runnign in a multi-thread environment,	*/
 #ifdef _STANDALONE_								/* set to != 1 to add exclusive access			*/
  #define NO_SYS					(1)				/* protection, semaphores & bloxking queues		*/
 #else
  #define NO_SYS					(0)
 #endif
#endif

#ifndef SYS_LIGHTWEIGHT_PROT
 #ifdef _STANDALONE_								/* lwIP multi-thread protection. != 0 is in a	*/
  #define SYS_LIGHTWEIGHT_PROT		(0)				/* multi-thread envirnoment, ==0 when in		*/
 #else												/* Standalone / bare-metal						*/
  #define SYS_LIGHTWEIGHT_PROT		(1)
 #endif
#endif

#ifndef MEM_ALIGNMENT								/* Data alignement (bytes) needed by the CPU	*/
 #if (((OS_PLATFORM) & 0x00FFFFFF) == 0x007753)		/* ARM A53										*/
  #define MEM_ALIGNMENT				(64)			/* 64 - cache line size for Ethernet I/F		*/

 #elif ((((OS_PLATFORM) & 0x00FFFFFF) == 0x007020)													\
    ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)													\
    ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5))	/* ARM A9										*/
  #define MEM_ALIGNMENT				(32)			/* 32 - cache line size for Ethernet I/F		*/
 #else
  #define MEM_ALIGNMENT				(8)				/* For any other CPU, that's sort of safe		*/
 #endif
#endif


/* ------------------------------------------------------------------------------------------------ */
/* Timeouts																							*/

#ifndef MEMP_NUM_SYS_TIMEOUT					/*Number of timeout that can simultaneously be		*/
  #define MEMP_NUM_SYS_TIMEOUT		(10)		/* dealt with										*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Checksum & padding - selection if done in S/W or in the EMAC controller and if packets have		*/
/* have padding or not and if they have, how many bytes												*/

#ifndef CHECKSUM_BY_HARDWARE
 #if ((((OS_PLATFORM) & 0x00FFFFFF) == 0x007020)													\
  ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x007753)													\
  ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)													\
  ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)													\
  ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x32F207)													\
  ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x32F407))
   #define CHECKSUM_BY_HARDWARE		1				/* NOT a Boolean, but a defined or non defined	*/
 #endif
#endif

#ifndef CHECKSUM_GEN_IP								/* Generation of TXed IP packet checksum		*/
 #ifndef CHECKSUM_BY_HARDWARE						/* != 0 is generated in S/W, == 0 is generated	*/
  #define CHECKSUM_GEN_IP			(1)				/* in H/W (EMAC controller)						*/
 #else
  #define CHECKSUM_GEN_IP			(0)
 #endif
#endif

#ifndef CHECKSUM_CHECK_IP							/* Verification of RXed IP packets checksum		*/
 #ifndef CHECKSUM_BY_HARDWARE						/* != 0 is checked in S/W, == 0 is checked		*/
  #define CHECKSUM_CHECK_IP			(1)				/* in H/W (EMAC controller)						*/
 #else
  #define CHECKSUM_CHECK_IP			(0)
 #endif
#endif

#ifndef CHECKSUM_GEN_UDP							/* Generation of TXed UDP packet checksum		*/
 #ifndef CHECKSUM_BY_HARDWARE						/* != 0 is generated in S/W, == 0 is generated	*/
  #define CHECKSUM_GEN_UDP			(1)				/* in H/W (EMAC controller)						*/
 #else
  #define CHECKSUM_GEN_UDP			(0)
 #endif
#endif

#ifndef CHECKSUM_CHECK_UDP							/* Verification of RXed UDP packets checksum	*/
 #ifndef CHECKSUM_BY_HARDWARE						/* != 0 is checked in S/W, == 0 is checked		*/
  #define CHECKSUM_CHECK_UDP		(1)				/* in H/W (EMAC controller)						*/
 #else
  #define CHECKSUM_CHECK_UDP		(0)
 #endif
#endif

#ifndef CHECKSUM_GEN_TCP							/* Generation of TXed TCP packet checksum		*/
 #ifndef CHECKSUM_BY_HARDWARE						/* != 0 is generated in S/W, == 0 is generated	*/
  #define CHECKSUM_GEN_TCP			(1)				/* in H/W (EMAC controller)						*/
 #else
  #define CHECKSUM_GEN_TCP			(0)
 #endif
#endif

#ifndef CHECKSUM_CHECK_TCP							/* Verification of RXed TC packets checksum		*/
 #ifndef CHECKSUM_BY_HARDWARE						/* != 0 is checked in S/W, == 0 is checked		*/
  #define CHECKSUM_CHECK_TCP		(1)				/* in H/W (EMAC controller)						*/
 #else
  #define CHECKSUM_CHECK_TCP		(0)
 #endif
#endif


#ifndef CHECKSUM_GEN_ICMP
 #if ((((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)													\
  ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5)													\
  ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x32F207)													\
  ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x32F407))
  #define CHECKSUM_GEN_ICMP			(0)			/* Where the ICMP checum is generated. != 0 is		*/
 #else											/* generated in S/W and ==0 it's done in the		*/
  #define CHECKSUM_GEN_ICMP			(1)			/* EMAC controller									*/
 #endif
#endif

#ifndef ETH_PAD_SIZE
 #if ((((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)													\
  ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5))
  #ifdef _STANDALONE_
    #define ETH_PAD_SIZE			(2)
  #else
    #define ETH_PAD_SIZE			(0)
  #endif
 #else
  #define ETH_PAD_SIZE				(0)
 #endif
#endif
#if ((ETH_PAD_SIZE) < 0)
  #error "ETH_PAD_SIZE value must be >= 0"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Fragmentation and re-assembly																	*/
/* Note, the MTU set for each netif in the driver interface: ethernetif.c							*/

#ifndef IP_REASSEMBLY							/* When != 0, enable re-assembling of fragmented	*/
  #define IP_REASSEMBLY				(1)			/* packets that are received						*/
#endif

#ifndef IP_FRAG									/* When != 0, enable the fragmentation of packets	*/
  #define IP_FRAG					(1)			/* when their size if larget that the MTU			*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Buffer size and number of resources																*/
/* Determine from the target platform if lots of data memory is available or not					*/
/* Lots or not changes the default number of ressources and buffer sizes							*/
 
#ifndef LWIP_LOTS_OF_MEMORY
 #if ((((OS_PLATFORM) & 0x00FFFFFF) == 0x007753)													\
  ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x007020)													\
  ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AA10)													\
  ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x00AAC5))
  #define LWIP_LOTS_OF_MEMORY  		(1)
 #else
  #define LWIP_LOTS_OF_MEMORY		(0)
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* lwIP heap and pBuf memory settings																*/

#ifndef MEM_SIZE								/* lwIP own's heap memory size. This is unrelated	*/
 #if LWIP_LOTS_OF_MEMORY						/* to the standard malloc() and free(), nor the		*/
  #define MEM_SIZE					(32*65536)	/* memory used by the pbufs of type PBUF_POOL.		*/
 #else											/* BUT!!!! it is used for the PBUF_RAM type and		*/
  #define MEM_SIZE					(5*1024)	/* this type of pbuf is heavily used				*/
 #endif
#endif

#ifndef PBUF_POOL_BUFSIZE						/* Number of bytes each pBuf can hold				*/
 #if LWIP_LOTS_OF_MEMORY
  #define PBUF_POOL_BUFSIZE			(1536+(ETH_PAD_SIZE))
 #else
  #define PBUF_POOL_BUFSIZE			((TCP_MSS)+44+(ETH_PAD_SIZE))
 #endif
#endif

#ifndef PBUF_POOL_SIZE							/* Number of pBuf in the pool. pBuf are the			*/
 #if LWIP_LOTS_OF_MEMORY						/* buffers used by lwIP to exchange the packet		*/
  #define PBUF_POOL_SIZE			(4096)		/* data across the modules							*/
 #else
  #define PBUF_POOL_SIZE			(20)
 #endif
#endif

#ifndef MEMP_NUM_PBUF							/* Alike PBUF_POOL_SIZE but this defines it for		*/
 #ifdef _STANDALONE_							/* set to != 1 to add exclusive access			*/
  #if LWIP_LOTS_OF_MEMORY						/* pBuf located in ROM / read-only memory			*/
   #define MEMP_NUM_PBUF				(512)
  #else
   #define MEMP_NUM_PBUF				(16)
  #endif
 #else
  #define MEMP_NUM_PBUF					0
 #endif
#endif

#ifndef MEMP_OVERFLOW_CHECK						/* lwIP can check the consistency of the pbuf.		*/
  #define MEMP_OVERFLOW_CHECK		(0)			/* i.e. marker at lo mem & hi mem to check overrun.	*/
#endif											/* Enable by efining and setting to != 0			*/

/* ------------------------------------------------------------------------------------------------ */
/* ICMP settings / options																			*/

#ifndef LWIP_ICMP								/* If lwIP supports ICMP. Set to != 0 to enable		*/
  #define LWIP_ICMP					(1)			/* support and ==0 to disable it					*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* DHCP settings / options																			*/

#ifndef LWIP_DHCP								/* If lwIP supports DHCP. Set to != 0 to enable		*/
  #define LWIP_DHCP					(1)			/* support and ==0 to disable it					*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* ARP settings / options																			*/

#ifndef ARP_QUEUEING							/* When  !=0, allow the queuing of multiple pakets	*/
  #define ARP_QUEUEING				(1)			/* for ARP											*/
#endif

#ifndef ETHARP_TRUST_IP_MAC						/* When !=0 allow the incoming packets to perform	*/
  #define ETHARP_TRUST_IP_MAC		(0)			/* updates in the ARP table							*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* UDP settings / options																			*/

#ifndef LWIP_UDP								/* If lwIP supports UDP. Set to != 0 to enable		*/
  #define LWIP_UDP					(1)			/* support and ==0 to disable it					*/
#endif

#ifndef UDP_TTL									/* Default time to live (TTL) maximum value is 255	*/
  #define UDP_TTL					(255)
#endif
#if ((UDP_TTL) > 255)
  #erro "UDP_TTL must be <= 255"
#endif

#ifndef MEMP_NUM_UDP_PCB						/* Number of UDP control blocks						*/
 #if LWIP_LOTS_OF_MEMORY
  #define MEMP_NUM_UDP_PCB			(32)
 #else
  #define MEMP_NUM_UDP_PCB			(6)
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* TCP settings / options																			*/

#ifndef LWIP_TCP								/* If lwIP supports TCP. Set to != 0 to enable		*/
  #define LWIP_TCP					(1)			/* support and ==0 to disable it					*/
#endif

#ifndef TCP_MSS									/* TCP maximum segment size							*/
  #define TCP_MSS					(1500-40)	/* MSS == Ethernet MTU - IP header - TCP header		*/
#endif

#ifndef TCP_TTL									/* Default time to live (TTL) maximum value is 255	*/
  #define TCP_TTL					(255)
#endif
#if ((UDP_TTL) > 255)
  #erro "UDP_TTL must be <= 255"
#endif

#ifndef MEMP_NUM_TCP_PCB						/* Number of simultaneous active TCP connections	*/
 #if LWIP_LOTS_OF_MEMORY						/* that can go on									*/
  #define MEMP_NUM_TCP_PCB			(64)
 #else
  #define MEMP_NUM_TCP_PCB			(10)
 #endif
#endif

#ifndef MEMP_NUM_TCP_PCB_LISTEN					/* Number of simultaneous active lsitening TCP		*/
 #if LWIP_LOTS_OF_MEMORY						/* connections that can go on						*/
  #define MEMP_NUM_TCP_PCB_LISTEN	(32)
 #else
  #define MEMP_NUM_TCP_PCB_LISTEN	(5)
 #endif
#endif

#ifndef MEMP_NUM_TCP_SEG						/* Number of TCP queued segments that can be held	*/
 #if LWIP_LOTS_OF_MEMORY						/* at anyytime										*/
  #define MEMP_NUM_TCP_SEG			(512)
 #else
  #define MEMP_NUM_TCP_SEG			(16)
 #endif
#endif

#ifndef TCP_QUEUE_OOSEQ							/* Selects it TCP queues or not packets that arrive	*/
 #if LWIP_LOTS_OF_MEMORY						/* out of sequence. Sert to !=0 to enable and to	*/
  #define TCP_QUEUE_OOSEQ			(1)			/* == 0 to not queue them (to drop them)			*/
 #else
  #define TCP_QUEUE_OOSEQ			(0)
 #endif
#endif

#ifndef TCP_SND_BUF								/* Size of the buffer for the TCP sender (bytes)	*/
 #if LWIP_LOTS_OF_MEMORY
  #define TCP_SND_BUF				(TCP_WND)
 #else
  #define TCP_SND_BUF				(2*TCP_MSS)
 #endif
#endif

#ifndef TCP_SND_QUEUELEN						/* Size for the TXP sender pBuf memory space		*/
 #if LWIP_LOTS_OF_MEMORY
  #define TCP_SND_QUEUELEN			((8*TCP_SND_BUF)/TCP_MSS)
 #else
  #define TCP_SND_QUEUELEN			((4*TCP_SND_BUF)/TCP_MSS)
 #endif
#endif
#if ((TCP_SND_QUEUELEN) < ((2*(TCP_SND_BUF)/(TCP_MSS))))
 #error "TCP_SND_QUEUELEN must be >= (2*(TCP_SND_BUF)/(TCP_MSS)"
#endif

#ifndef TCP_WND									/* TCP receive window size. Should ideally be set	*/
 #if LWIP_LOTS_OF_MEMORY						/* >= TCP_SND_BUF									*/
  #define TCP_WND					(65535-1)	/* Max stated is 65535 but issues if not 1 less		*/
 #else
  #define TCP_WND					(2*TCP_MSS)
 #endif
#endif
#if ((TCP_WND) < (2*(TCP_MSS)))
  #error "TCP_WND must be >= 2*TCP_MSS"
#endif
#if ((TCP_WND) > 65536)
  #error "TCP_WND must be < 65536"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* netconn settings / options																		*/

#ifndef LWIP_NETCONN							/* If lwIP supports netconn. Set to != 0 to enable	*/
 #ifdef _STANDALONE_							/* support and ==0 to disable it					*/
  #define LWIP_NETCONN				(0)			/* When enable, api_lib.c is needed					*/
 #else
  #define LWIP_NETCONN				(1)
 #endif
#endif

#ifndef MEMP_NUM_NETCONN						/* Maximum number of simultaneous "connections"		*/
 #if LWIP_LOTS_OF_MEMORY
  #define MEMP_NUM_NETCONN  	    (16)
 #else
  #define MEMP_NUM_NETCONN     	 	(4)
 #endif
#endif


/* ------------------------------------------------------------------------------------------------ */
/* BSD socket settings / options																	*/
/* BSD socket can be used if LWIP_SOCKET != 0														*/

#ifndef LWIP_SOCKET								/* If lwIP supports BSD sockets. Set to != 0 to		*/
  #define LWIP_SOCKET				(0)			/* enable support and ==0 to disable it				*/
#endif

#ifndef LWIP_SO_RCVTIMEO						/* For BSD socket, set to !=0 to enable timeout on	*/
  #define LWIP_SO_RCVTIMEO			(1)			/* reception										*/
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Stats collection settings / options																*/

#ifndef LWIP_STATS								/* If lwIP supports stats. Set to != 0 to enable	*/
  #define LWIP_STATS				(0)			/* support and ==0 to disable it					*/
#endif

#ifndef LWIP_STATS_DISPLAY						/* Boolean to display the statistics				*/
 #if ((LWIP_STATS) != 0)						/* == 0 for no display, != 0 for display			*/
  #define  LWIP_STATS_DISPLAY		(1)			/* LWIP_PLATFORM_DIAG() is used for displaying		*/
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Debug settings / options																			*/

#if ((LWIP_VERSION_MAJOR) == 1U)				/* lwIP version #2 not checked / tested yet			*/

 #ifndef LWIP_DEBUG								/* If lwIP has its debug facility enable.			*/
  #define LWIP_DEBUG				(0)			/* Set to == -1 to enable, ==0 to disable			*/
 #endif

 #if ((LWIP_DEBUG) == 0)						/* Debug not enable, these set set to do nothing	*/
  #define LWIP_PLATFORM_DIAG(message)
  #define LWIP_ERROR(message, cond, ret)
 #else
												/* ------------------------------------------------ */
												/* Set to one of these:								*/
												/* 		LWIP_DBG_LEVEL_ALL							*/
												/* 		LWIP_DBG_LEVEL_WARNING						*/
												/*		LWIP_DBG_LEVEL_SERIOUS						*/
 #ifndef  LWIP_DBG_MIN_LEVEL					/*		LWIP_DBG_LEVEL_SEVERE						*/
  #define LWIP_DBG_MIN_LEVEL		LWIP_DBG_LEVEL_ALL
 #endif
												/* ------------------------------------------------ */
												/* Set to LWIP_DBG_ON for any types					*/
												/* or ORing of these to select 						*/
												/*		LWIP_DBG_TRACE								*/
												/*		LWIP_DBG_STATE								*/
												/*		LWIP_DBG_FRESH								*/
 #ifndef  LWIP_DBG_TYPES_ON						/*		LWIP_DBG_FRESH								*/
  #define LWIP_DBG_TYPES_ON			LWIP_DBG_ON
 #endif
												/* ------------------------------------------------ */
												/* LWIP_DBG_OFF to disable, LWIP_DBG_ON to enable	*/
												/* When enable, OR with values used in				*/
												/* LWIP_DBG_MIN_LEVEL if not at LWIP_DBG_LEVEL_ALL	*/
 #ifndef  API_LIB_DEBUG							/* LWIP_DBG_TYPES_ON  if not at LWIP_DBG_ON			*/
  #define API_LIB_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  API_MSG_DEBUG
  #define API_MSG_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  AUTOIP_DEBUG
  #define AUTOIP_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  DHCP_DEBUG
  #define DHCP_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  DNS_DEBUG
  #define DNS_DEBUG					LWIP_DBG_OFF
 #endif
 #ifndef  ETHARP_DEBUG
  #define ETHARP_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  ETHARP_DEBUG
  #define ETHARP_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  ICMP_DEBUG
  #define ICMP_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  IGMP_DEBUG
  #define IGMP_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  INET_DEBUG
  #define INET_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  IP_DEBUG
  #define IP_DEBUG					LWIP_DBG_OFF
 #endif
 #ifndef  IP_REASS_DEBUG
  #define IP_REASS_DEBUG			LWIP_DBG_OFF
 #endif
 #ifndef  MEMP_DEBUG
  #define MEMP_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  MEM_DEBUG
  #define MEM_DEBUG					LWIP_DBG_OFF
 #endif
 #ifndef  NETIF_DEBUG
  #define NETIF_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  PBUF_DEBUG
  #define PBUF_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  PPP_DEBUG
  #define PPP_DEBUG					LWIP_DBG_OFF
 #endif
 #ifndef  RAW_DEBUG
  #define RAW_DEBUG					LWIP_DBG_OFF
 #endif
 #ifndef  SLIP_DEBUG
  #define SLIP_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  SNMP_MIB_DEBUG
  #define SNMP_MIB_DEBUG			LWIP_DBG_OFF
 #endif
 #ifndef  SNMP_MSG_DEBUG
  #define SNMP_MSG_DEBUG			LWIP_DBG_OFF
 #endif
 #ifndef  SOCKETS_DEBUG
  #define SOCKETS_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  TCPIP_DEBUG
  #define TCPIP_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  TCP_CWND_DEBUG
  #define TCP_CWND_DEBUG			LWIP_DBG_OFF
 #endif
 #ifndef  TCP_DEBUG
  #define TCP_DEBUG					LWIP_DBG_OFF
 #endif
 #ifndef  TCP_FR_DEBUG
  #define TCP_FR_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  TCP_INPUT_DEBUG
  #define TCP_INPUT_DEBUG			LWIP_DBG_OFF
 #endif
 #ifndef  TCP_OUTPUT_DEBUG
  #define TCP_OUTPUT_DEBUG			LWIP_DBG_OFF
 #endif
 #ifndef  TCP_QLEN_DEBUG
  #define TCP_QLEN_DEBUG			LWIP_DBG_OFF
 #endif
 #ifndef  TCP_RST_DEBUG
  #define TCP_RST_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  TCP_RTO_DEBUG
  #define TCP_RTO_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  TIMERS_DEBUG
  #define TIMERS_DEBUG				LWIP_DBG_OFF
 #endif
 #ifndef  UDP_DEBUG
  #define UDP_DEBUG					LWIP_DBG_OFF
 #endif

 #undef  LWIP_PLATFORM_DIAG
 #define LWIP_PLATFORM_DIAG(message) 				do {											\
														 MTXlock(G_OSmutex, -1);					\
													     printf message;							\
													     MTXunlock(G_OSmutex);						\
													} while(0)

 #undef  LWIP_ERROR
 #define LWIP_ERROR(message, expression, handler)	do {											\
														 if (!(expression)) {						\
															MTXlock(G_OSmutex, -1);					\
															puts(message); 							\
															MTXunlock(G_OSmutex);					\
															handler;								\
														}											\
													} while(0)
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Multi-thread settings / options																	*/

#ifndef _STANDALONE_

 #ifdef OS_MP_TYPE									/* In BMP, if all task are to run on one  core	*/
  #if ((((OS_MP_TYPE) & 4) != 0) && ((OS_N_CORE) > 1))	/* set the define value to the target core	*/
   #ifndef LWIP_ABASSI_BMP_CORE_2_USE
    #define LWIP_ABASSI_BMP_CORE_2_USE	(-1)		/* Default, -ve means no target core			*/
   #endif
  #endif
 #endif


 #ifndef _MBOX_SIZE
  #define _MBOX_SIZE				(32)
 #endif

 #if (((OS_PLATFORM) & 0x00FFFFFF) == 0x007753)
  #define LWIP_DEF_STACKSIZE		(32768)
 #elif LWIP_LOTS_OF_MEMORY
  #define LWIP_DEF_STACKSIZE		(8192)
 #else
  #define LWIP_DEF_STACKSIZE		(2048)
 #endif

 #ifndef TCPIP_THREAD_STACKSIZE
  #define TCPIP_THREAD_STACKSIZE	(LWIP_DEF_STACKSIZE)
 #endif

 #ifndef TCPIP_THREAD_PRIO
  #ifndef osCMSIS
   #if ((OS_N_CORE) == 1)
    #define TCPIP_THREAD_PRIO		(2)
   #else
    #if (((OS_MP_TYPE) & 2) == 0)
      #define TCPIP_THREAD_PRIO		(0)
    #else
      #define TCPIP_THREAD_PRIO		(1)
    #endif
   #endif
  #else
   #if ((OS_N_CORE) == 1)
    #define TCPIP_THREAD_PRIO		(osPriorityAboveNormal)
   #else
    #if (((OS_MP_TYPE) & 2) == 0)
      #define TCPIP_THREAD_PRIO		(osPriorityRealtime)
    #else
      #define TCPIP_THREAD_PRIO		(osPriorityHigh)
    #endif
   #endif
  #endif
 #endif

 #ifndef TCPIP_MBOX_SIZE
  #define TCPIP_MBOX_SIZE			(32)
 #endif

 #ifndef NETIF_TASK_STACK_SIZE
  #define NETIF_TASK_STACK_SIZE		(LWIP_DEF_STACKSIZE)
 #endif

 #ifndef DEFAULT_UDP_RECVMBOX_SIZE
  #define DEFAULT_UDP_RECVMBOX_SIZE	(32)
 #endif

 #ifndef DEFAULT_TCP_RECVMBOX_SIZE
  #define DEFAULT_TCP_RECVMBOX_SIZE	(32)
 #endif

 #ifndef DEFAULT_ACCEPTMBOX_SIZE
  #define DEFAULT_ACCEPTMBOX_SIZE	(32)
 #endif

 #ifndef DEFAULT_THREAD_STACKSIZE
  #define DEFAULT_THREAD_STACKSIZE	(LWIP_DEF_STACKSIZE)
 #endif

 #ifndef LWIP_NETCONN_FULLDUPLEX
  #define LWIP_NETCONN_FULLDUPLEX		(1)
 #endif
 #ifndef LWIP_NETCONN_SEM_PER_THREAD
  #define LWIP_NETCONN_SEM_PER_THREAD	(1)
 #endif

 extern sys_sem_t *LWIP_NETCONN_THREAD_SEM_GET(void);
 #define           LWIP_NETCONN_THREAD_SEM_ALLOC()	OX_DO_NOTHING()
 #define           LWIP_NETCONN_THREAD_SEM_FREE()	OX_DO_NOTHING()

#endif

#ifdef __cplusplus
}
#endif

#endif
/* EOF */
