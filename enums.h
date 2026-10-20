#ifndef ENUMS_H
#define ENUMS_H
#define NO_TRANSITION -1
#define BAD_SYMBOL -2
#define SYMBOLCOUNT 14
#define WHITESPACECOUNT 3
#define DELIMCOUNT 9
#define varname(x) #x
extern const char SYMBOLS[];
extern const char WHITESPACES[];
extern const char DELIMETERS[];
extern const char *tokenNames[];
extern const char *stateNames[];
typedef enum{
    S_IDENTIFIER,
    //Arithmetic Operation Types
    S_Op_ADD, S_Op_SUB, S_Op_DIV, S_Op_EXP,
    S_Op_DIV_INT, S_Op_MULT, S_Op_MOD,
    //Relational operators
    S_Rel_G, S_Rel_GEQ, S_Rel_EQ,
    S_Rel_L, S_Rel_LEQ, S_Rel_NEQ,
    //Boolean Operators
    S_Cond_AND, S_Cond_OR, S_Cond_NOT,
    //Assignment Operators
    S_ASSIGN,
    //Comments
    S_C_LINE, S_C_BLOCK,
    //delimiters
    S_LINE_END, S_LBRACE, S_RBRACE,
    S_LBRACKET, S_RBRACKET,
    S_LPAREN, S_RPAREN,
    S_COMMA, S_DOT,
    S_For_LIM,
    S_T_NULL,
    //yes
    S_NOISE,
    S_IMPORT,
    //control flow
    S_FOR, S_FOREACH, S_RETURN,
    S_ELSE, S_IF, S_WHILE,
    S_FUNCTION, S_BREAK,
    S_VARIABLE,
    S_PROG_MAIN,
    S_PROG_END,
    //literals
    S_Lit_STRING, S_Lit_CHAR,
    S_Lit_INT, S_Lit_DEC,
    S_TRUE, S_FALSE,
    S_WHITESPACE,
    S_CONST,
    //state-exclusive types
    START,
    INTERMEDIARY
} StateType;
typedef enum{
    IDENTIFIER,
    //Arithmetic Operation Types
    Op_ADD, Op_SUB, Op_DIV, Op_EXP,
    Op_DIV_INT, Op_MULT, Op_MOD,
    //Relational operators
    Rel_G, Rel_GEQ, Rel_EQ,
    Rel_L, Rel_LEQ, Rel_NEQ,
    //Boolean Operators
    Cond_AND, Cond_OR, Cond_NOT,
    //Assignment Operators
    ASSIGN,
    //Comments
    C_LINE, C_BLOCK,
    //delimiters
    LINE_END, LBRACE, RBRACE,
    LBRACKET, RBRACKET,
    LPAREN, RPAREN,
    COMMA, DOT,
    For_LIM,
    T_NULL,
    //yes
    NOISE,
    IMPORT,
    //control flow
    FOR, FOREACH, RETURN,
    ELSE, IF, WHILE,
    FUNCTION, BREAK,
    VARIABLE,
    PROG_MAIN,
    PROG_END,
    //literals
    Lit_STRING, Lit_CHAR,
    Lit_INT, Lit_DEC,
    TRUE, FALSE,
    WHITESPACE,
    CONST,
} TokenType;

#endif