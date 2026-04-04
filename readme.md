# SDL3 GPU API ENGINE

## Description

A side project to learn more about graphics programming using SDL3's gpu api

## Getting Started 

In order to compile you will need cmake and a c compiler. In the root directory,
create a new directory called build, then enter that directory. Once in that directory
call cmake .. and then cmake --build . in order to build the project. The executables
will be placed in bin.

## Dependencies

This project has a few dependencies: SDL3, cglm, glslang, and shaderc. These are
the ones that you have to provide on the system path. SPIRV-Reflect is another dependency,
but it is integrated directly into the project under the external directory. 

## Notes on shaders

The engine expects SPIR-V byte code or glsl code (hence the requirement for shaderc), and
imposes a few conventions in order for ease of integration with the engine. A key
limitation of SDL3's gpu api is that you can't have more than 4 uniform slots. For vertex uniforms, they 
must be of set=1 (this is an SDL3 convention), engine frame data will go in slot 0 (binding = 0), engine object data
will go in slot 1 (binding = 1), user frame data will go in slot 2 (binding = 2), and user object data will go in 
slot 3 (binding = 3). For fragment uniforms, they must be of set=3 (this is an SDL3 convention), material data will
go in slot 0 (binding = 0), user frame data will go in slot 1 (binding = 1), and user object data will go in 
slot 2 (binding = 2)