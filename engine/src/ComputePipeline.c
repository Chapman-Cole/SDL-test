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
        compile_glsl_to_spirv(&computeShaderSource, &STRING("Internal String Source"), &shaderSource, entryPoint, shaderc_glsl_compute_shader);
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
    shader_layout_init(&shaderLayout);
    extract_shader_binding_info(&shaderSource, &shaderLayout);

    pipelineInfo.num_samplers = shaderLayout.num_samplers;
    pipelineInfo.num_readonly_storage_textures = shaderLayout.num_readonly_storage_textures;
    pipelineInfo.num_readonly_storage_buffers = shaderLayout.num_readonly_storage_buffers;
    pipelineInfo.num_readwrite_storage_textures = shaderLayout.num_readwrite_storage_textures;
    pipelineInfo.num_readwrite_storage_buffers = shaderLayout.num_readwrite_storage_buffers;
    pipelineInfo.num_uniform_buffers = shaderLayout.num_uniform_buffers;

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
    SDL_ReleaseGPUComputePipeline(get_SDL_gpu_device(), pipeline->computePipeline);
    return 0;
}