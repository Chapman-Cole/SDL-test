#ifndef GPUBUFFERS_H
#define GPUBUFFERS_H

#include <SDL3/SDL.h>

typedef struct TBStackElement {
    SDL_GPUTransferBuffer* transferBuffer;
    SDL_GPUBuffer* buffer;
    Uint32 dataSize;
} TBStackElement;

static Uint32 TBStackLen = 0;
static Uint32 TBStackCapacity = 1;
static TBStackElement* TBStack = NULL;

// Must call to setup the TBStack array
int GPB_init(void);

// Must be called to free up the memory used by the TBStack array
int GPB_terminate(void);

// Pushes an element to the TBStack
int GPB_TBStack_push(TBStackElement element);

// Pops an element off the TBStack, which has the effect of freeing the used transfer buffer
// once it is done using it
int GPB_TBStack_pop(void);

// Creates an SDL_GPUBuffer* and adds it to an internal stack for submitting to the gpu later on
SDL_GPUBuffer* GPB_create_buffer(Uint8 type, void* data, Uint32 size);

// Starts a copy pass to submit all of the created transfer buffers
// it then frees up all of the created transfer buffers once they are not needed
int GPB_submit_all_transfer_buffers(void);

#endif