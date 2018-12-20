#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include <time.h>
#include <SDL/SDL.h>

//#define _TEST
#define FPS 60
#define RESOLUTION_WIDTH    800
#define RESOLUTION_HEIGHT   600
#define GRAVITY         0.35
#define FLY_VELOCITY    4.0

#define CEILING_TOP         64
#define PIPE_GAP_MIN_HEIGHT     60
#define PIPE_GAP_MAX_HEIGHT     100

#define BIRD_OFFSET_X   45

#define MAP_SCROLL_PIXELS_PER_FRAME   2

enum GameState
{
    READY,
    RUNNING,
    PAUSED,
    OVER
};

typedef struct
{
    int x;
    int y;
    int right;
    int bottom;
    SDL_Surface *bmp;
} Sprite;

typedef struct
{
    int x;
    int y;
    int right;
    int bottom;
} Rect;

typedef struct
{
    Rect up;
    Rect down;
    int gap;
} Pipe;

typedef struct
{
    Rect rect;
    int bmpIndex;
    SDL_Surface* bmp[4];
} Bird;

SDL_Surface *g_screen;
SDL_Surface *g_font_numbers[10];
SDL_Surface *g_font_numbers2[10];
SDL_Surface *g_medal[4];
SDL_Surface *g_splash;
SDL_Surface *g_land;
int g_land_count;
SDL_Surface *g_sky;
int g_sky_count;
SDL_Surface *g_ceiling;
int g_ceiling_count;
SDL_Surface *g_scoreBoard;
SDL_Surface *g_pipeBody;
SDL_Surface *g_pipeUp;
SDL_Surface *g_pipeDown;
Sprite g_replayButton;
Pipe g_pipeList[640];
int g_pipeCount;

int g_frames;
float g_velocity;
float g_gravity;
int g_mapOffsetX;
Bird g_bird;
int g_limitUp;
int g_limitBottom;
int g_score;
int g_highScore = 0;
GameState g_state;
bool g_autopilot = true;
int g_minInterval = 100;

void autopilot();

void reset()
{
    g_score = 0;
    g_frames = 0;
    g_mapOffsetX = 0;
    g_velocity = 0;
    g_gravity = GRAVITY;
    g_pipeCount = 0;
    g_state = READY;
    g_bird.rect.y = (g_screen->h - g_bird.bmp[0]->h)/2;
    g_bird.rect.x = BIRD_OFFSET_X;
    g_bird.bmpIndex = 0;
    srand(time(0));
}

SDL_Surface* loadImage(char fileName[])
{
    char buf[100];
    sprintf(buf, "assets\\%s", fileName);
    SDL_Surface *bmp = SDL_LoadBMP(buf);
    if(bmp)
    {
        SDL_SetColorKey(bmp, SDL_SRCCOLORKEY, SDL_MapRGB(bmp->format, 0, 0, 0));
    }
    return bmp;
}

void loadAssets()
{
    int i;
    char buf[40];
    for(i=0; i<10; i++)
    {
        sprintf(buf, "font_big_%d.bmp", i);
        g_font_numbers[i] = loadImage(buf);
    }

    for(i=0; i<10; i++)
    {
        sprintf(buf, "font_small_%d.bmp", i);
        g_font_numbers2[i] = loadImage(buf);
    }

    for(i=0; i<4; i++)
    {
        sprintf(buf, "bird_%d.bmp", i);
        g_bird.bmp[i] = loadImage(buf);
    }

    for(i=0; i<4; i++)
    {
        sprintf(buf, "medal_%d.bmp", i);
        g_medal[i] = loadImage(buf);
    }

    g_splash = loadImage((char*)"splash.bmp");
    g_sky = loadImage((char*)"sky.bmp");
    g_land = loadImage((char*)"land.bmp");
    g_ceiling = loadImage((char*)"ceiling.bmp");
    g_scoreBoard = loadImage((char*)"scoreboard.bmp");
    g_pipeBody = loadImage((char*)"pipe.bmp");
    g_pipeDown = loadImage((char*)"pipe-down.bmp");
    g_pipeUp = loadImage((char*)"pipe-up.bmp");

    g_replayButton.bmp = loadImage((char*)"replay.bmp");
    g_replayButton.x = (g_screen->w - g_replayButton.bmp->w) / 2;
    g_replayButton.y = (g_screen->h - g_scoreBoard->h) / 2 + 200;
    g_replayButton.right = g_replayButton.x + g_replayButton.bmp->w;
    g_replayButton.bottom = g_replayButton.y + g_replayButton.bmp->h;

    g_land_count = g_screen->w/g_land->w + 2;
    g_sky_count = g_screen->w/g_sky->w + 2;
    g_ceiling_count = g_screen->w/g_ceiling->w+2;
    g_limitBottom = g_screen->h - g_land->h;
    g_limitUp = CEILING_TOP + g_ceiling->h;
    g_minInterval = (float)(g_limitBottom-g_limitUp-g_pipeBody->h-g_pipeDown->h-PIPE_GAP_MAX_HEIGHT)*MAP_SCROLL_PIXELS_PER_FRAME/FLY_VELOCITY;
}

void drawImage(SDL_Surface *bmp, int x, int y)
{
    if(!bmp)
        return;

    SDL_Rect r;
    r.x = x;
    r.y = y;
    r.w = bmp->w;
    r.h = bmp->h;
    SDL_BlitSurface(bmp, 0, g_screen, &r);
}

void drawNumbers(SDL_Surface* imageList[],
                 int imageCount,
                 int x,
                 int y,
                 int number,
                 int interval = 2)
{
    int score = number;
    int w = imageList[0]->w;
    do
    {
        drawImage(imageList[score%10], x, y);
        score /= 10;
        x -= w + 2;
    }
    while(score);
}

void drawSmallScore(int x, int y, int number)
{
    x -= g_font_numbers2[0]->w;
    drawNumbers(g_font_numbers2, 10, x, y, number);
}

void drawBigScore()
{
    drawNumbers(g_font_numbers, 10, 10 + (g_font_numbers[0]->w+5)*5, CEILING_TOP - g_font_numbers[0]->h - 5, g_score);
}

void drawPipeBody(int x, int y, int height)
{
    int i;
    for(i=0; i<height/g_pipeBody->h; i++)
    {
        drawImage(g_pipeBody, x, y+i*g_pipeBody->h);
    }
}

void drawPipe(int i)
{
    Pipe p = g_pipeList[i];

    int x = p.up.x;

    // up pipe
    drawPipeBody(x, p.up.y, p.up.bottom-p.up.y-g_pipeUp->h);
    drawImage(g_pipeDown, x, p.up.bottom-g_pipeDown->h);

    // down pipe
    drawPipeBody(x, p.down.y+g_pipeUp->h, p.down.bottom-p.down.y-g_pipeUp->h);
    drawImage(g_pipeUp, x, p.down.y);
#ifdef _TEST
    drawSmallScore(x, p.down.bottom + 20, i);
#endif // _TEST
}

void drawPipes()
{
    int i;
    for(i=0; i<g_pipeCount; i++)
    {
        drawPipe(i);
    }
}

void drawBird()
{
    drawImage(g_bird.bmp[g_bird.bmpIndex], g_bird.rect.x, g_bird.rect.y);
}

void drawBackground()
{
    int i;
    // land
    int offset = g_mapOffsetX%g_land->w;
    for(i=0; i<g_land_count; i++)
    {
        drawImage(g_land, i * g_land->w - offset, g_limitBottom);
    }

    // sky
    int landY = g_limitBottom - g_sky->h;
    offset = (g_mapOffsetX/8)%g_sky->w;
    for(i=0; i<g_sky_count; i++)
    {
        drawImage(g_sky, i * g_sky->w - offset, landY);
    }

    // ceiling
    offset = g_mapOffsetX%g_ceiling->w;
    for(i=0; i<g_ceiling_count; i++)
    {
        drawImage(g_ceiling, i * g_ceiling->w - offset, CEILING_TOP);
    }
    drawBigScore();
}

void drawForeground()
{
    drawPipes();
    drawBird();

    // splash
    int x, y;
    SDL_Surface *bmp = NULL;
    if(g_state==READY)
    {
        bmp = g_splash;
        x = g_bird.rect.x - g_mapOffsetX;
        y = 150;
    }
    else if (g_state==OVER)
    {
        x = (g_screen->w - g_scoreBoard->w) / 2;
        y = (g_screen->h - g_scoreBoard->h) / 2;
        bmp = g_scoreBoard;
    }
    if(bmp)
    {
        drawImage(bmp, x, y);
    }
    if (g_state==OVER)
    {
        drawSmallScore(x + 212, y + 109, g_score);
        drawSmallScore(x + 212, y + 151, g_highScore);

        // draw medal
        if(g_score <= 10)
            bmp = g_medal[0];
        else if (g_score <= 20)
            bmp = g_medal[1];
        else if (g_score <= 30)
            bmp = g_medal[2];
        else
            bmp = g_medal[3];

        drawImage(bmp, x+32, y+113);

        // Restart button
        drawImage(g_replayButton.bmp, g_replayButton.x, g_replayButton.y);
    }
}

void draw()
{
    drawBackground();
    drawForeground();
}
#include <math.h>

void generateRandomPipe()
{
    int index = g_pipeCount;
    int validHeight = g_limitBottom - g_limitUp;
    int randGapHeight = rand()%(PIPE_GAP_MAX_HEIGHT-PIPE_GAP_MIN_HEIGHT) + PIPE_GAP_MIN_HEIGHT;
    int maxUpHeight =  validHeight - randGapHeight - g_pipeDown->h - g_pipeUp->h - g_pipeBody->h *2;
    int upHeight = rand()%maxUpHeight + g_pipeDown->h + g_pipeBody->h;
    int downHeight = validHeight - upHeight - randGapHeight;
    int startX = 0;
    if(index > 0)
    {
        int heightDiff = abs(downHeight - (g_pipeList[index-1].down.bottom - g_pipeList[index-1].down.y));
        g_pipeList[index].gap = randGapHeight>heightDiff?randGapHeight:heightDiff;
        startX = g_pipeList[index-1].down.right + g_pipeList[index].gap;
    } else {
        g_pipeList[index].gap = g_minInterval;
        startX = RESOLUTION_WIDTH/2;
    }
    g_pipeList[index].up.x = startX;
    g_pipeList[index].up.y = g_limitUp;
    g_pipeList[index].up.right = startX + g_pipeBody->w;
    g_pipeList[index].up.bottom = g_limitUp + upHeight;

    g_pipeList[index].down.x = startX;
    g_pipeList[index].down.y = g_limitBottom - downHeight;
    g_pipeList[index].down.right = startX + g_pipeBody->w;
    g_pipeList[index].down.bottom = g_limitBottom;

    g_pipeCount++;
}

void gameOver()
{
    g_state = OVER;
    if(g_score > g_highScore)
    {
        g_highScore = g_score;
    }
}

bool collided(Rect r1, Rect r2)
{
    Rect r;
    r.x = r1.x>r2.x?r1.x:r2.x;
    r.right = r1.right<r2.right?r1.right:r2.right;
    r.y = r1.y>r2.y?r1.y:r2.y;
    r.bottom = r1.bottom<r2.bottom?r1.bottom:r2.bottom;
    return (r.right>r.x) && (r.bottom>r.y);
}

void updatePipes()
{
    int i;
    int unitWidth = g_pipeBody->w;
    int needPipes = RESOLUTION_WIDTH / unitWidth - g_pipeCount;
    for(i=0; i<needPipes; i++)
    {
        generateRandomPipe();
    }

    if(g_state==RUNNING)
    {
        // update pipe's position
        for(i=0; i<g_pipeCount; i++)
        {
            g_pipeList[i].up.x -= MAP_SCROLL_PIXELS_PER_FRAME;
            g_pipeList[i].up.right -= MAP_SCROLL_PIXELS_PER_FRAME;
            g_pipeList[i].down.x -= MAP_SCROLL_PIXELS_PER_FRAME;
            g_pipeList[i].down.right -= MAP_SCROLL_PIXELS_PER_FRAME;

            if((collided(g_bird.rect, g_pipeList[i].up) || collided(g_bird.rect, g_pipeList[i].down)))
            {
#ifndef _TEST
                SDL_SaveBMP(g_screen, "z:/bird.bmp");
                autopilot();
                collided(g_bird.rect, g_pipeList[i].down);
                gameOver();
#endif // _TEST
            }
        }
        static bool g_flyOver = false;
        if(g_pipeList[0].up.right >= 0)
        {
            if(!g_flyOver && g_pipeList[0].up.right<g_bird.rect.x)
            {
                g_score++;
                g_flyOver = true;
            }
        }
        else
        {
            for(i=1; i<g_pipeCount; i++)
            {
                g_pipeList[i-1] = g_pipeList[i];
            }
            g_pipeCount--;
            generateRandomPipe();
            g_flyOver = false;
        }
    }
}

bool collidedPipe(Pipe pipe, Rect rect)
{
    return collided(pipe.down, rect) || collided(pipe.up, rect);
}

bool willFallingFail(Pipe pipe, Rect rect, float velocity, float gravity)
{
    bool r = false;
    while(true)
    {
        velocity += gravity;
        rect.x += MAP_SCROLL_PIXELS_PER_FRAME;
        rect.right += MAP_SCROLL_PIXELS_PER_FRAME;
        rect.y += velocity;
        rect.bottom += velocity;
        if(rect.bottom >= g_limitBottom)
            break;

        if(rect.right>pipe.down.x)
        {
            if(collidedPipe(pipe, rect))
            {
                r = true;
            }
            break;
        }
    }
    return r;
}

bool willJumpFail(Pipe pipe, Rect rect, float velocity, float gravity)
{
    bool r = false;
    int startLine = rect.right<pipe.down.x?rect.right:rect.x;
    int stopLine = rect.right<pipe.down.x?pipe.down.x:pipe.down.right;
    while(startLine<=stopLine)
    {
        velocity += gravity;
        rect.x += MAP_SCROLL_PIXELS_PER_FRAME;
        rect.right += MAP_SCROLL_PIXELS_PER_FRAME;
        startLine += MAP_SCROLL_PIXELS_PER_FRAME;
        rect.y += velocity;
        if(rect.y < g_limitUp)
            rect.y = g_limitUp;
        rect.bottom = rect.y  + g_bird.bmp[g_bird.bmpIndex]->h;
        if(collidedPipe(pipe, rect) || rect.bottom>=g_limitBottom)
        {
            r = true;
            break;
        }
    }
    return r;
}

int falling(Rect rect, float velocity, float gravity, float lowLimit)
{
    int frames = 0;
    while(true)
    {
        velocity += gravity;
        rect.y += velocity;
        rect.bottom += velocity;

        if(rect.bottom >= lowLimit)
        {
            break;
        }
        frames++;
    }
    return frames;
}

Rect simulate(Rect rect, float flySpeed, float velocity, float gravity, int frames = 1)
{
    int h = rect.bottom - rect.y;
    int w = rect.right - rect.x;

    while(frames--)
    {
        velocity += gravity;
        rect.x += flySpeed;
        rect.y += velocity;
    }
    //rect.x += 0.5;
    //rect.y += 0.5;
    rect.right = rect.x + w;
    rect.bottom = rect.y + h;
    return rect;
}

void jump()
{
    g_velocity = -FLY_VELOCITY;
}

int getMaxJumpHeight(float velocity, float gravity)
{
    int y = 0;
    while(velocity<0)
    {
        y += velocity-0.5;
        velocity += gravity;
    }
    return -y;
}

void autopilot()
{
    if(!g_autopilot)
        return;

    Rect fakeBirdRect = g_bird.rect;
    int index = 0;
    // find the next pipe
    int i;
    for(i=0; i<g_pipeCount; i++)
    {
        if(g_pipeList[i].down.right >= fakeBirdRect.x)
        {
            index = i;
            break;
        }
    }
    Pipe currentPipe = g_pipeList[index];
    Pipe nextPipe = g_pipeList[index+1];
    bool needJump = false;
    bool nextPipeIsUp = nextPipe.up.bottom < currentPipe.up.bottom;
    Rect nextRect = simulate(fakeBirdRect, MAP_SCROLL_PIXELS_PER_FRAME, g_velocity, g_gravity);
    int maxJumpHeight = getMaxJumpHeight(-FLY_VELOCITY, g_gravity);
    if(nextPipeIsUp)
    {
        if(fakeBirdRect.y>currentPipe.up.bottom+maxJumpHeight)
        {
            needJump = true;
        }
    } else {
        if(nextRect.bottom >= currentPipe.down.y)
        {
            needJump = true;
        }
    }
    if(nextRect.bottom >= g_limitBottom)
    {
        needJump = true;
    }
    if(needJump)
    {
        jump();
    }
}

void update()
{
    if(g_state==RUNNING || g_state==OVER)
    {
        g_velocity += g_gravity;
#ifndef _TEST
        g_bird.rect.y += g_velocity;
        g_bird.rect.bottom = g_bird.rect.y + g_bird.bmp[g_bird.bmpIndex]->h;
#endif // _TEST
    }

    if(g_bird.rect.y+g_bird.bmp[0]->h>g_limitBottom)
    {
        g_bird.rect.y = g_limitBottom - g_bird.bmp[0]->h;
    }
    if(g_frames%3==0 && g_state!=OVER)
    {
        g_bird.bmpIndex++;
        if(g_bird.bmpIndex<0 || g_bird.bmpIndex>3)
        {
            g_bird.bmpIndex = 0;
        }
        g_bird.rect.right = g_bird.rect.x + g_bird.bmp[g_bird.bmpIndex]->w;
        g_bird.rect.bottom = g_bird.rect.y + g_bird.bmp[g_bird.bmpIndex]->h;
    }
    if(g_state==RUNNING)
    {
        g_mapOffsetX += MAP_SCROLL_PIXELS_PER_FRAME;
        // 限制高度
        if(g_bird.rect.y<g_limitUp)
            g_bird.rect.y = g_limitUp;
        else if(g_bird.rect.y+g_bird.bmp[0]->h>=g_limitBottom)
        {
            gameOver();
        }
    }
    updatePipes();

    if(g_state == RUNNING)
    {
        autopilot();
    }
}

int main ( int argc, char** argv )
{
    // initialize SDL video
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return 1;
    }

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // create a new window
    g_screen = SDL_SetVideoMode(RESOLUTION_WIDTH, RESOLUTION_HEIGHT, 32,
                                SDL_HWSURFACE|SDL_DOUBLEBUF);
    if ( !g_screen )
    {
        printf("Unable to set %dx%d video: %s\n", RESOLUTION_WIDTH, RESOLUTION_HEIGHT, SDL_GetError());
        return 1;
    }

    loadAssets();

    // program main loop
    bool done = false;
    reset();
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

            // check for keypresses
            case SDL_KEYDOWN:
            {
                // exit if ESCAPE is pressed
                SDLKey key = event.key.keysym.sym;
                if (key == SDLK_ESCAPE)
                    done = true;
                else if(key == SDLK_SPACE)
                {
                    static GameState g_oldState;
                    if(g_state==PAUSED)
                    {
                        g_state = g_oldState;
                    }
                    else
                    {
                        g_oldState = g_state;
                        g_state = PAUSED;
                    }
                }
                else if(key == SDLK_a)
                {
                    g_autopilot = !g_autopilot;
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                if(g_state==READY)
                {
                    g_state = RUNNING;
                }
                else if(g_state==RUNNING)
                {
                    if(event.button.state == SDL_PRESSED)
                    {
                        jump();
                    }
                }
                else if(g_state==OVER)
                {
                    int x = event.button.x;
                    int y = event.button.y;
                    if(x>=g_replayButton.x && x<=g_replayButton.right
                            && y>=g_replayButton.y && y<=g_replayButton.bottom)
                    {
                        reset();
                    }
                }
            }
            } // end switch
        } // end of message processing

        // update here
        update();

        // clear g_screen
        SDL_FillRect(g_screen, 0, SDL_MapRGB(g_screen->format, 0x4e, 0xc0, 0xca));

        // Draw background
        draw();

        // DRAWING ENDS HERE

        // finally, update the g_screen :)
        SDL_Flip(g_screen);

        g_frames++;
        SDL_Delay(1000/FPS);
    } // end main loop

    // all is well ;)
    printf("Exited cleanly\n");
    return 0;
}
