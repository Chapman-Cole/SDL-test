#ifndef RENDERQUEUE_H
#define RENDERQUEUE_H

#include <stdint.h>
#include "RenderQueue.h"
#include "GraphicsPipeline.h"
#include "Material.h"
#include "MeshObject.h"
#include "RenderObject.h"
#include "Camera.h"

// The high/top 32 bits of the high key are dedicated for the graphics pipeline id,
// the low/bottom 32 bits of the high key are dedicated for the material id.
// The high/top 32 bits of the 
typedef struct RenderItemSortKey {
    uint64_t low;
    uint64_t high;
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
} RenderQueue;

typedef enum RenderQueueErrors {
    RENDER_QUEUE_ERROR_SWAPCHAIN_FAILURE = -1
} RenderQueueErrors;

int render_queue_init(RenderQueue* queue, Camera* cam);

int render_queue_init2D(RenderQueue* queue, Camera2D* cam2D);

int render_queue_destroy(RenderQueue* queue);

int render_queue_add(RenderQueue* queue, RenderObject* object);

int render_queue_submit(RenderQueue* queue);

#endif