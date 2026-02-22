cmake_minimum_required(VERSION 4.1.0)
project(SDL-test LANGUAGES C CXX)

# Place all executables in a bin folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>")

add_subdirectory(engine)
add_subdirectory(ShaderCompInterface)