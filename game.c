#include <stdio.h>
#include <string.h>
#include "game.h"

void initGameState(GameState* state) {
    memset(state->grid, 0, sizeof(state->grid));
    memset(state->ships, 0, sizeof(state->ships));
    state->shipsLeft = NUM_SHIPS;
}

void displayGrid(int grid[GRID_SIZE][GRID_SIZE], int showShips) {
    printf("  0 1 2 3 4 5 6 7 8 9\n");
    for (int y = 0; y < GRID_SIZE; y++) {
        printf("%d ", y);
        for (int x = 0; x < GRID_SIZE; x++) {
            char c;
            switch(grid[x][y]) {
                case 0: c = '.'; break;  // Vide
                case 1: c = showShips ? 'O' : '.'; break;  // Bateau
                case 2: c = 'X'; break;  // Touché
                case 3: c = '~'; break;  // Manqué
                default: c = '?'; break;
            }
            printf("%c ", c);
        }
        printf("\n");
    }
}

int isValidPlacement(GameState* state, int x, int y, int length, int isHorizontal) {
    if (isHorizontal) {
        if (x + length > GRID_SIZE) return 0;
        for (int i = 0; i < length; i++) {
            if (state->grid[x + i][y] != 0) return 0;
        }
    } else {
        if (y + length > GRID_SIZE) return 0;
        for (int i = 0; i < length; i++) {
            if (state->grid[x][y + i] != 0) return 0;
        }
    }
    return 1;
}

void placeShip(GameState* state, int x, int y, int length, int isHorizontal, int shipIndex) {
    state->ships[shipIndex].length = length;
    state->ships[shipIndex].x = x;
    state->ships[shipIndex].y = y;
    state->ships[shipIndex].isHorizontal = isHorizontal;
    state->ships[shipIndex].hits = 0;
    
    if (isHorizontal) {
        for (int i = 0; i < length; i++) {
            state->grid[x + i][y] = 1;
        }
    } else {
        for (int i = 0; i < length; i++) {
            state->grid[x][y + i] = 1;
        }
    }
}

int processShot(GameState* state, int x, int y) {
    if (state->grid[x][y] == 1) {  // Touché
        state->grid[x][y] = 2;
        
        // Vérifier si un bateau est coulé
        for (int i = 0; i < NUM_SHIPS; i++) {
            Ship* ship = &state->ships[i];
            if (ship->isHorizontal) {
                if (x >= ship->x && x < ship->x + ship->length && y == ship->y) {
                    ship->hits++;
                    if (ship->hits == ship->length) {
                        state->shipsLeft--;
                        printf("Bateau coulé !\n");
                    } else {
                        printf("Touché !\n");
                    }
                }
            } else {
                if (x == ship->x && y >= ship->y && y < ship->y + ship->length) {
                    ship->hits++;
                    if (ship->hits == ship->length) {
                        state->shipsLeft--;
                        printf("Bateau coulé !\n");
                    } else {
                        printf("Touché !\n");}
                }
            }
        }
        return 1;
    } else {  // Manqué
        state->grid[x][y] = 3;
        printf("Manqué !\n");
        return 0;
    }
}