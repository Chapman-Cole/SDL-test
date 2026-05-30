#version 450

#define PI 3.141592653589793238462643383279502884

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout (std430, set = 1, binding = 0) buffer PositionBuffer {
    float positions[];
};

layout (std430, set = 1, binding = 1) buffer VelocityBuffer {
    float velocities[];
};

layout (std140, set = 2, binding = 0) uniform UniformBuffer {
    uint data_size;
    float elapsed;
    vec2 mousePos;
    uint mode;
    vec3 pad;
};

vec3 rotateZ(vec3 vector, float angle) {
    float co = cos(angle);
    float si = sin(angle);

    mat3 mat = mat3(
        co, -si, 0, 
        si, co, 0,
        0, 0, 1
    );

    return mat * vector;
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    uint index = 3 * gid;

    if (index > 3 * data_size) {
        return;
    }

    vec3 pos = vec3(positions[index], positions[index + 1], positions[index + 2]);
    vec3 vel = vec3(velocities[index], velocities[index + 1], velocities[index + 2]);

    if (mode == 0) {
        float dist = distance(vec3(mousePos, 0.0), pos);
        float dropoff = 0.05f / clamp(dist * dist, 0.1, 1000.0);

        vec3 direction = normalize(vec3(mousePos, 0.0) - pos);

        float dotProductResult = abs(1.0 / length(vel) * dot(vel, direction));

        vec3 direction2 = length(vel) * rotateZ(direction, PI / 2.0);

        direction2 = mix(vel, direction2, dotProductResult * elapsed);
        vel = direction2;

        vec3 acceleration = elapsed * dropoff * direction;
        vel += acceleration;

        pos += elapsed * vel;
    } else if (mode == 1) {
        float dist = distance(vec3(mousePos, 0.0), pos);

        vec3 direction = normalize(vec3(mousePos, 0.0) - pos);

        vec3 acceleration = elapsed * clamp(1.0 / (dist * dist + 0.1), 0.1, 10.0) * direction;

        vel = clamp(vel + acceleration, -2.0, 2.0);

        pos += vel * elapsed;
    } 

    positions[index] = pos[0];
    positions[index + 1] = pos[1];
    positions[index + 2] = pos[2];

    velocities[index] = vel[0];
    velocities[index + 1] = vel[1];
    velocities[index + 2] = vel[2];
}