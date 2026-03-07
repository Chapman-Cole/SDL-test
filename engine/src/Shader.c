#include "Shader.h"
#include "Strings.h"
#include "SDLDevice.h"
#include "SPIRV-Reflect/spirv_reflect.h"
#include <shaderc/shaderc.h>

void set_shader_format(unsigned int shader_format) {
    ShaderFormat = shader_format;
}

int extract_shader_binding_info(string* spirv_file, SDL_GPUShaderCreateInfo* shaderInfo) {
    // Make sure these values are 0 so the counting later on is accurate
    shaderInfo->num_samplers = 0;
    shaderInfo->num_storage_buffers = 0;
    shaderInfo->num_storage_textures = 0;
    shaderInfo->num_uniform_buffers = 0;

    // Extract uniform information by creating spvreflect shader module
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(spirv_file->len, spirv_file->str, &module);

    Uint32 count = 0;
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_Log("Failed to properly initialize SPIRV-Reflect module\n");
        SDL_Quit();
        exit(-1);
    }

    // Counts the number of descriptor bindings
    result = spvReflectEnumerateDescriptorBindings(&module, &count, NULL);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_Log("Failed to retrieve spirv bindings count\n");
        SDL_Quit();
        exit(-1);
    }

    // Extract the descriptor set bindings (info on things like number of uniform buffer objects and so on)
    SpvReflectDescriptorBinding** bindings = (SpvReflectDescriptorBinding**)SDL_malloc(count * sizeof(SpvReflectDescriptorBinding*));
    if (bindings == NULL) {
        SDL_Log("Failed to allocate memory for spirv bindings.");
        SDL_Quit();
        exit(-1);
    }

    // Fetches pointers to the modules descriptor buffers, and stores them in bindings pointer
    result = spvReflectEnumerateDescriptorBindings(&module, &count, bindings);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_Log("Failed to enumerate through spirv descriptor bindings.\n");
        SDL_Quit();
        exit(-1);
    }

    // Iterates through the descriptor set to count the number of uniform buffers
    for (Uint32 i = 0; i < count; i++) {
        SpvReflectDescriptorBinding* binding = bindings[i];
        if (
            binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER 
            || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
                shaderInfo->num_uniform_buffers++;
        } else if (
            binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER
            || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                shaderInfo->num_samplers++;
        } else if (
            binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER
            || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
                shaderInfo->num_storage_buffers++;
        } else if (
            binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE
            || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
                shaderInfo->num_storage_textures++;
        }
    }

    // Frees up the memory
    SDL_free(bindings);
    spvReflectDestroyShaderModule(&module);

    return 0;
}

int compile_glsl_to_spirv(string* glslSource, string* glslSourceName, string* spirvOut, string* entry_point, Uint32 shaderType) {
    // Create the shader compiler
    shaderc_compiler_t compiler = shaderc_compiler_initialize();

    // Compile the glsl to SPIR-V
    shaderc_compilation_result_t result;
    result = shaderc_compile_into_spv(
        compiler, glslSource->str, glslSource->len, shaderType, glslSourceName->str, entry_point->str, NULL
    );

    // Handle potential errors
    if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
        SDL_Log("Error: %s\n", shaderc_result_get_error_message(result));
    } else {
        const uint32_t* spirv_bin = (const uint32_t*)shaderc_result_get_bytes(result);
        size_t spirv_bin_size = shaderc_result_get_length(result);

        // The pointer returned by shaderc_result_get_bytes is automatically released at the cleanup at the end of this
        // function, so it is necessary to copy the data to the string spirvOut
        string_copy(spirvOut, &(string){.str = (char*)spirv_bin, .len = spirv_bin_size / sizeof(char), .__memsize = -1});
    }
    
    // Cleanup
    shaderc_result_release(result);
    shaderc_compiler_release(compiler);

    return 0;
}

SDL_GPUShader* create_vertex_shader(string source, string entry_point, Uint32 sourceType) {
    SDL_GPUShaderCreateInfo vertexInfo = {0};
    vertexInfo.entrypoint = entry_point.str;
    vertexInfo.format = ShaderFormat; // loading .spv shaders
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;  // vertex shader
    vertexInfo.num_samplers = 0;
    vertexInfo.num_storage_buffers = 0;
    vertexInfo.num_storage_textures = 0;
    vertexInfo.num_uniform_buffers = 0;

    string spirv_file;
    string_init(&spirv_file);

    if (sourceType == SHADER_COMPILATION_GLSL_PATH) {
        string glslSource;
        string_init(&glslSource);
        string_read_file(&glslSource, &source);

        compile_glsl_to_spirv(&glslSource, &source, &spirv_file, &entry_point, shaderc_glsl_vertex_shader);

        string_free(&glslSource);
    } else if (sourceType == SHADER_COMPILATION_GLSL_STRING) {
        compile_glsl_to_spirv(&source, &STRING("Internal String Source"), &spirv_file, &entry_point, shaderc_glsl_vertex_shader);
    } else if (sourceType == SHADER_COMPILATION_SPIRV_PATH) {
        string_read_file(&spirv_file, &source);
    } else if (sourceType == SHADER_COMPILATION_SPIRV_STRING) {
        // In this case, the source string is the spirv code
        spirv_file = source;
    } else {
        SDL_Log("Invalid source type entered into creation of shader.");
        SDL_Quit();
        exit(-1);
    }

    vertexInfo.code = (Uint8*)spirv_file.str;
    vertexInfo.code_size = spirv_file.len / sizeof(char);

    extract_shader_binding_info(&spirv_file, &vertexInfo);

    SDL_GPUShader* vertexShader = SDL_CreateGPUShader(get_SDL_gpu_device(), &vertexInfo);

    if (vertexShader == NULL) {
        SDL_Log("Failed to create vertex shader.\n");
        SDL_Quit();
        exit(-1);
    }

    // Cleanup
    // In the case when the user specifies they are passing in the spirv code as a string,
    // it is up to them to handle their memory with the passed string
    if (sourceType != SHADER_COMPILATION_SPIRV_STRING) {
        string_free(&spirv_file);
    }
    return vertexShader;
}

SDL_GPUShader* create_fragment_shader(string source, string entry_point, Uint32 sourceType) {
    SDL_GPUShaderCreateInfo fragmentInfo = {0};
    fragmentInfo.entrypoint = entry_point.str;
    fragmentInfo.format = ShaderFormat; // loading .spv shaders
    fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;  // fragment shader
    fragmentInfo.num_samplers = 0;
    fragmentInfo.num_storage_buffers = 0;
    fragmentInfo.num_storage_textures = 0;
    fragmentInfo.num_uniform_buffers = 0;

    string spirv_file;
    string_init(&spirv_file);

    if (sourceType == SHADER_COMPILATION_GLSL_PATH) {
        string glslSource;
        string_init(&glslSource);
        string_read_file(&glslSource, &source);

        compile_glsl_to_spirv(&glslSource, &source, &spirv_file, &entry_point, shaderc_glsl_fragment_shader);

        string_free(&glslSource);
    } else if (sourceType == SHADER_COMPILATION_GLSL_STRING) {
        compile_glsl_to_spirv(&source, &STRING("Internal String Source"), &spirv_file, &entry_point, shaderc_glsl_fragment_shader);
    } else if (sourceType == SHADER_COMPILATION_SPIRV_PATH) {
        string_read_file(&spirv_file, &source);
    } else if (sourceType == SHADER_COMPILATION_SPIRV_STRING) {
        // In this case, the source string is the spirv code
        spirv_file = source;
    } else {
        SDL_Log("Invalid source type entered into creation of shader.");
        SDL_Quit();
        exit(-1);
    }

    fragmentInfo.code = (Uint8*)spirv_file.str;
    fragmentInfo.code_size = spirv_file.len / sizeof(char);

    extract_shader_binding_info(&spirv_file, &fragmentInfo);

    SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(get_SDL_gpu_device(), &fragmentInfo);

    if (fragmentShader == NULL) {
        SDL_Log("Failed to create vertex shader.\n");
        SDL_Quit();
        exit(-1);
    }

    // Cleanup
    // In the case when the user specifies they are passing in the spirv code as a string,
    // it is up to them to handle their memory with the passed string
    if (sourceType != SHADER_COMPILATION_SPIRV_STRING) {
        string_free(&spirv_file);
    }
    return fragmentShader;
}
