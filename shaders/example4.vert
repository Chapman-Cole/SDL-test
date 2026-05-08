#version 450

layout (location = 0) in vec3 a_position;

layout (location = 0) out vec3 v_pos;

layout (std140, set = 1, binding = 0) uniform EngineFrameData {
    mat4 view;
} EFData;

layout (std140, set = 1, binding = 1) uniform EngineObjectData {
    mat4 model;
} EOData;

layout (std140, set = 1, binding = 2) uniform UserFrameData {
    vec2 mouse;
    float aspectRatio;
    float pad;
} UFData;

layout (std140, set = 1, binding = 3) uniform UserObjectData {
    vec4 pad;
} UOData;

void main() {
    vec4 pos = EFData.view * EOData.model * vec4(a_position.x, a_position.y, a_position.z, 1.0);

    pos.x *= UFData.aspectRatio;
    
    gl_Position = pos;
    v_pos = pos.xyz;
}