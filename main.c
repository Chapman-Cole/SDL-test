#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

typedef struct Vertex {
    float x, y, z;
    float r, g, b, a;
} Vertex;

typedef struct UniformParams
{
    float u_scale;
    float pad[3];   // 16 bytes total
} UniformParams; 

static Vertex vertices[] = {
    {-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // Bottom left
    {1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // Bottom right
    {-1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // Top left
    {1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f} // Top right
};

static Uint16 indices[] = {
    0, 1, 2,
    1, 3, 2
};

SDL_Window* window = NULL;
SDL_GPUDevice* device = NULL;

SDL_GPUGraphicsPipeline* graphicsPipeline = NULL;

SDL_GPUBuffer* vertexBuffer = NULL;
SDL_GPUTransferBuffer* transferBuffer = NULL;

SDL_GPUBuffer* indexBuffer = NULL;
SDL_GPUTransferBuffer* transferBuffer2 = NULL;

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

    // Create the Vertex Buffer (generally an expensive operation)
    SDL_GPUBufferCreateInfo bufferInfo = {0};
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);

    // Create the transfer buffer to upload the vertex buffer to the gpu
    SDL_GPUTransferBufferCreateInfo transferInfo = {0};
    transferInfo.size = sizeof(vertices);
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

    // Fill the transfer buffer with data by mapping it to a pointer
    Vertex* data = (Vertex*)SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    SDL_memcpy(data, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    // Find where the data is
    SDL_GPUTransferBufferLocation location = {0};
    location.transfer_buffer = transferBuffer; //size of the data in bytes
    location.offset = 0; //begin writing from the first vertex

    // Find where to upload the data to
    SDL_GPUBufferRegion region = {0};
    region.buffer = vertexBuffer;
    region.size = sizeof(vertices); //size of data in bytes
    region.offset = 0; //begin writing from the first vertex

    // Create the index buffer
    SDL_GPUBufferCreateInfo bufferInfo2 = {0};
    bufferInfo2.size = sizeof(indices);
    bufferInfo2.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    indexBuffer = SDL_CreateGPUBuffer(device, &bufferInfo2);

    // Create the transfer buffer to upload the index buffer to the gpu
    SDL_GPUTransferBufferCreateInfo transferInfo2 = {0};
    transferInfo2.size = sizeof(vertices);
    transferInfo2.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferBuffer2 = SDL_CreateGPUTransferBuffer(device, &transferInfo2);

    // Fill the transfer buffer with data by mapping it to a pointer
    Uint16* data2 = (Uint16*)SDL_MapGPUTransferBuffer(device, transferBuffer2, false);
    SDL_memcpy(data2, indices, sizeof(indices));
    SDL_UnmapGPUTransferBuffer(device, transferBuffer2);

    // Find where the data is on the gpu
    SDL_GPUTransferBufferLocation location2 = {0};
    location2.transfer_buffer = transferBuffer2;
    location2.offset = 0;

    // Find where to upload the data to
    SDL_GPUBufferRegion region2 = {0};
    region2.buffer = indexBuffer;
    region2.size = sizeof(indices);
    region2.offset = 0;

    // Start a copy pass to get the data in the transfer buffer to the vertex buffer
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    // Upload the data
    SDL_UploadToGPUBuffer(copyPass, &location, &region, true);
    SDL_UploadToGPUBuffer(copyPass, &location2, &region2, true);

    // End the copy pass
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    // Load the vertex shader code
    size_t vertexCodeSize;
    void* vertexCode = SDL_LoadFile("../shaders/vertex.spv", &vertexCodeSize);

    // Create the vertex shader
    SDL_GPUShaderCreateInfo vertexInfo = {0};
    vertexInfo.code = (Uint8*)vertexCode; //Convert to an array of bytes
    vertexInfo.code_size = vertexCodeSize;
    vertexInfo.entrypoint = "main";
    vertexInfo.format = SDL_GPU_SHADERFORMAT_SPIRV; //loading .spv shaders
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX; // vertex shader
    vertexInfo.num_samplers = 0;
    vertexInfo.num_storage_buffers = 0;
    vertexInfo.num_storage_textures = 0;
    vertexInfo.num_uniform_buffers = 1;
    SDL_GPUShader* vertexShader = SDL_CreateGPUShader(device, &vertexInfo);

    // Free the file
    SDL_free(vertexCode);

    // Load the fragment shader file
    size_t fragmentCodeSize;
    void* fragmentCode = SDL_LoadFile("../shaders/fragment.spv", &fragmentCodeSize);

    // Create the fragment shader
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

    // Free the file
    SDL_free(fragmentCode);

    // Begin the creation of the graphics pipeline
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {0};

    // bind the shaders
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;

    // draw triangles as the primitive type
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    // describe the vertex buffers
    SDL_GPUVertexBufferDescription vertexBufferDescriptions[1];
    vertexBufferDescriptions[0].slot = 0;
    vertexBufferDescriptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescriptions[0].instance_step_rate = 0;
    vertexBufferDescriptions[0].pitch = sizeof(Vertex);

    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = vertexBufferDescriptions;

    // describe the vertex attribute
    SDL_GPUVertexAttribute vertexAttributes[2];

    // a_position
    vertexAttributes[0].buffer_slot = 0; // fetch data from the buffer at slot 0
    vertexAttributes[0].location = 0; // layout (location = 0) in shader
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; // vec3
    vertexAttributes[0].offset = 0; // start from the first byte from the current buffer

    // a_color
    vertexAttributes[1].buffer_slot = 0; // use buffer at slot 0
    vertexAttributes[1].location = 1; // layout (location = 1) in shader
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4; // vec4
    vertexAttributes[1].offset = sizeof(float) * 3; // 4th float from current buffer position

    pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
    pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

    // describe the color target
    SDL_GPUColorTargetDescription colorTargetDescriptions[1];
    colorTargetDescriptions[0] = (SDL_GPUColorTargetDescription){0};
    colorTargetDescriptions[0].blend_state.enable_blend = true;
    colorTargetDescriptions[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTargetDescriptions[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTargetDescriptions[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTargetDescriptions[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTargetDescriptions[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTargetDescriptions[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTargetDescriptions[0].format = SDL_GetGPUSwapchainTextureFormat(device, window);

    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

    graphicsPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);

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
    double fps = (double)perfFrequency / (double)(perfCounterNow - perfCounterPrev);
    double elapsed = 1.0f / fps;
    perfCounterPrev = perfCounterNow;
    elapsedTime += elapsed;
    time += (float)elapsed;

    if (elapsedTime >= 0.5) {
        SDL_Log("\033[2J\033[HFPS: %lf", fps);
        elapsedTime = 0;
    }

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);

    SDL_GPUTexture* swapchainTexture;
    Uint32 width, height;
    SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &width, &height);
    if (swapchainTexture == NULL) {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return SDL_APP_CONTINUE;
    }

    SDL_GPUColorTargetInfo colorTargetInfo = {0};
    colorTargetInfo.clear_color = (SDL_FColor){255/255.0f, 219/255.0f, 187/255.0f, 255/255.0f};
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
    bufferBindings[0].offset = 0; // start from the first byte

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

    // issue a draw call
    // SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);
    SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);

    // End the render pass before submitting the command buffer
    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    // Release buffers since they are no longer needed
    SDL_ReleaseGPUBuffer(device, vertexBuffer);
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    SDL_ReleaseGPUBuffer(device, indexBuffer);
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer2);

    // release the pipeline
    SDL_ReleaseGPUGraphicsPipeline(device, graphicsPipeline);

    // destroy the gpu device
    SDL_DestroyGPUDevice(device);

    // destory the window
    SDL_DestroyWindow(window);
}
