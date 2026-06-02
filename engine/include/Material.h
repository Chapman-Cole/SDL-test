#ifndef MATERIAL_H
#define MATERIAL_H

#include "GraphicsPipeline.h"
#include "UniformBuffer.h"
#include <stdatomic.h>

#define MAX_TEXTURE_SAMPLER_PAIRS 16

static _Atomic uint32_t mat_idCount = 0;

typedef struct TextureSamplerPair {
    SDL_GPUTexture* texture;
    SDL_GPUSampler* sampler;
} TextureSamplerPair;


typedef struct Material {
    uint32_t id;
    UniformBuffer uniform;

    TextureSamplerPair textureSamplerPairs[MAX_TEXTURE_SAMPLER_PAIRS];
    uint32_t numTextureSamplerPairs;
} Material;

// Creates the material using the info in the created graphics pipeline
int material_create(Material* mat, GraphicsPipeline* pipeline);

// Frees up the memory used by the material
int material_destroy(Material* mat);

int material_append_texture_sampler_pair(Material* mat, SDL_GPUTexture* tex, SDL_GPUSampler* samp);

// Index cannot go over 4
int material_set_texture_sampler_pair(Material* mat, uint32_t index, SDL_GPUTexture* tex, SDL_GPUSampler* samp);

// Returns the index (a MaterialElementHandle) of the specified uniform element with the given name in 
// slot 0 as defined by the convention
UBElementHandle material_get_handle(Material* mat, string* name);

#endif