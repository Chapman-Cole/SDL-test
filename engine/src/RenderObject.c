#include "RenderObject.h"

int render_object_create(RenderObject* object, GraphicsPipeline* pipeline, Material* material) {
    object->pipeline = pipeline;
    object->material = material;
    uniform_buffer_create(&object->vertexUniform, &pipeline->vertexLayout, UNIFORM_VERTEX_USER_OBJECT_DATA_SLOT);
    uniform_buffer_create(&object->fragmentUniform, &pipeline->fragmentLayout, UNIFORM_FRAGMENT_USER_OBJECT_DATA_SLOT);

    return 0;
}

int render_object_destroy(RenderObject* object) {
    uniform_buffer_destroy(&object->vertexUniform);
    uniform_buffer_destroy(&object->fragmentUniform);
    object->pipeline = NULL;
    object->material = NULL;

    return 0;
}