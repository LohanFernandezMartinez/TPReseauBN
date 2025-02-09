#ifndef INTERFACE_H
#define INTERFACE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 600
#define GRID_SIZE 10
#define CELL_SIZE 50
#define GRID_OFFSET_X 50
#define GRID_OFFSET_Y 50

typedef struct {
    int x;
    int y;
    int length;
    int isHorizontal;
} Ship;

typedef struct {
    int grid[GRID_SIZE][GRID_SIZE];  // 0=empty, 1=ship, 2=hit, 3=miss
    int enemyGrid[GRID_SIZE][GRID_SIZE];  // 0=unknown, 1=hit, 2=miss
    Ship ships[8];  // 2x2cases, 2x3cases, 1x4cases, 1x5cases
    int currentShip;
    int placementPhase;
    int myTurn;
    int gameOver;
    int winner;
} GameState;

extern SDL_Renderer* renderer;
extern TTF_Font* font;

int initSDL(void);
void closeSDL(void);
void drawGrid(int offsetX, const char* title);
void drawShips(GameState* state);
void drawHitsAndMisses(GameState* state);
int handleShipPlacement(GameState* state, int mouseX, int mouseY);
void rotateCurrentShip(GameState* state);
void drawGameState(GameState* state);
void drawEndGame(GameState* state, int playerNumber);

#endif