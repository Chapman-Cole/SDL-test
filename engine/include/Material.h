#ifndef MATERIAL_H
#define MATERIAL_H

#include "GraphicsPipeline.h"
#include "UniformBuffer.h"
#include <stdatomic.h>

static _Atomic(uint32_t) mat_idCount = 0;

typedef struct Material {
    uint32_t id;
    UniformBuffer uniform;
} Material;

// Creates the material using the info in the created graphics pipeline
int material_create(Material* mat, GraphicsPipeline* pipeline);

// Frees up the memory used by the material
int material_destroy(Material* mat);

// Returns the index (a MaterialElementHandle) of the specified uniform element with the given name in 
// slot 0 as defined by the convention
UBElementHandle material_get_handle(Material* mat, string* name);

#endif