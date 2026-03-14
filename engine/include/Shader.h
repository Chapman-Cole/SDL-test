#ifndef SHADER_H
#define SHADER_H

#include <SDL3/SDL.h>
#include "Strings.h"

typedef enum ShaderCompilationSourceTypes {
    SHADER_COMPILATION_GLSL_PATH,
    SHADER_COMPILATION_GLSL_STRING,
    SHADER_COMPILATION_SPIRV_PATH,
    SHADER_COMPILATION_SPIRV_STRING
} ShaderCompilationSourceTypes;

// The default shader format is SPIRV, but this can be modified
static unsigned int ShaderFormat = SDL_GPU_SHADERFORMAT_SPIRV;

// This should be called once before beginning the compilation of shaders
void set_shader_format(unsigned int shader_format);

// Sets the .num_samplers, .num_storage_buffers, .num_storage_textures, and .num_uniform_buffers
// fields of the struct using spirv reflect. This is mostly for internal use to the shader compilation api.
// spirv_file - A pointer to a string containing the spir-v code
// shaderInfo - A pointer to the shadercreateinfo struct that contains the necessary fields
int extract_shader_binding_info(string* spirv_file, SDL_GPUShaderCreateInfo* shaderInfo);

// Compiles glsl source to SPIR-V code
// glslSource - A pointer to a string containing the glsl source code
// glslSourceName - A pointer to a string containing the name of the glsl source code (used for debugging info if needed)
// spirvOut - A pointer to an initialized string that the output of the compilation will be placed in.
//            This is where the SPIR-V code ends up.
// entry_point - A pointer to a string containing the name of the entry point function in the glsl source
// shaderType - Specifies whether the shader is a vertex, fragment, or compute shader. The possible values
//              are shaderc_glsl_vertex_shader, shaderc_glsl_fragment_shader, and shaderc_glsl_compute_shader
int compile_glsl_to_spirv(string* glslSource, string* glslSourceName, string* spirvOut, string* entry_point, Uint32 shaderType);

// The path string should point to a glsl file, and not a SPIR-V file
// Plan: At some point add the ability to pass in a precompiled file with an additional file specifying the
// number of uniforms, buffers, etc. to allow for precompilation benefits
// source type refers to how the source string is treated, meaning it could be treated as the source shader code itself,
// a path to the source, or even a path to spirv source code (use the ShaderCompilationSourceTypes enum)
SDL_GPUShader* create_vertex_shader(string* source, string* entry_point, Uint32 sourceType);

// The path string should point to a glsl file, and not a SPIR-V file
// Operates very similar to the create_vertex_shader function, but for fragment shaders
// source type refers to how the source string is treated, meaning it could be treated as the source shader code itself,
// a path to the source, or even a path to spirv source code (use the ShaderCompilationSourceTypes enum)
SDL_GPUShader* create_fragment_shader(string* source, string* entry_point, Uint32 sourceType);

#endif