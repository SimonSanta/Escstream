#include "terasic_includes.h"
#include "vip_fr.h"

void FrameReader_SetFrame0(alt_u32 *VipBase, alt_u32 FrameBase, alt_u32 words, alt_u32 cycle, alt_u32 width, alt_u32 height, alt_u32 interlace);
void FrameReader_SetFrame1(alt_u32 *VipBase, alt_u32 FrameBase, alt_u32 words, alt_u32 cycle, alt_u32 width, alt_u32 height, alt_u32 interlace);
void FrameReader_SelectFrame(alt_u32 *VipBase, alt_u32 FrameIndex);
void FrameReader_Go(alt_u32 *VipBase, bool bGO);


VIP_FRAME_READER* VIPFR_Init(alt_u32 *VipBase, void* Frame0_Base, void* Frame1_Base, alt_u32 Frame_Width, alt_u32 Frame_Height){

    VIP_FRAME_READER *p;
    
    p = malloc(sizeof(VIP_FRAME_READER));
    p->VipBase = VipBase;
    p->Frame0_Base = Frame0_Base;
    p->Frame1_Base = Frame1_Base;
    p->DisplayFrame = 0;
     
    p->bytes_per_pixel = 4;
    p->color_depth = 32;
    p->interlace = 0;
    
    FrameReader_Go(VipBase, FALSE); // stop for config
    
    VIPFR_SetFrameSize(p, Frame_Width, Frame_Height);
    
    FrameReader_SelectFrame(VipBase, p->DisplayFrame);
    //
    FrameReader_Go(VipBase, TRUE); // go
    
    return p;
}

void VIPFR_UnInit(VIP_FRAME_READER* p){
    if (p)
        free(p);
}

void VIPFR_SetFrameSize(VIP_FRAME_READER* p, int width, int height){
    alt_u32 words, cycle;
    words = width*height;
    cycle = width*height;
    
  //  p->Frame_Width = width;
  //  p->Frame_Height = height;
    //
    p->width = width;
    p->height = height;
    //
    FrameReader_SetFrame0(p->VipBase, (alt_u32)p->Frame0_Base, words, cycle, p->width, p->height, p->interlace);
    FrameReader_SetFrame1(p->VipBase, (alt_u32)p->Frame1_Base, words, cycle, p->width, p->height, p->interlace);
    
}

void VIPFR_Go(VIP_FRAME_READER* p, bool bGo){
	alt_write_word(p->VipBase + 0x00, bGo?0x01:0x00);
}

void* VIPFR_GetDrawFrame(VIP_FRAME_READER* p){
        if (p->DisplayFrame == 0)
            return p->Frame1_Base;
        return p->Frame0_Base;
}
void VIPFR_ActiveDrawFrame(VIP_FRAME_READER* p){
     p->DisplayFrame =  (p->DisplayFrame+1)%2;
     FrameReader_SelectFrame(p->VipBase, p->DisplayFrame);
// ********     alt_dcache_flush_all();
}

void DRAW_EraseScreen(VIP_FRAME_READER *p, alt_u32 Color){
    memset(VIPFR_GetDrawFrame(p), Color, p->width*p->height*p->bytes_per_pixel);
}

////////////////////////////////////////////////////////////////////
// internal function
////////////////////////////////////////////////////////////////////


void FrameReader_SetFrame0(alt_u32 *VipBase, alt_u32 FrameBase, alt_u32 words, alt_u32 cycle, alt_u32 width, alt_u32 height, alt_u32 interlace){
	alt_write_word(VipBase + 4, FrameBase); // frame0 base address
    alt_write_word(VipBase + 5, words); // frame0 words
    alt_write_word(VipBase + 6, cycle); // frame0 single cylce color pattern
    alt_write_word(VipBase + 8, width); // frame0 width
    alt_write_word(VipBase + 9, height); // frame0 height
    alt_write_word(VipBase + 10, interlace); // frame0 interlaced
}

void FrameReader_SetFrame1(alt_u32 *VipBase, alt_u32 FrameBase, alt_u32 words, alt_u32 cycle, alt_u32 width, alt_u32 height, alt_u32 interlace){
	alt_write_word(VipBase + 11, FrameBase); // frame0 base address
    alt_write_word(VipBase + 12, words); // frame0 words
    alt_write_word(VipBase + 13, cycle); // frame0 single cylce color pattern
    alt_write_word(VipBase + 15, width); // frame0 width
    alt_write_word(VipBase + 16, height); // frame0 height
    alt_write_word(VipBase + 17, interlace); // frame0 interlaced
}

void FrameReader_SelectFrame(alt_u32 *VipBase, alt_u32 FrameIndex){
	alt_write_word(VipBase + 3, FrameIndex);
}        

void FrameReader_Go(alt_u32 *VipBase, bool bGO){
	alt_write_word(VipBase + 0, bGO?0x01:0x00);
}



///////////////////////////////////////////////////////////////


void VIPFR_ReserveBackground(VIP_FRAME_READER* p){
    alt_u32 *pSrc, *pDes;
    int nSize;
    
    nSize = p->width * p->height * p->bytes_per_pixel;
        
    
    if (p->DisplayFrame == 0){
        pSrc = p->Frame0_Base;
        pDes = p->Frame1_Base;
    }else{
        pDes = p->Frame0_Base;
        pSrc = p->Frame1_Base;
    }
   
    memcpy(pDes, pSrc, nSize);
}


