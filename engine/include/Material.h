#ifndef MATERIAL_H
#define MATERIAL_H

#include "GraphicsPipeline.h"

// Contains a reference to a graphics pipeline object
typedef struct Material {
    // The pointer to the GraphicsPipeline object
    GraphicsPipeline* pipeline;

    // This will store fragment shader uniform info in the Vulkan std140 format
    void* fragUniformBuffer;
    // This will store the vertex shader uniform info in the Vulkan std140 format
    void* vertUniformBuffer;
} Material;

// Initializes the Material object for later use
int material_init(Material* mat);

// Frees up the memory used by the material
int material_destroy(Material* mat);

// Creates the material using the info in the created graphics pipeline
int material_create(Material* mat, GraphicsPipeline* pipeline);

#endif