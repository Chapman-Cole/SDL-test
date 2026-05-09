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
    vec4 pad;
} UOData;

void main() {
    FragColor = MatData.color;
}