#ifndef INTERFACE_H
#define INTERFACE_H

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define GRID_SIZE 10
#define CELL_SIZE 40

extern SDL_Renderer *renderer;

int init_SDL();
void close_SDL();
void draw_grid();
void draw_boat(int x, int y, int length, int horizontal);

#endif // INTERFACE_H