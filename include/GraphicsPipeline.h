#ifndef GRAPHICSPIPELINE_H
#define GRAPHICSPIPELINE_H

#include <SDL3/SDL.h>
#include "Strings.h"
#include "Window.h"

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

// The name string is used so that it can be found by searching
// with strings
typedef struct GraphicsPipelineFactoryRegistryItem {
    GraphicsPipelineFactory factory;
    string name;
} GraphicsPipelineFactoryRegistryItem;

// A registry for created graphics pipelines so 
static GraphicsPipelineFactoryRegistryItem* GraphicsPipelineFactoryRegistry;
static Uint32 GraphicsPipelineFactoryRegistryLen = 0;
static Uint32 GraphicsPipelineFactoryRegistryMemsize = 1;

// Must be called directly after declaration of a graphics pipeline factory to 
// properly initialize it
void graphics_pipeline_factory_init(GraphicsPipelineFactory* factory);

// Frees up the data used by the graphics pipeline factory. Must be called once
// finished with the graphics pipeline factory, which is usually after you have
// fetched the actual SDL graphics pipeline from it
void graphics_pipeline_factory_destroy(GraphicsPipelineFactory* factory);

// Sets the primitive type for the graphics pipeline factory
void graphics_pipeline_factory_set_primitive_type(GraphicsPipelineFactory* factory, SDL_GPUPrimitiveType primType);

// Adds to the vertex buffer descriptions. input_rate refers to whether you are addressing vertex index or instance index
// (example: SDL_GPU_VERTEXINPUTRATE_VERTEX). Pitch is the size of a single element + the offset between elements
void graphics_pipeline_factory_append_vertex_buffer_description(GraphicsPipelineFactory* factory, SDL_GPUVertexInputRate input_rate, Uint32 pitch);

// Adds to the vertex attributes list
// buffer_slot: which vertex buffer description you are referencing
// location: layout (location = x) in glsl
// format: The size and type of the attribute data (use the SDL enums for this parameter)
// offset: The offset relative to the start of the vertex element
void graphics_pipeline_factory_append_vertex_atribute(GraphicsPipelineFactory* factory, Uint32 buffer_slot, Uint32 location, SDL_GPUVertexElementFormat format, Uint32 offset);

// Adds to the color target description list
// For more info on the paramters, reference the SDL_GPUColorTargetDescription struct fields
// Format refers to the format of the pixel layout. This will usually end up being the result of calling SDL_GetGPUSwapchainTextureFormat()
void graphics_pipeline_factory_append_color_target_description(GraphicsPipelineFactory* factory, bool enable_blend, SDL_GPUBlendOp color_blend_op, SDL_GPUBlendOp alpha_blend_op, SDL_GPUBlendFactor src_color_blendfactor, SDL_GPUBlendFactor dst_color_blendfactor, SDL_GPUBlendFactor src_alpha_blendfactor, SDL_GPUBlendFactor dst_alpha_blendfactor, SDL_GPUTextureFormat format);

// Adds to the color target description list, but uses specific default values, which can be seen below
// enable_blend = true;
// color_blend_op = SDL_GPU_BLENDOP_ADD;
// alpha_blend_op = SDL_GPU_BLENDOP_ADD;
// src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
// dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
// src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
// dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
// Format refers to the format of the pixel layout. This will usually end up being the result of calling SDL_GetGPUSwapchainTextureFormat()
void graphics_pipeline_factory_append_color_target_description_default(GraphicsPipelineFactory* factory, SDL_GPUTextureFormat format);

// Initializes the graphics pipeline factory registry. This will provide a default
// graphics pipeline factory to be used for basic scenarios
void graphics_pipeline_factory_registry_init(void);

// Frees up memory related to the graphics pipeline factory registry
void graphics_pipeline_factory_registry_terminate(void);

// Returns the actual SDL graphics pipeline using the factory that was previously created
SDL_GPUGraphicsPipeline* graphics_pipeline_factory_generate_pipeline(GraphicsPipelineFactory* factory, SDL_GPUShader* vertexShader, SDL_GPUShader* fragmentShader);

// Appends the specified factory to the registry with the given name
void graphics_pipeline_factory_registry_append(GraphicsPipelineFactory* factory, string name);

// Returns a pointer to the specified graphics pipeline in the registry
GraphicsPipelineFactory* graphics_pipeline_factory_registry_get_handle(string name);

// Generates an SDL_GPUGraphicsPipeline* from the registry
SDL_GPUGraphicsPipeline* graphics_pipeline_factory_registry_generate_pipeline(string name, SDL_GPUShader* vertexShader, SDL_GPUShader* fragmentShader);

#endif