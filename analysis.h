#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <stdio.h>
#include <stdbool.h>
#include "state.h"
#include "symtbl.h"

#define TRENSLATE_EXT ".trn"

/**
 * Validates that the source file has the unique Trenslate file extension (.trn).
 */
bool validateExtension(const char *filename, const char *expectedExt);

/**
 * Executes the Analysis Phase:
 * 1. Validates the source program file extension (.trn).
 * 2. Reads the source program line-by-line using a buffer.
 * 3. Traverses the DFA character-by-character purely using DFAState->next pointers.
 * 4. Recognizes tokens when reaching NO_TRANSITION based on DFA accepting states.
 * 5. Emits recognized tokens into the symbol table without using any hardcoded strcmp.
 * 
 * Returns the populated SymTBL pointer, or NULL on error.
 */
SymTBL* runAnalysis(DFAStateList *dsl, const char *sourceFilename);

/**
 * Formats and prints the symbol table to the given output stream.
 */
void printSymbolTable(SymTBL *st, FILE *out);

#endif
