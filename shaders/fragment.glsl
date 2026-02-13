#version 460

layout (location = 0) in vec3 pos;

layout (location = 0) out vec4 FragColor;

layout(set = 3, binding = 0) uniform Params
{
    float time;
    vec3 _pad;
} params;

layout (set = 3, binding = 1) uniform Color {
    vec4 col1;
    vec4 col2;
    vec4 col3;
} color;

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

float atan2(float y, float x) {
    /*
    This more clearly shows what math is going on here
    if (x > 0 && y > 0) {
        return atan(y / x);
    } else if (x < 0 && y > 0) {
        return PI + atan(y / x);
    } else if (x < 0 && y < 0) {
        return PI + atan(y / x);
    } else if (x > 0 && y < 0) {
        return TAU + atan(y / x);
    } else {
        return 0;
    }
    */

    if (x > 0) {
        if (y > 0) {
            return atan(y / x);
        } else if (y < 0) {
            return TAU + atan(y / x);
        }
    } else if (x < 0) {
        return PI + atan(y / x);
    } else {
        return 0;
    }
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
    vec3 position = vec3(pos.x, pos.y, pos.z);

    float addTime = 0.5 * (sin(5 * sqrt(position.x * position.x + position.y * position.y) - params.time) + 1.1);

    float perlin1 = perlinNoise(position.xy * 5 - params.time * 0.5);

    float perlin2 = 0.5 * perlinNoise(position.xy * 6 + params.time * 0.5);

    float perlin = (perlin1 + perlin2 + 1.5) / 3;

    //if (perlin >= 0.52) {
    //    FragColor = vec4(3.0 / 255.0, 64.0 / 255.0, 120.0 / 255.0, 1.0);
    //} else if (perlin >= 0.48) {
    //    FragColor = vec4(0.0, 31 / 255.0, 84.0 / 255.0, 1.0);
    //} else {
    //    FragColor = vec4(10.0 / 255.0, 17.0 / 255.0, 40 / 255.0, 1.0);
    //}

    if (perlin >= 0.52) {
        FragColor = color.col1;
    } else if (perlin >= 0.48) {
        FragColor = color.col2;
    } else {
        FragColor = color.col3;
    }

    clamp(FragColor, 0.0, 1.0);
}