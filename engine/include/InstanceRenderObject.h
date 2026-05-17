#ifndef INSTANCERENDEROBJECT_H
#define INSTANCERENDEROBJECT_H

#include "GraphicsPipeline.h"
#include "Material.h"
#include "UniformBuffer.h"
#include "cglm/cglm.h"
#include "MeshObject.h"

typedef struct InstanceRenderObject {
    GraphicsPipeline* pipeline;
    Material* material;
    Mesh mesh;
} InstanceRenderObject;

#endif