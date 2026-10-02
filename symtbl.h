#ifndef SYMTBL
#define SYMTBL
typedef struct{
    char* identifier;
    char* token;
    int line;
} Symbol;

typedef struct{
    Symbol *data;
    int curIdx;
    int maxSize;
} SymTBL;
static SymTBL* createTable();
static void addSymbol(SymTBL *st, Symbol *s);
static bool contains(SymTBL *st, Symbol *s);
#endif