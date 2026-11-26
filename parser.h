#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "symtbl.h"

bool initParser(SymTBL *tbl, const char *sf);
bool parse(void);

#endif
