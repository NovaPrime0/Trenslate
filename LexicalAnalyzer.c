#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "state.h"
#include "symtbl.h"
#include "analysis.h"
#include "keyword_identifier.h"

/* CURRENT PLANNED FLOW:
    1. Prep phase (Make states list and load all from lexStates.txt)
    2. Analysis phase (Load source program and traverse states char-by-char, line-by-line)
        - all states that started with an alphabet (and still match identifier logic) will resolve to an identifier 
        when keyword chain no longer matches
        - something something semi-dfa solution so we dont have to track all possible state transitions for a symbol
    3. Emit token on NO_TRANSITION (emit to sym tbl)
    4. Output : symbol table (extension to update later)
*/

//const indices for important states for reference later
//structs and prototypes
int main(int argc, char *argv[]){

    //Preparation phase
    //change print format to make it more understandable
    StateList *st = loadList("Misc/newStates.txt");
    DFAStateList *dst = subsetConstruction(st);
    FILE *NFAdump = fopen("Test/NFAdump.txt", "w+");
    FILE *DFAdump = fopen("Test/DFAdump.txt", "w+");
    
    if (NFAdump) {
        for(int i = 0; i < st->length; i++) printState(&st->data[i], NFAdump); 
        fclose(NFAdump);
    }
    if (DFAdump) {
        dumpDFAList(dst, DFAdump);
        fclose(DFAdump);
    }
    freeList(st);

    // Keyword DFA verification suite
    validateAllKeywordsDFA(dst, stdout);

    //analysis phase
    const char *sourceFile = (argc > 1) ? argv[1] : "sample.trn";
    SymTBL *symtbl = runAnalysis(dst, sourceFile);
    if (symtbl) {
        printSymbolTable(symtbl, stdout);
    }

    // Collect and print identifier semantic table
    IdentifierTable *idTable = collectIdentifiersDFA(dst, sourceFile);
    if (idTable) {
        printIdentifierTable(idTable, stdout);
        freeIdentifierTable(idTable);
    }

    return 0;
}
