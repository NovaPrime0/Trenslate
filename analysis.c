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

// Invalid token tracking and rule classification

InvalidTokenTable* createInvalidTable(void) {
    InvalidTokenTable *table = malloc(sizeof(InvalidTokenTable));
    if (!table) return NULL;
    table->capacity = 16;
    table->count = 0;
    table->data = malloc(sizeof(InvalidToken) * table->capacity);
    return table;
}

void addInvalidToken(InvalidTokenTable *table, int line, int col, const char *lexeme, const char *rule, const char *desc) {
    if (!table || !lexeme) return;
    if (table->count >= table->capacity) {
        table->capacity *= 2;
        InvalidToken *newArr = realloc(table->data, sizeof(InvalidToken) * table->capacity);
        if (!newArr) return;
        table->data = newArr;
    }
    InvalidToken *tok = &table->data[table->count];
    tok->index = table->count + 1;
    tok->line = line;
    tok->col = col;
    tok->lexeme = strdup(lexeme);
    tok->rule = strdup(rule ? rule : "Rule 1");
    tok->description = strdup(desc ? desc : "Token does not conform to Trenslate grammar specifications.");
    table->count++;
}

void freeInvalidTable(InvalidTokenTable *table) {
    if (!table) return;
    for (int i = 0; i < table->count; i++) {
        if (table->data[i].lexeme) free(table->data[i].lexeme);
        if (table->data[i].rule) free(table->data[i].rule);
        if (table->data[i].description) free(table->data[i].description);
    }
    if (table->data) free(table->data);
    free(table);
}

// Classifies invalid tokens and intermediary halts into rules 1 to 7
static void classifyInvalidRule(StateType stateType, const char *lexeme, char lookahead, const char **ruleOut, const char **descOut) {
    if (!ruleOut || !descOut) return;
    
    // Rule 2: Identifier starting with digit
    if (stateType == S_Lit_INT || stateType == S_Lit_DEC) {
        if (isalpha((unsigned char)lookahead) || lookahead == '_') {
            *ruleOut = "Rule 2";
            *descOut = "Identifiers must start with a letter (A-Z, a-z) or underscore (_), never a digit (0-9).";
            return;
        }
    }
    
    // Rule 4: Incomplete logical operators (Intermediary states)
    if (stateType == INTERMEDIARY) {
        if (strcmp(lexeme, "|") == 0) {
            *ruleOut = "Rule 4";
            *descOut = "Single '|' is not a recognized operator in Trenslate. Logical OR requires '||'.";
            return;
        }
        if (strcmp(lexeme, "&") == 0) {
            *ruleOut = "Rule 4";
            *descOut = "Single '&' is not a recognized operator in Trenslate. Logical AND requires '&&'.";
            return;
        }
        if (lexeme[0] == '"') {
            *ruleOut = "Rule 6";
            *descOut = "String literal was opened with '\"' but reached newline or EOF without closing '\"'.";
            return;
        }
        if (lexeme[0] == '\'') {
            *ruleOut = "Rule 7";
            *descOut = "Character literal was opened with ''' but reached delimiter/newline/EOF without closing '''.";
            return;
        }
        if (strncmp(lexeme, "/*", 2) == 0) {
            *ruleOut = "Rule 7";
            *descOut = "Block comment was opened with '/*' but reached EOF without terminating '*/'.";
            return;
        }
        if (strlen(lexeme) > 0 && lexeme[strlen(lexeme) - 1] == '.') {
            *ruleOut = "Rule 5";
            *descOut = "Decimal point must be followed by at least one digit (0-9).";
            return;
        }
        *ruleOut = "Rule 4";
        *descOut = "Lexeme halted in a non-accepting intermediary state without completing a valid token.";
        return;
    }
    
    // Rule 1: Unknown character set symbol or special symbol inside identifier
    if (stateType == S_INVALID || stateType == START || strlen(lexeme) == 1) {
        *ruleOut = "Rule 1";
        *descOut = "Character is outside the Trenslate alphabet and cannot appear in source code or identifiers.";
        return;
    }
    
    // Default fallback rule
    *ruleOut = "Rule 1";
    *descOut = "Lexeme does not match any valid keyword, identifier, literal, operator, or delimiter.";
}

void printInvalidTable(InvalidTokenTable *table, const char *sourceFile, FILE *out) {
    if (!out) return;
    
    fprintf(out, "========================================================================================================================\n");
    fprintf(out, "                                   TRENSLATE INVALID TOKENS & LEXICAL ERROR REPORT                                      \n");
    fprintf(out, " Source File: %s\n", sourceFile ? sourceFile : "unknown");
    fprintf(out, "========================================================================================================================\n\n");
    
    fprintf(out, "------------------------------------------------------------------------------------------------------------------------\n");
    fprintf(out, " %-5s | %-5s | %-5s | %-20s | %-14s | %s\n",
            "Index", "Line", "Col", "Invalid Lexeme", "Violated Rule", "Description");
    fprintf(out, "------------------------------------------------------------------------------------------------------------------------\n");
    
    if (!table || table->count == 0) {
        fprintf(out, "  (No invalid tokens detected. All source lexemes successfully conformed to Trenslate language rules!)\n");
    } else {
        for (int i = 0; i < table->count; i++) {
            char escLex[MAX_LEXEME_LEN * 2];
            escapeString(table->data[i].lexeme, escLex, sizeof(escLex));
            fprintf(out, " %-5d | %-5d | %-5d | %-20s | %-14s | %s\n",
                    table->data[i].index,
                    table->data[i].line,
                    table->data[i].col,
                    escLex,
                    table->data[i].rule,
                    table->data[i].description);
        }
    }
    fprintf(out, "------------------------------------------------------------------------------------------------------------------------\n");
    fprintf(out, " Total Invalid Entries: %d\n", table ? table->count : 0);
    fprintf(out, "========================================================================================================================\n\n");
    
    // Language rules reference summary
    fprintf(out, "========================================================================================================================\n");
    fprintf(out, "                                     TRENSLATE RULES REFERENCE GUIDE                                                    \n");
    fprintf(out, "========================================================================================================================\n");
    fprintf(out, " Rule 1:\n");
    fprintf(out, "   - Recognized Alphabet: Letters (A-Z, a-z), Digits (0-9), Arithmetic (+, -, *, /, %%, //, **),\n");
    fprintf(out, "     Relational (<, >, =, !, ==, !=, >=, <=), Logical (&&, ||, !), Delimiters (,, ;, ., :), Brackets ((), {}, []),\n");
    fprintf(out, "     and Quotes (\", '). Any other character (@, $, ~, ^, ?, etc.) is strictly INVALID.\n\n");
    
    fprintf(out, " Rule 2:\n");
    fprintf(out, "   - Must start with a letter (A-Z, a-z) or an underscore (_), NEVER a digit (0-9).\n");
    fprintf(out, "   - Valid:   _count, pamasahe, computeArea\n");
    fprintf(out, "   - Invalid: 2count, 9pasahero, 0bilang\n\n");
    
    fprintf(out, " Rule 3:\n");
    fprintf(out, "   - After the first character, identifiers may contain letters, digits, and underscores.\n");
    fprintf(out, "   - Special symbols (-, @, #, !, etc.) or whitespace are NOT allowed inside identifier names.\n");
    fprintf(out, "   - Invalid: pasa@hero, sukli#1, pamasahe-rate\n\n");
    
    fprintf(out, " Rule 4:\n");
    fprintf(out, "   - Logical operators are '&&' (AND), '||' (OR), and '!' (NOT).\n");
    fprintf(out, "   - A single '|' or single '&' is an incomplete INTERMEDIARY DFA state.\n");
    fprintf(out, "   - Halting in an intermediary state without completing the multi-character operator is strictly INVALID.\n\n");
    
    fprintf(out, " Rule 5:\n");
    fprintf(out, "   - Integer literals: one or more digits (0-9).\n");
    fprintf(out, "   - Decimal literals: must have digits on both sides of the decimal point (e.g. 15.50).\n");
    fprintf(out, "   - Trailing decimal points without following digits (e.g. 15.) remain in INTERMEDIARY state and are INVALID.\n\n");
    
    fprintf(out, " Rule 6:\n");
    fprintf(out, "   - Strings must be enclosed by matching double quotation marks (\"...\").\n");
    fprintf(out, "   - Characters must enclose exactly one character within single quotation marks ('c').\n");
    fprintf(out, "   - Reaching line end or EOF without closing quotation leaves the DFA in INTERMEDIARY state and is INVALID.\n\n");
    
    fprintf(out, " Rule 7:\n");
    fprintf(out, "   - Multi-line block comments must open with '/*' and terminate with '*/'.\n");
    fprintf(out, "   - Reaching EOF without the terminating '*/' leaves the DFA in INTERMEDIARY state and is INVALID.\n");
    fprintf(out, "========================================================================================================================\n\n");
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
        char c = '\0';
        bool isEof = false;
        if (i < len) {
            c = inputStr[i];
            formatChar(c, charBuf, sizeof(charBuf));
        } else {
            isEof = true;
            snprintf(charBuf, sizeof(charBuf), "<EOF>");
        }
        
        // Peek at transition for character 'c'
        DFAState *nextState = isEof ? NULL : currentState->next[(unsigned char)c];
        
        if (nextState != NULL) {
            // Valid transition found! Print input step
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
            if (!isEof) {
                formatChar(c, charBuf, sizeof(charBuf));
            } else {
                snprintf(charBuf, sizeof(charBuf), "<EOF>");
            }
            formatStateSet(currentState->stateNumber, stateBuf, sizeof(stateBuf));
            const char *curToken = getTokenNameFromDFA(currentState->type);
            
            if ((currentState->type == S_Lit_INT || currentState->type == S_Lit_DEC) &&
                (isalpha((unsigned char)c) || c == '_')) {
                // Malformed identifier starting with digit: e.g. 9pasahero
                while (i < len && (isalnum((unsigned char)inputStr[i]) || inputStr[i] == '_')) {
                    if (lexLen < MAX_LEXEME_LEN - 1) {
                        lexeme[lexLen++] = inputStr[i];
                        lexeme[lexLen] = '\0';
                    }
                    i++;
                }
                char escLex[MAX_LEXEME_LEN * 2];
                escapeString(lexeme, escLex, sizeof(escLex));
                fprintf(out, "        --------------------------------------------------------------\n");
                fprintf(out, "        ==> [MALFORMED IDENTIFIER]: Identifier starting with digit\n");
                fprintf(out, "            >>> EMIT TOKEN: [INVALID] (Lexeme: \"%s\")\n", escLex);
                fprintf(out, "            >>> RULE: Rule 2 - Identifiers cannot start with digit\n");
                fprintf(out, "            >>> RESET DFA: Return to Start State {0} (START)\n");
                fprintf(out, "        --------------------------------------------------------------\n");
                currentState = startState;
                lexLen = 0;
                lexeme[0] = '\0';
                continue;
            }
            
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
                
                if (isEof) {
                    fprintf(out, "        ==> [END OF INPUT]: Reached end of string (<EOF>) -> Simulation Complete\n");
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
                if (!isEof) i++;
                else break;
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
    
    InvalidTokenTable *invalidTbl = createInvalidTable();
    
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
            
            // Traverse using the DFA next transition pointer directly
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
                
                // Rule 2: Check for malformed identifier starting with digit (e.g., 9pasahero, 2count)
                if ((curType == S_Lit_INT || curType == S_Lit_DEC) &&
                    (isalpha((unsigned char)c) || c == '_')) {
                    int startCol = col - lexLen + 1;
                    while (col < lineLen && (isalnum((unsigned char)lineBuffer[col]) || lineBuffer[col] == '_')) {
                        if (lexLen < MAX_LEXEME_LEN - 1) {
                            lexeme[lexLen++] = lineBuffer[col];
                            lexeme[lexLen] = '\0';
                        }
                        col++;
                    }
                    Symbol sym;
                    sym.identifier = strdup(lexeme);
                    sym.token = "INVALID";
                    sym.line = lineNum;
                    addSymbol(symtbl, &sym);
                    
                    if (invalidTbl) {
                        addInvalidToken(invalidTbl, lineNum, startCol, lexeme,
                            "Rule 2",
                            "Identifiers must start with a letter (A-Z, a-z) or underscore (_), never a digit (0-9).");
                    }
                    
                    char escLex[MAX_LEXEME_LEN * 2];
                    escapeString(lexeme, escLex, sizeof(escLex));
                    if (histFile) {
                        fprintf(histFile, "  Line %3d, Col %2d | Input: %-8s | State: %-20s | Token: %-14s | Acc: \"%s\"\n",
                                lineNum, startCol, "MALFORM", stateBuf, "INVALID", escLex);
                        fprintf(histFile, "             ------------------------------------------------------------------\n");
                        fprintf(histFile, "             ==> [RULE VIOLATION Rule 2]: Malformed Identifier \"%s\"\n", escLex);
                        fprintf(histFile, "                 >>> EMIT TOKEN: [INVALID] Lexeme: \"%s\"\n", escLex);
                        fprintf(histFile, "                 >>> RESET DFA: Return to Start State {0} (START)\n");
                        fprintf(histFile, "             ------------------------------------------------------------------\n");
                    }
                    printf("  Line %3d, Col %2d | Malformed Identifier: \"%s\" (starts with digit)\n",
                           lineNum, startCol, escLex);
                    printf("             ==> [RULE VIOLATION Rule 2]: Emit INVALID and Reset\n");
                    
                    currentState = startState;
                    lexLen = 0;
                    lexeme[0] = '\0';
                    continue;
                }
                
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
                    } else if (curType == S_INVALID) {
                        Symbol sym;
                        sym.identifier = strdup(lexeme);
                        sym.token = "INVALID";
                        sym.line = lineNum;
                        
                        addSymbol(symtbl, &sym);
                        const char *ruleName = NULL;
                        const char *ruleDesc = NULL;
                        classifyInvalidRule(S_INVALID, lexeme, c, &ruleName, &ruleDesc);
                        if (invalidTbl) {
                            addInvalidToken(invalidTbl, lineNum, col - lexLen + 1, lexeme, ruleName, ruleDesc);
                        }
                        
                        char escLex[MAX_LEXEME_LEN * 2];
                        escapeString(sym.identifier, escLex, sizeof(escLex));
                        if (histFile) {
                            fprintf(histFile, "             ------------------------------------------------------------------\n");
                            fprintf(histFile, "             ==> [LOOKAHEAD on %s]: NO_TRANSITION in State %s\n", charBuf, stateBuf);
                            fprintf(histFile, "                 >>> EMIT TOKEN: [INVALID] Lexeme: \"%s\"\n", escLex);
                            fprintf(histFile, "                 >>> RESET DFA: Return to Start State {0} (START)\n");
                            fprintf(histFile, "             ------------------------------------------------------------------\n");
                        }
                        printf("             ------------------------------------------------------------------\n");
                        printf("             ==> [LOOKAHEAD on %s]: NO_TRANSITION in State %s\n", charBuf, stateBuf);
                        printf("                 >>> EMIT TOKEN: [INVALID] Lexeme: \"%s\"\n", escLex);
                        printf("                 >>> RESET DFA: Return to Start State {0} (START)\n");
                        printf("             ------------------------------------------------------------------\n");
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
                        
                        const char *ruleName = NULL;
                        const char *ruleDesc = NULL;
                        classifyInvalidRule(START, invStr, c, &ruleName, &ruleDesc);
                        if (invalidTbl) {
                            addInvalidToken(invalidTbl, lineNum, col + 1, invStr, ruleName, ruleDesc);
                        }
                        
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
                    
                    const char *ruleName = NULL;
                    const char *ruleDesc = NULL;
                    classifyInvalidRule(INTERMEDIARY, lexeme, c, &ruleName, &ruleDesc);
                    if (invalidTbl) {
                        addInvalidToken(invalidTbl, lineNum, col - lexLen + 1, lexeme, ruleName, ruleDesc);
                    }
                    
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
    
    // End of file: flush any remaining accepted or intermediary token in buffer
    if (currentState != startState && currentState->type != START) {
        if (currentState->type == INTERMEDIARY) {
            Symbol sym;
            sym.identifier = strdup(lexeme);
            sym.token = "INVALID";
            sym.line = lineNum;
            addSymbol(symtbl, &sym);
            
            const char *ruleName = NULL;
            const char *ruleDesc = NULL;
            classifyInvalidRule(INTERMEDIARY, lexeme, '$', &ruleName, &ruleDesc);
            if (invalidTbl) {
                addInvalidToken(invalidTbl, lineNum, 1, lexeme, ruleName, ruleDesc);
            }
            if (histFile) {
                fprintf(histFile, "  [EOF FLUSH INTERMEDIARY] Token: INVALID Lexeme: %s\n", sym.identifier);
            }
            printf("  [EOF FLUSH INTERMEDIARY] Token: INVALID Lexeme: %s\n", sym.identifier);
        } else if (currentState->type != S_WHITESPACE &&
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
    
    // Generate Output/Invalid Tokens.txt
    FILE *invFile = fopen("Output/Invalid Tokens.txt", "w");
    if (invFile) {
        printInvalidTable(invalidTbl, sourceFilename, invFile);
        fclose(invFile);
        printf("[Output File Generated]: 'Output/Invalid Tokens.txt'\n");
    }
    if (invalidTbl) {
        freeInvalidTable(invalidTbl);
    }
    
    fclose(src);
    printf("----------------------------------------------------------------------\n");
    printf("[4] Analysis Complete! Emitted %d tokens into Symbol Table.\n\n", symtbl->curIdx);
    
    return symtbl;
}

void printSymbolTable(SymTBL *st, FILE *out) {
    if (!st || !out) return;
    
    int validCount = 0;
    int invalidCount = 0;
    for (int i = 0; i < st->curIdx; i++) {
        if (st->data[i].token && strcmp(st->data[i].token, "INVALID") == 0) {
            invalidCount++;
        } else {
            validCount++;
        }
    }
    
    // SECTION 1: VALID LANGUAGE SYMBOLS
    fprintf(out, "======================================================================\n");
    fprintf(out, "                   TRENSLATE VALID SYMBOL TABLE                       \n");
    fprintf(out, "======================================================================\n");
    fprintf(out, " %-6s | %-6s | %-18s | %s\n", "Index", "Line", "Token Type", "Lexeme");
    fprintf(out, "----------------------------------------------------------------------\n");
    int vIdx = 1;
    for (int i = 0; i < st->curIdx; i++) {
        if (st->data[i].token && strcmp(st->data[i].token, "INVALID") != 0) {
            fprintf(out, " %-6d | %-6d | %-18s | %s\n",
                    vIdx++,
                    st->data[i].line,
                    st->data[i].token ? st->data[i].token : "",
                    st->data[i].identifier ? st->data[i].identifier : "");
        }
    }
    fprintf(out, "======================================================================\n");
    fprintf(out, " Total Valid Symbols Recorded: %d\n", validCount);
    fprintf(out, "======================================================================\n\n");
    
    // SECTION 2: SEPARATED INVALID ENTRIES
    if (invalidCount > 0) {
        fprintf(out, "======================================================================\n");
        fprintf(out, "              TRENSLATE SEPARATED INVALID ENTRIES                     \n");
        fprintf(out, "======================================================================\n");
        fprintf(out, " %-6s | %-6s | %-18s | %s\n", "Index", "Line", "Token Type", "Lexeme");
        fprintf(out, "----------------------------------------------------------------------\n");
        int invIdx = 1;
        for (int i = 0; i < st->curIdx; i++) {
            if (st->data[i].token && strcmp(st->data[i].token, "INVALID") == 0) {
                fprintf(out, " %-6d | %-6d | %-18s | %s\n",
                        invIdx++,
                        st->data[i].line,
                        st->data[i].token ? st->data[i].token : "",
                        st->data[i].identifier ? st->data[i].identifier : "");
            }
        }
        fprintf(out, "======================================================================\n");
        fprintf(out, " Total Invalid Entries: %d\n", invalidCount);
        fprintf(out, " [Note]: Comprehensive rule-by-rule report in: 'Output/Invalid Tokens.txt'\n");
        fprintf(out, "======================================================================\n\n");
    }
}
