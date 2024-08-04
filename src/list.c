#include <list.h>

#define arrayLen(array) sizeof(array)/sizeof(array[0])

#define printList(list,type) \
    printf("["); \
    for(int i = 0;i < list.length;i++){\
        printf("%i", *((type*) getFromList(&list, i))); \
        if(list.length + 1 != i) printf(",");\
    } \
    printf("]\n"); \

void printListHeader(List* list){
    printf("== List(%p) ==\n",list);
    printf("Length: %i\n",list->length);
    printf("Real length:%i\n====\n\n", list->realLength);
}

List* newList(int initialSize){
    List* list = malloc(sizeof(List));
    list->data = calloc(initialSize, sizeof(void*));
    list->length = 0;
    list->realLength = initialSize;
    return list;
}
void freeList(List* list){
    free(list->data);
    free(list);
}
void addToList(List* list, void* element){
    if(list->realLength <= list->length+1){ // Real array is too small, double the size
        list->realLength *= 2;
        list->data = realloc(list->data, list->realLength * sizeof(void*));
    }

    list->data[list->length] = element;
    list->length++;
}
void setListElement(List* list, int index, void* element){
    if(index < 0 || index >= list->length) return;

    list->data[index] = element;
}

void removeFromList(List* list, int index){
    for(int i = index;i < list->length;i++){
        list->data[i] = list->data[i+1];
    }

    list->length--;
}
void removeLast(List* list){
    removeFromList(list, list->length-1);
}
void clearList(List* list){
    list->length = 0;
}

void* getFromList(List* list, int index){
    return list->data[index];
}
void* getLast(List* list){
    return list->data[list->length-1];
}