#version 460

layout (location = 0) in vec3 a_position;

layout (location = 0) out vec3 v_pos;

layout (set = 1, binding = 0) uniform Params {
    float time;
    vec3 _pad;
} params;

//layout (set = 1, binding = 1) uniform MatrixBlock1 {
//    mat4 trans;
//} matrices;

void main() {
    gl_Position = vec4(a_position.xyz, 1.0f);
    v_pos = a_position;
}