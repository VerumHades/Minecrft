#include <structures/posmap.h>

static inline PositionMapNode* getNodeFromPositionMap(PositionMap* map, Vec3* key);

uint32_t hash3D(Vec3* vec) {
    // Convert floats to integers
    uint32_t xi = (uint32_t)(vec->x * 1000.0f);
    uint32_t yi = (uint32_t)(vec->y * 1000.0f);
    uint32_t zi = (uint32_t)(vec->z * 1000.0f);

    // Perform bitwise operations to mix the bits
    xi ^= xi >> 16;
    xi *= 0x85ebca6b;
    xi ^= xi >> 13;
    xi *= 0xc2b2ae35;
    xi ^= xi >> 16;

    yi ^= yi >> 16;
    yi *= 0x85ebca6b;
    yi ^= yi >> 13;
    yi *= 0xc2b2ae35;
    yi ^= yi >> 16;

    zi ^= zi >> 16;
    zi *= 0x85ebca6b;
    zi ^= zi >> 13;
    zi *= 0xc2b2ae35;
    zi ^= zi >> 16;

    // Combine the hashes
    uint32_t hash = xi ^ yi ^ zi;
    return hash;
}

static inline int positionsEqual(Vec3* a, Vec3* b){
    return (a->x == b->x) && (a->y == b->y) && (a->z == b->z);
}

PositionMap* newPositionMap(){
    PositionMap* map = (PositionMap*) calloc(1,sizeof(PositionMap));
    
    map->capacity = 1024;
    map->list = calloc(map->capacity,sizeof(PositionMapNode));

    //map->nodesTotal = 0;
    //map->nodesSize = 1;
    //map->nodes = calloc(map->nodesSize, sizeof(PositionMapNode*));

    return map;
}

/*void addRegisteredNode(PositionMap* map, PositionMapNode* node){
    if(map->nodesSize < map->nodesTotal + 1){
        map->nodesSize *= 2;
        map->nodes = realloc(map->nodes, sizeof(PositionMapNode*) * map->nodesSize);
    }

    map->nodes[map->nodesTotal++] = node;
}*/

void freePositionMapNode(PositionMapNode* node){
    if(node->hasNext) freePositionMapNode(node->next);
    free(node);
}
void freePositionMap(PositionMap* map){
    for(int i = 0;i < map->capacity;i++){
        PositionMapNode* node = &map->list[i];
        if(node->taken && node->hasNext) freePositionMapNode(node->next);
    }
    free(map->list);
    free(map);
}

PositionMapNode emptyNode = {0};
void putIntoPositionMap(PositionMap* map, Vec3* key, void* value){
    uint32_t hash = hash3D(key);
    int index = hash % map->capacity;
    //printf("putting to [%i](%i)\n", hash,index);

    PositionMapNode* newNode;

    PositionMapNode* node = &map->list[index];
    if(!node->taken) newNode = node;
    else if(node->taken && positionsEqual(&node->key,key)) newNode = node; 
    else{
        PositionMapNode* cNode = node;

        int nodeMissing = 1;
        while(cNode->hasNext){
            cNode = cNode->next;
            if(!positionsEqual(&cNode->key,key)) continue;
            
            newNode = cNode;
            nodeMissing = 0;
            break;
        }
        
        if(nodeMissing){
            newNode = calloc(1,sizeof(PositionMapNode));
            //addRegisteredNode(map, newNode);
            cNode->hasNext = 1;
            cNode->next = newNode;
            newNode->parent = cNode;
            newNode->free = 1;
        }
    }

    *newNode = emptyNode;

    newNode->taken = 1;
    newNode->value = value;
    newNode->next = NULL;
    newNode->key = *key;
    //newNode->keyIndex = map->nodesTotal - 1;
}

void removeFromPositionMap(PositionMap* map, Vec3* key){
    PositionMapNode* node = getNodeFromPositionMap(map, key);
    if(node == NULL) return;

    if(node->free){
        node->parent->next = node->next;
        free(node);
        return;
    }
    
    if(node->hasNext){
        *node = *node->next;
        node->free = 0;
    }
    else *node = emptyNode;
}

void* getFromPositionMap(PositionMap* map, Vec3* key){
    PositionMapNode* node = getNodeFromPositionMap(map, key);
    if(node == NULL) return node;
    return node->value;
}

static inline PositionMapNode* getNodeFromPositionMap(PositionMap* map, Vec3* key){
    uint32_t hash = hash3D(key);
    int index = hash % map->capacity;
    //printf("getting from [%i](%i)\n", hash,index);

    PositionMapNode* node = &map->list[index];

    if(!node->taken) return NULL;
    
    do{
        //printf("Looking at %s\n", node->name);
        if(positionsEqual(&node->key, key)){
            //printf("Found %s\n", node->name);
            return node;
        }

        node = node->next;
    }while(node != NULL);

    return NULL;
}

