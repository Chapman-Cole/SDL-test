#define SDL_MAIN_USE_CALLBACKS 1
#include "GPUBuffers.h"
#include "GraphicsPipeline.h"
#include "MeshObject.h"
#include "SDLDevice.h"
#include "Shader.h"
#include "Strings.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cglm/cglm.h>
#include "Window.h"

typedef struct UniformParams {
    float u_scale;
    float offset;
    float pad[2]; // 16 bytes total
} UniformParams;

typedef struct ColorParams {
    SDL_FColor col1;
    SDL_FColor col2;
    SDL_FColor col3;
} ColorParams;

SDL_Window* window = NULL;
SDL_GPUDevice* device = NULL;

SDL_GPUGraphicsPipeline* graphicsPipeline = NULL;

Mesh quadMesh;
Mesh quadMesh2;

Uint64 perfFrequency;
Uint64 perfCounterPrev;

double elapsedTime = 0.0;

float time = 0.0;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    perfFrequency = SDL_GetPerformanceFrequency();
    perfCounterPrev = SDL_GetPerformanceCounter();

    window = SDL_CreateWindow("SDL-test App Window", 960, 540, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        SDL_Log("Window Creation Failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, false, NULL);
    if (device == NULL) {
        SDL_Log("GPU device creation failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_ClaimWindowForGPUDevice(device, window);

    // Set the sdl gpu device for all modules, and initialize gpb (gpu buffers)
    set_SDL_gpu_device(device);
    set_SDL_main_window(window);
    GPB_init();

    // Create the quad mesh
    meshobject_init(&quadMesh);
    meshobject_load_objfile(&quadMesh, STRING("../objects/StarWars.obj"));

    meshobject_init(&quadMesh2);
    meshobject_load_objfile(&quadMesh2, STRING("../objects/Quad.obj"));

    GPB_submit_all_transfer_buffers();

    // Create the vertex shader
    SDL_GPUShader* vertexShader = create_vertex_shader(STRING("../shaders/vertex.glsl"), STRING("main"), false);

    // Create the fragment shader
    SDL_GPUShader* fragmentShader = create_fragment_shader(STRING("../shaders/fragment.glsl"), STRING("main"), false);

    graphics_pipeline_factory_registry_init();
    graphicsPipeline = graphics_pipeline_factory_registry_generate_pipeline(STRING("default"), vertexShader, fragmentShader);
    graphics_pipeline_factory_registry_terminate();

    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentShader);

    // Disable vsync
    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    Uint64 perfCounterNow = SDL_GetPerformanceCounter();
    double elapsed = (double)(perfCounterNow - perfCounterPrev) / (double)perfFrequency;
    perfCounterPrev = perfCounterNow;
    time += (float)elapsed;

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);

    SDL_GPUTexture* swapchainTexture;
    Uint32 width, height;
    SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &width, &height);
    if (swapchainTexture == NULL) {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return SDL_APP_CONTINUE;
    }

    SDL_GPUColorTargetInfo colorTargetInfo = {0};
    colorTargetInfo.clear_color = (SDL_FColor){255 / 255.0f, 219 / 255.0f, 187 / 255.0f, 255 / 255.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorTargetInfo.texture = swapchainTexture;

    // Begin the render pass
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

    // bind the graphics pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, graphicsPipeline);

    // Pass data to the uniform
    UniformParams params = {0};
    params.u_scale = time;
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &params, sizeof(params));

    //mat4 trans;
    //glm_mat4_identity(trans);
    //glm_rotate(trans, time / 2.0f, (vec3){0.0f, 0.0f, 1.0f});
    //glm_scale(trans, (vec3){0.5, 0.5, 0.5});

    //SDL_PushGPUVertexUniformData(commandBuffer, 1, trans, sizeof(mat4));

    params.offset = 0.6f;
    SDL_PushGPUFragmentUniformData(commandBuffer, 0, &params, sizeof(params));
    SDL_PushGPUFragmentUniformData(commandBuffer, 1, &(ColorParams){
        .col1 = (SDL_FColor){3.0 / 255.0, 64.0 / 255.0, 120.0 / 255.0, 1.0},
        .col2 = (SDL_FColor){0.0, 31 / 255.0, 84.0 / 255.0, 1.0},
        .col3 = (SDL_FColor){10.0 / 255.0, 17.0 / 255.0, 40 / 255.0, 1.0}
    },
    sizeof(ColorParams)
    );
    meshobject_render(&quadMesh2, renderPass);

    params.offset = -0.6f;
    SDL_PushGPUFragmentUniformData(commandBuffer, 0, &params, sizeof(params));
    SDL_PushGPUFragmentUniformData(commandBuffer, 1, &(ColorParams){
        .col1 = (SDL_FColor){255.0 / 255.0, 71.0 / 255.0, 76.0 / 255.0, 1.0},
        .col2 = (SDL_FColor){255.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0, 1.0},
        .col3 = (SDL_FColor){153.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0, 1.0}
    },
    sizeof(ColorParams)
    );
    meshobject_render(&quadMesh, renderPass);

    // End the render pass before submitting the command buffer
    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    GPB_terminate();

    meshobject_destroy(&quadMesh);
    meshobject_destroy(&quadMesh2);

    // release the pipeline
    SDL_ReleaseGPUGraphicsPipeline(device, graphicsPipeline);

    // destroy the gpu device
    SDL_DestroyGPUDevice(device);

    // destory the window
    SDL_DestroyWindow(window);
}