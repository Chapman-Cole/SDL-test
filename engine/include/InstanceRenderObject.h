#ifndef INSTANCERENDEROBJECT_H
#define INSTANCERENDEROBJECT_H

#include "GraphicsPipeline.h"
#include "Material.h"
#include "UniformBuffer.h"
#include "cglm/cglm.h"
#include "MeshObject.h"
#include "GPUBuffers.h"

#define MAX_INSTANCE_BUFFERS 5

typedef struct InstanceRenderObject {
    GraphicsPipeline* pipeline;
    Material* material;
    Mesh mesh;

    // These would be applied to every instance in the vertex shader, if you choose to set it up that way
    versor quaternion;
    vec3 scale;

    // The object specific uniforms
    UniformBuffer vertexUniform;
    UniformBuffer fragmentUniform;

    uint32_t instanceSlot;
    uint32_t numInstances;

    GPUBuffer* instanceBuffers[MAX_INSTANCE_BUFFERS];
    uint32_t numInstanceBuffers;
} InstanceRenderObject;

int instance_render_object_create(InstanceRenderObject* object, GraphicsPipeline* pipeline, Material* material, uint32_t instanceSlot);

int instance_render_object_add_instance_buffer(InstanceRenderObject* object, GPUBuffer* buffer);

int instance_render_object_destroy(InstanceRenderObject* object);

#endif