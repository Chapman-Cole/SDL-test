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
uint32_t material_get_vertex_uniform_index(Material* mat, string* name);

// Returns the index of the specified uniform element with the given name in the 
// fragment UniformElementType array pointed to by the material itself
uint32_t material_get_fragment_uniform_index(Material* mat, string* name);

// #####################################################################
// # Vulkan std140 imposes strict alignment and padding restrictions,  #
// # meaning that it is highly inefficient to use arrays with it.      #
// # Consequently, I will not be providing any functions to use arrays #
// # with uniforms due to the severe memory inefficiency, among other  #
// # associated technical challenges.                                  #
// #####################################################################

// ######################
// # Material Accessors #
// ######################

// Note that the indices you pass are fetched from 
// material_get_vertex_uniform_index or material_get_fragment_uniform_index

// ############################
// # Vertex Uniform Functions #
// ############################

// <<<<<<<<<<<<<<<<<< Primitives >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

int32_t material_get_vertex_uniform_int(Material* mat, uint32_t index);

uint32_t material_get_vertex_uniform_uint(Material* mat, uint32_t index);

float material_get_vertex_uniform_float(Material* mat, uint32_t index);

// --------------------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<<< Vector Types >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

int32_t* material_get_vertex_uniform_ivec(Material* mat, uint32_t index, uint32_t* len);

uint32_t* material_get_vertex_uniform_uvec(Material* mat, uint32_t index, uint32_t* len);

float* material_get_vertex_uniform_vec(Material* mat, uint32_t index, uint32_t* len);

// --------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<< Matrix Type >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Matrices are stored in memory in column-major format, meaning the
// columns are lined up contigously in memory

float* material_get_vertex_mat(Material* mat, uint32_t index, uint32_t* numCols, uint32_t* numRows);

// --------------------------------------------------------------------

// ##############################
// # Fragment Uniform Functions #
// ##############################

// <<<<<<<<<<<<<<<<<< Primitives >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

int32_t material_get_fragment_uniform_int(Material* mat, uint32_t index);

uint32_t material_get_fragment_uniform_uint(Material* mat, uint32_t index);

float material_get_fragment_uniform_float(Material* mat, uint32_t index);

// --------------------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<<< Vector Types >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

int32_t* material_get_fragment_uniform_ivec(Material* mat, uint32_t index, uint32_t* len);

uint32_t* material_get_fragment_uniform_uvec(Material* mat, uint32_t index, uint32_t* len);

float* material_get_fragment_uniform_vec(Material* mat, uint32_t index, uint32_t* len);

// --------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<< Matrix Type >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Matrices are stored in memory in column-major format, meaning the
// columns are lined up contigously in memory

float* material_get_fragment_mat(Material* mat, uint32_t index, uint32_t* numCols, uint32_t* numRows);

// --------------------------------------------------------------------

// ######################
// # Material Modifiers #
// ######################

// Note that the indices you pass are fetched from 
// material_get_vertex_uniform_index or material_get_fragment_uniform_index

// ############################
// # Vertex Uniform Functions #
// ############################

// <<<<<<<<<<<<<<<<<< Primitives >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

uint32_t material_set_vertex_uniform_int(Material* mat, uint32_t index, int32_t input);

uint32_t material_set_vertex_uniform_uint(Material* mat, uint32_t index, uint32_t input);

uint32_t material_set_vertex_uniform_float(Material* mat, uint32_t index, float input);

// ----------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<<< Vector Types >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

uint32_t material_set_vertex_uniform_ivec(Material* mat, uint32_t index, int32_t* input, uint32_t len);

uint32_t material_set_vertex_uniform_uvec(Material* mat, uint32_t index, uint32_t* input, uint32_t len);

uint32_t material_set_vertex_uniform_vec(Material* mat, uint32_t index, float* input, uint32_t len);

// --------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<< Matrix Type >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Matrices are stored in memory in column-major format, meaning the
// columns are lined up contigously in memory

uint32_t material_set_vertex_mat(Material* mat, uint32_t index, float* input, uint32_t numCols, uint32_t numRows);

// --------------------------------------------------------------------

// ##############################
// # Fragment Uniform Functions #
// ##############################

// <<<<<<<<<<<<<<<<<< Primitives >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

uint32_t material_set_fragment_uniform_int(Material* mat, uint32_t index, int32_t input);

uint32_t material_set_fragment_uniform_uint(Material* mat, uint32_t index, uint32_t input);

uint32_t material_set_fragment_uniform_float(Material* mat, uint32_t index, float input);

// ----------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<<< Vector Types >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

uint32_t material_set_fragment_uniform_ivec(Material* mat, uint32_t index, int32_t* input, uint32_t len);

uint32_t material_set_fragment_uniform_uvec(Material* mat, uint32_t index, uint32_t* input, uint32_t len);

uint32_t material_set_fragment_uniform_vec(Material* mat, uint32_t index, float* input, uint32_t len);

// --------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<< Matrix Type >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Matrices are stored in memory in column-major format, meaning the
// columns are lined up contigously in memory

uint32_t material_set_fragment_mat(Material* mat, uint32_t index, float* input, uint32_t numCols, uint32_t numRows);

// --------------------------------------------------------------------

#endif