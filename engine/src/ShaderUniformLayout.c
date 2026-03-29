#include "ShaderUniformLayout.h"
#include "SPIRV-Reflect/spirv_reflect.h"

int shader_uniform_layout_init(ShaderUniformLayout* shaderLayout) {
    shaderLayout->num_samplers = 0;
    shaderLayout->num_storage_buffers = 0;
    shaderLayout->num_storage_textures = 0;
    shaderLayout->num_uniform_buffers = 0;

    shaderLayout->uniformElements = NULL;
    shaderLayout->uniformElementsLen = 0;
    shaderLayout->uniformElementsCapacity = 0;

    return 0;
}

int shader_uniform_layout_destroy(ShaderUniformLayout* shaderLayout) {
    for (int i = 0; i < shaderLayout->uniformElementsLen; i++) {
        string_free(&shaderLayout->uniformElements[i].name);
    }
    SDL_free(shaderLayout->uniformElements);

    shaderLayout->num_samplers = 0;
    shaderLayout->num_storage_buffers = 0;
    shaderLayout->num_storage_textures = 0;
    shaderLayout->num_uniform_buffers = 0;

    shaderLayout->uniformElements = NULL;
    shaderLayout->uniformElementsLen = 0;
    shaderLayout->uniformElementsCapacity = 0;
    
    return 0;
}

// The purpose is to take the given spirv type and convert it into the proper type that corresponds to one of the
// elements of UniformShaderTypes enum
// typeDesc - A pointer to the SpvReflectTypeDescription struct that contains the important type info
// returns the unsigned 8 bit integer that corresponds to one of UniformShaderType enum
static uint8_t extract_spirv_type_info(const SpvReflectTypeDescription* typeDesc) {
    if (typeDesc == NULL) {
        // If there is an unrecognized type, just crash the program. There could
        // be better ways to handle this in the future.
        SDL_Log("Unrecognized type in SPIRV file.");
        SDL_Quit();
        exit(-1);
    }

    // Handle all of the unsupported flags. This list is subject to change though as more features are added
    if (
        typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_EXTERNAL_ACCELERATION_STRUCTURE == true ||
        typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK == true ||
        typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_EXTERNAL_IMAGE == true ||
        typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_EXTERNAL_MASK == true ||
        typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLED_IMAGE == true ||
        typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLER == true ||
        typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_REF == true ||
        typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT == true ||
        typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_UNDEFINED == true ||
        typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_VOID == true
    ) {
        return UNIFORM_SHADER_TYPE_UNSUPPORTED;
    }

    // Start with the primitive types and narrow it down from there
    if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
        // Now branch off depending on whether it is a float (32 bit) or a double (64 bit)
        int bitWidth = typeDesc->traits.numeric.scalar.width;

        if (bitWidth == 32) {
            // This is the float case. That means we now have to check whether it is a
            // a float array, a vector, a matrix, or simply a float if none of the other apply
            
            if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
                return UNIFORM_SHADER_TYPE_VEC;
            } else if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
                return UNIFORM_SHADER_TYPE_MAT;
            } else if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) {
                return UNIFORM_SHADER_TYPE_FLOAT_ARRAY;
            } else {
                return UNIFORM_SHADER_TYPE_FLOAT;
            }
        } else if (bitWidth == 64) {
            // This is the double case. That means we now have to check whether it is a
            // a double array, a double vector, a double matrix, or simply a double if none of the other apply

            if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
                return UNIFORM_SHADER_TYPE_DVEC;
            } else if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
                return UNIFORM_SHADER_TYPE_DMAT;
            } else if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) {
                return UNIFORM_SHADER_TYPE_DOUBLE_ARRAY;
            } else {
                return UNIFORM_SHADER_TYPE_DOUBLE;
            }
        }
    } else if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
        // Now branch off depending on if it is a signed or unsigned integer
        int isSigned = typeDesc->traits.numeric.scalar.signedness;

        if (isSigned) {
            if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
                return UNIFORM_SHADER_TYPE_VEC;
            } else if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) {
                return UNIFORM_SHADER_TYPE_INT_ARRAY;
            } else {
                return UNIFORM_SHADER_TYPE_INT;
            }
        } else {
            if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
                return UNIFORM_SHADER_TYPE_UVEC;
            } else if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) {
                return UNIFORM_SHADER_TYPE_UINT_ARRAY;
            } else {
                return UNIFORM_SHADER_TYPE_UINT;
            }
        }
    } else if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL) {
        if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
            return UNIFORM_SHADER_TYPE_BVEC;
        } else if (typeDesc->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) {
            return UNIFORM_SHADER_TYPE_BOOL_ARRAY;
        } else {
            return UNIFORM_SHADER_TYPE_BOOL;
        }
    }
    
    return UNIFORM_SHADER_TYPE_UNSUPPORTED;
}

// Extracts the element info from a SPIRV block variable. Only the bottomost blocks are added as elements
// since only the primitive data types are wanted. This is meant to be entirely internal to this library
// shaderLayout - A pointer to a ShaderUniformLayout struct that contains the uniformElements array
// block - A pointer to a SpvReflectBlockVariable that contains the block information (this is important for extracting the element information)
// bindingNum - The current binding num for the current uniform buffer
static int extract_spirv_element_info(ShaderUniformLayout* shaderLayout, const SpvReflectBlockVariable* block, int bindingNum) {
    if (block->member_count == 0) {
        // This is just useful for debugging
        //SDL_Log("Block Member: name=%s type=%s offset=%u size=%u padded=%u\n",
        //    block->name, 
        //    shader_uniform_element_type_get_name(extract_spirv_type_info(block->type_description)),
        //    block->offset, block->size, block->padded_size);

        string elementName;
        string_init(&elementName);
        string_copy(&elementName, &(string){.str = (char*)block->name, .len = strlen(block->name), .__memsize = -1});

        shader_uniform_layout_append_element(
            shaderLayout,
            &(UniformElementType){
                .bindingNum = bindingNum,
                .name = elementName,
                .offset = block->offset,
                .sizeBytes = block->size,
                .sizePaddedBytes = block->padded_size,
                .type = extract_spirv_type_info(block->type_description)
            }
        );
    } else {
        for (int i = 0; i < block->member_count; i++) {
            // This works recursively so that only the bottomost elements in the chain have their elements extracted, which
            // is important because my goal is to extract the primitive types only
            extract_spirv_element_info(shaderLayout, &block->members[i], bindingNum);
        }
    }

    return 0;
}

int extract_shader_binding_info(string* spirv_file, ShaderUniformLayout* shaderLayout) {
    // Make sure these values are 0 so the counting later on is accurate
    shaderLayout->num_samplers = 0;
    shaderLayout->num_storage_buffers = 0;
    shaderLayout->num_storage_textures = 0;
    shaderLayout->num_uniform_buffers = 0;

    // Extract uniform information by creating spvreflect shader module
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(spirv_file->len, spirv_file->str, &module);

    Uint32 count = 0;
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_Log("Failed to properly initialize SPIRV-Reflect module\n");
        SDL_Quit();
        exit(-1);
    }

    // Counts the number of descriptor bindings
    result = spvReflectEnumerateDescriptorBindings(&module, &count, NULL);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_Log("Failed to retrieve spirv bindings count\n");
        SDL_Quit();
        exit(-1);
    }

    // Extract the descriptor set bindings (info on things like number of uniform buffer objects and so on)
    SpvReflectDescriptorBinding** bindings = (SpvReflectDescriptorBinding**)SDL_malloc(count * sizeof(SpvReflectDescriptorBinding*));
    if (bindings == NULL) {
        SDL_Log("Failed to allocate memory for spirv bindings.");
        SDL_Quit();
        exit(-1);
    }

    // Fetches pointers to the modules descriptor buffers, and stores them in bindings pointer
    result = spvReflectEnumerateDescriptorBindings(&module, &count, bindings);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_Log("Failed to enumerate through spirv descriptor bindings.\n");
        SDL_Quit();
        exit(-1);
    }

    // Iterates through the descriptor set to count the number of uniform buffers
    for (Uint32 i = 0; i < count; i++) {
        SpvReflectDescriptorBinding* binding = bindings[i];
        if (
            binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER 
            || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
                // This gets the uniform block, which is usually a struct of some sort since
                // SDL3 imposes a limit of 4 uniforms per shader stage
                const SpvReflectBlockVariable* block = &binding->block;

                // This will get the total size of the buffer
                shaderLayout->bufferSizes[shaderLayout->num_uniform_buffers] = block->size;
                shaderLayout->num_uniform_buffers++;

                // This is just some debug info stuff
                //SDL_Log("Uniform: name=%s set=%u binding=%u\n", binding->name, binding->set, binding->binding);
                //SDL_Log("Block: name=%s size=%u padded=%u members=%u\n", block->name, block->size, block->padded_size, block->member_count);

                for (Uint32 j = 0; j < block->member_count; j++) {
                    extract_spirv_element_info(shaderLayout, &block->members[j], binding->binding);
                }
        } else if (
            binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER
            || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                shaderLayout->num_samplers++;
        } else if (
            binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER
            || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
                shaderLayout->num_storage_buffers++;
        } else if (
            binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE
            || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
                shaderLayout->num_storage_textures++;
        }
    }

    // Frees up the memory
    SDL_free(bindings);
    spvReflectDestroyShaderModule(&module);

    return 0;
}

int shader_uniform_layout_append_element(ShaderUniformLayout* shaderLayout, UniformElementType* element) {
    if (shaderLayout->uniformElementsLen >= shaderLayout->uniformElementsCapacity) {
        // Right now I am just planning on having the uniformElements array grow by 2 items at a time since it really shouldn't
        // have that many elements
        shaderLayout->uniformElementsCapacity += 2;

        shaderLayout->uniformElements = SDL_realloc(shaderLayout->uniformElements, shaderLayout->uniformElementsCapacity * sizeof(UniformElementType));
        if (shaderLayout->uniformElements == NULL) {
            SDL_Log("Failed to allocate memory for a ShaderUniformLayout struct's uniformElementsArray.");
            SDL_Quit();
            exit(-1);
        }
    }

    shaderLayout->uniformElements[shaderLayout->uniformElementsLen] = *element;
    shaderLayout->uniformElementsLen++;

    return 0;
}

const char* shader_uniform_element_type_get_name(uint8_t elementType) {
    switch (elementType) {
    case UNIFORM_SHADER_TYPE_BOOL:
        return "bool";
        break;

    case UNIFORM_SHADER_TYPE_INT:
        return "int";
        break;

    case UNIFORM_SHADER_TYPE_UINT:
        return "uint";
        break;
    
    case UNIFORM_SHADER_TYPE_FLOAT:
        return "float";
        break;
    
    case UNIFORM_SHADER_TYPE_DOUBLE:
        return "double";
        break;
    
    case UNIFORM_SHADER_TYPE_BVEC:
        return "bvec";
        break;
    
    case UNIFORM_SHADER_TYPE_IVEC:
        return "ivec";
        break;

    case UNIFORM_SHADER_TYPE_UVEC:
        return "uvec";
        break;
    
    case UNIFORM_SHADER_TYPE_VEC:
        return "vec";
        break;

    case UNIFORM_SHADER_TYPE_DVEC:
        return "dvec";
        break;
    
    case UNIFORM_SHADER_TYPE_MAT:
        return "mat";
        break;

    case UNIFORM_SHADER_TYPE_DMAT:
        return "dmat";
        break;

    case UNIFORM_SHADER_TYPE_BOOL_ARRAY:
        return "bool[]";
        break;

    case UNIFORM_SHADER_TYPE_INT_ARRAY:
        return "int[]";
        break;

    case UNIFORM_SHADER_TYPE_UINT_ARRAY:
        return "uint[]";
        break;

    case UNIFORM_SHADER_TYPE_FLOAT_ARRAY:
        return "float[]";
        break;

    case UNIFORM_SHADER_TYPE_DOUBLE_ARRAY:
        return "double[]";
        break;

    case UNIFORM_SHADER_TYPE_BVEC_ARRAY:
        return "bvec[]";
        break;
    
    case UNIFORM_SHADER_TYPE_IVEC_ARRAY:
        return "ivec[]";
        break;

    case UNIFORM_SHADER_TYPE_UVEC_ARRAY:
        return "uvec[]";
        break;

    case UNIFORM_SHADER_TYPE_VEC_ARRAY:
        return "vec[]";
        break;

    case UNIFORM_SHADER_TYPE_DVEC_ARRAY:
        return "dvec[]";
        break;

    case UNIFORM_SHADER_TYPE_MAT_ARRAY:
        return "mat[]";
        break;

    case UNIFORM_SHADER_TYPE_DMAT_ARRAY:
        return "dmat[]";
        break;
    
    default:
        return "Unkown Type";
        break;
    }
}

void shader_uniform_elements_print(ShaderUniformLayout* shaderLayout) {
    for (int i = 0; i < shaderLayout->uniformElementsLen; i++) {
        SDL_Log(
            "name=%s type=%s size=%u padded=%u offset=%u binding=%u\n",
            shaderLayout->uniformElements[i].name.str,
            shader_uniform_element_type_get_name(shaderLayout->uniformElements[i].type),
            shaderLayout->uniformElements[i].sizeBytes,
            shaderLayout->uniformElements[i].sizePaddedBytes,
            shaderLayout->uniformElements[i].offset,
            shaderLayout->uniformElements[i].bindingNum
        );
    }
}