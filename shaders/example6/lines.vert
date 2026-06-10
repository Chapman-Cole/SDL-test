#version 450

layout (location = 0) in vec3 a_position;

layout (location = 0) out vec3 v_pos;

layout (std140, set = 1, binding = 0) uniform EngineObjectData {
    mat4 VP;
    mat4 model;
} EOData;

layout (std140, set = 1, binding = 1) uniform UserFrameData {
    vec4 pad;
} UFData;

layout (std140, set = 1, binding = 2) uniform UserObjectData {
    vec4 pad;
} UOData;

void main() {
    v_pos = a_position;
    vec3 pos = a_position;
    gl_Position = EOData.VP * EOData.model * vec4(pos, 1.0);
}