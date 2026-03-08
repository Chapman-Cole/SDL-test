#version 450

layout (location = 0) in vec3 pos;

layout (location = 0) out vec4 FragColor;

layout (set = 3, binding = 0) uniform Params {
    vec4 color;
} params;

void main() {
    FragColor = vec4(params.color.rgb, 1.0);
}