#include "SDLDevice.h"

void set_SDL_gpu_device(SDL_GPUDevice* gpd) {
    gpu_device = gpd;
}

SDL_GPUDevice* get_SDL_gpu_device(void) {
    return gpu_device;
}

void destroy_SDL_gpu_device(void) {
    SDL_DestroyGPUDevice(gpu_device);
}