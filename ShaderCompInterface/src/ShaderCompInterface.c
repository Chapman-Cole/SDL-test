#include <SDL3/SDL.h>
#include <shaderc/shaderc.h>
#include "Strings.h"

// A basic program that will be used to compile .glsl files to spirv by spawning this small program
// as a subprocess

// Arguments expected in argv are...
// (0) executable name
// (1) Path to shader (glsl file)
// (2) Shader Type (vertex, fragment, or compute)
// (3) Entry Point (string like main)
// (4) output file path (string)

int main(int argc, char* argv[]) {
    // Verify all the necessary arguments are provided
    if (argc < 5) {
        SDL_Log("Not enough arguments provided to ShaderInterface for compilation.\n");
        return -1;
    }

    string path = (string){.str = argv[1], .len = SDL_strlen(argv[1]), .__memsize = -1};
    string shader_type = (string){.str = argv[2], .len = SDL_strlen(argv[2]), .__memsize = -1};
    string entry_point = (string){.str = argv[3], .len = SDL_strlen(argv[3]), .__memsize = -1};
    string output_file_path = (string){.str = argv[4], .len = SDL_strlen(argv[4]), .__memsize = -1};

    string file;
    string_init(&file);
    string_read_file(&file, &path);

    // Create the shader compiler
    shaderc_compiler_t compiler = shaderc_compiler_initialize();

    // Compile the glsl to SPIR-V
    shaderc_compilation_result_t result;
    if (string_compare(&shader_type, &STRING("vertex")) == true) {
        result = shaderc_compile_into_spv(
            compiler, file.str, file.len, shaderc_glsl_vertex_shader, path.str, entry_point.str, NULL
        );
    } else if (string_compare(&shader_type, &STRING("fragment")) == true) {
        result = shaderc_compile_into_spv(
            compiler, file.str, file.len, shaderc_glsl_fragment_shader, path.str, entry_point.str, NULL
        );
    } else {
        SDL_Log("Unrecognized shader type.\n");
        return -1;
    }

    // Handle potential errors
    if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
        SDL_Log("Error: %s\n", shaderc_result_get_error_message(result));
    } else {
        const uint32_t* spirv_bin = (const uint32_t*)shaderc_result_get_bytes(result);
        size_t spirv_bin_size = shaderc_result_get_length(result);

        if (!SDL_SaveFile(output_file_path.str, spirv_bin, spirv_bin_size)) {
            SDL_Log("Failed to save generated spirv code to a file.\n");
        }
    }
    
    // Cleanup
    shaderc_result_release(result);
    shaderc_compiler_release(compiler);
    string_free(&file);
    return 0;
}