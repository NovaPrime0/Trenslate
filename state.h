#ifndef STATE
#define STATE
#include "enums.h"
#include "set.h"
typedef struct {
    int stateNumber;
    char symbol[30];
} Transition;
typedef struct{
    Transition *data;
    int length;
    int maxSize;
} TransitionList;
typedef struct State{
    int stateNumber;
    TransitionList* transitionList;
    StateType type;
} State;  

typedef struct DFAState DFAState;
struct DFAState{
    Set *stateNumber; 
    Set *transitionNumbers[256]; 
    DFAState *next[256];
    StateType type;
};

typedef struct {
    DFAState* data;
    int length;
    int maxsize;
} DFAStateList;

typedef struct{
    State *data;
    int length;
    int maxsize;
} StateList;


//statelist
StateList* createList();
StateList* loadList(char* file);
void freeList(StateList* sl);
//state
State* createState(int stateNumber, StateType type);
void printState(State* s, FILE* fpt);
#endif