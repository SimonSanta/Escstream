/*-----------------------------------------------------------
 *
 * MyApp - mAbassi
 *
 *-----------------------------------------------------------*/

#include "../inc/MyApp_MTL.h"

#include "mAbassi.h"          /* MUST include "SAL.H" and not uAbassi.h        */
#include "Platform.h"         /* Everything about the target platform is here  */
#include "HWinfo.h"           /* Everything about the target hardware is here  */
#include "SysCall.h"          /* System Call layer stuff                        */

#include "arm_pl330.h"
#include "dw_i2c.h"
#include "cd_qspi.h"
#include "dw_sdmmc.h"
#include "dw_spi.h"
#include "dw_uart.h"
#include "alt_gpio.h"

#include "gui.h"
#include "../painter/terasic_lib/multi_touch.h"

MTC_INFO *myTouch;
VIP_FRAME_READER *myReader;
uint32_t  *myFrameBuffer;

#define MTC_REG_INT_ACK      1     // write only (write any value to ack)

/*-----------------------------------------------------------*/

void Task_MTL(void)
{
    MTX_t    *PrtMtx;
    PrtMtx = MTXopen("Printf Mtx");
    
    myFrameBuffer = 0x20000000;
    
    TSKsleep(OS_MS_TO_TICK(500));
    
    MTXlock(PrtMtx, -1);
    printf("Starting MTL initialization\n");

    myTouch = MTC_Init(fpga_i2c, fpga_mtc, LCD_TOUCH_INT_IRQ);
    
    // Enable IRQ for SPI & MTL
    
    OSisrInstall(GPT_SPI_IRQ, (void *) &spi_CallbackInterrupt);
    GICenable(GPT_SPI_IRQ, 128, 1);
    OSisrInstall(GPT_MTC_IRQ, (void *) &mtc_CallbackInterrupt);
    GICenable(GPT_MTC_IRQ, 128, 1);
    
    // Enable interruptmask and edgecapture of PIO core for mtc2
    alt_write_word(PIOinterruptmask_fpga_MTL, 0x3);
    alt_write_word(PIOedgecapture_fpga_MTL, 0x3);
    
    // Mount drive 0 to a mount point
    if (0 != mount(FS_TYPE_NAME_AUTO, "/", 0, "0")) {
        printf("ERROR: cannot mount volume 0\n");
    }
    // List the current directory contents
    cmd_ls();
    
    printf("MTL initialization completed\n");
    MTXunlock(PrtMtx);
    
    for( ;; )
    {
        GUI(myTouch);
        TSKsleep(OS_MS_TO_TICK(5));
    }
}

/*-----------------------------------------------------------*/

/* Align on cache lines if cached transfers */
static char g_Buffer[9600] __attribute__ ((aligned (OX_CACHE_LSIZE)));

void Task_DisplayFile(void)
{
    SEM_t    *PtrSem;
    int       FdSrc;
    int       Nrd;
    uint32_t  pixel;
    int       i;
    
    static const char theFileName[] = "MTL_Image.dat";
    
    PtrSem = SEMopen("MySem_DisplayFile");
    
    for( ;; )
    {
        SEMwait(PtrSem, -1);    // -1 = Infinite blocking
        SEMreset(PtrSem);
        
        FdSrc = open(theFileName, O_RDONLY, 0777);
        if (FdSrc >= 0) {
            myFrameBuffer = 0x20000000;
            do {
                Nrd = read(FdSrc, &g_Buffer[0], sizeof(g_Buffer));
                i=0;
                while(i < Nrd) {
                    *myFrameBuffer++ = (g_Buffer[i] << 16) + (g_Buffer[i+1] << 8) + g_Buffer[i+2];
                    i += 3;
                }
            } while (Nrd >= sizeof(g_Buffer));
            close(FdSrc);
            myFrameBuffer = 0x20000000;
        }
    }
}

/*-----------------------------------------------------------*/

void spi_CallbackInterrupt (uint32_t icciar, void *context)
{
    uint32_t status = alt_read_word(SPI_STATUS);
    uint32_t rxdata = alt_read_word(SPI_RXDATA);
    
    // Do something
    // printf("INFO: IRQ from SPI : %x (status = %x)\r\n", rxdata, status);
    alt_write_word(SPI_TXDATA, 0x113377FF);
    
    *myFrameBuffer++ = rxdata;
    if (myFrameBuffer >= 0x20000000 + (800 * 480 * 4))
    myFrameBuffer = 0x20000000;
    
    // Clear the status of SPI core
    alt_write_word(SPI_STATUS, 0x00);
}

/*-----------------------------------------------------------*/

void mtc_CallbackInterrupt (uint32_t icciar, void *context)
{
    //printf("INFO: IRQ from MTC\n");
    
    mtc_QueryData(myTouch);
    
    // Clear the interrupt (in i2c core)
    alt_write_word(fpga_i2c + (MTC_REG_INT_ACK*4), 0x0);
    // Clear the interruptmask of PIO core
    alt_write_word(PIOinterruptmask_fpga_MTL, 0x0);

    // Enable the interruptmask and edge register of PIO core for new interrupt
    alt_write_word(PIOinterruptmask_fpga_MTL, 0x1);
    alt_write_word(PIOedgecapture_fpga_MTL, 0x1);
}

/*-----------------------------------------------------------*/


void Task_HPS_Led(void)
{
    MTX_t    *PrtMtx;       // Mutex for exclusive access to printf()
    MBX_t    *PrtMbx;
    intptr_t *PtrMsg;
    
    setup_hps_gpio();   // This is the Adam&Eve Task and we have first to setup the gpio
    button_ConfigureInterrupt();
    
    PrtMtx = MTXopen("Printf Mtx");
    MTXlock(PrtMtx, -1);
    printf("\n\nDE10-Nano - MyApp_MTL\n\n");
    printf("Task_HPS_Led running on core #%d\n\n", COREgetID());
    MTXunlock(PrtMtx);
    
    PrtMbx = MBXopen("MyMailbox", 128);

	for( ;; )
	{
        if (MBXget(PrtMbx, PtrMsg, 0) == 0) {  // 0 = Never blocks
            MTXlock(PrtMtx, -1);
            printf("Receive message (Core = %d)\n", COREgetID());
            MTXunlock(PrtMtx);
        }
        toogle_hps_led();
        
        TSKsleep(OS_MS_TO_TICK(500));
	}
}

/*-----------------------------------------------------------*/

void Task_FPGA_Led(void)
{
    uint32_t leds_mask;
    
    alt_write_word(fpga_leds, 0x01);

	for( ;; )
	{
        leds_mask = alt_read_word(fpga_leds);
        if (leds_mask != (0x01 << (LED_PIO_DATA_WIDTH - 1))) {
            // rotate leds
            leds_mask <<= 1;
        } else {
            // reset leds
            leds_mask = 0x1;
        }
        alt_write_word(fpga_leds, leds_mask);
        
        TSKsleep(OS_MS_TO_TICK(250));
	}
}
/*-----------------------------------------------------------*/

void Task_FPGA_Button(void)
{
    MTX_t    *PrtMtx;       // Mutex for exclusive access to printf()
    MBX_t    *PrtMbx;
    intptr_t  PtrMsg;
    SEM_t    *PtrSem;
    
    PrtMtx = MTXopen("Printf Mtx");
    PrtMbx = MBXopen("MyMailbox", 128);
    PtrSem = SEMopen("MySemaphore");
    
    for( ;; )
    {
        SEMwait(PtrSem, -1);    // -1 = Infinite blocking
        SEMreset(PtrSem);
        MTXlock(PrtMtx, -1);
        printf("Receive IRQ from Button and send message (Core = %d)\n", COREgetID());
        MTXunlock(PrtMtx);
        
        MBXput(PrtMbx, PtrMsg, -1); // -1 = Infinite blocking
    }
}

/*-----------------------------------------------------------*/

void button_CallbackInterrupt (uint32_t icciar, void *context)
{
    SEM_t    *PtrSem;
    uint32_t button;
    
    button = alt_read_word(PIOdata_fpga_button);
    
    // Clear the interruptmask of PIO core
    alt_write_word(PIOinterruptmask_fpga_button, 0x0);
    
    // Enable the interruptmask and edge register of PIO core for new interrupt
    alt_write_word(PIOinterruptmask_fpga_button, 0x3);
    alt_write_word(PIOedgecapture_fpga_button, 0x3);
    
    PtrSem = SEMopen("MySemaphore");
    SEMpost(PtrSem);
    
    if (button == 1) {
        PtrSem = SEMopen("MySem_DisplayFile");
        SEMpost(PtrSem);
    }
}

/*-----------------------------------------------------------*/

void button_ConfigureInterrupt( void )
{
    OSisrInstall(GPT_BUTTON_IRQ, (void *) &button_CallbackInterrupt);
    GICenable(GPT_BUTTON_IRQ, 128, 1);
    
    // Enable interruptmask and edgecapture of PIO core for buttons 0 and 1
    alt_write_word(PIOinterruptmask_fpga_button, 0x3);
    alt_write_word(PIOedgecapture_fpga_button, 0x3);
}

/*-----------------------------------------------------------*/

void setup_hps_gpio()
{
    uint32_t hps_gpio_config_len = 2;
    ALT_GPIO_CONFIG_RECORD_t hps_gpio_config[] = {
        {HPS_LED_IDX  , ALT_GPIO_PIN_OUTPUT, 0, 0, ALT_GPIO_PIN_DEBOUNCE, ALT_GPIO_PIN_DATAZERO},
        {HPS_KEY_N_IDX, ALT_GPIO_PIN_INPUT , 0, 0, ALT_GPIO_PIN_DEBOUNCE, ALT_GPIO_PIN_DATAZERO}
    };
    
    assert(ALT_E_SUCCESS == alt_gpio_init());
    assert(ALT_E_SUCCESS == alt_gpio_group_config(hps_gpio_config, hps_gpio_config_len));
}

/*-----------------------------------------------------------*/

void toogle_hps_led()
{
    uint32_t hps_led_value = alt_read_word(ALT_GPIO1_SWPORTA_DR_ADDR);
    hps_led_value >>= HPS_LED_PORT_BIT;
    hps_led_value = !hps_led_value;
    hps_led_value <<= HPS_LED_PORT_BIT;
    alt_gpio_port_data_write(HPS_LED_PORT, HPS_LED_MASK, hps_led_value);
}

/*-----------------------------------------------------------*/

void cmd_ls()
{
    DIR_t         *Dinfo;
    struct dirent *DirFile;
    struct stat    Finfo;
    char           Fname[SYS_CALL_MAX_PATH+1];
    char           MyDir[SYS_CALL_MAX_PATH+1];
    struct tm     *Time;
    
    /* Refresh the current directory path            */
    if (NULL == getcwd(&MyDir[0], sizeof(MyDir))) {
        printf("ERROR: cannot obtain current directory\n");
        return;
    }
    
    /* Open the dir to see if it's there            */
    if (NULL == (Dinfo = opendir(&MyDir[0]))) {
        printf("ERROR: cannot open directory\n");
        return;
    }
    
    /* Valid directory, read each entries and print    */
    while(NULL != (DirFile = readdir(Dinfo))) {
        strcpy(&Fname[0], &MyDir[0]);
        strcat(&Fname[0], "/");
        strcat(&Fname[0], &(DirFile->d_name[0]));
        
        stat(&Fname[0], &Finfo);
        putchar(((Finfo.st_mode & S_IFMT) == S_IFLNK) ? 'l' :
                ((Finfo.st_mode & S_IFMT) == S_IFDIR) ? 'd':' ');
        putchar((Finfo.st_mode & S_IRUSR) ? 'r' : '-');
        putchar((Finfo.st_mode & S_IWUSR) ? 'w' : '-');
        putchar((Finfo.st_mode & S_IXUSR) ? 'x' : '-');
        
        if ((Finfo.st_mode & S_IFLNK) == S_IFLNK) {
            printf(" (%-9s mnt point)           - ", media_info(DirFile->d_drv));
        }
        else {
            Time = localtime(&Finfo.st_mtime);
            if (Time != NULL) {
                printf(" (%04d.%02d.%02d %02d:%02d:%02d) ", Time->tm_year + 1900,
                       Time->tm_mon,
                       Time->tm_mday,
                       Time->tm_hour,
                       Time->tm_min,
                       Time->tm_sec);
            }
            printf(" %10lu ", Finfo.st_size);
        }
        puts(DirFile->d_name);
    }
    closedir(Dinfo);
}

/*-----------------------------------------------------------*/

