#define SCREEN_WIDTH 640;
#define SCREEN_HEIGHT 480;
#define COLOR_DEPTH 32;
#define HORI true;
#define VERTI false;

typedef struct{
    int x;
    int y;
    int width;
    int height;
}player;

typedef struct{
    int x;
    int y;
    bool axe;
    int width;
    int height;
}line;
