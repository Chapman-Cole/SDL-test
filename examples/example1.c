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
#include "ShaderUniformLayout.h"
#include "engine.h"

// To compile with debug symbols on linux, do cmake -DCMAKE_BUILD_TYPE=Debug ..
// For debug symbols on windows, cmake --build . --config Debug

// gcc src/main.c src/GPUBuffers.c src/GraphicsPipeline.c src/MeshObject.c src/SDLDevice.c src/Shader.c src/Strings.c src/Window.c -Iinclude/ -lSDL3 -lm -lSPIRV-Tools-opt -lSPIRV-Tools -lglslang -lshaderc_combined -lm -fsanitize=address -o build/main

/*
typedef struct UniformParams {
    float u_scale;
    float offset;
    float xScaling;
    int mode;
    int shouldScaleX;
    float rippleScale;
    float mouseX;
    float mouseY;
} UniformParams;

typedef struct ColorParams {
    SDL_FColor col1;
    SDL_FColor col2;
    SDL_FColor col3;
} ColorParams;

Uint64 perfFrequency = 0;
Uint64 perfCounterPrev = 0;

// IMPORTANT NOTE: This globabl variable cannot be called "time" because then it will conflict
// with other libc/posix functions with the same name. This will result in absolutely horrnedous 
// undefined behavior that will leave you questioning your sanity. This has to do with the way symbol 
// tables work for ELF executables, as global variables and function names can become interposed and
// cause some truly awful bugs
float appTime = 0.0;
string RenderObjectPath;

GraphicsPipeline graphicsPipeline;
Mesh background;
Mesh FrontObject;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    string_init(&RenderObjectPath);
    if (argc > 1) {
        string_copy(&RenderObjectPath, &(string){.str = argv[1], .len = strlen(argv[1]), .__memsize = -1});
    } else {
        string_copy(&RenderObjectPath, &STRING("../objects/Flower.obj"));
    }

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

    set_SDL_gpu_device(device);
    set_SDL_main_window(window);

    GPB_init();
    meshobject_init(&background);
    meshobject_load_objfile(&background, STRING("../objects/SubdPlane.obj"));

    meshobject_init(&FrontObject);
    meshobject_load_objfile(&FrontObject, RenderObjectPath);
    GPB_submit_all_transfer_buffers();

    graphics_pipeline_init(&graphicsPipeline);
    graphics_pipeline_append_vertex_buffer_description(&graphicsPipeline, SDL_GPU_VERTEXINPUTRATE_VERTEX, 3 * sizeof(float));
    graphics_pipeline_append_vertex_attribute(&graphicsPipeline, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
    graphics_pipeline_append_color_target_description_default(&graphicsPipeline, SDL_GetGPUSwapchainTextureFormat(get_SDL_gpu_device(), get_SDL_main_window()));
    graphics_pipeline_attach_vertex_shader(&graphicsPipeline, &STRING("../shaders/vertex.glsl"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_attach_fragment_shader(&graphicsPipeline, &STRING("../shaders/fragment.glsl"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_generate(&graphicsPipeline);

    //SDL_Log("Vertex Uniform Layout:\n");
    //shader_uniform_elements_print(&graphicsPipeline.vertexLayout);

    //SDL_Log("\nFragment Uniform Layout:\n");
    //shader_uniform_elements_print(&graphicsPipeline.fragmentLayout);

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

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
    appTime += (float)elapsed;


    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());

    SDL_GPUTexture* swapchainTexture;
    Uint32 wdith, height;
    SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, get_SDL_main_window(), &swapchainTexture, &wdith, &height);
    if (swapchainTexture == NULL) {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return SDL_APP_CONTINUE;
    }

    SDL_GPUColorTargetInfo colorTargetInfo = {0};
    colorTargetInfo.clear_color = (SDL_FColor){255 / 255.0f, 219 / 255.0f, 187 / 255.0f, 255 / 255.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorTargetInfo.texture = swapchainTexture;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

    SDL_BindGPUGraphicsPipeline(renderPass, graphicsPipeline.graphicsPipeline);

    int windowWidth, windowHeight;
    SDL_GetWindowSizeInPixels(get_SDL_main_window(), &windowWidth, &windowHeight);

    UniformParams params = {0};
    SDL_GetMouseState(&params.mouseX, &params.mouseY);
    params.mouseX = (params.mouseX / (float)windowWidth) * 2.0f - 1.0f;
    params.mouseY = -((params.mouseY / (float)windowHeight) * 2.0f - 1.0f);
    params.u_scale = appTime;
    params.mode = 2;
    params.shouldScaleX = false;
    params.rippleScale = 0.8f;
    params.xScaling = (float)windowHeight / (float)windowWidth;
    params.offset = 0.6f;

    SDL_PushGPUVertexUniformData(commandBuffer, 0, &params, sizeof(params));
    SDL_PushGPUFragmentUniformData(commandBuffer, 0, &params, sizeof(params));
    SDL_PushGPUFragmentUniformData(commandBuffer, 1, &(ColorParams){
        .col1 = (SDL_FColor){3.0 / 255.0, 64.0 / 255.0, 120.0 / 255.0, 1.0},
        .col2 = (SDL_FColor){0.0, 31 / 255.0, 84.0 / 255.0, 1.0},
        .col3 = (SDL_FColor){10.0 / 255.0, 17.0 / 255.0, 40 / 255.0, 1.0}
    },
    sizeof(ColorParams)
    );
    meshobject_render(&background, renderPass);

    params.offset = -0.6f;
    params.shouldScaleX = true;
    params.rippleScale = 1.2f;
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &params, sizeof(params));
    SDL_PushGPUFragmentUniformData(commandBuffer, 0, &params, sizeof(params));
    SDL_PushGPUFragmentUniformData(commandBuffer, 1, &(ColorParams){
        .col1 = (SDL_FColor){255.0 / 255.0, 71.0 / 255.0, 76.0 / 255.0, 1.0},
        .col2 = (SDL_FColor){255.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0, 1.0},
        .col3 = (SDL_FColor){153.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0, 1.0}
    },
    sizeof(ColorParams)
    );
    meshobject_render(&FrontObject, renderPass);


    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    meshobject_destroy(&background);
    meshobject_destroy(&FrontObject);
    string_free(&RenderObjectPath);

    graphics_pipeline_destroy(&graphicsPipeline);

    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}

*/

Uint64 perfFrequency = 0;
Uint64 perfCounterPrev = 0;

// IMPORTANT NOTE: This globabl variable cannot be called "time" because then it will conflict
// with other libc/posix functions with the same name. This will result in absolutely horrnedous 
// undefined behavior that will leave you questioning your sanity. This has to do with the way symbol 
// tables work for ELF executables, as global variables and function names can become interposed and
// cause some truly awful bugs
float appTime = 0.0;
string RenderObjectPath;

GraphicsPipeline graphicsPipeline;

Material backgroundMat;
Material foregroundMat;

RenderObject background;
RenderObject foreground;

typedef struct RenderObjectParams {
    float offset;
    float xScaling;
    int mode;
    int shouldScaleX;
    float rippleScale;
    vec3 pad;
} RenderObjectParams;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    string_init(&RenderObjectPath);
    if (argc > 1) {
        string_copy(&RenderObjectPath, &(string){.str = argv[1], .len = strlen(argv[1]), .__memsize = -1});
    } else {
        string_copy(&RenderObjectPath, &STRING("../objects/Flower.obj"));
    }

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

    set_SDL_gpu_device(device);
    set_SDL_main_window(window);

    GPB_init();

    graphics_pipeline_init(&graphicsPipeline);
    graphics_pipeline_append_vertex_buffer_description(&graphicsPipeline, SDL_GPU_VERTEXINPUTRATE_VERTEX, 3 * sizeof(float));
    graphics_pipeline_append_vertex_attribute(&graphicsPipeline, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
    graphics_pipeline_append_color_target_description_default(&graphicsPipeline, SDL_GetGPUSwapchainTextureFormat(get_SDL_gpu_device(), get_SDL_main_window()));
    graphics_pipeline_attach_vertex_shader(&graphicsPipeline, &STRING("../shaders/vertex.glsl"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_attach_fragment_shader(&graphicsPipeline, &STRING("../shaders/fragment.glsl"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_generate(&graphicsPipeline);

    material_create(&backgroundMat, &graphicsPipeline);
    material_create(&foregroundMat, &graphicsPipeline);

    render_object_create(&background, &graphicsPipeline, &backgroundMat);
    render_object_create(&foreground, &graphicsPipeline, &foregroundMat);

    meshobject_load_objfile(&background.mesh, STRING("../objects/SubdPlane.obj"));

    meshobject_load_objfile(&foreground.mesh, RenderObjectPath);

    uniform_buffer_set_vec(
        &backgroundMat.uniform,
        material_get_handle(&backgroundMat, &STRING("col1")),
        (vec4){3.0f / 255.0f, 64.0f / 255.0f, 120.0f / 255.0f, 1.0f},
        4
    );

    uniform_buffer_set_vec(
        &backgroundMat.uniform,
        material_get_handle(&backgroundMat, &STRING("col2")),
        (vec4){0.0f, 31.0f / 255.0f, 84.0f / 255.0f, 1.0f},
        4
    );

    uniform_buffer_set_vec(
        &backgroundMat.uniform,
        material_get_handle(&backgroundMat, &STRING("col3")),
        (vec4){10.0f / 255.0f, 17.0f / 255.0f, 40.0f / 255.0f, 1.0f},
        4
    );

    uniform_buffer_set_vec(
        &foregroundMat.uniform,
        material_get_handle(&foregroundMat, &STRING("col1")),
        (vec4){255.0f / 255.0f, 71.0f / 255.0f, 76.0f / 255.0f, 1.0f},
        4
    );

    uniform_buffer_set_vec(
        &foregroundMat.uniform,
        material_get_handle(&foregroundMat, &STRING("col2")),
        (vec4){255.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0, 1.0},
        4
    );

    uniform_buffer_set_vec(
        &foregroundMat.uniform,
        material_get_handle(&foregroundMat, &STRING("col3")),
        (vec4){153.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 1.0f},
        4
    );

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

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
    appTime += (float)elapsed;

    RenderQueue rQueue;
    Camera2D cam = CAMERA2D_DEFAULT;
    render_queue_init2D(&rQueue, &cam);

    int windowWidth, windowHeight;
    SDL_GetWindowSizeInPixels(get_SDL_main_window(), &windowWidth, &windowHeight);

    float mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    // Vertex Frame Data
    uniform_buffer_set_vec(
        &graphicsPipeline.vertexFrameData,
        graphics_pipeline_vertex_get_handle(&graphicsPipeline, &STRING("mouse")),
        (vec2){(mouseX / (float)windowWidth) * 2.0f - 1.0f, -((mouseY / (float)windowHeight) * 2.0f - 1.0f)},
        2
    );

    uniform_buffer_set_float(
        &graphicsPipeline.vertexFrameData,
        graphics_pipeline_vertex_get_handle(&graphicsPipeline, &STRING("time")),
        appTime
    );

    // Fragment Frame Data
    uniform_buffer_set_vec(
        &graphicsPipeline.fragmentFrameData,
        graphics_pipeline_fragment_get_handle(&graphicsPipeline, &STRING("mouse")),
        (vec2){(mouseX / (float)windowWidth) * 2.0f - 1.0f, -((mouseY / (float)windowHeight) * 2.0f - 1.0f)},
        2
    );

    uniform_buffer_set_float(
        &graphicsPipeline.fragmentFrameData,
        graphics_pipeline_fragment_get_handle(&graphicsPipeline, &STRING("time")),
        appTime
    );

    // background render object data
    RenderObjectParams bgParams = (RenderObjectParams){
        .mode = 2,
        .shouldScaleX = false,
        .rippleScale = 0.8f,
        .xScaling = (float)windowHeight / (float)windowWidth,
        .offset = 0.6f,
    };
    SDL_memcpy(background.vertexUniform.uniform, &bgParams, sizeof(bgParams));
    SDL_memcpy(background.fragmentUniform.uniform, &bgParams, sizeof(bgParams));

    RenderObjectParams fgParams = (RenderObjectParams){
        .mode = 2,
        .offset = -0.6f,
        .shouldScaleX = true,
        .rippleScale = 1.2f,
        .xScaling = (float)windowHeight / (float)windowWidth
    };
    SDL_memcpy(foreground.vertexUniform.uniform, &fgParams, sizeof(fgParams));
    SDL_memcpy(foreground.fragmentUniform.uniform, &fgParams, sizeof(fgParams));

    render_queue_add(&rQueue, &background);
    render_queue_add(&rQueue, &foreground);

    render_queue_submit(&rQueue);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    render_object_destroy(&background);
    render_object_destroy(&foreground);

    material_destroy(&backgroundMat);
    material_destroy(&foregroundMat);

    string_free(&RenderObjectPath);

    graphics_pipeline_destroy(&graphicsPipeline);

    GPB_terminate();

    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}