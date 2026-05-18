#include "InstanceRenderObject.h"

int instance_render_object_create(InstanceRenderObject* object, GraphicsPipeline* pipeline, Material* material, uint32_t instanceSlot) {
    object->pipeline = pipeline;
    object->material = material;
    object->instanceSlot;

    object->instanceData = NULL;
    object->instanceStepSize = 0;
    object->numInstances = 0;

    object->instanceBuffer = NULL;

    uniform_buffer_create(&object->vertexUniform, &pipeline->vertexLayout, UNIFORM_VERTEX_USER_OBJECT_DATA_SLOT);
    uniform_buffer_create(&object->fragmentUniform, &pipeline->fragmentLayout, UNIFORM_FRAGMENT_USER_OBJECT_DATA_SLOT);

    meshobject_init(&object->mesh);

    return 0;
}

int instance_render_object_destroy(InstanceRenderObject* object) {
    uniform_buffer_destroy(&object->vertexUniform);
    uniform_buffer_destroy(&object->fragmentUniform);

    meshobject_destroy(&object->mesh);

    SDL_free(object->instanceData);
    object->instanceData = NULL;
    object->instanceStepSize = 0;
    object->numInstances = 0;

    SDL_ReleaseGPUBuffer(get_SDL_gpu_device(), object->instanceBuffer);
    object->instanceBuffer = NULL;

    return 0;
}

int instance_render_object_instantiate(InstanceRenderObject* object, uint32_t numInstances, uint32_t instanceStepSize) {
    SDL_free(object->instanceData);
    object->numInstances = numInstances;
    object->instanceStepSize = instanceStepSize;
    object->instanceData = SDL_malloc(object->numInstances * object->instanceStepSize);
    return 0;
}

int instance_render_object_upload(InstanceRenderObject* object) {
    if (object->instanceBuffer != NULL) {
        SDL_ReleaseGPUBuffer(get_SDL_gpu_device(), object->instanceBuffer);
    }

    object->instanceBuffer = GPB_create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, object->instanceData, object->numInstances * object->instanceStepSize);
    return 0;
}