#ifndef LEVEL_H_INCLUDED
#define LEVEL_H_INCLUDED
void level(int lvl_number, LVL *lvl);
void endLevel(LVL* level);
int test();
void reset_lvl(LVL* lvl);
void change_lvl(LVL* lvl,int lvl_num);
void copy_lvl(LVL* lvl,LVL* lvl2);

#endif // LEVEL_H_INCLUDED
