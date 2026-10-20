#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "state.h"
#include "symtbl.h"


/* CURRENT PLANNED FLOW:
    1. Prep phase (Make states list and load all from lexStates.txt)
    2. Analysis phase (Load source program and traverse states char-by-char, may need to revise state machine to make this work)
        - all states that started with an alphabet (and still match identifier logic) will resolve to an identifier 
        when keyword chain no longer matches
        - something something semi-dfa solution so we dont have to track all possible state transitions for a symbol
    3. Emit token on NO_TRANSITION (emit to sym tbl)
    4. Output : symbol table (extension to update later)
*/

//const indices for important states for reference later
//structs and prototypes
int main(){

    //Preparation phase
    //change print format to make it more understandable
    StateList *st = loadList("Misc/newStates.txt");
    FILE *dump = fopen("Test/dump.txt", "w+");
    //debug line for dumping state data into dump.txt
    for(int i = 0; i < st->length; i++) printState(&st->data[i], dump); 
    freeList(st);

    //analysis phase

    return 0;
}



