#include "Window.h"

void set_SDL_main_window(SDL_Window* mainWindow) {
    sdlMainWindow = mainWindow;
}

SDL_Window* get_SDL_main_window(void) {
    return sdlMainWindow;
}