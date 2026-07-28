#include "../include/slot_array.hpp"

int SlotArrayInit(SlotArray* arr, size_t capacity, size_t dataElementSize) {
    arr->data        = NULL;
    arr->active      = NULL;
    arr->freeIndices = NULL;

    arr->data = malloc(dataElementSize * capacity);
    if (!arr->data)
        goto error;
    arr->dataLen         = 0;
    arr->dataElementSize = dataElementSize;
    arr->active          = (bool*) malloc(sizeof(bool) * capacity);
    if (!arr->active)
        goto error;
    arr->freeIndices = (size_t*) malloc(sizeof(size_t) * capacity);
    if (!arr->freeIndices)
        goto error;
    arr->freeLen  = 0;
    arr->capacity = capacity;

    for (size_t i = 0; i < arr->capacity; i++)
        arr->active[i] = false;
    return EXIT_SUCCESS;
error:
    SlotArrayTerminate(arr);
    return EXIT_FAILURE;
}
void SlotArrayTerminate(SlotArray* arr) {
    if (arr->data)
        free(arr->data);
    if (arr->active)
        free(arr->active);
    if (arr->freeIndices)
        free(arr->freeIndices);
    arr->data        = NULL;
    arr->active      = NULL;
    arr->freeIndices = NULL;
}
void SlotArrayClear(SlotArray* arr) {
    arr->dataLen = 0;
    arr->freeLen = 0;
    for (size_t i = 0; i < arr->capacity; i++)
        arr->active[i] = false;
}
void* SlotArrayGet(const SlotArray* arr, size_t index) {
    return (uint8_t*) arr->data + index * arr->dataElementSize;
}
void* SlotArrayInsert(SlotArray* arr) {
    ssize_t index;

    if (arr->freeLen == 0) {
        if (arr->dataLen >= arr->capacity)
            return NULL;
        index = arr->dataLen++;
    } else
        index = arr->freeIndices[--arr->freeLen];
    arr->active[index] = true;
    return SlotArrayGet(arr, index);
}
void* SlotArrayInsertGetIndex(SlotArray* arr, size_t* index) {
    if (arr->freeLen == 0) {
        if (arr->dataLen >= arr->capacity)
            return NULL;
        *index = arr->dataLen++;
    } else
        *index = arr->freeIndices[--arr->freeLen];
    arr->active[*index] = true;
    return SlotArrayGet(arr, *index);
}
void SlotArrayDel(SlotArray* arr, size_t index) {
    if (!arr->active[index])
        return;
    arr->active[index]               = false;
    arr->freeIndices[arr->freeLen++] = index;
}
bool SlotArrayFull(const SlotArray* arr) {
    return arr->dataLen >= arr->capacity && arr->freeLen == 0;
}
bool SlotArrayEmpty(const SlotArray* arr) {
    return arr->dataLen == arr->freeLen;
}
size_t SlotArrayLen(const SlotArray* arr) {
    return arr->dataLen - arr->freeLen;
}
