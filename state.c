#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "state.h"
#include "enums.h"
#include "set.h"

TransitionList* createTransitionList(){
    TransitionList* s = malloc(sizeof(TransitionList));
    if(!s){
        perror("TransitionList malloc fail");
        return NULL;
    }
    s -> data = malloc(sizeof(Transition)*2);
    if(!s->data){
        perror("TransitionList data malloc fail");
        free(s);
        return NULL;
    }
    s -> length = 0;
    s -> maxSize = 2;
    printf("[state.c: %d]: Set Created\n", __LINE__);
    return s;
}
void freeTransitionList(TransitionList* s){
    if(!s){
        return;
    }
    free(s->data);
    free(s);
}

bool expandTransitionList(TransitionList* s){
    if(!s) {
        perror("null transitionList");
        return false;
    }   
    int newSize = s->maxSize*2;
    Transition* temp =  realloc(s->data, sizeof(Transition) * newSize);
    if(!temp){
        perror("realloc fail for transitionList");
        return false;
    }

    s -> maxSize = newSize;
    s -> data = temp;
    printf("[state.c: %d]: Expanded Transition List to %d\n", __LINE__, s->maxSize);
    return true;
};

//state
static State* createState(int stateNumber, StateType type){   
    State* s = malloc(sizeof(State));
    if (!s) {
        perror("CreateState Allocation failed");
        return NULL;
    }
    s -> stateNumber = stateNumber; 
    s -> transitionList = createTransitionList();
    if (!s->transitionList){
        perror("CreateState TransitionList Allocation failed");
        free(s);
        return NULL;
    }
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
    s->transitionList = NULL;
}

static void initDFAState(DFAState* s){
    s->stateNumber = NULL;
    s->type = -1;
    s->typeSet = NULL;       
    for(int i = 0; i < 256; i++){
        s->next[i] = NULL;
        s->transitionNumbers[i] = NULL;
    }
}
static bool resetDFAState(DFAState* s){
    Set* temp = createSet();
    if(!temp){
        perror("TypeSet allocation fail inside resetDFAState");
        return false;
    }
    s->typeSet = temp;
    return true;
}

static bool bindChar(DFAState *s, unsigned char c, int x){
    if (!s) return false;
    if(!s->transitionNumbers[c]) {
        Set* tempSet = createSet();
        if(!tempSet){
            perror("bindChar set allocation fail");
            return false;
        }
        s->transitionNumbers[c] = tempSet;
    }
    return addToSet(s->transitionNumbers[c], x);

}
//statelist
static bool expandList(StateList* sl){
    int old = sl->maxsize;
    int newSize = sl->maxsize*2;
    State* temp = realloc(sl->data, sizeof(State) * newSize);
    if (!temp) {
        perror("[state.c]: Realloc Fail\n");
        return false;
    }
    sl->maxsize = newSize;
    sl -> data = temp;
    printf("[state.c: %d]: Expanded List to %d\n",__LINE__,sl->maxsize);
    for(int i = old; i < sl->maxsize; i++){
        printf("\treset %d\n",i);
        resetState(&sl->data[i]);
    }
    return true;
}
static bool expandDFAList(DFAStateList* sl){
    int old = sl->maxsize;
    int newSize = sl->maxsize*2;
    DFAState* temp = realloc(sl->data, sizeof(DFAState) * newSize);
    if (!temp) {
        perror("[state.c]: Realloc Fail\n");
        return false;
    };
    sl -> data = temp;
    sl -> maxsize = newSize;
    printf("[state.c: %d]: Expanded List to %d\n",__LINE__,sl->maxsize);
    for(int i = old; i < sl->maxsize; i++){
        initDFAState(&sl->data[i]);
    }
    for(int i = old; i < sl->maxsize; i++){
        printf("\treset %d\n",i);
        if(!resetDFAState(&sl->data[i])){
            perror("resetDFA failed");
            return false;
        };
    }
    return true;
}

StateList* createList(){
    StateList *sl= malloc(sizeof(StateList));
    if(!sl) return NULL;
    sl->data = malloc(sizeof(State)*2);
    if(!sl->data) {
        free(sl);
        return NULL;
    }
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
    if(!sl){
        perror("StateList allocation on loadlist fail");
        fclose(f);
        return NULL;
    }
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
        if(!tl){
            perror("TransitionList in loadList allocation fail");
            freeList(sl);
            fclose(f);
            return NULL;
        }
        while(transitionSplit && symbolSplit){
            if(tl->length + 1 >= tl->maxSize) {
                if(!expandTransitionList(tl)){
                    perror("expansion failed for transitionList");
                    freeTransitionList(tl);
                    freeList(sl);
                    fclose(f);
                    return NULL;
                }
            };
            if(strcmp(transitionSplit, "none") != 0){
                tl->data[tl->length].stateNumber = atoi(transitionSplit);
                size_t l = strlen(symbolSplit);
                size_t cap = sizeof(tl->data[tl->length].symbol);
                if(l >= cap){
                    perror("Token/TokenGroups larger than transitionSymbol size");
                    freeTransitionList(tl);
                    freeList(sl);
                    fclose(f);
                    return NULL;
                }
                strcpy(tl->data[tl->length].symbol, symbolSplit);
                printf("[state.c: %d]: StateNum: %d, Symbol: %s\n", __LINE__, tl->data[tl->length].stateNumber, tl->data[tl->length].symbol);
                tl->length++;
            }
            transitionSplit = strtok_r(NULL, ":", &transitionSave);
            symbolSplit = strtok_r(NULL, ":", &symbolSave);
        }
        if((transitionSplit == NULL) != (symbolSplit == NULL)){
            perror("mismatched transition and symbol lengths");
            freeTransitionList(tl);
            freeList(sl);
            fclose(f);
            return NULL;
        }
        sl->data[sl->length].transitionList = tl;
        int tempType = resolveType(typebuffer);
        sl->data[sl->length].type = tempType != -1 ? tempType : INTERMEDIARY;
        sl->length++;
        if(sl->length >= sl->maxsize){
            if(!expandList(sl)) {
                perror("expand List Fail on Loadlist");
                freeList(sl);
                fclose(f);
                return NULL;
            }
        }
    }
    printf("\n\n[state.c: %d]: list size:maxsize = %d:%d\n\n",__LINE__, sl->length, sl->maxsize);
    fclose(f);
    return sl;
}

int inSigma(int x){
    if((x >= 'A' && x <= 'Z') || 
        (x >= 'a' && x <= 'z') ||
        (x >= '0' && x <= '9')) {
            return 1;
        }
    else return x != 0 && strchr(SIGMA, x) != NULL;
}

void freeList(StateList* sl){
    if(!sl){
        return;
    }
    for (int i = 0; i < sl->maxsize; i++)
    {
        if(sl->data[i].transitionList) freeTransitionList(sl->data[i].transitionList);
    } 
    free(sl->data);
    free(sl);
}
void freeDFAList(DFAStateList* sl){
    if(!sl){
        return;
    }
    for (int i = 0; i < sl->maxsize; i++)
    {
        freeSet(sl->data[i].stateNumber);
        for (int j = 0; j < 256; j++)
        {
            if(sl->data[i].transitionNumbers[j]) freeSet(sl->data[i].transitionNumbers[j]);
        }
        freeSet(sl->data[i].typeSet);
    }
    free(sl->data);
    free(sl);
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
        initDFAState(&sl->data[i]);
    }
    for(int i = sl -> length; i < sl->maxsize; i++){
        if(!resetDFAState(&sl->data[i])){
            perror("DFA state reset fail");
            freeDFAList(sl);
            return NULL;
        }
    }
    return sl;
}
static StateType resolveTypePriority(Set* s){
    if (!s || s->length <= 0) return INTERMEDIARY;
    int typePriorities[s->length];
    for (int i = 0; i < s->length; i++)
    {
        if(s->data[i] == S_IDENTIFIER){
            typePriorities[i] = 6;
        } 
        else if(s->data[i] >= S_FOR && s->data[i] <= S_IMPORT){
            typePriorities[i] = 1;
        }
        else if(s->data[i] >= S_Op_DIV_INT && s->data[i] <= S_Cond_NOT){
            typePriorities[i] = 2;
        }
        else if(s->data[i] >= S_LINE_END && s->data[i] <= S_Rel_L){
            typePriorities[i] = 3;
        }
        else if(s->data[i] >= S_Lit_STRING && s->data[i] <= S_CONST){
            typePriorities[i] = 4;
        }
        else if(s->data[i] >= S_WHITESPACE && s->data[i] <= S_C_LINE){
            typePriorities[i] = 5;
        }
        else{
            typePriorities[i] = 7;
        }
    }
    int highestPriorityIDX = 0;
    for(int i = 0; i < s->length; i++){
        if(typePriorities[i] < typePriorities[highestPriorityIDX]){
            highestPriorityIDX = i;
        }
    }
    return (StateType) s->data[highestPriorityIDX];
}


// decision tree, repurpose to finalize DFAState later
bool addTransition(DFAState* s, Transition* t){
    if(!s || !t) return false;
    printf("\n[state.c: %d]:Received: %s\n",__LINE__, t->symbol);
    char *tokSave;
    char symbolCpy[65                       ];
    strncpy(symbolCpy, t->symbol, sizeof symbolCpy -1);
    symbolCpy[sizeof symbolCpy -1] = '\0';
    symbolCpy[strcspn(symbolCpy,"\n")] = '\0';
    char *tokBuff = strtok_r(symbolCpy, "|", &tokSave);
    while(tokBuff){
        if(strlen(tokBuff) > 2){
            if(strcmp("ALPHA", tokBuff) == 0) {
                for(int i = 'A'; i <= 'Z'; i++){
                    if(!bindChar(s, i, t->stateNumber))return false;
                    printf("[state.c: %d]: bound transition to %c\n", __LINE__, (char)i);
                }
            }
            else if(strcmp("alpha", tokBuff) == 0) {
                for(int i = 'a'; i <= 'z'; i++) {
                    if(!bindChar(s, i, t->stateNumber)) return false;
                    printf("[state.c: %d]: bound transition to %c\n", __LINE__, (char)i);
                }
            }
            else if(strcmp("num", tokBuff) == 0) {
                for(int i = '0'; i <= '9'; i++) {
                    if(!bindChar(s, i, t->stateNumber))return false;
                    printf("[state.c: %d]: bound transition to %c\n", __LINE__, (char)i);
                }
            }
            else if(strcmp("sym", tokBuff) == 0) {
                for(int i = 0; i < SYMBOLCOUNT; i++) {
                    if(!bindChar(s, SYMBOLS[i], t->stateNumber))return false;
                    printf("[state.c: %d]: bound transition to %c\n", __LINE__, SYMBOLS[i]);
                }
            }
            else if(strcmp("whitespace", tokBuff) == 0) {
                for(int i = 0; i < WHITESPACECOUNT; i++) {
                    if(!bindChar(s, WHITESPACES[i], t->stateNumber))return false;
                    printf("[state.c: %d]: bound transition to %c\n", __LINE__, WHITESPACES[i]);
                }
            }
            else if(strcmp("comma", tokBuff) == 0) {
                if(!bindChar(s, ',', t->stateNumber))return false;
                printf("[state.c: %d]: bound transition to %c\n", __LINE__, ',');
            }
            else if(strcmp("pipe", tokBuff) == 0) {
                if(!bindChar(s, '|', t->stateNumber)) return false;
                printf("[state.c: %d]: bound transition to %c\n", __LINE__, '|');
            }
            else if(strcmp("invalid", tokBuff) == 0) {
                for(int i = 0; i < 256; i++){
                    if(inSigma(i)) continue; 
                    if(!bindChar(s, i, t->stateNumber)) return false;
                    printf("[state.c: %d]: bound transition to %d\n", __LINE__, i);
                }
            } else {
                return false;
            }
        } else if (strlen(tokBuff) == 2 && tokBuff[0] == '\\'){
            int c;
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
                    c = -1;
                    printf("[state.c: %d]: Unknown special character \\%c\n", __LINE__, tokBuff[1]);
                    return false;
                    break;
            }
            if(c != -1 && !bindChar(s, c, t->stateNumber)) return false;
            printf("[state.c: %d]:\tmodified %c:%s\n",__LINE__, (tokBuff[1]=='n') ? '\n': '\t', tokBuff);
        } else if (strlen(tokBuff) == 1) {
            if(!bindChar(s, tokBuff[0], t->stateNumber)) return false;
            printf("[state.c: %d]:\tmodified %c:%s\n",__LINE__, (unsigned char) *tokBuff, tokBuff);
        }
        tokBuff = strtok_r(NULL, "|", &tokSave);
    }
    return true;
}
bool NToD(State* s, DFAState* d){
    if(!s || !d || !s->transitionList || !d->typeSet || !addToSet(d->typeSet, (int) s->type)) return false;
    for(int i = 0; i < s->transitionList->length; i++){
        if(!addTransition(d, &s->transitionList->data[i])){
            return false;
        }
    }
    return true;
}

bool SetToD(StateList* sl, DFAState* d, Set* set){
    d->stateNumber = cloneSet(set);
    if (!d->stateNumber){
        perror("null StateNumber");
        return false;
    }
    for(int i = 0; i < d->stateNumber->length; i++){
        //find NFA equivalent for each statenumber in d
        State* equivalent = NULL;
        for(int j = 0; j < sl->length; j++){
            if(sl->data[j].stateNumber == d->stateNumber->data[i]){
                equivalent = &sl->data[j];
                break;
            }
        }
        if(equivalent){
            if(!NToD(equivalent, d)){
                return false;
            }
        } else return false;
    }
    return true;
}
//set
DFAStateList* subsetConstruction(StateList *st){
    if(!st || st->length <=0){
        perror("Statelist received by subset constructor is NULL || length <=0");
        return NULL;
    }

    DFAStateList *DFAList = createDFAList();
    Sets* workingSet = createSets();
    Set* start = createSet();

    if(!DFAList || !workingSet || !start){
        perror("Subset construction prerequisite allocation fail");
        freeDFAList(DFAList);
        freeSets(workingSet);
        freeSet(start);
        return NULL;
    }
    //include start state to set
    if(!addToSet(start, st->data[0].stateNumber)){
        perror("failed to add to start set");
        freeDFAList(DFAList);
        freeSets(workingSet);
        freeSet(start);
        return NULL;
    }
    if(!addToSets(workingSet, start)){
        perror("failed to add start to workingset");
        freeDFAList(DFAList);
        freeSets(workingSet);
        freeSet(start);
        return NULL;
    }
    start = NULL;
    int processed = 0;
    while (processed < workingSet->length)
    {  
        //pick the next unprocessed state
        if(processed >= DFAList->maxsize){
            if(!expandDFAList(DFAList)){
                perror("DFAList expansion fail");
                freeDFAList(DFAList);
                freeSets(workingSet);
                return NULL;
            }
        };
        DFAState* workingDFAState = &DFAList->data[processed];
        if(!SetToD(st, workingDFAState, workingSet->data[processed])){
            perror("Set to DFA conversion fail");
            freeDFAList(DFAList);
            freeSets(workingSet);
            return NULL;
        }
        for(int i = 0; i < 256; i++){
            //add all sets not included in working set (by value)
            if(workingDFAState->transitionNumbers[i]){
                Set* clone = cloneSet(workingDFAState->transitionNumbers[i]);
                if(!addToSets(workingSet, clone)){                    
                    perror("clone not added to sets");
                    freeSet(clone); 
                    freeDFAList(DFAList);
                    freeSets(workingSet);
                    return NULL;
                }
            }
        }
        DFAList->length++;
        processed++;
    }
    //link all **next step, useful for traversal
    for(int i = 0; i < DFAList->length; i++){
        DFAState* workingState = &DFAList->data[i];
        workingState->type = resolveTypePriority(workingState->typeSet);
        for(int j = 0; j < 256; j++){
            if(!workingState->transitionNumbers[j]) continue;
            //find the state in DFAList corresponding to the transitionNumber
            //link to **next 
            for(int k = 0; k < DFAList->length; k++){
                if(isEqSets(workingState->transitionNumbers[j], DFAList->data[k].stateNumber)){
                    workingState->next[j] = &DFAList->data[k];
                    break;
                };
            }
        }
    }
    printf("\nlengths: %d:%d", DFAList->length, DFAList->maxsize);
    freeSets(workingSet);
    return DFAList;
}

void dumpDFAList(DFAStateList *dsl, FILE *f){
    if (!dsl || !f) return;
    
    for(int i = 0; i < dsl->length; i++){
        DFAState *printDFA = &dsl->data[i];
        fprintf(f, "StateNumber: [");
        for(int j = 0; j < printDFA -> stateNumber -> length; j++){
            fprintf(f, "%d%s", printDFA->stateNumber->data[j] , j < printDFA->stateNumber->length - 1 ? ", " : "]\n");
        }
        fprintf(f, "StateType: %s,\nStateConnections: \n[", stateNames[printDFA->type]);
        int printed = 0;
        for(int j = 0; j < 256; j++){
            DFAState* next = printDFA -> next[j];
            if(next){
                if(printed%10 == 0) {
                    printf("%d", printed);
                    fprintf(f, "\n\t");
                    printed = 0;
                };
                unsigned char val[2] = {0};
                unsigned char* c;
                if(!inSigma(j)){
                    c = (unsigned char*) "Invalid";
                }
                else{
                    val[0] = (unsigned char) j;
                    c = val;
                }
                fprintf(f, "%s : [", c);
                for(int k = 0; k < next->stateNumber->length; k ++){
                    fprintf(f, "%d%s", next->stateNumber->data[k] , k < next->stateNumber->length - 1 ? ", " : "], ");
                }
                printed++;
            }
        }
        fprintf(f, "\n]\n\n");
    }
}