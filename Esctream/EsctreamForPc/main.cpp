#ifdef __cplusplus
    #include <cstdlib>
#else
    #include <stdlib.h>
#endif
#include <stdbool.h>
#include <SDL/SDL.h>
#include <Const.h>
#include <level.c>
//block la fonction en attendant qu'on quite l'ecran
void pause()
{
    int continuer = 1;
    SDL_Event event;
    while (continuer)
    {
        SDL_WaitEvent(&event);
        switch(event.type)
        {
            case SDL_QUIT:
                continuer = 0;
            case SDL_KEYDOWN:
                continuer = 0;

        }
    }
}
//return true si on a gagne, false otherwise
bool success(LVL* lvl){
    int nbr_succes=0;
    for(int i=0;i<lvl->nbr_players;i++){
      PLAYER* player=lvl->players+i;
        if((player->fin_x<player->x+PLAYER_WIDTH && player->fin_x>player->x) && (player->fin_y<player->y+PLAYER_HEIGHT && player->fin_y>player->y)){
        nbr_succes++;
            printf("VICTORY condition\n");
        }
    }
    return nbr_succes==lvl->nbr_players;
}

//return true si le joueur se trouve sur la partie non-coupee d'une ligne verticale
bool notcuty(LINE* line,PLAYER* player){
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
        case DOWN: //to complete !!!!!!!!!!!!!!!!!!!!!
                return false;
            break;
        }
    }
}

//return true si le joueur se trouve sur la partie non-coupee d'une ligne horizontale
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
                return false;
            break;
        }
    }
}

//analyse la position des players par rapport aux lines, en cas de collision declare une defaite ou declare une ligne comme interompue
void pos_correlator(LVL* lvl){
    for(int j=0;j<lvl->nbr_lines;j++){
        LINE* line=lvl->lines+j;
        bool no_interrupt[lvl->nbr_players];//permet de remettre à false line->interrupt quand il n'y a plus d'interrupt
        for(int i=0;i<lvl->nbr_players;i++){
            PLAYER* player=lvl->players+i;
            int lx=line->x;
            int px=player->x;
            int pxpw=player->x+PLAYER_WIDTH;
            int lxlw=line->x+line->width;
            no_interrupt[i]=true;
           if((lx>px && lx<pxpw) ||( lxlw >px && lxlw<pxpw)){
                if(line->color==player->color){
                    if(notcuty(line,player)){
                        defeat=true;
                    }
                }
                else{
                    line->interrupt=true;
                    no_interrupt[i]=false;
                    if(line->dir==UP){
                        line->stop_at=player->y+PLAYER_HEIGHT;
                    }
                    else if(line->dir==DOWN){
                        line->stop_at=player->y;
                    }
                }
           }
           int ly=line->y;
           int lylh=line->y+line->height;
           int py=player->y;
           int pyph=player->y+PLAYER_HEIGHT;
           if((ly<pyph && ly>py) || (lylh>py && lylh <pyph )){
                if(line->color==player->color){
                    if(notcutx(line,player)){
                        defeat=true;
                    }
                }
                else{
                    line->interrupt=true;
                    no_interrupt[i]=false;
                    if(line->dir==LEFT){
                        line->stop_at=player->x+PLAYER_WIDTH;
                    }
                    else if(line->dir==RIGHT){
                        line->stop_at=player->x+PLAYER_HEIGHT;
                    }
                }
            }
        printf("line->interrupt %d\n",line->interrupt);
        }
        if(no_interrupt[0] && no_interrupt[1]){
            line->interrupt=false;
            printf("(no_interrupt[0] && no_interrupt[1])=true\n");
        }
    }
}
void change_pos(PLAYER* player,int dest_x,int dest_y){
    if(dest_x>player->x)
        player->x++;
    else
        player->x--;
    if(dest_y>player->y)
        player->y++;
    else
        player->y--;
}

void interrupt_modif(LVL* lvl,SDL_Rect Lines_Rect[],SDL_Surface *Lines_Surfaces[],SDL_Surface *ecran){
    for(int i=0; i<lvl->nbr_lines; i++){
            LINE* line=lvl->lines+i;
            printf("interrupt_modif1 line->interrupt %d\n",line->interrupt);
            if(line->interrupt==true){
                printf("interrupt_modif2\n");
                switch (line->dir){
                    case UP:
                        Lines_Rect[i].y=line->stop_at;
                        break;
                    case DOWN:
                        Lines_Rect[i].y=line->stop_at-line->height;
                        break;
                    case LEFT:
                        Lines_Rect[i].x=line->stop_at;
                        break;
                    case RIGHT:
                        Lines_Rect[i].x=line->stop_at;
                        break;
                }
            }
            else{
                        Lines_Rect[i].y=line->y;
                        Lines_Rect[i].x=line->x;
            }
            SDL_BlitSurface(Lines_Surfaces[i], NULL, ecran, &Lines_Rect[i]); // Collage de la surface sur l'écran
        }
}

bool play_lvl(int niv,SDL_Surface* ecran){
    Uint32 white_val=SDL_MapRGB(ecran->format, 255, 255, 255);
    Uint32 black_val=SDL_MapRGB(ecran->format, 0, 0, 0);
    bool QUIT=false;
    LVL *lvl=level(niv);
    PLAYER* player1= lvl->players;
    PLAYER* player2= lvl->players+1;
    SDL_Surface *Lines_Surfaces[lvl->nbr_lines];
    SDL_Surface *Players_Surfaces[lvl->nbr_players];
    SDL_Surface *Dest[lvl->nbr_players];
    SDL_Rect Lines_Rect[lvl->nbr_lines];
    SDL_Rect Players_Rect[lvl->nbr_players];
    SDL_Rect R_Dest[lvl->nbr_players];

    // Allocation de la surface
    for(int i=0;i<lvl->nbr_lines;i++)
    {
            LINE* line=lvl->lines+i;
            Lines_Surfaces[i] = SDL_CreateRGBSurface(SDL_HWSURFACE, line->width, line->height, COLOR_DEPTH, 0, 0, 0, 0);
            Lines_Rect[i].x=line->x;
            Lines_Rect[i].y=line->y;
            if(line->color==WHITE){
                    SDL_FillRect(Lines_Surfaces[i], NULL, white_val);}
            else
                    SDL_FillRect(Lines_Surfaces[i], NULL, black_val);
            SDL_BlitSurface(Lines_Surfaces[i], NULL, ecran, &Lines_Rect[i]); // Collage de la surface sur l'écra
    }
    for(int j=0;j<lvl->nbr_players;j++)
    {
            PLAYER* player=lvl->players+j;
            Players_Surfaces[j] = SDL_CreateRGBSurface(SDL_HWSURFACE, PLAYER_WIDTH, PLAYER_HEIGHT, COLOR_DEPTH, 0, 0, 0, 0);
            Players_Rect[j].x=player->start_x;
            Players_Rect[j].y=player->start_y;
            Dest[j] = SDL_CreateRGBSurface(SDL_HWSURFACE, 10, 10, COLOR_DEPTH, 0, 0, 0, 0);
            R_Dest[j].x = player->fin_x;
            R_Dest[j].y = player->fin_y;
            if(player->color==WHITE){
                    SDL_FillRect(Players_Surfaces[j], NULL, white_val);
                    SDL_FillRect(Dest[j], NULL, white_val);
            }
            else{
                    SDL_FillRect(Players_Surfaces[j], NULL, black_val);
                    SDL_FillRect(Dest[j], NULL, black_val);
            }
            SDL_BlitSurface(Players_Surfaces[j], NULL, ecran, &Players_Rect[j]); // Collage de la surface sur l'écran*/
            SDL_BlitSurface(Dest[j], NULL, ecran, &R_Dest[j]); // Collage de la dest sur l'écran*/
            printf("address player%d: %d\n",j,player);
            printf("player:%d player->x:%d player->start_x:%d player->y:%d player->start_y:%d player->color:%d\r\n",j,player->x,player->start_x,player->y,player->start_y,player->color);
    }
    SDL_Flip(ecran); // Mise à jour de l'écran
    bool GO=true;
    SDL_Event event;
    while(GO)
    {
        SDL_WaitEvent(&event);
        switch(event.type){
            case SDL_QUIT:
                QUIT=true;
                GO=0;
                break;
           /* case SDL_MOUSEMOTION: //modif la position du joueur par rapport à la position de la souris
                change_pos(player1,event.motion.x,event.motion.y);
                Players_Rect[0].x=player1->x;
                Players_Rect[0].y=player1->y;
                break;*/
            case SDL_KEYDOWN://modif de la position du joueur
                switch(event.key.keysym.sym){
                    case SDLK_UP:
                        player1->y=player1->y-10;
                        Players_Rect[0].y=player1->y;
                        break;
                    case SDLK_DOWN:
                        player1->y=player1->y+10;
                        Players_Rect[0].y=player1->y;
                        break;
                    case SDLK_RIGHT:
                        player1->x=player1->x+10;
                        Players_Rect[0].x=player1->x;
                        break;
                    case SDLK_LEFT:
                        player1->x=player1->x-10;
                        Players_Rect[0].x=player1->x;
                        break;

                    case SDLK_w:
                        player2->y=player2->y-10;
                        Players_Rect[1].y=player2->y;
                        break;
                    case SDLK_s:
                        player2->y=player2->y+10;
                        Players_Rect[1].y=player2->y;
                        break;
                    case SDLK_d:
                        player2->x=player2->x+10;
                        Players_Rect[1].x=player2->x;
                        break;
                    case SDLK_a:
                        player2->x=player2->x-10;
                        Players_Rect[1].x=player2->x;
                        break;
                }
        }
        SDL_FillRect(ecran, NULL, SDL_MapRGB(ecran->format, 17, 206, 112));//réécriture du foncd
        for(int i=0;i<lvl->nbr_players;i++){
            SDL_BlitSurface(Players_Surfaces[i], NULL, ecran, &Players_Rect[i]); //plot le player
            SDL_BlitSurface(Dest[i], NULL, ecran, &R_Dest[i]);
        }
        pos_correlator(lvl);//detect les collisions
        interrupt_modif(lvl, Lines_Rect,Lines_Surfaces,ecran);//modifie si collision + replot les lines
        victory=success(lvl);//determine si il y a victoire
        SDL_Flip(ecran);//mise à jour de l'ecran
        GO=((!victory)&&(!defeat)&&(!QUIT));//arret si defaite ou victoire ou quit
    }
    free_lvl(lvl);
    if(victory)
        printf("VICTORY\n");
    if(defeat)
        printf("DEFEAT\n");
    return !QUIT;
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Surface *ecran = NULL;
    ecran = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_DEPTH, SDL_HWSURFACE);
    SDL_WM_SetCaption("DERU", NULL);
    SDL_FillRect(ecran, NULL, SDL_MapRGB(ecran->format, 17, 206, 112));
    bool GO=true;
    while(GO){
        defeat=false;
        victory=false;
        //int level= lvl_selection();
        GO=play_lvl(1,ecran);
        printf("GO=%d\n",GO);
    }
    SDL_Quit();
    return EXIT_SUCCESS;
}
