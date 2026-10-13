#include "enums.h"
const char SYMBOLS[] = {'-', '+', '/', '*','#', '&', '>', '<', '!', '=', '|',  '%',  '"', '\''};
const char WHITESPACES[] = {' ', '\n', '\t'};
const char DELIMETERS[] = {';', '(', ')','{', '}', '[', ']',};
const char *stateNames[] = {
    "START",
    "S_IDENTIFIER",
    //Arithmetic Operation Types
    "S_Op_ADD", "S_Op_SUB", "S_Op_DIV", 
    "S_Op_DIV_INT", "S_Op_MULT", "S_Op_MOD",
    //Relational operators
    "S_Cond_G", "S_Cond_GEQ", "S_Cond_EQ",
    "S_Cond_L", "S_Cond_LEQ", "S_Cond_NEQ",
    //Boolean Operators
    "S_Bool_AND", "S_Bool_OR", "S_Bool_NOT",
    //Assignment Operators
    "S_ASSIGN",
    //Comments
    "S_C_LINE", "S_WHITESPACE", "S_C_BLOCK",
    //delimiters
    "S_LINE_END", "S_LBRACE", "S_RBRACE",
    "S_LBRACKET", "S_RBRACKET",
    "S_LPAREN", "S_RPAREN",
    
    "S_T_NULL",
    //yes
    "S_NOISE",
    //control flow
    "S_FOR", "S_FOREACH", "S_RETURN",
    "S_ELSE", "S_IF", "S_WHILE",
    "S_FUNCTION", "S_BREAK",
    "S_VARIABLE",
    "S_PROG_MAIN",
    "S_PROG_END",
    //literals
    "S_Lit_STRING",
    "S_Lit_CHAR",
    "S_Lit_INT",
    "S_Lit_DEC",
    "S_INTERMEDIARY"
};
const char *tokenNames[] = {
    "IDENTIFIER",
    //Arithmetic Operation Types
    "Op_ADD", "Op_SUB", "Op_DIV", 
    "Op_DIV_INT", "Op_MULT", "Op_MOD",
    //Relational operators
    "Cond_G", "Cond_GEQ", "Cond_EQ",
    "Cond_L", "Cond_LEQ", "Cond_NEQ",
    //Boolean Operators
    "Bool_AND", "Bool_OR", "Bool_NOT",
    //Assignment Operators
    "ASSIGN",
    //Comments
    "C_LINE", "WHITESPACE", "C_BLOCK",
    //delimiters
    "LINE_END", "LBRACE", "RBRACE",
    "LBRACKET", "RBRACKET",
    "LPAREN", "RPAREN",

    "T_ NULL"
    //yes
    "NOISE",
    //control flow
    "FOR", "FOREACH", "RETURN",
    "ELSE", "IF", "WHILE",
    "FUNCTION", "BREAK",
    "VARIABLE",
    "PROG_MAIN",
    "PROG_END",
    //literals
    "Lit_STRING",
    "Lit_CHAR",
    "Lit_INT",
    "Lit_DEC",

};