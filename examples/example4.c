#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "engine.h"

typedef struct Particle {
    InstanceRenderObject robj;
    vec3* pos;
    vec3* velocity;
} Particle;

typedef struct ComputeUniformData {
    uint32_t data_size;
    float elapsed;
    float mouseX;
    float mouseY;
    uint32_t mode;
    vec3 pad;
} ComputeUniformData;

GraphicsPipeline graphicsPipeline;

ComputePipeline computePipeline;

GPUBuffer positionBuffer;
GPUBuffer velocityBuffer;

Material objMat1;

#define NUM_PARTICLES 3000000
Particle particles;

Uint64 perfFrequency = 0;
Uint64 perfCounterPrev = 0;
float appTime = 0.0;

Camera2D cam = CAMERA2D_DEFAULT;

float mouseX, mouseY;
float prevMouseX = 0.0f; 
float prevMouseY = 0.0f;
float mouseVelX, mouseVelY;

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
    // Base position
    graphics_pipeline_append_vertex_buffer_description(&graphicsPipeline, SDL_GPU_VERTEXINPUTRATE_VERTEX, 3 * sizeof(float));
    graphics_pipeline_append_vertex_attribute(&graphicsPipeline, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);

    // Instance position
    graphics_pipeline_append_vertex_buffer_description(&graphicsPipeline, SDL_GPU_VERTEXINPUTRATE_INSTANCE, 3 * sizeof(float));
    graphics_pipeline_append_vertex_attribute(&graphicsPipeline, 1, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);

    // Instance velocity
    graphics_pipeline_append_vertex_buffer_description(&graphicsPipeline, SDL_GPU_VERTEXINPUTRATE_INSTANCE, 3 * sizeof(float));
    graphics_pipeline_append_vertex_attribute(&graphicsPipeline, 2, 2, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);

    graphics_pipeline_append_color_target_description_default(&graphicsPipeline, SDL_GetGPUSwapchainTextureFormat(get_SDL_gpu_device(), get_SDL_main_window()));
    graphics_pipeline_attach_vertex_shader(&graphicsPipeline, &STRING("../shaders/example4/example4.vert"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_attach_fragment_shader(&graphicsPipeline, &STRING("../shaders/example4/example4.frag"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_generate(&graphicsPipeline);

    material_create(&objMat1, &graphicsPipeline);
    uniform_buffer_set_vec(
        &objMat1.uniform,
        material_get_handle(&objMat1, &STRING("col")),
        (float[]){(float)0x4f / 255.0f, (float)0xcc / 255.0f, (float)0xff / 255.0f, 1.0f},
        4
    );
    
    GPB_init();

    instance_render_object_create(&particles.robj, &graphicsPipeline, &objMat1, 1);
    meshobject_load_manual(
            &particles.robj.mesh,
            (float[]){
                -1.000000, 1.000000, 0.000000,
                -1.000000, -1.000000, 0.000000,
                1.000000, 1.000000, 0.000000,
                1.000000, -1.000000, 0.000000
            },
            12 * sizeof(float),
            (Uint32[]){
                0, 1, 3,
                0, 3, 2
            },
            6 * sizeof(Uint32)
    );

    // Don't forget to set the number of instances
    particles.robj.numInstances = NUM_PARTICLES;
    particles.pos = SDL_malloc(NUM_PARTICLES * sizeof(vec3));
    particles.velocity = SDL_malloc(NUM_PARTICLES * sizeof(vec3));

    for (int i = 0; i < NUM_PARTICLES; i++) {
        glm_vec3_copy((vec3){2 * (SDL_randf() - 0.5), 2 * (SDL_randf() - 0.5), 0.0f}, particles.pos[i]);
        particles.velocity[i][0] = 1.0f;
        particles.velocity[i][1] = 1.0f;
        particles.velocity[i][2] = 0.0f;
        glm_vec3_rotate(particles.velocity[i], 2.0f * SDL_PI_F * SDL_randf(), (vec3){0.0f, 0.0f, 1.0f});
        vec3 tempVec;
        glm_vec3_copy(particles.velocity[i], tempVec);
        glm_vec3_scale(tempVec, 0.2f * SDL_sqrt(SDL_randf()), particles.velocity[i]);


        if (particles.velocity[i][0] == 0.0f) {
            particles.velocity[i][0] = 0.1f;
        }

        if (particles.velocity[i][1] == 0.0f) {
            particles.velocity[i][1] = 0.1f;
        }
    }

    particles.robj.scale[0] = 0.002f;
    particles.robj.scale[1] = 0.002f;

    compute_pipeline_create(&computePipeline, &STRING("../shaders/example4/example4compute.glsl"), SHADER_COMPILATION_GLSL_PATH, &STRING("main"));
    GPUBuffer_create(&positionBuffer, NUM_PARTICLES * sizeof(vec3), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_VERTEX);
    GPUBuffer_create(&velocityBuffer, NUM_PARTICLES * sizeof(vec3), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_VERTEX);

    instance_render_object_add_instance_buffer(&particles.robj, &positionBuffer);
    instance_render_object_add_instance_buffer(&particles.robj, &velocityBuffer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
    GPUBuffer_upload(&positionBuffer, particles.pos, NUM_PARTICLES * sizeof(vec3), true, copyPass);
    GPUBuffer_upload(&velocityBuffer, particles.velocity, NUM_PARTICLES * sizeof(vec3), true, copyPass);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);

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
        cam.zoom = glm_clamp(cam.zoom, 0.0f, 1000.0f);
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
    float acceleration = 650.0f;
    float maxSpeed = 5.0f;

    rQueue.backgroundColor = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f};

    prevMouseX = mouseX;
    prevMouseY = mouseY;
    SDL_MouseButtonFlags buttonFlags = SDL_GetMouseState(&mouseX, &mouseY);

    float mouseDeltaX = mouseX - prevMouseX; 
    float mouseDeltaY = mouseY - prevMouseY;

    if (buttonFlags & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) {
        cam.position.x -= mouseDeltaX * elapsed;
        cam.position.y += mouseDeltaY * elapsed;
    }

    vec2 mousePos;
    camera2D_screen_to_world(&cam, (float)windowWidth / (float)windowHeight, (vec2){(mouseX / (float)windowWidth) * 2.0f - 1.0f, -((mouseY / (float)windowHeight) * 2.0f - 1.0f)}, mousePos);

    /*
    if (state[SDL_SCANCODE_O]) {
        for (int i = 0; i < NUM_PARTICLES; i++) {
            float distance = glm_vec2_distance(mousePos, (vec2){particles.pos[i][0], particles.pos[i][1]});

            vec3 direction;
            glm_vec3_sub((vec3){mousePos[0], mousePos[1], 0.0f}, particles.pos[i], direction);
            glm_vec3_normalize(direction);

            vec3 tempVec, acceleration;

            glm_vec3_scale(direction, elapsed * glm_clamp(1.0f / distance, 0.1f, 1000.0f), acceleration);
            glm_vec3_add(particles.velocity[i], acceleration, tempVec);
            glm_vec3_clamp(tempVec, -10.0f, 10.0f);
            glm_vec3_copy(tempVec, particles.velocity[i]);

            vec3 newPos;
            glm_vec3_scale(particles.velocity[i], elapsed, tempVec);
            glm_vec3_add(particles.pos[i], tempVec, newPos);

            glm_vec3_copy(newPos, particles.pos[i]);
        }
    } else if (state[SDL_SCANCODE_P]) {
        for (int i = 0; i < NUM_PARTICLES; i++) {
            float distance = glm_vec2_distance(mousePos, (vec2){particles.pos[i][0], particles.pos[i][1]});
            float multiplier = 0.1f * SDL_sinf(4.0f * distance - 3.0f * appTime);

            vec3 direction, direction2;
            glm_vec3_sub((vec3){mousePos[0], mousePos[1], 0.0f}, particles.pos[i], direction);
            glm_vec3_normalize(direction);
            glm_vec3_scale(direction, multiplier * elapsed, direction2);

            vec3 newPos;
            glm_vec3_add(particles.pos[i], direction2, newPos);

            glm_vec3_copy(newPos, particles.pos[i]);
        }
    } else {
        for (int i = 0; i < NUM_PARTICLES; i++) {
            float distance = glm_vec2_distance(mousePos, (vec2){particles.pos[i][0], particles.pos[i][1]});
            float dropoff = 0.05f / glm_clamp(distance * distance, 0.1f, 1000.0f);

            vec3 direction, direction2;
            glm_vec3_sub((vec3){mousePos[0], mousePos[1], 0.0f}, particles.pos[i], direction);
            glm_vec3_normalize(direction);
            glm_vec3_copy(direction, direction2);

            vec3 tempVec, acceleration;

            float dotProductResult = SDL_fabs((1.0f / glm_vec3_norm(particles.velocity[i])) * glm_vec3_dot(particles.velocity[i], direction));
            glm_vec3_rotate(direction2, SDL_PI_F / 2.0f, (vec3){0.0f, 0.0f, 1.0f});
            glm_vec3_scale(direction2, glm_vec3_norm(particles.velocity[i]), tempVec);
            glm_vec3_lerp(particles.velocity[i], tempVec, dotProductResult * elapsed, direction2);
            glm_vec3_copy(direction2, particles.velocity[i]);

            glm_vec3_scale(direction, elapsed * dropoff, acceleration);
            glm_vec3_add(particles.velocity[i], acceleration, tempVec);
            glm_vec3_copy(tempVec, particles.velocity[i]);

            vec3 newPos;
            glm_vec3_scale(particles.velocity[i], elapsed, tempVec);
            glm_vec3_add(particles.pos[i], tempVec, newPos);

            glm_vec3_copy(newPos, particles.pos[i]);
        }
    }
    */

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());
    
    SDL_GPUComputePass* compPass = SDL_BeginGPUComputePass(
        cmd,
        NULL,
        0,
        (SDL_GPUStorageBufferReadWriteBinding[]){
            {.buffer = positionBuffer.gpu_buffer, .cycle = false},
            {.buffer = velocityBuffer.gpu_buffer, .cycle = false}
        },
        2
    );

    SDL_BindGPUComputePipeline(compPass, computePipeline.computePipeline);
    SDL_BindGPUComputeStorageBuffers(
        compPass,
        0,
        (SDL_GPUBuffer*[]){
            positionBuffer.gpu_buffer,
            velocityBuffer.gpu_buffer
        },
        2
    );

    uint32_t computeMode = 0;
    if (state[SDL_SCANCODE_O]) {
        computeMode = 1;
    } else if (state[SDL_SCANCODE_P]) {
        computeMode = 2;
    }

    SDL_PushGPUComputeUniformData(
        cmd, 
        0,
        &(ComputeUniformData){
            .data_size = NUM_PARTICLES,
            .elapsed = elapsed,
            .mouseX = mousePos[0],
            .mouseY = mousePos[1],
            .mode = computeMode
        },
        sizeof(ComputeUniformData)
    );

    SDL_DispatchGPUCompute(compPass, (NUM_PARTICLES + computePipeline.thread_count_x - 1) / computePipeline.thread_count_x, 1, 1);

    SDL_EndGPUComputePass(compPass);

    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    SDL_WaitForGPUFences(get_SDL_gpu_device(), true, &fence, 1);
    SDL_ReleaseGPUFence(get_SDL_gpu_device(), fence);

    render_queue_add_instanced(&rQueue, &particles.robj);

    render_queue_submit(&rQueue);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    SDL_free(particles.velocity);
    SDL_free(particles.pos);

    GPUBuffer_destroy(&positionBuffer);
    GPUBuffer_destroy(&velocityBuffer);
    compute_pipeline_destroy(&computePipeline);

    instance_render_object_destroy(&particles.robj);

    material_destroy(&objMat1);
    graphics_pipeline_destroy(&graphicsPipeline);

    GPB_terminate();

    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}