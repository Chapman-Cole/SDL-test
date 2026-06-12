#define SDL_MAIN_USE_CALLBACKS
#include "FontParser.h"
#include "ParsingHelpers.h"
#include "engine.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

typedef struct GlyphObject {
    uint32_t numPoints;
    RenderObject* points;

    uint32_t numLines;
    RenderObject* lines;
} GlyphObject;

typedef struct CompositeGlyphScaling {
    uint16_t flags;

    uint16_t arg1;
    uint16_t arg2;

    F2DOT14 scale;
    F2DOT14 xscale;
    F2DOT14 yscale;
    F2DOT14 scale01;
    F2DOT14 scale10;
} CompositeGlyphScaling;


Camera2D cam = CAMERA2D_DEFAULT;

OTFFontFile* font;

GraphicsPipeline pointsPipeline;
GraphicsPipeline linePipeline;
Material pointMat;
Material lineMat;

GlyphObject renderedGlyph;

Glyph currGlyph;

static inline int32_t f2dot14_mult(int32_t num, F2DOT14 frac) {
    return (num * frac + (1 << 13)) >> 14;
}

void glyph_object_set_points(RenderObject* points, OTFFontFile* font, uint32_t glyphID, CompositeGlyphScaling* scaling) {
    Glyph glyph = font->glyf->glyphs[glyphID];

    int16_t offsetX = 0;
    int16_t offsetY = 0;

    // 1 in F2DOT14 format is 1 << 14 since the most significant bit determines sign, and the next bit after that
    // is the whole number part of the fraction
    F2DOT14 scaleX = 1 << 14;
    F2DOT14 scaleY = 1 << 14;

    F2DOT14 scale01 = 0;
    F2DOT14 scale10 = 0;


    uint8_t flags = 0;
    if (scaling != NULL) {
        flags = scaling->flags;

        // Remember to implement pivot point calculation at some point (that is the else condition for this flag)
        if (scaling->flags & CG_ARGS_ARE_XY_VALUES) {
            offsetX = scaling->arg1;
            offsetY = scaling->arg2;
        }

        if (scaling->flags & CG_WE_HAVE_A_SCALE) {
            scaleX = scaling->scale;
            scaleY = scaling->scale;
        } else if (scaling->flags & CG_WE_HAVE_AN_X_AND_Y_SCALE) {
            scaleX = scaling->xscale;
            scaleY = scaling->yscale;
        } else if (scaling->flags & CG_WE_HAVE_A_TWO_BY_TWO) {
            scaleX = scaling->xscale;
            scaleY = scaling->yscale;
            scale01 = scaling->scale01;
            scale10 = scaling->scale10;
        }
    }

    int32_t cumulativeX = 0;
    int32_t cumulativeY = 0;
    for (uint32_t i = 0; i < glyph.sg.numPoints; i++) {
        render_object_create(&points[i], &pointsPipeline, &pointMat);
        meshobject_load_objfile(&points[i].mesh, STRING("../../objects/Quad.obj"));

        cumulativeX += font->glyf->glyphs[glyphID].sg.xCoordinates[i];
        cumulativeY += font->glyf->glyphs[glyphID].sg.yCoordinates[i];

        int32_t absoluteX = cumulativeX;
        int32_t absoluteY = cumulativeY;

        if (flags & CG_SCALED_COMPONENT_OFFSET) {
            int32_t temp_offsetx = offsetX;
            int32_t temp_offsety = offsetY;
            offsetX = f2dot14_mult(temp_offsetx, scaleX) + f2dot14_mult(temp_offsety, scale10);
            offsetY = f2dot14_mult(temp_offsetx, scale01) + f2dot14_mult(temp_offsety, scaleY);

            int32_t tempx = absoluteX;
            int32_t tempy = absoluteY;
            absoluteX = f2dot14_mult(tempx, scaleX) + f2dot14_mult(tempy, scale10);
            absoluteY = f2dot14_mult(tempx, scale01) + f2dot14_mult(tempy, scaleY);

            absoluteX += offsetX;
            absoluteY += offsetY;
        } else {
            int32_t tempx = absoluteX;
            int32_t tempy = absoluteY;
            absoluteX = f2dot14_mult(tempx, scaleX) + f2dot14_mult(tempy, scale10);
            absoluteY = f2dot14_mult(tempx, scale01) + f2dot14_mult(tempy, scaleY);

            absoluteX += offsetX;
            absoluteY += offsetY;
        }

        points[i].pos[0] = (float)absoluteX;
        points[i].pos[1] = (float)absoluteY;
        points[i].pos[2] = 0.0;

        points[i].scale[0] = 10.0;
        points[i].scale[1] = 10.0;
        points[i].scale[2] = 10.0;
    }
}

uint32_t glyph_object_get_line_count(OTFFontFile* font, uint32_t glyphID) {
    Glyph glyph = font->glyf->glyphs[glyphID];

    uint32_t numLines = 0;
    for (uint32_t i = 0; i < glyph.header.numberOfContours; i++) {
        if (i > 0) {
            numLines += glyph.sg.endPtsOfContours[i] - glyph.sg.endPtsOfContours[i - 1];
        } else {
            numLines += glyph.sg.endPtsOfContours[i] + 1;
        }
    }

    return numLines;
}

void glyph_object_set_lines(RenderObject* lines, RenderObject* points, OTFFontFile* font, uint32_t glyphID) {
    Glyph glyph = font->glyf->glyphs[glyphID];

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
                xEnd = points[j + 1].pos[0];
                yEnd = points[j + 1].pos[1];
            }

            float centerX = (xBegin + xEnd) / 2.0;
            float centerY = (yBegin + yEnd) / 2.0;

            float angle = SDL_atan2f(yEnd - yBegin, xEnd - xBegin);
            float length = sqrtf((xEnd - xBegin) * (xEnd - xBegin) + (yEnd - yBegin) * (yEnd - yBegin));

            lines[lineCount].pos[0] = centerX;
            lines[lineCount].pos[1] = centerY;

            glm_quat(lines[lineCount].quaternion, angle, 0, 0, 1.0);

            lines[lineCount].scale[0] = length / 2.0;
            lines[lineCount].scale[1] = 1.6;

            lineCount++;
        }

        prevIndex = glyph.sg.endPtsOfContours[i] + 1;
    }
}

void glyph_object_create(GlyphObject* glyphObj, OTFFontFile* font, uint32_t character) {
    uint32_t glyphID = FontParser_get_glyphID(font, character);
    Glyph glyph = font->glyf->glyphs[glyphID];

    if (glyph.header.numberOfContours == 0) {
        glyphID = 0;
        // Make sure glyph is using the new glyphID of 0 so the .notdef glyph is rendered
        glyph = font->glyf->glyphs[glyphID];
    }

    if (glyph.header.numberOfContours > 0) {
        glyphObj->numPoints = font->glyf->glyphs[glyphID].sg.numPoints;
        glyphObj->points = (RenderObject*)malloc(glyphObj->numPoints * sizeof(RenderObject));

        glyph_object_set_points(glyphObj->points, font, glyphID, NULL);

        glyphObj->numLines = glyph_object_get_line_count(font, glyphID);

        glyphObj->lines = (RenderObject*)malloc(glyphObj->numLines * sizeof(RenderObject));

        glyph_object_set_lines(glyphObj->lines, glyphObj->points, font, glyphID);
    } else {
        glyphObj->numPoints = 0;
        glyphObj->numLines = 0;
        for (uint32_t i = 0; i < glyph.cg.componentGlyphsLength; i++) {
            uint32_t compGlyphID = glyph.cg.componentGlyphs[i].glyphIndex;
            Glyph compGlyph = font->glyf->glyphs[compGlyphID];
            glyphObj->numPoints += compGlyph.sg.numPoints;
            glyphObj->numLines += glyph_object_get_line_count(font, compGlyphID);
        }

        glyphObj->points = (RenderObject*)malloc(glyphObj->numPoints * sizeof(RenderObject));
        glyphObj->lines = (RenderObject*)malloc(glyphObj->numLines * sizeof(RenderObject));

        RenderObject* points_ptr = glyphObj->points;
        RenderObject* lines_ptr = glyphObj->lines;
        for (uint32_t i = 0; i < glyph.cg.componentGlyphsLength; i++) {
            uint32_t compGlyphID = glyph.cg.componentGlyphs[i].glyphIndex;
            Glyph compGlyph = font->glyf->glyphs[compGlyphID];

            CompositeGlyphScaling scaler;
            scaler.flags = glyph.cg.componentGlyphs[i].flags;
            scaler.arg1 = glyph.cg.componentGlyphs[i].argument1;
            scaler.arg2 = glyph.cg.componentGlyphs[i].argument2;
            scaler.scale = glyph.cg.componentGlyphs[i].scale;
            scaler.xscale = glyph.cg.componentGlyphs[i].xscale;
            scaler.yscale = glyph.cg.componentGlyphs[i].yscale;
            scaler.scale01 = glyph.cg.componentGlyphs[i].scale01;
            scaler.scale10 = glyph.cg.componentGlyphs[i].scale10;

            glyph_object_set_points(points_ptr, font, compGlyphID, &scaler);

            glyph_object_set_lines(lines_ptr, points_ptr, font, compGlyphID);
            points_ptr += compGlyph.sg.numPoints;
            lines_ptr += glyph_object_get_line_count(font, compGlyphID);
        }
    }
}

void glyph_object_free(GlyphObject* glyphObj) {
    // Make sure any pending submits are handled before freeing the mesh objects
    GPB_submit_all_transfer_buffers();

    for (uint32_t i = 0; i < glyphObj->numPoints; i++) {
        render_object_destroy(&glyphObj->points[i]);
    }
    free(glyphObj->points);

    for (uint32_t i = 0; i < glyphObj->numLines; i++) {
        render_object_destroy(&glyphObj->lines[i]);
    }
    free(glyphObj->lines);
}

void glyph_object_render(GlyphObject* glyphObj, RenderQueue* rQueue) {
    for (uint32_t i = 0; i < glyphObj->numLines; i++) {
        render_queue_add(rQueue, &glyphObj->lines[i]);
    }

    for (uint32_t i = 0; i < glyphObj->numPoints; i++) {
        float* uniformCenter = (float*)glyphObj->points[i].fragmentUniform.uniform;
        glm_vec3_copy(glyphObj->points[i].pos, uniformCenter);
        render_queue_add(rQueue, &glyphObj->points[i]);
    }
}

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
        8.0);
    uniform_buffer_set_vec(
        &pointMat.uniform,
        material_get_handle(&pointMat, &STRING("color")),
        (vec4){131.0/255.0, 167.0/255.0, 211.0/255.0, 1.0},
        4);

    material_create(&lineMat, &linePipeline);
    uniform_buffer_set_vec(
        &lineMat.uniform,
        material_get_handle(&lineMat, &STRING("color")),
        (vec4){196.0/255.0, 164.0/255.0, 132.0/255.0, 1.0},
        4);

    font = FontParser_acquire_font("/usr/share/fonts/TTF/DejaVuMathTeXGyre.ttf");

    uint32_t character = L'ö';
    glyph_object_create(&renderedGlyph, font, character);
    currGlyph = font->glyf->glyphs[FontParser_get_glyphID(font, character)];

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        return SDL_APP_SUCCESS;
    } else if (event->type == SDL_EVENT_KEY_DOWN) {
        glyph_object_free(&renderedGlyph);

        glyph_object_create(&renderedGlyph, font, event->key.key);
        currGlyph = font->glyf->glyphs[FontParser_get_glyphID(font, event->key.key)];
        if (currGlyph.header.numberOfContours == 0) {
            currGlyph = font->glyf->glyphs[0];
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    int windowWidth, windowHeight;
    SDL_GetWindowSizeInPixels(get_SDL_main_window(), &windowWidth, &windowHeight);

    float centerX = ((float)currGlyph.header.xMax + (float)currGlyph.header.xMin) / 2.0;
    float centerY = ((float)currGlyph.header.yMax + (float)currGlyph.header.yMin) / 2.0;
    float distX = (float)currGlyph.header.xMax - (float)currGlyph.header.xMin;
    float distY = (float)currGlyph.header.yMax - (float)currGlyph.header.yMin;

    cam.horizontalBounds.x = centerX - (distX / 2.0) * 1.2;
    cam.horizontalBounds.y = centerX + (distX / 2.0) * 1.2;
    cam.verticalBounds.x = centerY - (distY / 2.0) * 1.2;
    cam.verticalBounds.y = centerY + (distY / 2.0) * 1.2;

    RenderQueue rQueue;
    render_queue_init2D(&rQueue, &cam, (float)windowWidth / (float)windowHeight);
    rQueue.backgroundColor = (SDL_FColor){.r = 0.3, .g = 0.4, .b = 0.3, .a = 1.0};

    glyph_object_render(&renderedGlyph, &rQueue);

    render_queue_submit(&rQueue, NULL, 0, 0, false);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    glyph_object_free(&renderedGlyph);

    FontParser_release_font(&font);

    material_destroy(&lineMat);
    graphics_pipeline_destroy(&linePipeline);

    material_destroy(&pointMat);
    graphics_pipeline_destroy(&pointsPipeline);

    GPB_terminate();
    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}