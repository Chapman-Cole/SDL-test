#include "Material.h"
#include <stdatomic.h>

int material_create(Material* mat, GraphicsPipeline* pipeline) {
    mat->id = atomic_fetch_add(&mat_idCount, 1);
    uniform_buffer_create(&mat->uniform, &pipeline->fragmentLayout, UNIFORM_FRAGMENT_MATERIAL_SLOT);
    mat->numTextureSamplerPairs = 0;
    return 0;
}

int material_destroy(Material* mat) {
    uniform_buffer_destroy(&mat->uniform);
    return 0;
}

int material_append_texture_sampler_pair(Material* mat, SDL_GPUTexture* tex, SDL_GPUSampler* samp) {
    if (mat->numTextureSamplerPairs < MAX_TEXTURE_SAMPLER_PAIRS) {
        mat->textureSamplerPairs[mat->numTextureSamplerPairs] = (TextureSamplerPair){
            .sampler = samp,
            .texture = tex
        };
        mat->numTextureSamplerPairs++;
    }

    return 0;
}

int material_set_texture_sampler_pair(Material* mat, uint32_t index, SDL_GPUTexture* tex, SDL_GPUSampler* samp) {
    mat->textureSamplerPairs[index] = (TextureSamplerPair){
        .texture = tex,
        .sampler = samp
    };
    return 0;
}

UBElementHandle material_get_handle(Material* mat, string* name) {
    for (uint32_t i = 0; i < mat->uniform.layout->uniformElementsLen; i++) {
        if (
            // Material properties are specific to slot 0 for uniform buffers
            // in the fragment shader
            mat->uniform.layout->uniformElements[i].bindingNum == UNIFORM_FRAGMENT_MATERIAL_SLOT &&
            string_compare(name, &mat->uniform.layout->uniformElements[i].name) == true
        ) {
            return (UBElementHandle){.index = i, .shaderType = UNIFORM_BUFFER_FRAGMENT};
        }
    }

    return INVALID_UNIFORM_BUFFER_ELEMENT_HANDLE;
}