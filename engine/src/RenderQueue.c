#include "RenderQueue.h"

int render_queue_init(RenderQueue* queue, Camera* cam, float aspectRatio) {
    queue->capacity = 1;
    queue->len = 0;
    queue->renderItems = NULL;
    queue->cam = *cam;
    queue->isCam3D = true;
    queue->backgroundColor = (SDL_FColor){255 / 255.0f, 219 / 255.0f, 187 / 255.0f, 255 / 255.0f};
    queue->fov = cam->fov;
    queue->nearZ = cam->nearZ;
    queue->farZ = cam->farZ;
    queue->aspectRatio = aspectRatio;
    return 0;
}

int render_queue_init2D(RenderQueue* queue, Camera2D* cam2D, float aspectRatio) {
    queue->capacity = 1;
    queue->len = 0;
    queue->renderItems = NULL;
    queue->cam2D = *cam2D;
    queue->isCam3D = false;
    queue->backgroundColor = (SDL_FColor){255 / 255.0f, 219 / 255.0f, 187 / 255.0f, 255 / 255.0f};
    queue->aspectRatio = aspectRatio;
    queue->nearZ = cam2D->nearZ;
    queue->farZ = cam2D->farZ;
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
        queue->renderItems = (RenderItem*)SDL_realloc(queue->renderItems, queue->capacity * sizeof(RenderItem));
        if (queue->renderItems == NULL) {
            SDL_Log("Failed to allocate memory for render_queue.");
            SDL_Quit();
            exit(-1);
        }
    }

    RenderItemSortKey key;
    key.high = (uint64_t)object->pipeline->id << 32;
    key.high += object->material->id;

    queue->renderItems[queue->len] = (RenderItem){
        .material = object->material,
        .pipeline = object->pipeline,
        .object = object,
        .objectType = RENDER_ITEM_OBJECT,
        .sortKey = key
    };

    queue->len++;

    return 0;
}

int render_queue_add_instanced(RenderQueue* queue, InstanceRenderObject* object) {
    if (queue->len + 1 >= queue->capacity) {
        queue->capacity *= 2;
        queue->renderItems = (RenderItem*)SDL_realloc(queue->renderItems, queue->capacity * sizeof(RenderItem));
        if (queue->renderItems == NULL) {
            SDL_Log("Failed to allocate memory for render_queue.");
            SDL_Quit();
            exit(-1);
        }
    }

    RenderItemSortKey key;
    key.high = (uint64_t)object->pipeline->id << 32;
    key.high += object->material->id;

    queue->renderItems[queue->len] = (RenderItem){
        .material = object->material,
        .pipeline = object->pipeline,
        .instanceObject = object,
        .objectType = RENDER_ITEM_INSTANCED_OBJECT,
        .sortKey = key
    };

    queue->len++;

    return 0;
}

int render_queue_add_text(RenderQueue* queue, TextRenderObject* object) {
    if (queue->len + 1 >= queue->capacity) {
        queue->capacity *= 2;
        queue->renderItems = (RenderItem*)SDL_realloc(queue->renderItems, queue->capacity * sizeof(RenderItem));
        if (queue->renderItems == NULL) {
            SDL_Log("Failed to allocate memory for render_queue.");
            SDL_Quit();
            exit(-1);
        }
    }

    RenderItemSortKey key;
    key.high = (uint64_t)object->pipeline->id << 32;
    // text objects don't reference a material, so just assign the high part of the key the maximum for an unsigned 64 bit integer
    key.high += UINT64_MAX;

    queue->renderItems[queue->len] = (RenderItem){
        .material = NULL,
        .pipeline = object->pipeline,
        .textObject = object,
        .objectType = RENDER_ITEM_TEXT_OBJECT,
        .sortKey = key
    };

    queue->len++;

    return 0;
}

int render_queue_sort_basic(RenderQueue* queue) {
    // Basic insertion algorithm for now. Could be interesting to look into something more efficient like merge sort
    // later on, but for now I think insertion sort should be plenty goods

    for (uint32_t i = 1; i < queue->len; i++) {
        RenderItem curr_val = queue->renderItems[i];
        int64_t j = i - 1;

        while (j >= 0 && queue->renderItems[j].sortKey.high > curr_val.sortKey.high) {
            queue->renderItems[j + 1] = queue->renderItems[j];
            j--;
        }

        queue->renderItems[j + 1] = curr_val;
    }

    return 0;
}

// Uses radix sorting for better speed at large numbers
int render_queue_sort_radix(RenderQueue* queue) {
    if (queue->len == 0) {
        // Nothing to sort
        return 0;
    }

    RenderItem* itemSrc = queue->renderItems;
    RenderItem* itemDest = (RenderItem*)SDL_malloc(queue->len * sizeof(RenderItem));
    if (itemDest == NULL) {
        SDL_Log("Failed to allocate memory for tempItems");
        SDL_Quit();
        exit(-1);
    }

    // 8 passes of 8 bytes each for the buckets since the sort key is currently just a 64 bit integer. Adjust this 
    // as needed if the sort key is modified
    for (uint32_t i = 0; i < 8; i++) {
        uint32_t count_arr[256] = {0};
        uint32_t offset_arr[256] = {0};

        int numBitShifts = i * 8;

        for (uint32_t j = 0; j < queue->len; j++) {
            // the & 0xFF clears out all other bits aside from the first 8, which is the byte value we want
            uint8_t currByte = (itemSrc[j].sortKey.high >> numBitShifts) & 0xFF;
            count_arr[currByte]++;
        }

        uint32_t currTotal = 0;
        for (uint32_t j = 0; j < 256; j++) {
            offset_arr[j] = currTotal;
            currTotal += count_arr[j];
        }

        for (uint32_t j = 0; j < queue->len; j++) {
            uint8_t currByte = (itemSrc[j].sortKey.high >> numBitShifts) & 0xFF;

            itemDest[offset_arr[currByte]] = itemSrc[j];

            offset_arr[currByte]++;
        }

        // Swap the buffers
        RenderItem* temp = itemSrc;
        itemSrc = itemDest;
        itemDest = temp;
    }

    SDL_free(itemDest);
    return 0;
}

int render_queue_submit(RenderQueue* queue, SDL_GPUColorTargetInfo* color_target_info, uint32_t num_color_targets, int swapchain_index, bool waitToFinish) {
    render_queue_sort_radix(queue);

    // Make sure all transfer buffers and data uploads are done before rendering
    GPB_submit_all_transfer_buffers();

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());

    SDL_GPURenderPass* renderPass = NULL;

    if (color_target_info == NULL) {
        SDL_GPUTexture* swapchainTexture;
        Uint32 wdith, height;
        SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, get_SDL_main_window(), &swapchainTexture, &wdith, &height);
        if (swapchainTexture == NULL) {
            // This frame will essentially get skipped
            SDL_SubmitGPUCommandBuffer(commandBuffer);
            render_queue_destroy(queue);
            return RENDER_QUEUE_ERROR_SWAPCHAIN_FAILURE;
        }

        SDL_GPUColorTargetInfo colorTargetInfo = {0};
        colorTargetInfo.clear_color = queue->backgroundColor;
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
        colorTargetInfo.texture = swapchainTexture;

        renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);
    } else {
        if (swapchain_index >= 0 && swapchain_index < num_color_targets) {
            SDL_GPUTexture* swapchainTexture;
            Uint32 wdith, height;
            SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, get_SDL_main_window(), &swapchainTexture, &wdith, &height);
            if (swapchainTexture == NULL) {
                // This frame will essentially get skipped
                SDL_SubmitGPUCommandBuffer(commandBuffer);
                render_queue_destroy(queue);
                return RENDER_QUEUE_ERROR_SWAPCHAIN_FAILURE;
            }

            SDL_zero(color_target_info[swapchain_index]);
            color_target_info[swapchain_index].clear_color = queue->backgroundColor;
            color_target_info[swapchain_index].load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info[swapchain_index].store_op = SDL_GPU_STOREOP_STORE;
            color_target_info[swapchain_index].texture = swapchainTexture;
        }

        renderPass = SDL_BeginGPURenderPass(commandBuffer, color_target_info, num_color_targets, NULL);
    }

    // It's important to keep in mind that glm functions operate as right multiplication, meaning the ordering
    // becomes the opposite of what you would initially expect

    // This is the MVP matrix, or model, view, projection matrix. 
    // In the case of a 2D camera, there is no projection matrix
    mat4 VP;
    glm_mat4_identity(VP);
    
    if (queue->isCam3D == true) {
        mat4 tempPerspective;
        glm_perspective(glm_rad(queue->fov), queue->aspectRatio, queue->nearZ, queue->farZ, tempPerspective);

        mat4 tempView;
        if (queue->cam.treatDirectionAsTarget == true) {
            glm_lookat(queue->cam.position.arr, queue->cam.target.arr, queue->cam.up.arr, tempView);
        } else {
            glm_look(queue->cam.position.arr, queue->cam.direction.arr, queue->cam.up.arr, tempView);
        }

        glm_mat4_mul(tempPerspective, tempView, VP);
    } else {
        if (queue->cam2D.fitAspectRatio == true) {
            // Calculate the new x bounds in order to make the aspect ratio work properly
            float centerX = (queue->cam2D.horizontalBounds.x + queue->cam2D.horizontalBounds.y) / 2.0f;
            float newDistX = (queue->cam2D.verticalBounds.y - queue->cam2D.verticalBounds.x) * queue->aspectRatio * queue->cam2D.zoom;

            float centerY = (queue->cam2D.verticalBounds.x + queue->cam2D.verticalBounds.y) / 2.0f;
            float newDistY = (queue->cam2D.verticalBounds.y - queue->cam2D.verticalBounds.x) * queue->cam2D.zoom;

            float xLowBound = centerX - newDistX / 2.0f;
            float xHighBound = centerX + newDistX / 2.0f;

            float yLowBound = centerY - newDistY / 2.0f;
            float yHighBound = centerY + newDistY / 2.0f;

            glm_ortho(xLowBound, xHighBound, yLowBound, yHighBound, queue->nearZ, queue->farZ, VP);
        } else {
            // Calculations for the zoom
            float centerX = (queue->cam2D.horizontalBounds.x + queue->cam2D.horizontalBounds.y) / 2.0f;
            float newDistX = (queue->cam2D.verticalBounds.y - queue->cam2D.verticalBounds.x) * queue->cam2D.zoom;

            float centerY = (queue->cam2D.verticalBounds.x + queue->cam2D.verticalBounds.y) / 2.0f;
            float newDistY = (queue->cam2D.verticalBounds.y - queue->cam2D.verticalBounds.x) * queue->cam2D.zoom;

            float xLowBound = centerX - newDistX / 2.0f;
            float xHighBound = centerX + newDistX / 2.0f;

            float yLowBound = centerY - newDistY / 2.0f;
            float yHighBound = centerY + newDistY / 2.0f;

            glm_ortho(xLowBound, xHighBound, yLowBound, yHighBound, queue->nearZ, queue->farZ, VP);
        }

        glm_translate(VP, (vec3){-queue->cam2D.position.x, -queue->cam2D.position.y, 0.0f});
    }

    // Start the main loop for binding and draw calls
    uint32_t curr_graphics_pipeline; 
    uint32_t curr_material; 

    if (queue->len > 0) {
        curr_graphics_pipeline = queue->renderItems[0].pipeline->id;
        curr_material = queue->renderItems[0].material->id;
    }

    for (int i = 0; i < queue->len; i++) {
        // Handle switching graphics pipelines
        if (curr_graphics_pipeline != queue->renderItems[i].pipeline->id || i == 0) {
            SDL_BindGPUGraphicsPipeline(renderPass, queue->renderItems[i].pipeline->graphicsPipeline);
            curr_graphics_pipeline = queue->renderItems[i].pipeline->id;

            // Handle user frame data
            if (queue->renderItems[i].pipeline->vertexFrameData.uniform != NULL) {
                SDL_PushGPUVertexUniformData(commandBuffer, UNIFORM_VERTEX_USER_FRAME_DATA_SLOT, queue->renderItems[i].pipeline->vertexFrameData.uniform, queue->renderItems[i].pipeline->vertexFrameData.uniformSize);
            }

            if (queue->renderItems[i].pipeline->fragmentFrameData.uniform != NULL) {
                SDL_PushGPUFragmentUniformData(commandBuffer, UNIFORM_FRAGMENT_USER_FRAME_DATA_SLOT, queue->renderItems[i].pipeline->fragmentFrameData.uniform, queue->renderItems[i].pipeline->fragmentFrameData.uniformSize);
            }
        }

        // Handle switching materials
        if (queue->renderItems[i].material->uniform.uniform != NULL && (curr_material != queue->renderItems[i].material->id || i == 0)) {
            if (queue->renderItems[i].material->numTextureSamplerPairs > 0) {
                SDL_GPUTextureSamplerBinding texSamplBindings[MAX_TEXTURE_SAMPLER_PAIRS];
                for (int j = 0; j < queue->renderItems[i].material->numTextureSamplerPairs; j++) {
                    texSamplBindings[j] = (SDL_GPUTextureSamplerBinding){
                        .sampler = queue->renderItems[i].material->textureSamplerPairs[j].sampler,
                        .texture = queue->renderItems[i].material->textureSamplerPairs[j].texture
                    };
                }

                SDL_BindGPUFragmentSamplers(renderPass, 0, texSamplBindings, queue->renderItems[i].material->numTextureSamplerPairs);
            }
            SDL_PushGPUFragmentUniformData(commandBuffer, UNIFORM_FRAGMENT_MATERIAL_SLOT, queue->renderItems[i].material->uniform.uniform, queue->renderItems[i].material->uniform.uniformSize);
            curr_material = queue->renderItems[i].material->id;
        }

        // Push engine object data
        if (queue->renderItems[i].objectType == RENDER_ITEM_OBJECT) {
            mat4 objectTransform;
            glm_mat4_identity(objectTransform);
            glm_translate(objectTransform, queue->renderItems[i].object->position.arr);
            mat4 tempRotation;
            glm_quat_rotate(objectTransform, queue->renderItems[i].object->quaternion, tempRotation);
            glm_scale(tempRotation, queue->renderItems[i].object->scale.arr);

            mat4 objectData[2];
            glm_mat4_copy(VP, objectData[0]);
            glm_mat4_copy(tempRotation, objectData[1]);

            SDL_PushGPUVertexUniformData(commandBuffer, UNIFORM_VERTEX_ENGINE_OBJECT_DATA_SLOT, objectData, sizeof(objectData));

            // Push user object specific data
            if (queue->renderItems[i].object->vertexUniform.uniform != NULL) {
                SDL_PushGPUVertexUniformData(commandBuffer, UNIFORM_VERTEX_USER_OBJECT_DATA_SLOT, queue->renderItems[i].object->vertexUniform.uniform, queue->renderItems[i].object->vertexUniform.uniformSize);
            }

            if (queue->renderItems[i].object->fragmentUniform.uniform != NULL) {
                SDL_PushGPUFragmentUniformData(commandBuffer, UNIFORM_FRAGMENT_USER_OBJECT_DATA_SLOT, queue->renderItems[i].object->fragmentUniform.uniform, queue->renderItems[i].object->fragmentUniform.uniformSize);
            }

            SDL_GPUBufferBinding bufferBindings[1];
            bufferBindings[0].buffer = queue->renderItems[i].object->mesh.vertexBuffer;
            bufferBindings[0].offset = 0;

            SDL_GPUBufferBinding indexBinding;
            indexBinding.buffer = queue->renderItems[i].object->mesh.indexBuffer;
            indexBinding.offset = 0;

            SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, 1);
            SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

            SDL_DrawGPUIndexedPrimitives(renderPass, queue->renderItems[i].object->mesh.numIndices, 1, 0, 0, 0);
        } else if (queue->renderItems[i].objectType == RENDER_ITEM_INSTANCED_OBJECT) {
            mat4 objectTransform;
            glm_mat4_identity(objectTransform);
            mat4 tempRotation;
            glm_quat_rotate(objectTransform, queue->renderItems[i].instanceObject->quaternion, tempRotation);
            glm_scale(tempRotation, queue->renderItems[i].instanceObject->scale);

            mat4 objectData[2];
            glm_mat4_copy(VP, objectData[0]);
            glm_mat4_copy(tempRotation, objectData[1]);

            SDL_PushGPUVertexUniformData(commandBuffer, UNIFORM_VERTEX_ENGINE_OBJECT_DATA_SLOT, objectData, sizeof(objectData));

            // Push user object specific data
            if (queue->renderItems[i].object->vertexUniform.uniform != NULL) {
                SDL_PushGPUVertexUniformData(commandBuffer, UNIFORM_VERTEX_USER_OBJECT_DATA_SLOT, queue->renderItems[i].object->vertexUniform.uniform, queue->renderItems[i].object->vertexUniform.uniformSize);
            }

            if (queue->renderItems[i].object->fragmentUniform.uniform != NULL) {
                SDL_PushGPUFragmentUniformData(commandBuffer, UNIFORM_FRAGMENT_USER_OBJECT_DATA_SLOT, queue->renderItems[i].object->fragmentUniform.uniform, queue->renderItems[i].object->fragmentUniform.uniformSize);
            }

            SDL_GPUBufferBinding bufferBindings[MAX_INSTANCE_BUFFERS + 1];
            bufferBindings[0] = (SDL_GPUBufferBinding){.buffer = queue->renderItems[i].instanceObject->mesh.vertexBuffer, .offset = 0};
            
            for (int instanceBinding = 0; instanceBinding < queue->renderItems[i].instanceObject->numInstanceBuffers && instanceBinding < MAX_INSTANCE_BUFFERS; instanceBinding++) {
                bufferBindings[instanceBinding + 1] = (SDL_GPUBufferBinding){.buffer = queue->renderItems[i].instanceObject->instanceBuffers[instanceBinding]->gpu_buffer, .offset = 0};
            }

            SDL_GPUBufferBinding indexBinding = {.buffer = queue->renderItems[i].instanceObject->mesh.indexBuffer, .offset = 0};

            SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, queue->renderItems[i].instanceObject->numInstanceBuffers + 1);
            SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

            SDL_DrawGPUIndexedPrimitives(renderPass, queue->renderItems[i].instanceObject->mesh.numIndices, queue->renderItems[i].instanceObject->numInstances, 0, 0, 0);
        } else if (queue->renderItems[i].objectType == RENDER_ITEM_TEXT_OBJECT) {
            mat4 objectTransform;
            glm_mat4_identity(objectTransform);
            mat4 tempRotation;
            glm_quat_rotate(objectTransform, queue->renderItems[i].instanceObject->quaternion, tempRotation);
            glm_scale(tempRotation, queue->renderItems[i].instanceObject->scale);

            mat4 objectData[2];
            glm_mat4_copy(VP, objectData[0]);
            glm_mat4_copy(tempRotation, objectData[1]);

            SDL_PushGPUVertexUniformData(commandBuffer, UNIFORM_VERTEX_ENGINE_OBJECT_DATA_SLOT, objectData, sizeof(objectData));

            
        }
    }

    SDL_EndGPURenderPass(renderPass);
    
    if (waitToFinish == false) {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
    } else {
        SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commandBuffer);
        SDL_WaitForGPUFences(get_SDL_gpu_device(), true, &fence, 1);
        SDL_ReleaseGPUFence(get_SDL_gpu_device(), fence);
    }

    // Make sure the contents of the render queue are freed up to make way for the next frame
    render_queue_destroy(queue);

    return 0;
}