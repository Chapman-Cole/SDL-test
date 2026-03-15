#version 450

layout (location = 0) in vec3 pos;

layout (location = 1) in float yScale;

layout (location = 0) out vec4 FragColor;

layout (set = 3, binding = 0) uniform Params {
    vec4 color;
} params;

void main() {
    vec3 col1 = mix(params.color.rgb, vec3(255.0f / 255.0f, 49.0f / 255.0f, 49.0f / 255.0f), sqrt(1.22 * yScale));
    FragColor = vec4(col1, 1.0);
}