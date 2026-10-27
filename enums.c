#include "enums.h"
#include "string.h"


const char *stateNames[] = {
    //1
    "S_FOR", "S_FOREACH", "S_RETURN",
    "S_ELSE", "S_IF", "S_WHILE",
    "S_FUNCTION", "S_BREAK", "S_CONTINUE", "S_INVALID",
    "S_VARIABLE", "S_PROG_MAIN", "S_PROG_END",
    "S_For_LIM", "S_T_NULL", "S_IMPORT",
    //2
    "S_Op_DIV_INT", "S_Rel_GEQ", "S_Rel_EQ",
    "S_Rel_LEQ", "S_Rel_NEQ",
    "S_Cond_AND", "S_Cond_OR", "S_Cond_NOT",
    //3
    "S_LINE_END", "S_LBRACE", "S_RBRACE",
    "S_LBRACKET", "S_RBRACKET",
    "S_LPAREN", "S_RPAREN",
    "S_COMMA", "S_DOT",
    "S_Op_ADD", "S_Op_SUB", "S_Op_DIV", "S_Op_EXP",
    "S_Op_MULT", "S_Op_MOD", "S_ASSIGN",
    "S_Rel_G", "S_Rel_L", 
    //4
    "S_Lit_STRING", "S_Lit_CHAR",
    "S_Lit_INT", "S_Lit_DEC",
    "S_TRUE", "S_FALSE",
    "S_CONST",
    //5
    "S_WHITESPACE", "S_NOISE", "S_C_BLOCK", "S_C_LINE",
    "S_IDENTIFIER",
    //state exclusives:
    "START", "INTERMEDIARY"
};
const char *tokenNames[] = {
    //1
    "FOR", "FOREACH", "RETURN",
    "ELSE", "IF", "WHILE",
    "FUNCTION", "BREAK", "CONTINUE", "INVALID",
    "VARIABLE", "PROG_MAIN", "PROG_END",
    "For_LIM", "T_NULL", "IMPORT",
    //2
    "Op_DIV_INT", "Rel_GEQ", "Rel_EQ",
    "Rel_LEQ", "Rel_NEQ",
    "Cond_AND", "Cond_OR", "Cond_NOT",
    //3
    "LINE_END", "LBRACE", "RBRACE",
    "LBRACKET", "RBRACKET",
    "LPAREN", "RPAREN",
    "COMMA", "DOT",
    "Op_ADD", "Op_SUB", "Op_DIV", "Op_EXP",
    "Op_MULT", "Op_MOD", "ASSIGN",
    "Rel_G", "Rel_L", 
    //4
    "Lit_STRING", "Lit_CHAR",
    "Lit_INT", "Lit_DEC",
    "TRUE", "FALSE",
    "CONST",
    //5
    "WHITESPACE", "NOISE", "C_BLOCK", "C_LINE"
    "IDENTIFIER",
};