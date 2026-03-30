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

    // 16 byte alignment is necessary in order to conform to vulkan std140. 
    // It is also necessary for alignment purposes pertaining to cglm mat4 type
    for (int i = 0; i < pipeline->vertexLayout.num_uniform_buffers; i++) {
        mat->vertUniformBuffersSizes[i] = pipeline->vertexLayout.bufferSizes[i];
        mat->vertUniformBuffers[i] = SDL_aligned_alloc(16, mat->vertUniformBuffersSizes[i] * sizeof(uint8_t));
    }

    for (int i = 0; i < pipeline->fragmentLayout.num_uniform_buffers; i++) {
        mat->fragUniformBuffersSizes[i] = pipeline->fragmentLayout.bufferSizes[i];
        mat->fragUniformBuffers[i] = SDL_aligned_alloc(16, mat->fragUniformBuffersSizes[i] * sizeof(uint8_t));
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

uint32_t material_get_vertex_uniform_index(Material* mat, string* name) {
    for (int i = 0; i < mat->pipeline->vertexLayout.uniformElementsLen; i++) {
        if (string_compare(name, &mat->pipeline->vertexLayout.uniformElements[i].name) == true) {
            return i;
        }
    }

    return -1;
}

uint32_t material_get_fragment_uniform_index(Material* mat, string* name) {
    for (int i = 0; i < mat->pipeline->fragmentLayout.uniformElementsLen; i++) {
        if (string_compare(name, &mat->pipeline->fragmentLayout.uniformElements[i].name) == true) {
            return i;
        }
    }

    return -1;
}

// <<<<<<<<<<<<<<<<<<< Material Vertex Accessors >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

int32_t material_get_vertex_uniform_int(Material* mat, uint32_t index) {
    if (
        index < 0 || 
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_INT
    ) {
        SDL_Log("Error in material_get_vertex_uniform_int: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    return *(int32_t*)(mat->vertUniformBuffers[element.bindingNum] + element.offset);
}

uint32_t material_get_vertex_uniform_uint(Material* mat, uint32_t index) {
    if (
        index < 0 || 
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_UINT
    ) {
        SDL_Log("Error in material_get_vertex_uniform_uint: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    return *(uint32_t*)(mat->vertUniformBuffers[element.bindingNum] + element.offset);
}

float material_get_vertex_uniform_float(Material* mat, uint32_t index) {
    if (
        index < 0 || 
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_FLOAT
    ) {
        SDL_Log("Error in material_get_vertex_uniform_float: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    return *(float*)(mat->vertUniformBuffers[element.bindingNum] + element.offset);
}

int32_t* material_get_vertex_uniform_ivec(Material* mat, uint32_t index, uint32_t* len) {
    if (
        index < 0 || 
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_IVEC
    ) {
        SDL_Log("Error in material_get_vertex_uniform_ivec: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }
    
    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    *len = element.vecLen;
    return (int32_t*)(mat->vertUniformBuffers[element.bindingNum] + element.offset);
}

uint32_t* material_get_vertex_uniform_uvec(Material* mat, uint32_t index, uint32_t* len) {
    if (
        index < 0 || 
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_UVEC
    ) {
        SDL_Log("Error in material_get_vertex_uniform_uvec: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }
    
    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    *len = element.vecLen;
    return (uint32_t*)(mat->vertUniformBuffers[element.bindingNum] + element.offset);
}

float* material_get_vertex_uniform_vec(Material* mat, uint32_t index, uint32_t* len) {
    if (
        index < 0 || 
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_VEC
    ) {
        SDL_Log("Error in material_get_vertex_uniform_vec: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }
    
    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    *len = element.vecLen;
    return (float*)(mat->vertUniformBuffers[element.bindingNum] + element.offset);
}

float* material_get_vertex_mat(Material* mat, uint32_t index, uint32_t* numCols, uint32_t* numRows) {
    if (
        index < 0 || 
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_MAT
    ) {
        SDL_Log("Error in material_get_vertex_mat: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }
    
    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    *numCols = element.numCols;
    *numRows = element.numRows;
    return (float*)(mat->vertUniformBuffers[element.bindingNum] + element.offset);
}

// --------------------------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<<<<<<< Material Fragment Accessors >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

int32_t material_get_fragment_uniform_int(Material* mat, uint32_t index) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_INT
    ) {
        SDL_Log("Error in material_get_fragment_uniform_int: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    return *(int32_t*)(mat->fragUniformBuffers[element.bindingNum] + element.offset);
}

uint32_t material_get_fragment_uniform_uint(Material* mat, uint32_t index) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_UINT
    ) {
        SDL_Log("Error in material_get_fragment_uniform_uint: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    return *(uint32_t*)(mat->fragUniformBuffers[element.bindingNum] + element.offset);
}

float material_get_fragment_uniform_float(Material* mat, uint32_t index) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_FLOAT
    ) {
        SDL_Log("Error in material_get_fragment_uniform_float: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    return *(float*)(mat->fragUniformBuffers[element.bindingNum] + element.offset);
}

int32_t* material_get_fragment_uniform_ivec(Material* mat, uint32_t index, uint32_t* len) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_IVEC
    ) {
        SDL_Log("Error in material_get_fragment_uniform_ivec: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    *len = element.vecLen;
    return (int32_t*)(mat->fragUniformBuffers[element.bindingNum] + element.offset);
}

uint32_t* material_get_fragment_uniform_uvec(Material* mat, uint32_t index, uint32_t* len) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_UVEC
    ) {
        SDL_Log("Error in material_get_fragment_uniform_uvec: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    *len = element.vecLen;
    return (uint32_t*)(mat->fragUniformBuffers[element.bindingNum] + element.offset);
}

float* material_get_fragment_uniform_vec(Material* mat, uint32_t index, uint32_t* len) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_VEC
    ) {
        SDL_Log("Error in material_get_fragment_uniform_vec: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    *len = element.vecLen;
    return (float*)(mat->fragUniformBuffers[element.bindingNum] + element.offset);
}

float* material_get_fragment_mat(Material* mat, uint32_t index, uint32_t* numCols, uint32_t* numRows) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_MAT
    ) {
        SDL_Log("Error in material_get_fragment_mat: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    *numCols = element.numCols;
    *numRows = element.numRows;
    return (float*)(mat->fragUniformBuffers[element.bindingNum] + element.offset);
}

// --------------------------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<<<<<<<<< Material Vertex Modifiers >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

uint32_t material_set_vertex_uniform_int(Material* mat, uint32_t index, int32_t input) {
    if (
        index < 0 ||
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_INT
    ) {
        SDL_Log("Error in material_set_vertex_uniform_int: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    *(int32_t*)(mat->vertUniformBuffers[element.bindingNum] + element.offset) = input;

    return 0;
}

uint32_t material_set_vertex_uniform_uint(Material* mat, uint32_t index, uint32_t input) {
    if (
        index < 0 ||
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_UINT
    ) {
        SDL_Log("Error in material_set_vertex_uniform_uint: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    *(uint32_t*)(mat->vertUniformBuffers[element.bindingNum] + element.offset) = input;

    return 0;
}

uint32_t material_set_vertex_uniform_float(Material* mat, uint32_t index, float input) {
    if (
        index < 0 ||
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_FLOAT
    ) {
        SDL_Log("Error in material_set_vertex_uniform_float: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    *(float*)(mat->vertUniformBuffers[element.bindingNum] + element.offset) = input;

    return 0;
}

uint32_t material_set_vertex_uniform_ivec(Material* mat, uint32_t index, int32_t* input, uint32_t len) {
    if (
        index < 0 ||
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_IVEC ||
        mat->pipeline->vertexLayout.uniformElements[index].vecLen != len
    ) {
        SDL_Log("Error in material_set_vertex_uniform_ivec: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    SDL_memcpy(mat->vertUniformBuffers[element.bindingNum] + element.offset, input, len * sizeof(int32_t));

    return 0;
}

uint32_t material_set_vertex_uniform_uvec(Material* mat, uint32_t index, uint32_t* input, uint32_t len) {
    if (
        index < 0 ||
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_UVEC ||
        mat->pipeline->vertexLayout.uniformElements[index].vecLen != len
    ) {
        SDL_Log("Error in material_set_vertex_uniform_uvec: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    SDL_memcpy(mat->vertUniformBuffers[element.bindingNum] + element.offset, input, len * sizeof(uint32_t));

    return 0;
}

uint32_t material_set_vertex_uniform_vec(Material* mat, uint32_t index, float* input, uint32_t len) {
    if (
        index < 0 ||
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_VEC ||
        mat->pipeline->vertexLayout.uniformElements[index].vecLen != len
    ) {
        SDL_Log("Error in material_set_vertex_uniform_vec: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    SDL_memcpy(mat->vertUniformBuffers[element.bindingNum] + element.offset, input, len * sizeof(float));

    return 0;
}

uint32_t material_set_vertex_mat(Material* mat, uint32_t index, float* input, uint32_t numCols, uint32_t numRows) {
    if (
        index < 0 ||
        index >= mat->pipeline->vertexLayout.uniformElementsLen ||
        mat->pipeline->vertexLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_MAT ||
        mat->pipeline->vertexLayout.uniformElements[index].numCols != numCols ||
        mat->pipeline->vertexLayout.uniformElements[index].numRows != numRows
    ) {
        SDL_Log("Error in material_set_vertex_mat: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->vertexLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->vertexLayout.uniformElements[index];
    SDL_memcpy(mat->vertUniformBuffers[element.bindingNum] + element.offset, input, (numCols * numRows) * sizeof(float));

    return 0;
}

// --------------------------------------------------------------------------------------

// <<<<<<<<<<<<<<<<<<<<< Material Fragment Modifiers >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

uint32_t material_set_fragment_uniform_int(Material* mat, uint32_t index, int32_t input) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_INT
    ) {
        SDL_Log("Error in material_set_fragment_uniform_int: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    *(int32_t*)(mat->fragUniformBuffers[element.bindingNum] + element.offset) = input;

    return 0;
}

uint32_t material_set_fragment_uniform_uint(Material* mat, uint32_t index, uint32_t input) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_UINT
    ) {
        SDL_Log("Error in material_set_fragment_uniform_uint: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    *(uint32_t*)(mat->fragUniformBuffers[element.bindingNum] + element.offset) = input;

    return 0;
}

uint32_t material_set_fragment_uniform_float(Material* mat, uint32_t index, float input) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_FLOAT
    ) {
        SDL_Log("Error in material_set_fragment_uniform_float: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    *(float*)(mat->fragUniformBuffers[element.bindingNum] + element.offset) = input;

    return 0;
}

uint32_t material_set_fragment_uniform_ivec(Material* mat, uint32_t index, int32_t* input, uint32_t len) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_IVEC ||
        mat->pipeline->fragmentLayout.uniformElements[index].vecLen != len
    ) {
        SDL_Log("Error in material_set_fragment_uniform_ivec: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    SDL_memcpy(mat->fragUniformBuffers[element.bindingNum] + element.offset, input, len * sizeof(int32_t));

    return 0;
}

uint32_t material_set_fragment_uniform_uvec(Material* mat, uint32_t index, uint32_t* input, uint32_t len) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_UVEC ||
        mat->pipeline->fragmentLayout.uniformElements[index].vecLen != len
    ) {
        SDL_Log("Error in material_set_fragment_uniform_uvec: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    SDL_memcpy(mat->fragUniformBuffers[element.bindingNum] + element.offset, input, len * sizeof(uint32_t));

    return 0;
}

uint32_t material_set_fragment_uniform_vec(Material* mat, uint32_t index, float* input, uint32_t len) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_VEC ||
        mat->pipeline->fragmentLayout.uniformElements[index].vecLen != len
    ) {
        SDL_Log("Error in material_set_fragment_uniform_vec: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    SDL_memcpy(mat->fragUniformBuffers[element.bindingNum] + element.offset, input, len * sizeof(float));

    return 0;
}

uint32_t material_set_fragment_mat(Material* mat, uint32_t index, float* input, uint32_t numCols, uint32_t numRows) {
    if (
        index < 0 ||
        index >= mat->pipeline->fragmentLayout.uniformElementsLen ||
        mat->pipeline->fragmentLayout.uniformElements[index].type != UNIFORM_SHADER_TYPE_MAT ||
        mat->pipeline->fragmentLayout.uniformElements[index].numCols != numCols ||
        mat->pipeline->fragmentLayout.uniformElements[index].numRows != numRows
    ) {
        SDL_Log("Error in material_set_fragment_mat: Arguments: type=%s index=%u ", 
            shader_uniform_element_type_get_name(mat->pipeline->fragmentLayout.uniformElements[index].type),
            index
        );
        SDL_Quit();
        exit(-1);
    }

    UniformElementType element = mat->pipeline->fragmentLayout.uniformElements[index];
    SDL_memcpy(mat->fragUniformBuffers[element.bindingNum] + element.offset, input, (numCols * numRows) * sizeof(float));

    return 0;
}

// --------------------------------------------------------------------------------------