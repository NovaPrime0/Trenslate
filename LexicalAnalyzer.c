#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "state.h"
#include "symtbl.h"
#include "analysis.h"
#include "keyword_identifier.h"

/*
 * Trenslate Lexical Analyzer
 * Main program that loads the state machine, builds the DFA,
 * and analyzes .tren source files to generate the symbol table and reports.
 */

int main(int argc, char *argv[]){

    // Load states and construct DFA
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

    // Optional single-word transition simulation mode: ./main.exe --simulate <word>
    if (argc > 2 && strcmp(argv[1], "--simulate") == 0) {
        simulateDFAInput(dst, argv[2], stdout);
        return 0;
    }

    // Interactive demonstration: Compare Identifier vs Keyword transition
    printf("\n======================================================================\n");
    printf("             DFA TRANSITION DEMONSTRATION: IDENTIFIER vs KEYWORD      \n");
    printf("======================================================================\n");
    simulateDFAInput(dst, "bagahe", stdout);   // Turns from IDENTIFIER -> VARIABLE (keyword)
    simulateDFAInput(dst, "pamasahe", stdout); // Starts with 'p', remains IDENTIFIER throughout

    // Analysis phase
    const char *sourceFile = (argc > 1) ? argv[1] : "sample.tren";
    SymTBL *symtbl = runAnalysis(dst, sourceFile);
    if (symtbl) {
        printSymbolTable(symtbl, stdout);

        // Save symbol table to Output folder
        FILE *symFile = fopen("Output/Symbol Table.txt", "w");
        if (symFile) {
            printSymbolTable(symtbl, symFile);
            fclose(symFile);
            printf("[Output File Generated]: 'Output/Symbol Table.txt'\n\n");
        }
    }

    // Collect and print identifier semantic table
    IdentifierTable *idTable = collectIdentifiersDFA(dst, sourceFile);
    if (idTable) {
        printIdentifierTable(idTable, stdout);
        freeIdentifierTable(idTable);
    }

    return 0;
}
