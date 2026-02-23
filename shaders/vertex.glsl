#version 460

layout (location = 0) in vec3 a_position;

layout (location = 0) out vec3 v_pos;

layout (set = 1, binding = 0) uniform Params {
    float time;
    float offset;
    float xScaling;
    int mode;
    int shouldScaleX;
    float rippleScale;
    vec2 mouse;
} params;

//layout (set = 1, binding = 1) uniform MatrixBlock1 {
//    mat4 trans;
//} matrices;

float minus_one_power(int x) {
    float answer = 1.0f;
    for (int i = 0; i < x; i++) {
        answer *= -1.0f;
    }
    return answer;
}

#define PI 3.14159265359
#define HALF_PI 1.57079632679
#define TAU 6.28318530718

float rand(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec2 randomUnitVec(vec2 seed) {
    float random = TAU * rand(seed);
    //Have to do square root of random seed for more uniform distribution across the circle
    return sqrt(rand(seed.yx)) * vec2(cos(random), sin(random));
}

float easingFunction(float value) {
    return (3 * value * value) - (2 * value * value * value);
}

float perlinNoise(vec2 seed) {
    vec2 seedClamped = floor(seed);

    vec2 blRand = randomUnitVec(seedClamped);
    float bl = dot(blRand, seed - seedClamped);

    vec2 brRand = randomUnitVec(seedClamped + vec2(1.0, 0.0));
    float br = dot(brRand, seed - (seedClamped + vec2(1.0, 0.0)));

    vec2 tlRand = randomUnitVec(seedClamped + vec2(0.0, 1.0));
    float tl = dot(tlRand, seed - (seedClamped + vec2(0.0, 1.0)));

    vec2 trRand = randomUnitVec(seedClamped + vec2(1.0, 1.0));
    float tr = dot(trRand, seed - (seedClamped + vec2(1.0, 1.0)));

    float horizontal = bl + (easingFunction(seed.x - seedClamped.x) * (br - bl));
    float vertical = tl + (easingFunction(seed.x - seedClamped.x) * (tr - tl));

    return horizontal + easingFunction(seed.y - seedClamped.y) * (vertical - horizontal);
}

void main() {
    vec3 pos = a_position;
    float time = params.time;
    float multiplier = 1.0f;

    if (params.mode == 0) {
        if (params.shouldScaleX == 1) {
            pos.x *= params.xScaling;
        }

        gl_Position = vec4(pos, 1.0f);
        v_pos = a_position;
    } else if (params.mode == 1) {
        float x = pos.x;
        float y = pos.y;
        float r = sqrt(x*x + y*y) - params.time;
        multiplier = sin(r) + 0.5 * sin(2 * r - 1.0) + 0.1 * sin(4 * r - 5.0);

        if (params.shouldScaleX == 1) {
            pos.x *= params.xScaling;
        }

        gl_Position = vec4(multiplier * pos, 1.0f);
        v_pos = a_position;
    } else if (params.mode == 2) {
        vec3 newPos = pos;
        newPos.x += 0.011;
        newPos.y += 0.011;
        newPos.x -= pos.x;
        newPos.y -= pos.y;

        float x = newPos.x;
        float y = newPos.y;

        float zoomFactor = params.rippleScale;

        float tMulty = time * minus_one_power(int(10.0f * perlinNoise(vec2(zoomFactor * pos.x - time - params.offset, zoomFactor * pos.y + time - params.offset))));
        newPos.x = x * cos(tMulty) - y * sin(tMulty);
        newPos.y = x * sin(tMulty) + y * cos(tMulty);

        newPos.x += pos.x;
        newPos.y += pos.y;

        pos.xy = newPos.xy;

        if (params.shouldScaleX == 1) {
            pos.x *= params.xScaling;
        }

        float dropoff = 1.0f / clamp(pow(distance(5.0f * params.mouse, 5.0f * pos.xy), 2), 1.2f, 10000.0f);

        pos.xy += dropoff * (params.mouse - pos.xy);

        gl_Position = vec4(pos, 1.0f);
        v_pos = a_position;
    }
}