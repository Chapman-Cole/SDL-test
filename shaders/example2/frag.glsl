#version 450

layout (location = 0) in vec3 pos;

layout (location = 0) out vec4 FragColor;

layout (std140, set = 3, binding = 0) uniform MaterialData {
    vec4 pad1;
};

layout (std140, set = 3, binding = 1) uniform UserFrameData {
    float time;
    vec3 pad2;
};

layout (std140, set = 3, binding = 2) uniform UserObjectData {
    vec4 pad3;
};

layout (set = 2, binding = 0) uniform sampler2D simpleSampler;

void main() {
    float multiplier = 0.5 * sin(distance(pos, vec3(0,0,0)) - time) + 0.5;
    vec3 newPos = (0.3 * multiplier + 1) * pos;
    FragColor = texture(simpleSampler, 0.5 * vec2(newPos.x, -newPos.y) - 0.5);
}