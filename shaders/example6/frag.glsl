#version 450

layout (location = 0) in vec3 v_pos;

layout (location = 0) out vec4 FragColor;

layout (std140, set = 3, binding = 0) uniform MaterialData {
    vec4 color;
    float radius;
    vec3 pad;
} MatData;

layout (std140, set = 3, binding = 1) uniform UserFrameData {
    vec4 pad;
} UFData;

layout (std140, set = 3, binding = 2) uniform UserObjectData {
    vec3 center;
    float pad;
} UOData;

void main() {
    float alphaValue = 1.0;
    if (length(v_pos - UOData.center) > MatData.radius) {
        discard;
    } else {
        FragColor = MatData.color;
    }
}