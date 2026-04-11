#ifndef RENDERQUEUE_H
#define RENDERQUEUE_H

#include <stdint.h>
#include "RenderQueue.h"
#include "GraphicsPipeline.h"
#include "Material.h"
#include "MeshObject.h"
#include "RenderObject.h"

typedef enum RenderQueueItemType {
    RENDER_QUEUE_ITEM_GRAPHICS_PIPELINE,
    RENDER_QUEUE_ITEM_MATERIAL,
    RENDER_QUEUE_ITEM_OBJECT
} RenderQueueItemType;

// This will function like an abstract syntax tree
typedef struct RenderQueue {
    RenderQueue* renderItems;
    uint32_t len;
    uint32_t capacity;
    uint8_t type;

    union {
        GraphicsPipeline* pipeline;
        Material* material;
        RenderObject* object;
    };
} RenderQueue;

int render_queue_init(RenderQueue* queue);

int render_queue_destroy(RenderQueue* queue);

int render_queue_add(RenderQueue* queue, RenderObject* object);

int render_queue_submit(RenderQueue* queue);

#endif