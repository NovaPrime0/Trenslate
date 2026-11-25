#include "symtbl.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "parserUtils.h"
#include <string.h>
#include "enums.h"

// Sync state file path (can be overridden at runtime)
#define SYNC_FILE_PATH ".ppl_sync_state.txt"

const char* ruleNames[] = {
    "NONE", "PROGRAM", "STMT", "DECL_STMT",
    "VAR_DECL", "CONST_DECL", "IMPORT",
    "VALUE", "FUNCTION_DECL", "PARAM_LIST",
    "BLOCK_STMT", "INLINE_STMT", "IF_STMT",
    "ELSE_CHAIN", "WHILE_STMT", "FOR_STMT",
    "FOREACH_STMT", "KEYWORD_STMT", "RETURN_STMT",
    "CONTINUE_STMT", "BREAK_STMT",
    "ASSIGN_STMT", "EXPR", "OR_EXPR",
    "AND_EXPR", "REL_EXPR", "REL_OP",
    "ARITH_EXPR", "MUL", "UNARY", "POWER",
    "TERM", "IDENT_TERM", "LITERAL",
    "ARRAY_LIT", "CALL_EXPR", "CALL_STMT",
    "ARG_LIST", "BLOCK", "END_RULE"
};

/* SYNTACTIC ANALYZER 1 Utils
    Recursive Descent Parser
        - Rule names, Stack (tracking), Panic-recovery method

    !!!DO NOT REMOVE ANY COMMENTS!!!
    -- they are extensions if a full-fledged parse tree is required
*/

static Stack *tokenStack;
static int curDepth;
static int lastSyncTokIndex;
static int tokDistFromLastSync;
// static RuleStack *ruleStack;



const char* getName(Rule r){
    return ruleNames[r];
}

void resetEntry(Entry* s){
    // if(s->children) free(s->children);
    memset(s, 0, sizeof(Entry));
}

bool peek(Stack *s, Entry **w){
    if(!s || !w || s->curSize < 1) return false;
    *w = &s->data[s->curSize-1];
    return true; 
}

bool expand(Stack *s){
    int newSize = s->maxSize*2;
    Entry *temp = realloc(s->data, sizeof(Entry)*newSize);
    if(!temp){
        printf("[perserUtils.c: %d]: Failed to allocate stack", __LINE__);
        s->data = NULL;
        return false;
    }
    s->data = temp;
    s->maxSize = newSize;
    for(int i = s->curSize; i < s->maxSize; i++) resetEntry(&s->data[i]);
    return true;
}
bool startTokenStack(){
    tokenStack = malloc(sizeof(Stack));
    if(!tokenStack){
        printf("[parserUtils.c | %s: %d]: Failed to allocate token stack \n", __func__, __LINE__);
        return false;
    }
    if(!initStack(tokenStack)) return false;
    curDepth = 0;
    lastSyncTokIndex = -1;
    return true;
}
bool initStack(Stack *s){
    int newSize = 2;
    s->data = malloc(sizeof(Entry)*2);
    if(!s->data){
        printf("[perserUtils.c: %d]: Failed to allocate stack", __LINE__);
        return false;
    }
    s->curSize = 0;
    s->maxSize = newSize;
    for(int i = 0; i < s->maxSize; i++) resetEntry(&s->data[i]);
    curDepth = 0;
    return true;
}

//pop to an already-defined var
bool pop(Stack *st, Entry* w){
    if(!st){
        printf("[parserUtils.c | %s: %d]: Stack not initialized \n", __func__, __LINE__);
        return false;
    }
    if(st->curSize <= 0) {
        printf("[parserUtils.c | %s: %d]: No entries in stack \n", __func__, __LINE__);
        return false;
    } else if (!w){
        printf("[parserUtils.c | %s: %d]: Null entry, resetting top node instead \n", __func__, __LINE__);
        resetEntry(&st->data[--st->curSize]);
        return true;
    }
    printf("[parserUtils.c | %s: %d]: Attaching data to passed address \n", __func__, __LINE__);
    
    Entry *poppedNode = &st->data[--st->curSize];
    w->sym = poppedNode->sym;
    w->rule = poppedNode->rule;
    w->depth = poppedNode->depth;
    // w->parent = poppedNode -> parent;
    // w->children = poppedNode -> children;
    // w->childrenCount = poppedNode -> childrenCount;
    // w->childrenMaxCount = poppedNode -> childrenMaxCount;
    if(poppedNode->rule == END_RULE) curDepth++;
    else if (poppedNode->rule != NONE) curDepth--;
    printf("[parserUtils.c | %s: %d]: Popped Entry: [%s|%s], Rule: %sw\n",
        __func__, __LINE__,
         w->sym ? w->sym->lexeme : "<Rule", 
         w->sym ? tokenNames[w->sym->token] : "Marker>",
         getName(w->rule));
    resetEntry(poppedNode);
    return true;
}

bool push(Stack *st, Symbol *s, Rule r){
    if(st->curSize + 1 > st->maxSize && !expand(st)) {
        printf("[parserUtils.c | %s: %d]: Failed to Expand Stack \n", __func__, __LINE__);
        return false;
    }
    Entry *w = &st->data[st->curSize++];
    w->depth = curDepth;
    w->sym = s;
    w->rule = r;
    return true;
}


//data is only a wrapper for symTBL symbol for AST, can free only the data without freeing the symbol inside
void freeStack(Stack *s){
    free(s->data);
    free(s); 
}

//only valid delim sync tokens for fallback
bool isDelim(Symbol* s){
    if (!s) return false;
    switch (s->token) {
        case LINE_END: return true;
        case LBRACE: return true;
        case RBRACE: return true;
        default: return false;
    }
}

//all possible lookahead tokens for recovery
bool isSync(Symbol* s, TokenType lookahead){
    if (!s) return false;
    switch (s->token) {
        //control-flow statements
        case IF: return true;
        case WHILE: return true;
        case FOR: return true;
        case FOREACH: return true;
        //keywords
        case BREAK: return true;
        case CONTINUE: return true;
        case RETURN: return true;
        //decl
        case VARIABLE: return true;
        case CONST: return true;
        case LINE_END: return true;
        case RBRACE: return true;
        case PROG_END: return true;
        case END_OF_FILE: return true;


        case IDENTIFIER: 
            switch (lookahead) {
                case ASSIGN: return true;
                default: return false;
            }
        default: return false;
    }
}

//important shared functions

//handles token descent & rule marking
bool pushToken(Symbol *s, Rule r){
    if(r == END_RULE) curDepth--;
    if(!push(tokenStack, s, r)){
        if(r == END_RULE) curDepth++;
        printf("[parserUtils.c | %s: %d]: Failed to push symbol in to stack \n", __func__, __LINE__);
        return false;   
    } 
    if(r != NONE && r != END_RULE) {
        curDepth++;
    }
    if(isDelim(s)) {
        lastSyncTokIndex = tokenStack->curSize - 1;
    }
        // Increment only for actual tokens, NOT END_RULE markers with NULL symbol
    return true;
}

void unwindToLastSync(int stmtDepth){
    int old = tokenStack -> curSize;
    int cut = lastSyncTokIndex + 1;
    
    while(cut < old 
        && tokenStack->data[cut].rule == END_RULE
        && tokenStack->data[cut].sym == NULL) cut++;
    
    if(cut < old){
        Entry* first = &tokenStack->data[cut];
        curDepth = (first->rule == END_RULE)? first->depth + 1 : first->depth++;
        for(int i = cut; i < old; i++) resetEntry(&tokenStack->data[i]);
        tokenStack->curSize = cut;
    }

    while(curDepth > stmtDepth) {
        pushToken(NULL, END_RULE);
    }
}
int getDepth() {
    return curDepth;
}
bool closeAll(){
    while(curDepth > 0) pushToken(NULL, END_RULE);
}