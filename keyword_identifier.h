#ifndef KEYWORD_IDENTIFIER_H
#define KEYWORD_IDENTIFIER_H

#include <stdio.h>
#include <stdbool.h>
#include "state.h"
#include "enums.h"

// Keyword validation using DFA state types
typedef struct {
    const char *keyword;
    StateType expectedType;
    const char *description;
} DFAKeywordTest;

// Checks if state type is a keyword (S_FOR to S_IMPORT, or S_CONST)
bool isDFAKeywordType(StateType type);

// Validates a single keyword by feeding characters through the DFA
bool validateKeywordThroughDFA(DFAStateList *dsl, const char *keyword, StateType expectedType);

// Runs tests on all keywords against the DFA
bool validateAllKeywordsDFA(DFAStateList *dsl, FILE *outStream);

// Identifier tracking (name, value, line, scope, flags)
typedef struct {
    char *variableName;   // Variable or function name
    char *value;          // Assigned value string (implied type, e.g. "15", "Recto")
    int line;             // Declaration line number
    int lastUsedLine;     // Line of last reference or update
    char *scope;          // Scope name ("global" or enclosing function)
    bool isConstant;      // Declared as constant ('laging')
    bool isFunction;      // Declared as function ('route')
    int refCount;         // Reference / usage count
} IdentifierRecord;

typedef struct {
    IdentifierRecord *records;
    int count;
    int capacity;
} IdentifierTable;

IdentifierTable* createIdentifierTable(void);

// Inserts a new identifier entry into the table
bool insertIdentifierEntry(IdentifierTable *table,
                           const char *variableName,
                           const char *value,
                           int line,
                           const char *scope,
                           bool isConstant,
                           bool isFunction);

// Updates an existing identifier's value and last-used line
bool updateIdentifierEntry(IdentifierTable *table,
                           const char *variableName,
                           const char *newValue,
                           int line,
                           const char *scope);

// Finds an identifier entry by name and scope
IdentifierRecord* findIdentifierEntry(IdentifierTable *table, const char *variableName, const char *scope);

// Increments refCount for an identifier
void recordIdentifierUsage(IdentifierTable *table, const char *variableName, int line, const char *scope);

// Scans source file using DFA to extract identifiers and their values
IdentifierTable* collectIdentifiersDFA(DFAStateList *dsl, const char *sourceFilename);

// Prints the formatted identifier table
void printIdentifierTable(IdentifierTable *table, FILE *stream);

// Frees memory used by the identifier table
void freeIdentifierTable(IdentifierTable *table);

#endif
