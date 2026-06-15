#include "FontVectorTypes.h"
#include <stdio.h>
#include <string.h>

void DynamicArray_create(DynamicArray* dyn_arr, size_t element_size) {
    dyn_arr->len = 0;
    dyn_arr->capacity = 1;
    dyn_arr->element_size = element_size;
    dyn_arr->arr = NULL;
}

void DynamicArray_append(DynamicArray* dyn_arr, void* data) {
    if (dyn_arr->len + 1 >= dyn_arr->capacity) {
        dyn_arr->capacity *= 2;

        void* test = realloc(dyn_arr->arr, dyn_arr->capacity * dyn_arr->element_size);

        if (test == NULL) {
            printf("Failed to allocate memory for dynamic array.\nAttempted to allocate %u bytes.\nlen=%u\ncapacity=%u\nelement_size=%u\n", dyn_arr->capacity * dyn_arr->element_size, dyn_arr->len, dyn_arr->capacity, dyn_arr->element_size);
            exit(-1);
        }

        dyn_arr->arr = test;
    }

    memcpy((uint8_t*)dyn_arr->arr + dyn_arr->len * dyn_arr->element_size, data, dyn_arr->element_size);
    dyn_arr->len++;
}

void DynamicArray_insert(DynamicArray* dyn_arr, void* data, size_t index) {
    if (dyn_arr->len + 1 >= dyn_arr->capacity) {
        dyn_arr->capacity *= 2;

        void* test = realloc(dyn_arr->arr, dyn_arr->capacity * dyn_arr->element_size);

        if (test == NULL) {
            printf("Failed to allocate memory for dynamic array\n");
            exit(-1);
        }

        dyn_arr->arr = test;
    }

    memmove((uint8_t*)dyn_arr->arr + (index + 1) * dyn_arr->element_size, (uint8_t*)dyn_arr->arr + index * dyn_arr->element_size, (dyn_arr->len - index) * dyn_arr->element_size);
    memcpy((uint8_t*)dyn_arr->arr + index * dyn_arr->element_size, data, dyn_arr->element_size);
    dyn_arr->len++;
}

void DynamicArray_pop(DynamicArray* dyn_arr) {
    if (dyn_arr->len - 1 < dyn_arr->capacity / 2) {
        dyn_arr->capacity /= 2;
        if (dyn_arr->capacity == 0) {
            dyn_arr->capacity = 1;
        }

        void* test = realloc(dyn_arr->arr, dyn_arr->capacity * dyn_arr->element_size);

        if (test == NULL) {
            printf("Failed to allocate memory for dynamic array\n");
            exit(-1);
        }

        dyn_arr->arr = test;
    }

    dyn_arr->len--;
}

void DynamicArray_remove(DynamicArray* dyn_arr, size_t index) {
    if (dyn_arr->len - 1 < dyn_arr->capacity / 2) {
        dyn_arr->capacity /= 2;
        if (dyn_arr->capacity == 0) {
            dyn_arr->capacity = 1;
        }

        void* test = realloc(dyn_arr->arr, dyn_arr->capacity * dyn_arr->element_size);

        if (test == NULL) {
            printf("Failed to allocate memory for dynamic array\n");
            exit(-1);
        }

        dyn_arr->arr = test;
    }

    memmove((uint8_t*)dyn_arr->arr + index * dyn_arr->element_size, (uint8_t*)dyn_arr->arr + (index + 1) * dyn_arr->element_size, (dyn_arr->len - index - 1) * dyn_arr->element_size);
    dyn_arr->len--;
}

void DynamicArray_destroy(DynamicArray* dyn_arr) {
    free(dyn_arr->arr);
    dyn_arr->len = 0;
    dyn_arr->capacity = 1;
    dyn_arr->element_size = 0;
    dyn_arr->arr = NULL;
}