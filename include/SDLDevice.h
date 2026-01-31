#ifndef SDLDEVICE_H
#define SDLDEVICE_H

#include <SDL3/SDL.h>

// A simple header file for keeping track of the gpu device across files easily
// The gpu device should be set once and used across all calls to SDL
static SDL_GPUDevice* gpu_device;

void set_SDL_gpu_device(SDL_GPUDevice* gpd);

SDL_GPUDevice* get_SDL_gpu_device(void);

#endif