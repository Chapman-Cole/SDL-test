#version 450

layout (location = 0) in vec3 a_position;

layout (location = 1) in vec3 instancePos;

layout (location = 2) in vec3 instanceVel;

layout (location = 0) out vec3 v_pos;

layout (location = 1) out vec3 vel;

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
    vec4 tempPos = EOData.model * vec4(a_position, 1.0);
    vec4 pos = EOData.VP * vec4(tempPos.xyz + instancePos, 1.0);
    
    gl_Position = pos;
    v_pos = pos.xyz;

    vel = instanceVel;
}