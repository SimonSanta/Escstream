/* ------------------------------------------------------------------------------------------------ */
/* FILE :		dw_ethernet.h																		*/
/*																									*/
/* CONTENTS :																						*/
/*				Driver for the DesignWare Ethernet.													*/
/*																									*/
/*																									*/
/* Copyright (c) 2013-2019, Code-Time Technologies Inc. All rights reserved.						*/
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
/*	$Revision: 1.17 $																				*/
/*	$Date: 2019/01/10 18:07:03 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/*																									*/
/* See dw_ethernet.c for NOTE / LIMITATIONS / NOT YET SUPPORTED										*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Supported value for the build options															*/
/* (OS_PLATFORM & 0x00FFFFFF):																		*/
/*					0xAA10 : Arria 10 																*/
/*					0xAAC5 : Cyclone V & Arria V													*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */
/* Default values that depend on (OS_PLATFORM & 0x00FFFFFF):										*/
/* ETH_MAX_DEVICES:																					*/
/*					0xAA10 : 3																		*/
/*					0xAAC5 : 2																		*/
/* ETH_LIST_DEVICE:																					*/
/*					0xAA10 : 0x07																	*/
/*					0xAAC5 : 0x03																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __DW_ETHERNET_H
#define __DW_ETHERNET_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ------------------------------------------------------------------------------------------------ */

#if defined(ETH_MAX_DEVICES) && defined(ETH_LIST_DEVICE)
  #error "Define one of the two: ETH_MAX_DEVICES or ETH_LIST_DEVICE"
  #error "Do not define both as there could be conflicting info"
#endif

#if !defined(ETH_MAX_DEVICES)
  #if (((OS_PLATFORM) & 0x00FFFFFF) == 0xAAC5)
	#define ETH_MAX_DEVICES	2						/* Arria V / Cyclone V has a total of 2 ETHs	*/
  #elif (((OS_PLATFORM) & 0x00FFFFFF) == 0xAA10)
	#define ETH_MAX_DEVICES	3						/* Arria 10 has a total of 3 ETHs				*/
  #else
	#define ETH_MAX_DEVICES	4
  #endif
#endif

#ifndef ETH_LIST_DEVICE
  #define ETH_LIST_DEVICE	((1U<<(ETH_MAX_DEVICES))-1U)
#endif

#if ((ETH_MAX_DEVICES) <= 0)
  #error "ETH_MAX_DEVICES must be 1 or greater"
#endif

#if (((1U<<(ETH_MAX_DEVICES))-1U) < (ETH_LIST_DEVICE))
  #error "too many devices in ETH_LIST_DEVICE vs the max # device defined by ETH_MAX_DEVICES"
#endif

/* ------------------------------------------------------------------------------------------------ */

#define USE_ENHANCED_DMA_DESCRIPTORS				/* Type of DMA buffer to use					*/
#define ETH_BUFFER_UNCACHED		0					/* Buffers are in non-cached memory				*/
#define ETH_BUFFER_CACHED		1					/* Buffers are in cached memory					*/
#define ETH_BUFFER_PBUF			3					/* TX:cached, RX: lwIP PBUF cached				*/
#ifndef ETH_BUFFER_TYPE
  #define ETH_BUFFER_TYPE		ETH_BUFFER_UNCACHED	/* Type of EMAC DMA Descriptors & buffers		*/
#endif												/* Use one of the above defines					*/

#define ETH_IS_BUF_CACHED		((ETH_BUFFER_TYPE) & 1)
#define ETH_IS_BUF_UNCACHED		(!(ETH_IS_BUF_CACHED))
#define ETH_IS_BUF_PBUF			((ETH_BUFFER_TYPE) & 2)

													/* -------------------------------------------- */
#ifndef ETH_RX_BUFSIZE								/* DMA buffer sizes & # of buffers				*/
  #define ETH_RX_BUFSIZE		1536				/* Size in bytes of the DMA buffers				*/
#endif												/* Only 1524 is needed, but set to N*32 if the	*/
#ifndef ETH_TX_BUFSIZE								/* buff lands in the cache + flush / invalidate	*/
  #define ETH_TX_BUFSIZE		1536				/* Size in bytes of the DMA buffers				*/
#endif

#ifndef ETH_N_RXBUF
 #if (ETH_IS_BUF_UNCACHED)
  #define ETH_N_RXBUF			4					/* # of RX buffers the DMA can use (uncached)	*/
 #else
  #define ETH_N_RXBUF			64					/* # of RX buffers the DMA can use (cached)		*/
 #endif
#endif
#ifndef ETH_N_TXBUF
 #if (ETH_IS_BUF_UNCACHED)
  #define ETH_N_TXBUF			4					/* # of TX buffers the DMA can use (uncached)	*/
 #else
  #define ETH_N_TXBUF			64					/* # of TX buffers the DMA can use (cached)		*/
 #endif
#endif

#if ((ETH_RX_BUFSIZE) > 0x1FFF)
	#error "ETH_RX_BUFSIZE exceeds the DMA maximum transfer size" 
#endif
#if ((ETH_TX_BUFSIZE) > 0x1FFF)
	#error "ETH_TX_BUFSIZE exceeds the DMA maximum transfer size" 
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Stuff used to read/write the EMAC registers														*/

extern volatile uint32_t * const G_EMACaddr[];
extern const int                 G_EMACreMap[];

#define EMACreg(dev,x)			(*(G_EMACaddr[dev]+(x)))
#define ETH_MAP_DEV(dev)		(G_EMACreMap[dev])

/* ------------------------------------------------------------------------------------------------ */
/* EMAC Register definitions																		*/

#define EMAC_CONF_REG					(0x0000/4)
  #define EMAC_CONF_PRELEN				  (3<<0)
  #define EMAC_CONF_PRELEN_SHF		 	  (0)
  #define EMAC_CONF_RE					  (1<<2)
  #define EMAC_CONF_TE					  (1<<3)
  #define EMAC_CONF_DC					  (1<<4)
  #define EMAC_CONF_BL					  (3<<5)
  #define EMAC_CONF_BL_SHF				  (5)
  #define EMAC_CONF_ACS					  (1<<7)
  #define EMAC_CONF_LUD					  (1<<8)
  #define EMAC_CONF_DR					  (1<<9)
  #define EMAC_CONF_IPC					  (1<<10)
  #define EMAC_CONF_DM					  (1<<11)
  #define EMAC_CONF_LM					  (1<<12)
  #define EMAC_CONF_DO					  (1<<13)
  #define EMAC_CONF_FES					  (1<<14)
  #define EMAC_CONF_PS					  (1<<15)
  #define EMAC_CONF_DCRS				  (1<<16)
  #define EMAC_CONF_IFG					  (7<<17)
  #define EMAC_CONF_IFG_SHF				  (17)
  #define EMAC_CONF_JE					  (1<<20)
  #define EMAC_CONF_BE					  (1<<21)
  #define EMAC_CONF_JD					  (1<<22)
  #define EMAC_CONF_WD					  (1<<23)
  #define EMAC_CONF_TC					  (1<<24)
  #define EMAC_CONF_CST					  (1<<25)
  #define EMAC_CONF_TWOKPE				  (1<<27)
#define EMAC_MAC_FILT_REG				(0x0004/4)
  #define EMAC_MAC_FILT_PR				  (1<<0)
  #define EMAC_MAC_FILT_RA				  (1<<31)
#define EMAC_GMII_ADDR_REG				(0x0010/4)
  #define EMAC_GMII_ADDR_GB				  (1<<0)
  #define EMAC_GMII_ADDR_GW				  (1<<1)
  #define EMAC_GMII_ADDR_CR				  (0xF<<2)
  #define EMAC_GMII_ADDR_CR_SHF			  (2)
	#define EMAC_GMII_ADDR_CR_E_DIV42		(0)			/* l3_sp_clk/42 ( l3_sp_clk: 60-100MHz)		*/
	#define EMAC_GMII_ADDR_CR_E_DIV62		(1)			/* l3_sp_clk/62  (l3_sp_clk:100-150MHz)		*/
	#define EMAC_GMII_ADDR_CR_E_DIV16		(2)			/* l3_sp_clk/16  (l3_sp_clk: 25- 35MHz)		*/
	#define EMAC_GMII_ADDR_CR_E_DIV26		(3)			/* l3_sp_clk/26  (l3_sp_clk: 35- 60MHz)		*/
	#define EMAC_GMII_ADDR_CR_E_DIV102		(4)			/* l3_sp_clk/102 (l3_sp_clk:150-250MHz)		*/
	#define EMAC_GMII_ADDR_CR_E_DIV124		(5)			/* l3_sp_clk/124 (l3_sp_clk:250-300MHz)		*/
	#define EMAC_GMII_ADDR_CR_E_DIV4		(8)			/* l3_sp_clk/4								*/
	#define EMAC_GMII_ADDR_CR_E_DIV6		(9)			/* l3_sp_clk/6								*/
	#define EMAC_GMII_ADDR_CR_E_DIV8		(0xA)		/* l3_sp_clk/8								*/
	#define EMAC_GMII_ADDR_CR_E_DIV10		(0xB)		/* l3_sp_clk/10								*/
	#define EMAC_GMII_ADDR_CR_E_DIV12		(0xC)		/* l3_sp_clk/12								*/
	#define EMAC_GMII_ADDR_CR_E_DIV14		(0xD)		/* l3_sp_clk/14								*/
	#define EMAC_GMII_ADDR_CR_E_DIV16AGAIN	(0xE)		/* l3_sp_clk/16								*/
	#define EMAC_GMII_ADDR_CR_E_DIV18		(0xF)		/* l3_sp_clk/18								*/
  #define EMAC_GMII_ADDR_GR				  (0x1F<<6)
  #define EMAC_GMII_ADDR_GR_SHF		 	  (6)
  #define EMAC_GMII_ADDR_PA				  (0x1F<<11)
  #define EMAC_GMII_ADDR_PA_SHF		 	  (11)
#define EMAC_GMII_DATA_REG				(0x0014/4)
#define EMAC_FLOW_CTL_REG				(0x0018/4)
  #define EMAC_FLOW_CTL_FCA_BPA			  (1<<0)
  #define EMAC_FLOW_CTL_TFE				  (1<<1)
  #define EMAC_FLOW_CTL_RFE				  (1<<2)
  #define EMAC_FLOW_CTL_UP				  (1<<3)
  #define EMAC_FLOW_CTL_PLT				  (3<<4)
  #define EMAC_FLOW_CTL_PLT_SHF		 	  (4)
  #define EMAC_FLOW_CTL_DZPQ			  (1<<7)
  #define EMAC_FLOW_CTL_PT				  (0xFFFF<<16)
  #define EMAC_FLOW_CTL_PT_SHF		 	  (16)
#define EMAC_VLAN_TAG_REG				(0x001C/4)
#define EMAC_LPI_CTL_STAT_REG			(0x0030/4)
#define EMAC_LPI_TIM_CTL_REG			(0x0034/4)
#define EMAC_INT_STAT_REG				(0x0038/4)
  #define EMAC_INT_STAT_RGSMIIIS		  (1<<0)
  #define EMAC_INT_STAT_PCSLCHGIS		  (1<<1)
  #define EMAC_INT_STAT_PCSANCIS		  (1<<2)
  #define EMAC_INT_STAT_MMCIS			  (1<<4)
  #define EMAC_INT_STAT_MMCRXIS			  (1<<5)
  #define EMAC_INT_STAT_MMCTXIS			  (1<<6)
  #define EMAC_INT_STAT_MMCRXIPIS		  (1<<7)
  #define EMAC_INT_STAT_TSIS			  (1<<9)
  #define EMAC_INT_STAT_LPIIS			  (1<<10)
#define EMAC_INT_MASK_REG				(0x003C/4)
#define EMAC_MAC_ADDR_HI_REG			(0x0040/4)
#define EMAC_MAC_ADDR_LO_REG			(0x0044/4)
  #define ETH_MAC_Address0				  (0x00/4)
  #define ETH_MAC_Address1				  (0x08/4)
  #define ETH_MAC_Address2				  (0x10/4)
  #define ETH_MAC_Address3				  (0x18/4)
  #define ETH_MAC_Address4				  (0x20/4)
  #define ETH_MAC_Address5				  (0x28/4)
  #define ETH_MAC_Address6				  (0x30/4)
  #define ETH_MAC_Address7				  (0x38/4)
  #define ETH_MAC_Address8				  (0x40/4)
  #define ETH_MAC_Address9				  (0x48/4)
  #define ETH_MAC_Address10				  (0x50/4)
  #define ETH_MAC_Address11				  (0x58/4)
  #define ETH_MAC_Address12				  (0x60/4)
  #define ETH_MAC_Address13				  (0x68/4)
  #define ETH_MAC_Address14				  (0x70/4)
  #define ETH_MAC_Address15				  (0x78/4)
#define EMAC_PHY_CTL_STAT_REG			(0x00D8/4)
  #define EMAC_LNK_MOD					  (1<<0)
	#define EMAC_LNK_HALFDUP				(0<<0)
	#define EMAC_LNK_FULLDUP				(1<<0)
  #define EMAC_LNK_SPEED_MASK			  (3<<1)
	#define EMAC_LNK_SPEED_2_5MHZ			(0<<1)
	#define EMAC_LNK_SPEED_25MHZ			(1<<1)
	#define EMAC_LNK_SPEED_125MHZ			(2<<1)
  #define EMAC_LNK_STAT					  (1<<3)
	#define EMAC_LNK_STAT_DOWN				(0<<3)
	#define EMAC_LNK_STAT_UP				(1<<3)
#define EMAC_MMC_CONTROL_REG			(0x0100/4)
  #define EMACS_MMC_CTL_CNTRST				(1<<0)
  #define EMACS_MMC_CTL_CNTSTOPRO			(1<<1)
  #define EMACS_MMC_CTL_RSTONRD				(1<<2)
  #define EMACS_MMC_CTL_CNTFREEZ			(1<<3)
  #define EMACS_MMC_CTL_CNTPRST				(1<<4)
  #define EMACS_MMC_CTL_CNTPRSTLVL			(1<<5)
  #define EMACS_MMC_CTL_UCDBC				(1<<8)
#define EMAC_MMC_TX_INT_STAT_REG		(0x0104/4)
#define EMAC_MMC_RX_INT_STAT_REG		(0x0108/4)
#define EMAC_MMC_TX_INT_MASK_REG		(0x010C/4)
#define EMAC_MMC_RX_INT_MASK_REG		(0x0110/4)
#define EMAC_MMC_CKS_INT_MASK_REG		(0x0200/4)
#define EMAC_MMC_CKS_INT_STAT_REG		(0x0208/4)
#define EMAC_DMA_BUS_REG				(0x1000/4)
  #define EMAC_DMA_BUS_SWR				  (1<<0)
  #define EMAC_DMA_BUS_DSL				  (0x1F<<2)
  #define EMAC_DMA_BUS_DSL_SHF		 	 (2)
  #define EMAC_DMA_BUS_ATDS				  (1<<7)
  #define EMAC_DMA_BUS_PBL				  (0x3F<<8)
  #define EMAC_DMA_BUS_PBL_SHF			  (8)
  #define EMAC_DMA_BUS_FB				  (1<<16)
  #define EMAC_DMA_BUS_RPBL				  (0x3F<<17)
  #define EMAC_DMA_BUS_RPBL_SHF			  (17)
  #define EMAC_DMA_BUS_USP				  (1<<23)
  #define EMAC_DMA_BUS_EIGHTXPBL		  (1<<24)
  #define EMAC_DMA_BUS_AAL				  (1<<25)
#define EMAC_DMA_TX_POLL_REG			(0x1004/4)
#define EMAC_DMA_RX_POLL_REG			(0x1008/4)
#define EMAC_DMA_RXDSC_ADDR_REG			(0x100C/4)
#define EMAC_DMA_TXDSC_ADDR_REG			(0x1010/4)
#define EMAC_DMA_STAT_REG				(0x1014/4)
  #define EMAC_DMA_STAT_TI				  (1<<0)
  #define EMAC_DMA_STAT_TPS				  (1<<1)
  #define EMAC_DMA_STAT_TU				  (1<<2)
  #define EMAC_DMA_STAT_TJT				  (1<<3)
  #define EMAC_DMA_STAT_OVF				  (1<<4)
  #define EMAC_DMA_STAT_UNF				  (1<<5)
  #define EMAC_DMA_STAT_RI				  (1<<6)
  #define EMAC_DMA_STAT_RU				  (1<<7)
  #define EMAC_DMA_STAT_RPS				  (1<<8)
  #define EMAC_DMA_STAT_RWT				  (1<<9)
  #define EMAC_DMA_STAT_ETI				  (1<<10)
  #define EMAC_DMA_STAT_FBI				  (1<<13)
  #define EMAC_DMA_STAT_ERI				  (1<<14)
  #define EMAC_DMA_STAT_AIS				  (1<<15)
  #define EMAC_DMA_STAT_NIS				  (1<<16)
  #define EMAC_DMA_STAT_RS_MASK			  (7<<17)
  #define EMAC_DMA_STAT_TS_MASK			  (7<<20)
  #define EMAC_DMA_STAT_EB_MASK			  (7<<23)
  #define EMAC_DMA_STAT_GLI				  (1<<26)
  #define EMAC_DMA_STAT_GMI				  (1<<29)
  #define EMAC_DMA_STAT_GPLII			  (1<<30)
#define EMAC_DMA_OPMODE_REG				(0x1018/4)
  #define EMAC_DMA_OPMODE_SR			  (1<<1)
  #define EMAC_DMA_OPMODE_OSF			  (1<<2)
  #define EMAC_DMA_OPMODE_RTC			  (3<<3)
  #define EMAC_DMA_OPMODE_RTC_SHF		  (3)
  #define EMAC_DMA_OPMODE_FUF			  (1<<6)
  #define EMAC_DMA_OPMODE_FEF			  (1<<7)
  #define EMAC_DMA_OPMODE_EFC			  (1<<8)
  #define EMAC_DMA_OPMODE_RFA			  (3<<10)
  #define EMAC_DMA_OPMODE_RFA_SHF		  (10)
  #define EMAC_DMA_OPMODE_RFD			  (3<<12)
  #define EMAC_DMA_OPMODE_RFD_SHF		  (12)
  #define EMAC_DMA_OPMODE_ST			  (1<<13)
  #define EMAC_DMA_OPMODE_TTC			  (7<<17)
  #define EMAC_DMA_OPMODE_TTC_SHF		  (14)
  #define EMAC_DMA_OPMODE_FTF			  (1<<20)
  #define EMAC_DMA_OPMODE_TSF			  (1<<21)
  #define EMAC_DMA_OPMODE_DFF			  (1<<24)
  #define EMAC_DMA_OPMODE_RSF			  (1<<25)
  #define EMAC_DMA_OPMODE_DT			  (1<<26)
#define EMAC_DMA_INT_REG				  (0x101C/4)
  #define EMAC_DMA_INT_TIE				  (1<<0)
  #define EMAC_DMA_INT_TSE				  (1<<1)
  #define EMAC_DMA_INT_TUE				  (1<<2)
  #define EMAC_DMA_INT_TJE				  (1<<3)
  #define EMAC_DMA_INT_OVE				  (1<<4)
  #define EMAC_DMA_INT_UNE				  (1<<5)
  #define EMAC_DMA_INT_RIE				  (1<<6)
  #define EMAC_DMA_INT_RUE				  (1<<7)
  #define EMAC_DMA_INT_RSE				  (1<<8)
  #define EMAC_DMA_INT_RWE				  (1<<9)
  #define EMAC_DMA_INT_ETE				  (1<<10)
  #define EMAC_DMA_INT_FBE				  (1<<13)
  #define EMAC_DMA_INT_ERE				  (1<<14)
  #define EMAC_DMA_INT_AIE				  (1<<15)
  #define EMAC_DMA_INT_NIE				  (1<<16)

#define EMAC_DMA_MF_OVFL_CNT_REG		(0x1020/4)
#define EMAC_DMA_AXI_BUS_REG			(0x1028/4)

/*--------------------------------------------------------------------------------------------------*/
/* EMAC DMA stuff																					*/
													/* If check-sum is in HW or  not				*/
  #define ETH_HW_CHKSUM	((uint32_t)0x00C00000)		/* TO set/clear in ETH_DMAdesc_t.Status			*/

typedef struct ETH_DMAdesc {
  volatile uint32_t   Status;						/* Status										*/
  uint32_t   ControlBufferSize;						/* Control and Buffer1, Buffer2 lengths			*/
  uint32_t   Buff;									/* Buffer1 address pointer						*/
  struct ETH_DMAdesc *NextDesc;						/* Buffer2 or next descriptor address pointer	*/
 #ifdef USE_ENHANCED_DMA_DESCRIPTORS				/* Enhanced ETHERNET DMA PTP Descriptors		*/
  uint32_t   ExtendedStatus;						/* Extended status for PTP receive descriptor	*/
  uint32_t   Reserved1;								/* Reserved										*/
  uint32_t   TimeStampLow;							/* Time Stamp Low value for transmit & receive	*/
  uint32_t   TimeStampHigh;							/* Time Stamp High value for transmit & receive	*/
  #endif
 #if ((ETH_IS_BUF_PBUF) != 0)
  void    *PbufPtr;
  uint32_t PadSkip;
 #endif
 #if ((ETH_IS_BUF_CACHED) != 0)						/* When descriptors are cached, make sure each	*/
  #ifndef USE_ENHANCED_DMA_DESCRIPTORS				/* descriptor occupies a size that fits exactly	*/
   #if ((ETH_IS_BUF_PBUF) != 0)						/* within two cache line boundaries				*/
	int32_t Padding[2];								/* If not, other descriptors would be affected	*/
   #else											/* by invalidation & flushing					*/
	int32_t Padding[4];
   #endif
  #else
   #if  ((ETH_IS_BUF_PBUF) != 0)
	int32_t Padding[6];
   #endif
  #endif
 #endif
} ETH_DMAdesc_t;

typedef struct{
  int32_t  length;
  uint32_t buffer;
  volatile ETH_DMAdesc_t *descriptor;
} FrameTypeDef;

typedef struct  {
  volatile ETH_DMAdesc_t *FS_Rx_Desc;				/* First Segment Rx Desc						*/
  volatile ETH_DMAdesc_t *LS_Rx_Desc;				/* Last Segment Rx Desc							*/
  volatile uint32_t       Seg_Count;				/* Segment count								*/
} ETH_DMArxInfo_t;

/* ------------------------------------------------------------------------------------------------ */
/* EMAC driver functions prototypes																	*/
/*																									*/
/* NOTE:																							*/
/* 		Do not use PHY_init or ETH_LinkStatus in a interrupt as they are mutex protected			*/


void         ETH_BackPressureDisable             (int Dev);
void         ETH_BackPressureEnable              (int Dev);
void         ETH_DMARxDescChainInit              (int Dev);
void         ETH_DMARxDescReceiveITConfig        (int Dev, int NewState);
void         ETH_DMARxDescTransmitITConfig       (int Dev, int NewState);
void         ETH_DMATxDescChainInit              (int Dev);
void         ETH_DMATxDescChecksumInsertionConfig(int Dev, int ChkSum);
int          ETH_FlowControlStatus               (int Dev);
FrameTypeDef ETH_Get_Received_Frame              (int Dev);
FrameTypeDef ETH_Get_Received_Multi              (int Dev);
int          ETH_Get_Received_Frame_Length       (int Dev);
void        *ETH_Get_Transmit_Buffer             (int Dev);
void         ETH_IRQclr                          (int Dev, uint32_t IQRflags);
uint32_t     ETH_IRQget                          (int Dev);
void         ETH_IRQset                          (int Dev, uint32_t IRQflags);
int          ETH_LinkStatus                      (int Dev);					/* *** SEE NOTE ABOVE	*/
int         mETH_LinkStatus                      (int Dev, int PHYaddr);	/* *** SEE NOTE ABOVE	*/
void         ETH_MACAddressConfig                (int Dev, uint32_t MacAddr, uint8_t *Addr);
void         ETH_MACaddressGet                   (int Dev, uint32_t MacAddr, uint8_t *Addr);
int          ETH_MacConfigDMA                    (int Dev, int Rate);
void         ETH_PauseCtrlFrame                  (int Dev);
void         ETH_PowerDown                       (int Dev);					/* *** SEE NOTE ABOVE	*/
void        mETH_PowerDown                       (int Dev, int PHYaddr);	/* *** SEE NOTE ABOVE	*/
void         ETH_PowerUp                         (int Dev);					/* *** SEE NOTE ABOVE	*/
void        mETH_PowerUp                         (int Dev, int PHYaddr);	/* *** SEE NOTE ABOVE	*/
int          ETH_Prepare_Multi_Transmit          (int Dev, int Len, int IsFirst, int IsLast);
int          ETH_Prepare_Transmit_Descriptors    (int Dev, int Len);
void         ETH_Release_Received                (int Dev);
void         ETH_ResetEMAC                       (int Dev);
void         ETH_ResetEMACs                      (void);
void         ETH_Restart                         (int Dev);
void         ETH_RestartNegotiation              (int Dev);					/* *** SEE NOTE ABOVE	*/
void        mETH_RestartNegotiation              (int Dev, int PHYaddr);	/* *** SEE NOTE ABOVE	*/
void         ETH_RXdisable                       (int Dev);
void         ETH_RXenable                        (int Dev);
void         ETH_Start                           (int Dev);
void         ETH_Stop                            (int Dev);
void         ETH_TXdisable                       (int Dev);
void         ETH_TXenable                        (int Dev);

int          PHY_init                            (int Dev, int Rate);		/* *** SEE NOTE ABOVE	*/

/* ------------------------------------------------------------------------------------------------ */
/* EMAC (DMA) driver function prototypes															*/

void         ETH_DMAclrIRQstatus                 (int Dev, uint32_t IRQflags);
void         ETH_DMAclrPendIRQ                   (int Dev);
uint32_t     ETH_DMAgetTXstate                   (int Dev);
void         ETH_DMAflushTX                      (int Dev);
uint32_t     ETH_DMAgetIRQstatus                 (int Dev);
void         ETH_DMAisrDisable                   (int Dev, uint32_t ISRflags);
void         ETH_DMAisrEnable                    (int Dev, uint32_t ISRflags);
int          ETH_DMAgetFlushTXstatus             (int Dev);
uint32_t     ETH_DMAoverflowStat                 (int Dev, uint32_t OVflagMask);
void         ETH_DMArxDisable                    (int Dev);
void         ETH_DMArxEnable                     (int Dev);
void         ETH_DMArxRestart                    (int Dev);
void         ETH_DMAtxDisable                    (int Dev);
void         ETH_DMAtxEnable                     (int Dev);
void         ETH_DMAtxRestart                    (int Dev);
void         ETH_MMCchksumDisable                (int Dev, uint32_t IRQflags);
void         ETH_MMCchksumEnable                 (int Dev, uint32_t IRQflags);
uint32_t     ETH_MMCchksumStat                   (int Dev);
void         ETH_MMCrxDisable                    (int Dev, uint32_t IRQflags);
void         ETH_MMCrxEnable                     (int Dev, uint32_t IRQflags);
uint32_t     ETH_MMCrxStat                       (int Dev);
void         ETH_MMCtxDisable                    (int Dev, uint32_t IRQflags);
void         ETH_MMCtxEnable                     (int Dev, uint32_t IRQflags);
uint32_t     ETH_MMCtxStat                       (int Dev);

/* ------------------------------------------------------------------------------------------------ */
/* External PHY I/F stuff																			*/

#define ETH_ALT_PHY_IF_INIT			0				/* Reset & communication nitialization			*/
#define ETH_ALT_PHY_IF_CFG			1				/* Extra initialization	set-up					*/
#define ETH_ALT_PHY_IF_REG_RD		2				/* Read a standard IEEE register				*/
#define ETH_ALT_PHY_IF_REG_WRT		3				/* Write to a standard IEEE register			*/
#define ETH_ALT_PHY_IF_GET_RATE		4				/* Return selected Rate (see PHY_init()			*/

extern int AltPhyIF(int Dev, int Cmd, int Addr, int P1, int P2);

#ifdef __cplusplus
}
#endif

#endif
 
/* EOF */
