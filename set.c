#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "set.h"
#include "state.h"
#include "enums.h"
//set
Set* createSet(){
    Set* s = malloc(sizeof(Set));
    if(!s){
        perror("Set allocation fail");
        return NULL;
    }
    s -> data = malloc(sizeof(int)*2);
    if(!s->data){
        perror("Set data allocation fail");
        free(s);
        return NULL;
    }
    s -> length = 0;
    s -> maxSize = 2;
    // printf("[set.c: %d]: Set Created\n", __LINE__);
    return s;
}
void freeSet(Set* s){
    if(!s) return;
    free(s->data);
    free(s);
}

bool addToSet(Set* s, int x){
    if(!s) {
        return false;
    }   
    for (int i = 0; i < s->length; i++)
    {
        if(s->data[i] == x) return true;
    }
    if(s -> length >= s -> maxSize){
        int newSize = s->maxSize*2;
        int *temp = realloc(s->data, sizeof(int) * newSize);
        if (!temp) return false;
        s -> data = temp;
        s -> maxSize = newSize;
        // printf("[set.c: %d]: Expanded set to %d\n", __LINE__, s->maxSize);
    }
    s->data[s->length++] = x;
    return true;
};
int compare(const void* x, const void* y){
    int a = *(const int*) x;
    int b = *(const int*) y;
    return (a>b) - (a<b);
}

//set equality
bool isEqSets(Set* x, Set* y){
    if(!x || !y){
        return false;
    }
    if(x->length != y->length){
        return false;
    } else{
        qsort(x->data, x->length, sizeof(int), compare);
        qsort(y->data, y->length, sizeof(int), compare);
        for (int i = 0; i < x->length; i++)
        {
            if(x->data[i] != y->data[i]) return false;
        }
    }
    return true;
}
//create shallow copy of a set s
//to reduce complexity of managing memory for DFA states
Set* cloneSet(Set* s){
    if (!s) {
        perror("Clone set received null set pointer");
        return NULL;
    }
    Set* clone = createSet();
    if(!clone) {
        perror("Clone set could not allocate for clone");
        return NULL;
    }
    for(int i = 0; i < s->length; i++){
        if(!addToSet(clone, s->data[i])){
            perror("Set add fail");
            freeSet(clone);
            return NULL;
        }
    }
    return clone;
}
//sets
Sets* createSets(){
    Sets* s = malloc(sizeof(Sets));
    if(!s){
        perror("Sets allocation fail");
        return NULL;
    }
    s -> data = malloc(sizeof(Set*)*2);
    if(!s->data){
        perror("Sets data allocation fail");
        free(s);
        return NULL;
    }
    s -> length = 0;
    s -> maxSize = 2;
    for(int i = s->length; i < s->maxSize; i++){
        s->data[i] = NULL;
    }
    // printf("[set.c: %d]: Set Created\n", __LINE__);
    return s;
}
void freeSets(Sets* s){
    if(!s){
        return;
    }
    for (int i = 0; i < s->length; i++)
    {
        freeSet(s->data[i]);
    }
    free(s->data);
    free(s);
}

bool addToSets(Sets* s, Set* x){
    if(!s || !x) {
        perror("received null Sets pointer");
        return false;
    }   
    for (int i = 0; i < s->length; i++)
    {
        if(isEqSets(s->data[i], x)) {
            // printf("[set.c: %d]: Set already part of Sets, cleaning...\n", __LINE__);
            freeSet(x);
            return true;
        };
    }
    if(s -> length >= s -> maxSize){
        int newSize = s->maxSize*2;
        Set **temp = realloc(s->data, sizeof(Set*) * newSize);
        if (!temp){
            perror("addToSets Reallocation fail");
            return false;
        }
        s -> data = temp;
        for(int i = s->maxSize; i < newSize; i++){
            s->data[i] =  NULL;
        }
        s -> maxSize = newSize;
        // printf("[set.c: %d]: Expanded set to %d\n", __LINE__, s->maxSize);
    }
    s->data[s->length++] = x;
    return true;
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

