#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "engine.h"

GraphicsPipeline graphicsPipeline;
Material objMat;
RenderObject testObj;

Uint64 perfFrequency = 0;
Uint64 perfCounterPrev = 0;
float appTime = 0.0;

Camera cam = CAMERA_DEFAULT;

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
    graphics_pipeline_attach_vertex_shader(&graphicsPipeline, &STRING("../shaders/example5.vert"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_attach_fragment_shader(&graphicsPipeline, &STRING("../shaders/example5.frag"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_generate(&graphicsPipeline);

    material_create(&objMat, &graphicsPipeline);
    uniform_buffer_set_vec(
        &objMat.uniform,
        material_get_handle(&objMat, &STRING("color")),
        (vec4){1.0f, 1.0f, 0.3f, 1.0f},
        4
    );

    GPB_init();

    render_object_create(&testObj, &graphicsPipeline, &objMat);
    meshobject_load_objfile(&testObj.mesh, STRING("../objects/Atom.obj"));
    
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

    int windowWidth, windowHeight;
    SDL_GetWindowSizeInPixels(get_SDL_main_window(), &windowWidth, &windowHeight);

    float mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    mouseX = (mouseX / (float)windowWidth) * 2.0f - 1.0f;
    mouseY = -((mouseY / (float)windowHeight) * 2.0f - 1.0f);

    vec3 tempDir = {0.0f, 0.0f, 1.0f};
    glm_vec3_rotate(tempDir, mouseX * SDL_PI_F, (vec3){0.0f, 1.0f, 0.0f});
    glm_vec3_rotate(tempDir, mouseY * SDL_PI_F / 2.0f, (vec3){1.0f, 0.0f, 0.0f});
    glm_vec3_copy(tempDir, cam.direction.arr);
    glm_normalize(cam.direction.arr);

    float speed = 50.0f;
    vec3 tempVec;
    const bool* state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_W]) {
        glm_vec3_scale(cam.direction.arr, elapsed, tempVec);
        glm_vec3_add(cam.position.arr, tempVec, cam.position.arr);
    } else if (state[SDL_SCANCODE_S]) {
        glm_vec3_scale(cam.direction.arr, -elapsed, tempVec);
        glm_vec3_add(cam.position.arr, tempVec, cam.position.arr);
    }
    
    vec3 camRight;
    glm_vec3_cross(cam.direction.arr, cam.up.arr, camRight);
    glm_normalize(camRight);

    if (state[SDL_SCANCODE_A]) {
        glm_vec3_scale(camRight, elapsed, tempVec);
        glm_vec3_add(cam.position.arr, tempVec, cam.position.arr);
    } else if (state[SDL_SCANCODE_D]) {
        glm_vec3_scale(camRight, -elapsed, tempVec);
        glm_vec3_add(cam.position.arr, tempVec, cam.position.arr);
    }

    if (state[SDL_SCANCODE_SPACE]) {
        glm_vec3_scale(cam.up.arr, elapsed, tempVec);
        glm_vec3_add(cam.position.arr, tempVec, cam.position.arr);
    } else if (state[SDL_SCANCODE_LSHIFT]) {
        glm_vec3_scale(cam.up.arr, -elapsed, tempVec);
        glm_vec3_add(cam.position.arr, tempVec, cam.position.arr);
    }

    uniform_buffer_set_float(
        &graphicsPipeline.vertexFrameData,
        graphics_pipeline_vertex_get_handle(&graphicsPipeline, &STRING("aspectRatio")),
        (float)windowHeight / (float)windowWidth
    );

    uniform_buffer_set_vec(
        &objMat.uniform,
        material_get_handle(&objMat, &STRING("color")),
        (vec4){SDL_sinf((float)SDL_GetTicks() * 0.001f), SDL_sinf((float)SDL_GetTicks() * 0.001f - 1.2), SDL_sinf((float)SDL_GetTicks() * 0.001f - 2.2), 1.0f},
        4
    );

    render_queue_init(&rQueue, &cam, (float)windowWidth / (float)windowHeight);

    render_queue_add(&rQueue, &testObj);

    render_queue_submit(&rQueue);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    render_object_destroy(&testObj);
    material_destroy(&objMat);
    graphics_pipeline_destroy(&graphicsPipeline);

    GPB_terminate();

    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}