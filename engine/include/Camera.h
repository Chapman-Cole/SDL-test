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

int camera_init(Camera* cam, vec3 position, vec3 direction, vec3 up, bool treatDirectionAsTarget) {
    cam->position.x = position[0]; cam->position.y = position[1]; cam->position.z = position[2];
    cam->direction.x = direction[0]; cam->direction.y = direction[1]; cam->direction.z = direction[2];
    cam->up.x = up[0]; cam->up.y = up[1]; cam->up.z = up[2];
    cam->treatDirectionAsTarget = treatDirectionAsTarget;
    return 0;
}


#endif