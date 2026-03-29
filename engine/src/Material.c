#include "Material.h"

int material_create(Material* mat, GraphicsPipeline* pipeline) {
    mat->pipeline = pipeline;

    mat->vertUniformBuffers[0] = NULL;
    mat->vertUniformBuffers[1] = NULL;
    mat->vertUniformBuffers[2] = NULL;
    mat->vertUniformBuffers[3] = NULL;

    mat->vertUniformBuffersSizes[0] = 0;
    mat->vertUniformBuffersSizes[1] = 0;
    mat->vertUniformBuffersSizes[2] = 0;
    mat->vertUniformBuffersSizes[3] = 0;

    mat->fragUniformBuffers[0] = NULL;
    mat->fragUniformBuffers[1] = NULL;
    mat->fragUniformBuffers[2] = NULL;
    mat->fragUniformBuffers[3] = NULL;

    mat->fragUniformBuffersSizes[0] = 0;
    mat->fragUniformBuffersSizes[1] = 0;
    mat->fragUniformBuffersSizes[2] = 0;
    mat->fragUniformBuffersSizes[3] = 0;

    for (int i = 0; i < pipeline->vertexLayout.num_uniform_buffers; i++) {
        mat->vertUniformBuffersSizes[i] = pipeline->vertexLayout.bufferSizes[i];
        mat->vertUniformBuffers[i] = SDL_malloc(mat->vertUniformBuffersSizes[i] * sizeof(uint8_t));
    }

    for (int i = 0; i < pipeline->fragmentLayout.num_uniform_buffers; i++) {
        mat->fragUniformBuffersSizes[i] = pipeline->fragmentLayout.bufferSizes[i];
        mat->fragUniformBuffers[i] = SDL_malloc(mat->fragUniformBuffersSizes[i] * sizeof(uint8_t));
    }

    return 0;
}

int material_destroy(Material* mat) {
    for (int i = 0; i < mat->pipeline->vertexLayout.num_uniform_buffers; i++) {
        SDL_free(mat->vertUniformBuffers[i]);
        mat->vertUniformBuffers[i] = NULL;
        mat->vertUniformBuffersSizes[i] = 0;
    }

    for (int i = 0; i < mat->pipeline->fragmentLayout.num_uniform_buffers; i++) {
        SDL_free(mat->fragUniformBuffers[i]);
        mat->fragUniformBuffers[i] = NULL;
        mat->vertUniformBuffersSizes[i] = 0;
    }

    mat->pipeline = NULL;

    return 0;
}

uint32_t material_get_vertex_property_index(Material* mat, string* name) {
    for (int i = 0; i < mat->pipeline->vertexLayout.uniformElementsLen; i++) {
        if (string_compare(name, &mat->pipeline->vertexLayout.uniformElements[i].name) == true) {
            return i;
        }
    }

    return -1;
}

uint32_t material_get_fragment_property_index(Material* mat, string* name) {
    for (int i = 0; i < mat->pipeline->fragmentLayout.uniformElementsLen; i++) {
        if (string_compare(name, &mat->pipeline->fragmentLayout.uniformElements[i].name) == true) {
            return i;
        }
    }

    return -1;
}

int material_get_vertex_property(Material* mat, uint32_t index) {
    
}

int material_get_fragment_property(Material* mat, uint32_t index) {

}