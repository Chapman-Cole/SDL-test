#version 450

layout (location = 0) in vec3 a_position;

layout (location = 0) out vec3 v_pos;

layout (std140, set = 1, binding = 0) uniform EngineObjectData {
    mat4 MVP;
} EOData;

layout (std140, set = 1, binding = 1) uniform UserFrameData {
    float aspectRatio;
    vec3 pad;
} UFData;

layout (std140, set = 1, binding = 2) uniform UserObjectData {
    vec4 pad;
} UOData;

void main() {
    gl_Position = EOData.MVP * vec4(a_position.x, a_position.y, a_position.z, 1.0);
    v_pos = a_position;
}