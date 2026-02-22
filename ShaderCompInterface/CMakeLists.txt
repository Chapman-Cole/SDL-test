cmake_minimum_required(VERSION 4.1.0)
project(SDL-test LANGUAGES C CXX)

set(project_exe_name "shader_comp_interface")
add_executable(${project_exe_name} src/ShaderCompInterface.c ../engine/src/Strings.c)
target_include_directories(${project_exe_name} PRIVATE ../engine/include)

if(UNIX)
    target_link_libraries(${project_exe_name} PRIVATE stdc++)
endif()

include(cmake/SDL3.cmake)
include(cmake/VulkanComponents.cmake)