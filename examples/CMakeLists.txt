cmake_minimum_required(VERSION 4.1.0)
project(examples LANGUAGES C CXX)

# This must come before including the other cmake files
set(project_name "example1")

find_library(MyEngine NAMES engine PATHS ../build/lib/static)

if(MyEngine)
    message(STATUS "engine lib found at ${MyEngine}")
else()
    message(FATAL_ERROR "Unable to find engine lib")
endif()

add_executable(${project_name} example1.c)

# It is very important the internal engine library comes first
# so that way the linker can resolve its dependencies by looking
# in the linked libraries that come after
target_include_directories(${project_name} PRIVATE ../engine/include)
target_link_libraries(${project_name} PRIVATE ${MyEngine})

include(../cmake/use_SDL3.cmake)
include(../cmake/use_VulkanComponents.cmake)
include(../cmake/use_cglm.cmake)

if(UNIX)
    target_link_libraries(${project_name} PRIVATE m stdc++)
endif()



