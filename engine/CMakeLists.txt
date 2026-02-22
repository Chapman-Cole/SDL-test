cmake_minimum_required(VERSION 4.1.0)
project(engine LANGUAGES C CXX)

# This must come before including the other cmake files
set(project_exe_name "main")
add_executable(${project_exe_name} src/main.c src/GPUBuffers.c src/Strings.c src/Shader.c src/SDLDevice.c src/GraphicsPipeline.c src/MeshObject.c src/Window.c ../external/SPIRV-Reflect/spirv_reflect.c)

include(cmake/SDL3.cmake)
include(cmake/cglm.cmake)

if(UNIX)
    target_link_libraries(main PRIVATE
        m
    )
endif()

target_include_directories(main PRIVATE "include" ../external ../external/SPIRV-Headers)


# Enable for address sanitization
#set_target_properties(main PROPERTIES LINKER_LANGUAGE CXX)
#target_compile_options(main PRIVATE -fsanitize=address)
#target_link_options(main PRIVATE -fsanitize=address)