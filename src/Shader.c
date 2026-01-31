#include "Shader.h"
#include "Strings.h"
#include "SDLDevice.h"
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Include/glslang_c_shader_types.h>
#include <glslang/Public/resource_limits_c.h>

void set_shader_format(unsigned int shader_format) {
    ShaderFormat = shader_format;
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

    // Iterate through the file now to figure out the number of uniform buffers, storage_textures, etc.
    // Right now I will only implement uniform buffers, but the others can come later on

    for (int i = 0; i < file.len; i++) {
        if (file.str[i] == '\n') {
            continue;
        }

        // Check for single line comments that need to be skipped
        if (i < file.len - 1 && file.str[i] == '/' && file.str[i] == '/') {
            int index = string_find_with_offset(&file, &STRING("\n"), i);

            // Make sure that i doesn't get set equal to minus 1
            if (index < 0) {
                index = file.len;
            }

            i = index;
            continue;
        }

        // Check for multi line comments that need to be skipped
        if (i < file.len - 1 && file.str[i] == '/' && file.str[i] == '*') {
            int index = string_find_with_offset(&file, &STRING("*/"), i);

            // Make sure that i doesn't get set equal to minus 1
            if (index < 0) {
                index = file.len;
            }

            i = index;
            continue;
        }

        // Now search for the uniform buffer
        int index = string_find_with_offset(&file, &STRING("uniform"), i);
        if (index < 0) {
            break;
        } else {
            vertexInfo.num_uniform_buffers++;
            i = index;
            continue;
        }
    }

    // Now, compile the shader to SPIRV
    glslang_initialize_process();

    // Specify the input
    const glslang_input_t input = (glslang_input_t){
        .language = GLSLANG_SOURCE_GLSL,
        .stage = GLSLANG_STAGE_VERTEX,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_2,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_5,
        .code = file.str,
        .default_version = 450,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = glslang_default_resource()
    };

    // Create the shader
    glslang_shader_t* compVertexShader = glslang_shader_create(&input);
    if (!glslang_shader_preprocess(compVertexShader, &input)) {
        SDL_Log("%s", glslang_shader_get_info_log(compVertexShader));
    }

    // Parse the shader
    if (!glslang_shader_parse(compVertexShader, &input)) {
        SDL_Log("%s", glslang_shader_get_info_log(compVertexShader));
    }

    // Create a program and add the shader
    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, compVertexShader);

    // Link the attached shader
    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        SDL_Log("%s", glslang_program_get_info_log(program));
    }

    // Generate the SPIRV code
    glslang_program_SPIRV_generate(program, input.stage);
    const char* spirv_log = glslang_program_SPIRV_get_messages(program);
    if (spirv_log != NULL && spirv_log[0] != '\0') {
        SDL_Log("%s", spirv_log);
    }

    // Retrieve the generated SPIRV code
    const uint32_t* spirv_code = glslang_program_SPIRV_get_ptr(program);
    size_t spirv_size = glslang_program_SPIRV_get_size(program);

    // Create the actual shader for use with SDL
    vertexInfo.code = (Uint8*)spirv_code; // Convert to an array of bytes
    vertexInfo.code_size = spirv_size * (sizeof(uint32_t) / sizeof(Uint8));
    SDL_GPUShader* vertexShader = SDL_CreateGPUShader(get_SDL_gpu_device(), &vertexInfo);

    // Clean up
    glslang_program_delete(program);
    glslang_shader_delete(compVertexShader);
    glslang_finalize_process();
    string_free(&file);

    return vertexShader;
}
