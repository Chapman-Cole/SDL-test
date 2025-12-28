cmake_minimum_required(VERSION 3.16)
project(SDL-test LANGUAGES C)

find_package(SDL3 REQUIRED)

add_executable(main src/main.c)
target_link_libraries(main PRIVATE SDL3::SDL3)