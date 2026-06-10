#ifndef RENDEROBJECT_H
#define RENDEROBJECT_H

#include "GraphicsPipeline.h"
#include "Material.h"
#include "UniformBuffer.h"
#include "cglm/cglm.h"
#include "MeshObject.h"

typedef struct RenderObject {
    GraphicsPipeline* pipeline;
    Material* material;
    Mesh mesh;

    // The object specific uniforms
    UniformBuffer vertexUniform;
    UniformBuffer fragmentUniform;

    vec3 pos;

    vec3 scale;

    // This is mostly used internally for rotations
    versor quaternion;
} RenderObject;

int render_object_create(RenderObject* object, GraphicsPipeline* pipeline, Material* material);

int render_object_destroy(RenderObject* object);

#endif