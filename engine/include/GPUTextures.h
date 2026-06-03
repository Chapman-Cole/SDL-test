#ifndef GPUTEXTURES_H
#define GPUTEXTURES_H

#include <SDL3/SDL.h>
#include "Strings.h"

typedef struct GPUTexture {
    SDL_GPUTexture* texture;

    SDL_GPUTransferBuffer* transfer_buffer;

    SDL_GPUTextureCreateInfo info;
} GPUTexture;

// Must call before using other functions. It is still up to you to
// set the texture info after this function has been called.
int GPUTexture_init(GPUTexture* tex);

// Creates the gpu texture according to the information defined in the info struct
int GPUTexture_create(GPUTexture* tex);

// Loads a gpu texture from a given file
int GPUTexture_load(GPUTexture* tex, string* path, bool waitToFinish);

// Cleans up resources used by the GPUTexture
int GPUTexture_destroy(GPUTexture* tex);

#endif