#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "state.h"
#include "enums.h"
#include "set.h"
TransitionList* createTransitionList(){
    TransitionList* s = malloc(sizeof(TransitionList));
    s -> data = malloc(sizeof(Transition)*2);
    s -> length = 0;
    s -> maxSize = 2;
    printf("[state.c: %d]: Set Created\n", __LINE__);
    return s;
}
void freeTransitionList(TransitionList* s){
    free(s->data);
    free(s);
}

void expandTransitionList(TransitionList* s){
    if(!s) {
        printf("[state.c: %d]: File not Found\n", __LINE__);
        return;
    }   
    s -> maxSize *= 2;
    s -> data = realloc(s->data, sizeof(Transition) * s->maxSize);
    printf("[state.c: %d]: Expanded Transition List to %d\n", __LINE__, s->maxSize);
};

//state
State* createState(int stateNumber, StateType type){   
    State* s = malloc(sizeof(State));
    s -> stateNumber = stateNumber; 
    s -> transitionList = malloc(sizeof(Transition)*2);
    s -> type = type;
    return s;
}

void printState(State *s, FILE* fpt) {
    if(!fpt || !s) {
        printf("[state.c: %d]: state or fpt not found\n",__LINE__);
        return;
    };
    
    fprintf(fpt, "State: %d, StateType: %d\nTransitionLength: %d\n", s->stateNumber, s->type, s->transitionList->length);
    if(s->transitionList->length > 0){
        fprintf(fpt, "{");
        for(int i = 0; i < s->transitionList->length; i++){
            fprintf(fpt, "%d:%s%s", s->transitionList->data[i].stateNumber, s->transitionList->data[i].symbol, i <= s->transitionList->length-2 ? ", ": "}\n\n");
        }
    }
    else fprintf(fpt,"\n");
}


static int resolveType(char* typestring){
    StateType lim = INTERMEDIARY;
    for(int i = 0; i <= (int) lim; i++){
        if(strcmp(stateNames[i], typestring) == 0) return i;
    }
    return -1;
}
static void resetState(State* s){
    s->stateNumber = -1;
    s->type = INTERMEDIARY;
}
static void resetDFAState(DFAState* s){
    s->stateNumber = NULL;
    s->type = INTERMEDIARY;
    for(int i = 0; i < 256; i++){
        s->next[i] = NULL;
        s->transitionNumbers[i] = createSet();
    }
}
//statelist
static void expandList(StateList* sl){
    sl -> maxsize *= 2;
    State* temp = realloc(sl->data, sizeof(State) * sl -> maxsize);
    if (!temp) {
        perror("[state.c]: Realloc Fail\n");
        return;
    }
    sl -> data = temp;
    printf("[state.c: %d]: Expanded List to %d\n",__LINE__,sl->maxsize);
    for(int i = sl -> length; i < sl->maxsize; i++){
        printf("\treset %d\n",i);
        resetState(&sl->data[i]);
    }
}
static void expandDFAList(DFAStateList* sl){
    sl -> maxsize *= 2;
    DFAState* temp = realloc(sl->data, sizeof(DFAState) * sl -> maxsize);
    if (!temp) {
        perror("[state.c]: Realloc Fail\n");
        return;
    };
    sl -> data = temp;
    printf("[state.c: %d]: Expanded List to %d\n",__LINE__,sl->maxsize);
    for(int i = sl -> length; i < sl->maxsize; i++){
        printf("\treset %d\n",i);
        resetDFAState(&sl->data[i]);
    }
}

StateList* createList(){
    StateList *sl= malloc(sizeof(StateList));
    sl->data = malloc(sizeof(State)*2);
    sl->length = 0;
    sl-> maxsize = 2;
    for(int i = sl -> length; i < sl->maxsize; i++){
        resetState(&sl->data[i]);
    }
    return sl;
}

StateList* loadList(char* file){
    printf("[state.c: %d]: Loading file...\n\n",__LINE__);
    FILE* f = fopen(file, "r");
    if(f == NULL) return NULL;
    printf("===================================\n\n");
    printf("[state.c: %d]:\tLoaded file\n\n",__LINE__);
    printf("===================================\n\n");
    StateList *sl = createList();
    char transitionbuffer[256];
    char symbolsbuffer[256];
    char *transitionSave;
    char *symbolSave;
    char typebuffer[64];
    while(fscanf(f,
        "State: %d, Transitions: %255[^,], Symbols: %255[^,], StateType: %63[^,],\n",
        &sl->data[sl->length].stateNumber, transitionbuffer, symbolsbuffer, typebuffer) == 4){
        printf("[state.c: %d]: %-12s %d\n\t%-20s %s\n\t%-20s %s\n",__LINE__, "State:", sl->data[sl->length].stateNumber, "Full Symbol List:", symbolsbuffer, "Full Dest list:", transitionbuffer);
        char *transitionSplit = strtok_r(transitionbuffer, ":", &transitionSave);
        char *symbolSplit = strtok_r(symbolsbuffer, ":", &symbolSave);
        TransitionList *tl = createTransitionList();
        while(transitionSplit && symbolSplit){
            if(tl->length + 1 >= tl->maxSize) expandTransitionList(tl);
            if(strcmp(transitionSplit, "none") == 0){
                tl->data[tl->length].stateNumber = atoi(transitionSplit);
                strcpy(tl->data[tl->length].symbol, symbolSplit);
                printf("[state.c: %d]: StateNum: %d, Symbol: %s\n", __LINE__, tl->data[tl->length].stateNumber, tl->data[tl->length].symbol);
                tl->length++;
            }
            transitionSplit = strtok_r(NULL, ":", &transitionSave);
            symbolSplit = strtok_r(NULL, ":", &symbolSave);
        }
        sl->data[sl->length].transitionList = tl;
        sl->data[sl->length].type = resolveType(typebuffer);
        sl->length++;
        if(sl->length >= sl->maxsize){
            expandList(sl);
        }
    }
    printf("\n\n[state.c: %d]: list size:maxsize = %d:%d\n\n",__LINE__, sl->length, sl->maxsize);
    fclose(f);
    return sl;
}

int inSigma(int x){
    if(x >= 'A' && x <= 'Z' || 
        x >= 'a' && x <= 'z' ||
        x >= '0' && x <= '9') {
            return 1;
        }
    else return x != 0 && strchr(SIGMA, x) != NULL;
}

// decision tree, repurpose to finalize DFAState later
void addTransition(DFAState* s, Transition* t){
    if(!s || !t) return;
    printf("\n[state.c: %d]:Received: %s\n",__LINE__, t->symbol);
    char *tokSave;
    char *tokBuff = strtok_r(t->symbol, "|", &tokSave);
    while(tokBuff){
        if(strlen(tokBuff) > 2){
            if(strcmp("ALPHA", tokBuff) == 0) {
                for(int i = 'A'; i <= 'Z'; i++){
                    addToSet(s->transitionNumbers[i], t->stateNumber);
                    printf("[state.c: %d]: bound transition to %c", __LINE__, i);
                }
            }
            else if(strcmp("alpha", tokBuff) == 0) {
                for(int i = 'a'; i <= 'z'; i++) {
                    addToSet(s->transitionNumbers[i], t->stateNumber);
                    printf("[state.c: %d]: bound transition to %c", __LINE__, i);
                }
            }
            else if(strcmp("num", tokBuff) == 0) {
                for(int i = '0'; i <= '9'; i++) {
                    addToSet(s->transitionNumbers[i], t->stateNumber);
                    printf("[state.c: %d]: bound transition to %c", __LINE__, i);
                }
            }
            else if(strcmp("sym", tokBuff) == 0) {
                for(int i = 0; i < SYMBOLCOUNT; i++) {
                    addToSet(s->transitionNumbers[(unsigned char) SYMBOLS[i]], t->stateNumber);
                    printf("[state.c: %d]: bound transition to %c", __LINE__, i);
                }
            }
            else if(strcmp("whitespace", tokBuff) == 0) {
                for(int i = 0; i < WHITESPACECOUNT; i++) {
                    addToSet(s->transitionNumbers[(unsigned char) WHITESPACES[i]], t->stateNumber);
                    printf("[state.c: %d]: bound transition to %c", __LINE__, i);
                }
            }
            else if(strcmp("comma", tokBuff) == 0) {
                addToSet(s->transitionNumbers[(unsigned char)','], t->stateNumber);
                printf("[state.c: %d]: bound transition to %c", __LINE__, ',');
            }
            else if(strcmp("pipe", tokBuff) == 0) {
                addToSet(s->transitionNumbers[(unsigned char)'|'], t->stateNumber);
                printf("[state.c: %d]: bound transition to %c", __LINE__, '|');
            }
            else if(strcmp("invalid", tokBuff) == 0) {
                for(int i = 0; i < 256; i++){
                    if(inSigma(i)) continue; 
                    addToSet(s->transitionNumbers[i], t->stateNumber);
                    printf("[state.c: %d]: bound transition to %d", __LINE__, i);
                }
            }
        } else if (strlen(tokBuff) == 2 && tokBuff[0] == '\\'){
            unsigned char c;
            switch(tokBuff[1]){
                case 'n':
                    c = '\n';
                    break;
                case 'r':
                    c = '\r';
                    break;
                case 't':
                    c = '\t';
                    break;
                default:
                    printf("[state.c: %d]: Unknown special character \\%c", __LINE__, tokBuff[1]);
                    break;
            }
            addToSet(s->transitionNumbers[c], t->stateNumber);
            printf("[state.c: %d]:\tmodified %c:%d\n",__LINE__, (tokBuff[1]=='n') ? '\n': '\t', tokBuff);
        } else if (strlen(tokBuff) == 1) {
            addToSet(s->transitionNumbers[(unsigned char) tokBuff[0]],  t->stateNumber);
            printf("[state.c: %d]:\tmodified %c:%d\n",__LINE__, (unsigned char) *tokBuff, tokBuff);
        }
        tokBuff = strtok_r(NULL, "|", &tokSave);
    }
}
void freeList(StateList* sl){
    free(sl->data);
    free(sl);
}
void freeDFAList(DFAStateList* sl){
    for (int i = 0; i < sl->length; i++)
    {
        free(sl->data[i].stateNumber);
        for (int i = 0; i < 256; i++)
        {
            free(sl->data[i].transitionNumbers[i]);
        }
        
        free(sl->data[i].stateNumber);
    }
    
    free(sl->data);
    free(sl);
}

static void expandList(DFAStateList* sl){
    sl -> maxsize *= 2;
    sl -> data = realloc(sl->data, sizeof(State) * sl -> maxsize);
    printf("[state.c: %d]: Expanded List to %d\n",__LINE__,sl->maxsize);
    for(int i = sl -> length; i < sl->maxsize; i++){
        printf("\treset %d\n",i);
        resetDFAState(&sl->data[i]);
    }
}

DFAStateList* createDFAList(){
    DFAStateList *sl = malloc(sizeof(DFAStateList));
    if(!sl){
        perror("DFAList malloc fail");
        return NULL;
    }
    sl->data = malloc(sizeof(DFAState)*2);
    if(!sl->data){
        perror("DFAList data malloc fail");
        free(sl);
        return NULL;
    }
    sl->length = 0;
    sl-> maxsize = 2;
    for(int i = sl -> length; i < sl->maxsize; i++){
        resetDFAState(&sl->data[i]);
    }
    return sl;
}


void NToD(State* s, DFAState* d){
    d->type = s->type;
    for(int i = 0; i < s->transitionList->length; i++){
        addTransition(d, &s->transitionList->data[i]);
    }
}

void SetToD(StateList* sl, DFAState* d, Set* set){
    d->stateNumber = set;
    d->type = 0;
    for(int i = 0; i < d->stateNumber->length; i++){
        //find NFA equivalent for each statenumber in d
        State* equivalent = NULL;
        for(int j = 0; j < sl->length; j++){
            if(sl->data[j].stateNumber == d->stateNumber->data[i]){
                equivalent = &sl->data[j];
            }
        }
        if(equivalent) NToD(equivalent, d);
    }
}
//set
void subsetConstruction(StateList *st){
    DFAStateList *DFAList = createDFAList();
    Sets* workingSet = createSets();
    Set* start = createSet();
    //include start state to set
    addToSet(start, st->data[0].stateNumber);
    addToSets(workingSet, start);
    int processed = 0;
    while (processed < workingSet->length)
    {  
        //pick the next unprocessed state
        DFAState* workingDFAState = &DFAList->data[processed];
        SetToD(st, workingDFAState, workingSet->data[processed]);
        for(int i = 0; i < 256; i++){
            //add all sets not included in working set (by value)
            if(workingDFAState->transitionNumbers[i]->length > 0){
                addToSets(workingSet, workingDFAState->transitionNumbers[i]);
            }
        }
        processed++;
    }
    freeSets(workingSet);
}