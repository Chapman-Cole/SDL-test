#ifndef INSTANCERENDEROBJECT_H
#define INSTANCERENDEROBJECT_H

#include "GraphicsPipeline.h"
#include "Material.h"
#include "UniformBuffer.h"
#include "cglm/cglm.h"
#include "MeshObject.h"
#include "GPUBuffers.h"

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

    void* instanceData;
    uint32_t instanceStepSize;
    uint32_t numInstances;

    SDL_GPUBuffer* instanceBuffer;
} InstanceRenderObject;

int instance_render_object_create(InstanceRenderObject* object, GraphicsPipeline* pipeline, Material* material, uint32_t instanceSlot);

int instance_render_object_destroy(InstanceRenderObject* object);

int instance_render_object_instantiate(InstanceRenderObject* object, uint32_t numInstances, uint32_t instanceStepSize);

int instance_render_object_upload(InstanceRenderObject* object);

#endif