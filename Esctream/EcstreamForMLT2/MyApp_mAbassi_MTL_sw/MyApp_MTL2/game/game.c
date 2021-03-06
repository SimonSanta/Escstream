#include <Const.h>
#include "game.h"
////
#include <string.h>

#include "mAbassi.h"          /* MUST include "SAL.H" and not uAbassi.h        */
#include "Platform.h"         /* Everything about the target platform is here  */
#include "HWinfo.h"           /* Everything about the target hardware is here  */
#include "SysCall.h"          /* System Call layer stuff     */

#include "arm_pl330.h"
#include "dw_i2c.h"
#include "cd_qspi.h"
#include "dw_sdmmc.h"
#include "dw_spi.h"
#include "dw_uart.h"
#include "alt_gpio.h"

#include "gui.h"

// #include "game.h"

int abs (int a) {
	if (a>0) { return a; }
	else { return -a; }
}

bool notcuty(LINE* line, PLAYER* player)
{
    if(line->interrupt==false){
        return true;
    }
    else{
        switch (line->dir){
        case UP:
            if(player->y+PLAYER_HEIGHT<line->stop_at)
                return false;
            else
                return true;
            break;
        case DOWN:
        	if(player->y>line->stop_at)
                return false;
        	else
        		return true;
            break;
        }
    }
}

bool notcutx(LINE* line,PLAYER* player){
    if(line->interrupt==false){
        return true;
    }
    else{
        switch (line->dir){
        case LEFT:
            if(player->x+PLAYER_WIDTH<line->stop_at)
                return false;
            else
                return true;
            break;
        case RIGHT:
        	if(player->x>line->stop_at)
                return false;
        	else
        		return true;
            break;
        }
    }
}

bool success(LVL* lvl){
    int nbr_succes=0;
    for(int i=0;i<lvl->nbr_players;i++){
      PLAYER* player=lvl->players+i;
        if((player->fin_x<player->x+PLAYER_WIDTH && player->fin_x>player->x) && (player->fin_y<player->y+PLAYER_HEIGHT && player->fin_y>player->y)){
        	nbr_succes++;
            printf("success - One player at final position\n");
        }
    }
    return nbr_succes==lvl->nbr_players;
}


void pos_correlator(LVL *lvl){
	// For each line of the level
    for(int j=0; j<lvl->nbr_lines; j++)
    {

        LINE* line = lvl->lines+j;
        bool no_interrupt[lvl->nbr_players]; //permet de remettre ? false line->interrupt quand il n'y a plus d'interrupt

        // For each player of the level
        for(int i=0; i<lvl->nbr_players; i++)
        {
            PLAYER* player = lvl->players+i;
            int lx = line->x;
            int px = player->x;
            int pxpw = player->x+PLAYER_WIDTH;
            int lxlw = line->x+line->width;

            no_interrupt[i] = true;

            if ((lx>px && lx<pxpw) ||( lxlw >px && lxlw<pxpw))
            {
            	// Player is cutting a line of same color
                if (line->color == player->color)
                {
                    if (notcuty(line,player))
                    {
                    	// Update defeat
                    	*flag = (*flag) | 0x00000002;
                    }
                }
                // Player is blocking a line of opposite color
                else
                {
                	// Update line informations
                    line->interrupt=true;
                    no_interrupt[i]=false;
                    if(line->dir==UP)
                    {
                        line->stop_at=player->y+PLAYER_HEIGHT;
                    }
                    else if(line->dir==DOWN)
                    {
                        line->stop_at=player->y;
                    }
                }
           }
           int ly=line->y;
           int lylh=line->y+line->height;
           int py=player->y;
           int pyph=player->y+PLAYER_HEIGHT;
           if((ly<pyph && ly>py) || (lylh>py && lylh <pyph ))
           {
        	    // Player is cutting a line of same color
                if(line->color==player->color)
                {
                    if(notcutx(line,player))
                    {
                    	// Update defeat
                    	*flag = (*flag) | 0x00000002;
                    }
                }
                // Player is blocking a line of opposite color
                else{
                	// Update line informations
                    line->interrupt=true;
                    no_interrupt[i]=false;
                    if(line->dir==LEFT){
                        line->stop_at=player->x+PLAYER_WIDTH;
                    }
                    else if(line->dir==RIGHT){
                        line->stop_at=player->x;
                    }
                }
            }
        }
        // No line interrupt
        if(no_interrupt[0] && no_interrupt[1]){
            line->interrupt=false;
        }
    }
}

IMAGE* initimage(const char *filename, int height, int width)
{
	IMAGE *img = malloc(sizeof(IMAGE));

	// Name
	img->name = malloc(strlen(filename)+1);
	strcpy(img->name, filename);

	// Dimension
	img->height = height;
    img->width = width;

    // Open file using file descriptor FdSrc
    img->FdSrc = open(img->name, O_RDONLY, 0777);
    if(img->FdSrc==-1)
    	printf("initimage - Error while opening image %s\n", img->name);

    // Buffer of length 3*height*width bytes
    img->g_Buffer = (char *) malloc(3*width*height*sizeof(char));	// Base address
    char *g_Buffer = img->g_Buffer;									// To navigate in g_Buffer

    // Size of a of a "row" (reading chunk) of the buffer
    img->rowsize = sizeof(char)*3*width;

    int j = 0;
    int Nrd;
    do {
    	// Read data from image into buffer, row by row (i.e. by chunks of 3*width bytes)
    	Nrd = read(img->FdSrc, g_Buffer, img->rowsize);
    	img->height = j+1;
    	if(Nrd==-1)
    		printf("initimage - Error while reading image %s\n", img->name);

    	// If image height shorter than expected
    	if(Nrd < img->rowsize) {
    		g_Buffer -= Nrd;
    		img->height=img->height-1;
    		break;
    	}

    	g_Buffer += img->rowsize; // Move ptr one row further
    	j++;
    } while (j<height);
    printf("initimage - Image fully %s stored in memory (%d rows)\n", img->name, img->height);

    // Close file
	int c = close(img->FdSrc);
	if(c==-1)
		printf("initimage - Error while closing image %s\n", img->name);

    return img;
}

void suppressimage(IMAGE* img)
{
	free(img->name);
	free(img->g_Buffer);
	free(img);
}

// offsetx, offsety the position wher to start plotting on myFrameBuffer
// startx, starty, endx, endy the chunk limits on the original image
void displayChunk(IMAGE *img, int offsetx, int offsety, int startx, int starty, int endx, int endy, VIP_FRAME_READER *pReader)
{
	uint32_t  *myFrameBuffer;
	int       i;

	myFrameBuffer = (uint32_t  *) VIPFR_GetDrawFrame(pReader);

	myFrameBuffer += 800*offsety; // en y
	myFrameBuffer += offsetx; 	  // en x

	char *g_Buffer = img->g_Buffer + startx*3 + starty*img->rowsize;	// To navigate in g_Buffer

	for(int j=0; j<(endy-starty); j++){
		i=0;
		while(i<3*(endx-startx)) {
			//if(!( *(g_Buffer+i)==0 && *(g_Buffer+i+1)==255 && *(g_Buffer+i+2)==0))
			if(!( *(g_Buffer+i)>=0 && *(g_Buffer+i)<=130 &&  *(g_Buffer+i+1)>=185 && *(g_Buffer+i+1)<=255 && *(g_Buffer+i+2)<=150 && *(g_Buffer+i+2)>=0))
				*myFrameBuffer = (*(g_Buffer+i) << 16) + (*(g_Buffer+i+1) << 8) + *(g_Buffer+i+2);
			myFrameBuffer++;
			i += 3;
		}
		g_Buffer += img->rowsize;
		myFrameBuffer += 800-(endx-startx);
	}
	//printf("displayimage - Image %s fully displayed\n", img->name);
}

void displayimage(IMAGE *img, int offsetx, int offsety, VIP_FRAME_READER * pReader)
{
	displayChunk(img, offsetx, offsety, 0, 0, img->width, img->height, pReader);
}
