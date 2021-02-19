#ifndef MULTI_TOUCH2_H_
#define MULTI_TOUCH2_H_

#include "queue.h"

#define TOUCH_QUEUE_SIZE    32

////////////////////////////////////
//
// 8-way ST Pan 
#define MTC_NO_GESTURE              0x00 
#define MTC_ST_UP                   0x1c
#define MTC_ST_LEFT                 0x10
#define MTC_ST_DOWN                 0x14
#define MTC_ST_RIGHT                0x18

// MT Zoom 
#define MTC_ZOOM_IN                 0x48 
#define MTC_ZOOM_OUT                0x49 



typedef struct{
	alt_u32 TOUCH_I2C_BASE;
    alt_u32 TOUCH_INT_BASE;
    alt_u32 INT_IRQ_NUM;
    QUEUE_STRUCT *pQueue;
}MTC2_INFO;


typedef struct{
//getture not support this version
    alt_u8 Event;
    alt_u8 TouchNum;
    alt_u16 x1;
    alt_u16 y1;
}MTC2_EVENT;


void mtc2_QueryData(MTC2_INFO *p);
MTC2_INFO* MTC2_Init(alt_u32 TOUCH_I2C_BASE,alt_u32 TOUCH_INT_BASE, alt_u32 INT_IRQ_NUM);
void MTC2_UnInit(MTC2_INFO *p);
//,int X3,int Y3,int X4,int Y4,int X5,int Y5
bool MTC2_GetStatus(MTC2_INFO *p,alt_u8 *Event, alt_u8 *TouchNum, uint16_t *X1, uint16_t *Y1);
//void MTC2_ShowEventText(alt_u8 Event);
void MTC2_ClearEvent(MTC2_INFO *p);

//void MTC_QueryEventText(alt_u8 Event, char szText[32]);



#endif /*MULTI_TOUCH_H_*/
