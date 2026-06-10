#include "Camera.h"

int camera2D_screen_to_world(Camera2D* cam, float aspectRatio, vec2 screen, vec2 out) {
    mat4 VP;

    if (cam->fitAspectRatio == true) {
            // Calculate the new x bounds in order to make the aspect ratio work properly
            float centerX = (cam->horizontalBounds.x + cam->horizontalBounds.y) / 2.0f;
            float newDistX = (cam->verticalBounds.y - cam->verticalBounds.x) * aspectRatio * cam->zoom;

            float centerY = (cam->verticalBounds.x + cam->verticalBounds.y) / 2.0f;
            float newDistY = (cam->verticalBounds.y - cam->verticalBounds.x) * cam->zoom;

            float xLowBound = centerX - newDistX / 2.0f;
            float xHighBound = centerX + newDistX / 2.0f;

            float yLowBound = centerY - newDistY / 2.0f;
            float yHighBound = centerY + newDistY / 2.0f;

            glm_ortho(xLowBound, xHighBound, yLowBound, yHighBound, cam->nearZ, cam->farZ, VP);
        } else {
            // Calculations for the zoom
            float centerX = (cam->horizontalBounds.x + cam->horizontalBounds.y) / 2.0f;
            float newDistX = (cam->verticalBounds.y - cam->verticalBounds.x) * cam->zoom;

            float centerY = (cam->verticalBounds.x + cam->verticalBounds.y) / 2.0f;
            float newDistY = (cam->verticalBounds.y - cam->verticalBounds.x) * cam->zoom;

            float xLowBound = centerX - newDistX / 2.0f;
            float xHighBound = centerX + newDistX / 2.0f;

            float yLowBound = centerY - newDistY / 2.0f;
            float yHighBound = centerY + newDistY / 2.0f;

            glm_ortho(xLowBound, xHighBound, yLowBound, yHighBound, cam->nearZ, cam->farZ, VP);
        }

        glm_translate(VP, (vec3){-cam->position.x, -cam->position.y, 0.0f});

        mat4 inverseProjection;
        glm_mat4_inv(VP, inverseProjection);

        vec4 temp;
        temp[0] = screen[0];
        temp[1] = screen[1];
        temp[2] = 0.0f;
        temp[3] = 1.0f; // Important for this to be one for the transformation to work properly

        vec4 temp2;
        glm_mat4_mulv(inverseProjection, temp, temp2);

        out[0] = temp2[0];
        out[1] = temp2[1];
        
        return 0;
}

int camera2D_world_to_screen(Camera2D* cam, float aspectRatio, vec2 world, vec2 out) {
    mat4 VP;

    if (cam->fitAspectRatio == true) {
            // Calculate the new x bounds in order to make the aspect ratio work properly
            float centerX = (cam->horizontalBounds.x + cam->horizontalBounds.y) / 2.0f;
            float newDistX = (cam->verticalBounds.y - cam->verticalBounds.x) * aspectRatio * cam->zoom;

            float centerY = (cam->verticalBounds.x + cam->verticalBounds.y) / 2.0f;
            float newDistY = (cam->verticalBounds.y - cam->verticalBounds.x) * cam->zoom;

            float xLowBound = centerX - newDistX / 2.0f;
            float xHighBound = centerX + newDistX / 2.0f;

            float yLowBound = centerY - newDistY / 2.0f;
            float yHighBound = centerY + newDistY / 2.0f;

            glm_ortho(xLowBound, xHighBound, yLowBound, yHighBound, cam->nearZ, cam->farZ, VP);
        } else {
            // Calculations for the zoom
            float centerX = (cam->horizontalBounds.x + cam->horizontalBounds.y) / 2.0f;
            float newDistX = (cam->verticalBounds.y - cam->verticalBounds.x) * cam->zoom;

            float centerY = (cam->verticalBounds.x + cam->verticalBounds.y) / 2.0f;
            float newDistY = (cam->verticalBounds.y - cam->verticalBounds.x) * cam->zoom;

            float xLowBound = centerX - newDistX / 2.0f;
            float xHighBound = centerX + newDistX / 2.0f;

            float yLowBound = centerY - newDistY / 2.0f;
            float yHighBound = centerY + newDistY / 2.0f;

            glm_ortho(xLowBound, xHighBound, yLowBound, yHighBound, cam->nearZ, cam->farZ, VP);
        }

        glm_translate(VP, (vec3){-cam->position.x, -cam->position.y, 0.0f});

        vec4 temp;
        glm_mat4_mulv(VP, (vec4){world[0], world[1], 0.0, 0.0}, temp);

        out[0] = temp[0];
        out[1] = temp[1];

        return 0;
}

// Define this later
int camera_screen_to_world(Camera* cam, vec2 screen, vec3 out) {
    return 0;
}