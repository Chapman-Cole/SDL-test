#define SDL_MAIN_USE_CALLBACKS
#include "FontCurves.h"
#include "FontParser.h"
#include "ParsingHelpers.h"
#include "engine.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

Camera2D cam = CAMERA2D_DEFAULT;

OTFFontFile* font;

GraphicsPipeline pointsPipeline;
GraphicsPipeline linePipeline;
Material pointMat;
Material lineMat;

Glyph currGlyph;

FontCharacter renderedChar;

RenderObject basePoint;
RenderObject baseLine;

float intersectionLineY = 0.0;
RenderObject intersectionLine;

void draw_straight_line(fvec2 p1, fvec2 p2, RenderQueue* rQueue) {
    float centerX = (p1.x + p2.x) / 2.0;
    float centerY = (p1.y + p2.y) / 2.0;

    float angle = SDL_atan2f(p2.y - p1.y, p2.x - p1.x);
    float length = sqrtf((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));

    baseLine.pos[0] = centerX;
    baseLine.pos[1] = centerY;

    glm_quat(baseLine.quaternion, angle, 0, 0, 1.0);

    baseLine.scale[0] = length / 2.0;
    baseLine.scale[1] = 3.2;

    render_queue_add(rQueue, &baseLine);
}

void draw_bezier_curve(fvec2 p1, fvec2 control, fvec2 p2, uint32_t granularity, RenderQueue* rQueue) {
    float time_advance = 1.0 / (float)granularity;
    
    float t = 0.0;
    fvec2 prevPoint = p1;

    for (uint32_t i = 0; i < granularity; i++) {
        t += time_advance;

        fvec2 Q1 = {control.x * t + (1 - t) * p1.x, control.y * t + (1 - t) * p1.y};
        fvec2 Q2 = {p2.x * t + (1 - t) * control.x, p2.y * t + (1 - t) * control.y};
        fvec2 res = {Q2.x * t + (1 - t) * Q1.x, Q2.y * t + (1 - t) * Q1.y};

        draw_straight_line(prevPoint, res, rQueue);

        prevPoint = res;
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
        13.0);
    uniform_buffer_set_vec(
        &pointMat.uniform,
        material_get_handle(&pointMat, &STRING("color")),
        (vec4){131.0 / 255.0, 167.0 / 255.0, 211.0 / 255.0, 1.0},
        4);

    material_create(&lineMat, &linePipeline);
    uniform_buffer_set_vec(
        &lineMat.uniform,
        material_get_handle(&lineMat, &STRING("color")),
        (vec4){196.0 / 255.0, 164.0 / 255.0, 132.0 / 255.0, 1.0},
        4);

    font = FontParser_acquire_font("/usr/share/fonts/TTF/DejaVuSans.ttf");

    uint32_t character = L'Ω';
    currGlyph = font->glyf->glyphs[FontParser_get_glyphID(font, character)];
    FontCharacter_create(&renderedChar, font, character);

    render_object_create(&basePoint, &pointsPipeline, &pointMat);
    meshobject_load_objfile(&basePoint.mesh, STRING("../../objects/Quad.obj"));

    render_object_create(&baseLine, &linePipeline, &lineMat);
    meshobject_load_objfile(&baseLine.mesh, STRING("../../objects/Quad.obj"));

    render_object_create(&intersectionLine, &linePipeline, &lineMat);
    meshobject_load_objfile(&intersectionLine.mesh, STRING("../../objects/Quad.obj"));

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        return SDL_APP_SUCCESS;
    } else if (event->type == SDL_EVENT_KEY_DOWN) {
        currGlyph = font->glyf->glyphs[FontParser_get_glyphID(font, event->key.key)];
        FontCharacter_destroy(&renderedChar);
        FontCharacter_create(&renderedChar, font, event->key.key);
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            float mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);

            int windowWidth, windowHeight;
            SDL_GetWindowSize(get_SDL_main_window(), &windowWidth, &windowHeight);
    
            intersectionLineY = 2.0 * (0.5 - mouseY / (float)(windowHeight));
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

    for (uint32_t i = 0; i < renderedChar.contours.len; i++) {
        FontContour currContour = ((FontContour*)renderedChar.contours.arr)[i];
        for (uint32_t j = 0; j < currContour.lines.len; j++) {
            StraightLine currLine = ((StraightLine*)currContour.lines.arr)[j];
            draw_straight_line(currLine.p1, currLine.p2, &rQueue);
        }

        for (uint32_t j = 0; j < currContour.curves.len; j++) {
            BezierCurve currCurve = ((BezierCurve*)currContour.curves.arr)[j];
            draw_bezier_curve(currCurve.p1, currCurve.control, currCurve.p2, 5, &rQueue);
        }
    }

    vec2 tempTranslation;
    camera2D_screen_to_world(&cam, (float)windowWidth / (float)windowHeight, (vec2){0.0, intersectionLineY}, tempTranslation);

    intersectionLine.pos[0] = centerX;
    intersectionLine.pos[1] = tempTranslation[1];

    // Have to remember to account for the scaling according to aspect ratio
    intersectionLine.scale[0] = distX * 1.2 * (float)windowHeight * (float)windowWidth;
    intersectionLine.scale[1] = 4.0;
    render_queue_add(&rQueue, &intersectionLine);

    DynamicArray intersections;
    FontCharacter_calc_intersections(&renderedChar, intersectionLine.pos[1], &intersections);

    for (uint32_t i = 0; i < intersections.len; i++) {
        fvec2 currPoint = ((fvec2*)intersections.arr)[i];

        basePoint.pos[0] = currPoint.x;
        basePoint.pos[1] = currPoint.y;

        basePoint.scale[0] = 13.0;
        basePoint.scale[1] = 13.0;

        ((float*)basePoint.fragmentUniform.uniform)[0] = basePoint.pos[0];
        ((float*)basePoint.fragmentUniform.uniform)[1] = basePoint.pos[1];
        ((float*)basePoint.fragmentUniform.uniform)[2] = 0.0;

        render_queue_add(&rQueue, &basePoint);
    }

    DynamicArray_destroy(&intersections);

    render_queue_submit(&rQueue, NULL, 0, 0, false);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    render_object_destroy(&intersectionLine);
    render_object_destroy(&basePoint);
    render_object_destroy(&baseLine);
    FontCharacter_destroy(&renderedChar);

    FontParser_release_font(&font);

    material_destroy(&lineMat);
    graphics_pipeline_destroy(&linePipeline);

    material_destroy(&pointMat);
    graphics_pipeline_destroy(&pointsPipeline);

    GPB_terminate();
    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}