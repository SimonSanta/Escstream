#ifndef MULTI_TOUCH_H_
#define MULTI_TOUCH_H_

#include "queue.h"

#define TOUCH_QUEUE_SIZE    32

////////////////////////////////////
//
// 8-way ST Pan 
#define MTC_NO_GESTURE              0x00 
#define MTC_ST_NORTH                0x10 
#define MTC_ST_N_EAST               0x12 
#define MTC_ST_EAST                 0x14 
#define MTC_ST_S_EAST               0x16 
#define MTC_ST_SOUTH                0x18 
#define MTC_ST_S_WEST               0x1A 
#define MTC_ST_WEST                 0x1C 
#define MTC_ST_N_WEST               0x1E 
// ST Rotate 
#define MTC_ST_ROTATE_CW            0x28 
#define MTC_ST_ROTATE_CCW           0x29 
// ST Click 
#define MTC_ST_CLICK                0x20 
#define MTC_ST_DOUBLECLICK          0x22 
// 8-way MT Pan 
#define MTC_MT_NORTH                0x30 
#define MTC_MT_N_EAST               0x32 
#define MTC_MT_EAST                 0x34 
#define MTC_MT_S_EAST               0x36 
#define MTC_MT_SOUTH                0x38 
#define MTC_MT_S_WEST               0x3A 
#define MTC_MT_WEST                 0x3C 
#define MTC_MT_N_WEST               0x3E 
// MT Zoom 
#define MTC_ZOOM_IN                 0x48 
#define MTC_ZOOM_OUT                0x49 
// MT Click 
#define MTC_MT_CLICK                0x40

// richard add
#define  MTC_1_POINT                0x80
#define  MTC_2_POINT                0x81


typedef struct{
    alt_u32 TOUCH_I2C_BASE;
    alt_u32 TOUCH_INT_BASE;
    alt_u32 INT_IRQ_NUM;
    QUEUE_STRUCT *pQueue;
}MTC_INFO;

typedef struct{
    alt_u8 Event;
    alt_u8 TouchNum;
    alt_u16 x1;
    alt_u16 y1;
    alt_u16 x2;
    alt_u16 y2;
}MTC_EVENT;

MTC_INFO* MTC_Init(alt_u32 MUTI_TOUCH_I2C_BASE, alt_u32 MUTI_TOUCH_INT_BASE, alt_u32 INT_IRQ_NUM);
void MTC_UnInit(MTC_INFO *p);
bool MTC_GetStatus(MTC_INFO *p, alt_u8 *Event, alt_u8 *TouchNum, int *X1, int *Y1, int *X2, int *Y2);
void MTC_ShowEventText(alt_u8 Event);
void MTC_ClearEvent(MTC_INFO *p);

void MTC_QueryEventText(alt_u8 Event, char szText[32]);

void mtc_QueryData(MTC_INFO *p);



#endif /*MULTI_TOUCH_H_*/
