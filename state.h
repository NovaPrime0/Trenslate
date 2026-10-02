#ifndef STATE
#define STATE
#include "enums.h"
typedef struct{
    int stateNumber;
    int transition[256];
    StateType type;
} State;

typedef struct{
    State *data;
    int length;
    int maxsize;
} StateList;

//statelist
StateList* createList();
void addTransition(State* s, int transition, char* transitionSymbol);
StateList* loadList(char* file);
void freeList(StateList* sl);
//state
State* createState(int stateNumber, StateType type);
int read(State* s, char symbol);
void printState(State* s, FILE* fpt);
#endif