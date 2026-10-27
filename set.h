#ifndef SET_H
#define SET_H
#include <stdbool.h>
typedef struct{
    int* data;
    int length;
    int maxSize;
} Set; 
typedef struct{
    Set** data;
    int length;
    int maxSize;
} Sets; 
// typedef struct{
//     struct DFAState** data;
//     int length;
//     int maxSize;
// } DFASet;

Set* createSet();
void freeSet(Set* s);
bool addToSet(Set* s, int x);
Set* cloneSet(Set* s);

Sets* createSets();
void freeSets(Sets* s);
bool addToSets(Sets* s, Set* x);
int compare(const void* x, const void* y);
bool isEqSets(Set* x, Set* Y);

// DFASet* createDFASet();
// void freeDFASet(DFASet* s);
// void addtoDFASet(DFASet* s, DFAState* x);

#endif