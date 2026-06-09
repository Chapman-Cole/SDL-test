#ifndef TEXTRENDEROBJECT_H
#define TEXTRENDEROBJECT_H

#include <SDL3/SDL.h>
#include "GraphicsPipeline.h"
#include <cglm/cglm.h>
#include "GPUBuffers.h"
#include "Strings.h"

static GraphicsPipeline TextGraphicsPipeline;

typedef struct TextRenderObject {
    GraphicsPipeline* pipeline;
    
    vec3 position;

    GPUBuffer vertexBuffer;
    GPUBuffer uvBuffer;
} TextRenderObject;

// Must be called before using any of the other text related functions
int Text_System_init(void);

// Destroys the internal GraphicsPipeline used for rendering text objects
int Text_System_terminate(void);

// Returns a pointer to the internal GraphicsPipeline for rendering text objects
GraphicsPipeline* TextRenderObject_pipeline(void);

// The pipeline can always be swapped out for a different one later
int TextRenderObject_create(TextRenderObject* obj, GraphicsPipeline* pipeline);

int TextRenderObject_destroy(TextRenderObject* obj);

#endif