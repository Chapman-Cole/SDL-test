#ifndef CAMERA_H
#define CAMERA_H

#include "cglm/cglm.h"
#include <stdbool.h>

typedef union CameraVector3D {
    vec3 arr;
    struct {
        float x;
        float y;
        float z;
    };
} CameraVector3D;

typedef struct Camera {
    CameraVector3D position;
    union {
        CameraVector3D target;
        CameraVector3D direction;
    };
    CameraVector3D up;
    // Instead providing a target direction, you provide a target
    // point to look at
    bool treatDirectionAsTarget;
    float fov;
    float nearZ;
    float farZ;

    // This is not used by the engine, it is just here in case
    // users want to track the rotation of the camera easier. In practice,
    // rotating the camera means changing the direction/target vector
    CameraVector3D rotation;
} Camera;

#define CAMERA_DEFAULT {.position.arr = {0.0f, 0.0f, 0.0f}, .direction.arr = {0.0f, 0.0f, 1.0f}, .up.arr = {0.0f, 1.0f, 0.0f}, .treatDirectionAsTarget = false, .fov = 90, .nearZ = 0.1f, .farZ = 1000.0f, .rotation.arr = {0.0f, 0.0f, 0.0f}}

typedef union CameraVector2D {
    vec2 arr;
    struct {
        float x;
        float y;
    };
} CameraVector2D;

typedef struct Camera2D {
    CameraVector2D position;
    // x (index 0) is left bound, y (index 1) is right bound
    CameraVector2D horizontalBounds;
    // x (index 0) is the bottom bound, y (index 1) is the top bound
    CameraVector2D verticalBounds;

    float nearZ;
    float farZ;

    bool fitAspectRatio;

    float zoom;
} Camera2D;

#define CAMERA2D_DEFAULT {.position.arr = {0.0f, 0.0f}, .horizontalBounds.arr = {-1.0f, 1.0f}, .verticalBounds.arr = {-1.0f, 1.0f}, .nearZ = 0.1f, .farZ = 1000.0f, .fitAspectRatio = true, .zoom = 1.0f}

int camera2D_screen_to_world(Camera2D* cam, float aspectRatio, vec2 screen, vec2 out);

int camera_screen_to_world(Camera* cam, vec2 screen, vec3 out);


#endif