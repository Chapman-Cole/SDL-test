#include "RenderQueue.h"

int render_queue_init(RenderQueue* queue) {
    queue->capacity = 1;
    queue->len = 0;
    queue->renderItems = NULL;

    return 0;
}

int render_queue_destroy(RenderQueue* queue) {
    for (uint32_t i = 0; i < queue->len; i++) {
        if (queue->renderItems[i].renderItems != NULL) {
            render_queue_destroy(&queue->renderItems[i].renderItems);
        }
    }
    SDL_free(queue->renderItems);

    return 0;
}

int render_queue_item_append(RenderQueue* queue, RenderQueue object) {
    if (queue->len + 1 >= queue->capacity) {
        queue->capacity += 5;
        queue->renderItems = SDL_realloc(queue->renderItems, queue->capacity * sizeof(RenderQueue));

        if (queue->renderItems == NULL) {
            SDL_Log("Failed to allocate memory in render_item_append");
            SDL_Quit();
            exit(-1);
        }
    }
    
    queue->renderItems[queue->len] = object;

    queue->len++;

    return 0;
}

// For internal use only
int render_queue_add_descend(RenderQueue* queue, RenderObject* object, uint8_t type) {
    // If RENDER_QUEUE_ITEM_GRAPHICS_PIPELINE ends up here, then something has gone wrong
    if (type == RENDER_QUEUE_ITEM_MATERIAL) {
        int64_t mat_offset = -1;

        for (uint32_t i = 0; i < queue->len; i++) {
            if (queue->renderItems[i].material->id == object->material->id) {
                mat_offset = i;
            }
        }

        if (mat_offset < 0) {
            mat_offset = queue->len;
            render_queue_item_append(
                queue,
                (RenderQueue){
                    .type = RENDER_QUEUE_ITEM_MATERIAL,
                    .material = object->material,
                    .renderItems = NULL,
                    .len = 0,
                    .capacity = 1
                }
            );
        }

        render_queue_add_descend(&queue->renderItems[mat_offset], object, RENDER_QUEUE_ITEM_OBJECT);
    } else {
        render_queue_item_append(
            queue,
            (RenderQueue){
                .type = RENDER_QUEUE_ITEM_OBJECT,
                .object = object,
                .renderItems = NULL,
                .len = 0,
                .capacity = 1
            }
        );
    }

    return 0;
}

int render_queue_add(RenderQueue* queue, RenderObject* object) {
    if (queue->type != RENDER_QUEUE_ITEM_GRAPHICS_PIPELINE) {
        return -1;
    }

    int64_t gp_offset = -1;

    for (uint32_t i = 0; i < queue->len; i++) {
        if (queue->renderItems[i].pipeline->id == object->pipeline->id) {
            gp_offset = i;
        }
    }

    if (gp_offset < 0) {
        gp_offset = queue->len;
        render_queue_item_append(
            queue, 
            (RenderQueue){
                .type = RENDER_QUEUE_ITEM_GRAPHICS_PIPELINE,
                .pipeline = object->pipeline,
                .renderItems = NULL,
                .len = 0,
                .capacity = 1
            }
        );
    }

    render_queue_add_descend(&queue->renderItems[gp_offset], object, RENDER_QUEUE_ITEM_MATERIAL);

    return 0;
}

// For internal use only
int render_queue_submit_descend(RenderQueue* queue, SDL_GPURenderPass* renderPass, SDL_GPUCommandBuffer* commandBuffer) {
    if (queue->type == RENDER_QUEUE_ITEM_MATERIAL) {
        for (uint32_t i = 0; i < queue->len; i++) {
            SDL_PushGPUFragmentUniformData(commandBuffer, UNIFORM_FRAGMENT_MATERIAL_SLOT, queue->renderItems[i].material->uniform.uniform, queue->renderItems[i].material->uniform.uniformSize);
            render_queue_submit_descend(&queue->renderItems[i], renderPass, commandBuffer);
        }
    } else {
        for (uint32_t i = 0; i < queue->len; i++) {
            mat4 tempMat;
            glm_mat4_identity(tempMat);
            glm_translate(tempMat, queue->renderItems[i].object->position.arr);
            SDL_PushGPUVertexUniformData(
                commandBuffer, 
                UNIFORM_VERTEX_ENGINE_OBJECT_DATA_SLOT,
                tempMat,
                sizeof(mat4)
            );
            SDL_PushGPUVertexUniformData(commandBuffer, UNIFORM_VERTEX_USER_OBJECT_DATA_SLOT, queue->renderItems[i].object->vertexUniform.uniform, queue->renderItems[i].object->vertexUniform.uniformSize);
        }
    }

    return 0;
}

int render_queue_submit(RenderQueue* queue) {
    if (queue->type != RENDER_QUEUE_ITEM_GRAPHICS_PIPELINE) {
        return 0;
    }

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());

    SDL_GPUTexture* swapchainTexture;
    Uint32 wdith, height;
    SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, get_SDL_main_window(), &swapchainTexture, &wdith, &height);
    if (swapchainTexture == NULL) {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return SDL_APP_CONTINUE;
    }

    // Remember to make this modifiable by the user of the api at some point
    SDL_GPUColorTargetInfo colorTargetInfo = {0};
    colorTargetInfo.clear_color = (SDL_FColor){255 / 255.0f, 219 / 255.0f, 187 / 255.0f, 255 / 255.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorTargetInfo.texture = swapchainTexture;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

    // Starts with the graphics pipelines, and uses a recursive function to go downwards from there
    for (uint32_t i = 0; i < queue->len; i++) {
        SDL_BindGPUGraphicsPipeline(renderPass, queue->renderItems[i].pipeline->graphicsPipeline);
        render_queue_submit_descend(&queue->renderItems[i], renderPass, commandBuffer);
    }

    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    return 0;
}