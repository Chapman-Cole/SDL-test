#include "GPUBuffers.h"
#include <stdlib.h>

void GPB_init(SDL_GPUDevice* dev) {
    gpb_device = dev;

    // Some good base sizes to start with (the sizes are in bytes)
    const Uint32 base_sizes[] = {
        6400, 6400, 6400, 6400,
        64000, 64000, 64000, 64000,
        256000, 256000, 256000, 256000};
    for (int i = 0; i < 3; i++) {
        uploadTransferBuffers[uploadTransferBuffersSize] = (GPBUploadBuffer){
            .utb = SDL_CreateGPUTransferBuffer(
                gpb_device,
                &(SDL_GPUTransferBufferCreateInfo){
                    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                    .size = base_sizes[i],
                    .props = 0}),
            .size = base_sizes[i],
            .available = true};
        uploadTransferBuffersSize++;
    }
}

void GPB_terminate(void) {
    for (int i = 0; i < uploadTransferBuffersSize; i++) {
        SDL_ReleaseGPUTransferBuffer(gpb_device, uploadTransferBuffers[i].utb);
    }
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
    GPBUploadBuffer* tb = GPB_get_transfer_buffer(size);
    // Indicate that the buffer is no longer available (helps prevent accidental overwriting of buffers)
    tb->available = false;

    // Fill the transfer buffer with data by mapping it to a pointer
    void* tb_data = SDL_MapGPUTransferBuffer(gpb_device, tb->utb, false);
    SDL_memcpy(tb_data, data, size);
    SDL_UnmapGPUTransferBuffer(gpb_device, tb->utb);

    // Find where the data is
    SDL_GPUTransferBufferLocation location = {0};
    location.transfer_buffer = tb->utb;
    location.offset = 0;

    // Find where to upload the data
    SDL_GPUBufferRegion region = {0};
    region.buffer = buffer;
    region.size = size;
    region.offset = 0;

    GPB_push_uinfo(&(upload_info){
        .location = location,
        .region = region,
        .buffer = tb
    });

    return buffer;
}

int GPB_push_uinfo(upload_info* uinfo) {
    if (uploadStackSize + 1 >= uploadStackMemsize) {
        uploadStackMemsize *= 2;
        void* test = SDL_realloc(uploadStack, uploadStackMemsize * sizeof(upload_info));
        if (test == NULL) {
            SDL_Log("Failed to allocate memory for the transfer buffer stack\n");
            SDL_Quit();
            exit(-1);
        }
        uploadStack = test;
    }

    uploadStack[uploadStackSize] = *uinfo;
    uploadStackSize++;
    return 0;
}

int GPB_pop_uinfo(void) {
    if (uploadStackSize > 0) {
        uploadStackSize--;
        // Make the transfer buffer available for new data again
        uploadStack[uploadStackSize].buffer->available = true;
    }
    return 0;
}

int GPB_submit_all_transfer_buffers(void) {
    // Start a copy pass to get the data in the transfer buffer to the gpu buffers
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(gpb_device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    for (int i = uploadStackSize - 1; i >= 0; i--) {
        SDL_UploadToGPUBuffer(copyPass, &uploadStack[i].location, &uploadStack[i].region, true);
        GPB_pop_uinfo();
    }

    // End the copy pass
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    return 0;
}

GPBUploadBuffer* GPB_get_transfer_buffer(Uint32 size) {
    GPBUploadBuffer* tb = NULL;
    for (int i = 0; i < uploadTransferBuffersSize && i < GPB_MAX_TRANSFER_BUFFERS; i++) {
        if (size <= uploadTransferBuffers[i].size && uploadTransferBuffers[i].available == true) {
            tb = &uploadTransferBuffers[i];
            break;
        }
    }

    if (tb == NULL && uploadTransferBuffersSize < GPB_MAX_TRANSFER_BUFFERS) {
        uploadTransferBuffers[uploadTransferBuffersSize] = (GPBUploadBuffer){
            .utb = SDL_CreateGPUTransferBuffer(
                gpb_device,
                &(SDL_GPUTransferBufferCreateInfo){
                    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                    .size = size,
                    .props = 0}),
            .size = size};
        tb = &uploadTransferBuffers[uploadTransferBuffersSize];
        uploadTransferBuffersSize++;
    } else if (tb == NULL && uploadTransferBuffersSize >= GPB_MAX_TRANSFER_BUFFERS) {
        SDL_Log("Ran out of slots for more transfer buffers\n");
        SDL_Quit();
        exit(-1);
    }

    return tb;
}