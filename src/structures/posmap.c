#include <structures/posmap.h>

static inline PositionMapNode* getNodeFromPositionMap(PositionMap* map, Vec3* key);

// Hashing function for combining the hash values
uint32_t hashCombine(uint32_t hash, int32_t value) {
    value += 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash ^ value;
}

uint32_t hash3D(Vec3* key) {
    // Convert floats to integers with rounding
    int32_t xi = (int32_t)roundf(key->x * 1000.0f);
    int32_t yi = (int32_t)roundf(key->y * 1000.0f);
    int32_t zi = (int32_t)roundf(key->z * 1000.0f);

    // Combine all three coordinates into a single hash
    uint32_t hash = 0;
    hash = hashCombine(hash, xi);
    hash = hashCombine(hash, yi);
    hash = hashCombine(hash, zi);

    return hash;
}

static inline int positionsEqual(Vec3* a, Vec3* b){
    return (a->x == b->x) && (a->y == b->y) && (a->z == b->z);
}

PositionMap* newPositionMap(){
    PositionMap* map = (PositionMap*) calloc(1,sizeof(PositionMap));
    
    map->capacity = 1024 * 4;
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

void forEachPositionInMap(PositionMap* map, void(*callback)(void*)){
    for(int i = 0;i < map->capacity;i++){

        PositionMapNode* node = &map->list[i];

        if(!node->taken) continue;
       
        do{
            callback(node->value);

            node = node->next;
        }while(node != NULL);
    }
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

