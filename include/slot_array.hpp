#ifndef SLOT_ARRAY_HPP
#define SLOT_ARRAY_HPP

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef struct {
    void* data;
    uint32_t dataLen;
    uint32_t dataElementSize;

    // Reading
    bool* active;

    // Writing
    uint32_t* freeIndices;
    uint32_t freeLen;

    uint32_t capacity;
} SlotArray;

int SlotArrayInit(SlotArray* arr, uint32_t capacity, uint32_t elementSize);
void SlotArrayTerminate(SlotArray* arr);
void SlotArrayClear(SlotArray* arr);
void* SlotArrayGet(const SlotArray* arr, uint32_t index);
void* SlotArrayInsert(SlotArray* arr);
void* SlotArrayInsertGetIndex(SlotArray* arr, uint32_t* index);
void SlotArrayDel(SlotArray* arr, uint32_t index);
bool SlotArrayFull(const SlotArray* arr);
bool SlotArrayEmpty(const SlotArray* arr);
uint32_t SlotArrayLen(const SlotArray* arr);

#endif
