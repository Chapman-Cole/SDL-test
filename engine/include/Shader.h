#ifndef SHADER_H
#define SHADER_H

#include <SDL3/SDL.h>
#include "Strings.h"

// The default shader format is SPIRV, but this can be modified
static unsigned int ShaderFormat = SDL_GPU_SHADERFORMAT_SPIRV;

// This should be called once before beginning the compilation of shaders
void set_shader_format(unsigned int shader_format);

// The path string should point to a glsl file, and not a SPIR-V file
// Plan: At some point add the ability to pass in a precompiled file with an additional file specifying the
// number of uniforms, buffers, etc. to allow for precompilation benefits
SDL_GPUShader* create_vertex_shader(string path, string entry_point, bool treat_path_as_shader_source);

// The path string should point to a glsl file, and not a SPIR-V file
// Operates very similar to the create_vertex_shader function, but for fragment shaders
SDL_GPUShader* create_fragment_shader(string path, string entry_point, bool treat_path_as_shader_source);

#endif