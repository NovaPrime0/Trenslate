#include "symtbl.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "parserUtils.h"
#include <string.h>
#include "enums.h"

const char* ruleNames[] = {
    "PROGRAM", "STMT", "DECL_STMT",
    "VAR_DECL", "CONST_DECL", "IMPORT",
    "VALUE", "FUNCTION_DECL", "PARAM_LIST",
    "BLOCK_STMT", "INLINE_STMT", "IF_STMT",
    "ELSE_CHAIN", "WHILE_STMT", "FOR_STMT",
    "FROEACH_STMT", "KEYWORD_STMT", "RETURN_STMT",
    "CONTINUE_STMT", "BREAK_STMT",
    "ASSIGN_STMT", "EXPR", "OR_EXPR",
    "AND_EXPR", "REL_EXPR", "REL_OP",
    "ARITH_EXPR", "MUL", "UNARY",
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


static Entry *startSymbol;
static Stack *tokenStack;
// static RuleStack *ruleStack;
static int curDepth;

const char* getName(Rule r){
    return ruleNames[r-1];
}


void resetEntry(Entry* s){
    // if(s->children) free(s->children);
    memset(s, 0, sizeof(Entry));
}

bool peek(Stack *s, Entry **w){
    if(!s || !w) return false;
    *w = &s->data[s->curSize-1];
    return true; 
}

bool expand(Stack *s){
    int newSize = s->maxSize*2;
    s->data = realloc(s->data, sizeof(Entry)*newSize);
    if(!s->data){
        printf("[perserUtils.c: %d]: Failed to allocate stack", __LINE__);
        return false;
    }
    s->maxSize = newSize;
    for(int i = s->curSize; i < s->maxSize; i++) resetEntry(&s->data[i]);
    return true;
}

bool initStack(Stack  *s){
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
    }
    printf("[parserUtils.c | %s: %d]: Attaching data to passed address \n", __func__, __LINE__);
    
    Entry *poppedNode = &st->data[st->curSize-1];
    w->sym = poppedNode->sym;
    w->rule = poppedNode->rule;
    w->depth = poppedNode->depth;
    // w->parent = poppedNode -> parent;
    // w->children = poppedNode -> children;
    // w->childrenCount = poppedNode -> childrenCount;
    // w->childrenMaxCount = poppedNode -> childrenMaxCount;
    printf("[parserUtils.c | %s: %d]: Popped Entry: [%s|%s], Rule: %s\n",
        __func__, __LINE__,
         w->sym->lexeme, 
         w->rule);
    
    resetEntry(poppedNode);
    return true;
}

bool push(Stack *st, Symbol *s, Rule r){
    if(st->curSize + 1 > st->maxSize && !expand(st)) return false;
    Entry *w = &st->data[st->curSize++];
    if(r > 0 && r != END_RULE) curDepth++;
    w->depth = curDepth;
    w->sym = s;
    w->rule = r;
    if(st->curSize == 1) startSymbol = &st->data[st->curSize-1];
    return true;
}

bool recover(){
    Entry *working;
    do{
        if(!pop(tokenNames, &working)) return false;
    } while(working->sym->token != LINE_END || working->rule != RBRACE); 
    pop(tokenStack, NULL);
    return true;
}
//data is only a wrapper for symTBL symbol for AST, can free only the data without freeing the symbol inside
void freeStack(Stack *s){
    free(s->data);
    free(s); 
}

// bool appendChild(Entry *parent, Entry *child){
//     if(!parent || !child)  return false;

//     if(parent->childrenMaxCount == 0 && parent->childrenCount && !parent->children){
//         int newSize = 2;
//         parent->children = malloc(sizeof(Entry*)*newSize);
//         if(!parent->children) return false;
//         parent->childrenMaxCount = newSize;
//     }

//     if(parent->childrenCount + 1 > parent->childrenMaxCount){
//         int newSize = parent->childrenMaxCount*2;
//         parent->children = realloc(parent->children, sizeof(Entry*)*newSize);
//         if(!parent->children){
//             printf("[parserUtils.c: %d]: Child Array Reallocation fail\n", __LINE__);
//             return false;
//         }
//         parent->childrenMaxCount = newSize;
//         for(int i = parent->childrenCount; i < parent->childrenMaxCount; i++) 
//             parent->children[i] = NULL;
//     }
//     child->parent = parent;
//     parent->children[parent->childrenCount++] = child;
//     return true;
// }

// bool expandRule(RuleStack *s){
//     int newSize = s->maxSize*2;
//     s->data = realloc(s->data, sizeof(Entry*)*newSize);
//     if(!s->data){
//         printf("[perserUtils.c: %d]: Failed to allocate stack", __LINE__);
//         return false;
//     }
//     s->maxSize = newSize;
//     for(int i = s->curSize; i < newSize; i++){
//         s->data[i] = NULL;
//     }
//     return true;
// }

// bool initRuleStack(RuleStack  *s){
//     int newSize = 2;
//     s -> data = malloc(sizeof(Entry*)*2);
//     if(!s->data){
//         printf("[perserUtils.c: %d]: Failed to allocate stack", __LINE__);
//         return false;
//     }
//     s->curSize = 0;
//     s->maxSize = newSize;
//     memset(s->data, 0, sizeof(Entry*)*s->maxSize);
//     return true;
// }

// //data is only a wrapper for symTBL symbol for AST, can free only the data without freeing the symbol inside
// void freeRuleStack(RuleStack *s){
//     free(s->data); 
//     free(s);
// }
//keep for when we make an actual parse tree
// //popping rule upon completion of production rule
// bool popRule(RuleStack *st, Entry **s){
//     if(!st || st->curSize == 0) {
//         printf("[parserUtils.c: %d]: Null RuleStack || No elements\n", __LINE__);
//         return false;
//     } else if (!*s){
//         printf("[parserUtils.c | %s: %d]: Null Entry, deducting from rulestack instead \n", __func__, __LINE__);
//         st->data[--st->curSize] = NULL;
//         return true;
//     }
//     printf("[parserUtils.c: %d]: Copying popped data to Entry \n", __LINE__);
//     *s = st->data[--st->curSize];
//     return true;
// }
// bool peekRule(RuleStack *st, Entry **s){
//     if(!st || st->curSize == 0) {
//         printf("[parserUtils.c: %d]: Null RuleStack || No elements\n", __LINE__);
//         return false;

//     } else if (!*s){
//         printf("[parserUtils.c | %s: %d]: Null Entry; Cannot peek at data \n", __func__, __LINE__);
//         return false;
//     }
//     printf("[parserUtils.c: %d]: Copying popped data to Entry \n", __LINE__);
//     *s = st->data[st->curSize-1];
//     return true;
// }

// bool appendChildToRule(RuleStack *st, Entry *s){
//     if(!st||!s) {
//         printf("[parserUtils.c | %s: %d]: Null RuleStack || Entry \n", __func__, __LINE__);
//         return false;
//     }
//     Entry *working;
//     peekRule(st, &working);
//     appendChild(working, s);
//     return true;
// }

// //pushing rule when descending into a production rule (LL(n))
// bool pushRule(RuleStack *s, Entry* a){
//     if(!s||!a) {
//         printf("[parserUtils.c | %s: %d]: Null RuleStack || Entry \n", __func__, __LINE__);
//         return false;
//     }
//     if(s->curSize + 1 > s->maxSize) expandRule(s);
//     s->data[s->curSize++] = a;
//     return true;    
// }

// //pop all until a sync token has been found, work on 
// bool startParse(){
    
//     if(!initStack(tokenStack) || !initRuleStack(ruleStack)) return false;
//     curDepth = 0;
//     return true;    
// }

// //define AST functions
// bool appendToken(Symbol* sym, Rule r){
//     if(!push(tokenStack, sym, r)) return false;
//     if(r != 0) {
//         Entry* peekNode;
//         if(!peek(tokenStack, &peekNode) || 
//             !pushRule(ruleStack, peek(tokenStack, peekNode)))
//         return false;
//     }
//     else if(r == 0) {
//         Entry* peekNode;
//         if(!peek(tokenStack, &peekNode) || 
//             !appendChildToRule(ruleStack, peek(tokenStack, peekNode))) 
//             return false;
//     }
//     return true;
// }