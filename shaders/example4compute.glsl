#version 450

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout (std430, set = 1, binding = 0) buffer InputBuffer {
    float positions[];
};

void main() {
    uint gid = gl_GlobalInvocationID;


}