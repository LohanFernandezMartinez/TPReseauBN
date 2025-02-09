#include "interface.h"
#include <stdio.h>

#define PORT 5000  // Au lieu de 8888

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

int initSDL(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    if (TTF_Init() < 0) return 0;
    
    window = SDL_CreateWindow("Bataille Navale", SDL_WINDOWPOS_CENTERED, 
                            SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 
                            SDL_WINDOW_SHOWN);
    if (!window) return 0;
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return 0;
    
    font = TTF_OpenFont("Arial.ttf", 20);
    if (!font) {
        printf("Erreur chargement police: %s\n", TTF_GetError());
        return 0;
    }
    
    return 1;
}

void closeSDL(void) {
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void drawGrid(int offsetX, const char* title) {
    // Draw grid background
    SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);
    SDL_Rect gridBg = {offsetX, GRID_OFFSET_Y, GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE};
    SDL_RenderFillRect(renderer, &gridBg);
    
    // Draw grid lines
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for (int i = 0; i <= GRID_SIZE; i++) {
        // Vertical lines
        SDL_RenderDrawLine(renderer, 
            offsetX + i * CELL_SIZE, GRID_OFFSET_Y,
            offsetX + i * CELL_SIZE, GRID_OFFSET_Y + GRID_SIZE * CELL_SIZE);
        // Horizontal lines
        SDL_RenderDrawLine(renderer,
            offsetX, GRID_OFFSET_Y + i * CELL_SIZE,
            offsetX + GRID_SIZE * CELL_SIZE, GRID_OFFSET_Y + i * CELL_SIZE);
    }
    
    // Draw labels
    SDL_Color textColor = {0, 0, 0, 255};
    char label[2];
    SDL_Surface* surface;
    SDL_Texture* texture;
    SDL_Rect rect;
    
    // Draw title
    surface = TTF_RenderText_Solid(font, title, textColor);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    rect.x = offsetX;
    rect.y = 10;
    rect.w = surface->w;
    rect.h = surface->h;
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    
    // Draw column labels (A-J)
    for (int i = 0; i < GRID_SIZE; i++) {
        label[0] = 'A' + i;
        label[1] = '\0';
        surface = TTF_RenderText_Solid(font, label, textColor);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        rect.x = offsetX + i * CELL_SIZE + CELL_SIZE/3;
        rect.y = GRID_OFFSET_Y - 25;
        rect.w = surface->w;
        rect.h = surface->h;
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
    
    // Draw row labels (1-10)
    for (int i = 0; i < GRID_SIZE; i++) {
        sprintf(label, "%d", i + 1);
        surface = TTF_RenderText_Solid(font, label, textColor);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        rect.x = offsetX - 25;
        rect.y = GRID_OFFSET_Y + i * CELL_SIZE + CELL_SIZE/3;
        rect.w = surface->w;
        rect.h = surface->h;
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

void drawShips(GameState* state) {
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    for (int i = 0; i < state->currentShip; i++) {
        Ship ship = state->ships[i];
        SDL_Rect rect;
        if (ship.isHorizontal) {
            rect.x = GRID_OFFSET_X + ship.x * CELL_SIZE;
            rect.y = GRID_OFFSET_Y + ship.y * CELL_SIZE;
            rect.w = ship.length * CELL_SIZE;
            rect.h = CELL_SIZE;
        } else {
            rect.x = GRID_OFFSET_X + ship.x * CELL_SIZE;
            rect.y = GRID_OFFSET_Y + ship.y * CELL_SIZE;
            rect.w = CELL_SIZE;
            rect.h = ship.length * CELL_SIZE;
        }
        SDL_RenderFillRect(renderer, &rect);
    }
}

void drawHitsAndMisses(GameState* state) {
    // Draw hits and misses on enemy grid
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (state->enemyGrid[i][j] > 0) {
                SDL_Rect rect = {
                    GRID_OFFSET_X + 600 + i * CELL_SIZE + CELL_SIZE/4,
                    GRID_OFFSET_Y + j * CELL_SIZE + CELL_SIZE/4,
                    CELL_SIZE/2,
                    CELL_SIZE/2
                };
                if (state->enemyGrid[i][j] == 1) {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Hit
                } else {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Miss
                }
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    
    // Draw hits and misses on my grid
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (state->grid[i][j] >= 2) {
                SDL_Rect rect = {
                    GRID_OFFSET_X + i * CELL_SIZE + CELL_SIZE/4,
                    GRID_OFFSET_Y + j * CELL_SIZE + CELL_SIZE/4,
                    CELL_SIZE/2,
                    CELL_SIZE/2
                };
                if (state->grid[i][j] == 2) {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Hit
                } else {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Miss
                }
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
}