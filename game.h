#ifndef GAME_H
#define GAME_H

#define GRID_SIZE 10
#define NUM_SHIPS 5

typedef struct {
    int length;
    int x;
    int y;
    int isHorizontal;
    int hits;
} Ship;

typedef struct {
    int grid[GRID_SIZE][GRID_SIZE];  // 0=vide, 1=bateau, 2=touché, 3=manqué
    Ship ships[NUM_SHIPS];
    int shipsLeft;
} GameState;

// Fonctions utilitaires
void initGameState(GameState* state);
void displayGrid(int grid[GRID_SIZE][GRID_SIZE], int showShips);
int isValidPlacement(GameState* state, int x, int y, int length, int isHorizontal);
void placeShip(GameState* state, int x, int y, int length, int isHorizontal, int shipIndex);
int processShot(GameState* state, int x, int y);

#endif