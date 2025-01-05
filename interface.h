#ifndef INTERFACE_H
#define INTERFACE_H

#include <SDL2/SDL.h>

extern SDL_Renderer *renderer;

void init_SDL();
void close_SDL();
void draw_grid();
void draw_boat(int x, int y, int length, int horizontal);

#endif // INTERFACE_H