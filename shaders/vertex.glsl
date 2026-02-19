#version 460

layout (location = 0) in vec3 a_position;

layout (location = 0) out vec3 v_pos;

layout (set = 1, binding = 0) uniform Params {
    float time;
    float offset;
    vec2 _pad;
} params;

//layout (set = 1, binding = 1) uniform MatrixBlock1 {
//    mat4 trans;
//} matrices;

void main() {
    float x = a_position.x;
    float y = a_position.y;
    float r = sqrt(x*x + y*y) - params.time;
    float multiplier = sin(r) + 0.5 * sin(2 * r - 1.0) + 0.1 * sin(4 * r - 5.0);

    if (params.offset < 0.5) {
        multiplier = 1;
    }

    gl_Position = vec4(multiplier * a_position.xyz, 1.0f);
    v_pos = a_position;
}