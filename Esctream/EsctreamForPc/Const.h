#ifndef CONST_H_INCLUDED
#define CONST_H_INCLUDED
#include <stdbool.h>
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define COLOR_DEPTH 32

#define WHITE true
#define BLACK false
#define PLAYER_WIDTH 40
#define PLAYER_HEIGHT 40

#define HORI true
#define VERTI false

bool defeat;
bool victory;

typedef enum{UP,DOWN,LEFT,RIGHT}DIR;

typedef struct{
    int start_x;
    int start_y;
    int fin_x;
    int fin_y;
    bool color;
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
    bool color;
}LINE;

typedef struct{
    int nbr_players;
    int nbr_lines;
    LINE* lines;
    PLAYER* players;
}LVL;


#endif // CONST_H_INCLUDED
