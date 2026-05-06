#version 450

layout (location = 0) in vec3 a_position;

layout (location = 0) out vec3 v_pos;

layout (std140, set = 1, binding = 0) uniform viewMatUniform {
    mat4 view;
} matUniform;

void main() {
    v_pos = a_position;
    gl_Position = matUniform.view * vec4(a_position, 1.0);
}