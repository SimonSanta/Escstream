/* ------------------------------------------------------------------------------------------------ */

#include "mAbassi.h"
#include "Platform.h"                                /* Everything about the target processor is here*/
#include "HWinfo.h"                                    /* Everything about the tagret hardware is here    */
#include "SysCall.h"                                /* System Call layer stuff                        */

#include "arm_pl330.h"
#include "dw_i2c.h"
#include "cd_qspi.h"
#include "dw_sdmmc.h"
#include "dw_spi.h"
#include "dw_uart.h"
#include "alt_gpio.h"

/* ------------------------------------------------------------------------------------------------ */
/* App variables																					*/

int G_UartDevIn  = UART_DEV;						/* Needed by the system call layer if used		*/
int G_UartDevOut = UART_DEV;						/* Needed by the system call layer if used		*/
int G_UartDevErr = UART_DEV;						/* Needed by the system call layer if used		*/

/* ------------------------------------------------------------------------------------------------ */
/* App functions																					*/

extern void Task_HPS_Led(void);
extern void Task_FPGA_Led(void);
extern void Task_FPGA_Button(void);
extern void Task_MTL(void);
extern void Task_DisplayFile(void);

/* ------------------------------------------------------------------------------------------------ */

int main(void)
{
TSK_t *     Task;
uint32_t    ACPrd;
uint32_t    ACPwrt;

	setvbuf(stdin,  NULL, _IONBF, 0);				/* By default, Newlib library flush the I/O		*/
	setvbuf(stdout, NULL, _IONBF, 0);				/* buffer when full or when new-line			*/
	setvbuf(stderr, NULL, _IONBF, 0);

/* ------------------------------------------------ */
/* Start uAbassi									*/

	OSstart();										/* Start uAbassi								*/

	TSKsetCore(TSKmyID(), 0);						/* Make sure App_0 will always run on Core #0	*/

	SysCallInit();									/* Initialize the System Call layer				*/

	OSintOn();										/* Enable the interrupts						*/

/* ------------------------------------------------ */
/* SysTick Set-up									*/
													/* Timer tick needed for Sleep() & timeouts		*/
	OSisrInstall(OS_TICK_INT, &TIMtick);			/* Install the RTOS timer tick function			*/
	TIMERinit(((((OS_TICK_SRC_FREQ/OS_TICK_PRESCL)/100000)*(OX_TIMER_US))/(10)));
	GICenable(OS_TICK_INT, 128, 1);					/* Priority == mid / Edge trigger				*/

/* ------------------------------------------------ */
/* UART set-up										*/
/* 8 bits, No Parity, 1 Stop						*/

   #if (0U == ((UART_LIST_DEVICE) & (1U << (UART_DEV))))
	#error "Selected UART device is not in UART_LIST_DEVICE"
   #endif

	uart_init(UART_DEV, BAUDRATE, 8, 0, 10, UART_Q_SIZE_RX, UART_Q_SIZE_TX, UART_FILT_OUT_LF_CRLF);

   #if (((UART_LIST_DEVICE) & 0x01U) != 0U)			/* Install the UART interrupt handler			*/
	OSisrInstall(UART0_INT, &UARTintHndl_0);
	GICenable(-UART0_INT, 128, 0);					/* UART Must use level and not edge trigger		*/
   #endif											/* Int # -ve: the interrupt targets all cores	*/

   #if (((UART_LIST_DEVICE) & 0x02U) != 0U)			/* Install the UART interrupt handler			*/
	OSisrInstall(UART1_INT, &UARTintHndl_1);
	GICenable(-UART1_INT, 128, 0);					/* UART Must use level and not edge trigger		*/
   #endif											/* Int # -ve: the interrupt targets all cores	*/

   #if (((UART_LIST_DEVICE) & 0x04U) != 0U)			/* Install the UART interrupt handler			*/
	OSisrInstall(UART2_INT, &UARTintHndl_2);
	GICenable(-UART2_INT, 128, 0);					/* UART Must use level and not edge trigger		*/
   #endif											/* Int # -ve: the interrupt targets all cores	*/

   #if (((UART_LIST_DEVICE) & 0x08U) != 0U)			/* Install the UART interrupt handler			*/
	OSisrInstall(UART3_INT, &UARTintHndl_3);
	GICenable(-UART3_INT, 128, 0);					/* UART Must use level and not edge trigger		*/
   #endif											/* Int # -ve: the interrupt targets all cores	*/

/* ------------------------------------------------ */
/* DMA set-up                                       */
    
#if ((((OS_PLATFORM) & 0x00FFFFFF) == 0x0000AA10)                                                    \
    ||  (((OS_PLATFORM) & 0x00FFFFFF) == 0x0000AAC5))
    if (0 != dma_init(0)) {
        printf("ERROR : dma_init()\n");
    }
    
#ifdef DMA0_INT
    OSisrInstall(DMA0_INT, &DMAintHndl_0);
    GICenable(DMA0_INT, 128, 0);
#endif
    
#ifdef DMA1_INT
    OSisrInstall(DMA1_INT, &DMAintHndl_1);
    GICenable(DMA1_INT, 128, 0);
#endif
    
#ifdef DMA2_INT
    OSisrInstall(DMA2_INT, &DMAintHndl_2);
    GICenable(DMA2_INT, 128, 0);
#endif
    
#ifdef DMA3_INT
    OSisrInstall(DMA3_INT, &DMAintHndl_3);
    GICenable(DMA3_INT, 128, 0);
#endif
    
#ifdef DMA4_INT
    OSisrInstall(DMA4_INT, &DMAintHndl_4);
    GICenable(DMA4_INT, 128, 0);
#endif
    
#ifdef DMA5_INT
    OSisrInstall(DMA5_INT, &DMAintHndl_5);
    GICenable(DMA5_INT, 128, 0);
#endif
    
#ifdef DMA6_INT
    OSisrInstall(DMA6_INT, &DMAintHndl_6);
    GICenable(DMA6_INT, 128, 0);
#endif
    
#ifdef DMA7_INT
    OSisrInstall(DMA7_INT, &DMAintHndl_7);
    GICenable(DMA7_INT, 128, 0);
#endif
    
#ifdef DMA8_INT
    OSisrInstall(DMA8_INT, &DMAintHndl_8);
    GICenable(DMA8_INT, 128, 0);
#endif
    
#ifdef DMA9_INT
    OSisrInstall(DMA9_INT, &DMAintHndl_9);
    GICenable(DMA9_INT, 128, 0);
#endif
    
#ifdef DMA10_INT
    OSisrInstall(DMA10_INT, &DMAintHndl_10);
    GICenable(DMA10_INT, 128, 0);
#endif
    
#ifdef DMA11_INT
    OSisrInstall(DMA11_INT, &DMAintHndl_11);
    GICenable(DMA11_INT, 128, 0);
#endif
    
#ifdef DMA12_INT
    OSisrInstall(DMA12_INT, &DMAintHndl_12);
    GICenable(DMA12_INT, 128, 0);
#endif
    
#ifdef DMA13_INT
    OSisrInstall(DMA13_INT, &DMAintHndl_13);
    GICenable(DMA13_INT, 128, 0);
#endif
    
#ifdef DMA14_INT
    OSisrInstall(DMA14_INT, &DMAintHndl_14);
    GICenable(DMA14_INT, 128, 0);
#endif
    
#ifdef DMA15_INT
    OSisrInstall(DMA15_INT, &DMAintHndl_15);
    GICenable(DMA15_INT, 128, 0);
#endif
#endif
    
/* ------------------------------------------------ */
/* SD/MMC interrupt handler                         */
    
#if (defined(DEMO_USE_SDMMC) && ((DEMO_USE_SDMMC) != 0) && defined(SDMMC_DEV))
#if (0U == ((SDMMC_LIST_DEVICE) & (1U << (SDMMC_DEV))))
#error "Selected SDMMC device is not in SDMMC_LIST_DEVICE"
#endif
    
#if (((SDMMC_LIST_DEVICE) & 0x01U) != 0U)            /* Install the SDMMC interrupt handler            */
    OSisrInstall(SDMMC0_INT, MMCintHndl_0);            /* The SD/MMC driver, mmc_init(), is not called    */
    GICenable(SDMMC0_INT, 128, 0);                    /* because the FS deals with the initialization    */
#endif
    
#if (((SDMMC_LIST_DEVICE) & 0x02U) != 0U)            /* Install the SDMMC interrupt handler            */
    OSisrInstall(SDMMC1_INT, MMCintHndl_1);            /* The SD/MMC driver, mmc_init(), is not called    */
    GICenable(SDMMC1_INT, 128, 0);                    /* because the FS deals with the initialization    */
#endif
    
#if (((SDMMC_LIST_DEVICE) & 0x04U) != 0U)            /* Install the SDMMC interrupt handler            */
    OSisrInstall(SDMMC2_INT, MMCintHndl_2);            /* The SD/MMC driver, mmc_init(), is not called    */
    GICenable(SDMMC2_INT, 128, 0);                    /* because the FS deals with the initialization    */
#endif
    
#if (((SDMMC_LIST_DEVICE) & 0x08U) != 0U)            /* Install the SDMMC interrupt handler            */
    OSisrInstall(SDMMC3_INT, MMCintHndl_3);            /* The SD/MMC driver, mmc_init(), is not called    */
    GICenable(SDMMC3_INT, 128, 0);                    /* because the FS deals with the initialization    */
#endif
#endif

/* ------------------------------------------------ */
/* ACP set-up                                       */
    
    // It is not necessary to use it !
    
    // ACPwrt = acp_enable(-1, 0, 0, 0);               /* Page 0 (0x00000000->0x3FFFFFFF) is set-up   */
    // ACPrd  = acp_enable(-1, 0, 0, 1);               /* to use ACP for both read & write            */
    
/* ------------------------------------------------ */
/* Application set-up								*/
    
	Task = TSKcreate("App FPGA Led", 3, 8192, &Task_FPGA_Led, 0);
	TSKsetCore(Task, 1);							/* Create new task, will always run on core #1	*/
	TSKresume(Task);								/* when BMP (G_OS_MP_TYPE == 4 or 5)			*/

	Task = TSKcreate("App FPGA Button", 3, 8192, &Task_FPGA_Button, 0);
	TSKsetCore(Task, 1);							/* Create new task, will always run on core #1	*/
	TSKresume(Task);								/* when BMP (G_OS_MP_TYPE == 4 or 5)			*/
    
    Task = TSKcreate("App MTL", 2, 16384, &Task_MTL, 0);
    TSKsetCore(Task, 1);
    TSKresume(Task);
 
    Task = TSKcreate("App Display File", 4, 8192, &Task_DisplayFile, 0);
    TSKresume(Task);
    
#if defined(USE_SHELL)
    TSKcreate("Shell", OX_PRIO_MIN, 16384, OSshell, 1);
    TSKsleep(OS_MS_TO_TICK(100));                   /* Make sure Shell Task initialization if over  */
#endif

	TSKsetCore(TSKmyID(), 0);						/* Make sure Task_HPS_Led always run on Core #0	*/

	Task_HPS_Led();									/* Go into core #0 Task_HPS_Led code			*/

	for(;;);										/* Should never come back here					*/
}

/* ------------------------------------------------------------------------------------------------ */
/* Not used but needed to build uAbassi																*/

void TIMcallBack(void)
{
	return;
}

/* EOF */
