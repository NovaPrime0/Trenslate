#include <stdio.h>
#include <stdlib.h>
#include "set.h"
#include "state.h"
#include "enums.h"
//set
Set* createSet(){
    Set* s = malloc(sizeof(Set));
    s -> data = malloc(sizeof(int)*2);
    s -> length = 0;
    s -> maxSize = 2;
    printf("[set.c: %d]: Set Created\n", __LINE__);
    return s;
}
void freeSet(Set* s){
    free(s->data);
    free(s);
}

void addToSet(Set* s, int x){
    if(!s) {
        printf("[set.c: %d]: File not Found\n", __LINE__);
        return;
    }   
    for (int i = 0; i < s->length; i++)
    {
        if(s->data[i] == x) return;
    }
    if(s -> length + 1 >= s -> maxSize){
        s -> maxSize *= 2;
        s -> data = realloc(s->data, sizeof(int) * s->maxSize);
        printf("[set.c: %d]: Expanded set to %d\n", __LINE__, s->maxSize);
    }
    s->data[s->length++] = x;
};
//set equality
int isEqSets(Set* x, Set* y){
    
    if(x->length != y->length){
        return 0;
    } else{
        qsort(x->data, x->length, sizeof(int), compare);
        qsort(y->data, y->length, sizeof(int), compare);
        for (int i = 0; i < x->length; i++)
        {
            if(x->data[i] != y->data[i]) return 0;
        }
    }
    return 1;
}
//sets
Sets* createSets(){
    Sets* s = malloc(sizeof(Sets));
    s -> data = malloc(sizeof(Set*)*2);
    s -> length = 0;
    s -> maxSize = 2;
    printf("[set.c: %d]: Set Created\n", __LINE__);
    return s;
}
void freeSets(Sets* s){
    for (int i = 0; i < s->length; i++)
    {
        free(s->data[i]);
    }
    free(s->data);
    free(s);
}
int compare(const void* x, const void* y){
    int a = *(const int*) x;
    int b = *(const int*) y;
    return (a>b) - (b-a);
}

void addToSets(Sets* s, Set* x){
    if(!s) {
        printf("[set.c: %d]: File not Found\n", __LINE__);
        return;
    }   
    for (int i = 0; i < s->length; i++)
    {
        if(isEqSets(s->data[i], x)) return;
    }
    if(s -> length + 1 >= s -> maxSize){
        s -> maxSize *= 2;
        s -> data = realloc(s->data, sizeof(Set*) * s->maxSize);
        printf("[set.c: %d]: Expanded set to %d\n", __LINE__, s->maxSize);
    }
    s->data[s->length++] = x;
};

// DFASet* createDFASet(){
//     DFASet* s = malloc(sizeof(DFASet));
//     s -> data = malloc(sizeof(DFAState)*2);
//     s -> length = 0;
//     s -> maxSize = 2;
//     printf("[set.c: %d]: Set Created\n", __LINE__);
//     return s;
// }
// void freeDFASet(DFASet* s){
//     free(s->data);
//     free(s);
// }

// void addtoDFASet(DFASet* s, DFAState *x){
//     if(!s) {
//         printf("[set.c: %d]: File not Found\n", __LINE__);
//         return;
//     }   
//     for (int i = 0; i < s->length; i++)
//     {
//         if(s->data[i] == x) return;
//     }
//     if(s -> length + 1 >= s -> maxSize){
//         s -> maxSize *= 2;
//         s -> data = realloc(s->data, sizeof(DFAState*) * s->maxSize);
//         printf("[set.c: %d]: Expanded set to %d\n", __LINE__, s->maxSize);
//     }
//     s->data[s->length++] = x; 
// }

