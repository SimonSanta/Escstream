#ifndef GUI_H_
#define GUI_H_

#include "terasic_includes.h"
#include "multi_touch2.h"
#include "vip_fr.h"
#include "stdbool.h"
#include "queue.h"
#include "Const.h"
#include "geometry.h"

void GUI(MTC2_INFO *pTouch);
bool SPI_GetStatus(uint16_t *XR, uint16_t *YR);//added
void print_selection_menu(RECT rc,VIP_FRAME_READER *pReader, LVL *lvl);
void print_lvl_selection(VIP_FRAME_READER *pReader );
void init_im_lvl(int lvl);
void in_lvl_sel_rect(POINT* Pt1, LVL* lvl ,VIP_FRAME_READER *pReader );
void GUI_DeskDraw(LVL *lvl);
void level(int lvl_number, LVL *lvl);

// ***ADDED
typedef struct{
    uint16_t xcoord;
    uint16_t ycoord;
}SPI_EVENT;

extern uint32_t *txdata;
extern SPI_EVENT *lastMsg;
extern int *flag;
extern int *flag2;
extern int *lvl1;
extern int *lvl2;
extern int new_lvl;

// ***ADDED
#define MSG_QUEUE_SIZE		32

#endif /*GUI_H_*/
