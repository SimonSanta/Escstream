/*
 * game.h
 *
 *  Created on: Mar 8, 2019
 *      Author: Noemie2
 */

#ifndef GAME_GAME_H_
#define GAME_GAME_H_
#include "Const.h"
#include "vip_fr.h"

int abs(int a);
void pos_correlator(LVL* lvl);
bool success(LVL* lvl);
extern int *flag;

IMAGE* initimage(const char *filename, int height, int width);
void suppressimage(IMAGE* img);
void displayimage(IMAGE *img, int offsetx, int offsety, VIP_FRAME_READER * pReader);
void displayChunk(IMAGE *img, int offsetx, int offsety, int startx, int starty, int endx, int endy, VIP_FRAME_READER *pReader);

#endif /* GAME_GAME_H_ */
