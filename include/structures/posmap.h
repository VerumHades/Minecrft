#ifndef POSITION_MAP_H
#define POSITION_MAP_H 

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef struct Vec3{
    float x; float y; float z;
} Vec3;

typedef struct PositionMapNode{
    unsigned taken: 1;
    unsigned free: 1;

    void* value;
    Vec3 key;
    //int keyIndex;
    unsigned hasNext: 1;

    struct PositionMapNode* next;
    struct PositionMapNode* parent;
} PositionMapNode;

typedef struct PositionMap{
    PositionMapNode* list;
    
    PositionMapNode** nodes;
    size_t nodesTotal;
    size_t nodesSize; 
    
    int capacity;
} PositionMap;

PositionMap* newPositionMap();
void freePositionMap(PositionMap* map);

uint32_t hash3D(Vec3* vec);
void putIntoPositionMap(PositionMap* map, Vec3* key, void* value);
void removeFromPositionMap(PositionMap* map, Vec3* key);
void* getFromPositionMap(PositionMap* map, Vec3* key);
void forEachPositionInMap(PositionMap* map, void(*callback)(void*));


#endif