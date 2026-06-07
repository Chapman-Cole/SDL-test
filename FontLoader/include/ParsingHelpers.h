#ifndef PARSINGHELPERS_H
#define PARSINGHELPERS_H

#include <stdint.h>

static inline uint8_t read_8b(void* ptr) {
    uint8_t* bytes = (uint8_t*)ptr;
    return (uint8_t)bytes[0];
}

static inline uint16_t read_16b(void* ptr) {
    uint8_t* bytes = (uint8_t*)ptr;
    return ((uint32_t)(bytes[0]) << 8) | ((uint32_t)(bytes[1]));
}

static inline uint32_t read_32b(void* ptr) {
    uint8_t* bytes = (uint8_t*)ptr;
    return (((uint32_t)bytes[0]) << 24) | (((uint32_t)bytes[1]) << 16) | (((uint32_t)bytes[2]) << 8) | (((uint32_t)bytes[3]));
}

static inline uint64_t read_64b(void* ptr) {
    uint8_t* bytes = (uint8_t*)ptr;
    return (((uint64_t)bytes[0]) << 56) | (((uint64_t)bytes[1]) << 48) | (((uint64_t)bytes[2]) << 40) | (((uint64_t)bytes[3]) << 32) | (((uint64_t)bytes[4]) << 24) | (((uint64_t)bytes[5]) << 16) | (((uint64_t)bytes[6]) << 8) | (((uint64_t)bytes[7]));
}

static inline uint8_t advance_8b(void* ptr) {
    uint8_t* bytes = (*((uint8_t**)ptr));
    *((uint8_t**)ptr) = (*((uint8_t**)ptr)) + 1;
    return (uint32_t)(bytes[0]);
}

static inline uint16_t advance_16b(void* ptr) {
    uint8_t* bytes = (*((uint8_t**)ptr));
    *((uint8_t**)ptr) = (*((uint8_t**)ptr)) + 2;
    return ((uint32_t)(bytes[0]) << 8) | ((uint32_t)(bytes[1]));
}

static inline uint32_t advance_32b(void* ptr) {
    uint8_t* bytes = (*((uint8_t**)ptr));
    *((uint8_t**)ptr) = (*((uint8_t**)ptr)) + 4;
    return (((uint32_t)bytes[0]) << 24) | (((uint32_t)bytes[1]) << 16) | (((uint32_t)bytes[2]) << 8) | (((uint32_t)bytes[3]));
}

static inline uint64_t advance_64b(void* ptr) {
    uint8_t* bytes = (*((uint8_t**)ptr));
    *((uint8_t**)ptr) = (*((uint8_t**)ptr)) + 8;
    return (((uint64_t)bytes[0]) << 56) | (((uint64_t)bytes[1]) << 48) | (((uint64_t)bytes[2]) << 40) | (((uint64_t)bytes[3]) << 32) | (((uint64_t)bytes[4]) << 24) | (((uint64_t)bytes[5]) << 16) | (((uint64_t)bytes[6]) << 8) | (((uint64_t)bytes[7]));
}

#endif