/* ------------------------------------------------------------------------------------------------ */
/* FILE :		HWinfo.h																			*/
/*																									*/
/* CONTENTS :																						*/
/*				Definitions of the HW peripherals on all supported boards.							*/
/*				Refer to Platform.txt for a description on why and how this file is use				*/
/*				For the definitions related to the supported chips, refer to Platform.h				*/
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
/*	$Revision: 1.28 $																				*/
/*	$Date: 2019/01/10 18:07:07 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#ifndef __HWINFO_H__
#define __HWINFO_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------ */

#if ((OS_PLATFORM) == 0x0000AAC5)					/* Cyclone V SocFPGA dev board					*/

  #define I2C_TEST_AVAIL		1					/* If the I2C square wave test available		*/
  #define I2C_USE_LCD			1					/* Boolean if LCD is available or not			*/
  #define I2C_LCD_DEVICE		0					/* Which I2C controller is used by the LCD		*/
  #define I2C_LCD_ADDR			(0x28)				/* Address of the I2C device					*/
  #define I2C_LCD_WIDTH			7					/* 7 or 10 bit addresses						*/
  #define I2C_LCD_SPEED			40000				/* This LCD is below standard speed				*/
  #define I2C_LCD_TYPE			2163				/* LCD is NewHeaven Display NHD-0216K3Z			*/

  #define I2C_USE_RTC			1					/* Boolean if RTC is available or not			*/
  #define I2C_RTC_DEVICE		0					/* Which I2C controller is used by the LCD		*/
  #define I2C_RTC_ADDR			(0x68)				/* Address of the I2C device					*/
  #define I2C_RTC_WIDTH			7					/* 7 or 10 bit addresses						*/
  #define I2C_RTC_SPEED			400000				/* Supports maximum rate						*/
  #define I2C_RTC_TYPE			1339				/* Maxim DS1339 is the RTC chip					*/

 #if 1												/* SET TO != 0 TO USE LT DEMO PADDLE BOARD MEM	*/
  #define I2C_USE_EEPROM		1					/* Most common EEPROMs on LT demo boards		*/
  #define I2C_EEPROM_DEVICE		0					/* Microchip M24LC025							*/
  #define I2C_EEPROM_ADDR		(0x50)				/* Address of the I2C device					*/
  #define I2C_EEPROM_WIDTH		7					/* 7 or 10 bit addresses						*/
  #define I2C_EEPROM_SPEED		400000				/* Supports maximum rate						*/
  #define I2C_EEPROM_TYPE		24025
 #else
  #define I2C_USE_EEPROM		1					/* On-board EEPROM								*/
  #define I2C_EEPROM_DEVICE		0					/* Microchip M24LC32							*/
  #define I2C_EEPROM_ADDR		(0x51)				/* Address of the I2C device					*/
  #define I2C_EEPROM_WIDTH		7					/* 7 or 10 bit addresses						*/
  #define I2C_EEPROM_SPEED		100000				/* 3.3V is not spec'ed for 400000 Hz			*/
  #define I2C_EEPROM_TYPE		2432
 #endif
													/* ---------------------------------------------*/
  #define MY_BUTTON_0			82					/* HPS_HLGPI11									*/
  #define MY_BUTTON_1			81					/* HPS_HLGPI10									*/
  #define MY_BUTTON_2			80					/* HPS_HLGPI9									*/
  #define MY_BUTTON_3			79					/* HPS_HLGPI8									*/
  #define BUT_GET(Button)		(0 != gpio_get(Button))
  #define MY_BUTTON_INIT()		do {																\
									gpio_init(0);													\
									gpio_dir(MY_BUTTON_0, 1);										\
									gpio_dir(MY_BUTTON_1, 1);										\
									gpio_dir(MY_BUTTON_2, 1);										\
									gpio_dir(MY_BUTTON_3, 1);										\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LCD_INIT()			MY_LCD_CLEAR()
  #define MY_LCD_NMB_LINE		2
  #define MY_LCD_WIDTH			16
  #define MY_LCD_CLEAR()		do {																\
									MY_LCD_CLRLINE(0);												\
									MY_LCD_CLRLINE(1);												\
								} while(0)
  #define MY_LCD_CLRLINE(Line)	MY_LCD_LINE(Line, "                    ")
  #define MY_LCD_LINE(x, txt)	do {																\
									if (x == 0) {													\
										LCDputs(txt, NULL);											\
									}																\
									else {															\
										LCDputs(NULL, txt);											\
									}																\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LED_0				44					/* HPS_GPIO44									*/
  #define MY_LED_1				43					/* HPS_GPIO43									*/
  #define MY_LED_2				42					/* HPS_GPIO42									*/
  #define MY_LED_3				41					/* HPS_GPIO41									*/
  #define MY_LED_FLASH			MY_LED_3
  #define MY_LED_FLASH_S		"3"
  #define MY_LED_OFF(LEDn)		gpio_set(LEDn, 1)
  #define MY_LED_ON(LEDn)		gpio_set(LEDn, 0)
  #define MY_LED_INIT()			do {																\
									gpio_init(0);													\
									gpio_dir(MY_LED_0, 0);											\
									gpio_dir(MY_LED_1, 0);											\
									gpio_dir(MY_LED_2, 0);											\
									gpio_dir(MY_LED_3, 0);											\
									MY_LED_OFF(MY_LED_0);											\
									MY_LED_OFF(MY_LED_1);											\
									MY_LED_OFF(MY_LED_2);											\
									MY_LED_OFF(MY_LED_3);											\
								} while(0)
													/* ---------------------------------------------*/
  extern int gpio_dir  (int IO, int Dir);
  extern int gpio_get  (int IO);					/* Duplicate the function prototypes to not		*/
  extern int gpio_init (int Forced);				/* have a depedency on #include "alt_gpio,h"	*/
  extern int gpio_set  (int IO, int Value);			/* If wrong, the compiler will issue a warning	*/

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x4000AAC5)					/* Arria 5 SocFPGA dev board					*/

  #define I2C_TEST_AVAIL		1					/* If the I2C square wave test available		*/
  #define I2C_USE_LCD			1					/* Boolean if LCD is available or not			*/
  #define I2C_LCD_DEVICE		0					/* Which I2C controller is used by the LCD		*/
  #define I2C_LCD_ADDR			(0x28)				/* Address of the I2C device					*/
  #define I2C_LCD_WIDTH			7					/* 7 or 10 bit addresses						*/
  #define I2C_LCD_SPEED			40000				/* This LCD is below standard speed				*/
  #define I2C_LCD_TYPE			2163				/* LCD is NewHeaven Display NHD-0216K3Z			*/

  #define I2C_USE_RTC			1					/* Boolean if RTC is available or not			*/
  #define I2C_RTC_DEVICE		0					/* Which I2C controller is used by the LCD		*/
  #define I2C_RTC_ADDR			(0x68)				/* Address of the I2C device					*/
  #define I2C_RTC_WIDTH			7					/* 7 or 10 bit addresses						*/
  #define I2C_RTC_SPEED			400000				/* Supports maximum rate						*/
  #define I2C_RTC_TYPE			1339				/* Maxim DS1339 is the RTC chip					*/

  #define I2C_USE_EEPROM		1					/* On-board EEPROM								*/
  #define I2C_EEPROM_DEVICE		0					/* Microchip M24LC32							*/
  #define I2C_EEPROM_ADDR		(0x51)				/* Address of the I2C device					*/
  #define I2C_EEPROM_WIDTH		7					/* 7 or 10 bit addresses						*/
  #define I2C_EEPROM_SPEED		100000				/* 3.3V is not spec'ed for 400000 Hz			*/
  #define I2C_EEPROM_TYPE		2432
													/* ---------------------------------------------*/
  #define MY_BUTTON_0			35					/* HPS_GPIO35									*/
  #define MY_BUTTON_1			41					/* HPS_GPIO41									*/
  #define MY_BUTTON_2			42					/* HPS_GPIO42									*/
  #define MY_BUTTON_3			43					/* HPS_GPIO43									*/
  #define BUT_GET(Button)		(0 != gpio_get(Button))
  #define MY_BUTTON_INIT()		do {																\
									gpio_init(0);													\
									gpio_dir(MY_BUTTON_0, 1);										\
									gpio_dir(MY_BUTTON_1, 1);										\
									gpio_dir(MY_BUTTON_2, 1);										\
									gpio_dir(MY_BUTTON_3, 1);										\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LCD_INIT()			MY_LCD_CLEAR()
  #define MY_LCD_NMB_LINE		2
  #define MY_LCD_WIDTH			16
  #define MY_LCD_CLEAR()		do {																\
									MY_LCD_CLRLINE(0);												\
									MY_LCD_CLRLINE(1);												\
								} while(0)
  #define MY_LCD_CLRLINE(Line)	MY_LCD_LINE(Line, "                    ")
  #define MY_LCD_LINE(x, txt)	do {																\
									if (x == 0) {													\
										LCDputs(txt, NULL);											\
									}																\
									else {															\
										LCDputs(NULL, txt);											\
									}																\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LED_0				 0					/* HPS_GPIO0									*/
  #define MY_LED_1				40					/* HPS_GPIO40									*/
  #define MY_LED_2				17					/* HPS_GPIO17									*/
  #define MY_LED_3				18					/* HPS_GPIO18									*/
  #define MY_LED_FLASH			MY_LED_3
  #define MY_LED_FLASH_S		"3"
  #define MY_LED_OFF(LEDn)		gpio_set(LEDn, 1)
  #define MY_LED_ON(LEDn)		gpio_set(LEDn, 0)
  #define MY_LED_INIT()			do {																\
									gpio_init(0);													\
									gpio_dir(MY_LED_0, 0);											\
									gpio_dir(MY_LED_1, 0);											\
									gpio_dir(MY_LED_2, 0);											\
									gpio_dir(MY_LED_3, 0);											\
									MY_LED_OFF(MY_LED_0);											\
									MY_LED_OFF(MY_LED_1);											\
									MY_LED_OFF(MY_LED_2);											\
									MY_LED_OFF(MY_LED_3);											\
								} while(0)
													/* ---------------------------------------------*/
  extern int gpio_dir  (int IO, int Dir);
  extern int gpio_get  (int IO);					/* Duplicate the function prototypes to not		*/
  extern int gpio_init (int Forced);				/* have a depedency on #include "alt_gpio,h"	*/
  extern int gpio_set  (int IO, int Value);			/* If wrong, the compiler will issue a warning	*/

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x0100AAC5)					/* DE0-nano / DE10-nano							*/

  #define I2C_TEST_AVAIL		1					/* If the I2C square wave test available		*/
  #define I2C_USE_XL345			1					/* Boolean if Accelerometer is available or not	*/
  #define I2C_XL345_DEVICE		0					/* Which I2C controller is used by the Acc		*/
  #define I2C_XL345_ADDR		0x53				/* Address of the I2C device					*/
  #define I2C_XL345_SPEED		400000				/* Maximum data rate							*/
  #define I2C_XL345_WIDTH		7					/* 7 or 10 bit addresses						*/
  #define I2C_XL345_INT_IO		61					/* GPIO when the ADXL345 int is connected to	*/
													/* Using 61 instead of ALT_GPIO_1BIT_61			*/
 #if 1												/* SET TO != 0 TO USE LT DEMO PADDLE BOARD MEM	*/
  #define I2C_USE_EEPROM		1					/* Most common EEPROMs on LT demo boards		*/
  #define I2C_EEPROM_DEVICE		1					/* Microchip M24LC025							*/
  #define I2C_EEPROM_ADDR		(0x50)				/* Address of the I2C device					*/
  #define I2C_EEPROM_WIDTH		7					/* 7 or 10 bit addresses						*/
  #define I2C_EEPROM_SPEED		400000				/* Supports maximum rate						*/
  #define I2C_EEPROM_TYPE		24025
 #else
  #define I2C_USE_EEPROM		1					/* On-board EEPROM								*/
  #define I2C_EEPROM_DEVICE		1					/* Microchip M24LC32							*/
  #define I2C_EEPROM_ADDR		(0x51)				/* Address of the I2C device					*/
  #define I2C_EEPROM_WIDTH		7					/* 7 or 10 bit addresses						*/
  #define I2C_EEPROM_SPEED		100000				/* 3.3V is not spec'ed for 400000 Hz			*/
  #define I2C_EEPROM_TYPE		2432
 #endif
													/* ---------------------------------------------*/
  #define MY_BUTTON_0			54					/* HPS_GPIO54									*/
  #define BUT_GET(Button)		(0 != gpio_get(Button))
  #define MY_BUTTON_INIT()		do {																\
									gpio_init(0);													\
									gpio_dir(MY_BUTTON_0, 1);										\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LED_0				53					/* No MY_LED_FLASH as sole LED used by DHCP		*/
  #define MY_LED_INIT()			do {																\
									gpio_init(0);													\
									gpio_dir(MY_LED_0, 0);											\
									MY_LED_OFF(MY_LED_0);											\
								} while(0)
  #define MY_LED_OFF(LEDn)		gpio_set(LEDn, 0)
  #define MY_LED_ON(LEDn)		gpio_set(LEDn, 1)
													/* ---------------------------------------------*/
  #define DE0_SELECT_LT_I2C()	do { gpio_init(0);													\
								     gpio_dir(40, 0);												\
								     gpio_set(40, 1);												\
								} while(0)
  #define DE0_SELECT_LT_SPI()	do { gpio_init(0);													\
								     gpio_dir(40, 0);												\
								     gpio_set(40, 0);												\
								} while(0)
													/* ---------------------------------------------*/
  extern int gpio_dir  (int IO, int Dir);
  extern int gpio_get  (int IO);					/* Duplicate the function prototypes to not		*/
  extern int gpio_init (int Forced);				/* have a depedency on #include "alt_gpio,h"	*/
  extern int gpio_set  (int IO, int Value);			/* If wrong, the compiler will issue a warning	*/

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x0200AAC5)					/* EBV Socrates board (Cyclone V)				*/

  #define I2C_TEST_AVAIL		1					/* If the I2C square wave test available		*/
  #define I2C_USE_RTC			1					/* Boolean if RTC is available or not			*/
  #define I2C_RTC_DEVICE		0					/* Which I2C controller is used by the LCD		*/
  #define I2C_RTC_ADDR			(0x68)				/* Address of the I2C device					*/
  #define I2C_RTC_WIDTH			7					/* 7 or 10 bit addresses						*/
  #define I2C_RTC_SPEED			400000				/* Device maximum rate							*/
  #define I2C_RTC_TYPE			4182				/* ST M41T82 is the RTC chip					*/
													/* ---------------------------------------------*/
  #define MY_BUTTON_0			77					/* Joystick										*/
  #define MY_BUTTON_1			78					/* HPS_HLGPI7									*/
  #define MY_BUTTON_2			82					/* HPS_HLGPI11									*/
  #define MY_BUTTON_3			81					/* HPS_HLGPI10									*/
  #define MY_BUTTON_4			73					/* HPS_HLGPI2									*/
  #define BUT_GET(Button)		(0 == gpio_get(Button))
  #define MY_BUTTON_INIT()		do {																\
									gpio_init(0);													\
									gpio_dir(MY_BUTTON_0, 1);										\
									gpio_dir(MY_BUTTON_1, 1);										\
									gpio_dir(MY_BUTTON_2, 1);										\
									gpio_dir(MY_BUTTON_3, 1);										\
									gpio_dir(MY_BUTTON_4, 1);										\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LED_0				28					/* HPS_GPIO28									*/
  #define MY_LED_1				48					/* HPS_GPIO48									*/
  #define MY_LED_2				54					/* HPS_GPIO54									*/
  #define MY_LED_FLASH			MY_LED_2
  #define MY_LED_FLASH_S		"2"
  #define MY_LED_OFF(LEDn)		gpio_set(LEDn, 1)
  #define MY_LED_ON(LEDn)		gpio_set(LEDn, 0)
  #define MY_LED_INIT()			do {																\
									gpio_init(0);													\
									gpio_dir(MY_LED_0, 0);											\
									gpio_dir(MY_LED_1, 0);											\
									gpio_dir(MY_LED_2, 0);											\
									MY_LED_OFF(MY_LED_0);											\
									MY_LED_OFF(MY_LED_1);											\
									MY_LED_OFF(MY_LED_2);											\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_SWITCH_0			71					/* P3 DIP switches								*/
  #define MY_SWITCH_1			75					/* HPS_HLGPI4									*/
  #define MY_SWITCH_2			79					/* HPS_HLGPI8									*/
  #define MY_SWITCH_3			84					/* HPS_HLGPI14									*/
  #define MY_SWITCH_4			76					/* HPS_HLGPI5									*/
  #define MY_SWITCH_5			74					/* HPS_HLGPI3									*/
  #define MY_SWITCH_6			80					/* HPS_HLGPI9									*/
  #define MY_SWITCH_7			83					/* HPS_HLGPI12									*/
  #define MY_SWITCH_INIT()		do {																\
									gpio_init(0);													\
									gpio_dir(MY_SWITCH_0, 1);										\
									gpio_dir(MY_SWITCH_1, 1);										\
									gpio_dir(MY_SWITCH_2, 1);										\
									gpio_dir(MY_SWITCH_3, 1);										\
									gpio_dir(MY_SWITCH_4, 1);										\
									gpio_dir(MY_SWITCH_5, 1);										\
									gpio_dir(MY_SWITCH_6, 1);										\
									gpio_dir(MY_SWITCH_7, 1);										\
								} while(0)
  #define SW_GET(Switch)		(0 == gpio_get(Switch))
													/* ---------------------------------------------*/
  extern int gpio_dir  (int IO, int Dir);
  extern int gpio_get  (int IO);					/* Duplicate the function prototypes to not		*/
  extern int gpio_init (int Forced);				/* have a depedency on #include "alt_gpio,h"	*/
  extern int gpio_set  (int IO, int Value);			/* If wrong, the compiler will issue a warning	*/

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x0300AAC5)					/* SOCkit board (Cyclone V)						*/

  #define MY_BUTTON_0			8					/* HPS_GPIO8									*/
  #define MY_BUTTON_1			9					/* HPS_GPIO9									*/
  #define MY_BUTTON_2			10					/* HPS_GPIO10									*/
  #define MY_BUTTON_3			11					/* HPS_GPIO11									*/
  #define BUT_GET(Button)		(0 != gpio_get(Button))
  #define MY_BUTTON_INIT()		do {																\
									gpio_init(0);													\
									gpio_dir(MY_BUTTON_0, 1);										\
									gpio_dir(MY_BUTTON_1, 1);										\
									gpio_dir(MY_BUTTON_2, 1);										\
									gpio_dir(MY_BUTTON_3, 1);										\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LED_0				53					/* HPS_GPIO53									*/
  #define MY_LED_1				54					/* HPS_GPIO54									*/
  #define MY_LED_2				55					/* HPS_GPIO55									*/
  #define MY_LED_3				56					/* HPS_GPIO56									*/
  #define MY_LED_FLASH			MY_LED_3
  #define MY_LED_FLASH_S		"3"
  #define MY_LED_OFF(LEDn)		gpio_set(LEDn, 1)
  #define MY_LED_ON(LEDn)		gpio_set(LEDn, 0)
  #define MY_LED_INIT()			do {																\
									gpio_init(0);													\
									gpio_dir(MY_LED_0, 0);											\
									gpio_dir(MY_LED_1, 0);											\
									gpio_dir(MY_LED_2, 0);											\
									gpio_dir(MY_LED_3, 0);											\
									MY_LED_OFF(MY_LED_0);											\
									MY_LED_OFF(MY_LED_1);											\
									MY_LED_OFF(MY_LED_2);											\
									MY_LED_OFF(MY_LED_3);											\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_SWITCH_0			7					/* HPS_GPIO7									*/
  #define MY_SWITCH_1			6					/* HPS_GPIO6									*/
  #define MY_SWITCH_2			5					/* HPS_GPIO5									*/
  #define MY_SWITCH_3			4					/* HPS_GPIO4									*/
  #define SW_GET(Switch)		(0 != gpio_get(Switch))
  #define MY_SWITCH_INIT()		do {																\
									gpio_init(0);													\
									gpio_dir(MY_SWITCH_0, 1);										\
									gpio_dir(MY_SWITCH_1, 1);										\
									gpio_dir(MY_SWITCH_2, 1);										\
									gpio_dir(MY_SWITCH_3, 1);										\
								} while(0)
													/* ---------------------------------------------*/
  extern int gpio_dir  (int IO, int Dir);
  extern int gpio_get  (int IO);					/* Duplicate the function prototypes to not		*/
  extern int gpio_init (int Forced);				/* have a depedency on #include "alt_gpio,h"	*/
  extern int gpio_set  (int IO, int Value);			/* If wrong, the compiler will issue a warning	*/

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x0400AAC5)					/* Custom board #1 Cyclone V (R.C)				*/
													/* ---------------------------------------------*/
  #define MY_LED_0				57					/* HPS_GPIO57									*/
  #define MY_LED_1				58					/* HPS_GPIO58									*/
  #define MY_LED_2				59					/* HPS_GPIO59									*/
  #define MY_LED_3				61					/* HPS_GPIO61									*/
  #define MY_LED_FLASH			MY_LED_3
  #define MY_LED_FLASH_S		"3"
  #define MY_LED_OFF(LEDn)		gpio_set(LEDn, 1)
  #define MY_LED_ON(LEDn)		gpio_set(LEDn, 0)
  #define MY_LED_INIT()			do {																\
									gpio_init(0);													\
									gpio_dir(MY_LED_0, 0);											\
									gpio_dir(MY_LED_1, 0);											\
									gpio_dir(MY_LED_2, 0);											\
									gpio_dir(MY_LED_3, 0);											\
									MY_LED_OFF(MY_LED_0);											\
									MY_LED_OFF(MY_LED_1);											\
									MY_LED_OFF(MY_LED_2);											\
									MY_LED_OFF(MY_LED_3);											\
								} while(0)
													/* ---------------------------------------------*/
  extern int gpio_dir  (int IO, int Dir);
  extern int gpio_get  (int IO);					/* Duplicate the function prototypes to not		*/
  extern int gpio_init (int Forced);				/* have a depedency on #include "alt_gpio,h"	*/
  extern int gpio_set  (int IO, int Value);			/* If wrong, the compiler will issue a warning	*/

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x0000AA10)					/* Arria 10 SocFPGA dev board					*/

  #define I2C_TEST_AVAIL		1					/* If the I2C square wave test available		*/
  #define I2C_USE_ADC			1					/* Boolean if ADCs available or not				*/
  #define I2C_ADC_DEVICE		1					/* Which I2C controller is used by the ADC		*/
  #define I2C_ADC_ADDR_0		(0x14)				/* Address of the I2C device					*/
  #define I2C_ADC_ADDR_1		(0x16)				/* Address of the I2C device					*/
  #define I2C_ADC_SPEED			400000				/* Maximum data rate							*/
  #define I2C_ADC_WIDTH			7					/* 7 or 10 bit addresses						*/
  #define I2C_ADC_TYPE			0x2002497			/* 002 appended to indicate 2 devices			*/
  #define ADC_TASK_PRIO			(OX_PRIO_MIN)		/* The ADC is so slow, do updates in the BG		*/

  #define I2C_USE_LCD			1					/* Boolean if LCD is available or not			*/
  #define I2C_LCD_DEVICE		1					/* Which I2C controller is used by the LCD		*/
  #define I2C_LCD_ADDR			(0x28)				/* Address of the I2C device					*/
  #define I2C_LCD_WIDTH			7					/* 7 or 10 bit addresses						*/
  #define I2C_LCD_SPEED			40000				/* This LCD is below standard speed				*/
  #define I2C_LCD_TYPE			2163				/* LCD is NewHeaven Display NHD-0216K3Z			*/

  #define I2C_USE_MAX1619		1
  #define I2C_MAX1619_DEVICE	1
  #define I2C_MAX1619_ADDR		0x4C				/* Address of the I2C device					*/
  #define I2C_MAX1619_SPEED		100000				/* Maximum data rate							*/
  #define I2C_MAX1619_WIDTH		7					/* 7 or 10 bit addresses						*/

  #define I2C_USE_RTC			1					/* Boolean if RTC is available or not			*/
  #define I2C_RTC_DEVICE		1					/* Which I2C controller is used by the RTC		*/
  #define I2C_RTC_ADDR			(0x68)				/* Address of the I2C device					*/
  #define I2C_RTC_WIDTH			7					/* 7 or 10 bit addresses						*/
  #define I2C_RTC_SPEED			400000				/* Supports maximum rate						*/
  #define I2C_RTC_TYPE			1339				/* Maxim DS1339 is the RTC chip					*/
													/* ---------------------------------------------*/
  #define MY_LCD_INIT()			MY_LCD_CLEAR()
  #define MY_LCD_NMB_LINE		2
  #define MY_LCD_WIDTH			16
  #define MY_LCD_CLEAR()		do {																\
									MY_LCD_CLRLINE(0);												\
									MY_LCD_CLRLINE(1);												\
								} while(0)
  #define MY_LCD_CLRLINE(Line)	MY_LCD_LINE(Line, "                    ")
  #define MY_LCD_LINE(x, txt)	do {																\
									if (x == 0) {													\
										LCDputs(txt, NULL);											\
									}																\
									else {															\
										LCDputs(NULL, txt);											\
									}																\
								} while(0)
													/* ---------------------------------------------*/
  extern int gpio_dir  (int IO, int Dir);
  extern int gpio_get  (int IO);					/* Duplicate the function prototypes to not		*/
  extern int gpio_init (int Forced);				/* have a depedency on #include "alt_gpio,h"	*/
  extern int gpio_set  (int IO, int Value);			/* If wrong, the compiler will issue a warning	*/

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x0000FEE6)					/* Sabrelite Quad core i.MX6					*/

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x00007020)					/* Zynq 7020 Zedboard							*/
													/* NON-standard: Maxim DS1339 on Pmod JA1		*/
  #define I2C_USE_RTC			1					/* Boolean if RTC is available or not			*/
  #define I2C_RTC_DEVICE		0					/* Which I2C controller is used by the RTC		*/
  #define I2C_RTC_ADDR			(0x68)				/* Address of the I2C device					*/
  #define I2C_RTC_WIDTH			7					/* 7 or 10 bit addresses						*/
  #define I2C_RTC_SPEED			400000				/* Supports maximum rate						*/
  #define I2C_RTC_TYPE			1339				/* Maxim DS1339 is the RTC chip					*/
													/* NON-standard: EEPROM on Pmod JA1				*/
  #define I2C_USE_EEPROM		1					/* Microchip M24LC01/02 EEPROM: not on-board	*/
  #define I2C_EEPROM_DEVICE		0
  #define I2C_EEPROM_ADDR		(0x50)				/* Address of the I2C device					*/
  #define I2C_EEPROM_WIDTH		7					/* 7 or 10 bit addresses						*/
  #define I2C_EEPROM_SPEED		100000				/* Play safe with speed							*/
  #define I2C_EEPROM_TYPE		2401				/* 24LC01. To use 24LC02 set to value to 2402	*/
													/* ---------------------------------------------*/
  #define MY_BUTTON_0			2000				/* GPIO module in the FPGA						*/
  #define MY_BUTTON_1			2001				/* Base address is 0x41210000					*/
  #define MY_BUTTON_2			2002				/* Must define in the compiler					*/
  #define MY_BUTTON_3			2003				/* GPIO_FPGA_ADDR_2=0x41210000					*/
  #define MY_BUTTON_4			2004
  #define BUT_GET(Button)		(0 == gpio_get(Button))
  #define MY_BUTTON_INIT()		do {																\
									gpio_init(0);													\
									gpio_dir(MY_BUTTON_0, 1);										\
									gpio_dir(MY_BUTTON_1, 1);										\
									gpio_dir(MY_BUTTON_2, 1);										\
									gpio_dir(MY_BUTTON_3, 1);										\
									gpio_dir(MY_BUTTON_4, 1);										\
								} while(0)
 													/* ---------------------------------------------*/
  #define MY_LED_0				1000				/* GPIO module in the FPGA						*/
  #define MY_LED_1				1001				/* Base address is 0x41200000					*/
  #define MY_LED_2				1002				/* Must define in the compiler					*/
  #define MY_LED_3				1003				/* GPIO_FPGA_ADDR_1=0x41200000					*/
  #define MY_LED_4				1004
  #define MY_LED_5				1005
  #define MY_LED_6				1006
  #define MY_LED_7				1007
  #define MY_LED_FLASH			MY_LED_7
  #define MY_LED_FLASH_S		"7"
  #define MY_LED_INIT()			do {																\
									gpio_init(0);													\
									gpio_dir(MY_LED_0, 0);											\
									gpio_dir(MY_LED_1, 0);											\
									gpio_dir(MY_LED_2, 0);											\
									gpio_dir(MY_LED_3, 0);											\
									gpio_dir(MY_LED_4, 0);											\
									gpio_dir(MY_LED_5, 0);											\
									gpio_dir(MY_LED_6, 0);											\
									gpio_dir(MY_LED_7, 0);											\
									MY_LED_OFF(MY_LED_0);											\
									MY_LED_OFF(MY_LED_1);											\
									MY_LED_OFF(MY_LED_2);											\
									MY_LED_OFF(MY_LED_3);											\
									MY_LED_OFF(MY_LED_4);											\
									MY_LED_OFF(MY_LED_5);											\
									MY_LED_OFF(MY_LED_6);											\
									MY_LED_OFF(MY_LED_7);											\
								} while(0)
  #define MY_LED_OFF(LEDn)		gpio_set(LEDn, 0)
  #define MY_LED_ON(LEDn)		gpio_set(LEDn, 1)
													/* ---------------------------------------------*/
  #define MY_SWITCH_0			3000				/* GPIO module in the FPGA						*/
  #define MY_SWITCH_1			3001				/* Base address is 0x41220000					*/
  #define MY_SWITCH_2			3002				/* Must define in the compiler					*/
  #define MY_SWITCH_3			3003				/* GPIO_FPGA_ADDR_3=0x41220000					*/
  #define MY_SWITCH_4			3004
  #define MY_SWITCH_5			3005
  #define MY_SWITCH_6			3006
  #define MY_SWITCH_7			3007
  #define MY_SWITCH_INIT()		gpio_init(0)		/* to truly initialize the GPIOs				*/
  #define SW_GET(Switch)		(0 == gpio_get(Switch))
													/* ---------------------------------------------*/
  extern int gpio_dir  (int IO, int Dir);
  extern int gpio_get  (int IO);					/* Duplicate the function prototypes to not		*/
  extern int gpio_init (int Forced);				/* have a depedency on #include "xlx_gpio,h"	*/
  extern int gpio_set  (int IO, int Value);			/* If wrong, the compiler will issue a warning	*/
													/* ---------------------------------------------*/
													/* Xilinx's BSP overloads some libc functions	*/
 #if defined(__GNUC__) && ((OS_SYS_CALL) != 0)		/* Defined so SysCall is directly accessed		*/
  #include <reent.h>
  struct _reent *__getreent(void);
  #include "SysCall.h"								/* Include as definition dependencies are there	*/

  #define close(a)			_close_r (__getreent(), a)
  #define fstat(a,b)		_fstat_r (__getreent(), a, b)
  #define isatty(a)			_isatty  (__getreent(), a)
  #define lseek(a,b,c)		_lseek_r (__getreent(), a, b, c)
  #define mkdir(a,b)		_mkdir_r (__getreent(), a, b)
  #define open(a,b,c)		_open_r  (__getreent(), a, b, c)
  #define read(a,b,c)		_read_r  (__getreent(), a, b, c)
  #define unlink(a)			_unlink_r(__getreent(), a)
  #define write(a,b,c)		_write_r (__getreent(), a, b, c)
 #endif

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x00007753)					/* Zynq UltraScale+ A53 processors				*/
													/* NON-standard: Maxim DS1339 on PMOD J169		*/
  #define I2C_USE_RTC			1					/* Boolean if RTC is available or not			*/
  #define I2C_RTC_DEVICE		0					/* Which I2C controller is used by the RTC		*/
  #define I2C_RTC_ADDR			(0x68)				/* Address of the I2C device					*/
  #define I2C_RTC_WIDTH			7					/* 7 or 10 bit addresses						*/
  #define I2C_RTC_SPEED			400000				/* Supports maximum rate						*/
  #define I2C_RTC_TYPE			1339				/* Maxim DS1339 is the RTC chip					*/
													/* NON-standard: EEPROM on Pmod JA1				*/
  #define I2C_USE_EEPROM		1					/* Microchip M24LC01/02 EEPROM: not on-board	*/
  #define I2C_EEPROM_DEVICE		0
  #define I2C_EEPROM_ADDR		(0x50)				/* Address of the I2C device					*/
  #define I2C_EEPROM_WIDTH		7					/* 7 or 10 bit addresses						*/
  #define I2C_EEPROM_SPEED		100000				/* Play safe with speed							*/
  #define I2C_EEPROM_TYPE		2401				/* 24LC01. To use 24LC02 set to value to 2402	*/
													/* ---------------------------------------------*/
  #define MY_BUTTON_0			22					/* PS pushbutton								*/
  #define BUT_GET(Button)		(0 == gpio_get(Button))
  #define MY_BUTTON_INIT()		do {																\
									gpio_init(0);													\
									gpio_dir(MY_BUTTON_0, 1);										\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LED_0				23					/* PS user LED									*/
  #define MY_LED_FLASH			MY_LED_0
  #define MY_LED_FLASH_S		"0"
  #define MY_LED_OFF(LEDn)		gpio_set(LEDn, 0)
  #define MY_LED_ON(LEDn)		gpio_set(LEDn, 1)
  #define MY_LED_INIT()			do {																\
									gpio_init(0);													\
									gpio_dir(MY_LED_0, 0);											\
									MY_LED_OFF(MY_LED_0);											\
								} while(0)
													/* ---------------------------------------------*/
  extern int gpio_dir  (int IO, int Dir);
  extern int gpio_get  (int IO);					/* Duplicate the function prototypes to not		*/
  extern int gpio_init (int Forced);				/* have a depedency on #include "xlx_gpio,h"	*/
  extern int gpio_set  (int IO, int Value);			/* If wrong, the compiler will issue a warning	*/

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x003C1124)					/* BB LPC11U24 board							*/
  #define MY_LCD_NMB_LINE		8
  #define MY_LCD_LINE(x, txt)	do {																\
									init_xy_axis(PAGE0+x,COL0);										\
									lcd_display_string((unsigned char *)txt);						\
								} while(0)
  #define MY_LCD_INIT()			do {																\
									init_glcd();													\
									glcd_clear();													\
									init_xy_axis(PAGE0,COL0);										\
									back_light_on();												\
								} while(0)

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x003C1343)					/* BlueBoard LPC1434							*/

  #define MY_LCD_LINE(_x,_T)	do{																	\
									init_xy_axis(PAGE0+_x,COL0);									\
									lcd_display_string((unsigned char *)_T);						\
								} while(0)

  #define MY_LCD_INIT()			do {																\
									init_glcd();													\
									glcd_clear();													\
									init_xy_axis(PAGE0,COL0);										\
									back_light_on();												\
								} while(0)

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x003C1766)					/* Olimex LPC1766-STK board						*/
  #define MY_LCD_NMB_LINE		10
  #define MY_LCD_WIDTH			21
  #define MY_LCD_LINE(x, txt)	do {																\
									int _x = x;														\
									GLCD_SetWindow(0, ((_x)*12)+3, GLCD_HORIZONTAL_SIZE-1,			\
									               (((_x)+1)*12)+3);								\
									GLCD_TextSetPos(0,0);											\
									GLCD_print("\f %s\r", txt);										\
								} while(0)
  #define MY_LCD_CLEAR()		LCD_Clear(BG_COLOR)
  #define MY_LCD_CLRLINE(Line)	MY_LCD_LINE(Line, "                    ")
  #define MY_LCD_INIT()			do {																\
									int _ii;														\
									GLCD_PowerUpInit(NULL);											\
									GLCD_Backlight(BACKLIGHT_ON);									\
									Dly100us((void*)3000);											\
									GLCD_SetFont(&Terminal_9_12_6, FG_COLOR, BG_COLOR);				\
									for (_ii=0 ; _ii<10 ; _ii++) {									\
										MY_LCD_LINE(_ii, " ");										\
									}																\
									GLCD_SetWindow(0,0,GLCD_HORIZONTAL_SIZE-1,GLCD_VERTICAL_SIZE-1);\
									GLCD_TextSetPos(0,0);											\
									GLCD_SetWindow(0, 120, GLCD_HORIZONTAL_SIZE-1, 131);			\
									GLCD_TextSetPos(0,0);											\
									GLCD_print("\f \r");											\
									GLCD_SetWindow(0, 0, GLCD_HORIZONTAL_SIZE-1, 12);				\
									GLCD_TextSetPos(0,0);											\
									GLCD_print("\f \r");											\
								} while(0)

  #define BG_COLOR				0x00000FFF			/* Background RGB color							*/
  #define FG_COLOR				0x00000000			/* Foreground RGB color							*/

  #define DISPLAY(x,y) 			do {																\
									MY_LCD_LINE(x,y);												\
									puts(y);														\
								} while(0)

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x1032F107)					/* Olimex STM-P107 board						*/

  #define MY_LED_0				LED1
  #define MY_LED_1				LED2
  #define MY_LED_FLASH			MY_LED_1
  #define MY_LED_FLASH_S		"1"
  #define MY_LED_OFF(LEDn)		STM_EVAL_LEDOff(LEDn)
  #define MY_LED_ON(LEDn)		STM_EVAL_LEDOn(LEDn)
  #define MY_LED_INIT()			do {																\
									STM_EVAL_LEDInit(MY_LED_0);										\
									STM_EVAL_LEDInit(MY_LED_1);										\
									MY_LED_OFF(MY_LED_0);											\
									MY_LED_OFF(MY_LED_1);											\
								} while(0)

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x0032F207)					/* STM STM3220G Evaluation board				*/

  #define MY_BUTTON_0	(100+(int)BUTTON_WAKEUP)
  #define MY_BUTTON_1	(100+(int)BUTTON_TAMPER)
  #define MY_BUTTON_2	(100+(int)BUTTON_KEY)
  #define MY_BUTTON_3	JOY_UP
  #define MY_BUTTON_4	JOY_DOWN
  #define MY_BUTTON_5	JOY_LEFT
  #define MY_BUTTON_6	JOY_RIGHT
  #define MY_BUTTON_7	JOY_CENTER
  #define BUT_GET(Button)		(((Button)==MY_BUTTON_0)											\
									? (!STM_EVAL_PBGetState(BUTTON_WAKEUP))							\
									: (((Button)==MY_BUTTON_1)										\
										?  STM_EVAL_PBGetState(BUTTON_TAMPER)						\
										: (((Button)==MY_BUTTON_2)									\
											?  STM_EVAL_PBGetState(BUTTON_KEY)						\
											: !(IOE_JoyStickGetState()==((JOYState_TypeDef)Button)))))
  #define MY_BUTTON_INIT()		do {																\
									STM_EVAL_PBInit(MY_BUTTON_0, BUTTON_MODE_GPIO);					\
									STM_EVAL_PBInit(MY_BUTTON_1, BUTTON_MODE_GPIO);					\
									STM_EVAL_PBInit(MY_BUTTON_2, BUTTON_MODE_GPIO);					\
									/*IOE_ITConfig(IOE_ITSRC_JOYSTICK);*/ 							\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LED_0				LED1
  #define MY_LED_1				LED2
  #define MY_LED_2				LED3
  #define MY_LED_3				LED4
  #define MY_LED_FLASH			MY_LED_3
  #define MY_LED_FLASH_S		"3"
  #define MY_LED_OFF(LEDn)		STM_EVAL_LEDOff(LEDn)
  #define MY_LED_ON(LEDn)		STM_EVAL_LEDOn(LEDn)
  #define MY_LED_INIT()			do {																\
									STM_EVAL_LEDInit(MY_LED_0);										\
									STM_EVAL_LEDInit(MY_LED_1);										\
									STM_EVAL_LEDInit(MY_LED_2);										\
									STM_EVAL_LEDInit(MY_LED_3);										\
									MY_LED_OFF(MY_LED_0);											\
									MY_LED_OFF(MY_LED_1);											\
									MY_LED_OFF(MY_LED_2);											\
									MY_LED_OFF(MY_LED_3);											\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LCD_NMB_LINE		10
  #define MY_LCD_WIDTH			20
  #define BG_COLOR				(((0xFC>>3)<<11)|((0x39>>2)<<5)|(0x02>>3))
  #define FG_COLOR				0x0000				/* Black										*/
  #define MY_LCD_LINE(x, txt)	LCD_DisplayStringLine(LINE(x), (uint8_t *)txt)
  #define MY_LCD_CLEAR()		LCD_Clear(BG_COLOR)
  #define MY_LCD_CLRLINE(Line)	MY_LCD_LINE(Line, "                    ")
  #define MY_LCD_INIT()			do {																\
									STM322xG_LCD_Init();											\
									LCD_Clear(BG_COLOR);											\
									LCD_SetColors(FG_COLOR, BG_COLOR);								\
								} while(0)

  extern void SD_ProcessDMAIRQ(void);				/* micro SD I/F interrupt handler				*/
  extern void SD_ProcessIRQSrc(void);				/* micro SD I/F interrupt handler				*/

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x0032F407)					/* STM STM32F4X Discovery board					*/

  #define MY_BUTTON_0			BUTTON_USER
  #define BUT_GET(Button)		(!STM_EVAL_PBGetState(Button))
  #define MY_BUTTON_INIT()		do {																\
									STM_EVAL_PBInit(MY_BUTTON_0, BUTTON_MODE_GPIO);					\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LED_0				LED4
  #define MY_LED_1				LED5
  #define MY_LED_2				LED6
  #define MY_LED_3				LED3
  #define MY_LED_FLASH			MY_LED_3
  #define MY_LED_FLASH_S		"3"
  #define MY_LED_OFF(LEDn)		STM_EVAL_LEDOff(LEDn)
  #define MY_LED_ON(LEDn)		STM_EVAL_LEDOn(LEDn)
  #define MY_LED_INIT()			do {																\
									STM_EVAL_LEDInit(MY_LED_0);										\
									STM_EVAL_LEDInit(MY_LED_1);										\
									STM_EVAL_LEDInit(MY_LED_2);										\
									STM_EVAL_LEDInit(MY_LED_3);										\
									MY_LED_OFF(MY_LED_0);											\
									MY_LED_OFF(MY_LED_1);											\
									MY_LED_OFF(MY_LED_2);											\
									MY_LED_OFF(MY_LED_3);											\
								} while(0)

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x1032F407)					/* Olimex STM32-P407							*/

  #define MY_BUTTON_0			BUTTON_TAMPER		/* Tamper button is inverted					*/
  #define MY_BUTTON_1			BUTTON_WAKEUP
  #define MY_BUTTON_2			BUTTON_RIGHT
  #define MY_BUTTON_3			BUTTON_LEFT
  #define MY_BUTTON_4			BUTTON_UP
  #define MY_BUTTON_5			BUTTON_DOWN
  #define MY_BUTTON_6			BUTTON_SEL
  #define BUT_GET(Button)		((Button == BUTTON_TAMPER) 											\
								?  (STM_EVAL_PBGetState(Button))									\
								: (!STM_EVAL_PBGetState(Button)))
  #define MY_BUTTON_INIT()		do {																\
									STM_EVAL_PBInit(MY_BUTTON_0, BUTTON_MODE_GPIO);					\
									STM_EVAL_PBInit(MY_BUTTON_1, BUTTON_MODE_GPIO);					\
									STM_EVAL_PBInit(MY_BUTTON_2, BUTTON_MODE_GPIO);					\
									STM_EVAL_PBInit(MY_BUTTON_3, BUTTON_MODE_GPIO);					\
									STM_EVAL_PBInit(MY_BUTTON_4, BUTTON_MODE_GPIO);					\
									STM_EVAL_PBInit(MY_BUTTON_5, BUTTON_MODE_GPIO);					\
									STM_EVAL_PBInit(MY_BUTTON_6, BUTTON_MODE_GPIO);					\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LED_0				LED1
  #define MY_LED_1				LED2
  #define MY_LED_2				LED3
  #define MY_LED_3				LED4
  #define MY_LED_FLASH			MY_LED_3
  #define MY_LED_FLASH_S		"3"
  #define MY_LED_OFF(LEDn)		STM_EVAL_LEDOff(LEDn)
  #define MY_LED_ON(LEDn)		STM_EVAL_LEDOn(LEDn)
  #define MY_LED_INIT()			do {																\
									STM_EVAL_LEDInit(MY_LED_0);										\
									STM_EVAL_LEDInit(MY_LED_1);										\
									STM_EVAL_LEDInit(MY_LED_3);										\
									STM_EVAL_LEDInit(MY_LED_2);										\
									MY_LED_OFF(MY_LED_0);											\
									MY_LED_OFF(MY_LED_1);											\
									MY_LED_OFF(MY_LED_2);											\
									MY_LED_OFF(MY_LED_3);											\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LCD_WIDTH			21					/* LCD has 21 chars per line					*/
  #define MY_LCD_NMB_LINE		10
  #define MY_BG_COLOR			0x00000FFF
  #define MY_FG_COLOR			0x00000000
  #define MY_LCD_LINE(x, txt)	do {																\
									int _x=x;														\
									GLCD_SetWindow(0, ((_x)*12)+3, GLCD_HORIZONTAL_SIZE-1, 			\
									               (((_x)+1)*12)+3);								\
									GLCD_TextSetPos(0,0);											\
									GLCD_print("\f %s\r", txt);										\
								} while(0)
  #define MY_LCD_CLRLINE(Line)	 MY_LCD_LINE(Line,"                  ")
  #define MY_LCD_INIT()			do {																\
									GLCD_PowerUpInit(NULL);											\
									GLCD_Backlight(BACKLIGHT_ON);									\
									DelayResolution100us(3000U);									\
									GLCD_SetFont(&Terminal_9_12_6, MY_FG_COLOR, MY_BG_COLOR);		\
									GLCD_SetWindow(0,0,GLCD_HORIZONTAL_SIZE-1,GLCD_VERTICAL_SIZE-1);\
									GLCD_TextSetPos(0,0);											\
									GLCD_print("\f \r");											\
								} while(0)

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x00351968)					/* TI ELK3S1968 board							*/

  #define MY_BUTTON_0			GPIO_PIN_3
  #define MY_BUTTON_1			GPIO_PIN_4
  #define MY_BUTTON_2			GPIO_PIN_5
  #define MY_BUTTON_3			GPIO_PIN_6
  #define BUT_GET(Button)		(0 != gpio_get(Button))
  #define MY_BUTTON_INIT()		do {																\
									gpio_init(0);													\
									gpio_dir(MY_BUTTON_0, 1);										\
									gpio_dir(MY_BUTTON_1, 1);										\
									gpio_dir(MY_BUTTON_2, 1);										\
									gpio_dir(MY_BUTTON_3, 1);										\
								} while(0)
													/* ---------------------------------------------*/
  #define MY_LCD_WIDTH			20
  #define MY_LCD_NMB_LINE		12
  #define MY_LCD_INIT()			do { RIT128x96x4Init(1000000); MY_LCD_CLEAR();   } while(0)
  #define MY_LCD_LINE(x, txt)	do { RIT128x96x4StringDraw(txt, 0, (8*(x)), 15); } while(0)
  #define MY_LCD_CLRLINE(Line)	MY_LCD_LINE(Line,"                  ")
													/* ---------------------------------------------*/
  #define gpio_dir(IO,Dir)		do {																\
									GPIOPinTypeGPIOInput(GPIO_PORTG_BASE, IO);						\
								    GPIOPadConfigSet(GPIO_PORTG_BASE, IO, GPIO_STRENGTH_2MA,		\
									                 GPIO_PIN_TYPE_STD_WPU);						\
				    				GPIOIntTypeSet(GPIO_PORTG_BASE, IO, GPIO_FALLING_EDGE);			\
								    GPIOPinIntEnable(GPIO_PORTG_BASE, IO);							\
								} while(0)
  #define gpio_get(IO)			(GPIOPinIntStatus(GPIO_PORTG_BASE, IO))
  #define gpio_init(Forced)		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG)

/* ------------------------------------------------------------------------------------------------ */

#elif ((OS_PLATFORM) == 0x00435438)					/* Olimex MSP-5438STK board						*/
  #define LCD_FREQ	 			(1000000L)			/* LCD serial clock rate						*/

/* ------------------------------------------------------------------------------------------------ */
#endif

#ifndef I2C_TEST_AVAIL
  #define I2C_TEST_AVAIL		0
#endif
#ifndef I2C_USE_ADC
  #define I2C_USE_ADC			0
#endif
#ifndef I2C_USE_LCD
  #define I2C_USE_LCD			0
#endif
#ifndef I2C_USE_MAX1619
  #define I2C_USE_MAX1619		0
#endif
#ifndef I2C_USE_RTC
  #define I2C_USE_RTC			0
#endif
#ifndef I2C_USE_XL345
  #define I2C_USE_XL345			0
#endif
#define I2C_IN_USE				  (((I2C_USE_ADC)     != 0) ||  ((I2C_USE_LCD) != 0)			\
								|| ((I2C_USE_MAX1619) != 0)	||  ((I2C_USE_RTC) != 0)			\
								|| ((I2C_USE_XL345)   != 0))
													/* ---------------------------------------------*/
#ifndef MY_BUTTON_0
  #define MY_BUTTON_INIT()		OX_DO_NOTHING()
  #define BUT_GET(Button)		OX_DO_NOTHING()
#endif
													/* ---------------------------------------------*/
#ifndef MY_LCD_NMB_LINE
  #define MY_LCD_NMB_LINE		0
  #define MY_LCD_WIDTH			0
  #define MY_LCD_WIDTH			0
  #define LCD_INIT()			OX_DO_NOTHING()
#endif

#ifndef MY_LCD_CLEAR
  #define MY_LCD_CLEAR()		do {																\
									int _ii;														\
									for(_ii=0 ; _ii<MY_LCD_NMB_LINE ; _ii++) {						\
										MY_LCD_CLRLINE(_ii);										\
									}																\
								} while(0)
#endif
#ifndef MY_LCD_CLRLINE
  #define MY_LCD_CLRLINE(Line)	OX_DO_NOTHING()
#endif
#ifndef MY_LCD_INIT
  #define MY_LCD_INIT()			OX_DO_NOTHING()
#endif
#ifndef MY_LCD_LINE
  #define MY_LCD_LINE(x, txt)	OX_DO_NOTHING()
#endif
#ifndef MY_LCD_STDOUT
  #define MY_LCD_STDOUT(L, T) 	do { MY_LCD_LINE(L,T); puts(T); } while(0)
#endif
													/* ---------------------------------------------*/
#ifndef MY_LED_0
  #define MY_LED_INIT()			OX_DO_NOTHING()
  #define MY_LED_OFF(LEDn)		OX_DO_NOTHING()
  #define MY_LED_ON(LEDn)		OX_DO_NOTHING()
#endif
													/* ---------------------------------------------*/
#ifndef MY_SWITCH_0
  #define MY_SWITCH_INIT()		OX_DO_NOTHING()
  #define SW_GET(Switch)		OX_DO_NOTHING()
#endif
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ADC XL345 definitions																			*/

#define XL345_RATE_3200				0x0f			/* Bit values in BW_RATE						*/
#define XL345_RATE_1600				0x0e			/* Expresed as output data rate					*/
#define XL345_RATE_800				0x0d
#define XL345_RATE_400				0x0c
#define XL345_RATE_200				0x0b
#define XL345_RATE_100				0x0a
#define XL345_RATE_50				0x09
#define XL345_RATE_25				0x08
#define XL345_RATE_12_5				0x07
#define XL345_RATE_6_25				0x06
#define XL345_RATE_3_125			0x05
#define XL345_RATE_1_563			0x04
#define XL345_RATE__782				0x03
#define XL345_RATE__39				0x02
#define XL345_RATE__195				0x01
#define XL345_RATE__098				0x00

#define XL345_RANGE_2G			 	0x00			/* Bit values in DATA_FORMAT					*/
#define XL345_RANGE_4G			 	0x01			/* Register values read in DATAX0 through		*/
#define XL345_RANGE_8G			 	0x02			/* DATAZ1 are dependant on the value specified	*/
#define XL345_RANGE_16G			 	0x03			/* in data format. Customer code will need to	*/
#define XL345_DATA_JUST_RIGHT	 	0x00			/* interpret the data as desired.				*/
#define XL345_DATA_JUST_LEFT	 	0x04
#define XL345_10BIT				 	0x00
#define XL345_FULL_RESOLUTION	 	0x08
#define XL345_INT_LOW			 	0x20
#define XL345_INT_HIGH			 	0x00
#define XL345_SPI3WIRE			 	0x40
#define XL345_SPI4WIRE			 	0x00
#define XL345_SELFTEST			 	0x80


#define XL345_OVERRUN			 	0x01			/* Bit values in INT_ENABLE, INT_MAP, and		*/
#define XL345_WATERMARK			 	0x02			/* INT_SOURCE are identical use these bit values*/
#define XL345_FREEFALL			 	0x04			/* to read or write any of these registers		*/
#define XL345_INACTIVITY		 	0x08
#define XL345_ACTIVITY			 	0x10
#define XL345_DOUBLETAP			 	0x20
#define XL345_SINGLETAP			 	0x40
#define XL345_DATAREADY			 	0x80

#define XL345_WAKEUP_8HZ		 	0x00			/* Bit values in POWER_CTL						*/
#define XL345_WAKEUP_4HZ		 	0x01
#define XL345_WAKEUP_2HZ		 	0x02
#define XL345_WAKEUP_1HZ		 	0x03
#define XL345_SLEEP				 	0x04
#define XL345_MEASURE			 	0x08
#define XL345_STANDBY			 	0x00
#define XL345_AUTO_SLEEP		 	0x10
#define XL345_ACT_INACT_SERIAL	 	0x20
#define XL345_ACT_INACT_CONCURRENT 0x00

#define XL345_REG_DEVID				0x00			/* Register List								*/
#define XL345_REG_POWER_CTL			0x2D
#define XL345_REG_DATA_FORMAT		0x31
#define XL345_REG_FIFO_CTL			0x38
#define XL345_REG_BW_RATE			0x2C
#define XL345_REG_INT_ENABLE		0x2E			/* default value: 0x00							*/
#define XL345_REG_INT_MAP			0x2F			/* default value: 0x00							*/
#define XL345_REG_INT_SOURCE		0x30			/* default value: 0x02							*/
#define XL345_REG_DATA_FORMAT		0x31			/* default value: 0x00							*/
#define XL345_REG_DATAX0			0x32			/* read only									*/
#define XL345_REG_DATAX1			0x33			/* read only									*/
#define XL345_REG_DATAY0			0x34			/* read only									*/
#define XL345_REG_DATAY1			0x35			/* read only									*/
#define XL345_REG_DATAZ0			0x36			/* read only									*/
#define XL345_REG_DATAZ1			0x37			/* read only									*/


#ifdef __cplusplus
}
#endif

#endif

/* EOF */

