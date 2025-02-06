#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "interface.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define GRID_SIZE 10
#define CELL_SIZE 40

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

int init_SDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    window = SDL_CreateWindow("Bataille Navale", 
                            SDL_WINDOWPOS_UNDEFINED, 
                            SDL_WINDOWPOS_UNDEFINED, 
                            WINDOW_WIDTH, 
                            WINDOW_HEIGHT, 
                            SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return 0;
    }

    return 1;
}

void close_SDL() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void draw_grid() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    for (int i = 0; i <= GRID_SIZE; ++i) {
        SDL_RenderDrawLine(renderer, i * CELL_SIZE, 0, i * CELL_SIZE, GRID_SIZE * CELL_SIZE);
        SDL_RenderDrawLine(renderer, 0, i * CELL_SIZE, GRID_SIZE * CELL_SIZE, i * CELL_SIZE);
    }
}

void draw_boat(int x, int y, int length, int horizontal) {
    SDL_Rect boat;
    boat.x = x * CELL_SIZE;
    boat.y = y * CELL_SIZE;
    boat.w = horizontal ? length * CELL_SIZE : CELL_SIZE;
    boat.h = horizontal ? CELL_SIZE : length * CELL_SIZE;

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &boat);
}