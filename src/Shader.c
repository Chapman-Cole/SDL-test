#include "Shader.h"
#include "Strings.h"
#include "SDLDevice.h"
#include <shaderc/shaderc.h>

void set_shader_format(unsigned int shader_format) {
    ShaderFormat = shader_format;
}

void get_shader_count_info(string file, SDL_GPUShaderCreateInfo* shaderInfo) {
    // Iterate through the file now to figure out the number of uniform buffers, storage_textures, etc.
    // Right now I will only implement uniform buffers, but the others can come later on
    for (int i = 0; i < file.len; i++) {
        if (file.str[i] == '\n') {
            continue;
        }

        // Check for single line comments that need to be skipped
        if (i < file.len - 1 && file.str[i] == '/' && file.str[i + 1] == '/') {
            int index = string_find_with_offset(&file, &STRING("\n"), i);

            // Make sure that i doesn't get set equal to -1
            if (index < 0) {
                index = file.len;
            }

            i = index;
            continue;
        }

        // Check for multi line comments that need to be skipped
        if (i < file.len - 1 && file.str[i] == '/' && file.str[i + 1] == '*') {
            int index = string_find_with_offset(&file, &STRING("*/"), i);

            // Make sure that i doesn't get set equal to minus 1
            if (index < 0) {
                index = file.len;
            }

            // +2 is for the length of the "*/" string
            i = index + 2;
            continue;
        }

        // Now search for the uniform buffer
        bool isUniform = string_compare_with_offset(&file, &STRING("uniform "), i);
        if (isUniform == true) {
            shaderInfo->num_uniform_buffers++;
            // The +8 is for the length of the "uniform " string
            i += 8 - 1;
            continue;
        }
    }
}

SDL_GPUShader* create_vertex_shader(string path, string entry_point) {
    string file;
    string_init(&file);

    string_read_file(&file, &path);

    SDL_GPUShaderCreateInfo vertexInfo = {0};
    vertexInfo.entrypoint = entry_point.str;
    vertexInfo.format = ShaderFormat; // loading .spv shaders
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;  // vertex shader
    vertexInfo.num_samplers = 0;
    vertexInfo.num_storage_buffers = 0;
    vertexInfo.num_storage_textures = 0;
    vertexInfo.num_uniform_buffers = 0;

    get_shader_count_info(file, &vertexInfo);

    // Create the shader compiler
    shaderc_compiler_t compiler = shaderc_compiler_initialize();

    // Compile the glsl to SPIR-V
    shaderc_compilation_result_t result = shaderc_compile_into_spv(
        compiler, file.str, file.len, shaderc_glsl_vertex_shader, path.str, entry_point.str, NULL
    );

    SDL_GPUShader* vertexShader = NULL;

    // Handle potential errors
    if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
        SDL_Log("Error: %s\n", shaderc_result_get_error_message(result));
    } else {
        const uint32_t* spirv_bin = (const uint32_t*)shaderc_result_get_bytes(result);
        size_t spirv_bin_size = shaderc_result_get_length(result);

        vertexInfo.code = (Uint8*)spirv_bin;
        vertexInfo.code_size = spirv_bin_size;
        vertexShader = SDL_CreateGPUShader(get_SDL_gpu_device(), &vertexInfo);
    }
    
    // Cleanup
    shaderc_result_release(result);
    shaderc_compiler_release(compiler);
    string_free(&file);

    return vertexShader;
}

SDL_GPUShader* create_fragment_shader(string path, string entry_point) {
    string file;
    string_init(&file);

    string_read_file(&file, &path);

    SDL_GPUShaderCreateInfo fragmentInfo = {0};
    fragmentInfo.entrypoint = entry_point.str;
    fragmentInfo.format = ShaderFormat; // loading .spv shaders
    fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;  // fragment shader
    fragmentInfo.num_samplers = 0;
    fragmentInfo.num_storage_buffers = 0;
    fragmentInfo.num_storage_textures = 0;
    fragmentInfo.num_uniform_buffers = 0;

    get_shader_count_info(file, &fragmentInfo);

    // Create the shader compiler
    shaderc_compiler_t compiler = shaderc_compiler_initialize();

    // Compile the glsl to SPIR-V
    shaderc_compilation_result_t result = shaderc_compile_into_spv(
        compiler, file.str, file.len, shaderc_glsl_fragment_shader, path.str, entry_point.str, NULL
    );

    SDL_GPUShader* fragmentShader = NULL;

    // Handle potential errors
    if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
        SDL_Log("Error: %s\n", shaderc_result_get_error_message(result));
    } else {
        const uint32_t* spirv_bin = (const uint32_t*)shaderc_result_get_bytes(result);
        size_t spirv_bin_size = shaderc_result_get_length(result);

        fragmentInfo.code = (Uint8*)spirv_bin;
        fragmentInfo.code_size = spirv_bin_size;
        fragmentShader = SDL_CreateGPUShader(get_SDL_gpu_device(), &fragmentInfo);
    }
    
    // Cleanup
    shaderc_result_release(result);
    shaderc_compiler_release(compiler);
    string_free(&file);

    return fragmentShader;
}
