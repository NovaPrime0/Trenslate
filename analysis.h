#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <stdio.h>
#include <stdbool.h>
#include "state.h"
#include "symtbl.h"

#define TRENSLATE_EXT ".tren"

/**
 * Validates that the source file has the unique Trenslate file extension (.tren).
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
 * Structure representing an invalid lexical token detected during analysis,
 * along with the specific language rule from the PUP specification document it violated.
 */
typedef struct {
    int index;
    int line;
    int col;
    char *lexeme;
    char *rule;
    char *description;
} InvalidToken;

typedef struct {
    InvalidToken *data;
    int count;
    int capacity;
} InvalidTokenTable;

InvalidTokenTable* createInvalidTable(void);
void addInvalidToken(InvalidTokenTable *table, int line, int col, const char *lexeme, const char *rule, const char *desc);
void freeInvalidTable(InvalidTokenTable *table);
void printInvalidTable(InvalidTokenTable *table, const char *sourceFile, FILE *out);

/**
 * Formats and prints the symbol table to the given output stream,
 * cleanly separating valid language tokens from invalid lexical entries.
 */
void printSymbolTable(SymTBL *st, FILE *out);

/**
 * Simulates DFA character-by-character transition for an input word,
 * displaying current state, token, line, accumulated lexeme, and token changes.
 */
void simulateDFAInput(DFAStateList *dsl, const char *inputStr, FILE *out);

#endif

