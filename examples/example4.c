#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "engine.h"

typedef struct Particle {
    RenderObject robj;
    vec3 velocity;
} Particle;


GraphicsPipeline graphicsPipeline;
Material objMat;

#define NUM_PARTICLES 5000
Particle particles[NUM_PARTICLES];

Uint64 perfFrequency = 0;
Uint64 perfCounterPrev = 0;
float appTime = 0.0;

Camera2D cam = CAMERA2D_DEFAULT;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Window* window = NULL;
    window = SDL_CreateWindow("Example 4", 960, 540, SDL_WINDOW_RESIZABLE);
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
    graphics_pipeline_attach_vertex_shader(&graphicsPipeline, &STRING("../shaders/example4.vert"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_attach_fragment_shader(&graphicsPipeline, &STRING("../shaders/example4.frag"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_generate(&graphicsPipeline);

    material_create(&objMat, &graphicsPipeline);
    uniform_buffer_set_vec(
        &objMat.uniform,
        material_get_handle(&objMat, &STRING("col")),
        (vec4){1.0f, 1.0f, 1.0f, 1.0f},
        4
    );
    
    GPB_init();

    for (int i = 0; i < NUM_PARTICLES; i++) {
        render_object_create(&particles[i].robj, &graphicsPipeline, &objMat);
        meshobject_load_objfile(&particles[i].robj.mesh, STRING("../objects/Quad.obj"));
        particles[i].robj.scale.x = 0.002;
        particles[i].robj.scale.y = 0.004;

        glm_vec3_copy((vec3){2 * (SDL_randf() - 0.5), 2 * (SDL_randf() - 0.5)}, particles[i].robj.position.arr);
        //glm_vec3_copy((vec3){0.7 * 2.0 * (SDL_randf() - 0.5), 0.7 * 2.0 * (SDL_randf() - 0.5), 0.0f}, particles[i].velocity);
        particles[i].velocity[0] = 1.0f;
        particles[i].velocity[1] = 1.0f;
        particles[i].velocity[2] = 1.0f;
        glm_vec3_rotate(particles[i].velocity, 2.0f * SDL_PI_F * SDL_randf(), (vec3){0.0f, 0.0f, 1.0f});
        vec3 tempVec;
        glm_vec3_copy(particles[i].velocity, tempVec);
        glm_vec3_scale(tempVec, 0.7f * SDL_sqrt(SDL_randf()), particles[i].velocity);


        if (particles[i].velocity[0] == 0.0f) {
            particles[i].velocity[0] = 0.4f;
        }

        if (particles[i].velocity[1] == 0.0f) {
            particles[i].velocity[1] = 0.4f;
        }
    }

    perfCounterPrev = SDL_GetPerformanceCounter();
    perfFrequency = SDL_GetPerformanceFrequency();

    cam.horizontalBounds.x = -2.0f;
    cam.horizontalBounds.y = 2.0f;

    return SDL_APP_CONTINUE;

}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        return SDL_APP_SUCCESS;
    } else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        // > 0 means scrolling up
        if (event->wheel.y > 0) {
            cam.zoom -= 0.04f;
        } else if (event->wheel.y < 0) {
            cam.zoom += 0.04f;
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    Uint64 perfCounterNow = SDL_GetPerformanceCounter();
    perfFrequency = SDL_GetPerformanceFrequency();
    double elapsed = (double)(perfCounterNow - perfCounterPrev) / (double)perfFrequency;
    perfCounterPrev = perfCounterNow;
    appTime += (float)elapsed;

    int windowWidth, windowHeight;
    SDL_GetWindowSizeInPixels(get_SDL_main_window(), &windowWidth, &windowHeight);

    RenderQueue rQueue;
    render_queue_init2D(&rQueue, &cam, (float)windowWidth / (float)windowHeight);

    const bool* state = SDL_GetKeyboardState(NULL);
    float speed = 5.0f;
    if (state[SDL_SCANCODE_UP]) {
        cam.position.y += speed * elapsed;
    } else if (state[SDL_SCANCODE_DOWN]) {
        cam.position.y -= speed * elapsed;
    }

    if (state[SDL_SCANCODE_RIGHT]) {
        cam.position.x += speed * elapsed;
    } else if (state[SDL_SCANCODE_LEFT]) {
        cam.position.x -= speed * elapsed;
    }

    rQueue.backgroundColor = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f};

    float mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    uniform_buffer_set_vec(
        &graphicsPipeline.vertexFrameData,
        graphics_pipeline_vertex_get_handle(&graphicsPipeline, &STRING("mouse")),
        (vec2){(mouseX / (float)windowWidth) * 2.0f - 1.0f, -((mouseY / (float)windowHeight) * 2.0f - 1.0f)},
        2
    );

    uniform_buffer_set_float(
        &graphicsPipeline.vertexFrameData,
        graphics_pipeline_vertex_get_handle(&graphicsPipeline, &STRING("aspectRatio")),
        (float)windowHeight / (float)windowWidth
    );

    for (int i = 0; i < NUM_PARTICLES; i++) {
        float distance = glm_vec2_distance((vec2){(mouseX / (float)windowWidth) * 2.0f - 1.0f, -((mouseY / (float)windowHeight) * 2.0f - 1.0f)}, (vec2){particles[i].robj.position.x, particles[i].robj.position.y});
        float dropoff = 1.0f / glm_clamp(distance * distance, 0.1f, 1000.0f);

        vec3 direction;
        glm_vec3_sub((vec3){(mouseX / (float)windowWidth) * 2.0f - 1.0f, -((mouseY / (float)windowHeight) * 2.0f - 1.0f), 0.0f}, particles[i].velocity, direction);

        vec3 tempVec;
        float angle = glm_vec3_angle(direction, particles[i].velocity) * dropoff;
        glm_vec3_scale(particles[i].velocity, elapsed, tempVec);
        glm_vec3_rotate(tempVec, angle, (vec3){0.0f, 0.0f, 1.0f});

        vec3 newPos;
        glm_vec3_add(particles[i].robj.position.arr, tempVec, newPos);

        glm_vec3_copy(newPos, particles[i].robj.position.arr);

        particles[i].robj.position.x = glm_clamp(particles[i].robj.position.x, -1.0f, 1.0f);
        particles[i].robj.position.y = glm_clamp(particles[i].robj.position.y, -1.0f, 1.0f);

        if (abs(particles[i].robj.position.x) >= 1.0f) {
            particles[i].velocity[0] *= -0.9;
        }

        if (abs(particles[i].robj.position.y) >= 1.0f) {
            particles[i].velocity[1] *= -0.9;
        }

        render_queue_add(&rQueue, &particles[i].robj);
    }

    render_queue_submit(&rQueue);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        render_object_destroy(&particles[i].robj);
    }
    material_destroy(&objMat);
    graphics_pipeline_destroy(&graphicsPipeline);

    GPB_terminate();

    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}