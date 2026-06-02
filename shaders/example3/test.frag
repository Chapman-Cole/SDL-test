#version 450

layout (location = 0) in vec3 v_pos;

layout (location = 0) out vec4 FragColor;

layout (location = 1) out vec4 FragColor2;

layout (std140, set = 3, binding = 0) uniform MaterialData {
    vec4 col;
} MatData;

void main() {
    FragColor = MatData.col;
    FragColor2 = MatData.col;
}