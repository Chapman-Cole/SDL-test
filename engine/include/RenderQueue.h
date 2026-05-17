#ifndef RENDERQUEUE_H
#define RENDERQUEUE_H

#include <stdint.h>
#include "RenderQueue.h"
#include "GraphicsPipeline.h"
#include "Material.h"
#include "MeshObject.h"
#include "RenderObject.h"
#include "Camera.h"
 
typedef struct RenderItemSortKey {
    uint64_t high; // top 32 bits represent the graphics pipeline id, and the bottom 32 bits represent the material id
    uint64_t low; // material id
} RenderItemSortKey;

typedef struct RenderItem {
    RenderItemSortKey sortKey;
    GraphicsPipeline* pipeline;
    RenderObject* object;
    Material* material;
} RenderItem;


// This will function like an abstract syntax tree
typedef struct RenderQueue {
    RenderItem* renderItems;
    uint32_t len;
    uint32_t capacity;
    bool isCam3D;
    union {
        Camera cam;
        Camera2D cam2D;
    };
    SDL_FColor backgroundColor;
    float fov;
    float nearZ;
    float farZ;
    float aspectRatio;
} RenderQueue;

typedef enum RenderQueueErrors {
    RENDER_QUEUE_ERROR_SWAPCHAIN_FAILURE = -1
} RenderQueueErrors;

// Aspect ratio is expected as width / height
int render_queue_init(RenderQueue* queue, Camera* cam, float aspectRatio);

// Aspect ratio is expected as width / height
int render_queue_init2D(RenderQueue* queue, Camera2D* cam2D, float aspectRatio);

int render_queue_destroy(RenderQueue* queue);

int render_queue_add(RenderQueue* queue, RenderObject* object);

int render_queue_submit(RenderQueue* queue);

#endif