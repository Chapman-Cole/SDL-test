#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;

layout (location = 0) out vec4 v_color;
layout (location = 1) out vec3 v_pos;

layout (set = 1, binding = 0) uniform Params {
    float time;
    vec3 _pad;
} params;

void main() {
    gl_Position = vec4(a_position.xyz, 1.0f);
    v_color = a_color;
    v_pos = a_position;
}