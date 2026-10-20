#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "state.h"
#include "enums.h"

//state
State* createState(int stateNumber, StateType type){   
    State* s = malloc(sizeof(State));
    s -> stateNumber = stateNumber; 
    s -> next = malloc(sizeof(State)*256);
    for(int i = 0; i < 256; i++){
        s -> transition[i] = i < 128 ? NO_TRANSITION : BAD_SYMBOL;
        s -> next[i] = NULL;
    }
    s -> type = type;
    return s;
}

int read(State* s, char symbol){
    return s->transition[(unsigned char)symbol];
}

void printState(State *s, FILE* fpt) {
    if(!fpt || !s) {
        printf("[state.c: %d]: state or fpt not found",__LINE__);
        return;
    };
    fprintf(fpt,"State: %d, transition: \n[", s->stateNumber);
    for(int i = 0; i < 256; i++){
        if(i % 25 == 0 && i > 0) fprintf(fpt,"\n");
        fprintf(fpt,"%c:%d%c ", (char) (i > 32 && i <127) ? i : ' ',  s->transition[i], (i==255)? ']':',');   
    }
    fprintf(fpt,"StateType: %d\n", s->type);
}

static void resetState(State* s){
    s->stateNumber = -1;
    for(int i = 0; i < 256; i++){
        s->transition[i] = i < 128 ? NO_TRANSITION : BAD_SYMBOL;
    }
    s->type = INTERMEDIARY;
}
static void attachTransition(State* s, StateList* sl){
    if(!s) return;
    for (int i = 0; i < 256; i++){
        if(s->transition[i] >= 0){
            s->next[i] = &sl->data[s->transition[i]];
        }
    }
}

//statelist
static void expandList(StateList* sl){
    sl -> maxsize *= 2;
    sl -> data = realloc(sl->data, sizeof(State) * sl -> maxsize);
    printf("[state.c: %d]: Expanded List to %d\n",__LINE__,sl->maxsize);
    for(int i = sl -> length; i < sl->maxsize; i++){
        printf("\treset %d\n",i);
        resetState(&sl->data[i]);
    }
}
static int resolveType(char* typestring){
    StateType lim = INTERMEDIARY;
    for(int i = 0; i <= lim; i++){
        if(strcmp(stateNames[i], typestring) == 0) return i;
    }
    return -1;
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
    char typebuffer[64];
    while(fscanf(f,
        "State: %d, Transitions: %255[^,], Symbols: %255[^,], StateType: %63[^,],\n",
        &sl->data[sl->length].stateNumber, transitionbuffer, symbolsbuffer, typebuffer) == 4){
        printf("[state.c: %d]: %-12s %d\n\t%-20s %s\n\t%-20s %s\n",__LINE__, "State:", sl->data[sl->length].stateNumber, "Full Symbol List:", symbolsbuffer, "Full Dest list:", transitionbuffer);
        char *symbolsSave;
        char *transitionSplit = strtok(transitionbuffer, ":");
        char *symbolSplit = strtok_r(symbolsbuffer, ":", &symbolsSave);
        while(transitionSplit != NULL && symbolSplit != NULL){
            
            printf("\n\n[state.c: %d]: Symbol: %s, Dest: %d\n\n", __LINE__, symbolSplit, atoi(transitionSplit));
            //anything beyond a special character, "\n" is "\\n"
            if(strlen(symbolSplit) > 2){
                char *catSave;
                char *categorySplit = strtok_r(symbolSplit, "|", &catSave); 
                while (categorySplit != NULL){
                    addTransition(&sl->data[sl->length], atoi(transitionSplit), categorySplit);
                    categorySplit = strtok_r(NULL, "|", &catSave);
                }
            } else
                addTransition(&sl->data[sl->length], atoi(transitionSplit), symbolSplit);
            transitionSplit = strtok(NULL, ":");
            symbolSplit = strtok_r(NULL, ":", &symbolsSave);
        }
        sl->data[sl->length].type = resolveType(typebuffer);
        sl->length++;
        if(sl->length >= sl->maxsize){
            expandList(sl);
        }
    }
    printf("\n\n[state.c: %d]: list size:maxsize = %d:%d\n\n",__LINE__, sl->length, sl->maxsize);
    for(int i = 0; i < sl->length; i++)
    fclose(f);
    return sl;
}

//decision tree
void addTransition(State* s, int transition, char* transitionSymbol){
    if(transitionSymbol == NULL) return;
    printf("\n[state.c: %d]:Received: %s\n",__LINE__, transitionSymbol); 
    if(strlen(transitionSymbol) > 2){
        if(strcmp("ALPHA", transitionSymbol) == 0) {
            for(int i = 'A'; i <= 'Z'; i++) {
                printf("[state.c: %d]:\tmodified %c:%d\n",__LINE__, i, transition);
                s->transition[i] = transition;
            }
        }
        else if(strcmp("alpha", transitionSymbol) == 0) {
            for(int i = 'a'; i <= 'z'; i++) {
                printf("[state.c: %d]:\tmodified %c:%d\n",__LINE__, i, transition);
                s->transition[i] = transition;
            }
        }
        else if(strcmp("num", transitionSymbol) == 0) {
            for(int i = '0'; i <= '9'; i++) {
                printf("[state.c: %d]:\tmodified %c:%d\n",__LINE__, i, transition);
                s->transition[i] = transition;
            }
        }
        else if(strcmp("sym", transitionSymbol) == 0) {
            for(int i = 0; i < SYMBOLCOUNT; i++) {
                printf("[state.c: %d]:\tmodified %c:%d\n",__LINE__, SYMBOLS[i], transition);
                s->transition[(unsigned char) SYMBOLS[i]] = transition;
            }
        }
        else if(strcmp("whitespace", transitionSymbol) == 0) {
            for(int i = 0; i < WHITESPACECOUNT; i++) {
                printf("[state.c: %d]:\tmodified %c:%d\n",__LINE__, WHITESPACES[i] ,transition);
                s->transition[(unsigned char) WHITESPACES[i]] = transition;
            }
        }
        else if(strcmp("comma", transitionSymbol) == 0) {
            printf("[state.c: %d]:\tmodified %c:%d\n",__LINE__, ',' , transition);
            s->transition[(unsigned char) ','] = transition; 
        }
    } else if (strlen(transitionSymbol) == 2 && transitionSymbol[0] == '\\'){
        printf("[state.c: %d]:\tmodified %c:%d\n",__LINE__, (transitionSymbol[1]=='n') ? '\n': '\t', transition);
        s -> transition[(transitionSymbol[1]=='n') ? '\n': '\t'] = transition;
    } else if (strlen(transitionSymbol) == 1) {
        printf("[state.c: %d]:\tmodified %c:%d\n",__LINE__, (unsigned char) *transitionSymbol, transition);
        s -> transition[(unsigned char) *transitionSymbol] = transition;
    } else {
        return;
    }
}
void freeList(StateList* sl){
    free(sl->data);
    free(sl);
}


