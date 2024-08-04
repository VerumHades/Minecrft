#ifndef HASHMAP_SIMPLE
#define HASHMAP_SIMPLE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list.h>

#include <standard.h>

typedef struct HashMapNode{
    void* value;
    int hash; 
    char* key;
    int keyIndex;
    unsigned hasNext: 1;

    struct HashMapNode* next;
} HashMapNode;

typedef struct HashMap{
    void** list;
    List* keys;
    int capacity;
} HashMap;

HashMap* newHashMap();
void freeHashMap(HashMap* map);

int hashCode(char* key);
void putIntoHashMap(HashMap* map, char* key, void* value);
void removeFromHashMap(HashMap* map, char* key);
void* getFromHashMap(HashMap* map, char* key);

#endif