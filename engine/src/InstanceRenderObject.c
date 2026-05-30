#include "InstanceRenderObject.h"

int instance_render_object_create(InstanceRenderObject* object, GraphicsPipeline* pipeline, Material* material, uint32_t instanceSlot) {
    object->pipeline = pipeline;
    object->material = material;
    object->instanceSlot = 0;

    object->numInstanceBuffers = 0;

    uniform_buffer_create(&object->vertexUniform, &pipeline->vertexLayout, UNIFORM_VERTEX_USER_OBJECT_DATA_SLOT);
    uniform_buffer_create(&object->fragmentUniform, &pipeline->fragmentLayout, UNIFORM_FRAGMENT_USER_OBJECT_DATA_SLOT);

    meshobject_init(&object->mesh);

    return 0;
}

int instance_render_object_destroy(InstanceRenderObject* object) {
    uniform_buffer_destroy(&object->vertexUniform);
    uniform_buffer_destroy(&object->fragmentUniform);

    meshobject_destroy(&object->mesh);
    return 0;
}

int instance_render_object_add_instance_buffer(InstanceRenderObject* object, GPUBuffer* buffer) {
    if (object->numInstanceBuffers < MAX_INSTANCE_BUFFERS) {
        object->instanceBuffers[object->numInstanceBuffers] = buffer;
        object->numInstanceBuffers++;
    }

    return 0;
}