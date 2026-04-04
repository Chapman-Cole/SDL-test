#ifndef UNIFORMBUFFER_H
#define UNIFORMBUFFER_H

#include <stdint.h>
#include "ShaderUniformLayout.h"

// Below are some useful defines for engine conventions
#define UNIFORM_FRAGMENT_MATERIAL_SLOT 0
#define UNIFORM_FRAGMENT_USER_FRAME_DATA_SLOT 1
#define UNIFORM_FRAGMENT_USER_OBJECT_DATA_SLOT 2

#define UNIFORM_VERTEX_ENGINE_FRAME_DATA_SLOT 0
#define UNIFORM_VERTEX_ENGINE_OBJECT_DATA_SLOT 1
#define UNIFORM_VERTEX_USER_FRAME_DATA_SLOT 2
#define UNIFORM_VERTEX_USER_OBJECT_DATA_SLOT 3

typedef struct UniformBuffer {
    ShaderUniformLayout* layout;
    uint8_t* uniform;
    uint32_t uniformSize;
} UniformBuffer;

typedef enum UniformBufferShaderTypes {
    UNIFORM_BUFFER_VERTEX,
    UNIFORM_BUFFER_FRAGMENT
} UniformBufferShaderTypes;


typedef struct UBElementHandle {
    int32_t index;
    // Refers to whether the index is for the vertex or fragment layout buffer
    // Can be either UNIFORM_BUFFER_VERTEX or UNIFORM_BUFFER_FRAGMENT
    uint8_t shaderType;
} UBElementHandle;

#define INVALID_UNIFORM_BUFFER_ELEMENT_HANDLE (UBElementHandle){.index = -1, .shaderType = UINT8_MAX}

int uniform_buffer_create(UniformBuffer* uniform, ShaderUniformLayout* layout, uint8_t slot);

int uniform_buffer_destroy(UniformBuffer* uniform);

// #####################################################################
// # Vulkan std140 imposes strict alignment and padding restrictions,  #
// # meaning that it is highly inefficient to use arrays with it.      #
// # Consequently, I will not be providing any functions to use arrays #
// # with uniforms due to the severe memory inefficiency, among other  #
// # associated technical challenges.                                  #
// #####################################################################

// ############################
// # Uniform Buffer Accessors #
// ############################

// <<<<<<<<<<<<<<<<<< Primitives >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

int32_t uniform_buffer_get_int(UniformBuffer* uniform, UBElementHandle handle);

uint32_t uniform_buffer_get_uint(UniformBuffer* uniform, UBElementHandle handle);

float uniform_buffer_get_float(UniformBuffer* uniform, UBElementHandle handle);

// --------------------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<<< Vector Types >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

const int32_t* uniform_buffer_get_ivec(UniformBuffer* uniform, UBElementHandle handle, uint32_t* len);

const uint32_t* uniform_buffer_get_uvec(UniformBuffer* uniform, UBElementHandle handle, uint32_t* len);

const float* uniform_buffer_get_vec(UniformBuffer* uniform, UBElementHandle handle, uint32_t* len);

// --------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<< Matrix Type >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Matrices are stored in memory in column-major format, meaning the
// columns are lined up contigously in memory

const float* uniform_buffer_get_mat(UniformBuffer* uniform, UBElementHandle handle, uint32_t* numCols, uint32_t* numRows);

// --------------------------------------------------------------------

// ############################
// # Uniform Buffer Modifiers #
// ############################

// <<<<<<<<<<<<<<<<<< Primitives >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void uniform_buffer_set_int(UniformBuffer* uniform, UBElementHandle handle, int32_t input);

void uniform_buffer_set_uint(UniformBuffer* uniform, UBElementHandle handle, uint32_t input);

void uniform_buffer_set_float(UniformBuffer* uniform, UBElementHandle handle, float input);

// ----------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<<< Vector Types >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void uniform_buffer_set_ivec(UniformBuffer* uniform, UBElementHandle handle, int32_t* input, uint32_t len);

void uniform_buffer_set_uvec(UniformBuffer* uniform, UBElementHandle handle, uint32_t* input, uint32_t len);

void uniform_buffer_set_vec(UniformBuffer* uniform, UBElementHandle handle, float* input, uint32_t len);

// --------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<< Matrix Type >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Matrices are stored in memory in column-major format, meaning the
// columns are lined up contigously in memory

void uniform_buffer_set_mat(UniformBuffer* uniform, UBElementHandle handle, float* input, uint32_t numCols, uint32_t numRows);

// --------------------------------------------------------------------

#endif