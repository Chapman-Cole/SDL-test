#include "Shader.h"
#include "Strings.h"
#include "SDLDevice.h"
#include "SPIRV-Reflect/spirv_reflect.h"

void set_shader_format(unsigned int shader_format) {
    ShaderFormat = shader_format;
}

SDL_GPUShader* create_vertex_shader(string path, string entry_point, bool treat_path_as_shader_source) {
    SDL_GPUShaderCreateInfo vertexInfo = {0};
    vertexInfo.entrypoint = entry_point.str;
    vertexInfo.format = ShaderFormat; // loading .spv shaders
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;  // vertex shader
    vertexInfo.num_samplers = 0;
    vertexInfo.num_storage_buffers = 0;
    vertexInfo.num_storage_textures = 0;
    vertexInfo.num_uniform_buffers = 0;

    string output_file_path, shader_base_name, shader_base_name_ext;
    string_init(&output_file_path);
    string_init(&shader_base_name);
    string_init(&shader_base_name_ext);

    // Extract the base file name without the absolute path
    int index = path.len - 1;
    int index2 = path.len - 1;
    bool dotFound = false;
    for (; index >= 0; index--) {
        if (path.str[index] == '/' || path.str[index] == '\\') {
            break;
        } else if (path.str[index] == '.' && dotFound == false) {
            index2 = index;
            dotFound = true;
        }
    }

    string_substring(&shader_base_name, &path, index+1, index2);
    string_concat(&shader_base_name_ext, &shader_base_name, &STRING(".spv"));

    string_concat(&output_file_path, &STRING("../shaders/"), &shader_base_name_ext);
    const char* args[] = {"./ShaderCompInterface/shader_comp_interface", path.str, "vertex", entry_point.str, output_file_path.str, NULL};

    SDL_Process* shader_comp_interface = NULL;
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, args);
    SDL_SetStringProperty(props, SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING, SDL_GetCurrentDirectory());
    shader_comp_interface = SDL_CreateProcessWithProperties(props);

    if (shader_comp_interface == NULL) {
        SDL_Log("Failed to create shader_comp_interface process");
        SDL_Quit();
        exit(-1);
    }

    SDL_WaitProcess(shader_comp_interface, true, NULL);
    SDL_DestroyProcess(shader_comp_interface);
    SDL_DestroyProperties(props);

    string spirv_file;
    string_init(&spirv_file);
    string_read_file(&spirv_file, &output_file_path);

    vertexInfo.code = (Uint8*)spirv_file.str;
    vertexInfo.code_size = spirv_file.len / sizeof(char);

    // Extract uniform information by creating spvreflect shader module
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(spirv_file.len * sizeof(char), spirv_file.str, &module);

    Uint32 count = 0;
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_Log("Failed to properly initialize SPIRV-Reflect module\n");
        SDL_Quit();
        exit(-1);
    }

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

    // Counts the number of descriptor bindings
    result = spvReflectEnumerateDescriptorBindings(&module, &count, bindings);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_Log("Failed to enumerate through spirv descriptor bindings.\n");
        SDL_Quit();
        exit(-1);
    }

    // Iterates through the descriptor set to count the number of uniform buffers
    for (Uint32 i = 0; i < count; i++) {
        SpvReflectDescriptorBinding* binding = bindings[i];
        if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            vertexInfo.num_uniform_buffers++;
        }
    }

    // Frees up the memory
    SDL_free(bindings);
    spvReflectDestroyShaderModule(&module);

    SDL_GPUShader* vertexShader = SDL_CreateGPUShader(get_SDL_gpu_device(), &vertexInfo);



    if (vertexShader == NULL) {
        SDL_Log("Failed to create vertex shader.\n");
        SDL_Quit();
        exit(-1);
    }

    string_free(&output_file_path);
    string_free(&shader_base_name);
    string_free(&shader_base_name_ext);
    return vertexShader;
}

SDL_GPUShader* create_fragment_shader(string path, string entry_point, bool treat_path_as_shader_source) {
    SDL_GPUShaderCreateInfo fragmentInfo = {0};
    fragmentInfo.entrypoint = entry_point.str;
    fragmentInfo.format = ShaderFormat; // loading .spv shaders
    fragmentInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;  // vertex shader
    fragmentInfo.num_samplers = 0;
    fragmentInfo.num_storage_buffers = 0;
    fragmentInfo.num_storage_textures = 0;
    fragmentInfo.num_uniform_buffers = 0;

    string output_file_path, shader_base_name, shader_base_name_ext;
    string_init(&output_file_path);
    string_init(&shader_base_name);
    string_init(&shader_base_name_ext);

    // Extract the base file name without the absolute path
    int index = path.len - 1;
    int index2 = path.len - 1;
    bool dotFound = false;
    for (; index >= 0; index--) {
        if (path.str[index] == '/' || path.str[index] == '\\') {
            break;
        } else if (path.str[index] == '.' && dotFound == false) {
            index2 = index;
            dotFound = true;
        }
    }

    string_substring(&shader_base_name, &path, index+1, index2);
    string_concat(&shader_base_name_ext, &shader_base_name, &STRING(".spv"));

    string_concat(&output_file_path, &STRING("../shaders/"), &shader_base_name_ext);
    const char* args[] = {"./ShaderCompInterface/shader_comp_interface", path.str, "fragment", entry_point.str, output_file_path.str, NULL};

    SDL_Process* shader_comp_interface = NULL;
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, args);
    SDL_SetStringProperty(props, SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING, SDL_GetCurrentDirectory());
    shader_comp_interface = SDL_CreateProcessWithProperties(props);

    if (shader_comp_interface == NULL) {
        SDL_Log("Failed to create shader_comp_interface process");
        SDL_Quit();
        exit(-1);
    }

    SDL_WaitProcess(shader_comp_interface, true, NULL);
    SDL_DestroyProcess(shader_comp_interface);
    SDL_DestroyProperties(props);

    string spirv_file;
    string_init(&spirv_file);
    string_read_file(&spirv_file, &output_file_path);

    fragmentInfo.code = (Uint8*)spirv_file.str;
    fragmentInfo.code_size = spirv_file.len / sizeof(char);

    // Extract uniform information by creating spvreflect shader module
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(spirv_file.len * sizeof(char), spirv_file.str, &module);

    Uint32 count = 0;
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_Log("Failed to properly initialize SPIRV-Reflect module\n");
        SDL_Quit();
        exit(-1);
    }

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

    // Counts the number of descriptor bindings
    result = spvReflectEnumerateDescriptorBindings(&module, &count, bindings);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_Log("Failed to enumerate through spirv descriptor bindings.\n");
        SDL_Quit();
        exit(-1);
    }

    // Iterates through the descriptor set to count the number of uniform buffers
    for (Uint32 i = 0; i < count; i++) {
        SpvReflectDescriptorBinding* binding = bindings[i];
        if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER || binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            fragmentInfo.num_uniform_buffers++;
        }
    }

    // Frees up the memory
    SDL_free(bindings);
    spvReflectDestroyShaderModule(&module);

    SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(get_SDL_gpu_device(), &fragmentInfo);



    if (fragmentShader == NULL) {
        SDL_Log("Failed to create vertex shader.\n");
        SDL_Quit();
        exit(-1);
    }

    string_free(&output_file_path);
    string_free(&shader_base_name);
    string_free(&shader_base_name_ext);
    return fragmentShader;
}
