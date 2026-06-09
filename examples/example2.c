#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "engine.h"

GraphicsPipeline graphicsPipeline;
Material objMat;
RenderObject basicObject;

GPUTexture texture;
SDL_GPUSampler* sampler;

Camera2D cam = CAMERA2D_DEFAULT;

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

    graphics_pipeline_init(&graphicsPipeline);
    graphics_pipeline_append_vertex_buffer_description(&graphicsPipeline, SDL_GPU_VERTEXINPUTRATE_VERTEX, 3 * sizeof(float));
    graphics_pipeline_append_vertex_attribute(&graphicsPipeline, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
    graphics_pipeline_append_color_target_description_default(&graphicsPipeline, SDL_GetGPUSwapchainTextureFormat(get_SDL_gpu_device(), get_SDL_main_window()));
    graphics_pipeline_attach_vertex_shader(&graphicsPipeline, &STRING("../../shaders/example2/vert.glsl"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_attach_fragment_shader(&graphicsPipeline, &STRING("../../shaders/example2/frag.glsl"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_generate(&graphicsPipeline);

    material_create(&objMat, &graphicsPipeline);

    GPB_init();

    render_object_create(&basicObject, &graphicsPipeline, &objMat);
    meshobject_load_objfile(&basicObject.mesh, STRING("../../objects/Quad.obj"));

    GPUTexture_init(&texture);
    GPUTexture_load(&texture, &STRING("../../img/JediOrder.jpg"), true);

    sampler = SDL_CreateGPUSampler(
        get_SDL_gpu_device(),
        &(SDL_GPUSamplerCreateInfo){
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .mip_lod_bias = 0.0f,
            .max_anisotropy = 0.4f,
            .compare_op = SDL_GPU_COMPAREOP_INVALID,
            .min_lod = 0.0f,
            .max_lod = 4.0f,
            .enable_anisotropy = false,
            .enable_compare = false,
            .props = 0
        }
    );

    material_append_texture_sampler_pair(&objMat, texture.texture, sampler);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    int windowWidth, windowHeight;
    SDL_GetWindowSizeInPixels(get_SDL_main_window(), &windowWidth, &windowHeight);

    uniform_buffer_set_float(
        &graphicsPipeline.fragmentFrameData,
        graphics_pipeline_fragment_get_handle(&graphicsPipeline, &STRING("time")),
        (float)SDL_GetTicksNS() * 1e-9
    );

    RenderQueue rQueue;
    render_queue_init2D(&rQueue, &cam, (float)windowWidth / (float)windowHeight);

    render_queue_add(&rQueue, &basicObject);

    render_queue_submit(&rQueue, NULL, 0, 0, false);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    GPUTexture_destroy(&texture);
    SDL_ReleaseGPUSampler(get_SDL_gpu_device(), sampler);
    render_object_destroy(&basicObject);
    material_destroy(&objMat);
    graphics_pipeline_destroy(&graphicsPipeline);
    GPB_terminate();
    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}