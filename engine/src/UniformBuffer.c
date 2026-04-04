#include "UniformBuffer.h"

int uniform_buffer_create(UniformBuffer* uniform, ShaderUniformLayout* layout, uint8_t slot) {
    uniform->layout = layout;

    uniform->uniformSize = layout->bufferSizes[slot];

    // No need to allocate any memory if the buffer has size 0
    if (uniform->uniformSize == 0) {
        uniform->uniform = NULL;
        return 0;
    }

    // 16 byte alignment is important for use with cglm, in which the mat4
    // type needs 16 byte alignment for efficient SIMD instructions
    uniform->uniform = SDL_aligned_alloc(16, uniform->uniformSize);

    if (uniform->uniform == NULL) {
        SDL_Log("Failed to allocate memory in uniform_buffer_create");
        SDL_Quit();
        exit(-1);
    }

    return 0;
}

int uniform_buffer_destroy(UniformBuffer* uniform) {
    uniform->layout = NULL;

    if (uniform->uniform != NULL) {
        SDL_aligned_free(uniform->uniform);
    }
    uniform->uniform = NULL;
    uniform->uniformSize = 0;
    return 0;
}

// ############################
// # Uniform Buffer Accessors #
// ############################

// <<<<<<<<<<<<<<<<<< Primitives >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

int32_t uniform_buffer_get_int(UniformBuffer* uniform, UBElementHandle handle) {
    return *(int32_t*)(uniform->uniform + uniform->layout->uniformElements[handle.index].offset);
}

uint32_t uniform_buffer_get_uint(UniformBuffer* uniform, UBElementHandle handle) {
    return *(uint32_t*)(uniform->uniform + uniform->layout->uniformElements[handle.index].offset);
}

float uniform_buffer_get_float(UniformBuffer* uniform, UBElementHandle handle) {
    return *(float*)(uniform->uniform + uniform->layout->uniformElements[handle.index].offset);
}

// --------------------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<<< Vector Types >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

const int32_t* uniform_buffer_get_ivec(UniformBuffer* uniform, UBElementHandle handle, uint32_t* len) {
    *len = uniform->layout->uniformElements[handle.index].vecLen;
    return (int32_t*)(uniform->uniform + uniform->layout->uniformElements[handle.index].offset);
}

const uint32_t* uniform_buffer_get_uvec(UniformBuffer* uniform, UBElementHandle handle, uint32_t* len) {
    *len = uniform->layout->uniformElements[handle.index].vecLen;
    return (uint32_t*)(uniform->uniform + uniform->layout->uniformElements[handle.index].offset);
}

const float* uniform_buffer_get_vec(UniformBuffer* uniform, UBElementHandle handle, uint32_t* len) {
    *len = uniform->layout->uniformElements[handle.index].vecLen;
    return (float*)(uniform->uniform + uniform->layout->uniformElements[handle.index].offset);
}

// --------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<< Matrix Type >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Matrices are stored in memory in column-major format, meaning the
// columns are lined up contigously in memory

const float* uniform_buffer_get_mat(UniformBuffer* uniform, UBElementHandle handle, uint32_t* numCols, uint32_t* numRows) {
    *numCols = uniform->layout->uniformElements[handle.index].numCols;
    *numRows = uniform->layout->uniformElements[handle.index].numRows;
    return (float*)(uniform->uniform + uniform->layout->uniformElements[handle.index].offset);
}

// --------------------------------------------------------------------

// ############################
// # Uniform Buffer Modifiers #
// ############################

// <<<<<<<<<<<<<<<<<< Primitives >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void uniform_buffer_set_int(UniformBuffer* uniform, UBElementHandle handle, int32_t input) {
    *(int32_t*)(uniform->uniform + uniform->layout->uniformElements[handle.index].offset) = input;
}

void uniform_buffer_set_uint(UniformBuffer* uniform, UBElementHandle handle, uint32_t input) {
    *(uint32_t*)(uniform->uniform + uniform->layout->uniformElements[handle.index].offset) = input;
}

void uniform_buffer_set_float(UniformBuffer* uniform, UBElementHandle handle, float input) {
    *(float*)(uniform->uniform + uniform->layout->uniformElements[handle.index].offset) = input;
}

// ----------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<<< Vector Types >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void uniform_buffer_set_ivec(UniformBuffer* uniform, UBElementHandle handle, int32_t* input, uint32_t len) {
    SDL_memcpy(uniform->uniform + uniform->layout->uniformElements[handle.index].offset, input, len * sizeof(int32_t));
}

void uniform_buffer_set_uvec(UniformBuffer* uniform, UBElementHandle handle, uint32_t* input, uint32_t len) {
    SDL_memcpy(uniform->uniform + uniform->layout->uniformElements[handle.index].offset, input, len * sizeof(uint32_t));
}

void uniform_buffer_set_vec(UniformBuffer* uniform, UBElementHandle handle, float* input, uint32_t len) {
    SDL_memcpy(uniform->uniform + uniform->layout->uniformElements[handle.index].offset, input, len * sizeof(float));
}

// --------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<< Matrix Type >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Matrices are stored in memory in column-major format, meaning the
// columns are lined up contigously in memory

void uniform_buffer_set_mat(UniformBuffer* uniform, UBElementHandle handle, float* input, uint32_t numCols, uint32_t numRows) {
    SDL_memcpy(uniform->uniform + uniform->layout->uniformElements[handle.index].offset, input, (numCols * numRows) * sizeof(float));
}

// --------------------------------------------------------------------

