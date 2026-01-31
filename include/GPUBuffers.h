#ifndef GPUBUFFERS_H
#define GPUBUFFERS_H

#include <SDL3/SDL.h>

// Transfer buffers of set sizes will be cached and used for uploading data to the gpu for vertex buffers and
// index buffers. If any of the pre-existing sizes are too small for the data requested to be uploaded, then a new
// transfer buffer will be appended of the required size and cached for use later if neeeded
typedef struct GPBUploadBuffer {
    SDL_GPUTransferBuffer* utb;
    Uint32 size;
    bool available;
} GPBUploadBuffer;

// Defines the possible sizes of the transfer buffers, with each size being
// allocated 4 transfer buffers each
// Note: these sizees should definitely be updated later
static const Uint32 transferBufferBinSizes[] = {6400, 64000, 256000};
#define TRANSFER_BUFFER_BIN_LEN sizeof(transferBufferBinSizes) / sizeof(transferBufferBinSizes[0])
#define TRANSFER_BUFFER_BIN_SPAN 4

// The actual array of transfer buffers (will be of size TRANSFER_BUFFER_BIN_LEN * TRANSFER_BUFFER_BIN_SPAN)
static GPBUploadBuffer* uploadTransferBuffers;
static Uint32 uploadTransferBuffersSize = 0;

// For use with the stack to store information relavent to the copy pass
typedef struct upload_info {
    SDL_GPUTransferBufferLocation location;
    SDL_GPUBufferRegion region;
    GPBUploadBuffer* buffer;
} upload_info;

// This will serve as the transfer buffer stack
static upload_info* uploadStack = NULL;
static Uint32 uploadStackSize = 0;
static Uint32 uploadStackMemsize = 1;

// Must be called before using any of the following functions
// sets the internal gpu device to whatever is passed into the function and sets up
// the upload transfer buffer array with some of the default sizes
void GPB_init();

// Must be called at the end of the program for proper de-initialization
// the primary thing it does is free up transfer buffers
void GPB_terminate(void);

// Creates a gpu buffer of the specified type using the specified data
// Note: type should be something like SDL_GPU_BUFFERUSAGE_*
// where * is VERTEX, INDEX, etc.
// Note: the size should be the number of bytes that the buffer is
SDL_GPUBuffer* GPB_create_buffer(Uint8 type, void* data, Uint32 size);

// pushes a transfer buffer onto the transfer buffer stack
int GPB_push_uinfo(upload_info* uinfo);

// Pops off a value at the end of the transfer buffer stack (and properly manages memory)
int GPB_pop_uinfo(void);

// Starts a copy pass to submit all of the created transfer buffers
// it then frees up all of the created transfer buffers once they are not needed
int GPB_submit_all_transfer_buffers(void);

// Returns a transfer buffer that is large enough for the given size. If the given size is larger than any
// of the existing transfer buffers, a new transfer buffer will be appended to the array of transfer buffers
// with a size large enough to fit the new size. If it runs out of space, then the program will crash and print
// out an error to the terminal
GPBUploadBuffer* GPB_get_transfer_buffer(Uint32 size);

#endif