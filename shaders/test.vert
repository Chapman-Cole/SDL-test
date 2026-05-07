#version 450

layout (location = 0) in vec3 a_position;

layout (location = 0) out vec3 v_pos;

layout (std140, set = 1, binding = 0) uniform viewMatUniform {
    mat4 view;
} matUniform;

layout (std140, set = 1, binding = 1) uniform EngineObjectData {
    mat4 model;
} EOData;

layout (std140, set = 1, binding = 2) uniform UserFrameData {
    float time;
    vec2 mouse;
    float pad;
} UFData;

void main() {
    v_pos = a_position;
    vec3 pos = a_position;
    gl_Position = matUniform.view * EOData.model * vec4(pos, 1.0);
}