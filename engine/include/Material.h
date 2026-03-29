#ifndef MATERIAL_H
#define MATERIAL_H

#include "GraphicsPipeline.h"

// Contains a reference to a graphics pipeline object
typedef struct Material {
    // The pointer to the GraphicsPipeline object
    GraphicsPipeline* pipeline;

    // uint8_t* is important for below because I want to be able to use byte offsets
    // This will store fragment shader uniform info in the Vulkan std140 format
    uint8_t* fragUniformBuffers[4];
    uint32_t fragUniformBuffersSizes[4];

    // This will store the vertex shader uniform info in the Vulkan std140 format
    uint8_t* vertUniformBuffers[4];
    uint32_t vertUniformBuffersSizes[4];
} Material;

// Creates the material using the info in the created graphics pipeline
int material_create(Material* mat, GraphicsPipeline* pipeline);

// Frees up the memory used by the material
int material_destroy(Material* mat);

// Returns the index of the specified uniform element with the given name in the 
// vertex UniformElementType array pointed to by the material itself. 
uint32_t material_get_vertex_property_index(Material* mat, string* name);

// Returns the index of the specified uniform element with the given name in the 
// fragment UniformElementType array pointed to by the material itself
uint32_t material_get_fragment_property_index(Material* mat, string* name);

// Allows you to get the value of a specific property using its index in the vertex UniformElementType array
int material_get_vertex_property(Material* mat, uint32_t index);

// Allows you to get the value of a specific property using its index in the fragment UniformElementType array
int material_get_fragment_property(Material* mat, uint32_t index);

#endif