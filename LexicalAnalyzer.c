#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "state.h"
#include "symtbl.h"


/*
CURRENT PLANNED FLOW:
    1. Prep phase (Make states list and load all from lexStates.txt)
    2. Analysis phase (Load source program and traverse states char-by-char, may need to revise states to make this work)
    3. Emit token on NO_TRANSITION (emit to sym tbl)
    4. Output : symbol table (extension to update later)
*/

//9/29/26: basic ds for lexical analyzer
//9/30/26: separation to c and h files + makefile, completion of lexStates.txt

//const indices for important states for reference later
//structs and prototypes
int main(){

    //Preparation phase
    StateList *st = loadList("Misc/lexStates.txt");
    FILE *dump = fopen("dump.txt", "w+");
    for(int i = 0; i < st->length; i++) printState(&st->data[i], dump);
    freeList(st);
    return 0;
}



