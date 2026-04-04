#include "Material.h"

int material_create(Material* mat, GraphicsPipeline* pipeline) {
    uniform_buffer_create(&mat->uniform, &pipeline->fragmentLayout, UNIFORM_FRAGMENT_MATERIAL_SLOT);
    return 0;
}

int material_destroy(Material* mat) {
    uniform_buffer_destroy(&mat->uniform);
    return 0;
}

UBElementHandle material_get_handle(Material* mat, string* name) {
    for (uint32_t i = 0; i < mat->uniform.layout->uniformElementsLen; i++) {
        if (
            // Material properties are specific to slot 0 for uniform buffers
            // in the fragment shader
            mat->uniform.layout->uniformElements[i].bindingNum == 0 &&
            string_compare(name, &mat->uniform.layout->uniformElements[i].name) == true
        ) {
            return (UBElementHandle){.index = i, .shaderType = UNIFORM_BUFFER_FRAGMENT};
        }
    }

    return INVALID_UNIFORM_BUFFER_ELEMENT_HANDLE;
}