#ifndef GPUBUFFERS_H
#define GPUBUFFERS_H

#include <SDL3/SDL.h>

// A reference to an externally created gpu device to be used as an 
// internal handle to the device itself to simplify the use of the 
// functions
static SDL_GPUDevice* gpb_device;

// For use with the stack to store information relavent to the copy pass
typedef struct tb_info {
    SDL_GPUTransferBuffer* tb;
    SDL_GPUTransferBufferLocation location;
    SDL_GPUBufferRegion region;
} tb_info;

// This will serve as the transfer buffer stack
static tb_info* tbStack = NULL;
static Uint32 tbStackSize = 0;
static Uint32 tbStackMemsize = 1;

// Must be called before using any of the following functions
void GPB_set_device(SDL_GPUDevice* dev);

// Creates a gpu buffer of the specified type using the specified data
// Note: type should be something like SDL_GPU_BUFFERUSAGE_*
// where * is VERTEX, INDEX, etc.
// Note: the size should be the number of bytes that the buffer is
SDL_GPUBuffer* GPB_create_buffer(Uint8 type, void* data, Uint32 size);

// pushes a transfer buffer onto the transfer buffer stack
int GPB_push_tb(SDL_GPUTransferBuffer* tb, SDL_GPUTransferBufferLocation location, SDL_GPUBufferRegion region);

// Pops off a value at the end of the transfer buffer stack (and properly manages memory)
int GPB_pop_tb(void);

// Starts a copy pass to submit all of the created transfer buffers
// it then frees up all of the created transfer buffers once they are not needed
int GPB_submit_all_transfer_buffers(void);

#endif