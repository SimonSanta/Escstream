/* ------------------------------------------------------------------------------------------------ */
/* FILE :		Platform.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Defintions for all platform (Chips) specific information.							*/
/*				Are defined here two classes of information:										*/
/*					- Interrupts & DMA information													*/
/*					- The on-chip peripherals used by the supported boards							*/
/*				Refer to Platform.txt for a description on why and how this file is use				*/
/*				For the defintions of the HW peripherals on supported boards, refer to HWinfo.h		*/
/*																									*/
/*																									*/
/* Copyright (c) 2016-2019, Code-Time Technologies Inc. All rights reserved.						*/
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
/*	$Revision: 1.29 $																				*/
/*	$Date: 2019/01/10 18:07:07 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__	1

#ifdef __cplusplus
 extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------ */
/* WHEN UPDATING THIS FILE, DO NOT FORGET to UPDATE BOTH SECTIONS:									*/
/*		- CHIP INTERNALS																			*/
/*		- CHIP PERIPHERAL ASSIGNMENTS																*/


/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* CHIP INTERNALS - Chip related defintions															*/
/* These are part, or part family specific and comp[letely imdependent of board hardware.			*/
/* Development board platform specifics are below													*/

#ifndef OS_PLATFORM
  #error "OS_PLATFORM must be defined"

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00007020)	/* Zynq 7020 chip								*/
  #define DMA0_INT			46						/* DMA channel #0 interrupt number				*/
  #define DMA1_INT			47						/* DMA channel #1 interrupt number				*/
  #define DMA2_INT			48						/* DMA channel #2 interrupt number				*/
  #define DMA3_INT			49						/* DMA channel #3 interrupt number				*/
  #define DMA4_INT			72						/* DMA channel #4 interrupt number				*/
  #define DMA5_INT			73						/* DMA channel #5 interrupt number				*/
  #define DMA6_INT			74						/* DMA channel #6 interrupt number				*/
  #define DMA7_INT			75						/* DMA channel #7 interrupt number				*/
  #define EMAC0_INT			54						/* EMAC #0 interrupt number on the Zynq 7020	*/
  #define EMAC1_INT			77						/* EMAC #1 interrupt number on the Zynq 7020	*/
  #define GPIO0_INT			52						/* PS GPIO interrupt number						*/
  #define I2C0_INT			57						/* I2C #0 interrupt number on the Zynq 7020		*/
  #define I2C1_INT			80						/* I2C #1 interrupt number on the Zynq 7020		*/
  #define QSPI0_INT			51						/* QSPI #0 interrupt number on the Zynq 7020	*/
  #define SDMMC0_INT		56						/* SDMMC #0 interrupt number on the Zynq 7020	*/
  #define SDMMC1_INT		79						/* SDMMC #1 interrupt number on the Zynq 7020	*/
  #define SPI0_INT			58						/* SPI #0 interrupt number on the Zynq 7020		*/
  #define SPI1_INT			81						/* SPI #1 interrupt number on the Zynq 7020		*/
  #define UART0_INT			59						/* UART #0 interrupt number on the Zynq 7020	*/
  #define UART1_INT			82						/* UART #1 interrupt number on the Zynq 7020	*/

  #define MEMORY_DMA_ID		-1						/* When src or dst is data memory				*/

  #define OS_TICK_INT		 29						/* Private timer used for the RTOS timer tick	*/
  #define OS_TICK_PRESCL	 2						/* Zynq uses a /2 pre-scaler for private timers	*/

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00007753)	/* Ultrascale+ A53s								*/
  #define DMA0_INT			156						/* DMA channel #0 interrupt number				*/
  #define DMA1_INT			157						/* DMA channel #1 interrupt number				*/
  #define DMA2_INT			158						/* DMA channel #2 interrupt number				*/
  #define DMA3_INT			159						/* DMA channel #3 interrupt number				*/
  #define DMA4_INT			160						/* DMA channel #4 interrupt number				*/
  #define DMA5_INT			161						/* DMA channel #5 interrupt number				*/
  #define DMA6_INT			162						/* DMA channel #6 interrupt number				*/
  #define DMA7_INT			163						/* DMA channel #7 interrupt number				*/
  #define EMAC0_INT			 89						/* EMAC #0 interrupt number						*/
  #define EMAC1_INT			 91						/* EMAC #1 interrupt number						*/
  #define EMAC2_INT			 93						/* EMAC #2 interrupt number						*/
  #define EMAC3_INT			 95						/* EMAC #3 interrupt number						*/
  #define GPIO0_INT			 48						/* PS GPIO interrupt number						*/
  #define I2C0_INT			 49						/* I2C #0 interrupt number						*/
  #define I2C1_INT			 50						/* I2C #1 interrupt number						*/
  #define QSPI0_INT			 47						/* QSPI #0 interrupt number						*/
  #define SDMMC0_INT		 80						/* SDMMC #0 interrupt number					*/
  #define SDMMC1_INT		 81						/* SDMMC #1 interrupt number					*/
  #define SPI0_INT			 51						/* SPI #0 interrupt number						*/
  #define SPI1_INT			 52						/* SPI #1 interrupt number						*/
  #define UART0_INT			 53						/* UART #0 interrupt number						*/
  #define UART1_INT			 54						/* UART #1 interrupt number						*/

  #define MEMORY_DMA_ID		-1						/* When src or dst is data memory				*/

  #define OS_TICK_INT		 77						/* Uses TTC3 timer/clock #0						*/

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x0000AAC5)	/* Cyclone V / Arria V chip						*/
  #define DMA0_INT			136						/* DMA channel #0 interrupt number				*/
  #define DMA1_INT			137						/* DMA channel #1 interrupt number				*/
  #define DMA2_INT			138						/* DMA channel #2 interrupt number				*/
  #define DMA3_INT			139						/* DMA channel #3 interrupt number				*/
  #define DMA4_INT			140						/* DMA channel #4 interrupt number				*/
  #define DMA5_INT			141						/* DMA channel #5 interrupt number				*/
  #define DMA6_INT			142						/* DMA channel #6 interrupt number				*/
  #define DMA7_INT			143						/* DMA channel #7 interrupt number				*/
  #define DMA_ABORT_INT		144						/* DMA abort interrupt							*/
  #define EMAC0_INT			147						/* EMAC #0 interrupt # on Cyclone V / Arria V	*/
  #define EMAC1_INT			152						/* EMAC #1 interrupt # on Cyclone V / Arria V	*/
  #define I2C0_INT			190						/* I2C #0 interrupt # on Cyclone V / Arria V	*/
  #define I2C1_INT			191						/* I2C #1 interrupt # on Cyclone V / Arria V	*/
  #define I2C2_INT			192						/* I2C #2 interrupt # on Cyclone V / Arria V	*/
  #define I2C3_INT			193						/* I2C #3 interrupt # on Cyclone V / Arria V	*/
  #define QSPI0_INT			183						/* QSPI #0 interrupt # on Cyclone V / Arria V	*/
  #define SDMMC0_INT		171						/* SDMMC #0 interrupt # on Cyclone V / Arria V	*/
  #define SPI0_INT			186						/* SPI #0 interrupt # on Cyclone V / Arria V	*/
  #define SPI1_INT			187						/* SPI #1 interrupt # on Cyclone V / Arria V	*/
  #define SPI2_INT			188						/* SPI #2 interrupt # on Cyclone V / Arria V	*/
  #define SPI3_INT			189						/* SPI #3 interrupt # on Cyclone V / Arria V	*/
  #define UART0_INT			194						/* UART #0 interrupt # on Cyclone V / Arria V	*/
  #define UART1_INT			195						/* UART #1 interrupt # on Cyclone V / Arria V	*/
  #define GPIO0_INT			196						/* GPIO port #0 interrupt						*/
  #define GPIO1_INT			197						/* GPIO port #1 interrupt						*/
  #define GPIO2_INT			198						/* GPIO port #2 interrupt						*/

  #define FPGA0_DMA_ID		 0						/* ID of the DMA peripheral requests			*/
  #define FPGA1_DMA_ID		 1
  #define FPGA2_DMA_ID		 2
  #define FPGA3_DMA_ID		 3
  #define CAN0_IF1_DMA_ID	 4
  #define CAN0_IF2_DMA_ID	 5
  #define CAN1_IF1_DMA_ID	 6
  #define CAN1_IF2_DMA_ID	 7
  #define I2C0_TX_DMA_ID	 8
  #define I2C0_RX_DMA_ID	 9
  #define I2C1_TX_DMA_ID	10
  #define I2C1_RX_DMA_ID	11
  #define I2C2_TX_DMA_ID	12
  #define I2C2_RX_DMA_ID	13
  #define I2C3_TX_DMA_ID	14
  #define I2C3_RX_DMA_ID	15
  #define SPI0_TX_DMA_ID	16						/* Our #0 is Altera's Master 0 					*/
  #define SPI0_RX_DMA_ID	17						/* Our #0 is Altera's Master 0 					*/
  #define SPI1_TX_DMA_ID	18						/* Our #1 is Altera's Master 1 					*/
  #define SPI1_RX_DMA_ID	19						/* Our #1 is Altera's Master 1 					*/
  #define SPI2_TX_DMA_ID	20						/* Our #2 is Altera's Slave 0 					*/
  #define SPI2_RX_DMA_ID	21						/* Our #2 is Altera's Slave 0 					*/
  #define SPI3_TX_DMA_ID	22						/* Our #3 is Altera's Slave 1 					*/
  #define SPI3_RX_DMA_ID	23						/* Our #3 is Altera's Slave 1 					*/
  #define QSPI0_TX_DMA_ID	24
  #define QSPI0_RX_DMA_ID	25
  #define UART0_TX_DMA_ID	28
  #define UART0_RX_DMA_ID	29
  #define UART1_TX_DMA_ID	30
  #define UART1_RX_DMA_ID	31

  #define MEMORY_DMA_ID		-1						/* When src or dst is data memory				*/

  #define DAP_ACP_ID		0x001					/* ACP Master IDs								*/
  #define DMA_ACP_ID		0x003
  #define EMAC0_ACP_ID		0x801
  #define EMAC1_ACP_ID		0x802
  #define ETR_ACP_ID		0x800
  #define FPGA_HPS_ACP_ID	0x004
  #define NAND_ACP_ID		0x804
  #define SDMMC0_ACP_ID		0x805
  #define USB0_ACP_ID		0x803
  #define USB1_ACP_ID		0x806

  #define OS_TICK_INT		 29						/* Private timer used for the RTOS timer tick	*/
  #define OS_TICK_PRESCL	 4						/* AR5/CY5 uses /4 pre-scaler for private timers*/

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x0000AA10)	/* Arria 10 chip								*/
  #define DMA0_INT			115						/* DMA channel #0 interrupt number				*/
  #define DMA1_INT			116						/* DMA channel #1 interrupt number				*/
  #define DMA2_INT			117						/* DMA channel #2 interrupt number				*/
  #define DMA3_INT			118						/* DMA channel #3 interrupt number				*/
  #define DMA4_INT			119						/* DMA channel #4 interrupt number				*/
  #define DMA5_INT			120						/* DMA channel #5 interrupt number				*/
  #define DMA6_INT			121						/* DMA channel #6 interrupt number				*/
  #define DMA7_INT			122						/* DMA channel #7 interrupt number				*/
  #define EMAC0_INT			124						/* EMAC #0 interrupt number on the Arria 10		*/
  #define EMAC1_INT			125						/* EMAC #1 interrupt number on the Arria 10		*/
  #define EMAC2_INT			126						/* EMAC #2 interrupt number on the Arria 10		*/
  #define I2C0_INT			137						/* I2C #0 interrupt # on Cyclone V / Arria V	*/
  #define I2C1_INT			138						/* I2C #1 interrupt # on Cyclone V / Arria V	*/
  #define I2C2_INT			139						/* I2C #2 interrupt # on Cyclone V / Arria V	*/
  #define I2C3_INT			140						/* I2C #3 interrupt # on Cyclone V / Arria V	*/
  #define I2C4_INT			141						/* I2C #3 interrupt # on Cyclone V / Arria V	*/
  #define QSPI0_INT			132						/* QSPI #0 interrupt number on the Arria 10		*/
  #define SDMMC0_INT		130						/* SDMMC #0 interrupt number on the Arria 10	*/
  #define UART0_INT			142						/* UART #0 interrupt number on the Arria 10		*/
  #define UART1_INT			143						/* UART #1 interrupt number on the Arria 10		*/
  #define GPIO0_INT			144						/* GPIO port #0 interrupt						*/
  #define GPIO1_INT			145						/* GPIO port #1 interrupt						*/
  #define GPIO2_INT			146						/* GPIO port #2 interrupt						*/

  #define FPGA0_DMA_ID			 0					/* ID of the DMA peripheral requests			*/
  #define FPGA1_DMA_ID			 1
  #define FPGA2_DMA_ID			 2
  #define FPGA3_DMA_ID			 3
  #define FPGA4_DMA_ID			 4
  #define FPGA5_SM_TX_DMA_ID	 5
  #define FPGA6_EMAC2_TX_DMA_ID	 6
  #define FPGA7_EMAC2_RX_DMA_ID	 7
  #define I2C0_TX_DMA_ID		 8
  #define I2C0_RX_DMA_ID		 9
  #define I2C1_TX_DMA_ID		10
  #define I2C1_RX_DMA_ID		11
  #define I2C2_EMAC0_TX_DMA_ID	12
  #define I2C2_EMAC0_RX_DMA_ID	13
  #define I2C3_EMAC1_TX_DMA_ID	14
  #define I2C3_EMAC1_RX_DMA_ID	15
  #define SPI0_TX_DMA_ID		16					/* Our #0 is Altera's Master 0 					*/
  #define SPI0_RX_DMA_ID		17					/* Our #0 is Altera's Master 0 					*/
  #define SPI1_TX_DMA_ID		18					/* Our #1 is Altera's Master 1 					*/
  #define SPI1_RX_DMA_ID		19					/* Our #1 is Altera's Master 1 					*/
  #define SPI2_TX_DMA_ID		20					/* Our #2 is Altera's Slave 0 					*/
  #define SPI2_RX_DMA_ID		21					/* Our #2 is Altera's Slave 0 					*/
  #define SPI3_TX_DMA_ID		22					/* Our #3 is Altera's Slave 1 					*/
  #define SPI3_RX_DMA_ID		23					/* Our #3 is Altera's Slave 1 					*/
  #define QSPI0_TX_DMA_ID		24
  #define QSPI0_RX_DMA_ID		25
  #define STM_DMA_ID			26
  #define FPGA_MNG_RX_DMA_ID	27
  #define UART0_TX_DMA_ID		28
  #define UART0_RX_DMA_ID		29
  #define UART1_TX_DMA_ID		30
  #define UART1_RX_DMA_ID		31

  #define MEMORY_DMA_ID			-1					/* When src or dst is data memory				*/

  #define DAP_ACP_ID		0x004					/* ACP Master IDs								*/
  #define DMA_ACP_ID		0x001
  #define EMAC0_ACP_ID		0x801
  #define EMAC1_ACP_ID		0x802
  #define FPGA_HPS_ACP_ID	0x000
  #define L2M0_ACP_ID		0x002
  #define NAND_ACP_ID		0x804
  #define SDMMC0_ACP_ID		0x805
  #define TMC_ACP_ID		0x800
  #define USB0_ACP_ID		0x803
  #define USB1_ACP_ID		0x806

  #define OS_TICK_INT		 29						/* Private timer used for the RTOS timer tick	*/
  #define OS_TICK_PRESCL	 4						/* AR10 uses a /4 pre-scaler for private timers	*/

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x0000FEE6)	/* Freescale iMX.6								*/
  #define EMAC0_INT			150						/* EMAC #0  (Freescale defines it as ENET) 		*/
  #define I2C0_INT			 68						/* I2C #0   (Freescale defines it as I2C1) 		*/
  #define I2C1_INT			 69						/* I2C #1   (Freescale defines it as I2C2) 		*/
  #define I2C2_INT			 70						/* I2C #2   (Freescale defines it as I2C3) 		*/
  #define SDMMC0_INT		 54						/* SDMMC #0 (Freescale defines it as uSDHC1)	*/
  #define SDMMC1_INT		 55						/* SDMMC #1 (Freescale defines it as uSDHC1)	*/
  #define SDMMC2_INT		 56						/* SDMMC #2 (Freescale defines it as uSDHC1)	*/
  #define SDMMC3_INT		 57						/* SDMMC #3 (Freescale defines it as uSDHC1)	*/
  #define SPI0_INT			 63						/* SPI #0   (Freescale defiens it as eCSPI1)	*/
  #define SPI1_INT			 64						/* SPI #0   (Freescale defiens it as eCSPI2)	*/
  #define SPI2_INT			 65						/* SPI #0   (Freescale defiens it as eCSPI3)	*/
  #define SPI3_INT			 66						/* SPI #0   (Freescale defiens it as eCSPI4)	*/
  #define SPI4_INT			 67						/* SPI #0   (Freescale defiens it as eCSPI5)	*/
  #define UART0_INT			 58						/* UART #0  (Freescale defines it as UART1) 	*/
  #define UART1_INT			 59						/* UART #1  (Freescale defines it as UART2) 	*/
  #define UART2_INT			 60						/* UART #2  (Freescale defines it as UART3) 	*/
  #define UART3_INT			 61						/* UART #3  (Freescale defines it as UART4) 	*/
  #define UART4_INT			 62						/* UART #4  (Freescale defines it as UART5) 	*/

  #define OS_TICK_INT		 29						/* Private timee used for the RTOS timer tick	*/
  #define OS_TICK_PRESCL	  2						/* I.MX6 uses /2 pre-scaler for private timers	*/

#elif (((OS_PLATFORM) & 0x00FF0000) == 0x000C0000)	/* NXP LPC line of Cortex M0					*/

#elif (((OS_PLATFORM) & 0x00FF0000) == 0x00350000)	/* TI's Stellaris								*/
  #define UART0_INT			 5
  #define UART1_INT			 6
  #define UART2_INT			33

  #define OS_TICK_INT		-1						/* The system timer is used for RTOS tomer tick	*/

#elif (((OS_PLATFORM) & 0x00FFFFF0) == 0x003C1120)	/* LPC11U2X							 			*/
  #define UART0_INT			 21						/* Only 1 UART on this chip						*/

  #define OS_TICK_INT		-1						/* The system timer is used for RTOS tomer tick	*/

#elif (((OS_PLATFORM) & 0x00FFFFF0) == 0x003C1220)	/* LPC122X							 			*/
  #define UART0_INT			 6
  #define UART1_INT			 7

  #define OS_TICK_INT		-1						/* The system timer is used for RTOS tomer tick	*/

#elif ((((OS_PLATFORM) & 0x00FFFFFF) == 0x003C1311)	/* LPC1311 / LPC1313 / LPC1342 / LPC1343	*/	\
   ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x003C1313)													\
   ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x003C1342)													\
   ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x003C1343))
  #define UART0_INT			 46						/* Only 1 UART on this chip						*/
  #define OS_TICK_INT		-1						/* The system timer is used for RTOS tomer tick	*/

#elif (((OS_PLATFORM) & 0x00FFFFF0) == 0x003C1760)	/* NXP LPC176X line of Cortex M3				*/
  #define UART0_INT			 5
  #define UART1_INT			 6
  #define UART2_INT			 7
  #define UART3_INT			 8

  #define OS_TICK_INT		-1						/* The system timer is used for RTOS tomer tick	*/

#elif (((OS_PLATFORM) & 0x00FFFF00) == 0x0032F100)	/* STM 32F1XX chips								*/
  #define DMA1_S0_INT		11						/* DMA #1 stream 0 interrupt number				*/
  #define DMA1_S1_INT		12						/* DMA #1 stream 1 interrupt number				*/
  #define DMA1_S2_INT		13						/* DMA #1 stream 2 interrupt number				*/
  #define DMA1_S3_INT		14						/* DMA #1 stream 3 interrupt number				*/
  #define DMA1_S4_INT		15						/* DMA #1 stream 4 interrupt number				*/
  #define DMA1_S5_INT		16						/* DMA #1 stream 5 interrupt number				*/
  #define DMA1_S6_INT		17						/* DMA #1 stream 6 interrupt number				*/
  #define DMA2_S0_INT		56						/* DMA #2 stream 0 interrupt number				*/
  #define DMA2_S1_INT		57						/* DMA #2 stream 1 interrupt number				*/
  #define DMA2_S2_INT		58						/* DMA #2 stream 2 interrupt number				*/
  #define DMA2_S3_INT		59						/* DMA #2 stream 3 interrupt number				*/
  #define DMA2_S4_INT		60						/* DMA #2 stream 4 interrupt number				*/
  #define DMA2_S5_INT		68						/* DMA #2 stream 5 interrupt number				*/
  #define DMA2_S6_INT		69						/* DMA #2 stream 6 interrupt number				*/
  #define DMA2_S7_INT		70						/* DMA #2 stream 7 interrupt number				*/
  #define EMAC0_INT			61						/* EMAC #0 interrupt number						*/
  #define EXTI0_INT			 6						/* EXTI 0 line interrupt number					*/
  #define EXTI1_INT			 7						/* EXTI 1 line interrupt number					*/
  #define EXTI2_INT			 8						/* EXTI 2 line interrupt number					*/
  #define EXTI3_INT			 9						/* EXTI 3 line interrupt number					*/
  #define EXTI4_INT			10						/* EXTI 4 line interrupt number					*/
  #define EXTI9_5_INT		23						/* EXTI 9:5 lines interrupt number				*/
  #define EXTI15_10_INT		40						/* EXTI 15:10 lines interrupt number			*/
  #define SDMMC0_INT		49						/* SDMMC #0 interrupt number					*/
  #define UART0_INT			37						/* UART #0 interrupt number (ST's USART1)		*/
  #define UART1_INT			38						/* UART #1 interrupt number (ST's USART2)		*/
  #define UART2_INT			39						/* UART #2 interrupt number (ST's USART3)		*/
  #define UART3_INT			52						/* UART #3 interrupt number (ST's USART4)		*/
  #define UART4_INT			53						/* UART #4 interrupt number (ST's USART5)		*/

  #define OS_TICK_INT		-1						/* The system timer is used for RTOS tomer tick	*/

#elif ((((OS_PLATFORM) & 0x00FFFFFF) == 0x0032F205)	/* STM 32F205 chip							   */\
   ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x0032F207)	/* STM 32F207 chip							   */\
   ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x0032F215)	/* STM 32F215 chip							   */\
   ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x0032F217)	/* STM 32F217 chip							   */\
   ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x0032F405)	/* STM 32F405 chip							   */\
   ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x0032F407)	/* STM 32F407 chip							   */\
   ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x0032F415)	/* STM 32F415 chip							   */\
   ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x0032F417))/* STM 32F417 chip								*/
  #define DMA1_S0_INT		11						/* DMA #1 stream 0 interrupt number				*/
  #define DMA1_S1_INT		12						/* DMA #1 stream 1 interrupt number				*/
  #define DMA1_S2_INT		13						/* DMA #1 stream 2 interrupt number				*/
  #define DMA1_S3_INT		14						/* DMA #1 stream 3 interrupt number				*/
  #define DMA1_S4_INT		15						/* DMA #1 stream 4 interrupt number				*/
  #define DMA1_S5_INT		16						/* DMA #1 stream 5 interrupt number				*/
  #define DMA1_S6_INT		17						/* DMA #1 stream 6 interrupt number				*/
  #define DMA1_S7_INT		47						/* DMA #1 stream 7 interrupt number				*/
  #define DMA2_S0_INT		56						/* DMA #2 stream 0 interrupt number				*/
  #define DMA2_S1_INT		57						/* DMA #2 stream 1 interrupt number				*/
  #define DMA2_S2_INT		58						/* DMA #2 stream 2 interrupt number				*/
  #define DMA2_S3_INT		59						/* DMA #2 stream 3 interrupt number				*/
  #define DMA2_S4_INT		60						/* DMA #2 stream 4 interrupt number				*/
  #define DMA2_S5_INT		68						/* DMA #2 stream 5 interrupt number				*/
  #define DMA2_S6_INT		69						/* DMA #2 stream 6 interrupt number				*/
  #define DMA2_S7_INT		70						/* DMA #2 stream 7 interrupt number				*/
  #define EMAC0_INT			61						/* EMAC #0 interrupt number						*/
  #define EXTI0_INT			 6						/* EXTI 0 line interrupt number					*/
  #define EXTI1_INT			 7						/* EXTI 1 line interrupt number					*/
  #define EXTI2_INT			 8						/* EXTI 2 line interrupt number					*/
  #define EXTI3_INT			 9						/* EXTI 3 line interrupt number					*/
  #define EXTI4_INT			10						/* EXTI 4 line interrupt number					*/
  #define EXTI9_5_INT		23						/* EXTI 9:5 lines interrupt number				*/
  #define EXTI15_10_INT		40						/* EXTI 15:10 lines interrupt number			*/
  #define SDMMC0_INT		49						/* SDMMC #0 interrupt number					*/
  #define UART0_INT			37						/* UART #0 interrupt number (ST's USART1)		*/
  #define UART1_INT			38						/* UART #1 interrupt number (ST's USART2)		*/
  #define UART2_INT			39						/* UART #2 interrupt number (ST's USART3)		*/
  #define UART3_INT			52						/* UART #3 interrupt number (ST's USART4)		*/
  #define UART4_INT			53						/* UART #4 interrupt number (ST's USART5)		*/
  #define UART5_INT			71						/* UART #5 interrupt number (ST's USART5)		*/

  #define OS_TICK_INT		-1						/* The system timer is used for RTOS tomer tick	*/

#elif (((OS_PLATFORM) & 0x00FFF000) == 0x00AE8000)	/* Atmel AVR									*/

#elif (((OS_PLATFORM) & 0x00FFF000) == 0x00AEA000)	/* Atmel AV32A									*/
  #define UART0_INT			0x0500					/* Group is MSByte, line is LSByte				*/
  #define UART1_INT			0x0600
  #define UART2_INT			0x0700
  #define UART3_INT			0x0800

#elif (((OS_PLATFORM) & 0x00FF0000) == 0x00CF0000)	/* Freescale ColdFire							*/

#elif (((OS_PLATFORM) & 0x00FFF000) == 0x00028000)	/* TI TMS320C28XX								*/

#elif (((OS_PLATFORM) & 0x00FF0000) == 0x00430000)	/* TI MSP430 & TI MSP430X						*/

#elif (((OS_PLATFORM) & 0x00FFF000) == 0x00320000)	/* PIC32										*/

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00008051)	/* Generic 8051									*/

#elif (((OS_PLATFORM) & 0x00FFFFFF) == 0x00080251)	/* Generic 80251								*/

#else
  #error "Invalid target platform defined by OS_PLATFORM"

#endif

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* CHIP PERIPHERAL ASSIGNMENTS - Hardware platform (dev board) related defitnions					*/
/* The are not part, or part family specific. Instead, they are specific to the board hardware		*/
/* Chips or chip family specifics are above															*/

/* NOTE:																							*/
/* There is an errata (AR #47511) for the Zynq SPI SS0 inmaster mode set-up. The FPGA bitfile		*/
/* provided does not have the required repair. That's why the default SPI_SLV is set to 1			*/

#if   ((OS_PLATFORM) == 0x00007020)					/* Zynq 7020 / Zedboard							*/
  #define HOST_NAME			"ZedBoard"				/* Name of the WebServer platform				*/
  #define SYS_FREQ			666666667				/* Processor clock frequency: 666 Hz			*/
  #define EMAC_DEV			0						/* EMAC device on Zedboard is #0				*/
  #define I2C_DEV			0						/* I2C device ID on Zedboard is #0				*/
  #define QSPI_DEV			0						/* QSPI device ID on Zedboard is #0				*/
  #define QSPI_SLV			0						/* QSPI slave ID on Zedboard is #0				*/
  #define SDMMC_DEV			0						/* SD/MMC device on Zedboard is #0				*/
  #define SPI_DEV			1						/* SPI connected on JE1 connector				*/
  #define SPI_SLV			1						/* SPI connected on JE1 (*** See Note above)	*/
  #define SPI_OLED_DEV		0						/* SPI connected on the OLED					*/
  #define SPI_OLED_SLV		0						/* SPI connected on the OLED					*/
  #define UART_DEV			1						/* UART on Zedboard is #1						*/

#elif ((OS_PLATFORM) == 0x00007753)					/* UltraScale+ / ZCU102 board					*/
  #define HOST_NAME			"ZCU-102"				/* Name of the WebServer platform				*/
  #define SYS_FREQ			1200000000				/* Processor clock frequency: 1.2 GHz			*/
  #define OS_TICK_SRC_FREQ	 100000000				/* Using the TTC for the RTOS timer tick		*/
  #define EMAC_DEV			3
  #define I2C_DEV			0						/* I2C on the PMOD J160 connector				*/
  #define QSPI_DEV			0
  #define QSPI_SLV			0
  #define SDMMC_DEV			1
//  #define SPI_DEV			-
//  #define SPI_SLV			-
  #define UART_DEV			0						/* UART on ZCU-102 is #1						*/

#elif ((OS_PLATFORM) == 0x0000AAC5)					/* Cyclone V SocFPGA dev board					*/
  #define HOST_NAME			"Cyclone V Soc"			/* Name of the WebServer platform				*/
  #define SYS_FREQ			925000000				/* Processor clock frequency: 925 MHz			*/
  #define EMAC_DEV			1						/* EMAC device used on this board				*/
  #define I2C_DEV			0						/* I2C device on this board						*/
  #define QSPI_DEV			0						/* QSPI device on this board					*/
  #define QSPI_SLV			0						/* QSPI slave ID on this board					*/
  #define SDMMC_DEV			0						/* SDMMC device used on this board				*/
  #define SPI_DEV			0						/* LT connector when J31 is shorted				*/
  #define SPI_SLV			0						/* LT connector when J31 is shorted (1 slave)	*/
  #define UART_DEV			0						/* UART device used on this board				*/
 
#elif ((OS_PLATFORM) == 0x0100AAC5)					/* DE0-nano / DE10-nano							*/
  #define HOST_NAME			"DE0 Nano SoC"			/* Name of the WebServer platform				*/
  #define SYS_FREQ			925000000				/* Processor clock frequency: 925 MHz			*/
  #define EMAC_DEV			1						/* EMAC device used on this board				*/
  #define I2C_DEV			0						/* I2C device on this board						*/
  #define SDMMC_DEV			0						/* SDMMC device used on this board				*/
  #define SPI_DEV			1						/* LT connector uses SPI master #1				*/
  #define SPI_SLV			0						/* LT connector	uses SPI master #1				*/
  #define UART_DEV			0						/* UART device used on this board				*/

#elif ((OS_PLATFORM) == 0x0200AAC5)					/* EBV Socrates board (Cyclone V)				*/
  #define HOST_NAME			"EBV Socrates"			/* Name of the WebServer platform				*/
  #define SYS_FREQ			800000000				/* Processor clock frequency: 800 MHz			*/
  #define EMAC_DEV			1						/* EMAC device used on this board				*/
  #define I2C_DEV			0						/* I2C device on this board						*/
  #define SDMMC_DEV			0						/* SDMMC device used on this board				*/
  #define UART_DEV			0						/* UART device used on this board				*/
 
#elif ((OS_PLATFORM) == 0x0300AAC5)					/* SOCkit evaluation board (Cyclone V)			*/
  #define HOST_NAME			"SocKit Cyclone V"		/* Name of the WebServer platform				*/
  #define SYS_FREQ			800000000				/* Processor clock frequency: 800 MHz			*/
  #define EMAC_DEV			1						/* EMAC device used on this board				*/
  #define SDMMC_DEV			0						/* SDMMC device used on this board				*/
  #define UART_DEV			0						/* UART device used on this board				*/
 
#elif ((OS_PLATFORM) == 0x0400AAC5)					/* Custom board (R.C.)							*/
  #define HOST_NAME			"Custom Board 1"		/* Name of the WebServer platform				*/
  #define SYS_FREQ			925000000				/* Processor clock frequency: 800 MHz			*/
  #define EMAC_DEV			0						/* EMAC device used on this board				*/
  #define SDMMC_DEV			0						/* SDMMC device used on this board				*/
  #define UART_DEV			0						/* UART device used on this board				*/
 
#elif ((OS_PLATFORM) == 0x3F00AAC5)					/* Custom board (C.M. LLC)						*/
  #define HOST_NAME			"C.M. LLC"				/* Name of the WebServer platform				*/
  #define SYS_FREQ			925000000				/* Processor clock frequency: 925 MHz			*/
  #define EMAC_DEV			1						/* EMAC device used on this board				*/
  #define I2C_DEV			0						/* I2C device on this board						*/
  #define SDMMC_DEV			0						/* SDMMC device used on this board				*/
  #define SPI_DEV			1						/* LT connector uses SPI master #1				*/
  #define SPI_SLV			0						/* LT connector	uses SPI master #1				*/
  #define UART_DEV			0						/* UART device used on this board				*/
  #define GPIO6_INT			75						/* GPIO 4000->4031 in FPGA						*/
  #define GPIO7_INT			76						/* GPIO 5000->5031 in FPGA						*/
  #define UART2_INT			77						/* NS16550 UART#0 in FPGA						*/
  #define UART3_INT			78						/* NS16550 UART#1 1in FPGA						*/
  #define UART4_INT			79						/* NS16550 UART#2 in FPGA						*/
  #define UART5_INT			80						/* NS16550 UART#3 1in FPGA						*/

#elif ((OS_PLATFORM) == 0x4000AAC5)					/* Arria V SocFPGA dev board					*/
  #define HOST_NAME			"Arria V Soc"			/* Name of the WebServer platform				*/
  #define SYS_FREQ			1050000000				/* Processor clock frequency: 1050 MHz			*/
  #define EMAC_DEV			1						/* EMAC device used on this board				*/
  #define I2C_DEV			0						/* I2C device on this board						*/
  #define QSPI_DEV			0						/* QSPI device on this board					*/
  #define QSPI_SLV			0						/* QSPI slave ID on this board					*/
  #define SDMMC_DEV			0						/* SDMMC device used on this board				*/
  #define UART_DEV			0						/* UART device used on this board				*/

#elif ((OS_PLATFORM) == 0x0000AA10)					/* Arria 10 SocFPGA dev board					*/
  #define HOST_NAME			"Arria 10 Soc"			/* Name of the WebServer platform				*/
  #define SYS_FREQ			1200000000				/* Processor clock 1.2GHz						*/
  #define EMAC_DEV			0						/* EMAC device used on this board				*/
  #define I2C_DEV			1						/* I2C device on this board						*/
  #define QSPI_DEV			0						/* QSPI device on this board					*/
  #define QSPI_SLV			0						/* QSPI slave ID on this board					*/
  #define SDMMC_DEV			0						/* SDMMC device used on this board				*/
  #define UART_DEV			1						/* UART device on this board					*/

#elif ((OS_PLATFORM) == 0x0000FEE6)					/* I.MX6 Sabrelite								*/
  #define HOST_NAME			"I.MX6 Sabrelite"		/* Name of the WebServer platform				*/
  #define SYS_FREQ			792000000				/* Processor clock frequency: 792 MHz			*/
  #define PLL3_FREQUENCY	 80000000
  #define UART_DEV			1						/* UART port to use								*/

#elif ((OS_PLATFORM) == 0x00028027)					/* Olimex TMX320-P28027							*/
  #define HOST_NAME			"Olimex TMX320-P28027"	/* Name of the WebServer platform				*/
  #define SYS_FREQ			60000000L				/* 60 MHz clock									*/
  #define UART_DEV			0						/* Dummy as the TMS320F28027 only has 1 UART	*/

#elif ((OS_PLATFORM) == 0x00351968)					/* TI's EKLM3S1968 evaluation board				*/
  #define HOST_NAME			"ElKM3S1968"			/* Name of the WebServer platform				*/
  #define SYS_FREQ			SysCtlClockGet()		/* System clock frequency is retriedv from this	*/
  #define UART_DEV			0						/* Only 1 uart is currently supported			*/

#elif ((OS_PLATFORM) == 0x003C1124)					/* NGX Blue Board LPC11U24			 			*/
  #define HOST_NAME			"Blue Board LPC11U24"	/* Name of the WebServer platform				*/
  #define SYS_FREQ			SystemFrequency			/* Processor clock								*/
  #define UART_DEV			0						/* Only one UART on this chip					*/

#elif ((OS_PLATFORM) == 0x003C1227)					/* Olimex LPC1227-STK							*/
  #define HOST_NAME			"Olimex LPC1227-STK"	/* Name of the WebServer platform				*/
  #define SYS_FREQ			24000000				/* Processor clock								*/
  #define UART_DEV			0						/* UART#1 can also be used						*/

#elif ((OS_PLATFORM) == 0x003C1343)					/* NGX Technologies BlueBoard LPC1343 			*/
  #include "system_LPC13xx.h"
  #define HOST_NAME			"NGX BB1343"			/* Name of the WebServer platform				*/
  #define SYS_FREQ			SystemCoreClock			/* System clock frequency is retriedv from this	*/
  #define UART_DEV			0						/* Only 1 UART on that chip						*/

#elif ((OS_PLATFORM) == 0x003C1766)					/* Olimex LPC1766-STK board			 			*/
  #define HOST_NAME			"Olimex LPC1766-STK"	/* Name of the WebServer platform				*/
  #define SYS_FREQ			100000000				/* Processor clock 100MHz						*/
  #define UART_DEV			0						/* UART#1 can also be used						*/

#elif ((OS_PLATFORM) == 0x1032F107)					/* Olimex STM32-P107 board (STM32F107)			*/
  #define HOST_NAME			"Olimex STM32-P107"		/* Name of the WebServer platform				*/
  #define SYS_FREQ			72000000				/* Processor clock 72MHz						*/
  #define UART_DEV			2						/* UART device on this board (USART2, USART3)	*/

  #define UART_PIN_PACK		UART_FILT_PIN_PACK2		/* UART PIN pack used on this board				*/

#elif ((OS_PLATFORM) == 0x0032F207)					/* STM3220G development board					*/
  #define HOST_NAME			"STM STM3220G"			/* Name of the WebServer platform				*/
  #define SYS_FREQ			120000000				/* Processor clock 120MHz						*/
  #define SDMMC_DEV			0						/* SDMMC device used on this board				*/
  #define UART_DEV			2						/* UART device on this board (USART3 only)		*/

  #define EMAC0_IO_INT		EXTI15_10_INT			/* EMAC #0 GPIO on PHY interrupt number			*/
  #define SDMMC_DMA_INT		DMA2_S3_INT				/* SDMMC #0 DMA interrupt number				*/
  #define UART_PIN_PACK		UART_FILT_PIN_PACK1		/* UART PIN pack used on this board				*/

#elif ((OS_PLATFORM) == 0x1032F207)					/* Olimex STM32-P207 board (STM32F207)			*/
  #define HOST_NAME			"Olimex STM32-P207"		/* Name of the WebServer platform				*/
  #define SYS_FREQ			168000000				/* Processor clock 168 MHz						*/
  #define SDMMC_DEV			0						/* SDMMC device used on this board				*/
  #define UART_DEV			5						/* UART device on this board					*/

#elif ((OS_PLATFORM) == 0x0032F407)					/* STM32F4 Discovery board						*/
  #define HOST_NAME			"STM32F4 Discovery"		/* Name of the WebServer platform				*/
  #define SYS_FREQ			144000000				/* Processor clock 144MHz						*/
  #define EMAC_DEV			0						/* Sort fo a dummy								*/
  #define SDMMC_DEV			0						/* SDMMC device used on this board				*/
  #define UART_DEV			5						/* UART device on this board (0, 2, or 5 only)	*/

  #define EMAC0_IO_INT		EXTI15_10_INT			/* EMAC #0 GPIO on PHY interrupt number			*/
  #define SDMMC_DMA_INT		DMA2_S3_INT				/* SDMMC #0 DMA interrupt number				*/
  #define UART_PIN_PACK		UART_FILT_PIN_PACK0

#elif ((OS_PLATFORM) == 0x1032F407)					/* Olimex STM32-P407 board						*/
  #define HOST_NAME			"Olimex STM32-P407"		/* Name of the WebServer platform				*/
  #define SYS_FREQ			168000000				/* Processor clock 168MHz						*/
  #define EMAC_DEV			0						/* Sort fo a dummy								*/
  #define SDMMC_DEV			0						/* SDMMC device used on this board				*/
  #define UART_DEV			5						/* UART device on this board (0, or 5 only)		*/

  #define ADC_DMA_INT		DMA2_S0_INT				/* DMA interrupt for the ADC					*/
  #define EMAC0_IO_INT		EXTI15_10_INT			/* EMAC #0 GPIO on PHY interrupt number			*/
  #define JOYSTICK0_INT		EXTI9_5_INT				/* The joystick utilises 2 interrupts			*/
  #define JOYSTICK1_INT		EXTI15_10_INT			/* The joystick utilises 2 interrupts			*/
  #define PB_WAKEUP_INT		EXTI0_INT				/* WAKE-UP push button interrupt number			*/
  #define PB_TAMPER_INT		EXTI15_10_INT			/* TAMPER push button interrupt number			*/
  #define SDMMC_DMA_INT		DMA2_S3_INT				/* SDMMC #0 DMA interrupt number				*/
 #if ((UART_DEV) == 2)
  #define UART_PIN_PACK		UART_FILT_PIN_PACK2
 #else
  #define UART_PIN_PACK		UART_FILT_PIN_PACK1
 #endif

#elif ((OS_PLATFORM) == 0x00AEA101)					/* Atmel AV32A EVK1101 evaluation board			*/
  #define HOST_NAME			"Atmel EVK1101"			/* Name of the WebServer platform				*/
  #define SYS_FREQ	 		(12000000L)				/* Processor clock frequency					*/
  #define UART_DEV			1						/* RS-232 DB9 USART								*/

#elif ((OS_PLATFORM) == 0x00431611)					/* Olimex MSP430-P1611 board					*/
  #define HOST_NAME			"Olimex  MSP430-P1611"	/* Name of the WebServer platform				*/
  #define SYS_FREQ	 		(7372800L)				/* Processor clock frequency					*/
  #define UART_DEV			0						/* RS-232 DB9 UART								*/
  #define TIMER0_NAME		TIMERA0_VECTOR

#elif ((OS_PLATFORM) == 0x00435438)					/* Olimex MSP-5438STK board						*/
  #define HOST_NAME			"Olimex MSP-5438STK"	/* Name of the WebServer platform				*/
  #define SYS_FREQ	 		(9000000L)				/* Processor clock frequency					*/
  #define UART_DEV			2						/* RS-232 DB9 UART (USB UART is #3)				*/
  #define TIMER0_NAME		TIMER0_A0_VECTOR

#else
  #error "Unknown target board defined by OS_PLATFORM"

#endif

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* Common defintons shared amongst almost all demos													*/

#ifndef BAUDRATE
  #define BAUDRATE			115200					/* Serial port baud rate						*/
#endif

#ifndef OS_TICK_SRC_FREQ
  #define OS_TICK_SRC_FREQ	SYS_FREQ				/* Using the private or system timer			*/
#endif

#ifndef OS_TICK_PRESCL								/* In case not defined or no prec-scaler, 1		*/
  #define OS_TICK_PRESCL	1						/* is a perfect match							*/
#endif

#ifndef UART_PIN_PACK								/* Only STM devices use PIN packs for the UART	*/
  #define UART_PIN_PACK		0
#endif

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* Same function name on all platforms for the RTOS timer initialization.							*/

#if (((OS_PLATFORM) & 0x00FF0000) == 0x00430000)
  void TIMERinit(long Reload);
#else
  void TIMERinit(unsigned int Reload);
#endif

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
