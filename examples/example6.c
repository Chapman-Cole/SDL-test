#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "engine.h"
#include "FontParser.h"
#include "ParsingHelpers.h"

Camera2D cam = CAMERA2D_DEFAULT;

OTFFontFile* font;

GraphicsPipeline pointsPipeline;
GraphicsPipeline linePipeline;
Material pointMat;
Material lineMat;

const uint32 glyphID = 0;

RenderObject* points;
RenderObject* lines;
uint32_t numLines = 0;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Window* window = NULL;
    window = SDL_CreateWindow("SDL-test", 960, 540, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        SDL_Log("Window Creation Failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUDevice* device = NULL;
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
    if (device == NULL) {
        SDL_Log("GPU device creation failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_ClaimWindowForGPUDevice(device, window);

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    set_SDL_gpu_device(device);
    set_SDL_main_window(window);

    GPB_init();

    graphics_pipeline_init(&pointsPipeline);
    graphics_pipeline_append_vertex_buffer_description(&pointsPipeline, SDL_GPU_VERTEXINPUTRATE_VERTEX, 3 * sizeof(float));
    graphics_pipeline_append_vertex_attribute(&pointsPipeline, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
    graphics_pipeline_append_color_target_description_default(&pointsPipeline, SDL_GetGPUSwapchainTextureFormat(get_SDL_gpu_device(), get_SDL_main_window()));
    graphics_pipeline_attach_vertex_shader(&pointsPipeline, &STRING("../../shaders/example6/vert.glsl"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_attach_fragment_shader(&pointsPipeline, &STRING("../../shaders/example6/frag.glsl"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_generate(&pointsPipeline);

    graphics_pipeline_init(&linePipeline);
    graphics_pipeline_append_vertex_buffer_description(&linePipeline, SDL_GPU_VERTEXINPUTRATE_VERTEX, 3 * sizeof(float));
    graphics_pipeline_append_vertex_attribute(&linePipeline, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
    graphics_pipeline_append_color_target_description_default(&linePipeline, SDL_GetGPUSwapchainTextureFormat(get_SDL_gpu_device(), get_SDL_main_window()));
    graphics_pipeline_attach_vertex_shader(&linePipeline, &STRING("../../shaders/example6/lines.vert"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_attach_fragment_shader(&linePipeline, &STRING("../../shaders/example6/lines.frag"), &STRING("main"), SHADER_COMPILATION_GLSL_PATH);
    graphics_pipeline_generate(&linePipeline);

    material_create(&pointMat, &pointsPipeline);
    uniform_buffer_set_float(
        &pointMat.uniform,
        material_get_handle(&pointMat, &STRING("radius")),
        0.02
    );
    uniform_buffer_set_vec(
        &pointMat.uniform,
        material_get_handle(&pointMat, &STRING("color")),
        (vec4){0.2, 0.2, 1.0, 1.0},
        4
    );

    material_create(&lineMat, &linePipeline);
    uniform_buffer_set_vec(
        &lineMat.uniform,
        material_get_handle(&lineMat, &STRING("color")),
        (vec4){1.0, 0.2, 0.2, 1.0},
        4
    );

    font = FontParser_acquire_font("../../fonts/LiberationMono-Regular.ttf");

    points = (RenderObject*)malloc(font->glyf->glyphs[glyphID].sg.numPoints * sizeof(RenderObject));

    Glyph glyph = font->glyf->glyphs[glyphID];

    int32_t cumulativeX = 0;
    int32_t cumulativeY = 0;
    for (uint32_t i = 0; i < glyph.sg.numPoints; i++) {
        render_object_create(&points[i], &pointsPipeline, &pointMat);
        meshobject_load_objfile(&points[i].mesh, STRING("../../objects/Quad.obj"));

        cumulativeX += font->glyf->glyphs[glyphID].sg.xCoordinates[i];
        cumulativeY += font->glyf->glyphs[glyphID].sg.yCoordinates[i];

        points[i].pos[0] = (float)(cumulativeX - (glyph.header.xMax + glyph.header.xMin) / 2) * (1.0 / (float)(glyph.header.xMax - glyph.header.xMin));
        points[i].pos[1] = (float)(cumulativeY - (glyph.header.yMax + glyph.header.yMin) / 2) * (1.0 / (float)(glyph.header.yMax - glyph.header.yMin));
        points[i].pos[2] = 0.0;

        points[i].scale[0] = 0.1;
        points[i].scale[1] = 0.1;
        points[i].scale[2] = 0.1;
    }

    for (uint32_t i = 0; i < glyph.header.numberOfContours; i++) {
        if (i > 0) {
            numLines += glyph.sg.endPtsOfContours[i] - glyph.sg.endPtsOfContours[i-1];
        } else {
            numLines += glyph.sg.endPtsOfContours[i] + 1;
        }
    }

    lines = (RenderObject*)malloc(numLines * sizeof(RenderObject));

    uint32_t lineCount = 0;
    uint32_t prevIndex = 0;
    for (uint32_t i = 0; i < glyph.header.numberOfContours; i++) {
        for (uint32_t j = prevIndex; j <= glyph.sg.endPtsOfContours[i]; j++) {
            render_object_create(&lines[lineCount], &linePipeline, &lineMat);
            meshobject_load_objfile(&lines[lineCount].mesh, STRING("../../objects/Quad.obj"));

            float xBegin, yBegin, xEnd, yEnd;

            if (j == glyph.sg.endPtsOfContours[i]) {
                xBegin = points[j].pos[0];
                yBegin = points[j].pos[1];
                xEnd = points[prevIndex].pos[0];
                yEnd = points[prevIndex].pos[1];
            } else {
                xBegin = points[j].pos[0];
                yBegin = points[j].pos[1];
                xEnd = points[j+1].pos[0];
                yEnd = points[j+1].pos[1];
            }

            float centerX = (xBegin + xEnd) / 2.0;
            float centerY = (yBegin + yEnd) / 2.0;

            float angle = SDL_atan2f(yEnd - yBegin, xEnd - xBegin);
            float length = sqrtf((xEnd - xBegin) * (xEnd - xBegin) + (yEnd - yBegin) * (yEnd - yBegin));

            lines[lineCount].pos[0] = centerX;
            lines[lineCount].pos[1] = centerY;

            glm_quat(lines[lineCount].quaternion, angle, 0, 0, 1.0);

            lines[lineCount].scale[0] = length / 2.0;
            lines[lineCount].scale[1] = 0.005;

            lineCount++;
        }

        prevIndex = glyph.sg.endPtsOfContours[i] + 1;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    int windowWidth, windowHeight;
    SDL_GetWindowSizeInPixels(get_SDL_main_window(), &windowWidth, &windowHeight);

    RenderQueue rQueue;
    render_queue_init2D(&rQueue, &cam, (float)windowWidth / (float)windowHeight);
    rQueue.backgroundColor = (SDL_FColor){.r = 0.5, .g = 0.5, .b = 0.5, .a = 1.0};

    for (uint32_t i = 0; i < numLines; i++) {
        render_queue_add(&rQueue, &lines[i]);
    }

    for (uint32_t i = 0; i < font->glyf->glyphs[glyphID].sg.numPoints; i++) {
        float* uniformCenter = (float*)points[i].fragmentUniform.uniform;
        glm_vec3_copy(points[i].pos, uniformCenter);
        render_queue_add(&rQueue, &points[i]);
    }

    render_queue_submit(&rQueue, NULL, 0, 0, false);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    for (uint32_t i = 0; i < font->glyf->glyphs[glyphID].sg.numPoints; i++) {
        render_object_destroy(&points[i]);
    }
    free(points);

    for (uint32_t i = 0; i < numLines; i++) {
        render_object_destroy(&lines[i]);
    }
    free(lines);

    FontParser_release_font(&font);

    material_destroy(&lineMat);
    graphics_pipeline_destroy(&linePipeline);

    material_destroy(&pointMat);
    graphics_pipeline_destroy(&pointsPipeline);

    GPB_terminate();
    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}