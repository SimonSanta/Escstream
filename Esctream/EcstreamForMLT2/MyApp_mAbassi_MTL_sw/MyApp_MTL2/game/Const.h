#ifndef CONST_H_INCLUDED
#define CONST_H_INCLUDED
#include <stdbool.h>
#include "mAbassi.h"          /* MUST include "SAL.H" and not uAbassi.h        */
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define COLOR_DEPTH 32

#define WHITE 0xFFFFFF
#define BLACK 0x000000
#define PLAYER_WIDTH 40
#define PLAYER_HEIGHT 40

#define DRAWRECT_WIDTH	SCREEN_WIDTH
#define DRAWRECT_HEIGHT	SCREEN_HEIGHT

#define DRAW_BORDER	 0
#define MENU_HEIGHT	 40

#define HORI true
#define VERTI false

typedef enum{UP,DOWN,LEFT,RIGHT}DIR;

typedef struct{
    int start_x;
    int start_y;
    int fin_x;
    int fin_y;
    int color;
    int x;
    int y;
}PLAYER;

typedef struct{
    int x;
    int y;
    int width;
    int height;
    bool interrupt;
    int stop_at;
    DIR dir;
    int color;
}LINE;

typedef struct{
    int nbr_players;
    int nbr_lines;
    LINE* lines;
    PLAYER* players;
}LVL;

typedef struct{
	char *name;
	int width;
	int height;
	int FdSrc;		// File descriptor of image
	char *g_Buffer;	// Base address of read buffer (unchanged)
	size_t rowsize;	// Size of a reading chunk ("row") - 3*width bytes
}IMAGE;

#endif // CONST_H_INCLUDED
