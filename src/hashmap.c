#include <hashmap.h>

HashMapNode* getFromHashMapNode(HashMap* map, char* key);

int hashCode(char* key){
    unsigned int out = 0;

    int len = strlen(key);
    for(int i = 0;i < len;i++){
        out += key[i] * 31;
    }

    return out;
}

HashMap* newHashMap(){
    HashMap* map = (HashMap*) malloc(sizeof(HashMap));
    
    map->capacity = 255;
    map->list = (void**) calloc(map->capacity,sizeof(void*));
    map->keys = newList(1);

    return map;
}

void freeNode(HashMapNode* node){
    if(node->hasNext) freeNode(node->next);
    free(node);
}
void freeHashMap(HashMap* map){
    for(int i = 0;i < map->keys->length;i++){
        freeNode(getFromHashMapNode(map, getFromList(map->keys, i)));
        free(getFromList(map->keys,i));
    }
    free(map->list);
    freeList(map->keys);
    free(map);
}

void putIntoHashMap(HashMap* map, char* key, void* value){
    int hash = hashCode(key);
    int index = hash % map->capacity;
    //printf("putting to [%i](%i)\n", hash,index);

    HashMapNode* node = map->list[index];
    HashMapNode* newNode = malloc(sizeof(HashMapNode));
    
    char* allocatedKey = allocateString(key);

    newNode->hash = hash;
    newNode->value = value;
    newNode->next = NULL;
    newNode->key = allocatedKey;
    newNode->keyIndex = map->keys->length;

    if(getFromHashMap(map,key) == NULL) addToList(map->keys, allocatedKey);

    if(node == NULL){
        map->list[index] = newNode;
        return;
    }

    while(node->next != NULL){
        node = node->next;
    }

    node->hasNext = 1;
    node->next = newNode;
}

void removeFromHashMap(HashMap* map, char* key){
    int hash = hashCode(key);
    int index = hash % map->capacity;
    //printf("getting from [%i](%i)\n", hash,index);

    HashMapNode* node = map->list[index];

    if(node == NULL){
        return;
    }

    HashMapNode* lastNode = NULL;
    do{
        //printf("Looking at %s\n", node->name);
        if(strcmp(node->key, key) == 0){
            //printf("Found %s\n", node->name);
            if(lastNode != NULL) lastNode->next = node->next;
            if(lastNode != NULL) lastNode->hasNext = node->hasNext;

            removeFromList(map->keys, node->keyIndex);
            free(node);
            if(lastNode == NULL) map->list[index] = NULL;
        }

        lastNode = node;
        node = node->next;
    }while(node != NULL);
}

void* getFromHashMap(HashMap* map, char* key){
    HashMapNode* node = getFromHashMapNode(map, key);
    if(node == NULL) return node;
    return node->value;
}

HashMapNode* getFromHashMapNode(HashMap* map, char* key){
    int hash = hashCode(key);
    int index = hash % map->capacity;
    //printf("getting from [%i](%i)\n", hash,index);

    HashMapNode* node = map->list[index];

    if(node == NULL){
        return NULL;
    }

    do{
        //printf("Looking at %s\n", node->name);
        if(strcmp(node->key, key) == 0){
            //printf("Found %s\n", node->name);
            return node;
        }

        node = node->next;
    }while(node != NULL);

    return NULL;
}

