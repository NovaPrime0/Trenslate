#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "symtbl.h"

SymTBL* createTable(){
    SymTBL *st = malloc(sizeof(SymTBL));
    st -> data = malloc(sizeof(Symbol)*2);
    st -> curIdx = 0;
    st -> maxSize = 2;
    return st;
}

void addSymbol(SymTBL *st, Symbol *s){
    if (st -> curIdx + 1 >= st -> maxSize){
        st -> maxSize *= 2;
        st -> data = realloc(st->data, st->maxSize * sizeof(Symbol));
    };
    st->data[st->curIdx++] = *s;
}

bool contains(SymTBL *st, Symbol *s){
    if(st == NULL || s == NULL) return false;
    
    for(int i = 0; i <= st->curIdx; i++){
        Symbol cur = st->data[i];
        if(strcmp(s->identifier, cur.identifier) == 0){
            return true;
        }
    };
    return false;
}