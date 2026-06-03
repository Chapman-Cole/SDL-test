#include "GPUTextures.h"
#include "SDLDevice.h"
#include <SDL3_image/SDL_image.h>

int GPUTexture_init(GPUTexture* tex) {
    tex->texture = NULL;
    tex->transfer_buffer = NULL;
    tex->info = (SDL_GPUTextureCreateInfo){0};
    return 0;
}

int GPUTexture_create(GPUTexture* tex) {
    tex->texture = SDL_CreateGPUTexture(
        get_SDL_gpu_device(),
        &tex->info
    );

    if (tex->texture == NULL) {
        SDL_Log("Failed to create gpu texture.");
        SDL_Quit();
        exit(-1);
    }

    return 0;
}

int GPUTexture_load(GPUTexture* tex, string* path, bool waitToFinish) {
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    tex->texture = IMG_LoadGPUTexture(get_SDL_gpu_device(), copyPass, path->str, &tex->info.width, &tex->info.height);

    if (tex->texture == NULL) {
        SDL_Log("Failed to create gpu texture.");
        SDL_Quit();
        exit(-1);
    }

    // This info is not used to create the texture, but is set according to how the IMG_LoadGPUTexture function works
    // This information is essentially just set in case something like the pixel format of the texture needs to be queried at runtime
    tex->info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tex->info.num_levels = 0;
    tex->info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ;
    tex->info.type = SDL_GPU_TEXTURETYPE_2D;

    SDL_EndGPUCopyPass(copyPass);

    if (waitToFinish == false) {
        SDL_SubmitGPUCommandBuffer(cmd); 
    } else {
        SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
        SDL_WaitForGPUFences(get_SDL_gpu_device(), true, &fence, 1);
        SDL_ReleaseGPUFence(get_SDL_gpu_device(), fence);
    }

    return 0;
}

int GPUTexture_destroy(GPUTexture* tex) {
    SDL_ReleaseGPUTexture(get_SDL_gpu_device(), tex->texture);

    tex->texture = NULL;
    tex->transfer_buffer = NULL;
    tex->info = (SDL_GPUTextureCreateInfo){0};

    return 0;
}