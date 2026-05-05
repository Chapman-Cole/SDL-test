#include "RenderQueue.h"

int render_queue_init(RenderQueue* queue, Camera* cam) {
    queue->capacity = 1;
    queue->len = 0;
    queue->renderItems = NULL;
    queue->cam = *cam;
    return 0;
}

int render_queue_destroy(RenderQueue* queue) {
    SDL_free(queue->renderItems);
    queue->capacity = 1;
    queue->len = 0;
    queue->renderItems = NULL;
    return 0;
}

int render_queue_add(RenderQueue* queue, RenderObject* object) {
    if (queue->len + 1 >= queue->capacity) {
        queue->capacity *= 2;
        queue->renderItems = (RenderItem*)SDL_malloc(queue->capacity * sizeof(RenderItem));
        if (queue->renderItems == NULL) {
            SDL_Log("Failed to allocate memory for render_queue.");
            SDL_Quit();
            exit(-1);
        }
    }

    RenderItemSortKey key;
    key.high = object->pipeline->id << 32;
    key.high += object->material->id;

    queue->renderItems[queue->len] = (RenderItem){
        .material = object->material,
        .pipeline = object->pipeline,
        .object = object,
        .sortKey = key
    };

    queue->len++;

    return 0;
}

int render_queue_sort(RenderQueue* queue) {
    // Basic insertion algorithm for now. Could be interesting to look into something more efficient like merge sort
    // later on, but for now I think insertion sort should be plenty goods

    for (uint32_t i = 1; i < queue->len; i++) {
        RenderItem curr_val = queue->renderItems[i];
        uint32_t j = i - 1;

        for (; j >= 0 && queue->renderItems[j].sortKey.high > curr_val.sortKey.high; j--) {
            queue->renderItems[j + 1] = queue->renderItems[j];
        }

        queue->renderItems[j + 1] = curr_val;
    }

    return 0;
}

int render_queue_submit(RenderQueue* queue) {
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());

    SDL_GPUTexture* swapchainTexture;
    Uint32 wdith, height;
    SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, get_SDL_main_window(), &swapchainTexture, &wdith, &height);
    if (swapchainTexture == NULL) {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return RENDER_QUEUE_ERROR_SWAPCHAIN_FAILURE;
    }

    SDL_GPUColorTargetInfo colorTargetInfo = {0};
    colorTargetInfo.clear_color = (SDL_FColor){255 / 255.0f, 219 / 255.0f, 187 / 255.0f, 255 / 255.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorTargetInfo.texture = swapchainTexture;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

    // Push camera data as a mat4
    mat4 viewMat;
    glm_mat4_identity(viewMat);
    
    if (queue->cam.treatDirectionAsTarget == true) {
        glm_lookat(queue->cam.position.arr, queue->cam.target.arr, queue->cam.up.arr, viewMat);
    } else {
        glm_look(queue->cam.position.arr, queue->cam.direction.arr, queue->cam.up.arr, viewMat);
    }

    SDL_PushGPUVertexUniformData(commandBuffer, UNIFORM_VERTEX_ENGINE_FRAME_DATA_SLOT, viewMat, sizeof(mat4));

    // Start the main loop for binding and draw calls
    uint32_t curr_graphics_pipeline = queue->renderItems[0].pipeline->id;
    uint32_t curr_material = queue->renderItems[0].material->id;
    for (int i = 0; i < queue->len; i++) {
        // Handle switching graphics pipelines
        if (curr_graphics_pipeline != queue->renderItems[i].pipeline->id || i == 0) {
            SDL_BindGPUGraphicsPipeline(renderPass, queue->renderItems[i].pipeline->graphicsPipeline);
            curr_graphics_pipeline = queue->renderItems[i].pipeline->id;

            // Handle user frame data
            SDL_PushGPUVertexUniformData(commandBuffer, UNIFORM_VERTEX_USER_FRAME_DATA_SLOT, queue->renderItems[i].pipeline->vertexFrameData.uniform, queue->renderItems[i].pipeline->vertexFrameData.uniformSize);
            SDL_PushGPUFragmentUniformData(commandBuffer, UNIFORM_FRAGMENT_USER_FRAME_DATA_SLOT, queue->renderItems[i].pipeline->fragmentFrameData.uniform, queue->renderItems[i].pipeline->fragmentFrameData.uniformSize);
        }

        // Handle switching materials
        if (curr_material != queue->renderItems[i].material->id || i == 0) {
            SDL_PushGPUFragmentUniformData(commandBuffer, UNIFORM_FRAGMENT_MATERIAL_SLOT, queue->renderItems[i].material->uniform.uniform, queue->renderItems[i].material->uniform.uniformSize);
            curr_material = queue->renderItems[i].material->id;
        }


    }

    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    return 0;
}