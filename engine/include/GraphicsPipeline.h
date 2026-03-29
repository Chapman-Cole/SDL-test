#ifndef GRAPHICSPIPELINE_H
#define GRAPHICSPIPELINE_H

#include <SDL3/SDL.h>
#include "Strings.h"
#include "Window.h"
#include "ShaderUniformLayout.h"

typedef struct GraphicsPipelineFactory {
    SDL_GPUShader* vertex_shader;
    SDL_GPUShader* fragment_shader;
    SDL_GPUPrimitiveType primitive_type;

    Uint32 vertexBufferDescriptionsLen;
    SDL_GPUVertexBufferDescription* vertexBufferDescriptions;

    Uint32 vertexAttributesLen;
    SDL_GPUVertexAttribute* vertexAttributes;

    Uint32 colorTargetDescriptionsLen;
    SDL_GPUColorTargetDescription* colorTargetDescriptions;
} GraphicsPipelineFactory;

typedef struct GraphicsPipeline {
    SDL_GPUGraphicsPipeline* graphicsPipeline;
    GraphicsPipelineFactory* factory;
    // The first slot is for the vertex shader, and
    // the second slot is for the fragment shader
    SDL_GPUShader* shaders[2];
    
    // The uniform layout info for the vertex shader and fragment shader
    ShaderUniformLayout vertexLayout;
    ShaderUniformLayout fragmentLayout;
} GraphicsPipeline;

// Must be called directly after declaration of a graphics pipeline to 
// properly initialize it
int graphics_pipeline_init(GraphicsPipeline* pipeline);

// Frees up the data used by the graphics pipeline and releases the graphics pipeline.
// Must be called once completely done with the pipeline.
int graphics_pipeline_destroy(GraphicsPipeline* pipeline);

// Sets the primitive type for the graphics pipeline
// primType - Can be SDL_GPU_PRIMITIVETYPE_TRIANGLELIST, SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP, 
// SDL_GPU_PRIMITIVETYPE_LINELIST, SDL_GPU_PRIMITIVETYPE_LINESTRIP, SDL_GPU_PRIMITIVETYPE_POINTLIST
int graphics_pipeline_set_primitive_type(GraphicsPipeline* pipeline, SDL_GPUPrimitiveType primType);

// Adds to the vertex buffer descriptions. input_rate refers to whether you are addressing vertex index or instance index
// (example: SDL_GPU_VERTEXINPUTRATE_VERTEX). Pitch is the size of a single element + the offset between elements
int graphics_pipeline_append_vertex_buffer_description(GraphicsPipeline* pipeline, SDL_GPUVertexInputRate input_rate, Uint32 pitch);

// Adds to the vertex attributes list
// buffer_slot: which vertex buffer description you are referencing
// location: layout (location = x) in glsl
// format: The size and type of the attribute data (use the SDL enums for this parameter)
// offset: The offset relative to the start of the vertex element
int graphics_pipeline_append_vertex_attribute(GraphicsPipeline* pipeline, Uint32 buffer_slot, Uint32 location, SDL_GPUVertexElementFormat format, Uint32 offset);

// Adds to the color target description list
// For more info on the paramters, reference the SDL_GPUColorTargetDescription struct fields
// Format refers to the format of the pixel layout. This will usually end up being the result of calling SDL_GetGPUSwapchainTextureFormat()
int graphics_pipeline_append_color_target_description(GraphicsPipeline* pipeline, bool enable_blend, SDL_GPUBlendOp color_blend_op, SDL_GPUBlendOp alpha_blend_op, SDL_GPUBlendFactor src_color_blendfactor, SDL_GPUBlendFactor dst_color_blendfactor, SDL_GPUBlendFactor src_alpha_blendfactor, SDL_GPUBlendFactor dst_alpha_blendfactor, SDL_GPUTextureFormat format);

// Adds to the color target description list, but uses specific default values, which can be seen below
// enable_blend = true;
// color_blend_op = SDL_GPU_BLENDOP_ADD;
// alpha_blend_op = SDL_GPU_BLENDOP_ADD;
// src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
// dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
// src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
// dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
// Format refers to the format of the pixel layout. This will usually end up being the result of calling SDL_GetGPUSwapchainTextureFormat()
int graphics_pipeline_append_color_target_description_default(GraphicsPipeline* pipeline, SDL_GPUTextureFormat format);

// Attaches a vertex shader to the pipeline. 
// source - Pointer to the source string for the glsl (or spirv code) depending on the sourceType
// entry_point - A pointer to the string with the name for the entry point in the glsl code
// sourceType - Specifies how the source string is interpreted. Can be SHADER_COMPILATION_GLSL_PATH,
//              SHADER_COMPILATION_GLSL_STRING, SHADER_COMPILATION_SPIRV_PATH, or SHADER_COMPILATION_SPIRV_STRING
int graphics_pipeline_attach_vertex_shader(GraphicsPipeline* pipeline, string* source, string* entry_point, Uint32 sourceType);

// Attaches a fragment shader to the pipeline.
// source - Pointer to the source string for the glsl (or spirv code) depending on the sourceType
// entry_point - A pointer to the string with the name for the entry point in the glsl code
// sourceType - Specifies how the source string is interpreted. Can be SHADER_COMPILATION_GLSL_PATH,
//              SHADER_COMPILATION_GLSL_STRING, SHADER_COMPILATION_SPIRV_PATH, or SHADER_COMPILATION_SPIRV_STRING
int graphics_pipeline_attach_fragment_shader(GraphicsPipeline* pipeline, string* source, string* entry_point, Uint32 sourceType);

// Uses the specified configurations to generate the internal SDL graphics pipeline
// This makes it ready for use with rendering. 
int graphics_pipeline_generate(GraphicsPipeline* pipeline);

#endif