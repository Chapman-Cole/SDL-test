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
} Camera;

#define CAMERA_DEFAULT (Camera){.position.arr = {0.0f, 0.0f, 0.0f}, .direction.arr = {0.0f, 0.0f, 1.0f}, .up.arr = {0.0f, 1.0f, 0.0f}, .treatDirectionAsTarget = false}

typedef union CameraVector2D {
    vec2 arr;
    struct {
        float x;
        float y;
    };
} CameraVector2D;

typedef struct Camera2D {
    CameraVector2D position;
} Camera2D;

#define CAMERA2D_DEFAULT (Camera2D){.position.arr = {0.0f, 0.0f}}


#endif