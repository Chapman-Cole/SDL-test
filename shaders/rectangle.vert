#version 450

layout (location = 0) in vec3 a_position;

layout (location = 0) out vec3 v_pos;

layout (set = 1, binding = 0) uniform Params {
    float xScale;
    float yScale;
    float xOffset;
    float pad;
} params;

void main() {
    gl_Position = vec4(a_position.x * params.xScale + params.xOffset, (a_position.y + 1.0f) * params.yScale - 1.0f, a_position.z, 1.0);
    v_pos = a_position;
}