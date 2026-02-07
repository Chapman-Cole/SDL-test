#define SDL_MAIN_USE_CALLBACKS 1
#include "GPUBuffers.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "SDLDevice.h"
#include "Shader.h"
#include "Strings.h"
#include "GraphicsPipeline.h"
#include <cglm/cglm.h>

typedef struct Vertex {
    float x, y, z;
    float r, g, b, a;
} Vertex;

typedef struct UniformParams {
    float u_scale;
    float pad[3]; // 16 bytes total
} UniformParams;

static Vertex vertices[] = {
    {-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // Bottom left
    {1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  // Bottom right
    {-1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  // Top left
    {1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}    // Top right
};

static Uint16 indices[] = {
    0, 1, 2,
    1, 3, 2};

SDL_Window* window = NULL;
SDL_GPUDevice* device = NULL;

SDL_GPUGraphicsPipeline* graphicsPipeline = NULL;

SDL_GPUBuffer* vertexBuffer = NULL;

SDL_GPUBuffer* indexBuffer = NULL;

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
    GPB_init();

    // Create the vertex and index buffers
    vertexBuffer = GPB_create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, vertices, sizeof(vertices));
    indexBuffer = GPB_create_buffer(SDL_GPU_BUFFERUSAGE_INDEX, indices, sizeof(indices));
    GPB_submit_all_transfer_buffers();

    // Create the vertex shader
    SDL_GPUShader* vertexShader = create_vertex_shader(STRING("../shaders/vertex.glsl"), STRING("main"));

    // Create the fragment shader
    SDL_GPUShader* fragmentShader = create_fragment_shader(STRING("../shaders/fragment.glsl"), STRING("main"));

    GraphicsPipelineFactory pipelineFactory;
    graphics_pipeline_factory_init(&pipelineFactory);

    // Setup the graphics pipeline parameters
    graphics_pipeline_factory_set_shaders(&pipelineFactory, vertexShader, fragmentShader);
    graphics_pipeline_factory_append_vertex_buffer_description(&pipelineFactory, SDL_GPU_VERTEXINPUTRATE_VERTEX, sizeof(Vertex));
    graphics_pipeline_factory_append_vertex_atribute(&pipelineFactory, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
    graphics_pipeline_factory_append_vertex_atribute(&pipelineFactory, 0, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, sizeof(float) * 3);
    graphics_pipeline_factory_append_color_target_description_default(&pipelineFactory, SDL_GetGPUSwapchainTextureFormat(get_SDL_gpu_device(), window));
    graphicsPipeline = graphics_pipeline_factory_generate_pipeline(&pipelineFactory);

    graphics_pipeline_factory_destroy(&pipelineFactory);

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

    // bind the vertex buffer
    SDL_GPUBufferBinding bufferBindings[1];
    bufferBindings[0].buffer = vertexBuffer; // index 0 is slot 0 in this example
    bufferBindings[0].offset = 0;            // start from the first byte

    SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, 1); // bind one buffer starting from slot 0

    // bind the index buffer
    SDL_GPUBufferBinding bufferBindings2;
    bufferBindings2.buffer = indexBuffer;
    bufferBindings2.offset = 0;

    SDL_BindGPUIndexBuffer(renderPass, &bufferBindings2, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    // Pass data to the uniform
    UniformParams params = {0};
    params.u_scale = time;
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &params, sizeof(params));
    SDL_PushGPUFragmentUniformData(commandBuffer, 0, &params, sizeof(params));

    mat4 trans;
    glm_mat4_identity(trans);
    glm_rotate(trans, time / 2.0f, (vec3){0.0f, 0.0f, 1.0f});
    glm_scale(trans, (vec3){0.5, 0.5, 0.5});

    SDL_PushGPUVertexUniformData(commandBuffer, 1, trans, sizeof(mat4));

    // issue a draw call
    // SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);
    SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);

    // End the render pass before submitting the command buffer
    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    GPB_terminate();

    // Release buffers since they are no longer needed
    SDL_ReleaseGPUBuffer(device, vertexBuffer);
    // SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    SDL_ReleaseGPUBuffer(device, indexBuffer);
    // SDL_ReleaseGPUTransferBuffer(device, transferBuffer2);

    // release the pipeline
    SDL_ReleaseGPUGraphicsPipeline(device, graphicsPipeline);

    // destroy the gpu device
    SDL_DestroyGPUDevice(device);

    // destory the window
    SDL_DestroyWindow(window);
}
