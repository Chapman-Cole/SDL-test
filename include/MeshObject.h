#ifndef MESHOBJECT_H
#define MESHOBJECT_H

#include <SDL3/SDL.h>
#include "SDLDevice.h"
#include "Strings.h"
#include "GPUBuffers.h"

// To Do:
// Add better support for vertex attrbutes with meshes, like normal data

// Mesh structs will contain a buffer for the vertices and indices
// Later on I would like to add some sort of system for attaching
// Mesh structs to a specific graphics pipeline so that way it can 
// be shaded according to the shaders in the graphics pipeline
typedef struct Mesh
{
    SDL_GPUBuffer* vertexBuffer;
    SDL_GPUBuffer* indexBuffer;

    // Needed to let SDL know how many 
    // indices there are when rendering
    Uint32 numIndices;
} Mesh;

// Call before using the mesh object so that it is properly initialized
void meshobject_init(Mesh* mesh);

// Call once done with the mesh object
void meshobject_destroy(Mesh* mesh);

// #######################################
// #              Note                   #
// # Loading mesh objects only preps the #
// # buffers for transfer to the gpu. It #
// # It is up to you to call             #
// # GPB_submit_all_transfer_buffers();  #
// # in order to upload the data to the  #
// # gpu.                                #
// #######################################

// Loads the vertex and index data manually by passing in pointers to the buffers
// and the size of each buffer in bytes
void meshobject_load_manual(Mesh* mesh, float* vertices, Uint32 vertexSize, Uint32* indices, Uint32 indexSize);

// Loads an obj file and extracts the vertex and index data to make the 
// vertex and index buffers
void meshobject_load_objfile(Mesh* mesh, string path);

// Binds the mesh data so it is prepped for rendering, and then renders it
void meshobject_render(Mesh* mesh, SDL_GPURenderPass* renderPass);



#endif