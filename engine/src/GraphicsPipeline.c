#include "GraphicsPipeline.h"
#include "SDLDevice.h"
#include <stdlib.h>
#include "Shader.h"

int graphics_pipeline_init(GraphicsPipeline* pipeline) {
    pipeline->factory = (GraphicsPipelineFactory*)SDL_malloc(sizeof(GraphicsPipelineFactory));
    if (pipeline->factory == NULL) {
        SDL_Log("Failed to initialize Graphics Pipeline. Memory allocation error.\n");
        SDL_Quit();
        exit(-1);
    }

    pipeline->factory->primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    pipeline->factory->vertexBufferDescriptionsLen = 0;
    pipeline->factory->vertexBufferDescriptions = NULL;

    pipeline->factory->vertexAttributesLen = 0;
    pipeline->factory->vertexAttributes = NULL;

    pipeline->factory->colorTargetDescriptionsLen = 0;
    pipeline->factory->colorTargetDescriptions = NULL;

    shader_uniform_layout_init(&pipeline->vertexLayout);
    shader_uniform_layout_init(&pipeline->fragmentLayout);

    return 0;
}

int graphics_pipeline_destroy(GraphicsPipeline* pipeline) {
    // Make sure all allocated memory is freed and the SDL stuff is cleared up.
    if (pipeline->factory != NULL) {
        SDL_free(pipeline->factory->vertexBufferDescriptions);
        SDL_free(pipeline->factory->vertexAttributes);
        SDL_free(pipeline->factory->colorTargetDescriptions);
        SDL_free(pipeline->factory);
    }

    if (pipeline->shaders[0] != NULL) {
        SDL_ReleaseGPUShader(get_SDL_gpu_device(), pipeline->shaders[0]);
    }

    if (pipeline->shaders[1] != NULL) {    
        SDL_ReleaseGPUShader(get_SDL_gpu_device(), pipeline->shaders[1]);
    }

    if (pipeline->graphicsPipeline != NULL) {
        SDL_ReleaseGPUGraphicsPipeline(get_SDL_gpu_device(), pipeline->graphicsPipeline);
    }

    pipeline->factory = NULL;
    pipeline->graphicsPipeline = NULL;
    pipeline->shaders[0] = NULL;
    pipeline->shaders[1] = NULL;

    shader_uniform_layout_destroy(&pipeline->vertexLayout);
    shader_uniform_layout_destroy(&pipeline->fragmentLayout);

    return 0;
}

int graphics_pipeline_set_primitive_type(GraphicsPipeline* pipeline, SDL_GPUPrimitiveType primType) {
    pipeline->factory->primitive_type = primType;
    return 0;
}

int graphics_pipeline_append_vertex_buffer_description(GraphicsPipeline* pipeline, SDL_GPUVertexInputRate input_rate, Uint32 pitch) {
    pipeline->factory->vertexBufferDescriptions = SDL_realloc(pipeline->factory->vertexBufferDescriptions, (pipeline->factory->vertexBufferDescriptionsLen + 1) * sizeof(SDL_GPUVertexBufferDescription));
    if (pipeline->factory->vertexBufferDescriptions == NULL) {
        SDL_Log("Failed to allocate memory in graphics_pipeline_factory_append_vertex_buffer_description\n");
        SDL_Quit();
        exit(-1);
    }

    pipeline->factory->vertexBufferDescriptions[pipeline->factory->vertexBufferDescriptionsLen] = (SDL_GPUVertexBufferDescription){
        .slot = pipeline->factory->vertexBufferDescriptionsLen,
        .input_rate = input_rate,
        .instance_step_rate = 0,
        .pitch = pitch
    };

    pipeline->factory->vertexBufferDescriptionsLen++;

    return 0;
}

int graphics_pipeline_append_vertex_attribute(GraphicsPipeline* pipeline, Uint32 buffer_slot, Uint32 location, SDL_GPUVertexElementFormat format, Uint32 offset) {
    pipeline->factory->vertexAttributes = SDL_realloc(pipeline->factory->vertexAttributes, (pipeline->factory->vertexAttributesLen + 1) * sizeof(SDL_GPUVertexAttribute));
    if (pipeline->factory->vertexAttributes == NULL) {
        SDL_Log("Failed to allocate memory in graphics_pipeline_factory_append_vertex_attribute\n");
        SDL_Quit();
        exit(-1);
    }

    pipeline->factory->vertexAttributes[pipeline->factory->vertexAttributesLen] = (SDL_GPUVertexAttribute){
        .buffer_slot = buffer_slot,
        .location = location,
        .format = format,
        .offset = offset
    };

    pipeline->factory->vertexAttributesLen++;

    return 0;
}

int graphics_pipeline_append_color_target_description(GraphicsPipeline* pipeline, bool enable_blend, SDL_GPUBlendOp color_blend_op, SDL_GPUBlendOp alpha_blend_op, SDL_GPUBlendFactor src_color_blendfactor, SDL_GPUBlendFactor dst_color_blendfactor, SDL_GPUBlendFactor src_alpha_blendfactor, SDL_GPUBlendFactor dst_alpha_blendfactor, SDL_GPUTextureFormat format) {
    pipeline->factory->colorTargetDescriptions = SDL_realloc(pipeline->factory->colorTargetDescriptions, (pipeline->factory->colorTargetDescriptionsLen + 1) * sizeof(SDL_GPUColorTargetDescription));
    if (pipeline->factory->colorTargetDescriptions == NULL) {
        SDL_Log("Failed to allocate memory in graphics_pipeline_factory_append_color_target_description\n");
        SDL_Quit();
        exit(-1);
    }

    pipeline->factory->colorTargetDescriptions[pipeline->factory->colorTargetDescriptionsLen] = (SDL_GPUColorTargetDescription){
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

    pipeline->factory->colorTargetDescriptionsLen++;

    return 0;
}

int graphics_pipeline_append_color_target_description_default(GraphicsPipeline* pipeline, SDL_GPUTextureFormat format) {
        pipeline->factory->colorTargetDescriptions = SDL_realloc(pipeline->factory->colorTargetDescriptions, (pipeline->factory->colorTargetDescriptionsLen + 1) * sizeof(SDL_GPUColorTargetDescription));
    if (pipeline->factory->colorTargetDescriptions == NULL) {
        SDL_Log("Failed to allocate memory in graphics_pipeline_factory_append_color_target_description\n");
        SDL_Quit();
        exit(-1);
    }

    pipeline->factory->colorTargetDescriptions[pipeline->factory->colorTargetDescriptionsLen] = (SDL_GPUColorTargetDescription){
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

    pipeline->factory->colorTargetDescriptionsLen++;

    return 0;
}

int graphics_pipeline_attach_vertex_shader(GraphicsPipeline* pipeline, string* source, string* entry_point, Uint32 sourceType) {
    pipeline->shaders[0] = create_vertex_shader(source, entry_point, sourceType, &pipeline->vertexLayout);
    return 0;
}

int graphics_pipeline_attach_fragment_shader(GraphicsPipeline* pipeline, string* source, string* entry_point, Uint32 sourceType) {
    pipeline->shaders[1] = create_fragment_shader(source, entry_point, sourceType, &pipeline->fragmentLayout);
    return 0;
}

int graphics_pipeline_generate(GraphicsPipeline* pipeline) {
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {0};

    pipelineInfo = (SDL_GPUGraphicsPipelineCreateInfo){
        .vertex_shader = pipeline->shaders[0],
        .fragment_shader = pipeline->shaders[1],
        .vertex_input_state = (SDL_GPUVertexInputState){
            .num_vertex_buffers = pipeline->factory->vertexBufferDescriptionsLen,
            .vertex_buffer_descriptions = pipeline->factory->vertexBufferDescriptions,
            .num_vertex_attributes = pipeline->factory->vertexAttributesLen,
            .vertex_attributes = pipeline->factory->vertexAttributes
        },
        .target_info = (SDL_GPUGraphicsPipelineTargetInfo){
            .num_color_targets = pipeline->factory->colorTargetDescriptionsLen,
            .color_target_descriptions = pipeline->factory->colorTargetDescriptions
        }
    };

    pipeline->graphicsPipeline = SDL_CreateGPUGraphicsPipeline(get_SDL_gpu_device(), &pipelineInfo);

    // Clean up memory that is no longer needed, including the gpu shaders since they are now attached to the pipeline
    SDL_free(pipeline->factory->vertexBufferDescriptions);
    SDL_free(pipeline->factory->vertexAttributes);
    SDL_free(pipeline->factory->colorTargetDescriptions);
    SDL_free(pipeline->factory);
    SDL_ReleaseGPUShader(get_SDL_gpu_device(), pipeline->shaders[0]);
    SDL_ReleaseGPUShader(get_SDL_gpu_device(), pipeline->shaders[1]);

    pipeline->factory = NULL;
    pipeline->shaders[0] = NULL;
    pipeline->shaders[1] = NULL;
    return 0;
}