#ifndef COMPUTEPIPELINE_H
#define COMPUTEPIPELINE_H

#include <SDL3/SDL.h>
#include "Strings.h"


typedef struct ComputePipeline {
    SDL_GPUComputePipeline* computePipeline;
} ComputePipeline;

// Creates the compute pipeline
// pipeline - The pipeline to be initialized/created
// computeShaderSource - Contains the source code for the compute shader
// sourceType - Can be either SHADER_COMPILATION_GLSL_PATH, SHADER_COMPILATION_GLSL_STRING,
//              SHADER_COMPILATION_SPIRV_PATH, or SHADER_COMPILATION_SPIRV_STRING
// entryPoint - A string containing the name of the entry point function in the shader
int compute_pipeline_create(ComputePipeline* pipeline, string* computeShaderSource, uint8_t sourceType, string* entryPoint);

// Frees up memory used by the compute pipeline
int compute_pipeline_destroy(ComputePipeline* pipeline);


#endif