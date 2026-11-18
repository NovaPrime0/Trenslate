#ifndef SYMTBL
#define SYMTBL
typedef struct{
    char* lexeme;
    char* token;
    int line;
} Symbol;

typedef struct{
    Symbol *data;
    int curIdx;
    int maxSize;
} SymTBL;
SymTBL* createTable();
void addSymbol(SymTBL *st, Symbol *s);
bool contains(SymTBL *st, Symbol *s);
#endif