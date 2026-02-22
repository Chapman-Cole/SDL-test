#include "GraphicsPipeline.h"
#include "SDLDevice.h"
#include <stdlib.h>

void graphics_pipeline_factory_init(GraphicsPipelineFactory* factory) {
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

SDL_GPUGraphicsPipeline* graphics_pipeline_factory_generate_pipeline(GraphicsPipelineFactory* factory, SDL_GPUShader* vertexShader, SDL_GPUShader* fragmentShader) {
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {0};

    pipelineInfo = (SDL_GPUGraphicsPipelineCreateInfo){
        .vertex_shader = vertexShader,
        .fragment_shader = fragmentShader,
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

void graphics_pipeline_factory_registry_init(void) {
    GraphicsPipelineFactory pipelineFactory;
    graphics_pipeline_factory_init(&pipelineFactory);

    // Setup the graphics pipeline parameters
    graphics_pipeline_factory_append_vertex_buffer_description(&pipelineFactory, SDL_GPU_VERTEXINPUTRATE_VERTEX, 3 * sizeof(float));
    graphics_pipeline_factory_append_vertex_atribute(&pipelineFactory, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
    graphics_pipeline_factory_append_color_target_description_default(&pipelineFactory, SDL_GetGPUSwapchainTextureFormat(get_SDL_gpu_device(), get_SDL_main_window()));

    graphics_pipeline_factory_registry_append(&pipelineFactory, STRING("default"));
}

void graphics_pipeline_factory_registry_terminate(void) {
    // Free up the individual graphics pipelines
    for (int i = 0; i < GraphicsPipelineFactoryRegistryLen; i++) {
        graphics_pipeline_factory_destroy(&GraphicsPipelineFactoryRegistry[i].factory);
        string_free(&GraphicsPipelineFactoryRegistry[i].name);
    }

    // The strings inside the graphics pipeline factory registry items should hopefully be constant, so
    // there should be no need to free the individual string buffers. 
    SDL_free(GraphicsPipelineFactoryRegistry);

    GraphicsPipelineFactoryRegistryLen = 0;
    GraphicsPipelineFactoryRegistryMemsize = 1;
    GraphicsPipelineFactoryRegistry = NULL;
}

void graphics_pipeline_factory_registry_append(GraphicsPipelineFactory* factory, string name) {
    if (GraphicsPipelineFactoryRegistryLen + 1 >= GraphicsPipelineFactoryRegistryMemsize) {
        GraphicsPipelineFactoryRegistryMemsize += 3;

        GraphicsPipelineFactoryRegistry = SDL_realloc(GraphicsPipelineFactoryRegistry, GraphicsPipelineFactoryRegistryMemsize * sizeof(GraphicsPipelineFactory));

        if (GraphicsPipelineFactoryRegistry == NULL) {
            SDL_Log("Failed to allocate memory for graphics pipeline factory registry.\n");
            SDL_Quit();
            exit(-1);
        }
    }

    // This ensures that the name string persists in memory
    string heapName;
    string_init(&heapName);
    string_copy(&heapName, &name);
    GraphicsPipelineFactoryRegistry[GraphicsPipelineFactoryRegistryLen] = (GraphicsPipelineFactoryRegistryItem){
        .factory = *factory,
        .name = heapName
    };

    GraphicsPipelineFactoryRegistryLen++;
}

GraphicsPipelineFactory* graphics_pipeline_factory_registry_get_handle(string name) {
    for (int i = 0; i < GraphicsPipelineFactoryRegistryLen; i++) {
        if (string_compare(&name, &GraphicsPipelineFactoryRegistry[i].name)) {
            return &GraphicsPipelineFactoryRegistry[i].factory;
        }
    }

    return NULL;
}

SDL_GPUGraphicsPipeline* graphics_pipeline_factory_registry_generate_pipeline(string name, SDL_GPUShader* vertexShader, SDL_GPUShader* fragmentShader) {
    return graphics_pipeline_factory_generate_pipeline(
        graphics_pipeline_factory_registry_get_handle(name),
        vertexShader,
        fragmentShader
    );
}