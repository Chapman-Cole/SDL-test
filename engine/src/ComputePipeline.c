#include "ComputePipeline.h"
#include "Shader.h"
#include "ShaderLayout.h"
#include <shaderc/shaderc.h>
#include "SDLDevice.h"

int compute_pipeline_create(ComputePipeline* pipeline, string* computeShaderSource, uint8_t sourceType, string* entryPoint) {
    string shaderSource;
    string_init(&shaderSource);

    switch (sourceType) {
    case  SHADER_COMPILATION_GLSL_PATH:
        string glslSource;
        string_init(&glslSource);
        string_read_file(&glslSource, computeShaderSource);

        compile_glsl_to_spirv(&glslSource, computeShaderSource, &shaderSource, entryPoint, shaderc_glsl_compute_shader);

        string_free(&glslSource);
        break;
    
    case SHADER_COMPILATION_GLSL_STRING:
        compile_glsl_to_spirv(computeShaderSource, &STRING("Internal String Source"), &shaderSource, entryPoint, shaderc_glsl_compute_shader);
        break;

    case SHADER_COMPILATION_SPIRV_PATH:
        string_read_file(&shaderSource, computeShaderSource);
        break;

    case SHADER_COMPILATION_SPIRV_STRING:
        string_copy(&shaderSource, computeShaderSource);
        break;
    
    default:
        SDL_Log("Unrecognized shader source type detected when trying to create compute pipeline.");
        SDL_Quit();
        exit(-1);
        break;
    }

    SDL_GPUComputePipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.entrypoint = entryPoint->str;
    pipelineInfo.code = (Uint8*)shaderSource.str;
    pipelineInfo.code_size = shaderSource.len * sizeof(char);
    pipelineInfo.format = get_shader_format();

    ShaderLayout shaderLayout;
    shader_layout_init(&shaderLayout, entryPoint);
    extract_shader_binding_info(&shaderSource, &shaderLayout);

    pipelineInfo.num_samplers = shaderLayout.num_samplers;
    pipelineInfo.num_readonly_storage_textures = shaderLayout.num_readonly_storage_textures;
    pipelineInfo.num_readonly_storage_buffers = shaderLayout.num_readonly_storage_buffers;
    pipelineInfo.num_readwrite_storage_textures = shaderLayout.num_readwrite_storage_textures;
    pipelineInfo.num_readwrite_storage_buffers = shaderLayout.num_readwrite_storage_buffers;
    pipelineInfo.num_uniform_buffers = shaderLayout.num_uniform_buffers;
    pipelineInfo.threadcount_x = shaderLayout.thread_count_x;
    pipelineInfo.threadcount_y = shaderLayout.thread_count_y;
    pipelineInfo.threadcount_z = shaderLayout.thread_count_z;

    //SDL_Log("num_readonly_storage_textures: %d", shaderLayout.num_readonly_storage_textures);
    //SDL_Log("num_readonly_storage_buffers: %d", shaderLayout.num_readonly_storage_buffers);
    //SDL_Log("num_readwrite_storage_textures: %d", shaderLayout.num_readwrite_storage_textures);
    //SDL_Log("num_readwrite_storage_buffers: %d", shaderLayout.num_readwrite_storage_buffers);

    pipeline->thread_count_x = shaderLayout.thread_count_x;
    pipeline->thread_count_y = shaderLayout.thread_count_y;
    pipeline->thread_count_z = shaderLayout.thread_count_z;

    pipeline->computePipeline = SDL_CreateGPUComputePipeline(get_SDL_gpu_device(), &pipelineInfo);

    if (pipeline->computePipeline == NULL) {
        SDL_Log("Failed to create compute pipeline: %s", SDL_GetError());
        SDL_Quit();
        exit(-1);
    }

    shader_layout_destroy(&shaderLayout);
    string_free(&shaderSource); 
    return 0;
}

int compute_pipeline_destroy(ComputePipeline* pipeline) {
    if (pipeline->computePipeline != NULL) {
        SDL_ReleaseGPUComputePipeline(get_SDL_gpu_device(), pipeline->computePipeline);
    }
    pipeline->computePipeline = NULL;
    pipeline->thread_count_x = 0;
    pipeline->thread_count_y = 0;
    pipeline->thread_count_z = 0;
    return 0;
}