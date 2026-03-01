#include "GPUBuffers.h"
#include <stdlib.h>
#include "SDLDevice.h"

int GPB_init(void) {
    TBStack = (TBStackElement*)SDL_malloc(TBStackCapacity * sizeof(TBStackElement));

    if (TBStack == NULL) {
        SDL_Log("Failed to allocate memory for TBStack in GPUBuffers.c");
        SDL_Quit();
        exit(-1);
    }

    return 0;
}

int GPB_terminate(void) {
    for (int i = 0; i < TBStackLen; i++) {
        SDL_ReleaseGPUTransferBuffer(get_SDL_gpu_device(), TBStack[i].transferBuffer);
    }
    SDL_free(TBStack);

    return 0;
}

int GPB_TBStack_push(TBStackElement element) {
    if (TBStackLen + 1 >= TBStackCapacity) {
        TBStackCapacity *= 2;

        TBStack = (TBStackElement*)SDL_realloc(TBStack, TBStackCapacity * sizeof(TBStackElement));

        if (TBStack == NULL) {
            SDL_Log("Failed to expand capacity of TBStack in GPUBuffers.c");
            SDL_Quit();
            exit(-1);
        }
    }

    TBStack[TBStackLen] = element;
    TBStackLen++;

    return 0;
}

int GPB_TBStack_pop(void) {
    SDL_ReleaseGPUTransferBuffer(get_SDL_gpu_device(), TBStack[TBStackLen-1].transferBuffer);

    if (TBStackLen - 1 < TBStackCapacity / 2) {
        TBStackCapacity /= 2;
        if (TBStackCapacity <= 0) {
            TBStackCapacity = 1;
        }

        TBStack = (TBStackElement*)SDL_realloc(TBStack, TBStackCapacity * sizeof(TBStackElement));

        if (TBStack == NULL) {
            SDL_Log("Failed to shrink size of capacity of TBStack in GPUBuffers.c");
            SDL_Quit();
            exit(-1);
        }
    }

    TBStackLen--;

    return 0;
}

SDL_GPUBuffer* GPB_create_buffer(Uint8 type, void* data, Uint32 size) {
    SDL_GPUBuffer* buffer = NULL;
    SDL_GPUTransferBuffer* transferBuffer = NULL;

    // The buffers are created below
    buffer = SDL_CreateGPUBuffer(
        get_SDL_gpu_device(),
        &(SDL_GPUBufferCreateInfo){
            .size = size,
            .usage = type,
            .props = 0
        }
    );

    if (buffer == NULL) {
        SDL_Log("Failed to create GPU Buffer in GPUBuffers.c");
        SDL_Quit();
        exit(-1);
    }

    transferBuffer = SDL_CreateGPUTransferBuffer(
        get_SDL_gpu_device(),
        &(SDL_GPUTransferBufferCreateInfo){
            .size = size,
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .props = 0
        }
    );

    if (transferBuffer == NULL) {
        SDL_Log("Failed to create transfer buffer in GPUBuffers.c");
        SDL_Quit();
        exit(-1);
    }
    
    // Fill the transfer buffer with the data
    void* gpuData = (void*)SDL_MapGPUTransferBuffer(get_SDL_gpu_device(), transferBuffer, false);

    if (gpuData == NULL) {
        SDL_Log("Failed to map transfer buffer to memory space in GPUBuffers.c");
        SDL_Quit();
        exit(-1);
    }

    SDL_memcpy(gpuData, data, size);
    SDL_UnmapGPUTransferBuffer(get_SDL_gpu_device(), transferBuffer);

    GPB_TBStack_push((TBStackElement){
        .buffer = buffer,
        .transferBuffer = transferBuffer,
        .dataSize = size
    });

    return buffer;
}

int GPB_submit_all_transfer_buffers(void) {
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());
    if (commandBuffer == NULL) {
        SDL_Log("Failed to create command buffer for copy pass in GPUBuffers.c");
        SDL_Quit();
        exit(-1);
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    for (int i = 0; i < TBStackLen; i++) {
        SDL_UploadToGPUBuffer(
            copyPass,
            &(SDL_GPUTransferBufferLocation){
                .transfer_buffer = TBStack[i].transferBuffer,
                .offset = 0
            },
            &(SDL_GPUBufferRegion){
                .buffer = TBStack[i].buffer,
                .size = TBStack[i].dataSize,
                .offset = 0
            },
            true
        );
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    while (TBStackLen > 0) {
        GPB_TBStack_pop();
    }

    return 0;
}