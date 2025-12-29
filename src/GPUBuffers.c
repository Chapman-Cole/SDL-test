#include "GPUBuffers.h"
#include <stdlib.h>

void GPB_set_device(SDL_GPUDevice* dev) {
    gpb_device = dev;
}

SDL_GPUBuffer* GPB_create_buffer(Uint8 type, void* data, Uint32 size) {
    // Create the gpu buffer
    SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(
        gpb_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = type,
            .size = size,
            .props = 0});

    // Create the transfer buffer for getting the data to the gpu buffer
    SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(
        gpb_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = size,
            .props = 0});

    // Fill the transfer buffer with data by mapping it to a pointer
    void* tb_data = SDL_MapGPUTransferBuffer(gpb_device, tb, false);
    SDL_memcpy(tb_data, data, size);
    SDL_UnmapGPUTransferBuffer(gpb_device, tb);

    // Find where the data is
    SDL_GPUTransferBufferLocation location = {0};
    location.transfer_buffer = tb;
    location.offset = 0;

    // Find where to upload the data
    SDL_GPUBufferRegion region = {0};
    region.buffer = buffer;
    region.size = size;
    region.offset = 0;

    GPB_push_tb(tb, location, region);

    return buffer;
}

int GPB_push_tb(SDL_GPUTransferBuffer* tb, SDL_GPUTransferBufferLocation location, SDL_GPUBufferRegion region) {
    if (tbStackSize + 1 >= tbStackMemsize) {
        tbStackMemsize *= 2;
        void* test = SDL_realloc(tbStack, tbStackMemsize * sizeof(tb_info));
        if (test == NULL) {
            SDL_Log("Failed to allocate memory for the transfer buffer stack\n");
            SDL_Quit();
            exit(-1);
        }
        tbStack = test;
    }

    tbStack[tbStackSize] = (tb_info){
        .tb = tb,
        .location = location,
        .region = region};
    tbStackSize++;
    return 0;
}

int GPB_pop_tb(void) {
    if (tbStackSize > 0) {
        SDL_ReleaseGPUTransferBuffer(gpb_device, tbStack[tbStackSize - 1].tb);
        tbStackSize--;
    }
}

int GPB_submit_all_transfer_buffers(void) {
    // Start a copy pass to get the data in the transfer buffer to the gpu buffers
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(gpb_device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    for (int i = tbStackSize - 1; i >= 0; i--) {
        SDL_UploadToGPUBuffer(copyPass, &tbStack[i].location, &tbStack[i].region, true);
        GPB_pop_tb();
    }

    // End the copy pass
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
}