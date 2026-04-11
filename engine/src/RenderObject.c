#include "RenderObject.h"

int render_object_create(RenderObject* object, GraphicsPipeline* pipeline, Material* material) {
    object->pipeline = pipeline;
    object->material = material;
    object->position.x = 0;
    object->position.y = 0;
    object->position.z = 0;
    glm_quat_identity(object->quaternion);
    uniform_buffer_create(&object->vertexUniform, &pipeline->vertexLayout, UNIFORM_VERTEX_USER_OBJECT_DATA_SLOT);
    uniform_buffer_create(&object->fragmentUniform, &pipeline->fragmentLayout, UNIFORM_FRAGMENT_USER_OBJECT_DATA_SLOT);

    return 0;
}

int render_object_destroy(RenderObject* object) {
    uniform_buffer_destroy(&object->vertexUniform);
    uniform_buffer_destroy(&object->fragmentUniform);
    object->pipeline = NULL;
    object->material = NULL;

    object->position.x = 0;
    object->position.y = 0;
    object->position.z = 0;
    glm_quat_identity(object->quaternion);

    return 0;
}