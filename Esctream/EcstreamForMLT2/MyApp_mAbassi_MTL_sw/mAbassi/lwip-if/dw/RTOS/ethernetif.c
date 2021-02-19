/* ------------------------------------------------------------------------------------------------ */
/* FILE :        ethernetif.c																		*/
/*																									*/
/* CONTENTS :																						*/
/*               lwIP ethernet interface for the Arria 5 / Arria 10 / Cyclone V						*/
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
/*  $Revision: 1.120 $																				*/
/*	$Date: 2018/11/18 20:03:29 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

/**
 * @file
 * Ethernet Interface Skeleton
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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

/*--------------------------------------------------------------------------------------------------*/
/*																									*/
/* NOTE:																							*/
/*		This code can handle multiple devices.														*/
/*		The device is indicated by the field name[0] in the struct netif with an ASCII character,	*/
/*		so device #1 has name[0] == '0'.															*/
/*		name[0] was selected instead of the field num because num is set in netif_add(), which is	*/
/*		told to call ethernetif_init().  name[] is left untouched so using name[] allows the upper	*/
/*		level calling the initialization to specifcy the device in netif instead of having to use	*/
/*		device specific ethernetif_init(). When netif_add call ethernetif_init() it only passes		*/
/*		the netif data structure and nothing else													*/
/*																									*/
/*--------------------------------------------------------------------------------------------------*/

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "sys_arch.h"
#include "ethernetif.h"
#include "WebApp.h"


#ifndef NETIF_MTU
  #define NETIF_MTU					1500		/* Maximum transfer unit							*/
#endif

#ifndef NETIF_TASK_STACK_SIZE
  #define NETIF_TASK_STACK_SIZE		8192
#endif
#ifndef NETIF_TASK_PRIORITY
 #if ((OS_N_CORE) == 1)
  #define NETIF_TASK_PRIORITY		1
 #else
  #if (((OS_MP_TYPE) & 4) == 0)					/* Not BMP, then the task can be on any core		*/
   #define NETIF_TASK_PRIORITY		1			/* Set its priority right below Adam & Eve			*/
  #else
   #define NETIF_TASK_PRIORITY		2			/* BMP, then will be on same core as upper handler	*/
  #endif										/* Upper handler is right below Adam&Eve, so 2		*/
 #endif
#endif

#ifndef ETH_LINK_SPEED							/* Speed of the Ethernet link.						*/
  #define ETH_LINK_SPEED			-1			/* See PHY_init() in dw_ethernet for the valid		*/
#endif											/* values. -ve is auto								*/

#ifndef ETH_MULTICORE_ISR
  #define ETH_MULTICORE_ISR			0			/* If 2 or more cores can handle the interrupt		*/
#endif

#ifndef ETH_DEBUG
  #define ETH_DEBUG					0			/* Set to non-zero to print debug info				*/
#endif

#if ((ETH_PAD_SIZE) > 0)
  #define SKIP_PAD(x)	pbuf_header((x), -ETH_PAD_SIZE)		/* Skip the padding word (-ve is ptr++)	*/
  #define CLAIM_PAD(x)	pbuf_header((x),  ETH_PAD_SIZE)		/* Reclaim the padding word				*/
#elif ((ETH_PAD_SIZE) == 0)
  #define SKIP_PAD(x)	do {int _=0;_=_;} while(0)
  #define CLAIM_PAD(x)	do {int _=0;_=_;} while(0)
#else
  #error "ETH_PAD_SIZE is negative";
#endif

#if ((ETH_IS_BUF_PBUF) != 0)
  #if ((PBUF_POOL_BUFSIZE) < (ETH_RX_BUFSIZE))
	#if ((ETH_PAD_SIZE) > 0)
	  #error "PBUF_POOL_BUFSIZE-ETH_PAD_SIZE must be >= ETH_RX_BUFSIZE when ETH_BUFFER_TYPE is PBUF"
	#else
	  #error "PBUF_POOL_BUFSIZE must be >= ETH_RX_BUFSIZE when ETH_BUFFER_TYPE is PBUF"
	#endif
  #endif
#endif

												/* Set to NULL in case ISR runs before init			*/
static SEM_t        *g_EthRXsem[5]    = {NULL, NULL, NULL, NULL, NULL};
static SEM_t        *g_EthTXsem[5]    = {NULL, NULL, NULL, NULL, NULL};
static MTX_t        *g_MyMutex[5]     = {NULL, NULL, NULL, NULL, NULL};
static const char    g_RXsemName[][9] = {"ETHRX-0", "ETHRX-1", "ETHRX-2", "ETHRX-3", "ETHRX-4"};
static const char    g_TXsemName[][9] = {"ETHTX-0", "ETHTX-1", "ETHTX-2", "ETHTX-3", "ETHTX-4"};
static const char    g_EthName[][6]   = {"ETH-0",   "ETH-1",   "ETH-2",   "ETH-3",   "ETH-4"};

static volatile int  g_DummyRead;				/* Needed to make sure to read DMA status register	*/

#if ((ETH_IS_BUF_PBUF) != 0)					/* Ethernet RX and Tx DMA Descriptors				*/
  extern ETH_DMAdesc_t G_DMArxDescTbl[ETH_N_RXBUF][ETH_N_RXBUF];
#endif

static err_t ethernetif_0_init(struct netif *netif);
static err_t ethernetif_1_init(struct netif *netif);
static err_t ethernetif_2_init(struct netif *netif);
static err_t ethernetif_3_init(struct netif *netif);
static err_t ethernetif_4_init(struct netif *netif);
static err_t ethernetif_init  (struct netif *netif);
static void  ethernetif_input (void *Arg);
static err_t low_level_output (struct netif *netif, struct pbuf *p);

err_t (*ethernet_init[])(struct netif *netif) = {  &ethernetif_0_init
                                                  ,&ethernetif_1_init
                                                  ,&ethernetif_2_init
                                                  ,&ethernetif_3_init
                                                  ,&ethernetif_4_init
                                                };

#if ((ETH_MULTICORE_ISR) != 0)
  static int g_LockVar[5];
#endif

/*--------------------------------------------------------------------------------------------------*/
/* Need to set the EMAC I/F # in netif->num but netif_add overwrites netif->num with its own #		*/
/* be done calling the init function.  We simply use individual init function that sets netif->num	*/

err_t ethernetif_0_init(struct netif *netif)
{
	netif->num = 0;
	return(ethernetif_init(netif));
}

err_t ethernetif_1_init(struct netif *netif)
{
	netif->num = 1;
	return(ethernetif_init(netif));
}

err_t ethernetif_2_init(struct netif *netif)
{
	netif->num = 2;
	return(ethernetif_init(netif));
}

err_t ethernetif_3_init(struct netif *netif)
{
	netif->num = 3;
	return(ethernetif_init(netif));
}

err_t ethernetif_4_init(struct netif *netif)
{
	netif->num = 4;
	return(ethernetif_init(netif));
}

/*--------------------------------------------------------------------------------------------------*/
/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the netif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */

static err_t ethernetif_init(struct netif *netif)
{
int DevNmb;										/* EMAC device number associated with this netif	*/
int First;										/* First time the init is called					*/
int ii;											/* General purpose									*/
#if ((ETH_IS_BUF_PBUF) != 0)
  struct pbuf *NewPbuf;
  int          ReMap;
#endif

	LWIP_ASSERT("netif != NULL", (netif != NULL));

	DevNmb = (int)netif->num;					/* ->num holds the EMAC I/F # of this netif			*/

  #if ((ETH_DEBUG) > 1) 
	printf("ETHN  [Dev:%d] - Initialization\n", DevNmb);
  #endif

	First = 0;									/* Assume is not first time init of this device		*/
	MTXlock(G_OSmutex, -1);						/* Protect against multi-thread						*/
	if (g_MyMutex[DevNmb] == NULL) {			/* Each netif has its own mutex						*/
		(void)sys_mutex_new(&g_MyMutex[DevNmb]);
		First = 1;								/* Is the first time init done on this device		*/
	}
	MTXunlock(G_OSmutex);

	sys_mutex_lock(&g_MyMutex[DevNmb]);			/* Protect against re-entrance						*/

	netif->output     = etharp_output;
	netif->linkoutput = low_level_output;
	netif->hwaddr_len = ETHARP_HWADDR_LEN;		/* Set the netif MAC hardware address length		*/
	GetMACaddr(DevNmb, &netif->hwaddr[0]);		/* Use a fct to allow reading from hardware			*/
    netif->mtu        = NETIF_MTU;				/* Set the netif maximum transfer unit				*/
	netif->flags      = NETIF_FLAG_BROADCAST	/* Accept broadcast address and ARP traffic			*/
	                  | NETIF_FLAG_ETHARP
	                  | NETIF_FLAG_LINK_UP;

	g_EthRXsem[DevNmb] = SEMopen(g_RXsemName[DevNmb]);
	g_EthTXsem[DevNmb] = SEMopen(g_TXsemName[DevNmb]);

	ii = ETH_MacConfigDMA(DevNmb, ETH_LINK_SPEED);	/* Get the ethernet link up and running			*/

  #if ((ETH_DEBUG) > 0)
	if (ii >= 0) {
		printf("ETHN  [Dev:%d] - Link is up at %d Mbps %s duplex\n", DevNmb, ii&~1,
		                                                          (ii&1)?"half":"full");
	}
	else {
		printf("ETHN  [Dev:%d] - Error - Ethernet link failed to connect\n", DevNmb);
	}
  #else
	ii = ii;									/* To remove compiler warning if debug off			*/
  #endif

	ETH_MACAddressConfig(DevNmb, ETH_MAC_Address0, netif->hwaddr);	/* MAC address in EMAC			*/

	ETH_DMARxDescChainInit(DevNmb);				/* Init Rx Descriptor ring / list					*/
 	ETH_DMATxDescChainInit(DevNmb);				/* Init Tx Descriptor ring / list					*/

  #if ((ETH_IS_BUF_PBUF) != 0)					/* Init Rx Desc list buffers						*/
	ReMap = ETH_MAP_DEV(DevNmb);
	for (ii=0 ; ii<ETH_N_RXBUF ; ii++) {		/* Install the buffers in the RX DMA descriptor		*/
		NewPbuf = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE, PBUF_POOL);
		G_DMArxDescTbl[ReMap][ii].PbufPtr = NewPbuf;

		if (NewPbuf == NULL) {
		  #if ((ETH_DEBUG) > 0)
			printf("ETHN  [Dev:%d] - Error - out of memory to allocate pbuf\n", DevNmb);
		  #else
			LWIP_ERROR("lwIP (ethernetif.c) - Out of pbufs", 0, OX_DO_NOTHING(); );
		  #endif
			return(ERR_MEM);
		}
		else if (pbuf_clen(NewPbuf) != 1) {
		  #if ((ETH_DEBUG) > 0)
			printf("ETHN  [Dev:%d] - Error - pbuf are too small (chained pbuf)\n", DevNmb);
			printf("                 Set PBUF_POOL_BUFSIZE at least %d\n",
			                                                  ETH_RX_BUFSIZE+ETH_PAD_SIZE);
		  #else
			LWIP_ERROR("lwIP (ethernetif.c) - RX chained pbufs", 0, OX_DO_NOTHING(); );
		  #endif
			return(ERR_MEM);
		}

		SKIP_PAD(NewPbuf);						/* Drop the padding word if needed					*/
		G_DMArxDescTbl[ReMap][ii].Buff = (uint32_t)NewPbuf->payload;
	  #if ((MEMP_OVERFLOW_CHECK) != 0)
		DCacheFlushRange((void *)(ETH_RX_BUFSIZE+ETH_PAD_SIZE+(intptr_t)NewPbuf->payload),
		                 OX_CACHE_LSIZE);
	  #endif
		DCacheInvalRange(NewPbuf->payload, NewPbuf->len);
	}
	DCacheFlushRange(&G_DMArxDescTbl[ReMap][0], sizeof(G_DMArxDescTbl[0]));
  #endif

  #ifdef CHECKSUM_BY_HARDWARE					/* Enable the checksum insertion for the TX frames	*/
	ETH_DMATxDescChecksumInsertionConfig(DevNmb, 1);
  #else
	ETH_DMATxDescChecksumInsertionConfig(DevNmb, 0);
  #endif

	ETH_DMARxDescReceiveITConfig(DevNmb, 1);	/* Enable Ethernet RX interrupt						*/
	ETH_DMARxDescTransmitITConfig(DevNmb, 0);	/* Ethernet TX interrupt not enable					*/

  #if SYS_STATS
	memset(&lwip_stats.link, 0, sizeof(lwip_stats.link));
  #endif

  #if ((ETH_MULTICORE_ISR) != 0)
	g_LockVar[DevNmb] = 0;
  #endif

  #if ((ETH_DEBUG) > 1) 
	printf("ETHN  [Dev:%d] - Ethernet Input task started\n", DevNmb);
  #endif

	if (First != 0) {							/* Create the task that handles the ETH_MAC			*/
		sys_thread_new(g_EthName[DevNmb], ethernetif_input, netif, NETIF_TASK_STACK_SIZE,
		               NETIF_TASK_PRIORITY);
	}

  #if ((ETH_DEBUG) > 1) 
	printf("ETHN  [Dev:%d] - EMAC driver initialization\n", DevNmb);
  #endif

	ETH_Start(DevNmb);							/* Enable MAC and DMA TX and RX						*/

	sys_mutex_unlock(&g_MyMutex[DevNmb]);

	return(ERR_OK);
}

/*--------------------------------------------------------------------------------------------------*/
/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
uint8_t       *BufPtr;
int            DevNmb;							/* EMAC device number associated with this netif	*/
volatile u8_t *DMAbuf;
int            ii;
int            IsFirst;							/* If is the first buffer of a packet				*/
int            IsLast;							/* If is the last buffer of a packet				*/
int            LeftOver;						/* Left over from a packet yet not transmitted		*/
struct pbuf   *MyPbuf;							/* P buffer being processed							*/
u32_t          Nbytes;							/* Number of bytes in the buffer					*/

	DevNmb = (int)netif->num;					/* ->num holds the EMAC I/F # of this netif			*/

	sys_mutex_lock(&g_MyMutex[DevNmb]);			/* Protect against re-entrance						*/

	SKIP_PAD(p);								/* Drop the padding word if needed					*/

	IsFirst = 1;
	IsLast  = 0;

	for (MyPbuf=p ; MyPbuf!=NULL ; MyPbuf=MyPbuf->next) {
		LeftOver = MyPbuf->len;					/* Number of bytes left to give to the EMAC driver	*/
		BufPtr   = MyPbuf->payload;				/* Current base address of the data to give			*/
		while (LeftOver > 0) {					/* Loop in case pbuf payload > DMA buffer size		*/
			DMAbuf = ETH_Get_Transmit_Buffer(DevNmb);	/* Get the next DMA transmit buffer		*/
			if (DMAbuf == NULL) {				/* If NULL, DMA is currently using all the buffers	*/
				SEMwaitBin(g_EthTXsem[DevNmb], 0);	/* Get rid of any previous postings				*/
				for (ii=0 ; ii<10 ; ii++) {		/* Try a few time in case DMA is using all buffers	*/
					DMAbuf = ETH_Get_Transmit_Buffer(DevNmb);	/* Get the next DMA transmit buffer	*/
					if (DMAbuf != NULL) {		/* If NULL, DMA is currently using all the buffers	*/
						break;					/* Wait a bit and try again							*/
					}							/* It may never succeed if not enough DMA desc		*/
					SEMwaitBin(g_EthTXsem[DevNmb], OS_MS_TO_TICK(20));	/* Use a timeout for safety	*/
				}

				if (DMAbuf == NULL) {			/* Make sure we got a buffer						*/
				  #if ((ETH_DEBUG) > 0)
					printf("ETHN  [Dev:%d] - Error - No TX DMA descriptors available\n", DevNmb);
				  #else
					LWIP_ERROR("lwIP (ethernetif.c) - Out of TX buf", 0, OX_DO_NOTHING(); );
				  #endif
					STATS_INC(link.memerr);
					CLAIM_PAD(p);
					sys_mutex_unlock(&g_MyMutex[DevNmb]);

					return(ERR_MEM);
				}
			}

			Nbytes = (LeftOver < ETH_TX_BUFSIZE)/* Cannot fill more than the DMA buffer size		*/
				   ? LeftOver
			       : ETH_TX_BUFSIZE;
			LeftOver -= Nbytes;					/* This less to attach next time					*/
			IsLast = (MyPbuf->len == MyPbuf->tot_len)
			       && (LeftOver == 0);			/* Set if it is the last segment of a packet		*/

		  #if ((ETH_IS_BUF_CACHED) != 0)
			if (Nbytes > 0) {
				SMEMCPY((void *)&DMAbuf[0], BufPtr, Nbytes);
				BufPtr += Nbytes;				/* Advance the pointer to the next segment payload	*/
			}
		  #else
			for (ii=0 ; ii<Nbytes ; ii++) {
				*DMAbuf++ = *BufPtr++;
			}
		  #endif

		  #if ((ETH_DEBUG) > 1)
			if ((IsFirst != 0) && (IsLast != 0)) {
				printf("ETHN  [Dev:%d] - Sending packet of %d bytes\n", DevNmb, (int)Nbytes);
			}
			else if (IsFirst != 0) {
				printf("ETHN  [Dev:%d] - Sending first partial packet of %d bytes\n", DevNmb,
				       (int)Nbytes);
			}
			else if (IsLast != 0) {
				printf("ETHN  [Dev:%d] - Sending last partial packet of %d bytes\n", DevNmb,
				       (int)Nbytes);
			}
			else {
				printf("ETHN  [Dev:%d] - Sending partial packet of %d bytes\n", DevNmb,
				       (int)Nbytes);
			}
		  #endif

			ii = ETH_Prepare_Multi_Transmit(DevNmb, Nbytes, IsFirst, IsLast);
			if (ii != 0) {
			  #if ((ETH_DEBUG) > 0)
				printf("ETHN  [Dev:%d] - Error - TX request error #%d\n", DevNmb, ii);
			  #else
				LWIP_ERROR("lwIP (ethernetif.c) - Out of TX desc", 0, OX_DO_NOTHING(); );
			  #endif
				STATS_INC(link.err);
			}
			IsFirst = 0;						/* Cannot be the first anymore when looping			*/
		}
		if (IsLast != 0) {						/* Was the last segment of a packet, re-loop as 	*/
			IsFirst = 1;						/* another pbuf may be in the linked liked list		*/
		}										/* which is a new packet							*/
   	}

	CLAIM_PAD(p);								/* Reclaim the padding word if needed				*/

	STATS_INC(link.xmit);

	sys_mutex_unlock(&g_MyMutex[DevNmb]);

	return(ERR_OK);
}

/*--------------------------------------------------------------------------------------------------*/
/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf * low_level_input(struct netif *netif)
{
int          DevNmb;							/* EMAC device number associated with this netif	*/
FrameTypeDef Frame;								/* Current frame being processced					*/
int          Len;								/* Remaining @ of bytes to extract from the packet	*/
struct pbuf *PbufDst;							/* pbuf with new packek to return to the called		*/
#if ((ETH_IS_BUF_PBUF) != 0)
  int          FrmSize;							/* Number of bytes in the current frame				*/
  int          IsFirst;							/* If it is the first frame to be processed			*/
  struct pbuf *PbufDesc;						/* pbuf used in the DMA descriptor					*/
  struct pbuf *PbufNew;							/* New pbuf to replace the one from DMA descriptor	*/
#endif
#if (((ETH_PAD_SIZE) != 0) || ((ETH_IS_BUF_PBUF) == 0))
  u8_t *Buf8;
#endif

	DevNmb = (int)netif->num;					/* ->num holds the EMAC I/F # of this netif			*/

	PbufDst = NULL;

	sys_mutex_lock(&g_MyMutex[DevNmb]);			/* Protect against re-entrance						*/

  #if ((ETH_IS_BUF_PBUF) != 0)					/* DMA buffers are pbufs							*/
	Len = ETH_Get_Received_Frame_Length(DevNmb);/* Get the length of the Received packet			*/

	if (Len >= 0) {								/* Good packet										*/
	  #if ((ETH_DEBUG) > 1)
		printf("ETHN  [Dev:%d] - Received a new packet of %d bytes\n", DevNmb, Len);
	  #endif

		IsFirst = 1;
		do {
			Frame   = ETH_Get_Received_Multi(DevNmb);
			FrmSize = Frame.length;
			if (FrmSize >= 0) {					/* We have a good frame								*/
												/* Make sure we can get a replacement pbuf			*/
				PbufNew = pbuf_alloc(PBUF_RAW, ETH_RX_BUFSIZE+ETH_PAD_SIZE, PBUF_POOL);

				if (PbufNew != NULL) {			/* Got a replacement pbuf, proceed					*/
					if (IsFirst != 0) {			/* First segment of the packet or full packet		*/
						IsFirst          = 0;
						PbufDst          = Frame.descriptor->PbufPtr;
						PbufDst->len     = FrmSize;
						PbufDst->tot_len = FrmSize;
					}
					else {						/* Not first segment,  concatenate					*/
						PbufDesc          = Frame.descriptor->PbufPtr;
						PbufDesc->len     = FrmSize;
						PbufDesc->tot_len = FrmSize;

						pbuf_cat(PbufDst, PbufDesc);
					}

					SKIP_PAD(PbufNew);			/* Replace the Pbuf in the DMA desc by the new one	*/
					Frame.descriptor->PbufPtr = PbufNew;
					Frame.descriptor->Buff    = (uint32_t)PbufNew->payload;
				  #if ((MEMP_OVERFLOW_CHECK) != 0)
					DCacheFlushRange((void *)(ETH_RX_BUFSIZE+ETH_PAD_SIZE
					                         +(intptr_t)PbufNew->payload), OX_CACHE_LSIZE);
				  #endif						/* ->payload invalidated in ETH_Release_Received()	*/
				}
				else {							/* Out of pbuf, no replacement pbuf avail. abort	*/
					STATS_INC(link.memerr);
				  #if ((ETH_DEBUG) > 0)
					printf("ETHN  [Dev:%d] - Error - Out of pbuf for RX\n", DevNmb);
				  #else
					LWIP_ERROR("lwIP (ethernetif.c) - Out of pbuf", 0, OX_DO_NOTHING(); );
				  #endif

					do {						/* Drop the packet as we've have already started 	*/
						Frame = ETH_Get_Received_Multi(DevNmb);	/* extracting it and can't rewind	*/
					} while (Frame.length >= 0);

					ETH_Release_Received(DevNmb);

					if (IsFirst == 0) {			/* Release the pbuf in process of creation			*/
						pbuf_free(PbufDst);
					}

					sys_mutex_unlock(&g_MyMutex[DevNmb]);

					return(NULL);
				}

				Len -= FrmSize;					/* This less to extract								*/

			  #if ((ETH_DEBUG) > 1)
				printf("ETHN  [Dev:%d] -  Extracted %d bytes from the packet\n", DevNmb,
				       (int)Frame.length);
			  #endif
			}
			else {								/* No more frames. Shouldn't happen, safety			*/
				Len = 0;
			}
		} while (Len > 0);

		ETH_Release_Received(DevNmb);			/* Give back the descriptors to the DMA				*/

		CLAIM_PAD(PbufDst);

	  #if ((ETH_PAD_SIZE) != 0)
		Buf8 = PbufDst->payload;				/* This zeroing of the padded bytes is done in case	*/
		for (Len=0 ; Len<ETH_PAD_SIZE ; Len++) {/* error correction is enable on the memory			*/
			Buf8[Len] = 0;						/* If it was left un-initialized, a checksum 		*/
		}										/* error could be detected if chksum used on memory	*/
	  #endif

		STATS_INC(link.recv);
	}

  #else											/* DMA buffers are regular buffers					*/
	Len = ETH_Get_Received_Frame_Length(DevNmb);/* Get the length of the RXed packet				*/
	if (Len >= 0) {								/* Good packet										*/

	  #if ((ETH_DEBUG) > 1)
		printf("ETHN  [Dev:%d] - Received a new packet of %d bytes\n", DevNmb, Len);
	  #endif
												/* (+ETH_PAD_SIZE) padding room if necessary		*/
	    PbufDst = pbuf_alloc(PBUF_RAW, Len+ETH_PAD_SIZE, PBUF_POOL);
												/* Get buffer to return from the pool				*/
		if (PbufDst == NULL) {					/* Out of pbuf, leave the RXed						*/
			STATS_INC(link.memerr);
		  #if ((ETH_DEBUG) > 0)
			printf("ETHN  [Dev:%d] - Error - Out of pbuf for RX\n", DevNmb);
		  #else
			LWIP_ERROR("lwIP (ethernetif.c) - Out of pbuf", 0, OX_DO_NOTHING(););
		  #endif
			sys_mutex_unlock(&g_MyMutex[DevNmb]);
			return(NULL);
		}
		else if (pbuf_clen(PbufDst) != 1) {		/* Chained pbuf leave the RXed						*/
			pbuf_free(PbufDst);
			do {
				Frame = ETH_Get_Received_Multi(DevNmb);
				if (Frame.length >= 0) {
					Len -= Frame.length;		/* This less to extract								*/
				}
				else {							/* No more frames. Shouldn't happen, safety			*/
					Len = 0;
				}
			} while (Len > 0);
			ETH_Release_Received(DevNmb);
			STATS_INC(link.err);
		  #if ((ETH_DEBUG) > 0)
			printf("ETHN  [Dev:%d] - Error - RXed packet too big (pbuf chained)\n", DevNmb);
			printf("                        Dropping the RXed packet\n");
		  #else
			LWIP_ERROR("lwIP (ethernetif.c) - RX pbufs are chained",  0, OX_DO_NOTHING(); );
			LWIP_ERROR("lwIP (ethernetif.c) - Dropping the RX packet",0, OX_DO_NOTHING(); );
		  #endif
			sys_mutex_unlock(&g_MyMutex[DevNmb]);
			return(NULL);
		}

		SKIP_PAD(PbufDst);

		Buf8 = PbufDst->payload;
		do {
			Frame = ETH_Get_Received_Multi(DevNmb);
			if (Frame.length >= 0) {

			  #if !defined(__CC_ARM) && ((ETH_IS_BUF_UNCACHED) == 0)
				SMEMCPY(Buf8, (void *)Frame.buffer, Frame.length);
			  #else								/* Can't explain why only this memory copy fails	*/
				{								/* With ARM's compiler, but only when cached		*/
					char *xsrc;
					char *xdst;
					int kk;
					xsrc = (char *)Frame.buffer;
					xdst = (char *)Buf8;
					for(kk=0 ; kk<Frame.length ; kk++) {
						xdst[kk] = xsrc[kk];
					}
				}
			  #endif

			  #if ((ETH_DEBUG) > 1)
				printf("ETHN  [Dev:%d] -   Extracted %d bytes from the packet\n", DevNmb,
				       (int)Frame.length);
			  #endif
				Buf8 += Frame.length;
				Len  -= Frame.length;			/* This less to extract								*/
			}
			else {								/* No more frames. Shouldn't happen, safety			*/
				Len = 0;
			}
		} while (Len > 0);

		ETH_Release_Received(DevNmb);

		CLAIM_PAD(PbufDst);

	  #if ((ETH_PAD_SIZE) != 0)
		Buf8 = PbufDst->payload;				/* This zeroing of the padded bytes is done in case	*/
		for (Len=0 ; Len<ETH_PAD_SIZE ; Len++) {/* error correction is enable on the memory			*/
			Buf8[Len] = 0;						/* If it was left un-initialized, a checksum 		*/
		}										/* error could be detected if chksum used on memory	*/
	  #endif

		STATS_INC(link.recv);
	}
  #endif

	sys_mutex_unlock(&g_MyMutex[DevNmb]);

	return(PbufDst);
}

/*--------------------------------------------------------------------------------------------------*/
/**
 * This function is the ethernetif_input task, it is processed when a packet 
 * is ready to be read from the interface. It uses the function low_level_input() 
 * that should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void ethernetif_input(void *Arg)
{
int           DevNmb;
struct netif *netif;
struct pbuf  *p;

	netif  = (struct netif *)Arg;			
	DevNmb = (int)netif->num;					/* ->num holds the EMAC I/F # of this netif			*/

	for(;;) {
												/* Block for max of 1s.  This allows periodic		*/
		SEMwait(g_EthRXsem[DevNmb], OS_TICK_PER_SEC);	/* checks for link up / link down and		*/
												/* also recovery in case the RX DMA chain is full	*/
		do {
			p = low_level_input(netif);			/* Process the input buffer							*/
			if (p != (struct pbuf *)NULL) {		/* OK, we got a valid buffer						*/
				if (ERR_OK != netif->input(p, netif)) {	/* Process it								*/
					pbuf_free(p);				/* If processing error, free the pbuf				*/
				}
			}
		} while (p != NULL);

		if (ETH_LinkStatus(DevNmb) == 0) {		/* The link is down, wait for it to be back			*/
		  #if ((ETH_DEBUG) > 0)
			printf("ETHN  [Dev:%d] - Warning - Link down\n", DevNmb);
		  #endif
			while (ETH_LinkStatus(DevNmb) == 0) {	/* Wait until link re-connected					*/
				TSKsleep(OS_MS_TO_TICK(100));	/* Done after the RX processing to not				*/
			}									/* delay it											*/
			PHY_init(DevNmb, ETH_LINK_SPEED);	/* Re-init the link communication					*/
		  #if ((ETH_DEBUG) > 0)
			if (ETH_LinkStatus(DevNmb) != 0) {
				printf("ETHN  [Dev:%d] - Link up\n", DevNmb);
			}
		  #endif
		}
	}
}

/*--------------------------------------------------------------------------------------------------*/
/* EMAC interrupt handlers																			*/
/*--------------------------------------------------------------------------------------------------*/

static void EMACintHndl(int Dev);

void Emac0_IRQHandler(void)
{
	EMACintHndl(0);
	return;
}

/* ------------------------------------------------------------------------------------------------ */

void Emac1_IRQHandler(void)
{
	EMACintHndl(1);
	return;
}

/* ------------------------------------------------------------------------------------------------ */

void Emac2_IRQHandler(void)
{
	EMACintHndl(2);
	return;
}

/* ------------------------------------------------------------------------------------------------ */

void Emac3_IRQHandler(void)
{
	EMACintHndl(3);
	return;
}

/* ------------------------------------------------------------------------------------------------ */

void Emac4_IRQHandler(void)
{
	EMACintHndl(4);
	return;
}

/* ------------------------------------------------------------------------------------------------ */

static void EMACintHndl(int Dev)
{
int      ii;									/* General purpose									*/
uint32_t StatReg;								/* Snapshot of the status register					*/
#if (((ETH_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1) && ((OX_NESTED_INTS) != 0))
  int ISRstate;
#endif

  #if (((ETH_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1))	/* If multiple cores handle the ISR			*/
   #if ((OX_NESTED_INTS) != 0)					/* Get exclusive access								*/
	ISRstate = OSintOff();
   #endif
	CORElock(ETH_SPINLOCK, &g_LockVar[Dev], 0, COREgetID()+1);
  #endif
												/* Looping to eliminate spurious interrupts and to	*/
	do {										/* handle ASAP an interrupt raised when in here		*/
		StatReg = EMACreg(Dev, EMAC_INT_STAT_REG);

		if (StatReg & EMAC_INT_STAT_RGSMIIIS) {	/* Check for a state change on the RGMMI link		*/
			g_DummyRead = EMACreg(Dev, EMAC_PHY_CTL_STAT_REG);
			if (g_EthRXsem[Dev] != NULL) {		/* Make sure the semaphore is valid					*/
				SEMpost(g_EthRXsem[Dev]);		/* Inform the BG about the change of the link state	*/
			}
		}

		if (StatReg & (EMAC_INT_STAT_MMCIS|EMAC_INT_STAT_MMCRXIPIS)) {
			if (StatReg & EMAC_INT_STAT_MMCIS) {/* MMC counter interrupt							*/
				for (ii=0x114/4 ; ii<=0x1E4/4 ; ii++) {	/* In case MMC ints are enabled, read all	*/
					g_DummyRead = EMACreg(Dev, ii);	/* counters to remove the interrupt(s) raised	*/
				}								/* Put App specific if stats are used				*/
			}
			if (StatReg & EMAC_INT_STAT_MMCRXIPIS) {
				for (ii=0x210/4 ; ii<=0x284/4 ; ii++) {	/* In case MMC ints are enabled, read all	*/
					g_DummyRead = EMACreg(Dev, ii);	/* counters to remove the interrupt(s) raised	*/
				}								/* Put App specific code if stats are used			*/
			}
		}

	  #if 0										/* Not used and this code hasn't been tested		*/
		if (StatReg & EMAC_INT_STAT_PCSLCHGIS) {/* Change on TBI, RTBI, or SGMII link				*/
			/* Need to read "AN" status register to clear .... where is that register (#49) ???		*/
		}
		if (StatReg & EMAC_INT_STAT_TSIS) {		/* Time stamp exceeded								*/
			g_DummyRead = EMACreg(Dev, EMAC_LPI_CTL_STAT_REG);	/* Rd LPI ctrl-status to clear ints	*/
		}
	  #endif

												/* ------------------------------------------------ */
		StatReg = EMACreg(Dev, EMAC_DMA_STAT_REG)
		        & (EMAC_DMA_STAT_TI				/* Only keep what is handled here to clear int rqst	*/
		         | EMAC_DMA_STAT_RI
		         | EMAC_DMA_STAT_NIS);

		EMACreg(Dev, EMAC_DMA_STAT_REG) = StatReg;	/* Clear the ints we are going to handle		*/

		if ((StatReg & EMAC_DMA_STAT_RI)		/* Was a new frame RXed?							*/
		&&  (g_EthRXsem[Dev] != NULL)) {		/* Make sure the semaphore is valid					*/
			SEMpost(g_EthRXsem[Dev]);			/* Post the semaphore ethernetif_input() blocks on	*/
		}

		if ((StatReg & EMAC_DMA_STAT_TI)		/* Was a new frame TXed?							*/
		&&  (g_EthTXsem[Dev] != NULL)) {		/* Make sure the semaphore is valid					*/
			SEMpost(g_EthTXsem[Dev]);			/* Post the semaphore low_level_output() uses		*/
		}
												/* Add INT_STAT_PCSLCHGIS & INT_STAT_TSIS if used	*/
	} while ((EMACreg(Dev, EMAC_INT_STAT_REG) & (EMAC_INT_STAT_RGSMIIIS
	                                          |  EMAC_INT_STAT_MMCIS
	                                          |  EMAC_INT_STAT_MMCRXIPIS))
	  |      (EMACreg(Dev, EMAC_DMA_STAT_REG) & (EMAC_DMA_STAT_TI
	                                          |  EMAC_DMA_STAT_RI
	                                          |  EMAC_DMA_STAT_NIS)));
												/* Note: binary | instead of || as possibly faster	*/

  #if (((ETH_MULTICORE_ISR) != 0) && ((OX_N_CORE) > 1))	/* If multiple cores handle the ISR			*/
	COREunlock(ETH_SPINLOCK, &g_LockVar[Dev], 0);	/* Release exclusive access						*/
   #if ((OX_NESTED_INTS) != 0)
	 OSintBack(ISRstate);
   #endif
  #endif

	return;
}

/* EOF */

