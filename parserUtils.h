#ifndef UTILS
#define UTILS
#include "symtbl.h"

typedef enum {
    NONE, PROGRAM, STMT, DECL_STMT,
    VAR_DECL, CONST_DECL, IMPORT_STMT,
    VALUE, FUNCTION_DECL, PARAM_LIST,
    BLOCK_STMT, INLINE_STMT, IF_STMT,
    ELSE_CHAIN, WHILE_STMT, FOR_STMT,
    FOREACH_STMT, KEYWORD_STMT, RETURN_STMT,
    CONTINUE_STMT, BREAK_STMT,
    ASSIGN_STMT, EXPR, OR_EXPR,
    AND_EXPR, REL_EXPR, REL_OP,
    ARITH_EXPR, MUL, UNARY, POWER,
    TERM, IDENT_TERM, LITERAL,
    ARRAY_LIT, CALL_EXPR, CALL_STMT,
    ARG_LIST, BLOCK, END_RULE
} Rule;


extern const char* ruleNames[];

typedef struct Entry Entry;

struct Entry{
    Symbol *sym;

    int depth;
    int rule;
    
    // ASTNode* parent;
    // int childrenCount;
    // int childrenMaxCount;
    // ASTNode** children;
};

typedef struct{
    int curSize;
    int maxSize;
    Entry* data;
} Stack;

const char* getName(Rule r);
bool peek(Stack *s, Entry **w);
bool initStack(Stack  *s);
bool pushToken(Symbol *s, Rule r);
void unwindToLastSync(int stmtDepth);
bool startTokenStack();
void freeStack();
bool isSync(Symbol* s, TokenType lookahead);
int getDepth();
void closeAll();
void printParseTree();

#endif