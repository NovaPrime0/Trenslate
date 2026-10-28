#ifndef KEYWORD_IDENTIFIER_H
#define KEYWORD_IDENTIFIER_H

#include <stdio.h>
#include <stdbool.h>
#include "state.h"
#include "enums.h"

/* ========================================================================= */
/* 1. KEYWORD VALIDATION MODULE (Based purely on DFA/NFA state types)        */
/* ========================================================================= */

typedef struct {
    const char *keyword;
    StateType expectedType;
    const char *description;
} DFAKeywordTest;

/**
 * Returns true if a StateType belongs to the DFA's Priority 1 Keyword range
 * (S_FOR through S_IMPORT) or S_CONST, purely checking enum values without strcmp.
 */
bool isDFAKeywordType(StateType type);

/**
 * Traverses a keyword string character-by-character through the loaded DFA
 * (using DFAState->next pointers) and checks if the final state matches expectedType.
 */
bool validateKeywordThroughDFA(DFAStateList *dsl, const char *keyword, StateType expectedType);

/**
 * Runs a complete verification suite against all Trenslate keywords using the DFA
 * as the basis, reporting whether each keyword correctly transitions to its
 * designated DFA accepting state.
 */
bool validateAllKeywordsDFA(DFAStateList *dsl, FILE *outStream);


/* ========================================================================= */
/* 2. IDENTIFIER SYMBOL TABLE MODULE                                         */
/*    (Specifically for identifiers, with variableName, value, line, scope)  */
/*    Weakly typed: type is implied by value                                 */
/* ========================================================================= */

typedef struct {
    char *variableName;   // Name of the identifier/variable
    char *value;          // String representation of value (implied type, e.g. "15", "Recto", "exit")
    int line;             // Line number of declaration
    int lastUsedLine;     // Line number of last reference or update
    char *scope;          // Scope name ("global", or enclosing function name)
    bool isConstant;      // True if declared under S_CONST ('laging')
    bool isFunction;      // True if declared under S_FUNCTION ('route')
    int refCount;         // Usage count (useful for semantic phase unused-variable checks)
} IdentifierRecord;

typedef struct {
    IdentifierRecord *records;
    int count;
    int capacity;
} IdentifierTable;

/**
 * Creates and initializes an Identifier Symbol Table.
 */
IdentifierTable* createIdentifierTable();

/**
 * Inserts an identifier with its assigned value, line, scope, and flags into the table.
 * If already declared in the same scope, returns false.
 */
bool insertIdentifierEntry(IdentifierTable *table,
                           const char *variableName,
                           const char *value,
                           int line,
                           const char *scope,
                           bool isConstant,
                           bool isFunction);

/**
 * Updates the value and last-used line of an existing identifier.
 * Fails if the identifier is marked as constant.
 */
bool updateIdentifierEntry(IdentifierTable *table,
                           const char *variableName,
                           const char *newValue,
                           int line,
                           const char *scope);

/**
 * Looks up an identifier by variableName and scope.
 */
IdentifierRecord* findIdentifierEntry(IdentifierTable *table, const char *variableName, const char *scope);

/**
 * Records a usage/reference of an identifier, incrementing its refCount.
 */
void recordIdentifierUsage(IdentifierTable *table, const char *variableName, int line, const char *scope);

/**
 * Traverses a source file using the loaded DFA (char-by-char) and extracts all identifiers
 * and their assigned values into an IdentifierTable based strictly on DFA StateTypes.
 */
IdentifierTable* collectIdentifiersDFA(DFAStateList *dsl, const char *sourceFilename);

/**
 * Prints the formatted Identifier Symbol Table with columns:
 * Index, Variable Name, Value, Implied Type, Line, Scope, Const?, Refs.
 */
void printIdentifierTable(IdentifierTable *table, FILE *stream);

/**
 * Frees memory allocated for the IdentifierTable.
 */
void freeIdentifierTable(IdentifierTable *table);

#endif
