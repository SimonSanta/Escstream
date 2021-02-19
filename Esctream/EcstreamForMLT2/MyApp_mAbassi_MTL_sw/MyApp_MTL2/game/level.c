#include <Const.h>
#include <stdlib.h>

void new_player(LVL* level,int offset, int start_x, int start_y, int fin_x, int fin_y, int color){
    PLAYER* player=level->players+offset;
        player->start_x=start_x;
        player->start_y=start_y;
        player->fin_x=fin_x;
        player->fin_y=fin_y;
        player->color=color;
        player->x=start_x;
        player->y=start_y;
}

void new_line(LVL* level,int offset,int x, int y, int width, int height, DIR dir, int color){
    LINE* line1 =level->lines+offset;
        line1->x=x;
        line1->y=y;
        line1->width=width;
        line1->height=height;
        line1->color=color;
        line1->interrupt=false;
        line1->dir=dir;
        line1->stop_at=0;
}
void free_lvl(LVL* lvl){
	if ((lvl->lines != NULL) && (lvl->players != NULL)) {
		free(lvl->lines);
		free(lvl->players);
	}
}

void reset_lvl(LVL* lvl){
	for(int i=0;i<lvl->nbr_lines;i++){
		LINE* line = lvl->lines+i;
		line->interrupt=false;
	}
	for(int i=0;i<lvl->nbr_players;i++){
		PLAYER* player = lvl->players+i;
		player->x=player->start_x;
		player->y=player->start_y;
	}
}

void change_lvl(LVL* lvl,int lvl_num){
	//free_lvl(lvl);
	level(lvl_num,lvl);
	printf("change lvl");
}

void level(int lvl_number, LVL *lvl){
    switch(lvl_number){
    // FARWEST (three vertical lines)
        case 1:{
            lvl->nbr_players=2;
            lvl->nbr_lines=3;

            new_line(lvl,0,200,DRAW_BORDER,39,DRAWRECT_HEIGHT,UP,WHITE);
            new_line(lvl,1,400,DRAW_BORDER,39,DRAWRECT_HEIGHT,DOWN,BLACK);
            new_line(lvl,2,600,DRAW_BORDER,39,DRAWRECT_HEIGHT,UP,BLACK);
            new_player(lvl,0,60,100,700,100,BLACK);
            new_player(lvl,1,60,340,700,340,WHITE);
         break;
        }
        // BATMAN
        case 2:{
            lvl->nbr_players=2;
            lvl->nbr_lines=2;

            new_line(lvl,0,481,0,39,DRAWRECT_HEIGHT,UP,BLACK);
            new_line(lvl,1,0,300,DRAWRECT_WIDTH,39,LEFT,WHITE);
            new_player(lvl,0,5,247,721,360,BLACK);
            new_player(lvl,1,408,405,40,164,WHITE);
        break;
        }
        case 3:{
           /* lvl->nbr_players=2;
			lvl->nbr_lines=6;

			new_line(lvl,0,150,DRAW_BORDER,20,DRAWRECT_HEIGHT,UP,BLACK);
			new_line(lvl,1,380,DRAW_BORDER,20,DRAWRECT_HEIGHT,DOWN,BLACK);
			new_line(lvl,2,600,DRAW_BORDER,20,DRAWRECT_HEIGHT,UP,BLACK);
			new_line(lvl,3,DRAW_BORDER,100,DRAWRECT_WIDTH,20,RIGHT,WHITE);
			new_line(lvl,4,DRAW_BORDER,240,DRAWRECT_WIDTH,20,LEFT,WHITE);
			new_line(lvl,5,DRAW_BORDER,350,DRAWRECT_WIDTH,20,RIGHT,WHITE);
			new_player(lvl,0,450,180,50,25,BLACK);
			new_player(lvl,1,250,300,650,400,WHITE);*/
        	lvl->nbr_players=2;
			lvl->nbr_lines=4;

			new_line(lvl,0,600,DRAW_BORDER,30,DRAWRECT_HEIGHT,UP,WHITE);
			new_line(lvl,1,DRAW_BORDER,180,DRAWRECT_WIDTH,30,RIGHT,WHITE);
			new_line(lvl,2,480,DRAW_BORDER,30,DRAWRECT_HEIGHT,DOWN,BLACK);
			new_line(lvl,3,DRAW_BORDER,100,DRAWRECT_WIDTH,30,LEFT,BLACK);

			new_player(lvl,0,30,300,700,30,WHITE);
			new_player(lvl,1,50,350,750,60,BLACK);
        break;
        }
        case 4:{
        	/*lvl->nbr_players=2;
        	lvl->nbr_lines=8;

        	new_line(lvl,0,60,DRAW_BORDER,20,DRAWRECT_HEIGHT,UP,BLACK);
        	new_line(lvl,1,155,DRAW_BORDER,20,DRAWRECT_HEIGHT,UP,BLACK);
        	new_line(lvl,2,625,DRAW_BORDER,20,DRAWRECT_HEIGHT,UP,BLACK);
        	new_line(lvl,3,700,DRAW_BORDER,20,DRAWRECT_HEIGHT,UP,BLACK);
        	new_line(lvl,4,DRAW_BORDER,60,DRAWRECT_WIDTH,20,LEFT,WHITE);
        	new_line(lvl,5,DRAW_BORDER,140,DRAWRECT_WIDTH,20,LEFT,WHITE);
        	new_line(lvl,6,DRAW_BORDER,300,DRAWRECT_WIDTH,20,LEFT,WHITE);
        	new_line(lvl,7,DRAW_BORDER,380,DRAWRECT_WIDTH,20,LEFT,WHITE);

        	new_player(lvl,0,90,10,60+20+75+20+450+20+30,60+20+30,BLACK);
        	new_player(lvl,1,60+20+75+20+450+20+75+20+10,4*60+4*20+10,60+20+40,60+20+30,WHITE);*/
        	lvl->nbr_players=2;
			lvl->nbr_lines=6;

			new_line(lvl,0,150,DRAW_BORDER,20,DRAWRECT_HEIGHT,UP,BLACK);
			new_line(lvl,1,380,DRAW_BORDER,20,DRAWRECT_HEIGHT,DOWN,BLACK);
			new_line(lvl,2,600,DRAW_BORDER,20,DRAWRECT_HEIGHT,UP,BLACK);
			new_line(lvl,3,DRAW_BORDER,100,DRAWRECT_WIDTH,20,RIGHT,WHITE);
			new_line(lvl,4,DRAW_BORDER,240,DRAWRECT_WIDTH,20,LEFT,WHITE);
			new_line(lvl,5,DRAW_BORDER,350,DRAWRECT_WIDTH,20,RIGHT,WHITE);
			new_player(lvl,0,450,180,50,25,BLACK);
			new_player(lvl,1,250,300,650,400,WHITE);
        break;
        }
        case 5:{
        	/*lvl->nbr_players=2;
        	lvl->nbr_lines=4;

        	new_line(lvl,0,560,DRAW_BORDER,39,DRAWRECT_HEIGHT,UP,BLACK);
        	new_line(lvl,1,DRAW_BORDER,250,DRAWRECT_WIDTH,39,RIGHT,WHITE);
        	new_line(lvl,2,200,DRAW_BORDER,39,DRAWRECT_HEIGHT,DOWN,WHITE);
        	new_line(lvl,3,DRAW_BORDER,320,DRAWRECT_WIDTH,39,RIGHT,BLACK);

        	new_player(lvl,0,100,390,50,380,WHITE);
        	new_player(lvl,1,100,50,660,390,BLACK);*/
        	lvl->nbr_players=2;
			lvl->nbr_lines=8;

			new_line(lvl,0,60,DRAW_BORDER,20,DRAWRECT_HEIGHT,UP,BLACK);
			new_line(lvl,1,155,DRAW_BORDER,20,DRAWRECT_HEIGHT,UP,BLACK);
			new_line(lvl,2,625,DRAW_BORDER,20,DRAWRECT_HEIGHT,UP,BLACK);
			new_line(lvl,3,700,DRAW_BORDER,20,DRAWRECT_HEIGHT,UP,BLACK);
			new_line(lvl,4,DRAW_BORDER,60,DRAWRECT_WIDTH,20,LEFT,WHITE);
			new_line(lvl,5,DRAW_BORDER,140,DRAWRECT_WIDTH,20,LEFT,WHITE);
			new_line(lvl,6,DRAW_BORDER,300,DRAWRECT_WIDTH,20,LEFT,WHITE);
			new_line(lvl,7,DRAW_BORDER,380,DRAWRECT_WIDTH,20,LEFT,WHITE);

			new_player(lvl,0,90,10,60+20+75+20+450+20+30,60+20+30,BLACK);
			new_player(lvl,1,60+20+75+20+450+20+75+20+10,4*60+4*20+10,60+20+40,60+20+30,WHITE);

        break;
        }
        case 6:{
        	/*lvl->nbr_players=2;
        	lvl->nbr_lines=4;

        	new_line(lvl,0,600,DRAW_BORDER,30,DRAWRECT_HEIGHT,UP,WHITE);
        	new_line(lvl,1,DRAW_BORDER,180,DRAWRECT_WIDTH,30,RIGHT,WHITE);
        	new_line(lvl,2,480,DRAW_BORDER,30,DRAWRECT_HEIGHT,DOWN,BLACK);
        	new_line(lvl,3,DRAW_BORDER,100,DRAWRECT_WIDTH,30,LEFT,BLACK);

        	new_player(lvl,0,30,300,700,30,WHITE);
        	new_player(lvl,1,50,350,750,60,BLACK);*/
        	lvl->nbr_players=2;
			lvl->nbr_lines=4;

			new_line(lvl,0,560,DRAW_BORDER,39,DRAWRECT_HEIGHT,UP,BLACK);
			new_line(lvl,1,DRAW_BORDER,250,DRAWRECT_WIDTH,39,RIGHT,WHITE);
			new_line(lvl,2,200,DRAW_BORDER,39,DRAWRECT_HEIGHT,DOWN,WHITE);
			new_line(lvl,3,DRAW_BORDER,320,DRAWRECT_WIDTH,39,RIGHT,BLACK);

			new_player(lvl,0,100,390,50,380,WHITE);
			new_player(lvl,1,100,50,660,390,BLACK);

        break;
        }
        default:
        break;
    }
}
