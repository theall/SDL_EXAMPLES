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

#define TILE_COLUMNS 10
#define TILE_ROWS 10

#define GRID_SIZE 32
#define APPLE_COUNT 1

#define DISTANCE_STUB      9999
#define DISTANCE_RIGID     10000

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
} Coordine;

typedef struct
{
    Coordine pos;
    int distance;
    Dir relativeDir;
}CoordineStep;

int g_Tiles[TILE_ROWS][TILE_COLUMNS];
Coordine g_snakeList[TILE_COLUMNS*TILE_ROWS];
Coordine g_apples[APPLE_COUNT] = {0};
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


int getDistance(Coordine c1, Coordine c2)
{
    return abs(c1.x-c2.x) + abs(c1.y-c2.y);
}

Coordine getNextCoorFromDir(Coordine currentPos, Dir dir)
{
    switch(dir)
    {
    case UP:
        currentPos.y--;
        break;
    case RIGHT:
        currentPos.x++;
        break;
    case LEFT:
        currentPos.x--;
        break;
    case DOWN:
        currentPos.y++;
        break;
    }
    return currentPos;
}

Coordine getNextTilePos()
{
    return getNextCoorFromDir(g_snakeList[0], g_nextDir);
}

void resetTiles()
{
    int i,j;
    for(i=0; i<TILE_ROWS; i++)
    {
        for(j=0; j<TILE_COLUMNS; j++)
        {
            if(i==0 || i==(TILE_ROWS-1) || j==0 || j==(TILE_COLUMNS-1))
                g_Tiles[i][j] = TILE_GREENSTAR;
            else
            {
                g_Tiles[i][j] = 0;
            }
        }
    }
}

int removeFirst(Coordine a[], int arraySize)
{
    int i;
    for(i=1; i<arraySize; i++)
    {
        a[i-1] = a[i];
    }
    return arraySize-1;
}

void fillTileDistanceFromPos(int distance[TILE_ROWS][TILE_COLUMNS], Coordine pos)
{
    int i,j;
    for(i=0; i<TILE_ROWS; i++)
    {
        for(j=0; j<TILE_COLUMNS; j++)
        {
            if(i==0 || i==(TILE_ROWS-1) || j==0 || j==(TILE_COLUMNS-1))
                distance[i][j] = DISTANCE_RIGID;
            else
            {
                distance[i][j] = DISTANCE_STUB;
            }
        }
    }
    for(i=0; i<g_snakeLength; i++)
    {
        distance[g_snakeList[i].y][g_snakeList[i].x] = DISTANCE_RIGID;
    }
    distance[pos.y][pos.x] = 0;
    Coordine pointList[TILE_ROWS*TILE_COLUMNS];
    pointList[0] = pos;
    int remainPointsCount = 1;
    while(remainPointsCount)
    {
        Coordine frontPos = pointList[0];
        remainPointsCount = removeFirst(pointList, remainPointsCount);
        for(i=0; i<4; i++)
        {
            Coordine nextPos = getNextCoorFromDir(frontPos, (Dir)i);
            if(distance[nextPos.y][nextPos.x] == DISTANCE_STUB)
            {
                // Is empty tile
                distance[nextPos.y][nextPos.x] = distance[frontPos.y][frontPos.x] + 1;
                pointList[remainPointsCount++] = nextPos;
            }
        }

    }
}

void reset()
{
    int tile_total = TILE_COLUMNS*TILE_ROWS;

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
    for(j=0; j<APPLE_COUNT; j++)
    {
        if(pos.x==g_apples[j].x && pos.y==g_apples[j].y)
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

Dir coordineToDir(Coordine d)
{
    Dir i;
    if(d.x==0)
    {
        i = d.y>0?DOWN:UP;
    } else {
        i = d.x>0?RIGHT:LEFT;
    }
    return i;
}

int getNextReachablePosList(Coordine source, Coordine destination, CoordineStep cs[])
{
    int distance[TILE_ROWS][TILE_COLUMNS];
    fillTileDistanceFromPos(distance, destination);
    int i;
    int index = 0;
    for(i=0;i<4;i++)
    {
        Coordine nextPos = getNextCoorFromDir(source, (Dir)i);
        int step = distance[nextPos.y][nextPos.x];
        if(step>=0 && step<DISTANCE_STUB)
        {
            cs[index].distance = step;
            cs[index].pos = nextPos;
            cs[index].relativeDir = (Dir)i;
            index++;
        }
    }
    return index;
}

bool isReachable(Coordine source, Coordine destination)
{
    CoordineStep cs[4];
    int posCount = getNextReachablePosList(source, destination, cs);
    return posCount>0;
}

void generateNextDir()
{
    CoordineStep cs[4];
    int posSize = getNextReachablePosList(g_snakeList[0], g_apples[0], cs);
    if(posSize > 0)
    {
        int i;
        int m = cs[0].distance;
        Dir findDir = cs[0].relativeDir;
        for(i=1;i<posSize;i++)
        {
            if(m>cs[i].distance)
            {
                m = cs[i].distance;
                findDir = cs[i].relativeDir;
            }
        }
        g_nextDir = findDir;
    } else {
        // Currently no path found
    }
}

void update()
{
    resetTiles();

    // clear snake
    int i;
    for(i=0; i<g_snakeLength; i++)
    {
        g_Tiles[g_snakeList[i].y][g_snakeList[i].x] = 0;
    }

    for(i=0; i<APPLE_COUNT; i++)
    {
        if(g_apples[i].x==g_snakeList[0].x && g_apples[i].y==g_snakeList[0].y)
        {
            g_snakeLength++;
            g_apples[i].x = 0;
            g_apples[i].y = 0;
            SDL_Delay(100);
            //speed += 0.2;
            break;
        }
    }

    Coordine nextPos = getNextTilePos();
    if(g_Tiles[nextPos.y][nextPos.x] == TILE_GREENSTAR || isSnakeBody(nextPos))
    {
        g_died = true;
        return;
    }
    for(i=g_snakeLength-1; i>0; i--)
    {
        g_snakeList[i].x = g_snakeList[i-1].x;
        g_snakeList[i].y = g_snakeList[i-1].y;
    }

    // next direction
    g_snakeList[0] = nextPos;

    // place apple
    // first find all the empty tiles
    Coordine emptyTileList[TILE_ROWS*TILE_COLUMNS];
    int emptyTileCount = 0;
    for(i=0; i<TILE_ROWS; i++)
    {
        int j;
        for(j=0; j<TILE_COLUMNS; j++)
        {
            if(g_Tiles[i][j]==0)
            {
                emptyTileList[emptyTileCount].x = j;
                emptyTileList[emptyTileCount].y = i;
                emptyTileCount++;
            }
        }
    }
    for(i=0; i<APPLE_COUNT; i++)
    {
        if(g_apples[i].x==0 && g_apples[i].y==0)
        {
            int index = rand() % emptyTileCount;
            g_apples[i] = emptyTileList[index];
        }
    }
    for(i=0; i<APPLE_COUNT; i++)
    {
        if(g_apples[i].x==0 && g_apples[i].y==0)
        {
            continue;
        }
        g_Tiles[g_apples[i].y][g_apples[i].x] = TILE_YELLOWSTAR;
    }

    // update snake
    g_Tiles[g_snakeList[0].y][g_snakeList[0].x] = TILE_REDSTAR;
    for(i=1; i<g_snakeLength; i++)
    {
        g_Tiles[g_snakeList[i].y][g_snakeList[i].x] = TILE_YELLOWSTAR;

    }

    if(g_autoRun)
    {
        generateNextDir();
    }
}

void render()
{
    int i, j;
    // clear screen
    SDL_FillRect(g_screen, 0, SDL_MapRGB(g_screen->format, 255, 255, 255));
    for(i=0; i<TILE_ROWS; i++)
    {
        for(j=0; j<TILE_COLUMNS; j++)
        {
            SDL_Rect r;
            r.x = j*GRID_SIZE;
            r.y = i*GRID_SIZE;
            r.w = GRID_SIZE;
            r.h = GRID_SIZE;
            SDL_Surface *bmp = NULL;

            int sprite = g_Tiles[i][j];
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
    if(argc>1 && strcmp(argv[1], "auto")==0)
    {
        g_autoRun = true;
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
    g_screen = SDL_SetVideoMode(GRID_SIZE*TILE_COLUMNS, GRID_SIZE*TILE_ROWS, 16,
                                SDL_HWSURFACE|SDL_DOUBLEBUF);
    if ( !g_screen )
    {
        printf("Unable to set 640x480 video: %s\n", SDL_GetError());
        return 1;
    }

    // load an image
    g_greenStar = loadImage((char*)"greenstar.bmp");
    g_yellowStar = loadImage((char*)"yellowstar.bmp");
    g_redStar = loadImage((char*)"redstar.bmp");

    srand(time(0));
    reset();
    // program main loop
    bool done = false;
    int interval = 60 / g_speed;
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
                if(g_Tiles[tileY][tileX])
                {
                    g_Tiles[tileY][tileX] = 0;
                }
                else
                {
                    g_apples[0].x = tileX;
                    g_apples[0].y = tileY;
                    //g_Tiles[tileX][tileY] = TILE_GREENSTAR;
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
                    g_Tiles[rand()%TILE_COLUMNS][rand()%TILE_ROWS] = TILE_GREENSTAR;
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
                    g_autoRun = !g_autoRun;
                }

                break;
            }
            } // end switch
        } // end of message processing

        if(g_died)
        {
            SDL_Delay(500);
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
