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
        int64_t j = i - 1;

        while (j >= 0 && queue->renderItems[j].sortKey.high > curr_val.sortKey.high) {
            queue->renderItems[j + 1] = queue->renderItems[j];
            j--;
        }

        queue->renderItems[j + 1] = curr_val;
    }

    return 0;
}

int render_queue_submit(RenderQueue* queue) {
    // Sort the render queue before continuing
    render_queue_sort(queue);

    // Make sure all transfer buffers and data uploads are done before rendering
    GPB_submit_all_transfer_buffers();

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(get_SDL_gpu_device());

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

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

    // It's important to keep in mind that glm functions operate as right multiplication, meaning the ordering
    // becomes the opposite of what you would initially expect

    // This is the MVP matrix, or model, view, projection matrix. 
    // In the case of a 2D camera, there is no projection matrix
    mat4 MVP;
    
    if (queue->isCam3D == true) {
        mat4 tempPerspective;
        glm_perspective(glm_rad(queue->fov), queue->aspectRatio, queue->nearZ, queue->farZ, tempPerspective);

        mat4 tempView;
        if (queue->cam.treatDirectionAsTarget == true) {
            glm_lookat(queue->cam.position.arr, queue->cam.target.arr, queue->cam.up.arr, tempView);
        } else {
            glm_look(queue->cam.position.arr, queue->cam.direction.arr, queue->cam.up.arr, tempView);
        }

        glm_mat4_mul(tempPerspective, tempView, MVP);
    } else {
        // The negative sign is because the objects in the world need to be translated in the opposite direction
        // that the camera would move
        glm_translate(MVP, (vec3){-queue->cam2D.position.x, -queue->cam2D.position.y, 0.0f});

        // Handle the aspect ratio automatically in the MVP matrix
        glm_scale(MVP, (vec3){1.0f / queue->aspectRatio, 1.0f, 1.0f});
    }

    // Start the main loop for binding and draw calls
    uint32_t curr_graphics_pipeline = queue->renderItems[0].pipeline->id;
    uint32_t curr_material = queue->renderItems[0].material->id;
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
            SDL_PushGPUFragmentUniformData(commandBuffer, UNIFORM_FRAGMENT_MATERIAL_SLOT, queue->renderItems[i].material->uniform.uniform, queue->renderItems[i].material->uniform.uniformSize);
            curr_material = queue->renderItems[i].material->id;
        }

        // Push engine object data
        mat4 objectTransform;
        glm_mat4_identity(objectTransform);
        glm_translate(objectTransform, queue->renderItems[i].object->position.arr);
        mat4 tempRotation;
        glm_quat_rotate(objectTransform, queue->renderItems[i].object->quaternion, tempRotation);
        glm_scale(tempRotation, queue->renderItems[i].object->scale.arr);

        mat4 testMult;
        glm_mat4_mul(MVP, tempRotation, testMult);

        SDL_PushGPUVertexUniformData(commandBuffer, UNIFORM_VERTEX_ENGINE_OBJECT_DATA_SLOT, testMult, sizeof(mat4));

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
    }

    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    // Make sure the contents of the render queue are freed up to make way for the next frame
    render_queue_destroy(queue);

    return 0;
}