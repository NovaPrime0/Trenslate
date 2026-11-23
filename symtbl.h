#ifndef SYMTBL_H
#define SYMTBL_H
#include <stdbool.h>
#include "enums.h"   // TokenType

typedef struct {
    char *lexeme;
    TokenType token;
    int line;
} Symbol;

typedef struct {
    Symbol *data;
    int curIdx;
    int maxSize;
} SymTBL;

SymTBL* createTable();
void addSymbol(SymTBL *st, Symbol *s);
bool contains(SymTBL *st, Symbol *s);
#endif