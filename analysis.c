#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "analysis.h"
#include "state.h"
#include "enums.h"
#include "symtbl.h"

#define MAX_LINE_LEN 2048
#define MAX_LEXEME_LEN 1024

// Validates that the source file ends with the unique file extension (e.g., .trn)
bool validateExtension(const char *filename, const char *expectedExt) {
    if (!filename || !expectedExt) return false;
    
    size_t nameLen = strlen(filename);
    size_t extLen = strlen(expectedExt);
    if (nameLen < extLen) return false;
    
    const char *ext = filename + (nameLen - extLen);
    return (strcmp(ext, expectedExt) == 0);
}

// Converts StateType enum into token name string using stateNames array (no strcmp used)
static const char* getTokenNameFromDFA(StateType type) {
    if (type <= INTERMEDIARY) {
        const char *name = stateNames[type];
        // Strip the leading "S_" prefix if present for clean token output
        if (name[0] == 'S' && name[1] == '_') {
            return name + 2;
        }
        return name;
    }
    return "UNKNOWN";
}

// Analysis Phase: reads source line-by-line and traverses DFA char-by-char
SymTBL* runAnalysis(DFAStateList *dsl, const char *sourceFilename) {
    if (!dsl || dsl->length <= 0) {
        fprintf(stderr, "[Analysis Phase Error]: DFAStateList is empty or null.\n");
        return NULL;
    }
    
    printf("\n======================================================================\n");
    printf("                     TRENSLATE ANALYSIS PHASE                         \n");
    printf("======================================================================\n");
    
    // Step 1: Validate unique file extension (.trn)
    printf("[1] Checking file extension for: '%s'...\n", sourceFilename);
    if (!validateExtension(sourceFilename, TRENSLATE_EXT)) {
        fprintf(stderr, "    [ERROR] Invalid file extension!\n");
        fprintf(stderr, "    Trenslate source programs must use the unique '%s' extension.\n", TRENSLATE_EXT);
        return NULL;
    }
    printf("    [OK] File extension '%s' is valid.\n\n", TRENSLATE_EXT);
    
    // Step 2: Open source file for line-by-line reading
    printf("[2] Opening source file '%s' (line-by-line)...\n", sourceFilename);
    FILE *src = fopen(sourceFilename, "r");
    if (!src) {
        fprintf(stderr, "    [ERROR] Could not open source file '%s'.\n", sourceFilename);
        return NULL;
    }
    printf("    [OK] Source file opened successfully.\n\n");
    
    SymTBL *symtbl = createTable();
    if (!symtbl) {
        fclose(src);
        return NULL;
    }
    
    // Step 3: DFA Traversal based purely on DFAState pointers and StateType
    printf("[3] Traversing loaded DFA character-by-character:\n");
    printf("----------------------------------------------------------------------\n");
    
    DFAState *startState = &dsl->data[0];
    DFAState *currentState = startState;
    
    char lineBuffer[MAX_LINE_LEN];
    char lexeme[MAX_LEXEME_LEN];
    int lexLen = 0;
    int lineNum = 0;
    
    while (fgets(lineBuffer, sizeof(lineBuffer), src) != NULL) {
        lineNum++;
        int lineLen = (int)strlen(lineBuffer);
        int col = 0;
        
        while (col < lineLen) {
            char c = lineBuffer[col];
            
            // Single-line comment state (S_C_LINE) continues until newline
            if (currentState->type == S_C_LINE) {
                if (c == '\n' || c == '\r') {
                    currentState = startState;
                    lexLen = 0;
                    lexeme[0] = '\0';
                }
                col++;
                continue;
            }
            
            // Traversal using the DFA transition pointer (next) directly:
            // Zero strcmp! The DFA handles all symbol and keyword recognition.
            DFAState *nextState = currentState->next[(unsigned char)c];
            
            if (nextState != NULL) {
                // Valid transition found in DFA
                currentState = nextState;
                if (lexLen < MAX_LEXEME_LEN - 1) {
                    lexeme[lexLen++] = c;
                    lexeme[lexLen] = '\0';
                }
                col++;
                
                // If block comment completed with '*/', reset to startState
                if (currentState->type == S_C_BLOCK) {
                    currentState = startState;
                    lexLen = 0;
                    lexeme[0] = '\0';
                }
            } else {
                // NO_TRANSITION reached: check if currentState is an accepting state
                StateType curType = currentState->type;
                
                if (currentState != startState &&
                    curType != INTERMEDIARY &&
                    curType != START) {
                    
                    // ACCEPTING STATE: Token recognized by DFA
                    if (curType == S_WHITESPACE) {
                        // Whitespace recognized by DFA -> skip
                    } else if (curType == S_C_LINE || curType == S_C_BLOCK) {
                        // Comment recognized by DFA -> skip
                    } else {
                        // Valid language token -> emit to symbol table
                        Symbol sym;
                        sym.identifier = strdup(lexeme);
                        sym.token = (char*)getTokenNameFromDFA(curType);
                        sym.line = lineNum;
                        
                        addSymbol(symtbl, &sym);
                        printf("  [Line %3d] Token: %-16s Lexeme: \"%s\"\n", lineNum, sym.token, sym.identifier);
                    }
                    
                    // Reset to DFA start state
                    currentState = startState;
                    lexLen = 0;
                    lexeme[0] = '\0';
                    
                    // Do not increment col: re-evaluate current character 'c' from startState
                } else if (currentState == startState) {
                    // In startState with no transition defined for 'c'
                    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                        // General whitespace separator
                        col++;
                    } else {
                        Symbol sym;
                        char invStr[2] = { c, '\0' };
                        sym.identifier = strdup(invStr);
                        sym.token = "INVALID";
                        sym.line = lineNum;
                        addSymbol(symtbl, &sym);
                        printf("  [Line %3d, Col %2d] Token: %-16s Lexeme: \"%s\"\n",
                               lineNum, col + 1, "INVALID", sym.identifier);
                        col++;
                    }
                } else {
                    // Reached NO_TRANSITION while in an INTERMEDIARY state
                    Symbol sym;
                    sym.identifier = strdup(lexeme);
                    sym.token = "INVALID";
                    sym.line = lineNum;
                    addSymbol(symtbl, &sym);
                    printf("  [Line %3d, Col %2d] Token: %-16s Lexeme: \"%s\"\n",
                           lineNum, col + 1, "INVALID", sym.identifier);
                    currentState = startState;
                    lexLen = 0;
                    lexeme[0] = '\0';
                    col++;
                }
            }
        }
        
        // End of line: reset single-line comment if active
        if (currentState->type == S_C_LINE) {
            currentState = startState;
            lexLen = 0;
            lexeme[0] = '\0';
        }
    }
    
    // End of file: flush any remaining accepted token in buffer
    if (currentState != startState &&
        currentState->type != INTERMEDIARY &&
        currentState->type != START) {
        
        if (currentState->type != S_WHITESPACE &&
            currentState->type != S_C_LINE &&
            currentState->type != S_C_BLOCK) {
            Symbol sym;
            sym.identifier = strdup(lexeme);
            sym.token = (char*)getTokenNameFromDFA(currentState->type);
            sym.line = lineNum;
            addSymbol(symtbl, &sym);
            printf("  [Line %3d] Token: %-16s Lexeme: \"%s\"\n", lineNum, sym.token, sym.identifier);
        }
    }
    
    fclose(src);
    printf("----------------------------------------------------------------------\n");
    printf("[4] Analysis Complete! Emitted %d tokens into Symbol Table.\n\n", symtbl->curIdx);
    
    return symtbl;
}

void printSymbolTable(SymTBL *st, FILE *out) {
    if (!st || !out) return;
    
    fprintf(out, "======================================================================\n");
    fprintf(out, "                         TRENSLATE SYMBOL TABLE                       \n");
    fprintf(out, "======================================================================\n");
    fprintf(out, " %-6s | %-6s | %-18s | %s\n", "Index", "Line", "Token Type", "Lexeme");
    fprintf(out, "----------------------------------------------------------------------\n");
    for (int i = 0; i < st->curIdx; i++) {
        fprintf(out, " %-6d | %-6d | %-18s | %s\n",
                i + 1,
                st->data[i].line,
                st->data[i].token ? st->data[i].token : "",
                st->data[i].identifier ? st->data[i].identifier : "");
    }
    fprintf(out, "======================================================================\n");
    fprintf(out, " Total Symbols Recorded: %d\n", st->curIdx);
    fprintf(out, "======================================================================\n\n");
}
