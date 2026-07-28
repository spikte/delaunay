#ifndef SLOT_ARRAY_HPP
#define SLOT_ARRAY_HPP

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef struct {
    void* data;
    size_t dataLen;
    size_t dataElementSize;

    // Reading
    bool* active;

    // Writing
    size_t* freeIndices;
    size_t freeLen;

    size_t capacity;
} SlotArray;

int SlotArrayInit(SlotArray* arr, size_t capacity, size_t elementSize);
void SlotArrayTerminate(SlotArray* arr);
void SlotArrayClear(SlotArray* arr);
void* SlotArrayGet(const SlotArray* arr, size_t index);
void* SlotArrayInsert(SlotArray* arr);
void* SlotArrayInsertGetIndex(SlotArray* arr, size_t* index);
void SlotArrayDel(SlotArray* arr, size_t index);
bool SlotArrayFull(const SlotArray* arr);
bool SlotArrayEmpty(const SlotArray* arr);
size_t SlotArrayLen(const SlotArray* arr);

#endif
