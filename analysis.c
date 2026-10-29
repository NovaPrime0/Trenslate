#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "analysis.h"
#include "state.h"
#include "enums.h"
#include "symtbl.h"

#define MAX_LINE_LEN 2048
#define MAX_LEXEME_LEN 1024

// Validates that the source file ends with the unique file extension (.tren)
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

// Formats a Set of NFA state numbers into a human-readable string: e.g. "{0}", "{1, 38, 48}"
static void formatStateSet(const Set *s, char *buf, size_t maxLen) {
    if (!s || s->length == 0) {
        snprintf(buf, maxLen, "{}");
        return;
    }
    size_t offset = 0;
    offset += snprintf(buf + offset, maxLen - offset, "{");
    int limit = (s->length > 6) ? 6 : s->length;
    for (int i = 0; i < limit; i++) {
        offset += snprintf(buf + offset, maxLen - offset, "%d%s",
                           s->data[i], (i < s->length - 1) ? ", " : "}");
    }
    if (s->length > 6 && offset < maxLen - 5) {
        snprintf(buf + offset, maxLen - offset, "...}");
    }
}

// Formats a character for logging, escaping control characters: e.g. 'a', '\n', ' '
static void formatChar(char c, char *buf, size_t maxLen) {
    switch (c) {
        case '\n': snprintf(buf, maxLen, "'\\n'"); break;
        case '\r': snprintf(buf, maxLen, "'\\r'"); break;
        case '\t': snprintf(buf, maxLen, "'\\t'"); break;
        case ' ':  snprintf(buf, maxLen, "' '"); break;
        default:
            if ((unsigned char)c >= 32 && (unsigned char)c <= 126) {
                snprintf(buf, maxLen, "'%c'", c);
            } else {
                snprintf(buf, maxLen, "'\\x%02X'", (unsigned char)c);
            }
            break;
    }
}

// Escapes special characters in string for clean single-line table display
static void escapeString(const char *src, char *dst, size_t maxLen) {
    if (!src || !dst || maxLen == 0) return;
    size_t o = 0;
    for (size_t i = 0; src[i] != '\0' && o + 5 < maxLen; i++) {
        switch (src[i]) {
            case '\n': dst[o++] = '\\'; dst[o++] = 'n'; break;
            case '\r': dst[o++] = '\\'; dst[o++] = 'r'; break;
            case '\t': dst[o++] = '\\'; dst[o++] = 't'; break;
            case '"':  dst[o++] = '\\'; dst[o++] = '"'; break;
            default:
                if ((unsigned char)src[i] >= 32 && (unsigned char)src[i] <= 126) {
                    dst[o++] = src[i];
                } else {
                    o += snprintf(dst + o, maxLen - o, "\\x%02X", (unsigned char)src[i]);
                }
                break;
        }
    }
    dst[o] = '\0';
}

// Simulates DFA character-by-character transition for an input word,
// displaying current state, token, line, accumulated lexeme, and token changes.
void simulateDFAInput(DFAStateList *dsl, const char *inputStr, FILE *out) {
    if (!dsl || !inputStr || !out) return;
    
    DFAState *startState = &dsl->data[0];
    DFAState *currentState = startState;
    char lexeme[MAX_LEXEME_LEN] = {0};
    int lexLen = 0;
    
    char stateBuf[128];
    char nextStateBuf[128];
    char charBuf[16];
    
    fprintf(out, "\n======================================================================\n");
    fprintf(out, "               DFA SIMULATION: Input String \"%s\"\n", inputStr);
    fprintf(out, "======================================================================\n");
    fprintf(out, " %-8s | %-20s | %-16s | %s\n",
            "Input", "Current State", "Current Token", "Accumulated Lexeme");
    fprintf(out, "----------------------------------------------------------------------\n");
    
    size_t len = strlen(inputStr);
    size_t i = 0;
    while (i <= len) {
        char c;
        bool isEof = false;
        if (i < len) {
            c = inputStr[i];
        } else {
            c = '$'; // Standard end-of-input lookahead marker
            isEof = true;
        }
        
        // Peek at transition for character 'c'
        DFAState *nextState = isEof ? NULL : currentState->next[(unsigned char)c];
        
        if (nextState != NULL) {
            // Valid transition found! Print input step
            formatChar(c, charBuf, sizeof(charBuf));
            formatStateSet(currentState->stateNumber, stateBuf, sizeof(stateBuf));
            const char *curToken = getTokenNameFromDFA(currentState->type);
            
            char escAcc[MAX_LEXEME_LEN * 2];
            escapeString(lexeme, escAcc, sizeof(escAcc));
            
            fprintf(out, " %-8s | %-20s | %-16s | \"%s\"\n",
                    charBuf, stateBuf, curToken, escAcc);
            
            formatStateSet(nextState->stateNumber, nextStateBuf, sizeof(nextStateBuf));
            const char *nextToken = getTokenNameFromDFA(nextState->type);
            bool tokenChanged = (currentState->type != nextState->type);
            
            char nextLexeme[MAX_LEXEME_LEN + 16];
            snprintf(nextLexeme, sizeof(nextLexeme), "%s%c", lexeme, c);
            char escNext[MAX_LEXEME_LEN * 2];
            escapeString(nextLexeme, escNext, sizeof(escNext));
            
            fprintf(out, "        --> Transited to State %s (Token: %s)\n", nextStateBuf, nextToken);
            if (tokenChanged) {
                fprintf(out, "        >>> [TOKEN TYPE CHANGED]: %s -> %s (Lexeme now: \"%s\")\n",
                        curToken, nextToken, escNext);
            }
            currentState = nextState;
            if (lexLen < MAX_LEXEME_LEN - 1) {
                lexeme[lexLen++] = c;
                lexeme[lexLen] = '\0';
            }
            i++; // Advance to next input character
        } else {
            // NO_TRANSITION on character 'c'!
            formatChar(c, charBuf, sizeof(charBuf));
            formatStateSet(currentState->stateNumber, stateBuf, sizeof(stateBuf));
            const char *curToken = getTokenNameFromDFA(currentState->type);
            
            if (currentState != startState &&
                currentState->type != INTERMEDIARY &&
                currentState->type != START) {
                
                // 1. EMIT THE ACCEPTED TOKEN FIRST!
                char escLex[MAX_LEXEME_LEN * 2];
                escapeString(lexeme, escLex, sizeof(escLex));
                fprintf(out, "        --------------------------------------------------------------\n");
                fprintf(out, "        ==> [LOOKAHEAD on %s]: NO_TRANSITION in State %s\n", charBuf, stateBuf);
                fprintf(out, "            >>> EMIT TOKEN: [%s] (Lexeme: \"%s\")\n", curToken, escLex);
                fprintf(out, "            >>> RESET DFA: Return to Start State {0} (START)\n");
                fprintf(out, "        --------------------------------------------------------------\n");
                
                // 2. RESET STATE AND ACCUMULATED LEXEME
                currentState = startState;
                lexLen = 0;
                lexeme[0] = '\0';
                
                // 3. Do NOT increment i: re-evaluate 'c' starting from State {0}!
            } else if (currentState == startState) {
                // In Start State {0}
                char escAcc[MAX_LEXEME_LEN * 2];
                escapeString(lexeme, escAcc, sizeof(escAcc));
                
                fprintf(out, " %-8s | %-20s | %-16s | \"%s\"\n",
                        charBuf, stateBuf, curToken, escAcc);
                
                if (isEof || c == '$') {
                    fprintf(out, "        ==> [END OF INPUT]: Reached terminal marker %s -> Simulation Complete\n", charBuf);
                    break;
                } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    fprintf(out, "        ==> [WHITESPACE]: Skip separator %s\n", charBuf);
                    i++;
                } else {
                    fprintf(out, "        ==> [NO_TRANSITION on %s]: INVALID character %s\n", charBuf, charBuf);
                    i++;
                }
            } else {
                // INTERMEDIARY state with no transition
                char escLex[MAX_LEXEME_LEN * 2];
                escapeString(lexeme, escLex, sizeof(escLex));
                
                fprintf(out, " %-8s | %-20s | %-16s | \"%s\"\n",
                        charBuf, stateBuf, curToken, escLex);
                fprintf(out, "        ==> [NO_TRANSITION on %s in INTERMEDIARY]: REJECTED / INVALID (Lexeme: \"%s\")\n",
                        charBuf, escLex);
                currentState = startState;
                lexLen = 0;
                lexeme[0] = '\0';
                i++;
            }
        }
    }
    fprintf(out, "======================================================================\n\n");
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
    
    // Step 1: Validate unique file extension (.tren)
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
    
    // Step 3: Open Transition History file in Test directory
    FILE *histFile = fopen("Test/Transition History.txt", "w");
    if (histFile) {
        fprintf(histFile, "========================================================================================================\n");
        fprintf(histFile, "                                  TRENSLATE DFA TRANSITION HISTORY                                      \n");
        fprintf(histFile, " Source File: %s\n", sourceFilename);
        fprintf(histFile, "========================================================================================================\n");
        fprintf(histFile, " FORMAT:\n");
        fprintf(histFile, "   Line L, Col C | Input: 'c' | State: {...} | Token: TOKEN | Acc: \"...\"\n");
        fprintf(histFile, "       --> Transition to State {...} (Token: TOKEN)\n");
        fprintf(histFile, "       >>> [TOKEN TYPE CHANGED]: OLD_TOKEN -> NEW_TOKEN (Lexeme: \"...\")\n");
        fprintf(histFile, "       ==> [NO_TRANSITION on 'c']: EMIT TOKEN [TOKEN] Lexeme: \"...\" -> Reset to State {0}\n");
        fprintf(histFile, "========================================================================================================\n\n");
    }
    
    // Step 4: DFA Traversal based purely on DFAState pointers and StateType
    printf("[3] Traversing loaded DFA character-by-character:\n");
    printf("----------------------------------------------------------------------\n");
    
    DFAState *startState = &dsl->data[0];
    DFAState *currentState = startState;
    
    char lineBuffer[MAX_LINE_LEN] = {0};
    char lexeme[MAX_LEXEME_LEN] = {0};
    int lexLen = 0;
    int lineNum = 0;
    
    while (fgets(lineBuffer, sizeof(lineBuffer), src) != NULL) {
        lineNum++;
        int lineLen = (int)strlen(lineBuffer);
        int col = 0;
        
        while (col < lineLen) {
            char c = lineBuffer[col];
            
            char charBuf[16];
            char stateBuf[128];
            char nextStateBuf[128];
            formatChar(c, charBuf, sizeof(charBuf));
            formatStateSet(currentState->stateNumber, stateBuf, sizeof(stateBuf));
            const char *curToken = getTokenNameFromDFA(currentState->type);
            
            char escAcc[MAX_LEXEME_LEN * 2];
            escapeString(lexeme, escAcc, sizeof(escAcc));
            
            // Single-line comment state (S_C_LINE) continues until newline
            if (currentState->type == S_C_LINE) {
                if (histFile) {
                    fprintf(histFile, "  Line %3d, Col %2d | Input: %-8s | State: %-20s | Token: %-14s | Acc: \"%s\"\n",
                            lineNum, col + 1, charBuf, stateBuf, curToken, escAcc);
                }
                if (c == '\n' || c == '\r') {
                    if (histFile) {
                        fprintf(histFile, "             ==> [COMMENT END]: Single-line comment terminated -> Reset to State {0}\n");
                    }
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
                if (histFile) {
                    fprintf(histFile, "  Line %3d, Col %2d | Input: %-8s | State: %-20s | Token: %-14s | Acc: \"%s\"\n",
                            lineNum, col + 1, charBuf, stateBuf, curToken, escAcc);
                }
                printf("  Line %3d, Col %2d | Input: %-8s | State: %-20s | Token: %-14s | Acc: \"%s\"\n",
                       lineNum, col + 1, charBuf, stateBuf, curToken, escAcc);
                
                formatStateSet(nextState->stateNumber, nextStateBuf, sizeof(nextStateBuf));
                const char *nextToken = getTokenNameFromDFA(nextState->type);
                bool tokenChanged = (currentState->type != nextState->type);
                
                char nextLexeme[MAX_LEXEME_LEN + 16];
                snprintf(nextLexeme, sizeof(nextLexeme), "%s%c", lexeme, c);
                char escNext[MAX_LEXEME_LEN * 2];
                escapeString(nextLexeme, escNext, sizeof(escNext));
                
                if (histFile) {
                    fprintf(histFile, "             --> Transition to State %s (Token: %s)\n", nextStateBuf, nextToken);
                    if (tokenChanged) {
                        fprintf(histFile, "             >>> [TOKEN TYPE CHANGED]: %s -> %s (Lexeme: \"%s\")\n",
                                curToken, nextToken, escNext);
                    }
                }
                printf("             --> Transition to State %s (Token: %s)\n", nextStateBuf, nextToken);
                if (tokenChanged) {
                    printf("             >>> [TOKEN TYPE CHANGED]: %s -> %s (Lexeme: \"%s\")\n",
                           curToken, nextToken, escNext);
                }
                
                currentState = nextState;
                if (lexLen < MAX_LEXEME_LEN - 1) {
                    lexeme[lexLen++] = c;
                    lexeme[lexLen] = '\0';
                }
                col++;
                
                // If block comment completed with '*/', reset to startState
                if (currentState->type == S_C_BLOCK) {
                    if (histFile) {
                        fprintf(histFile, "             ==> [BLOCK COMMENT END]: Comment closed -> Reset to State {0}\n");
                    }
                    printf("             ==> [BLOCK COMMENT END]: Comment closed -> Reset to State {0}\n");
                    currentState = startState;
                    lexLen = 0;
                    lexeme[0] = '\0';
                }
            } else {
                // NO_TRANSITION reached on character 'c': check if currentState is an accepting state
                StateType curType = currentState->type;
                
                if (currentState != startState &&
                    curType != INTERMEDIARY &&
                    curType != START) {
                    
                    // ACCEPTING STATE: Lookahead on delimiter 'c' triggers token emission FIRST!
                    if (curType == S_WHITESPACE) {
                        if (histFile) {
                            fprintf(histFile, "             ==> [LOOKAHEAD on %s]: Whitespace complete -> Reset to State {0}\n", charBuf);
                        }
                        printf("             ==> [LOOKAHEAD on %s]: Whitespace complete -> Reset to State {0}\n", charBuf);
                    } else if (curType == S_C_LINE || curType == S_C_BLOCK) {
                        if (histFile) {
                            fprintf(histFile, "             ==> [LOOKAHEAD on %s]: Comment complete -> Reset to State {0}\n", charBuf);
                        }
                        printf("             ==> [LOOKAHEAD on %s]: Comment complete -> Reset to State {0}\n", charBuf);
                    } else {
                        // Valid language token -> emit to symbol table
                        Symbol sym;
                        sym.identifier = strdup(lexeme);
                        sym.token = (char*)getTokenNameFromDFA(curType);
                        sym.line = lineNum;
                        
                        addSymbol(symtbl, &sym);
                        char escLex[MAX_LEXEME_LEN * 2];
                        escapeString(sym.identifier, escLex, sizeof(escLex));
                        if (histFile) {
                            fprintf(histFile, "             ------------------------------------------------------------------\n");
                            fprintf(histFile, "             ==> [LOOKAHEAD on %s]: NO_TRANSITION in State %s\n", charBuf, stateBuf);
                            fprintf(histFile, "                 >>> EMIT TOKEN: [%s] Lexeme: \"%s\"\n", sym.token, escLex);
                            fprintf(histFile, "                 >>> RESET DFA: Return to Start State {0} (START)\n");
                            fprintf(histFile, "             ------------------------------------------------------------------\n");
                        }
                        printf("             ------------------------------------------------------------------\n");
                        printf("             ==> [LOOKAHEAD on %s]: NO_TRANSITION in State %s\n", charBuf, stateBuf);
                        printf("                 >>> EMIT TOKEN: [%s] Lexeme: \"%s\"\n", sym.token, escLex);
                        printf("                 >>> RESET DFA: Return to Start State {0} (START)\n");
                        printf("             ------------------------------------------------------------------\n");
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
                        if (histFile) {
                            fprintf(histFile, "  Line %3d, Col %2d | Input: %-8s | State: %-20s | Token: %-14s | Acc: \"%s\"\n",
                                    lineNum, col + 1, charBuf, stateBuf, curToken, escAcc);
                            fprintf(histFile, "             ==> [NO_TRANSITION]: INVALID character %s -> Emit INVALID and Reset\n", charBuf);
                        }
                        printf("  Line %3d, Col %2d | Input: %-8s | State: %-20s | Token: %-14s | Acc: \"%s\"\n",
                               lineNum, col + 1, charBuf, stateBuf, curToken, escAcc);
                        printf("             ==> [NO_TRANSITION]: INVALID character %s -> Emit INVALID and Reset\n", charBuf);
                        col++;
                    }
                } else {
                    // Reached NO_TRANSITION while in an INTERMEDIARY state
                    Symbol sym;
                    sym.identifier = strdup(lexeme);
                    sym.token = "INVALID";
                    sym.line = lineNum;
                    addSymbol(symtbl, &sym);
                    char escLex[MAX_LEXEME_LEN * 2];
                    escapeString(lexeme, escLex, sizeof(escLex));
                    if (histFile) {
                        fprintf(histFile, "  Line %3d, Col %2d | Input: %-8s | State: %-20s | Token: %-14s | Acc: \"%s\"\n",
                                lineNum, col + 1, charBuf, stateBuf, curToken, escAcc);
                        fprintf(histFile, "             ==> [NO_TRANSITION in INTERMEDIARY]: INVALID lexeme \"%s\" -> Emit INVALID and Reset\n", escLex);
                    }
                    printf("  Line %3d, Col %2d | Input: %-8s | State: %-20s | Token: %-14s | Acc: \"%s\"\n",
                           lineNum, col + 1, charBuf, stateBuf, curToken, escAcc);
                    printf("             ==> [NO_TRANSITION in INTERMEDIARY]: INVALID lexeme \"%s\" -> Emit INVALID and Reset\n", escLex);
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
            if (histFile) {
                fprintf(histFile, "  [EOF FLUSH] Token: %-16s Lexeme: %s\n", sym.token, sym.identifier);
            }
            printf("  [EOF FLUSH] Token: %-16s Lexeme: %s\n", sym.token, sym.identifier);
        }
    }
    
    if (histFile) {
        fprintf(histFile, "\n========================================================================================================\n");
        fprintf(histFile, " Analysis Complete! Emitted %d tokens into Symbol Table.\n", symtbl->curIdx);
        fprintf(histFile, "========================================================================================================\n");
        fclose(histFile);
        printf("\n[Output File Generated]: 'Test/Transition History.txt'\n");
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
