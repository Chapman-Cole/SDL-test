#include "RenderObject.h"

int render_object_create(RenderObject* object, GraphicsPipeline* pipeline, Material* material) {
    object->pipeline = pipeline;
    object->material = material;

    object->pos[0] = 0;
    object->pos[1] = 0;
    object->pos[2] = 0;

    object->scale[0] = 1.0f;
    object->scale[1] = 1.0f;
    object->scale[2] = 1.0f;

    glm_quat_identity(object->quaternion);

    uniform_buffer_create(&object->vertexUniform, &pipeline->vertexLayout, UNIFORM_VERTEX_USER_OBJECT_DATA_SLOT);
    uniform_buffer_create(&object->fragmentUniform, &pipeline->fragmentLayout, UNIFORM_FRAGMENT_USER_OBJECT_DATA_SLOT);

    meshobject_init(&object->mesh);

    return 0;
}

int render_object_destroy(RenderObject* object) {
    uniform_buffer_destroy(&object->vertexUniform);
    uniform_buffer_destroy(&object->fragmentUniform);
    object->pipeline = NULL;
    object->material = NULL;

    object->pos[0] = 0;
    object->pos[1] = 0;
    object->pos[2] = 0;

    object->scale[0] = 1.0f;
    object->scale[1] = 1.0f;
    object->scale[2] = 1.0f;

    glm_quat_identity(object->quaternion);

    meshobject_destroy(&object->mesh);

    return 0;
}