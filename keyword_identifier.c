#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "keyword_identifier.h"
#include "state.h"
#include "enums.h"

// Keyword test list mapped to expected DFA state types
static const DFAKeywordTest KEYWORD_TEST_SET[] = {
    // Control flow and program structure
    { "byahe",    S_PROG_MAIN, "Program entry point" },
    { "bagahe",   S_VARIABLE,  "Variable declaration keyword" },
    { "route",    S_FUNCTION,  "Function declaration keyword" },
    { "sakali",   S_IF,        "Conditional if keyword" },
    { "liko",     S_ELSE,      "Alternative branch else keyword" },
    { "hakbang",  S_FOR,       "Counting loop for keyword" },
    { "bawat",    S_FOREACH,   "Collection for-each keyword" },
    { "habang",   S_WHILE,     "While loop keyword" },
    { "hinto",    S_BREAK,     "Break loop early keyword" },
    { "baba",     S_RETURN,    "Return value keyword" },
    { "dulo",     S_PROG_END,  "Program termination keyword" },
    { "exit",     S_T_NULL,    "Null value keyword" },
    { "daan",     S_IMPORT,    "Module import directive" },
    { "hanggang", S_For_LIM,   "Counting loop bound limit" },
    { "tuloy",    S_CONTINUE,  "Continue loop early keyword" },
    
    // Constant & Boolean keywords
    { "laging",   S_CONST,     "Constant/immutable declaration" },
    { "TRUE",     S_TRUE,      "Boolean literal true" },
    { "FALSE",    S_FALSE,     "Boolean literal false" },
    
    // Noise words
    { "ng",       S_NOISE,     "I/O readability particle" },
    { "sa",       S_NOISE,     "Preposition in loops" },
    { "mula",     S_NOISE,     "Starting bound particle" },
    { "ay",       S_NOISE,     "Assertion sugar particle" },
    { "na",       S_NOISE,     "Conditional descriptor particle" }
};

static const int KEYWORD_TEST_COUNT = sizeof(KEYWORD_TEST_SET) / sizeof(KEYWORD_TEST_SET[0]);

// Checks if a StateType belongs to Priority 1 Keyword range or S_CONST (no strcmp used)
bool isDFAKeywordType(StateType type) {
    if (type >= S_FOR && type <= S_IMPORT) {
        return true;
    }
    if (type == S_CONST) {
        return true;
    }
    return false;
}

// Validates a keyword string by feeding characters one-by-one into the DFA State machine
bool validateKeywordThroughDFA(DFAStateList *dsl, const char *keyword, StateType expectedType) {
    if (!dsl || !keyword || dsl->length <= 0) return false;
    
    DFAState *current = &dsl->data[0]; // Start DFA state
    size_t len = strlen(keyword);
    
    for (size_t i = 0; i < len; i++) {
        DFAState *next = current->next[(unsigned char)keyword[i]];
        if (!next) {
            // DFA has no transition for this character: validation fails
            return false;
        }
        current = next;
    }
    
    // The final DFA state's type must match the expected keyword StateType exactly
    return (current->type == expectedType);
}

// Runs validation report for all keywords using the loaded DFA as the basis
bool validateAllKeywordsDFA(DFAStateList *dsl, FILE *outStream) {
    if (!dsl || !outStream) return false;
    
    fprintf(outStream, "======================================================================\n");
    fprintf(outStream, "           TRENSLATE KEYWORD TOKEN DFA VALIDATION REPORT              \n");
    fprintf(outStream, "======================================================================\n");
    fprintf(outStream, " %-12s | %-16s | %-16s | %s\n", "Keyword", "Expected State", "DFA State", "Status");
    fprintf(outStream, "----------------------------------------------------------------------\n");
    
    int passed = 0;
    
    for (int i = 0; i < KEYWORD_TEST_COUNT; i++) {
        const char *kw = KEYWORD_TEST_SET[i].keyword;
        StateType expected = KEYWORD_TEST_SET[i].expectedType;
        
        // Traverse char-by-char through DFA
        DFAState *current = &dsl->data[0];
        bool broken = false;
        size_t len = strlen(kw);
        
        for (size_t j = 0; j < len; j++) {
            DFAState *next = current->next[(unsigned char)kw[j]];
            if (!next) {
                broken = true;
                break;
            }
            current = next;
        }
        
        StateType actualType = broken ? INTERMEDIARY : current->type;
        bool ok = (!broken && (actualType == expected));
        if (ok) passed++;
        
        fprintf(outStream, " %-12s | %-16s | %-16s | [%s]\n",
                kw,
                stateNames[expected],
                stateNames[actualType],
                ok ? "PASS" : "FAIL");
    }
    
    fprintf(outStream, "======================================================================\n");
    fprintf(outStream, " Summary: %d / %d Keywords Validated Successfully via DFA (%s)\n",
            passed, KEYWORD_TEST_COUNT,
            (passed == KEYWORD_TEST_COUNT) ? "ALL VALID" : "CHECK MISMATCHES");
    fprintf(outStream, "======================================================================\n\n");
    
    return (passed == KEYWORD_TEST_COUNT);
}


// Identifier symbol table implementation

// Simple type inference from value string
static const char* inferImpliedType(const char *val) {
    if (!val || strlen(val) == 0) return "uninitialized";
    if (val[0] == '"' || val[0] == '\'') return "string";
    
    // Check null
    if (val[0] == 'e' && val[1] == 'x' && val[2] == 'i' && val[3] == 't' && val[4] == '\0') {
        return "null";
    }
    // Check boolean
    if ((val[0] == 'T' && val[1] == 'R' && val[2] == 'U' && val[3] == 'E' && val[4] == '\0') ||
        (val[0] == 'F' && val[1] == 'A' && val[2] == 'L' && val[3] == 'S' && val[4] == 'E' && val[5] == '\0')) {
        return "boolean";
    }
    // Check function placeholder
    if (val[0] == '[' && val[1] == 'f') return "function";
    
    // Check numeric integer or decimal
    char *endPtr;
    strtol(val, &endPtr, 10);
    if (*endPtr == '\0') return "integer";
    
    strtod(val, &endPtr);
    if (*endPtr == '\0') return "decimal";
    
    return "expression";
}

IdentifierTable* createIdentifierTable() {
    IdentifierTable *table = malloc(sizeof(IdentifierTable));
    if (!table) return NULL;
    table->capacity = 16;
    table->count = 0;
    table->records = malloc(sizeof(IdentifierRecord) * table->capacity);
    return table;
}

IdentifierRecord* findIdentifierEntry(IdentifierTable *table, const char *variableName, const char *scope) {
    if (!table || !variableName) return NULL;
    const char *targetScope = scope ? scope : "global";
    
    for (int i = 0; i < table->count; i++) {
        if (table->records[i].variableName && strcmp(table->records[i].variableName, variableName) == 0) {
            if (table->records[i].scope && (strcmp(table->records[i].scope, targetScope) == 0 ||
                strcmp(table->records[i].scope, "global") == 0)) {
                return &table->records[i];
            }
        }
    }
    return NULL;
}

bool insertIdentifierEntry(IdentifierTable *table,
                           const char *variableName,
                           const char *value,
                           int line,
                           const char *scope,
                           bool isConstant,
                           bool isFunction) {
    if (!table || !variableName) return false;
    
    const char *effectiveScope = scope ? scope : "global";
    
    // Check uniqueness within the same scope
    for (int i = 0; i < table->count; i++) {
        if (table->records[i].variableName &&
            strcmp(table->records[i].variableName, variableName) == 0 &&
            table->records[i].scope &&
            strcmp(table->records[i].scope, effectiveScope) == 0) {
            // Already declared in this scope: do not duplicate
            return false;
        }
    }
    
    // Resize array if needed
    if (table->count >= table->capacity) {
        table->capacity *= 2;
        IdentifierRecord *newRecs = realloc(table->records, sizeof(IdentifierRecord) * table->capacity);
        if (!newRecs) return false;
        table->records = newRecs;
    }
    
    IdentifierRecord rec;
    rec.variableName = strdup(variableName);
    rec.value = value ? strdup(value) : strdup("uninitialized");
    rec.line = line;
    rec.lastUsedLine = line;
    rec.scope = strdup(effectiveScope);
    rec.isConstant = isConstant;
    rec.isFunction = isFunction;
    rec.refCount = 0;
    
    table->records[table->count++] = rec;
    return true;
}

bool updateIdentifierEntry(IdentifierTable *table,
                           const char *variableName,
                           const char *newValue,
                           int line,
                           const char *scope) {
    IdentifierRecord *rec = findIdentifierEntry(table, variableName, scope);
    if (!rec) return false;
    if (rec->isConstant) return false; // Constant values cannot be updated
    
    if (rec->value) free(rec->value);
    rec->value = newValue ? strdup(newValue) : strdup("uninitialized");
    rec->lastUsedLine = line;
    rec->refCount++;
    return true;
}

void recordIdentifierUsage(IdentifierTable *table, const char *variableName, int line, const char *scope) {
    IdentifierRecord *rec = findIdentifierEntry(table, variableName, scope);
    if (!rec) return;
    rec->lastUsedLine = line;
    rec->refCount++;
}

// Traverses source file using the DFA to collect identifiers and their values
IdentifierTable* collectIdentifiersDFA(DFAStateList *dsl, const char *sourceFilename) {
    if (!dsl || dsl->length <= 0 || !sourceFilename) return NULL;
    
    FILE *src = fopen(sourceFilename, "r");
    if (!src) return NULL;
    
    IdentifierTable *table = createIdentifierTable();
    if (!table) {
        fclose(src);
        return NULL;
    }
    
    DFAState *startState = &dsl->data[0];
    DFAState *currentState = startState;
    
    char lineBuffer[2048];
    char lexeme[1024];
    int lexLen = 0;
    int lineNum = 0;
    
    // Parser context for identifier data capture
    char currentScope[64] = "global";
    bool inVariableDecl = false;   // Followed S_VARIABLE (bagahe)
    bool inConstantDecl = false;   // Followed S_CONST (laging)
    bool inFunctionDecl = false;   // Followed S_FUNCTION (route)
    bool waitingForValue = false;  // Followed S_ASSIGN (=)
    char pendingName[64] = "";
    char pendingValue[256] = "";
    int pendingLine = 0;
    
    while (fgets(lineBuffer, sizeof(lineBuffer), src) != NULL) {
        lineNum++;
        int lineLen = (int)strlen(lineBuffer);
        int col = 0;
        
        while (col < lineLen) {
            char c = lineBuffer[col];
            
            // Single line comment
            if (currentState->type == S_C_LINE) {
                if (c == '\n' || c == '\r') {
                    currentState = startState;
                    lexLen = 0;
                    lexeme[0] = '\0';
                }
                col++;
                continue;
            }
            
            // DFA transition
            DFAState *nextState = currentState->next[(unsigned char)c];
            
            if (nextState != NULL) {
                currentState = nextState;
                if (lexLen < (int)sizeof(lexeme) - 1) {
                    lexeme[lexLen++] = c;
                    lexeme[lexLen] = '\0';
                }
                col++;

                if (currentState->type == S_C_BLOCK) {
                    currentState = startState;
                    lexLen = 0;
                    lexeme[0] = '\0';
                }
            } else {
                // NO_TRANSITION
                StateType curType = currentState->type;
                
                if (currentState != startState &&
                    curType != INTERMEDIARY &&
                    curType != START) {
                    
                    // Token recognized based purely on DFA StateType enum (no strcmp)!
                    if (curType == S_VARIABLE) {
                        inVariableDecl = true;
                    } else if (curType == S_CONST) {
                        inConstantDecl = true;
                    } else if (curType == S_FUNCTION) {
                        inFunctionDecl = true;
                    } else if (curType == S_ASSIGN) {
                        if (strlen(pendingName) > 0) {
                            waitingForValue = true;
                        }
                    } else if (curType == S_LINE_END) {
                        // Semicolon: commit pending identifier with its collected value
                        if (strlen(pendingName) > 0) {
                            const char *valStr = (strlen(pendingValue) > 0) ? pendingValue : "uninitialized";
                            insertIdentifierEntry(table, pendingName, valStr, pendingLine, currentScope, inConstantDecl, false);
                            pendingName[0] = '\0';
                            pendingValue[0] = '\0';
                            inVariableDecl = false;
                            inConstantDecl = false;
                            waitingForValue = false;
                        }
                    } else if (curType == S_IDENTIFIER) {
                        // DFA recognized an identifier!
                        if (inFunctionDecl) {
                            // Function identifier
                            insertIdentifierEntry(table, lexeme, "[function]", lineNum, "global", false, true);
                            strncpy(currentScope, lexeme, sizeof(currentScope) - 1);
                            inFunctionDecl = false;
                        } else if (inVariableDecl || inConstantDecl) {
                            // Variable identifier being declared
                            strncpy(pendingName, lexeme, sizeof(pendingName) - 1);
                            pendingLine = lineNum;
                            pendingValue[0] = '\0';
                        } else if (waitingForValue) {
                            // Identifier referenced in value expression
                            strncat(pendingValue, lexeme, sizeof(pendingValue) - strlen(pendingValue) - 1);
                            recordIdentifierUsage(table, lexeme, lineNum, currentScope);
                        } else {
                            // Identifier used as assignment target or reference
                            strncpy(pendingName, lexeme, sizeof(pendingName) - 1);
                            pendingLine = lineNum;
                            recordIdentifierUsage(table, lexeme, lineNum, currentScope);
                        }
                    } else if (curType == S_RBRACE) {
                        // Scope closed
                        strcpy(currentScope, "global");
                    } else {
                        // Literal value (S_Lit_INT, S_Lit_STRING, S_Lit_DEC, S_T_NULL, S_TRUE, S_FALSE, etc.)
                        if (waitingForValue) {
                            if (strlen(pendingValue) > 0 && pendingValue[strlen(pendingValue) - 1] != ' ') {
                                strncat(pendingValue, " ", sizeof(pendingValue) - strlen(pendingValue) - 1);
                            }
                            strncat(pendingValue, lexeme, sizeof(pendingValue) - strlen(pendingValue) - 1);
                        }
                    }
                    
                    currentState = startState;
                    lexLen = 0;
                    lexeme[0] = '\0';
                } else if (currentState == startState) {
                    col++;
                } else {
                    currentState = startState;
                    lexLen = 0;
                    lexeme[0] = '\0';
                    col++;
                }
            }
        }
        
        if (currentState->type == S_C_LINE) {
            currentState = startState;
            lexLen = 0;
            lexeme[0] = '\0';
        }
    }
    
    // Commit trailing declaration if any
    if (strlen(pendingName) > 0) {
        const char *valStr = (strlen(pendingValue) > 0) ? pendingValue : "uninitialized";
        insertIdentifierEntry(table, pendingName, valStr, pendingLine, currentScope, inConstantDecl, false);
    }
    
    fclose(src);
    return table;
}

void printIdentifierTable(IdentifierTable *table, FILE *stream) {
    if (!table || !stream) return;
    
    fprintf(stream, "===================================================================================================\n");
    fprintf(stream, "                             TRENSLATE IDENTIFIER SYMBOL TABLE                                     \n");
    fprintf(stream, "===================================================================================================\n");
    fprintf(stream, " %-5s | %-16s | %-20s | %-12s | %-5s | %-10s | %-6s | %s\n",
            "Index", "Variable Name", "Value", "Implied Type", "Line", "Scope", "Const?", "Refs");
    fprintf(stream, "---------------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < table->count; i++) {
        IdentifierRecord *r = &table->records[i];
        fprintf(stream, " %-5d | %-16s | %-20s | %-12s | %-5d | %-10s | %-6s | %d\n",
                i + 1,
                r->variableName ? r->variableName : "",
                r->value ? r->value : "",
                inferImpliedType(r->value),
                r->line,
                r->scope ? r->scope : "global",
                r->isConstant ? "YES" : "NO",
                r->refCount);
    }
    fprintf(stream, "===================================================================================================\n");
    fprintf(stream, " Total Identifiers Tracked: %d\n", table->count);
    fprintf(stream, "===================================================================================================\n\n");
}

void freeIdentifierTable(IdentifierTable *table) {
    if (!table) return;
    if (table->records) {
        for (int i = 0; i < table->count; i++) {
            if (table->records[i].variableName) free(table->records[i].variableName);
            if (table->records[i].value) free(table->records[i].value);
            if (table->records[i].scope) free(table->records[i].scope);
        }
        free(table->records);
    }
    free(table);
}
