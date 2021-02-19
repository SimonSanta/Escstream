// --------------------------------------------------------------------
// Copyright (c) 2010 by Terasic Technologies Inc. 
// --------------------------------------------------------------------
//
// Permission:
//
//   Terasic grants permission to use and modify this code for use
//   in synthesis for all Terasic Development Boards and Altera Development 
//   Kits made by Terasic.  Other use of this code, including the selling 
//   ,duplication, or modification of any portion is strictly prohibited.
//
// Disclaimer:
//
//   This VHDL/Verilog or C/C++ source code is intended as a design reference
//   which illustrates how these types of functions can be implemented.
//   It is the user's responsibility to verify their design for
//   consistency and functionality through the use of formal
//   verification methods.  Terasic provides no warranty regarding the use 
//   or functionality of this code.
//
// --------------------------------------------------------------------
//           
//                     Terasic Technologies Inc
//                     356 Fu-Shin E. Rd Sec. 1. JhuBei City,
//                     HsinChu County, Taiwan
//                     302
//
//                     web: http://www.terasic.com/
//                     email: support@terasic.com
//
// --------------------------------------------------------------------

#ifndef TERASIC_INCLUDES_H_
#define TERASIC_INCLUDES_H_

#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h>
#include <stddef.h>
#include <unistd.h>  // usleep (unix standard?)
#include <fcntl.h>
#include "debug.h"

#include "hwlib.h"
#include "hps_0.h"
#include "socal/alt_gpio.h"
#include "socal/hps.h"
#include "socal/socal.h"

#define DEBUG_DUMP  /*printf */ 

#define TRUE    1
#define FALSE   0

#define alt_u32 uint32_t
#define alt_32  int32_t
#define alt_u16 uint16_t
#define alt_16  int16_t
#define alt_u8  uint8_t
#define alt_8   int8_t

void delay_us(uint32_t us);

#define usleep(x) delay_us(x * 2)
#define IOWR(address, offset, data) alt_write_word(address + (offset*4), data)
#define IORD(address, offset)       alt_read_word(address + (offset*4))

#define IOWR_32DIRECT(address, offset, data)    alt_write_word(address + offset, data)
#define IOWR_16DIRECT(address, offset, data)    alt_write_hword(address + offset, data)
#define IOWR_8DIRECT(address, offset, data)     alt_write_byte(address + offset, data)

#define IORD_32DIRECT(address, offset)          alt_read_word(address + offset)
#define IORD_16DIRECT(address, offset)          alt_read_hword(address + offset)
#define IORD_8DIRECT(address, offset)           alt_read_byte(address + offset)

#endif /*TERASIC_INCLUDES_H_*/
