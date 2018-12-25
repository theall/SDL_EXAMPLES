#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include <SDL/SDL.h>
#include <time.h>

#define _TEST
#define TILE_GREENSTAR  1
#define TILE_YELLOWSTAR  2
#define TILE_REDSTAR  3
#define TILE_GRAYSTAR 4

#define TILE_COLUMNS 20
#define TILE_ROWS 20

#define GRID_SIZE 32
#define APPLE_COUNT 10

#define DISTANCE_STUB      0x7ffffffe
#define DISTANCE_RIGID     0x7fffffff

void render();

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
    RIGHT,
    DIR_COUNT
};

typedef struct
{
    int x;
    int y;
} Coordinate;

typedef struct
{
    Coordinate pos;
    int distance;
    Dir relativeDir;
} CoordinateStep;

int g_Tiles[TILE_ROWS][TILE_COLUMNS];
Coordinate g_snake[TILE_COLUMNS*TILE_ROWS];
Coordinate g_apples[APPLE_COUNT] = {0};
Dir g_nextDir;
int g_snakeLength;
float g_speed;
int g_frames;
bool g_died;
bool g_autoRun = true;
bool g_paused;
SDL_Surface *g_screen;
SDL_Surface *g_greenStar;
SDL_Surface *g_yellowStar;
SDL_Surface *g_redStar;
SDL_Surface *g_grayStar;

int getNextReachablePosList(int distance[TILE_ROWS][TILE_COLUMNS], Coordinate source, CoordinateStep cs[]);

int getDistance(Coordinate c1, Coordinate c2)
{
    return abs(c1.x-c2.x) + abs(c1.y-c2.y);
}

Coordinate getNextCoorFromDir(Coordinate currentPos, Dir dir)
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
    default:
        break;
    }
    return currentPos;
}

Coordinate getNextTilePos()
{
    return getNextCoorFromDir(g_snake[0], g_nextDir);
}

void resetTiles()
{
    int i,j;
    for(i=0; i<TILE_ROWS; i++)
    {
        for(j=0; j<TILE_COLUMNS; j++)
        {
            if(i==0 || i==(TILE_ROWS-1) || j==0 || j==(TILE_COLUMNS-1))
                g_Tiles[i][j] = TILE_GRAYSTAR;
            else
            {
                g_Tiles[i][j] = 0;
            }
        }
    }
}

int removeFirst(Coordinate a[], int arraySize)
{
    int i;
    for(i=1; i<arraySize; i++)
    {
        a[i-1] = a[i];
    }
    return arraySize-1;
}

void fillTileDistanceFromPos(int distance[TILE_ROWS][TILE_COLUMNS],
                             Coordinate pos,
                             Coordinate snake[],
                             int snakeLength,
                             Coordinate apples[] = 0,
                             int appleSize = 0)
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
    for(i=0; i<snakeLength; i++)
    {
        distance[snake[i].y][snake[i].x] = DISTANCE_RIGID;
    }
    for(i=0; i<appleSize; i++)
    {
        distance[apples[i].y][apples[i].x] = DISTANCE_RIGID;
    }
    distance[pos.y][pos.x] = 0;
    Coordinate pointList[TILE_ROWS*TILE_COLUMNS];
    pointList[0] = pos;
    int remainPointsCount = 1;
    while(remainPointsCount)
    {
        Coordinate frontPos = pointList[0];
        remainPointsCount = removeFirst(pointList, remainPointsCount);
        for(i=0; i<4; i++)
        {
            Coordinate nextPos = getNextCoorFromDir(frontPos, (Dir)i);
            if(distance[nextPos.y][nextPos.x] == DISTANCE_STUB)
            {
                // Is empty tile
                distance[nextPos.y][nextPos.x] = distance[frontPos.y][frontPos.x] + 1;
                pointList[remainPointsCount++] = nextPos;
            }
        }
    }
}

void clearApples()
{
    int i;
    for(i=0;i<APPLE_COUNT;i++)
    {
        g_apples[i].x = 0;
        g_apples[i].y = 0;
    }
}

void reset()
{
    g_snakeLength = 12;
    g_nextDir = UP;
    g_snake[0].x = 2;
    g_snake[0].y = 1;
    g_snake[1].x = 1;
    g_snake[1].y = 1;
    g_snakeLength = 2;
    g_nextDir = DOWN;
    g_speed = 16;
    g_frames = 0;
    g_died = false;
    g_paused = false;
    clearApples();
}

bool isApple(Coordinate pos)
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

bool isSnakeBody(Coordinate pos)
{
    int j;
    for(j=1; j<g_snakeLength; j++)
    {
        if(pos.x==g_snake[j].x && pos.y==g_snake[j].y)
        {
            return true;
        }
    }
    return false;
}

Dir CoordinateToDir(Coordinate d)
{
    Dir i;
    if(d.x==0)
    {
        i = d.y>0?DOWN:UP;
    }
    else
    {
        i = d.x>0?RIGHT:LEFT;
    }
    return i;
}

bool isReachable(Coordinate snake[], int snakeLength, Coordinate destination)
{
    CoordinateStep cs[4];
    int distance[TILE_ROWS][TILE_COLUMNS];
    fillTileDistanceFromPos(distance, destination, snake, snakeLength);
    int posCount = getNextReachablePosList(distance, snake[0], cs);
    int i;
    for(i=0;i<posCount;i++)
    {
        if(cs[i].distance > 1)
            return true;
    }
    return false;
}

bool canSimulate(int distanceTable[TILE_ROWS][TILE_COLUMNS],
                 Coordinate snake[],
                 int snakeLength,
                 Coordinate destination)
{
    if(snakeLength < 0)
        return false;

    Coordinate nextStep;
    while(snake[0].x!=destination.x || snake[0].y!=destination.y)
    {
        int i;
        CoordinateStep minStep;
        minStep.distance = DISTANCE_STUB;
        for(i=0; i<DIR_COUNT; i++)
        {
            nextStep = getNextCoorFromDir(snake[0], (Dir)i);
            if(distanceTable[nextStep.y][nextStep.x] < minStep.distance)
            {
                minStep.distance = distanceTable[nextStep.y][nextStep.x];
                minStep.pos = nextStep;
                minStep.relativeDir = (Dir)i;
            }
        }
        if(minStep.distance == DISTANCE_STUB)
        {
            return false;
        }
        for(i=snakeLength-1; i>0; i--)
        {
            snake[i] = snake[i-1];
        }
        snake[0] = minStep.pos;
    }
    return isReachable(snake, snakeLength, snake[snakeLength-1]);
}

int getNextReachablePosList(int distance[TILE_ROWS][TILE_COLUMNS], Coordinate pos, CoordinateStep cs[])
{
    int i;
    int index = 0;
    for(i=0; i<4; i++)
    {
        Coordinate nextPos = getNextCoorFromDir(pos, (Dir)i);
        int step = distance[nextPos.y][nextPos.x];
        if(step>=0 && step<DISTANCE_STUB)
        {
            cs[index].distance = step;
            cs[index].pos = nextPos;
            cs[index].relativeDir = (Dir)i;
            index++;
        } else {
            cs[index].distance = DISTANCE_RIGID;
            cs[index].pos.x = DISTANCE_RIGID;
            cs[index].pos.y = DISTANCE_RIGID;
            cs[index].relativeDir = DIR_COUNT;
        }
    }
    return index;
}

Dir findMinDistanceDir(int distanceTable[TILE_ROWS][TILE_COLUMNS], Coordinate pos, Dir currentDir)
{
    CoordinateStep cs[4];
    int csSize = getNextReachablePosList(distanceTable, pos, cs);
    if(csSize < 0)
        return (Dir)(rand()%DIR_COUNT);

    int i;
    int m = cs[0].distance;
    Dir findDir = cs[0].relativeDir;
    for(i=1; i<csSize; i++)
    {
        if(m>cs[i].distance || (m==cs[i].distance && (Dir)currentDir==cs[i].relativeDir))
        {
            m = cs[i].distance;
            findDir = cs[i].relativeDir;
        }
    }
    return findDir;
}

Dir findMaxDistanceDir(int distanceTable[TILE_ROWS][TILE_COLUMNS], Coordinate pos, Dir currentDir)
{
    CoordinateStep cs[4];
    int csSize = getNextReachablePosList(distanceTable, pos, cs);
    if(csSize <= 0)
        return DIR_COUNT;

    int i;
    int m = cs[0].distance;
    Dir findDir = cs[0].relativeDir;
    for(i=1; i<csSize; i++)
    {
        if(m<cs[i].distance || (m==cs[i].distance && (Dir)currentDir!=cs[i].relativeDir))
        {
            m = cs[i].distance;
            findDir = cs[i].relativeDir;
        }
    }
    return findDir;
}

bool canEatApple()
{
    return false;
}

int getPathDeep(int distanceTable[TILE_ROWS][TILE_COLUMNS], Coordinate pos)
{
    distanceTable[pos.y][pos.x] = DISTANCE_RIGID;
    int i;
    for(i=0; i<DIR_COUNT; i++)
    {
        Coordinate nextPos = getNextCoorFromDir(pos, (Dir)i);
        if(distanceTable[pos.y][pos.x] == DISTANCE_STUB)
        {
            return 1+getPathDeep(distanceTable, nextPos);
        }
    }
    return 0;
}

Dir getRandomDir()
{
    int distanceTable[TILE_ROWS][TILE_COLUMNS];
    fillTileDistanceFromPos(distanceTable, g_snake[0], g_snake, g_snakeLength);
    Dir d = findMaxDistanceDir(distanceTable, g_snake[0], g_nextDir);
    return d;
}

int getCloseApple()
{
    int i;
    int minD = DISTANCE_RIGID;
    int index = 0;
    for(i=0; i<APPLE_COUNT; i++)
    {
        if(g_apples[i].x==0 || g_apples[i].y==0)
            continue;

        int d = abs(g_apples[i].x-g_snake[0].x) + abs(g_apples[i].y-g_snake[0].y);
        if(minD>d)
        {
            minD = d;
            index = i;
        }
    }
    return index;
}

Dir getNextDir()
{
    int distanceTable[TILE_ROWS][TILE_COLUMNS];
    Coordinate fakeSnake[TILE_ROWS*TILE_COLUMNS];
    memcpy(fakeSnake, g_snake, sizeof(Coordinate)*TILE_ROWS*TILE_COLUMNS);
    int appleIndex = getCloseApple();
    fillTileDistanceFromPos(distanceTable, g_apples[appleIndex], g_snake, g_snakeLength);
    bool canEat = canSimulate(distanceTable, fakeSnake, g_snakeLength, g_apples[appleIndex]);
    if(canEat)
    {
        g_nextDir = findMinDistanceDir(distanceTable, g_snake[0], g_nextDir);
    }
    else
    {
        Coordinate snakeTail = g_snake[g_snakeLength-1];
        bool canFollowTrail = isReachable(g_snake, g_snakeLength, snakeTail);
        if(canFollowTrail)
        {
            // Generate distance table with snake tail position
            fillTileDistanceFromPos(distanceTable, snakeTail, g_snake, g_snakeLength, g_apples, APPLE_COUNT);
            distanceTable[snakeTail.y][snakeTail.x] = DISTANCE_RIGID;
            g_nextDir = findMaxDistanceDir(distanceTable, g_snake[0], g_nextDir);
        }
        else
        {
            // get away from apple
            g_nextDir = getRandomDir();
        }
    }
    if(g_nextDir >= DIR_COUNT) {
        // invalid dir
        g_nextDir = getRandomDir();
    }
    return g_nextDir;
}

Coordinate fetchElement(Coordinate el[], int arraySize, int index)
{
    Coordinate result = {-1,-1};
    if(index>=0 && index<arraySize)
    {
        int i;
        result = el[index];
        for(i=index; i<arraySize-1; i++)
        {
            el[i] = el[i+1];
        }
    }
    return result;
}

void backSpaceForTest()
{
    int i;
    for(i=0;i<g_snakeLength-1;i++)
    {
        g_snake[i] = g_snake[i+1];
    }
}

void update()
{
    resetTiles();

    // clear snake
    int i;
    Coordinate nextPos = getNextTilePos();
    if(g_Tiles[nextPos.y][nextPos.x] == TILE_GRAYSTAR || isSnakeBody(nextPos))
    {
#ifdef _TEST
        backSpaceForTest();
        g_apples[0] = g_snake[0];
        backSpaceForTest();
        SDL_SaveBMP(g_screen, "z:/sdl.bmp");
        g_nextDir = getNextDir();
#endif // _TEST
        isSnakeBody(nextPos);
        g_died = true;
        reset();
        return;
    }
    for(i=g_snakeLength-1; i>0; i--)
    {
        g_snake[i].x = g_snake[i-1].x;
        g_snake[i].y = g_snake[i-1].y;
    }

    // next directionfindMaxDistanceDir
    g_snake[0] = nextPos;

    // check apple ate
    for(i=0; i<APPLE_COUNT; i++)
    {
        if(g_apples[i].x==g_snake[0].x && g_apples[i].y==g_snake[0].y)
        {
            g_snake[g_snakeLength] = g_snake[g_snakeLength-1];
            g_snakeLength++;
            g_apples[i].x = 0;
            g_apples[i].y = 0;
            SDL_Delay(100);
            break;
        }
    }

    // update apples
    for(i=0; i<APPLE_COUNT; i++)
    {
        if(g_apples[i].x==0 || g_apples[i].y==0)
        {
            continue;
        }
        g_Tiles[g_apples[i].y][g_apples[i].x] = TILE_GREENSTAR;
    }
    // update snake
    for(i=1; i<g_snakeLength; i++)
    {
        g_Tiles[g_snake[i].y][g_snake[i].x] = TILE_YELLOWSTAR;
    }
    g_Tiles[g_snake[0].y][g_snake[0].x] = TILE_REDSTAR;

    // place apple
    // first find all the empty tiles
    Coordinate emptyTileList[TILE_ROWS*TILE_COLUMNS];
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
    if(emptyTileCount == 0)
    {
        g_died = true;
        return;
    }
    // check if need generate new apple
    for(i=0; i<APPLE_COUNT; i++)
    {
        if(g_apples[i].x==0 || g_apples[i].y==0)
        {
            int index = rand() % emptyTileCount;
            g_apples[i] = fetchElement(emptyTileList, emptyTileCount, index);
            emptyTileCount--;
        }
    }

    if(g_autoRun)
    {
        g_nextDir = getNextDir();
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
            else if(sprite==TILE_GRAYSTAR)
            {
                bmp = g_grayStar;
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
    g_grayStar = loadImage((char*)"graystar.bmp");

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
                    g_paused = !g_paused;
                }
                else if(key == SDLK_a)
                {
                    g_autoRun = !g_autoRun;
                }
                break;
            }
            } // end switch
        } // end of message processing

        if(g_frames%interval==0 && !g_paused)
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
