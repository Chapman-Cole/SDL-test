#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "engine.h"
#include <cglm/cglm.h>

// To compile with debug symbols on linux, do cmake -DCMAKE_BUILD_TYPE=Debug ..
// For debug symbols on windows, cmake --build . --config Debug

// must be 16 byte aligned to conform to vulkan std140
typedef struct RectangleVertexUniform {
    float xScale;
    float yScale;
    float xOffset;
    float pad;
} RectangleVertexUniform;

typedef struct RectangleFragmentUniform
{
    SDL_FColor color;
} RectangleFragmentUniform;


typedef struct ScalableRectangle {
    Mesh rect;
    float yScale;
    float xScale;
    float xOffset;
    SDL_FColor color;
} ScalableRectangle;

float appTime = 0.0f;
ScalableRectangle* rectangles = NULL;
Uint32 rectanglesLen = 10;

SDL_GPUGraphicsPipeline* graphicsPipeline = NULL;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
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
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (device == NULL) {
        SDL_Log("GPU device creation failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_ClaimWindowForGPUDevice(device, window);

    set_SDL_gpu_device(device);
    set_SDL_main_window(window);

    SDL_GPUShader* vertexShader = create_vertex_shader(STRING("../shaders/rectangle.vert"), STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    SDL_GPUShader* fragmentShader = create_fragment_shader(STRING("../shaders/rectangle.frag"), STRING("main"), SHADER_COMPILATION_GLSL_PATH);

    GraphicsPipelineFactory pipelineFactory;
    graphics_pipeline_factory_init(&pipelineFactory);
    graphics_pipeline_factory_append_vertex_buffer_description(&pipelineFactory, SDL_GPU_VERTEXINPUTRATE_VERTEX, 3 * sizeof(float));
    graphics_pipeline_factory_append_vertex_atribute(&pipelineFactory, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
    graphics_pipeline_factory_append_color_target_description_default(&pipelineFactory, SDL_GetGPUSwapchainTextureFormat(get_SDL_gpu_device(), get_SDL_main_window()));
    graphicsPipeline = graphics_pipeline_factory_generate_pipeline(&pipelineFactory, vertexShader, fragmentShader);
    graphics_pipeline_factory_destroy(&pipelineFactory);

    SDL_ReleaseGPUShader(get_SDL_gpu_device(), vertexShader);
    SDL_ReleaseGPUShader(get_SDL_gpu_device(), fragmentShader);

    GPB_init();
    rectangles = (ScalableRectangle*)SDL_malloc(rectanglesLen * sizeof(ScalableRectangle));

    for (int i = 0; i < rectanglesLen; i++) {
        meshobject_init(&rectangles[i].rect);
        meshobject_load_objfile(&rectangles[i].rect, STRING("../objects/Quad.obj"));
        rectangles[i].xScale = 1.0f / (float)rectanglesLen;
        rectangles[i].xOffset = (-1.0f + rectangles[i].xScale) + (2.0f * (float)i * rectangles[i].xScale);
        rectangles[i].color = (SDL_FColor){SDL_randf(), SDL_randf(), SDL_randf(), SDL_randf()};
    }
    GPB_submit_all_transfer_buffers();

    // Disable VSYNC, if possible (not gauranteed).
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
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());

    SDL_GPUTexture* swapchainTexture;
    Uint32 width, height;
    SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, get_SDL_main_window(), &swapchainTexture, &width, &height);
    if (swapchainTexture == NULL) {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return SDL_APP_CONTINUE;
    }

    SDL_GPUColorTargetInfo colorTargetInfo = {0};
    colorTargetInfo.clear_color = (SDL_FColor){0.45f, 0.54f, 0.76f, 1.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorTargetInfo.texture = swapchainTexture;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

    SDL_BindGPUGraphicsPipeline(renderPass, graphicsPipeline);

    RectangleVertexUniform vertexUniformData;
    RectangleFragmentUniform fragmentUniformData;
    for (int i = 0; i < rectanglesLen; i++) {
        vertexUniformData.xOffset = rectangles[i].xOffset;
        vertexUniformData.xScale = rectangles[i].xScale;
        
        fragmentUniformData.color = rectangles[i].color;

        SDL_PushGPUVertexUniformData(commandBuffer, 0, &vertexUniformData, sizeof(vertexUniformData));
        SDL_PushGPUFragmentUniformData(commandBuffer, 0, &fragmentUniformData, sizeof(fragmentUniformData));

        meshobject_render(&rectangles[i].rect, renderPass);
    }

    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    for (int i = 0; i < rectanglesLen; i++) {
        meshobject_destroy(&rectangles[i].rect);
    }
    SDL_free(rectangles);

    GPB_terminate();
    SDL_ReleaseGPUGraphicsPipeline(get_SDL_gpu_device(), graphicsPipeline);

    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}