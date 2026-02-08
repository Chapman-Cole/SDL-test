#include "MeshObject.h"

void meshobject_init(Mesh* mesh) {
    mesh->vertexBuffer = NULL;
    mesh->indexBuffer = NULL;
}

void meshobject_destroy(Mesh* mesh) {
    SDL_ReleaseGPUBuffer(get_SDL_gpu_device(), mesh->vertexBuffer);
    SDL_ReleaseGPUBuffer(get_SDL_gpu_device(), mesh->indexBuffer);
}

void meshobject_load_manual(Mesh* mesh, float* vertices, Uint32 vertexSize, Uint32* indices, Uint32 indexSize) {
    mesh->vertexBuffer = GPB_create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, vertices, vertexSize);
    mesh->indexBuffer = GPB_create_buffer(SDL_GPU_BUFFERUSAGE_INDEX, indices, indexSize);
    mesh->numIndices = indexSize / sizeof(Uint32);
}

void meshobject_load_objfile(Mesh* mesh, string path) {

}

void meshobject_render(Mesh* mesh, SDL_GPURenderPass* renderPass) {
    // bind the vertex buffer
    SDL_GPUBufferBinding bufferBindings[1];
    bufferBindings[0].buffer = mesh->vertexBuffer;
    bufferBindings[0].offset = 0;

    SDL_GPUBufferBinding indexBinding;
    indexBinding.buffer = mesh->indexBuffer;
    indexBinding.offset = 0;

    SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, 1);
    SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    SDL_DrawGPUIndexedPrimitives(renderPass, mesh->numIndices, 1, 0, 0, 0);
}