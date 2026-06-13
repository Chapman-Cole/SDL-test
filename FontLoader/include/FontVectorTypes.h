#ifndef FONTVECTORTYPES_H
#define FONTVECTORTYPES_H

#include <stdint.h>
#include <stdlib.h>

typedef struct intvec2 { int32_t x; int32_t y; } intvec2;

typedef struct fvec2 { float x; float y; } fvec2;
typedef struct fvec3 { float x; float y; float z; } fvec3;

typedef struct DynamicArray {
    uint32_t len;
    uint32_t capacity;
    uint32_t element_size;
    void* arr;
} DynamicArray;

// element_size - The size of the elements in bytes
void DynamicArray_create(DynamicArray* dyn_arr, uint32_t element_size);

void DynamicArray_append(DynamicArray* dyn_arr, void* data);

void DynamicArray_insert(DynamicArray* dyn_arr, void* data, uint32_t index);

void DynamicArray_pop(DynamicArray* dyn_arr);

void DynamicArray_remove(DynamicArray* dyn_arr, uint32_t index);

void DynamicArray_destroy(DynamicArray* dyn_arr);

#endif