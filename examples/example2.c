#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "engine.h"

ComputePipeline computePipeline;


SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Window* window = NULL;
    window = SDL_CreateWindow("SDL-test", 960, 540, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        SDL_Log("Window Creation Failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUDevice* device = NULL;
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
    if (device == NULL) {
        SDL_Log("GPU device creation failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_ClaimWindowForGPUDevice(device, window);

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    set_SDL_gpu_device(device);
    set_SDL_main_window(window);

    compute_pipeline_create(&computePipeline, &STRING("../shaders/example2/ComputeShader.glsl"), SHADER_COMPILATION_GLSL_PATH, &STRING("main"));

    GPUBuffer uploadBuffer;
    GPUBuffer downloadBuffer;

    GPUBuffer_create(&uploadBuffer, 1024 * sizeof(float), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ);
    GPUBuffer_create(&downloadBuffer, 1024 * sizeof(float), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE);

    float floatData[1024];
    for (int i = 0; i < 1024; i++) {
        floatData[i] = (float)i;
    }

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    GPUBuffer_upload(&uploadBuffer, floatData, 1024 * sizeof(float), false, copyPass);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);

    cmd = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());
    SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(
        cmd, 
        NULL, 
        0, 
        &(SDL_GPUStorageBufferReadWriteBinding){
            .buffer = downloadBuffer.gpu_buffer,
            .cycle = false
        }, 
        1
    );

    SDL_BindGPUComputePipeline(computePass, computePipeline.computePipeline);
    SDL_BindGPUComputeStorageBuffers(
        computePass,
        0,
        (SDL_GPUBuffer*[]){
            uploadBuffer.gpu_buffer,
            downloadBuffer.gpu_buffer
        },
        2
    );

    SDL_DispatchGPUCompute(computePass, (1024 + computePipeline.thread_count_x - 1) / computePipeline.thread_count_x, 1, 1);

    SDL_EndGPUComputePass(computePass);
    SDL_SubmitGPUCommandBuffer(cmd);

    cmd = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());
    copyPass = SDL_BeginGPUCopyPass(cmd);

    GPUBuffer_download_transfer(&downloadBuffer, copyPass);

    SDL_EndGPUCopyPass(copyPass);

    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    SDL_WaitForGPUFences(get_SDL_gpu_device(), true, &fence, 1);
    SDL_ReleaseGPUFence(get_SDL_gpu_device(), fence);

    float* data = (float*)GPUBuffer_download_open_view(&downloadBuffer);

    for (int i = 0; i < 100; i++) {
        SDL_Log("%f", data[i]);
    }

    GPUBuffer_download_close_view(&downloadBuffer, false);

    compute_pipeline_destroy(&computePipeline);
    GPUBuffer_destroy(&uploadBuffer);
    GPUBuffer_destroy(&downloadBuffer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    return SDL_APP_SUCCESS;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    compute_pipeline_destroy(&computePipeline);
    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}