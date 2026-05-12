#version 450

layout (location = 0) in vec3 pos;

layout (location = 0) out vec4 FragColor;

layout (std140, set = 3, binding = 0) uniform MaterialData {
    vec4 color;
} MatData;

layout (std140, set = 3, binding = 1) uniform UserFrameData {
    vec4 pad;
} UFData;

layout (std140, set = 3, binding = 2) uniform UserObjectData {
    vec3 velocity;
    float pad;
} UOData;

void main() {
    float velMag = clamp(length(UOData.velocity), 0.0, 2.0) * 0.5;
    FragColor = vec4(velMag * vec3(238.0 / 255.0, 75 / 255.0, 43 / 255.0) + (1.0  -velMag) * vec3(44.0 / 255.0, 200.0 / 255.0, 255.0 / 255.0), 1.0);
}