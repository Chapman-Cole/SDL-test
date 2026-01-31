#include "GPUBuffers.h"
#include <stdlib.h>
#include "SDLDevice.h"

void GPB_init() {
    uploadTransferBuffers = SDL_malloc((TRANSFER_BUFFER_BIN_LEN * TRANSFER_BUFFER_BIN_SPAN) * sizeof(GPBUploadBuffer));

    if (uploadTransferBuffers == NULL) {
        SDL_Log("GPUBuffers.c: Failed to allocate memory for the upload transfer buffers.");
        SDL_Quit();
        exit(-1);
    }

    for (int i = 0; i < TRANSFER_BUFFER_BIN_LEN; i++) {
        for (int j = 0; j < TRANSFER_BUFFER_BIN_SPAN; j++) {
            uploadTransferBuffers[(i * TRANSFER_BUFFER_BIN_SPAN) + j] = (GPBUploadBuffer){
                .utb = SDL_CreateGPUTransferBuffer(
                    get_SDL_gpu_device(),
                    &(SDL_GPUTransferBufferCreateInfo){
                        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                        .size = transferBufferBinSizes[i],
                        .props = 0}),
                .available = true,
                .size = transferBufferBinSizes[i]};
        }
    }
}

void GPB_terminate(void) {
    for (int i = 0; i < uploadTransferBuffersSize; i++) {
        SDL_ReleaseGPUTransferBuffer(get_SDL_gpu_device(), uploadTransferBuffers[i].utb);
    }

    SDL_free(uploadTransferBuffers);
    SDL_free(uploadStack);
}

SDL_GPUBuffer* GPB_create_buffer(Uint8 type, void* data, Uint32 size) {
    // Create the gpu buffer
    SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(
        get_SDL_gpu_device(),
        &(SDL_GPUBufferCreateInfo){
            .usage = type,
            .size = size,
            .props = 0});

    // Create the transfer buffer for getting the data to the gpu buffer
    GPBUploadBuffer* tb = GPB_get_transfer_buffer(size);

    // Fill the transfer buffer with data by mapping it to a pointer
    void* tb_data = SDL_MapGPUTransferBuffer(get_SDL_gpu_device(), tb->utb, false);
    SDL_memcpy(tb_data, data, size);
    SDL_UnmapGPUTransferBuffer(get_SDL_gpu_device(), tb->utb);

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
        .buffer = tb});

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
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());
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

    // Calculate the offset into the correct size
    int offset = -1;
    for (int i = 0; i < TRANSFER_BUFFER_BIN_LEN; i++) {
        if (size <= transferBufferBinSizes[i]) {
            offset = i * TRANSFER_BUFFER_BIN_SPAN;
        }
    }

    // Check to make sure that it actually fit into one of the transfer buffer categories
    if (offset == -1) {
        SDL_Log("GPUBuffers: GPB_get_transfer_buffer was given a size larger than the specified bin sizes\n");
        SDL_Quit();
        exit(-1);
    }

    int escape_con = 0;
search_again:
    // Find the first available transfer buffer in that range, if there is one available at all
    for (int i = offset; i < offset + TRANSFER_BUFFER_BIN_SPAN; i++) {
        if (uploadTransferBuffers[i].available == true) {
            tb = &uploadTransferBuffers[i];
        }
    }
    // Indicate that the buffer is no longer available (helps prevent accidental overwriting of buffers)
    tb->available = false;
    
    if (tb == NULL) {
        // If no slots of the specified size are available, it will submit everything that is already in the queue
        GPB_submit_all_transfer_buffers();

        if (escape_con == 0) {
            goto search_again;
            escape_con++;
        } else {
            SDL_Log("GPUBuffers.c: Infinite loop would have been entered\n");
            SDL_Quit();
            exit(-1);
        }
    }

    return tb;
}