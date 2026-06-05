#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "engine.h"
#include "FontParser.h"
#include "ParsingHelpers.h"

Camera2D cam = CAMERA2D_DEFAULT;

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

    size_t font_file_size;
    uint8_t* font_file = (uint8_t*)SDL_LoadFile("../fonts/AdwaitaMono-Bold.ttf", &font_file_size);

    OTFTableDirectory* tbdir = FontParser_acquire_table_directory(font_file);
    
    OTFTableHead* tbhead = FontParser_acquire_table_head(font_file, tbdir);

    OTFTableHHEA* tbhhea = FontParser_acquire_table_hhea(font_file, tbdir);

    FontParser_print_table_directory(tbdir);
    FontParser_print_table_head(tbhead);
    FontParser_print_table_hhea(tbhhea);

    FontParser_release_table_hhea(&tbhhea);
    FontParser_release_table_head(&tbhead);
    FontParser_release_table_directory(&tbdir);

    SDL_free(font_file);

    SDL_ClaimWindowForGPUDevice(device, window);

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    set_SDL_gpu_device(device);
    set_SDL_main_window(window);

    GPB_init();

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

    render_queue_submit(&rQueue, NULL, 0, 0, false);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    GPB_terminate();
    destroy_SDL_gpu_device();
    destroy_SDL_main_window();
}