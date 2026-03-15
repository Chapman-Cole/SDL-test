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
    int id;
    int numBins;
    float aspectRatio;
    float pad[2];
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

GraphicsPipeline graphicsPipeline;

SDL_Thread* audioThread;

// Get the pipewire audio implementation
#include "FetchSysAudio.c"

ScalableRectangle* rectangles = NULL;
Uint32 rectanglesLen = BIN_SIZE;

MainTInput* tempInput = NULL;

Uint64 perfFrequency = 0;
Uint64 perfCounterPrev = 0;

float waveformTime = 0.0;

// Zero memory initially
float prevAudioBuffer[BIN_SIZE] = {0};
float currAudioBuffer[BIN_SIZE] = {0};

#define WAVEFORM_RESPONSEIVENESS 0.1

#define RGB_F(x) (((float)x) / 255.0f)

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float smoothstep_basic(float t) {
    // The core cubic polynomial: 3*t^2 - 2*t^3
    return t * t * (3.0f - 2.0f * t);
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
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

    set_SDL_gpu_device(device);
    set_SDL_main_window(window);

    graphics_pipeline_init(&graphicsPipeline);
    graphics_pipeline_append_vertex_buffer_description(&graphicsPipeline, SDL_GPU_VERTEXINPUTRATE_VERTEX, 3 * sizeof(float));
    graphics_pipeline_append_vertex_attribute(&graphicsPipeline, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
    graphics_pipeline_append_color_target_description_default(&graphicsPipeline, SDL_GetGPUSwapchainTextureFormat(get_SDL_gpu_device(), get_SDL_main_window()));
    graphics_pipeline_attach_vertex_shader(&graphicsPipeline, &STRING("../shaders/rectangle.vert"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_attach_fragment_shader(&graphicsPipeline, &STRING("../shaders/rectangle.frag"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_generate(&graphicsPipeline);

    GPB_init();
    rectangles = (ScalableRectangle*)SDL_malloc(rectanglesLen * sizeof(ScalableRectangle));

    for (int i = 0; i < rectanglesLen; i++) {
        meshobject_init(&rectangles[i].rect);
        meshobject_load_objfile(&rectangles[i].rect, STRING("../objects/Quad.obj"));
        rectangles[i].xScale = 1.0f / (float)rectanglesLen;
        rectangles[i].xOffset = (-1.0f + rectangles[i].xScale) + (2.0f * (float)i * rectangles[i].xScale);
        rectangles[i].color = (SDL_FColor){RGB_F(100), RGB_F(149), RGB_F(237), 1.0f};
    }
    GPB_submit_all_transfer_buffers();

    // Disable VSYNC, if possible (not gauranteed).
    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    tempInput = (MainTInput*)SDL_malloc(sizeof(MainTInput));
    if (argc > 2) {
        *tempInput = (MainTInput){.objName = argv[1]};
    } else {
        *tempInput = (MainTInput){.objName = "spotify"};
    }
    audioThread = SDL_CreateThread(FetchAudioMainT, "AudioMainT", tempInput);
    if (!audioThread) {
        SDL_Log("Failed to spawn audio thread\n");
        SDL_Quit();
        exit(-1);
    }

    // The thread will clean itself up
    SDL_DetachThread(audioThread);

    perfCounterPrev = SDL_GetPerformanceCounter();
    perfFrequency = SDL_GetPerformanceFrequency();

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
    perfFrequency = SDL_GetPerformanceFrequency();
    double elapsed = (double)(perfCounterNow - perfCounterPrev) / (double)perfFrequency;
    perfCounterPrev = perfCounterNow;
    waveformTime += (float)elapsed;

    if (waveformTime >= WAVEFORM_RESPONSEIVENESS) {
        SDL_memcpy(prevAudioBuffer, currAudioBuffer, BIN_SIZE * sizeof(float));
        SDL_memcpy(currAudioBuffer, visualizerBars, BIN_SIZE * sizeof(float));
        waveformTime = 0.0f;
    }

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());

    SDL_GPUTexture* swapchainTexture;
    Uint32 width, height;
    SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, get_SDL_main_window(), &swapchainTexture, &width, &height);
    if (swapchainTexture == NULL) {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return SDL_APP_CONTINUE;
    }

    SDL_GPUColorTargetInfo colorTargetInfo = {0};
    colorTargetInfo.clear_color = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorTargetInfo.texture = swapchainTexture;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

    SDL_BindGPUGraphicsPipeline(renderPass, graphicsPipeline.graphicsPipeline);

    int windowWidth, windowHeight;
    SDL_GetWindowSizeInPixels(get_SDL_main_window(), &windowWidth, &windowHeight);

    RectangleVertexUniform vertexUniformData;
    vertexUniformData.aspectRatio = (float)windowHeight / (float)windowWidth;
    RectangleFragmentUniform fragmentUniformData;
    for (int i = 0; i < rectanglesLen; i++) {
        vertexUniformData.xOffset = rectangles[i].xOffset;
        vertexUniformData.xScale = rectangles[i].xScale;
        vertexUniformData.id = i;
        vertexUniformData.numBins = rectanglesLen;
        if (currAudioBuffer[i] > prevAudioBuffer[i]) {
            vertexUniformData.yScale = lerp(prevAudioBuffer[i], currAudioBuffer[i], SDL_clamp(1.24f * waveformTime / WAVEFORM_RESPONSEIVENESS, 0.0, 1.0f));
        } else {
            vertexUniformData.yScale = lerp(prevAudioBuffer[i], currAudioBuffer[i], SDL_clamp(waveformTime / WAVEFORM_RESPONSEIVENESS, 0.0, 1.0f));
        }

        
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
    SDL_free(tempInput);

    for (int i = 0; i < rectanglesLen; i++) {
        meshobject_destroy(&rectangles[i].rect);
    }
    SDL_free(rectangles);

    GPB_terminate();
    
    graphics_pipeline_destroy(&graphicsPipeline);

    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}