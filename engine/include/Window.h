#ifndef WINDOW_H
#define WINDOW_H

#include <SDL3/SDL.h>

static SDL_Window* sdlMainWindow = NULL;

// Allows you to set the main SDL window
void set_SDL_main_window(SDL_Window* mainWindow);

// Allows you to get an SDL_Window* to the main window
SDL_Window* get_SDL_main_window(void);

#endif