#version 450

layout (location = 0) in vec3 v_pos;

layout (location = 0) out vec4 FragColor;

void main() {
    FragColor = clamp(vec4(v_pos, 1.0), 0.0, 1.0);
}