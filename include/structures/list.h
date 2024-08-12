#ifndef LIST_SIMPLE
#define LIST_SIMPLE

#include <stdio.h>
#include <stdlib.h>

typedef struct List{
    void** data; 
    int length; 
    int realLength;
} List;

#define forEach(list, type, element) \
    for (int _i = 0; _i < (list)->length; _i++) \
        for (type* element = (type*) getFromList((list), _i); element != NULL; element = NULL)

void printListHeader(List* list);

List* newList(int initialSize);
void freeList(List* list);

void addToList(List* list, void* element);
void setListElement(List* list, int index, void* element);

void removeFromList(List* list, int index);
void removeLast(List* list);
void clearList(List* list);

void* getFromList(List* list, int index);
void* getLast(List* list);


#endif