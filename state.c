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
    for(int i = 0; i < 256; i++){
        s -> transition[i] = i < 128 ? NO_TRANSITION : BAD_SYMBOL;
    }
    s -> type = type;
    return s;
}
int read(State* s, char symbol){
    return s->transition[(unsigned char)symbol];
}
void printState(State *s, FILE* fpt) {
    if(!fpt) printf("not found");
    fprintf(fpt,"State: %d, transition: [", s->stateNumber);
    for(int i = 0; i < 256; i++){
        if(i % 25 == 0 && i > 0) fprintf(fpt,"\n");
        fprintf(fpt,"%d%c ", s->transition[i], (i==255)? ']':',');   
    }
    fprintf(fpt,"StateType: %d\n", s->type);
}
static void resetState(State* s){
    s->stateNumber = -1;
    for(int i = 0; i < 256; i++){
        s->transition[i] = i < 128 ? NO_TRANSITION : BAD_SYMBOL;
    }
    s->type = S_INTERMEDIARY;
}
//statelist
static void expandList(StateList* sl){
    sl -> maxsize *= 2;
    sl -> data = realloc(sl->data, sizeof(State) * sl -> maxsize);
    printf("Expanded List to %d\n",sl->maxsize);
    for(int i = sl -> length; i < sl->maxsize; i++){
        printf("\treset %d\n",i);
        resetState(&sl->data[i]);
    }
}
static int resolveType(char* typestring){
    StateType lim = S_INTERMEDIARY;
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

    printf("Loading file...\n\n");
    FILE* f = fopen(file, "r");
    if(f == NULL) return NULL;
    printf("===================================\n\n");
    printf("\tLoaded file\n\n");
    printf("===================================\n\n");
    StateList *sl = createList();
    char transitionbuffer[256];
    char symbolsbuffer[256];
    char typebuffer[64];
    while(fscanf(f,
        "State: %d, Transitions: %255[^,], Symbols: %255[^,], StateType: %63[^,],\n",
        &sl->data[sl->length].stateNumber, transitionbuffer, symbolsbuffer, typebuffer)==4){
        printf("%-12s %d\n%-20s %s\n%-20s %s\n","State:", sl->data[sl->length].stateNumber, "Full Symbol List:", symbolsbuffer, "Full Dest list:", transitionbuffer);
        char *symbolsSave;
        char *transitionSplit = strtok(transitionbuffer, ":");
        char *symbolSplit = strtok_r(symbolsbuffer, ":", &symbolsSave);
        while(transitionSplit != NULL && symbolSplit != NULL){
            
            printf("\n\nSymbol: %s, Dest: %d\n\n", symbolSplit, atoi(transitionSplit));
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
    printf("\n\n\n%d:%d\n\n\n", sl->length, sl->maxsize);
    fclose(f);
    return sl;
}
void addTransition(State* s, int transition, char* transitionSymbol){
    if(transitionSymbol == NULL) return;
    printf("\tReceived: %s\n\n", transitionSymbol); 
    if(strlen(transitionSymbol) > 2){
        if(strcmp("ALPHA", transitionSymbol) == 0) {
            for(int i = 'A'; i <= 'Z'; i++) {
                printf("\t\tmodified %c:%d\n", i, transition);
                s->transition[i] = transition;
            }
        }
        else if(strcmp("alpha", transitionSymbol) == 0) {
            for(int i = 'a'; i <= 'z'; i++) {
                printf("\t\tmodified %c:%d\n", i, transition);
                s->transition[i] = transition;
            }
        }
        else if(strcmp("num", transitionSymbol) == 0) {
            for(int i = '0'; i <= '9'; i++) {
                printf("\t\tmodified %c:%d\n", i, transition);
                s->transition[i] = transition;
            }
        }
        else if(strcmp("sym", transitionSymbol) == 0) {
            for(int i = 0; i < SYMBOLCOUNT; i++) {
                printf("\t\tmodified %c:%d\n", SYMBOLS[i], transition);
                s->transition[(unsigned char) SYMBOLS[i]] = transition;
            }
        }
        else if(strcmp("whitespace", transitionSymbol) == 0) {
            for(int i = 0; i < WHITESPACECOUNT; i++) {
                printf("\t\tmodified %c:%d\n", WHITESPACES[i] ,i);
                s->transition[(unsigned char) WHITESPACES[i]] = transition;
            }
        }
    } else if (strlen(transitionSymbol) == 2 && transitionSymbol[0] == '\\'){
        printf("\t\tmodified %c:%d\n", (transitionSymbol[1]=='n') ? '\n': '\t', transition);
        s -> transition[(transitionSymbol[1]=='n') ? '\n': '\t'] = transition;
    } else if (strlen(transitionSymbol) == 1) {
        printf("\t\tmodified %c:%d\n",(unsigned char) *transitionSymbol, transition);
        s -> transition[(unsigned char) *transitionSymbol] = transition;
    } else {
        return;
    }
}
void freeList(StateList* sl){
    free(sl->data);
    free(sl);
}


