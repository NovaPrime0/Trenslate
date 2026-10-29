#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <stdio.h>
#include <stdbool.h>
#include "state.h"
#include "symtbl.h"

#define TRENSLATE_EXT ".tren"

// Checks if the source file has the .tren extension
bool validateExtension(const char *filename, const char *expectedExt);

// Runs lexical analysis: reads source file, traverses the DFA char-by-char,
// and emits recognized tokens into the symbol table.
SymTBL* runAnalysis(DFAStateList *dsl, const char *sourceFilename);

// Structure for tracking invalid tokens and the rule they violated
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

// Formats and prints the symbol table, separating valid tokens from invalid ones
void printSymbolTable(SymTBL *st, FILE *out);

// Simulates DFA transitions character-by-character for a single input string
void simulateDFAInput(DFAStateList *dsl, const char *inputStr, FILE *out);

#endif

