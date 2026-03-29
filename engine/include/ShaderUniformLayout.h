#ifndef SHADERUNIFORMLAYOUT_H
#define SHADERUNIFORMLAYOUT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <Strings.h>

typedef enum UniformShaderTypes {
    // Primitive data types
    UNIFORM_SHADER_TYPE_BOOL,
    UNIFORM_SHADER_TYPE_INT,
    UNIFORM_SHADER_TYPE_UINT,
    UNIFORM_SHADER_TYPE_FLOAT,
    UNIFORM_SHADER_TYPE_DOUBLE,

    // Vector types
    UNIFORM_SHADER_TYPE_BVEC,
    UNIFORM_SHADER_TYPE_IVEC,
    UNIFORM_SHADER_TYPE_UVEC,
    UNIFORM_SHADER_TYPE_VEC,
    UNIFORM_SHADER_TYPE_DVEC,

    // Matrix types (for mat nxm, n is the number of columns and m is the number of rows (opposite of mathematical convention))
    // matrices consist of only floats or doubles
    UNIFORM_SHADER_TYPE_MAT,
    UNIFORM_SHADER_TYPE_DMAT,

    // This handles all of the array types
    // This handles the primitive array types
    UNIFORM_SHADER_TYPE_BOOL_ARRAY,
    UNIFORM_SHADER_TYPE_INT_ARRAY,
    UNIFORM_SHADER_TYPE_UINT_ARRAY,
    UNIFORM_SHADER_TYPE_FLOAT_ARRAY,
    UNIFORM_SHADER_TYPE_DOUBLE_ARRAY,

    // This handles the vector array types. Something like vec3[4]
    UNIFORM_SHADER_TYPE_BVEC_ARRAY,
    UNIFORM_SHADER_TYPE_IVEC_ARRAY,
    UNIFORM_SHADER_TYPE_UVEC_ARRAY,
    UNIFORM_SHADER_TYPE_VEC_ARRAY,
    UNIFORM_SHADER_TYPE_DVEC_ARRAY,

    // This handles the matrix array types
    UNIFORM_SHADER_TYPE_MAT_ARRAY,
    UNIFORM_SHADER_TYPE_DMAT_ARRAY,

    // This is for when an unsupported type is come across
    UNIFORM_SHADER_TYPE_UNSUPPORTED
} UniformShaderTypes;

// Stores the name of an element of a uniform struct and its
// associated type
typedef struct UniformElementType {
    // Refers to the data type of the uniform element, which should correspond to
    // one of the types outlined in the UniformShaderTypes enum
    uint8_t type;

    // The size of the element in bytes
    uint32_t sizeBytes;

    // The size of the element accounting for padding in bytes
    uint32_t sizePaddedBytes;

    // The specific uniform buffer this element belongs in
    uint32_t bindingNum;

    // The offset into the uniform buffer
    uint32_t offset;

    // The name extracted from spirv
    string name;
} UniformElementType;

// This struct will define how uniform memory is laid out for the shader, and provide a set of functions
// for easily setting the named values in the uniform
typedef struct ShaderUniformLayout {
    uint32_t num_samplers;
    uint32_t num_storage_buffers;
    uint32_t num_storage_textures;
    uint32_t num_uniform_buffers;

    // This stores the elements of the uniform buffer, in the order that they 
    // appear in the uniform buffer of the shader
    UniformElementType* uniformElements;
    uint32_t uniformElementsLen;
    uint32_t uniformElementsCapacity;

    // SDL3 gpu api allows a maximum of 4 uniform buffers, and below will store the size in bytes
    // of all the different buffers (if there are that many)
    uint32_t bufferSizes[4];
} ShaderUniformLayout;

// Initializes the shader uniform layout passed in
int shader_uniform_layout_init(ShaderUniformLayout* shaderLayout);

// Properly destroys the initialized ShaderUniformLayout struct
int shader_uniform_layout_destroy(ShaderUniformLayout* shaderLayout);

// Extracts the shader layout uniform info using
// fields of the struct using spirv reflect. This is mostly for internal use to the shader compilation api.
// spirv_file - A pointer to a string containing the spir-v code
// shaderLayout - A pointer to a shader layout struct that will store the uniform layout info
int extract_shader_binding_info(string* spirv_file, ShaderUniformLayout* shaderLayout);

// Appends an element to the uniformElements array in the given ShaderUniformLayout struct
// shaderLayout - A pointer to a ShaderUniformLayout struct that contains the given uniformElements array
// element - A pointer to a UniformElementType struct that contains the data about the element of the uniform
int shader_uniform_layout_append_element(ShaderUniformLayout* shaderLayout, UniformElementType* element);

// A simple helper function that takes an input of the of the defined types in the UniformShaderTypes enum
// and returns a string of its name
const char* shader_uniform_element_type_get_name(uint8_t elementType);

// Prints out the elements of the shaderLayout struct for debugging purposes. Specifically
// the array of elements
void shader_uniform_elements_print(ShaderUniformLayout* shaderLayout);

#endif