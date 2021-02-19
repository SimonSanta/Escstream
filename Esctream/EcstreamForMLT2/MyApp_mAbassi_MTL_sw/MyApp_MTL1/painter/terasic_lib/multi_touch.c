#include "terasic_includes.h"
#include "multi_touch.h"
//#include "i2c.h"

#define MTC_REG_CLEAR_FIFO    0     // write only (write any value to clear fifo)
#define MTC_REG_INT_ACK        1        // write only (write any value to ack)
#define MTC_REG_DATA_NUM        2       // read only
#define MTC_REG_TOUCH_NUM       3       // read only
#define MTC_REG_X1              4       // read only
#define MTC_REG_Y1              5       // read only
#define MTC_REG_X2              6       // read only
#define MTC_REG_Y2              7       // read only
#define MTC_REG_GESTURE         8       // read only

void mtc_QueryData(MTC_INFO *p){
    bool bValidEvnt=TRUE;
    alt_u8 Gesture, DataNum;
    MTC_EVENT *pEvent, *pOldEvent;
    
    DataNum = IORD(p->TOUCH_I2C_BASE, MTC_REG_DATA_NUM);
    if (DataNum > 0){
        
        pEvent = (MTC_EVENT *)malloc(sizeof(MTC_EVENT));
        
        pEvent->x1 = IORD(p->TOUCH_I2C_BASE, MTC_REG_X1);
        pEvent->y1 = IORD(p->TOUCH_I2C_BASE, MTC_REG_Y1);
        pEvent->x2 = IORD(p->TOUCH_I2C_BASE, MTC_REG_X2);
        pEvent->y2 = IORD(p->TOUCH_I2C_BASE, MTC_REG_Y2);
        pEvent->TouchNum = IORD(p->TOUCH_I2C_BASE, MTC_REG_TOUCH_NUM);
        Gesture = IORD(p->TOUCH_I2C_BASE, MTC_REG_GESTURE);
        
        // find event type
        if (Gesture == MTC_NO_GESTURE){
            if (pEvent->TouchNum  >= 1 && pEvent->TouchNum  <= 2){
                if (pEvent->TouchNum  == 2){
                    pEvent->Event = MTC_2_POINT;
                }else{
                    pEvent->Event = MTC_1_POINT;
                }
            }else{
                bValidEvnt = FALSE;
            }
        }else{
            if (Gesture == 0xFF) // filter the event
                bValidEvnt = FALSE;
            else
                pEvent->Event = Gesture;
        }
        
        // insert event
        if (bValidEvnt){
            if (QUEUE_IsFull(p->pQueue)){
                // remove the old one
                pOldEvent = (MTC_EVENT *)QUEUE_Pop(p->pQueue);
                free(pOldEvent);
            }
            QUEUE_Push(p->pQueue, (alt_u32)pEvent);
        }else{
            free(pEvent);;
        }
    }        
}


/*static void mtc_ISR(void* context, alt_u32 id){
    MTC_INFO *p = (MTC_INFO *)context;
    
    alt_irq_enable(id);
    // clear capture flag
    if (id == p->INT_IRQ_NUM){
        // query data
        mtc_QueryData(p);
        
        // clear interrupt
        IOWR_ALTERA_AVALON_PIO_EDGE_CAP(p->TOUCH_BASE,0);
        IOWR(p->TOUCH_BASE, MTC_REG_INT_ACK, 0x00);
        
    }
    alt_irq_enable(id);
}*/



MTC_INFO* MTC_Init(alt_u32 MUTI_TOUCH_I2C_BASE, alt_u32 MUTI_TOUCH_INT_BASE, alt_u32 INT_IRQ_NUM){
    int error;
    MTC_INFO *p;
    
    p = (MTC_INFO *)malloc(sizeof(MTC_INFO));
    p->TOUCH_I2C_BASE = MUTI_TOUCH_I2C_BASE;
    p->TOUCH_INT_BASE = MUTI_TOUCH_INT_BASE;
    p->INT_IRQ_NUM = INT_IRQ_NUM;
    p->pQueue = QUEUE_New(TOUCH_QUEUE_SIZE);
    
    //
    // enable interrupt
    /*IOWR_ALTERA_AVALON_PIO_IRQ_MASK(p->TOUCH_BASE,0x01);
    // clear capture flag
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(p->TOUCH_BASE,0);
    // register callback function
    error = alt_irq_register (p->INT_IRQ_NUM, p, mtc_ISR);*/
    /*if (error){
        printf("failed to register touch irq\r\n");
        MTC_UnInit(p);
        p = NULL;
    }*/
    return p;        
}

void MTC_UnInit(MTC_INFO *p){
    if (p){
        QUEUE_Delete(p->pQueue);
        free(p);
    }        
}

bool MTC_GetStatus(MTC_INFO *p, alt_u8 *Event, alt_u8 *TouchNum, int *X1, int *Y1, int *X2, int *Y2 ){
    bool bFind;
    MTC_EVENT *pEvent;
    
    bFind = QUEUE_IsEmpty(p->pQueue)?FALSE:TRUE;
    if (bFind){
        pEvent = (MTC_EVENT *)QUEUE_Pop(p->pQueue);
        *Event = pEvent->Event;
        *TouchNum = pEvent->TouchNum;
        *X1 = pEvent->x1;
        *Y1 = pEvent->y1;
        *X2 = pEvent->x2;
        *Y2 = pEvent->y2;
        free(pEvent);
    }
    
    
    return bFind;
}



void MTC_ShowEventText(alt_u8 Event){
  //  alt_irq_context t;    
    //t = alt_irq_disable_all();
                       // printf("Gesture:");
                        switch(Event){
                            case MTC_ST_NORTH: printf("ST_NORTH"); break;
                            case MTC_ST_N_EAST: printf("ST_N_EAST"); break;
                            case MTC_ST_EAST: printf("ST_EAST"); break;
                            case MTC_ST_S_EAST: printf("ST_S_EAST"); break;
                            case MTC_ST_SOUTH: printf("ST_SOUTH"); break;
                            case MTC_ST_S_WEST: printf("ST_S_WEST"); break;
                            case MTC_ST_WEST: printf("ST_WEST"); break;
                            case MTC_ST_N_WEST: printf("ST_N_WEST"); break;
                            
                            // ST Rotate
                            case MTC_ST_ROTATE_CW: printf("ST_ROTATE_CW"); break;
                            case MTC_ST_ROTATE_CCW: printf("ST_ROTATE_CCW"); break;
                            
                            // ST Click
                            case MTC_ST_CLICK: printf("ST_CLICK"); break;
                            case MTC_ST_DOUBLECLICK: printf("ST_DOUBLECLICK"); break;
                            
                            // 8-way MT Pan
                            case MTC_MT_NORTH: printf("MT_NORTH"); break;
                            case MTC_MT_N_EAST: printf("MT_N_EAST"); break;
                            case MTC_MT_EAST: printf("MT_EAST"); break;
                            case MTC_MT_S_EAST: printf("MT_S_EAST"); break;
                            case MTC_MT_SOUTH: printf("MT_SOUTH"); break;
                            case MTC_MT_S_WEST: printf("MT_S_WEST"); break;
                            case MTC_MT_WEST: printf("MT_WEST"); break;
                            case MTC_MT_N_WEST: printf("MT_N_WEST"); break;
                            
                            // MT Zoom
                            case MTC_ZOOM_IN: printf("ZOOM_IN"); break;
                            case MTC_ZOOM_OUT: printf("ZOOM_OUT"); break;
                            
                            // MT Click
                            case MTC_MT_CLICK: printf("MT_CLICK"); break;
                            
                            // richard add
                            case MTC_1_POINT: printf("1 Point"); break;
                            case MTC_2_POINT: printf("2 Point"); break;
                            default:
                                printf("undefined:%02xh", Event);
                                break;
                        }    
                        //printf("\r\n");
                        usleep(10*1000);
    //alt_irq_enable_all(t);
}



void MTC_ClearEvent(MTC_INFO *p){
    QUEUE_Empty(p->pQueue);
}

void MTC_QueryEventText(alt_u8 Event, char szText[32]){
                        switch(Event){
                            case MTC_ST_NORTH: strcpy(szText, "ST_NORTH"); break;
                            case MTC_ST_N_EAST: strcpy(szText, "ST_N_EAST"); break;
                            case MTC_ST_EAST: strcpy(szText, "ST_EAST"); break;
                            case MTC_ST_S_EAST: strcpy(szText, "ST_S_EAST"); break;
                            case MTC_ST_SOUTH: strcpy(szText, "ST_SOUTH"); break;
                            case MTC_ST_S_WEST: strcpy(szText, "ST_S_WEST"); break;
                            case MTC_ST_WEST: strcpy(szText, "ST_WEST"); break;
                            case MTC_ST_N_WEST: strcpy(szText, "ST_N_WEST"); break;
                            
                            // ST Rotate
                            case MTC_ST_ROTATE_CW: strcpy(szText, "ST_ROTATE_CW"); break;
                            case MTC_ST_ROTATE_CCW: strcpy(szText, "ST_ROTATE_CCW"); break;
                            
                            // ST Click
                            case MTC_ST_CLICK: strcpy(szText, "ST_CLICK"); break;
                            case MTC_ST_DOUBLECLICK: strcpy(szText, "ST_DOUBLECLICK"); break;
                            
                            // 8-way MT Pan
                            case MTC_MT_NORTH: strcpy(szText, "MT_NORTH"); break;
                            case MTC_MT_N_EAST: strcpy(szText, "MT_N_EAST"); break;
                            case MTC_MT_EAST: strcpy(szText, "MT_EAST"); break;
                            case MTC_MT_S_EAST: strcpy(szText, "MT_S_EAST"); break;
                            case MTC_MT_SOUTH: strcpy(szText, "MT_SOUTH"); break;
                            case MTC_MT_S_WEST: strcpy(szText, "MT_S_WEST"); break;
                            case MTC_MT_WEST: strcpy(szText, "MT_WEST"); break;
                            case MTC_MT_N_WEST: strcpy(szText, "MT_N_WEST"); break;
                            
                            // MT Zoom
                            case MTC_ZOOM_IN: strcpy(szText, "ZOOM_IN"); break;
                            case MTC_ZOOM_OUT: strcpy(szText, "ZOOM_OUT"); break;
                            
                            // MT Click
                            case MTC_MT_CLICK: strcpy(szText, "MT_CLICK"); break;
                            
                            // richard add
                            case MTC_1_POINT: strcpy(szText, "1 Point"); break;
                            case MTC_2_POINT: strcpy(szText, "2 Point"); break;
                            default:
                                sprintf(szText, "undefined:%02xh", Event);
                                break;
                        }    
    
}
