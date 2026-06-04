#include "TextRenderObject.h"
#include "Strings.h"
#include "Shader.h"
#include "SDLDevice.h"

#define SHADER_TEXT(text) #text

static string TextVertexShader = STRING(
"#version 450\n"
SHADER_TEXT(
    
layout (location = 0) in vec3 v_pos;

layout (location = 1) in vec2 v_uv;

layout (location = 0) out vec3 f_pos;

layout (location = 1) out vec2 f_uv;

layout (std140, set = 1, binding = 0) uniform EngineObjectData {
    mat4 VP;
    mat4 model;
} EOData;

void main() {
    gl_Position = EOData.VP * EOData.model * vec4(v_pos, 1.0);
    f_pos = v_pos;
    f_uv = v_uv;
}

)
);

static string TextFragmentShader = STRING(
"#version 450\n"
SHADER_TEXT(

layout (location = 0) in vec3 pos;

layout (location = 1) in vec2 uv;

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D simpleSampler;

void main() {
    FragColor = vec4(0, 0, 0, texture(simpleSampler, uv).a);
}

)
);

int Text_System_init(void) {
    graphics_pipeline_init(&TextGraphicsPipeline);

    // Position vertex buffer and attribute
    graphics_pipeline_append_vertex_buffer_description(&TextGraphicsPipeline, SDL_GPU_VERTEXINPUTRATE_VERTEX, 3 * sizeof(float));
    graphics_pipeline_append_vertex_attribute(&TextGraphicsPipeline, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);

    // UV coordinates buffer and attribute
    graphics_pipeline_append_vertex_buffer_description(&TextGraphicsPipeline, SDL_GPU_VERTEXINPUTRATE_VERTEX, 2 * sizeof(float));
    graphics_pipeline_append_vertex_attribute(&TextGraphicsPipeline, 1, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0);

    // Eventually it will be necessary to add a way to create a graphics pipeline with these specifications, but also for user created textures
    // if they first want to render to a texture
    graphics_pipeline_append_color_target_description_default(&TextGraphicsPipeline, SDL_GetGPUSwapchainTextureFormat(get_SDL_gpu_device(), get_SDL_main_window()));

    graphics_pipeline_attach_vertex_shader(&TextGraphicsPipeline, &TextVertexShader, &STRING("main"), SHADER_COMPILATION_GLSL_STRING);
    graphics_pipeline_attach_fragment_shader(&TextGraphicsPipeline, &TextFragmentShader, &STRING("main"), SHADER_COMPILATION_GLSL_STRING);

    graphics_pipeline_generate(&TextGraphicsPipeline);

    TextEngine = TTF_CreateGPUTextEngine(get_SDL_gpu_device());

    if (TextEngine == NULL) {
        SDL_Log("Failed to create gpu text engine");
        SDL_Quit();
        exit(-1);
    }

    return 0;
}

int Text_System_terminate(void) {
    TTF_DestroyGPUTextEngine(TextEngine);
    TextEngine = NULL;
    graphics_pipeline_destroy(&TextGraphicsPipeline);
}

GraphicsPipeline* TextRenderObject_pipeline(void) {
    return &TextGraphicsPipeline;
}

int TextRenderObject_create(TextRenderObject* obj, GraphicsPipeline* pipeline) {
    obj->pipeline = pipeline;
    obj->position[0] = 0;
    obj->position[1] = 0;
    obj->position[2] = 0;

    obj->vertexBuffer.gpu_buffer = NULL;
    obj->vertexBuffer.gpu_buffer_size = 0;
    obj->vertexBuffer.transfer_buffer = NULL;

    obj->uvBuffer.gpu_buffer = NULL;
    obj->uvBuffer.gpu_buffer_size = 0;
    obj->uvBuffer.transfer_buffer = NULL;

    return 0;
}

int TextRenderObject_destroy(TextRenderObject* obj) {
    obj->pipeline = NULL;
    obj->position[0] = 0;
    obj->position[1] = 0;
    obj->position[2] = 0;

    GPUBuffer_destroy(&obj->vertexBuffer);
    GPUBuffer_destroy(&obj->uvBuffer);

    return 0;
}