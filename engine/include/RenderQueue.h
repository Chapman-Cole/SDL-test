#ifndef RENDERQUEUE_H
#define RENDERQUEUE_H

#include <stdint.h>
#include "RenderQueue.h"
#include "GraphicsPipeline.h"
#include "Material.h"
#include "MeshObject.h"
#include "RenderObject.h"
#include "Camera.h"
#include "InstanceRenderObject.h"
#include "TextRenderObject.h"
 
typedef struct RenderItemSortKey {
    uint64_t high; // top 32 bits represent the graphics pipeline id, and the bottom 32 bits represent the material id
    uint64_t low; // material id
} RenderItemSortKey;

typedef enum {
    RENDER_ITEM_OBJECT,
    RENDER_ITEM_INSTANCED_OBJECT,
    RENDER_ITEM_TEXT_OBJECT
} RenderItemObjectType;

typedef struct RenderItem {
    RenderItemSortKey sortKey;
    GraphicsPipeline* pipeline;
    union {
        struct {    
            RenderObject* object;
            // This allows for multiple copies of the same object to be rendered
            vec3 objPos;
            vec3 objScale;
            versor objQuaternion;
            uint8_t vertexUniform[128];
            uint8_t fragmentUniform[128];
        };
        InstanceRenderObject* instanceObject;
        TextRenderObject* textObject;
    };
    // References options from the enum RenderItemObjectType
    uint8_t objectType;
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

int render_queue_add_instanced(RenderQueue* queue, InstanceRenderObject* object);

int render_queue_add_text(RenderQueue* queue, TextRenderObject* object);

// If either texture or color_target_info are NULL, then the swapchain will be rendered to directly
// swapchain_index - The index in the color_target_info array that the swapchain color_target_info will be inserted into. If you
// don't want to render to the swapchain, then this can be negative
int render_queue_submit(RenderQueue* queue, SDL_GPUColorTargetInfo* color_target_info, uint32_t num_color_targets, int swapchain_index, bool waitToFinish);

int render_queue_sort_basic(RenderQueue* queue);

int render_queue_sort_radix(RenderQueue* queue);

#endif