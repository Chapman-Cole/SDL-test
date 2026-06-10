#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "engine.h"

GraphicsPipeline graphicsPipeline;
Material objMat;
Material objMat2;
RenderObject testObj;
RenderObject secondObj;

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
    graphics_pipeline_attach_vertex_shader(&graphicsPipeline, &STRING("../../shaders/example3/test.vert"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_attach_fragment_shader(&graphicsPipeline, &STRING("../../shaders/example3/test.frag"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_generate(&graphicsPipeline);

    material_create(&objMat, &graphicsPipeline);
    uniform_buffer_set_vec(
        &objMat.uniform,
        material_get_handle(&objMat, &STRING("col")),
        (vec4){1.0f, 1.0f, 0.3f, 1.0f},
        4
    );

    material_create(&objMat2, &graphicsPipeline);
    uniform_buffer_set_vec(
        &objMat2.uniform,
        material_get_handle(&objMat2, &STRING("col")),
        (vec4){0.0f, 1.0f, 0.2f, 1.0f},
        4
    );

    GPB_init();

    render_object_create(&testObj, &graphicsPipeline, &objMat);
    meshobject_load_objfile(&testObj.mesh, STRING("../../objects/Quad.obj"));

    render_object_create(&secondObj, &graphicsPipeline, &objMat2);
    meshobject_load_objfile(&secondObj.mesh, STRING("../../objects/Quad.obj"));

    testObj.scale[0] = 0.5;
    testObj.scale[1] = 0.5;

    testObj.pos[0] = 0.2;
    testObj.pos[1] = 0.2;

    glm_quat(testObj.quaternion, SDL_PI_F / 4.0f, 0.0f, 0.0f, 1.0f);

    secondObj.scale[0] = 0.2;
    secondObj.scale[1] = 0.2;

    secondObj.pos[0] = 0.2;
    secondObj.pos[1] = 0.2;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    RenderQueue rQueue;
    Camera2D cam = CAMERA2D_DEFAULT;

    int windowWidth, windowHeight;
    SDL_GetWindowSizeInPixels(get_SDL_main_window(), &windowWidth, &windowHeight);

    render_queue_init2D(&rQueue, &cam, (float)windowWidth / (float)windowHeight);

    render_queue_add(&rQueue, &testObj);
    render_queue_add(&rQueue, &secondObj);

    render_queue_submit(&rQueue, NULL, 0, 0, false);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    render_object_destroy(&testObj);
    render_object_destroy(&secondObj);
    material_destroy(&objMat);
    material_destroy(&objMat2);
    graphics_pipeline_destroy(&graphicsPipeline);

    GPB_terminate();

    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}