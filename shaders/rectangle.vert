#version 450

#define TAU 6.2831853071795864769252867665590

layout (location = 0) in vec3 a_position;

layout (location = 0) out vec3 v_pos;

layout (location = 1) out float yScale;

layout (set = 1, binding = 0) uniform Params {
    float xScale;
    float yScale;
    float xOffset;
    int id;
    int numBins;
    float aspectRatio;
    vec2 pad;
} params;

vec3 roateTheta(vec3 vector, float theta) {
    vec3 newVec;
    newVec.x = vector.x * cos(theta) - vector.y * sin(theta);
    newVec.y = vector.x * sin(theta) + vector.y * cos(theta);
    newVec.z = vector.z;
    return newVec;
}



void main() {
    gl_Position = vec4(a_position.x * params.xScale * 0.80f + params.xOffset, (a_position.y + 1.0f) * params.yScale - 1.0f, a_position.z, 1.0);

    //vec3 pos = a_position;
    //pos.y = 0.5 * (pos.y + 1);
    //pos.y *= params.yScale;
    //pos.x *= params.xScale;
    //pos.y += 0.4;
    //vec3 transformedPos = roateTheta(pos, float(params.id) / float(params.numBins) * TAU);
    //gl_Position = vec4(transformedPos.x * params.aspectRatio, transformedPos.y, transformedPos.z, 1.0);

    v_pos = a_position;
    yScale = params.yScale;
}