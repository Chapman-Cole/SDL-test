#include "GraphicsPipeline.h"
#include "SDLDevice.h"
#include <stdlib.h>

void graphics_pipeline_factory_init(GraphicsPipelineFactory* factory) {
    factory->vertex_shader = NULL;
    factory->fragment_shader = NULL;
    factory->primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    factory->vertexBufferDescriptionsLen = 0;
    factory->vertexBufferDescriptions = NULL;

    factory->vertexAttributesLen = 0;
    factory->vertexAttributes = NULL;

    factory->colorTargetDescriptionsLen = 0;
    factory->colorTargetDescriptions = NULL;
}

void graphics_pipeline_factory_destroy(GraphicsPipelineFactory* factory) {
    SDL_free(factory->vertexBufferDescriptions);
    SDL_free(factory->vertexAttributes);
    SDL_free(factory->colorTargetDescriptions);
}

void graphics_pipeline_factory_set_shaders(GraphicsPipelineFactory* factory, SDL_GPUShader* vertexShader, SDL_GPUShader* fragmentShader) {
    factory->vertex_shader = vertexShader;
    factory->fragment_shader = fragmentShader;
}

void graphics_pipeline_factory_set_primitive_type(GraphicsPipelineFactory* factory, SDL_GPUPrimitiveType primType) {
    factory->primitive_type = primType;
}

void graphics_pipeline_factory_append_vertex_buffer_description(GraphicsPipelineFactory* factory, SDL_GPUVertexInputRate input_rate, Uint32 pitch) {
    factory->vertexBufferDescriptions = SDL_realloc(factory->vertexBufferDescriptions, (factory->vertexBufferDescriptionsLen + 1) * sizeof(SDL_GPUVertexBufferDescription));
    if (factory->vertexBufferDescriptions == NULL) {
        SDL_Log("Failed to allocate memory in graphics_pipeline_factory_append_vertex_buffer_description\n");
        SDL_Quit();
        exit(-1);
    }

    factory->vertexBufferDescriptions[factory->vertexBufferDescriptionsLen] = (SDL_GPUVertexBufferDescription){
        .slot = factory->vertexBufferDescriptionsLen,
        .input_rate = input_rate,
        .instance_step_rate = 0,
        .pitch = pitch
    };

    factory->vertexBufferDescriptionsLen++;
}

void graphics_pipeline_factory_append_vertex_atribute(GraphicsPipelineFactory* factory, Uint32 buffer_slot, Uint32 location, SDL_GPUVertexElementFormat format, Uint32 offset) {
    factory->vertexAttributes = SDL_realloc(factory->vertexAttributes, (factory->vertexAttributesLen + 1) * sizeof(SDL_GPUVertexAttribute));
    if (factory->vertexAttributes == NULL) {
        SDL_Log("Failed to allocate memory in graphics_pipeline_factory_append_vertex_attribute\n");
        SDL_Quit();
        exit(-1);
    }

    factory->vertexAttributes[factory->vertexAttributesLen] = (SDL_GPUVertexAttribute){
        .buffer_slot = buffer_slot,
        .location = location,
        .format = format,
        .offset = offset
    };

    factory->vertexAttributesLen++;
}

void graphics_pipeline_factory_append_color_target_description(GraphicsPipelineFactory* factory, bool enable_blend, SDL_GPUBlendOp color_blend_op, SDL_GPUBlendOp alpha_blend_op, SDL_GPUBlendFactor src_color_blendfactor, SDL_GPUBlendFactor dst_color_blendfactor, SDL_GPUBlendFactor src_alpha_blendfactor, SDL_GPUBlendFactor dst_alpha_blendfactor, SDL_GPUTextureFormat format) {
    factory->colorTargetDescriptions = SDL_realloc(factory->colorTargetDescriptions, (factory->colorTargetDescriptionsLen + 1) * sizeof(SDL_GPUColorTargetDescription));
    if (factory->colorTargetDescriptions == NULL) {
        SDL_Log("Failed to allocate memory in graphics_pipeline_factory_append_color_target_description\n");
        SDL_Quit();
        exit(-1);
    }

    factory->colorTargetDescriptions[factory->colorTargetDescriptionsLen] = (SDL_GPUColorTargetDescription){
        .blend_state = (SDL_GPUColorTargetBlendState){
            .enable_blend = enable_blend,
            .color_blend_op = color_blend_op,
            .alpha_blend_op = alpha_blend_op,
            .src_color_blendfactor = src_color_blendfactor,
            .dst_color_blendfactor = dst_color_blendfactor,
            .src_alpha_blendfactor = src_alpha_blendfactor,
            .dst_alpha_blendfactor = dst_alpha_blendfactor
        },
        .format = format
    };

    factory->colorTargetDescriptionsLen++;
}

void graphics_pipeline_factory_append_color_target_description_default(GraphicsPipelineFactory* factory, SDL_GPUTextureFormat format) {
        factory->colorTargetDescriptions = SDL_realloc(factory->colorTargetDescriptions, (factory->colorTargetDescriptionsLen + 1) * sizeof(SDL_GPUColorTargetDescription));
    if (factory->colorTargetDescriptions == NULL) {
        SDL_Log("Failed to allocate memory in graphics_pipeline_factory_append_color_target_description\n");
        SDL_Quit();
        exit(-1);
    }

    factory->colorTargetDescriptions[factory->colorTargetDescriptionsLen] = (SDL_GPUColorTargetDescription){
        .blend_state = (SDL_GPUColorTargetBlendState){
            .enable_blend = true,
            .color_blend_op = SDL_GPU_BLENDOP_ADD,
            .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
        },
        .format = format
    };

    factory->colorTargetDescriptionsLen++;
}

SDL_GPUGraphicsPipeline* graphics_pipeline_factory_generate_pipeline(GraphicsPipelineFactory* factory) {
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {0};

    pipelineInfo = (SDL_GPUGraphicsPipelineCreateInfo){
        .vertex_shader = factory->vertex_shader,
        .fragment_shader = factory->fragment_shader,
        .vertex_input_state = (SDL_GPUVertexInputState){
            .num_vertex_buffers = factory->vertexBufferDescriptionsLen,
            .vertex_buffer_descriptions = factory->vertexBufferDescriptions,
            .num_vertex_attributes = factory->vertexAttributesLen,
            .vertex_attributes = factory->vertexAttributes
        },
        .target_info = (SDL_GPUGraphicsPipelineTargetInfo){
            .num_color_targets = factory->colorTargetDescriptionsLen,
            .color_target_descriptions = factory->colorTargetDescriptions
        }
    };

    return SDL_CreateGPUGraphicsPipeline(get_SDL_gpu_device(), &pipelineInfo);
}
