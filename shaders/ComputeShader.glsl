#version 450 core

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 0) buffer InputBuffer {
    float data_in[];
};

layout (std430, binding = 1) buffer OutputBuffer {
    float data_out[];
};

void main() {
    uint gid = gl_GlobalInvocationID.x;

    data_out[gid] = data_in[gid] * 5.0f;
}