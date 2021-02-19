// --------------------------------------------------------------------
// Copyright (c) 2011 by Terasic Technologies Inc. 
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


#include "terasic_includes.h"
#include "gui.h"
#include "vip_fr.h"
#include "multi_touch.h"
#include "simple_graphics.h"
#include "geometry.h"
#include "gesture.h"

extern const struct bmp_struct gesture_bmp;

//#define ENALBE_TOUCH_FILTER
//#define DUAL_FRAME_BUFFER

#define FR_FRAME_0  0x20000000
#define FR_FRAME_1 (0x20000000 + FRAME_WIDTH*FRAME_HEIGHT*4)

#define FRAME_WIDTH  800
#define FRAME_HEIGHT 480
#define DOT_SIZE    6

//#define VALID_POINT(x,y) (((x)>=DOT_SIZE && (x)<FRAME_WIDTH-DOT_SIZE && (y)>=DOT_SIZE && (y)<FRAME_HEIGHT-DOT_SIZE)?TRUE:FALSE)

void GUI_ShowInfo(VIP_FRAME_READER *pReader, char *pText);
void GUI_ShowWelcome(VIP_FRAME_READER *pReader);
void GUI_ShowTouchPoint(VIP_FRAME_READER *pReader, int X1, int Y1, int color);


alt_u32 szPallete[] = {
    BLACK_24,
    WHITE_24,
    0xFFFFFF,
    0x0000FF,
    0x00FF00,
    0xFF0000,
    0x00FFFF,
    0xFFFF00,
    0xFF00FF,
    //0x7F7F7F
};

typedef enum{
    BTN_GESTRUE = 0,
    BTN_CLEAR,
    BTN_WHITE,
    BTN_BLUE,
    BTN_GREEN,
    BTN_RED,
    BTN_SKY,
    BTN_YELLOW,
    BTN_PINK,
    //BTN_GRAY,
    BTN_NUM,
    
    BTN_NONE
}BUTTON_ID;

typedef struct{
    RECT rc;
    int  color;
}PALETTE_INFO;

/*typedef struct{
    int Paint0_Index;
    int Paint1_Index;
    //add by matthew for mtl2
    int Paint2_Index;
    int Paint3_Index;
    int Paint4_Index;
    RECT rcPaint;
    PALETTE_INFO szPalette[BTN_NUM];
}DESK_INFO;*/

typedef struct{
    int Paint0_Index;
    int Paint1_Index;
    RECT rcPaint;
    PALETTE_INFO szPalette[BTN_NUM];
}DESK_INFO;

void GUI_DeskInit(VIP_FRAME_READER *pReader, DESK_INFO *pDeskInfo){
    int i;
    RECT rc;
    const int BoxH = 70;
    const int BoxW = 70;
    const int GapH = 10;
    const int DrawBorder = 2;
    const int TextW = 120;
    int GapW;

    
    GapW = (pReader->width - TextW - (BTN_NUM*BoxW))/(BTN_NUM+1);
    rc.top = pReader->height - BoxH - GapH;
    rc.bottom = rc.top + BoxH;
    rc.left = GapW + TextW;
    rc.right = rc.left + BoxW;
    
    // desk drawing area    
    RectSet(&pDeskInfo->rcPaint, DrawBorder, pReader->width-DrawBorder, DrawBorder, rc.top - DrawBorder);
    
    for(i=0;i<BTN_NUM;i++){
        RectCopy(&pDeskInfo->szPalette[i].rc, &rc);
        pDeskInfo->szPalette[i].color = szPallete[i];
        RectOffset(&rc, BoxW+GapW, 0);
    }
    pDeskInfo->Paint0_Index = BTN_BLUE;
    pDeskInfo->Paint1_Index = BTN_GREEN;
    //add by matthew for mtl2
    /*pDeskInfo->Paint2_Index = BTN_RED;
    pDeskInfo->Paint3_Index = BTN_SKY;
    pDeskInfo->Paint4_Index = BTN_YELLOW;*/
}

int GUI_CheckButton(DESK_INFO *pDeskInfo, POINT *pt){
    int ButtonId = BTN_NONE;
    int i;
    
    for(i=0;i<BTN_NUM && (ButtonId == BTN_NONE);i++){
        if (IsPtInRect(pt, &pDeskInfo->szPalette[i].rc))
            ButtonId = i;
    }
    return ButtonId;
}

void GUI_DeskDraw(VIP_FRAME_READER *pReader, DESK_INFO *pDeskInfo){
    int i, active;
    RECT rc;
    
    
    // show text
    vid_print_string_alpha(pDeskInfo->rcPaint.left, pDeskInfo->szPalette[0].rc.top, BLUE_24, BLACK_24, tahomabold_20, pReader, "Terasic");
    vid_print_string_alpha(pDeskInfo->rcPaint.left, pDeskInfo->szPalette[0].rc.top+22, BLUE_24, BLACK_24, tahomabold_20, pReader, "MTL 1");

    // draw border
    RectCopy(&rc, &pDeskInfo->rcPaint);
    vid_draw_box (rc.left, rc.top, rc.right, rc.bottom, WHITE_24, DO_NOT_FILL, pReader);
    

    // draw palette
    for(i=0;i<BTN_NUM;i++){
        RectCopy(&rc, &(pDeskInfo->szPalette[i].rc));
        if (i == BTN_CLEAR){
            vid_draw_box (rc.left, rc.top, rc.right, rc.bottom, pDeskInfo->szPalette[i].color, DO_NOT_FILL, pReader);
            vid_print_string_alpha(rc.left+8, rc.top+(RectHeight(&rc)-22)/2, pDeskInfo->szPalette[i].color, BLACK_24, tahomabold_20, pReader, "Clear");
        }else{
            vid_draw_box (rc.left, rc.top, rc.right, rc.bottom, pDeskInfo->szPalette[i].color, DO_FILL, pReader);
        }            
    }
  
    // draw activce color
    active = pDeskInfo->Paint0_Index;
	RectCopy(&rc, &pDeskInfo->szPalette[active].rc);
	vid_print_string_alpha(rc.left+2, rc.top+2, BLACK_24, pDeskInfo->szPalette[active].color, tahomabold_20, pReader, "1");

	active = pDeskInfo->Paint1_Index;
	RectCopy(&rc, &pDeskInfo->szPalette[active].rc);
	vid_print_string_alpha(rc.left+2, rc.top+2, BLACK_24, pDeskInfo->szPalette[active].color, tahomabold_20, pReader, "2");
    /*active = pDeskInfo->Paint0_Index;
    RectCopy(&rc, &pDeskInfo->szPalette[active].rc);
    vid_print_string_alpha(rc.left+2, rc.top+2, BLACK_24, pDeskInfo->szPalette[active].color, tahomabold_20, pReader, "1");
        
    active = pDeskInfo->Paint1_Index;
    RectCopy(&rc, &pDeskInfo->szPalette[active].rc);
    vid_print_string_alpha(rc.left+2, rc.top+2, BLACK_24, pDeskInfo->szPalette[active].color, tahomabold_20, pReader, "2");
    active = pDeskInfo->Paint2_Index;
	RectCopy(&rc, &pDeskInfo->szPalette[active].rc);
	vid_print_string_alpha(rc.left+2, rc.top+2, BLACK_24, pDeskInfo->szPalette[active].color, tahomabold_20, pReader, "3");
	active = pDeskInfo->Paint3_Index;
	RectCopy(&rc, &pDeskInfo->szPalette[active].rc);
	vid_print_string_alpha(rc.left+2, rc.top+2, BLACK_24, pDeskInfo->szPalette[active].color, tahomabold_20, pReader, "4");
	active = pDeskInfo->Paint4_Index;
	RectCopy(&rc, &pDeskInfo->szPalette[active].rc);
	vid_print_string_alpha(rc.left+2, rc.top+2, BLACK_24, pDeskInfo->szPalette[active].color, tahomabold_20, pReader, "5");*/
        
        
}

bool IsContinuedPoint(POINT *ptPre, POINT *ptNew){
    bool bYes = TRUE;
    const int nMax = 50;
    if (abs(ptPre->x - ptNew->x) > nMax)
        bYes = FALSE;
    else if (abs(ptPre->y - ptNew->y) > nMax)
        bYes = FALSE;
        
    return bYes;        
        
}

void GUI_ShowGesture(VIP_FRAME_READER *pReader, POINT *ptDraw, alt_u8 GestureId ){
	// show bitmap
	int nIndex = -1, x, y;
	const int BmpW = 60;
	const int BmpH = 60;
	alt_u8 *pDes, *pSrc, *pDesStart;

	// find gesture bitmap id
	switch(GestureId){
		case MTC_ST_NORTH:  nIndex = 0; break;
		case MTC_ST_SOUTH:  nIndex = 1; break;
		case MTC_ST_EAST:   nIndex = 2; break;
		case MTC_ST_WEST:   nIndex = 3; break;
		case MTC_ST_N_WEST: nIndex = 4; break;
		case MTC_ST_N_EAST: nIndex = 5; break;
		case MTC_ST_S_WEST: nIndex = 6; break;
		case MTC_ST_S_EAST: nIndex = 7; break;

		case MTC_ST_CLICK:       nIndex = 8; break;
		case MTC_ST_DOUBLECLICK: nIndex = 9; break;

		case MTC_1_POINT:        nIndex = 10; break;

		case MTC_MT_NORTH:        nIndex = 11; break;
		case MTC_MT_SOUTH:        nIndex = 12; break;
		case MTC_MT_EAST:         nIndex = 13; break;
		case MTC_MT_WEST:         nIndex = 14; break;
		case MTC_MT_S_WEST:       nIndex = 17; break;
		case MTC_MT_N_EAST:       nIndex = 16; break;
		case MTC_MT_N_WEST:       nIndex = 15; break;
		case MTC_MT_S_EAST:       nIndex = 18; break;

		case MTC_MT_CLICK:        nIndex = 19; break;
		case MTC_2_POINT:         nIndex = 21; break;

		case MTC_ST_ROTATE_CW:    nIndex = 22; break;
		case MTC_ST_ROTATE_CCW:   nIndex = 23; break;


		case MTC_ZOOM_IN:         nIndex = 24; break;
		case MTC_ZOOM_OUT:        nIndex = 25; break;
	}

	if (nIndex  == -1)
		return;

	pSrc = (alt_u8 *)gesture_bmp.pixel_data + nIndex * BmpW * BmpH * 3;

	pDesStart = (alt_u8 *)VIPFR_GetDrawFrame(pReader) + (ptDraw->y*pReader->width*pReader->bytes_per_pixel) + (ptDraw->x*pReader->bytes_per_pixel);


	for(y=0;y<BmpH;y++){
		pDes = pDesStart;
		for(x=0;x<BmpW;x++){
			memcpy(pDes, pSrc, 3);
			pDes += 4;
			pSrc += 3;
		}
		pDesStart += pReader->width*pReader->bytes_per_pixel;
	}
}

void GUI_TraceGesture(VIP_FRAME_READER *pReader, POINT *ptDraw, alt_u8 GestureId ){
	if (GestureId == MTC_2_POINT)
		return;
	if (GestureId == MTC_1_POINT)
		return;

    GUI_ShowGesture(pReader, ptDraw, GestureId);
}

void GUI_ClearPaintArea(VIP_FRAME_READER *pReader, DESK_INFO *pDeskInfo){
    RECT rc;

    // clear paint area
    RectCopy(&rc, &pDeskInfo->rcPaint);
    RectInflate(&rc, -1, -1);
    rc.left += 1;
    vid_draw_box (rc.left, rc.top, rc.right, rc.bottom, BLACK_24, DO_FILL, pReader);

    // clear gesture
    RectCopy(&rc, &pDeskInfo->szPalette[BTN_GESTRUE].rc);
    vid_draw_box (rc.left, rc.top, rc.right, rc.bottom, BLACK_24, DO_FILL, pReader);
}

void GUI(MTC_INFO *pTouch){
    // video
	static InitFlag = true;
    static DESK_INFO DeskInfo;
    static VIP_FRAME_READER *pReader;
    static int X1, Y1, X2, Y2;
    static POINT Pt1, Pt2, ptGesture;
    static alt_u8 Event, TouchNum;
    static const int nDotSize = DOT_SIZE;
    static RECT rcTouch;
    static int ColorPen0, ColorPen1, ButtonId, ButtonId2;
    static bool UpdatePalette;
#ifdef ENALBE_TOUCH_FILTER
    static POINT LastPt1, LastPt2;
    static alt_u32 LastTimePt1=0,LastTimePt2=0;
    static const alt_u32 ContinuedTime = alt_ticks_per_second()/50;
#endif

    if (InitFlag) {

    	// frame reader
		#ifdef DUAL_FRAME_BUFFER
            pReader =  VIPFR_Init((alt_u32 *)(ALT_LWFPGASLVS_OFST + ALT_VIP_VFR_0_BASE), (void *)FR_FRAME_0, (void *)FR_FRAME_1, FRAME_WIDTH, FRAME_HEIGHT);
        #else
            pReader =  VIPFR_Init((alt_u32 *)(ALT_LWFPGASLVS_OFST + ALT_VIP_VFR_0_BASE), (void *)FR_FRAME_0, (void *)FR_FRAME_0, FRAME_WIDTH, FRAME_HEIGHT);
    	#endif // DUAL_FRAME_BUFFER
    	VIPFR_Go(pReader, TRUE);
    
        // clean screen
        vid_clean_screen(pReader, BLACK_24);


        GUI_DeskInit(pReader, &DeskInfo);
        GUI_DeskDraw(pReader, &DeskInfo);

        VIPFR_ActiveDrawFrame(pReader);
    
        RectCopy(&rcTouch, &DeskInfo.rcPaint);
        RectInflate(&rcTouch, -nDotSize-2, -nDotSize-2);
    
        ColorPen0 = DeskInfo.szPalette[DeskInfo.Paint0_Index].color;
        ColorPen1 = DeskInfo.szPalette[DeskInfo.Paint1_Index].color;
    
        //  printf("ContinuedTime=%d\r\n", ContinuedTime);
        PtSet(&ptGesture, DeskInfo.szPalette[BTN_GESTRUE].rc.left+5, DeskInfo.szPalette[BTN_GESTRUE].rc.top+5);
        
        InitFlag = false;
    }
    
 //   while(1)
//    {
        // touch
        if (MTC_GetStatus(pTouch,&Event, &TouchNum, &X1, &Y1, &X2, &Y2)){ //, &X3, &Y3, &X4, &Y4, &X5, &Y5)){
            
           //printf("GUI : %d - x1=%d y1=%d x2=%d y2=%d\n", TouchNum, X1, Y1, X2, Y2);
           GUI_TraceGesture(pReader, &ptGesture, Event);
           if (TouchNum >= 1)
			   PtSet(&Pt1, X1, Y1);
		   if (TouchNum >= 2)
			   PtSet(&Pt2, X2, Y2);

            if (TouchNum >= 1 && IsPtInRect(&Pt1, &rcTouch)){
#ifdef ENALBE_TOUCH_FILTER
                if ((alt_nticks() > (ContinuedTime + LastTimePt1)) || IsContinuedPoint(&LastPt1, &Pt1)){
                    vid_draw_circle(Pt1.x, Pt1.y, nDotSize, ColorPen0, DO_FILL, pReader);
                    PtCopy(&LastPt1, &Pt1);
                    LastTimePt1 = alt_nticks();
                    if (TouchNum == 2 && IsPstatic tInRect(&Pt2, &rcTouch)){
                        if ((alt_nticks() > (ContinuedTime + LastTimePt2)) || IsContinuedPoint(&LastPt2, &Pt2)){
                            PtCopy(&LastPt2, &Pt2);
                            LastTimePt2 = alt_nticks();
                            vid_draw_circle(Pt2.x, Pt2.y, nDotSize, ColorPen1, DO_FILL, pReader);
                        }else{
                            printf("invalid pt2\r\n");
                        }
                    }
                }else{
                    printf("invalid pt1(%d,%d) pre(%d,%d)\r\n", Pt1.x, Pt1.y, LastPt1.x, LastPt1.y);
                }
#else
                vid_draw_circle(Pt1.x, Pt1.y, nDotSize, ColorPen0, DO_FILL, pReader);
				if (TouchNum == 2 && IsPtInRect(&Pt2, &rcTouch))
					vid_draw_circle(Pt2.x, Pt2.y, nDotSize, ColorPen1, DO_FILL, pReader);
#endif
//            }else if (Event == MTC_ST_DOUBLECLICK || (TouchNum == 1 && GUI_CheckButton(&DeskInfo, &Pt1) == BTN_CLEAR)){
                VIPFR_ActiveDrawFrame(pReader);
            }else if (TouchNum == 1 && GUI_CheckButton(&DeskInfo, &Pt1) == BTN_CLEAR){
                GUI_ClearPaintArea(pReader, &DeskInfo);
                //vid_clean_screen(pReader, BLACK_24);
                //GUI_DeskDraw(pReader, &DeskInfo);
                VIPFR_ActiveDrawFrame(pReader);
                MTC_ClearEvent(pTouch);
            }else if (TouchNum >= 1){
                ButtonId = GUI_CheckButton(&DeskInfo, &Pt1);
                UpdatePalette = FALSE;
                if (ButtonId != BTN_NONE && ButtonId != BTN_CLEAR && ButtonId != BTN_GESTRUE){
					// chang draw color
					if (TouchNum == 1 && ButtonId != DeskInfo.Paint0_Index && ButtonId != DeskInfo.Paint1_Index){
						DeskInfo.Paint0_Index = ButtonId;
						UpdatePalette = TRUE;
						//
					}else if (TouchNum == 2){
						ButtonId2 = GUI_CheckButton(&DeskInfo, &Pt2);
						if (ButtonId2 != BTN_NONE && ButtonId != ButtonId2){
							DeskInfo.Paint0_Index = ButtonId;
							DeskInfo.Paint1_Index = ButtonId2;
							UpdatePalette = TRUE;
						}
					}

					if (UpdatePalette){
						ColorPen0 = DeskInfo.szPalette[DeskInfo.Paint0_Index].color;
						ColorPen1 = DeskInfo.szPalette[DeskInfo.Paint1_Index].color;
						GUI_DeskDraw(pReader, &DeskInfo);
						VIPFR_ActiveDrawFrame(pReader);
						MTC_ClearEvent(pTouch);
					}
				}
			}
        }
//    }
}



void GUI_ShowInfo(VIP_FRAME_READER *pReader, char *pText){
    static int x=0,y=100;
   // vid_clean_screen(pReader, BLACK_24);
    vid_print_string_alpha(x, y, BLUE_24, BLACK_24, tahomabold_20, pReader, pText);
    VIPFR_ActiveDrawFrame(pReader);
}

void GUI_ShowTouchPoint(VIP_FRAME_READER *pReader, int X, int Y, int color){
    vid_draw_circle(X, Y, 10, color, DO_FILL, pReader);
    VIPFR_ActiveDrawFrame(pReader);    
}

