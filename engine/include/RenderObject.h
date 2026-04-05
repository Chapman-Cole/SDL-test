#ifndef RENDEROBJECT_H
#define RENDEROBJECT_H

#include "GraphicsPipeline.h"
#include "Material.h"
#include "UniformBuffer.h"
#include "cglm/cglm.h"

// Interposes a cglm and an anonymous struct with 3 floats
// to provide 2 different ways of accessing the same data
typedef union RenderObjectVector3 {
    // Anonymous struct so x, y, and z are accessed directly
    // instead of being nested with dots like object.____
    struct {
        float x;
        float y;
        float z;
    };
    vec3 arr;
} RenderObjectVector3;

typedef struct RenderObject {
    GraphicsPipeline* pipeline;
    Material* material;

    // The object specific uniforms
    UniformBuffer vertexUniform;
    UniformBuffer fragmentUniform;

    RenderObjectVector3 position;
    RenderObjectVector3 rotation;

    // This is mostly used internally for rotations
    versor quaternion;
} RenderObject;

int render_object_create(RenderObject* object, GraphicsPipeline* pipeline, Material* material);

int render_object_destroy(RenderObject* object);

#endif