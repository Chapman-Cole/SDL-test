#version 450

layout (location = 0) in vec3 a_position;

layout (location = 0) out vec3 fragPos;

layout (std140, set = 1, binding = 0) uniform EngineObjectData {
    mat4 VP;
    mat4 model;
};

layout (std140, set = 1, binding = 1) uniform UserFrameData {
    vec4 pad1;
};

layout (std140, set = 1, binding = 2) uniform UserObjectData {
    vec4 pad2;
};

void main() {
    gl_Position = VP * model * vec4(a_position, 1.0);
    fragPos = a_position;
}