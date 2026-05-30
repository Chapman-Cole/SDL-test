#version 450

layout (location = 0) in vec3 pos;

layout (location = 1) in vec3 vel;

layout (location = 0) out vec4 FragColor;

layout (std140, set = 3, binding = 0) uniform MaterialData {
    vec4 col;
} MatData;

layout (std140, set = 3, binding = 1) uniform UserFrameData {
    vec4 pad;
} UFData;

layout (std140, set = 3, binding = 2) uniform UserObjectData {
    vec4 pad;
} UOData;

void main() {
    float speed = 1.4 * length(vel);
    FragColor = mix(MatData.col, vec4(1.0, 0.2, 0.2, 1.0), clamp(speed, 0.0, 1.0));
}