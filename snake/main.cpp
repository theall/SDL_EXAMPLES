#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include <SDL/SDL.h>
#include <time.h>

#define TILE_GREENSTAR  1
#define TILE_YELLOWSTAR  2
#define TILE_REDSTAR  3

#define TILE_WIDTH 20
#define TILE_HEIGHT 15

#define GRID_SIZE 32

SDL_Surface *loadImage(char *path)
{
    SDL_Surface *r = SDL_LoadBMP(path);
    SDL_SetColorKey(r, SDL_SRCCOLORKEY, SDL_MapRGB(r->format, 255, 255, 255));
    return r;
}

enum Dir
{
    UP,
    LEFT,
    DOWN,
    RIGHT
};
typedef struct
{
    int x;
    int y;
    int sprite;
} Tile;

typedef struct
{
    int x;
    int y;
} Coordine;

Tile g_Tiles[TILE_HEIGHT][TILE_WIDTH];
Coordine g_snakeList[TILE_WIDTH*TILE_HEIGHT];
Coordine apples[10] = {0};
Dir g_nextDir;
int g_snakeLength;
float g_speed;
int g_frames;
bool g_died;
bool g_autoRun = true;
SDL_Surface *g_screen;
SDL_Surface *g_greenStar;
SDL_Surface *g_yellowStar;
SDL_Surface *g_redStar;

void resetTiles()
{
    int i,j;
    for(i=0; i<TILE_HEIGHT; i++)
    {
        for(j=0; j<TILE_WIDTH; j++)
        {
            if(i==0 || i==(TILE_HEIGHT-1) || j==0 || j==(TILE_WIDTH-1))
                g_Tiles[i][j].sprite = TILE_GREENSTAR;
            else
            {
                g_Tiles[i][j].sprite = 0;
            }
        }
    }
}

void reset()
{
    int tile_total = TILE_WIDTH*TILE_HEIGHT;

    int i;
    for(i=0; i<tile_total; i++)
    {
        g_snakeList[i].x = -1;
        g_snakeList[i].y = -1;
    }
    g_snakeList[0].x = 3;
    g_snakeList[0].y = 1;
    g_snakeList[1].x = 2;
    g_snakeList[1].y = 1;

    g_nextDir = DOWN;
    g_snakeLength = 2;
    g_speed = 6;
    g_frames = 0;
    g_died = false;
}

bool isApple(Coordine pos)
{
    int j;
    for(j=0; j<10; j++)
    {
        if(pos.x==apples[j].x && pos.y==apples[j].y)
        {
            return true;
        }
    }
    return false;
}

bool isSnakeBody(Coordine pos)
{
    int j;
    for(j=1; j<g_snakeLength; j++)
    {
        if(pos.x==g_snakeList[j].x && pos.y==g_snakeList[j].y)
        {
            return true;
        }
    }
    return false;
}

Coordine getNextTilePos()
{
    Coordine nextPos = g_snakeList[0];
    switch(g_nextDir)
    {
    case UP:
        nextPos.y--;
        break;
    case RIGHT:
        nextPos.x++;
        break;
    case LEFT:
        nextPos.x--;
        break;
    case DOWN:
        nextPos.y++;
        break;
    }
    return nextPos;
}

Dir generateNextDir()
{
    int i, j;

    int x = g_snakeList[0].x;
    int y = g_snakeList[0].y;
    Dir possibleDirs[4];
    int dirCount = 0;
    Coordine nextPos = getNextTilePos();
    bool isEmpty = g_Tiles[nextPos.y][nextPos.x].sprite==0;
    if(isEmpty)
    {
        int minDx = 99999;
        int minDy = 99999;
        for(i=0; i<10; i++)
        {
            int dx = abs(apples[i].x-g_snakeList[0].x);
            int dy = abs(apples[i].y-g_snakeList[0].y);
            if(abs(minDx)+abs(minDy)>dx+dy)
            {
                minDx = apples[i].x;
                minDy = apples[i].y;
            }
        }
        if(abs(minDx) > abs(minDy))
        {
            if(minDy>0)
            {
                if(g_nextDir!=UP)
                    g_nextDir = DOWN;
            }
            else
            {
                if(g_nextDir!=DOWN)
                    g_nextDir = UP;
            }
        }
        else
        {
            if(minDx>0)
            {
                if(g_nextDir!=LEFT)
                    g_nextDir = RIGHT;
            }
            else
            {
                if(g_nextDir!=RIGHT)
                    g_nextDir = LEFT;
            }
        }
    }
    else
    {
        Coordine t[4] = {{0,-1},{-1,0},{0,1},{1,0}};
        bool getApple = false;
        for(i=0; i<4; i++)
        {
            Coordine nextPossiblePos = g_snakeList[0];
            nextPossiblePos.x += t[i].x;
            nextPossiblePos.y += t[i].y;

            if(isApple(nextPossiblePos))
            {
                getApple = true;
                g_nextDir = (Dir)i;
                break;
            }
            if(!isSnakeBody(nextPossiblePos) && g_Tiles[nextPossiblePos.y][nextPossiblePos.x].sprite==0)
            {
                possibleDirs[dirCount] = (Dir)i;
                dirCount++;
            }
        }
        if(!getApple)
        {
            if(dirCount==0)
            {
                g_died = true;
            }
            else
            {
                g_nextDir = possibleDirs[rand()%dirCount];
            }
        }
    }
}

void update()
{
    resetTiles();

    // clear snake
    int i, j;
    for(i=0; i<g_snakeLength; i++)
    {
        g_Tiles[g_snakeList[i].y][g_snakeList[i].x].sprite = 0;
    }

    // check died
    g_died = g_snakeList[0].x==0 || g_snakeList[0].x==TILE_WIDTH-1 || g_snakeList[0].y==0 || g_snakeList[0].y==TILE_HEIGHT-1;
    if(!g_died)
    {
        for(i=1; i<g_snakeLength; i++)
        {
            if(g_snakeList[0].x==g_snakeList[i].x && g_snakeList[0].y==g_snakeList[i].y)
            {
                g_died = true;
                break;
            }
        }
    }
    for(i=0; i<10; i++)
    {
        if(apples[i].x==g_snakeList[0].x && apples[i].y==g_snakeList[0].y)
        {
            g_snakeLength++;
            apples[i].x = 0;
            apples[i].y = 0;
            SDL_Delay(100);
            //speed += 0.2;
            break;
        }
    }

    for(i=g_snakeLength-1; i>0; i--)
    {
        g_snakeList[i].x = g_snakeList[i-1].x;
        g_snakeList[i].y = g_snakeList[i-1].y;
    }

    // next direction
    switch(g_nextDir)
    {
    case UP:
        g_snakeList[0].y--;
        break;
    case RIGHT:
        g_snakeList[0].x++;
        break;
    case LEFT:
        g_snakeList[0].x--;
        break;
    case DOWN:
        g_snakeList[0].y++;
        break;
    }

    // place apple
    // first find all the empty tiles
    Coordine emptyTileList[TILE_HEIGHT*TILE_WIDTH];
    int emptyTileCount = 0;
    for(i=0; i<TILE_HEIGHT; i++)
    {
        int j;
        for(j=0; j<TILE_WIDTH; j++)
        {
            if(g_Tiles[i][j].sprite==0)
            {
                emptyTileList[emptyTileCount].x = j;
                emptyTileList[emptyTileCount].y = i;
                emptyTileCount++;
            }
        }
    }
    for(i=0; i<10; i++)
    {
        if(apples[i].x==0 && apples[i].y==0)
        {
            int index = rand() % emptyTileCount;
            apples[i] = emptyTileList[index];
        }
    }
    for(i=0; i<10; i++)
    {
        if(apples[i].x==0 && apples[i].y==0)
        {
            continue;
        }
        g_Tiles[apples[i].y][apples[i].x].sprite = TILE_YELLOWSTAR;
    }

    // update snake
    g_Tiles[g_snakeList[0].y][g_snakeList[0].x].sprite = TILE_REDSTAR;
    for(i=1; i<g_snakeLength; i++)
    {
        g_Tiles[g_snakeList[i].y][g_snakeList[i].x].sprite = TILE_YELLOWSTAR;

    }

    if(g_autoRun)
    {
        g_nextDir = generateNextDir();
    }
}

void render()
{
    int i, j;
    // clear screen
    SDL_FillRect(g_screen, 0, SDL_MapRGB(g_screen->format, 255, 255, 255));
    for(i=0; i<TILE_HEIGHT; i++)
    {
        for(j=0; j<TILE_WIDTH; j++)
        {
            SDL_Rect r;
            r.x = j*GRID_SIZE;
            r.y = i*GRID_SIZE;
            r.w = GRID_SIZE;
            r.h = GRID_SIZE;
            SDL_Surface *bmp = NULL;

            int sprite = g_Tiles[i][j].sprite;
            if(sprite==TILE_GREENSTAR)
            {
                bmp = g_greenStar;
            }
            else if(sprite==TILE_YELLOWSTAR)
            {
                bmp = g_yellowStar;
            }
            else if(sprite==TILE_REDSTAR)
            {
                bmp = g_redStar;
            }
            if(bmp)
            {
                SDL_BlitSurface(bmp, 0, g_screen, &r);
            }
        }
    }
}

int main ( int argc, char** argv )
{
    bool autoRun = true;
    if(argc>1 && strcmp(argv[1], "auto")==0)
    {
        autoRun = true;
    }
    // initialize SDL video
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return 1;
    }

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // create a new window
    g_screen = SDL_SetVideoMode(640, 480, 16,
                                           SDL_HWSURFACE|SDL_DOUBLEBUF);
    if ( !g_screen )
    {
        printf("Unable to set 640x480 video: %s\n", SDL_GetError());
        return 1;
    }

    // load an image
    g_greenStar = loadImage("greenstar.bmp");
    g_yellowStar = loadImage("yellowstar.bmp");
    g_redStar = loadImage("redstar.bmp");

    srand(time(0));
    reset();
    // program main loop
    bool done = false;
    int interval = 60 / g_speed;
    int i, j;
    while (!done)
    {
        // message processing loop
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // check for messages
            switch (event.type)
            {
            // exit if the window is closed
            case SDL_QUIT:
                done = true;
                break;

            case SDL_MOUSEBUTTONDOWN:
            {
                int tileX = event.button.x/GRID_SIZE;
                int tileY = event.button.y/GRID_SIZE;
                if(g_Tiles[tileY][tileX].sprite)
                {
                    g_Tiles[tileY][tileX].sprite = 0;
                }
                else
                {
                    apples[0].x = tileX;
                    apples[0].y = tileY;
                    //g_Tiles[tileX][tileY].sprite = TILE_GREENSTAR;
                }
                break;
            }
            // check for keypresses
            case SDL_KEYDOWN:
            {
                // exit if ESCAPE is pressed
                SDLKey key = event.key.keysym.sym;
                if (key == SDLK_ESCAPE)
                {
                    done = true;
                }
                else if(key == SDLK_1)
                {
                    g_Tiles[rand()%TILE_WIDTH][rand()%TILE_HEIGHT].sprite = TILE_GREENSTAR;
                }
                else if(key == SDLK_EQUALS)
                {
                    interval *= 2;
                }
                else if(key == SDLK_MINUS)
                {
                    interval /= 1.2;
                    if(interval==0)
                        interval = 1;
                }
                else if(key == SDLK_UP)
                {
                    if(g_nextDir!=UP && g_nextDir!=DOWN)
                        g_nextDir = UP;
                }
                else if(key == SDLK_LEFT)
                {
                    if(g_nextDir!=LEFT && g_nextDir!=RIGHT)
                        g_nextDir = LEFT;
                }
                else if(key == SDLK_RIGHT)
                {
                    if(g_nextDir!=LEFT && g_nextDir!=RIGHT)
                        g_nextDir = RIGHT;
                }
                else if(key == SDLK_DOWN)
                {
                    if(g_nextDir!=UP && g_nextDir!=DOWN)
                        g_nextDir = DOWN;
                }
                else if(key == SDLK_SPACE)
                {
                    autoRun = !autoRun;
                }

                break;
            }
            } // end switch
        } // end of message processing

        if(g_died)
        {
            reset();
        }
        else if(g_frames%interval==0)
        {
            update();
        }

        // DRAWING ENDS HERE
        render();

        // finally, update the screen :)
        SDL_Flip(g_screen);
        g_frames++;
        SDL_Delay(1000/60);
    } // end main loop

    return 0;
}
