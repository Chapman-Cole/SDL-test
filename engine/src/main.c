/*
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "GPUBuffers.h"
#include "GraphicsPipeline.h"
#include "MeshObject.h"
#include "SDLDevice.h"
#include "Shader.h"
#include "Strings.h"
#include <cglm/cglm.h>
#include "Window.h"

// For arch linux, use MANGOHUD=1 ./main to get fps info since something recently
// changed about how SDL3 handles Vulkan hooks that breaks the normal usage of
// mangohud. This could also be an issue with vulkan-icd-loader changes, as asan
// has been flagging that recently

// To compile with debug symbols on linux, do cmake -DCMAKE_BUILD_TYPE=Debug ..
// For debug symbols on windows, cmake --build . --config Debug

// gcc src/main.c src/GPUBuffers.c src/GraphicsPipeline.c src/MeshObject.c src/SDLDevice.c src/Shader.c src/Strings.c src/Window.c -Iinclude/ -lSDL3 -lm -lSPIRV-Tools-opt -lSPIRV-Tools -lglslang -lshaderc_combined -lm -fsanitize=address -o build/main

typedef struct UniformParams {
    float u_scale;
    float offset;
    float xScaling;
    int mode;
    int shouldScaleX;
    float rippleScale;
    float pad1;
    float pad2;
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

string RenderObjectPath;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("SDL-test", 960, 540, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        SDL_Log("Window Creation Failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (device == NULL) {
        SDL_Log("GPU device creation failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_ClaimWindowForGPUDevice(device, window);

    string_init(&RenderObjectPath);
    if (argc > 1) {
        string_copy(&RenderObjectPath, &(string){.str = argv[1], .len = strlen(argv[1]), .__memsize = -1});
    } else {
        string_copy(&RenderObjectPath, &STRING("../objects/Flower.obj"));
    }

    perfFrequency = SDL_GetPerformanceFrequency();
    perfCounterPrev = SDL_GetPerformanceCounter();

    // Set the sdl gpu device for all modules, and initialize gpb (gpu buffers)
    set_SDL_gpu_device(device);
    set_SDL_main_window(window);
    GPB_init();

    // Create the quad mesh
    meshobject_init(&quadMesh);
    meshobject_load_objfile(&quadMesh, RenderObjectPath);

    meshobject_init(&quadMesh2);
    meshobject_load_objfile(&quadMesh2, STRING("../objects/SubdPlane.obj"));

    GPB_submit_all_transfer_buffers();

    // Create the vertex shader
    SDL_GPUShader* vertexShader = create_vertex_shader(STRING("../shaders/vertex.glsl"), STRING("main"), false);

    // Create the fragment shader
    SDL_GPUShader* fragmentShader = create_fragment_shader(STRING("../shaders/fragment.glsl"), STRING("main"), false);

    graphics_pipeline_factory_registry_init();
    graphicsPipeline = graphics_pipeline_factory_registry_generate_pipeline(STRING("default"), vertexShader, fragmentShader);

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

    // Determines how the vertex shader affects the vertices
    params.mode = 0;
    params.shouldScaleX = false;
    params.rippleScale = 1.0f;
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

    params.u_scale = time;
    params.offset = 0.8;

    //Determines how the fragment shader will modify the vertices
    params.mode = 2;
    params.shouldScaleX = true;
    params.rippleScale = 1.2f;

    int windowWidth, windowHeight;
    SDL_GetWindowSizeInPixels(window, &windowWidth, &windowHeight);
    params.xScaling = (float)windowHeight / (float)windowWidth;
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &params, sizeof(params));
    meshobject_render(&quadMesh, renderPass);

    // End the render pass before submitting the command buffer
    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    string_free(&RenderObjectPath);

    graphics_pipeline_factory_registry_terminate();
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
*/

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_shadercross/SDL_shadercross.h>

// the vertex input layout
struct Vertex
{
    float x, y, z;      //vec3 position
    float r, g, b, a;   //vec4 color
};

// a list of vertices
static struct Vertex vertices[] = {
    {0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},     // top vertex
    {-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f},   // bottom left vertex
    {0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}     // bottom right vertex
};

struct UniformBuffer
{
    float time;
    // you can add other properties here
};

static struct UniformBuffer timeUniform = {0};

SDL_Window* window;
SDL_GPUDevice* device;
SDL_GPUBuffer* vertexBuffer;
SDL_GPUTransferBuffer* transferBuffer;
SDL_GPUGraphicsPipeline* graphicsPipeline;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    
    // create a window
    window = SDL_CreateWindow("Hello, Triangle!", 960, 540, SDL_WINDOW_RESIZABLE);
    
    // create the device
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
    SDL_ClaimWindowForGPUDevice(device, window);
    
    // load the vertex shader code
    size_t vertexCodeSize; 
    void* vertexCode = SDL_LoadFile("vertex.spv", &vertexCodeSize);

    // create the vertex shader
    SDL_GPUShaderCreateInfo vertexInfo = {0};
    vertexInfo.code = (Uint8*)vertexCode;
    vertexInfo.code_size = vertexCodeSize;
    vertexInfo.entrypoint = "main";
    vertexInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertexInfo.num_samplers = 0;
    vertexInfo.num_storage_buffers = 0;
    vertexInfo.num_storage_textures = 0;
    vertexInfo.num_uniform_buffers = 0;

    SDL_GPUShader* vertexShader = SDL_CreateGPUShader(device, &vertexInfo);

    // free the file
    SDL_free(vertexCode);

    // load the fragment shader code
    size_t fragmentCodeSize; 
    void* fragmentCode = SDL_LoadFile("fragment.spv", &fragmentCodeSize);

    // create the fragment shader
    SDL_GPUShaderCreateInfo fragmentInfo = {0};
    fragmentInfo.code = (Uint8*)fragmentCode;
    fragmentInfo.code_size = fragmentCodeSize;
    fragmentInfo.entrypoint = "main";
    fragmentInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragmentInfo.num_samplers = 0;
    fragmentInfo.num_storage_buffers = 0;
    fragmentInfo.num_storage_textures = 0;
    fragmentInfo.num_uniform_buffers = 1;

    SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(device, &fragmentInfo);

    // free the file
    SDL_free(fragmentCode);

    // create the graphics pipeline
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    
    // describe the vertex buffers
    SDL_GPUVertexBufferDescription vertexBufferDesctiptions[1];
    vertexBufferDesctiptions[0].slot = 0;
    vertexBufferDesctiptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDesctiptions[0].instance_step_rate = 0;
    vertexBufferDesctiptions[0].pitch = sizeof(struct Vertex);

    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = vertexBufferDesctiptions;

    // describe the vertex attribute
    SDL_GPUVertexAttribute vertexAttributes[2];

    // a_position
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].location = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[0].offset = 0;

    // a_color
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].location = 1;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertexAttributes[1].offset = sizeof(float) * 3;

    pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
    pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

    // describe the color target
    SDL_GPUColorTargetDescription colorTargetDescriptions[1] = {0};
    colorTargetDescriptions[0] = (SDL_GPUColorTargetDescription){0};
    colorTargetDescriptions[0].format = SDL_GetGPUSwapchainTextureFormat(device, window);

    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

    // create the pipeline
    graphicsPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);

    // we don't need to store the shaders after creating the pipeline
    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentShader);

    // create the vertex buffer
    SDL_GPUBufferCreateInfo bufferInfo = {0};
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);

    // create a transfer buffer to upload to the vertex buffer
    SDL_GPUTransferBufferCreateInfo transferInfo = {0};
    transferInfo.size = sizeof(vertices);
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

    // fill the transfer buffer
    struct Vertex* data = (struct Vertex*)SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    
    SDL_memcpy(data, (void*)vertices, sizeof(vertices));

    // data[0] = vertices[0];
    // data[1] = vertices[1];
    // data[2] = vertices[2];

    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    // start a copy pass
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    // where is the data
    SDL_GPUTransferBufferLocation location = {0};
    location.transfer_buffer = transferBuffer;
    location.offset = 0;
    
    // where to upload the data
    SDL_GPUBufferRegion region = {0};
    region.buffer = vertexBuffer;
    region.size = sizeof(vertices);
    region.offset = 0;

    // upload the data
    SDL_UploadToGPUBuffer(copyPass, &location, &region, true);

    // end the copy pass
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    // acquire the command buffer
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);

    // get the swapchain texture
    SDL_GPUTexture* swapchainTexture;
    Uint32 width, height;
    SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &width, &height);

    // end the frame early if a swapchain texture is not available
    if (swapchainTexture == NULL)
    {
        // you must always submit the command buffer
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return SDL_APP_CONTINUE;
    }

    // create the color target
    SDL_GPUColorTargetInfo colorTargetInfo = {0};
    colorTargetInfo.clear_color = (SDL_FColor){240/255.0f, 240/255.0f, 240/255.0f, 255/255.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorTargetInfo.texture = swapchainTexture;

    // begin a render pass
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

    // bind the pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, graphicsPipeline);
    
    // bind the vertex buffer
    SDL_GPUBufferBinding bufferBindings[1];
    bufferBindings[0].buffer = vertexBuffer;
    bufferBindings[0].offset = 0;
    
    SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, 1);
    
    // update the time uniform
    timeUniform.time = SDL_GetTicksNS() / 1e9f;
    SDL_PushGPUFragmentUniformData(commandBuffer, 0, &timeUniform, sizeof(struct UniformBuffer));

    // issue a draw call
    SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

    // end the render pass
    SDL_EndGPURenderPass(renderPass);

    // submit the command buffer
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    // close the window on request
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    // release buffers
    SDL_ReleaseGPUBuffer(device, vertexBuffer);
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    // release the pipeline
    SDL_ReleaseGPUGraphicsPipeline(device, graphicsPipeline);

    // destroy the GPU device
    SDL_DestroyGPUDevice(device);

    // destroy the window
    SDL_DestroyWindow(window);
}