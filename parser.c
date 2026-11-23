#include <stdio.h>
#include "enums.h"
#include "symtbl.h"
#include "parserUtils.h"

/* SYNTACTIC ANALYZER 1 Utils
    Recursive Descent Parser
        - Production rule functions
        - expect & syntax error reproting
*/


static SymTBL* lexxedTable;
static int curr;
char ptree[1024];
char* sourcefile;

void initParser(SymTBL* tbl, char* sf){
    lexxedTable = tbl;
    curr = 0;
    sourcefile = sf;
}

//add a way to safeguard end-of-table
static TokenType peekTok(int i){
    return lexxedTable->data[curr+i].token; 
}
static Symbol *peekSym(int i){
    return &lexxedTable->data[curr+i]; 
}

static int match(TokenType t){
    if(curr > lexxedTable->curIdx) return -1;
    return peekTok(0) == t;
}
void syntaxErr(Symbol *s, TokenType expectedTok){
    printf("====================================================\n");
    printf("SYNTAX ERROR detected in %s, line %d \n", sourcefile, s->line);
    printf("  -> Received Token: %s (Type ID: %d)\n", tokenNames[(int)s->token], s->token);
    printf("  -> Expected Token: %s (Type ID: %d)\n", tokenNames[expectedTok], expectedTok);
    printf("====================================================\n");
}

static int matchOffset(TokenType t, int offset){

    if(offset + curr > lexxedTable->curIdx) return -1;
    printf("[parser.c | %s: %d]: Lookahead %d Token(s)\n", __func__, __LINE__, offset);
    TokenType tok = peekTok(curr + offset);
    printf("[parser.c | %s: %d]: Found %s \n", __func__, __LINE__, tokenNames[tok]);
    
    return tok == t;
}

//attach parserutils methods
static int expectRule(TokenType target, Rule r){
    printf("[parser.c | %s: %d]: Expecting %s to attach <%s> \n", __func__, __LINE__, tokenNames[target], getName(r));
    if(peekTok(0) == target) {
        printf("[parser.c | %s: %d]: Token matched with current pointer \n", __func__, __LINE__);
        curr++;
    }
    else syntaxErr(peekSym(0), target);
}

static int expect(TokenType target){
    printf("[parser.c | %s: %d]: Expecting %s \n", __func__, __LINE__, tokenNames[target]);
    if(peekTok(0) == target) {
        printf("[parser.c | %s: %d]: Token matched with current pointer \n", __func__, __LINE__);
        curr++;
    }
    else syntaxErr(peekSym(0), target);
}

static void parseProgram();
static void parseStmt();
static void parseDeclarationStmt();
static void parseVarDecl();
static void parseConstDecl();
static void parseImport();
static void parseValue();
static void parseFunctionDecl();
static void parseParamList();
static void parseBlockStmt();
static void parseInlineStmt();
static void parseIfStmt();
static void parseElseChain();
static void parseWhileStmt();
static void parseForStmt();
static void parseForeachStmt();
static void parseKeywordStmt();
static void parseReturnStmt();
static void parseContinueStmt();
static void parseBreakStmt();
static void parseAssignStmt();
static void parseExpr();
static void parseOrExpr();
static void parseAndExpr();
static void parseRelExpr();
static void parseRelOp();
static void parseArithExpr();
static void parseMul();
static void parseUnary();
static void parsePower();
static void parseTerm();
static void parseIdentTerm();
static void parseLiteral();
static void parseArrayLit();
static void parseCallExpr();
static void parseCallStmt();
static void parseArgList();
static void parseBlock();




static void parseProgram(){
    while(match(IMPORT)){
        parseImport();
    }
    while(match(FUNCTION)){
        parseFunctionDecl();
    }
    expect(PROG_MAIN);
    expect(LBRACE);
    while (!match(PROG_END)){
        parseStmt();
    }
    
    //expect <stmt>
    expect(PROG_END);
    expect(LINE_END);
    expect(RBRACE);
    while(match(FUNCTION)){
        parseFunctionDecl();
    }
}

static void parseStmt(){
    //if not block then parse inline basically since block stmts start with an easy-to-check keyword
    if(match(IF) || match(WHILE) || match(FOR) || match(FOREACH)) parseBlockStmt();
    else parseInlineStmt();

}

static void parseDeclarationStmt(){
    if(match(VARIABLE)){
        parseVarDecl();
    }else if(match(CONST)){
        parseConstDecl();
    }
}

static void parseVarDecl(){
    expect(VARIABLE);
    expect(IDENTIFIER);
    while(match(LBRACKET)){
        expect(LBRACKET);
        if (!match(RBRACKET)) parseExpr();
        expect(RBRACKET);
    }
    if(match(ASSIGN)){
        expect(ASSIGN);
        parseValue();
    } else if(match(Noise_AY)){
        expect(Noise_AY);
        parseValue();
    }
}

static void parseConstDecl(){
    expect(CONST);
    expect(IDENTIFIER);
    while(match(LBRACKET)){
        expect(LBRACKET);
        if (!match(RBRACKET)) parseExpr();
        expect(RBRACKET);
    }
    if(match(ASSIGN)){
        expect(ASSIGN);
        parseValue();
    } else if(match(Noise_AY)){
        expect(Noise_AY);
        parseValue();
    } else syntaxErr(peekSym(0), ASSIGN);
}

static void parseImport(){
    expect(IMPORT);
    expect(Lit_STRING);
    expect(LINE_END);
}

static void parseValue(){
    if(match(LPAREN)){
        parseArrayLit();
    } else {
        parseExpr();
    }
}



static void parseFunctionDecl(){
    expect(FUNCTION);
    expect(IDENTIFIER);
    expect(LPAREN);
    parseParamList();
    expect(RPAREN);
    parseBlock();
}

static void parseParamList(){
    if(match(IDENTIFIER)){
        expect(IDENTIFIER);
        while(match(COMMA)){
            expect(COMMA);
            expect(IDENTIFIER);
        }
    }
}

static void parseBlockStmt(){
    if(match(IF)){
        parseIfStmt();
    } else if(match(WHILE)){
        parseWhileStmt();
    } else if(match(FOR)){
        parseForStmt();
    } else if(match(FOREACH)){
        parseForeachStmt();
    }
}

static void parseInlineStmt(){
    if(match(IDENTIFIER)){
        if(matchOffset(ASSIGN, 1)) parseAssignStmt();
        else if(matchOffset(LPAREN, 1)) parseCallStmt();
    } else if(match(VARIABLE) || match(CONST)){
        parseDeclarationStmt();
    } else {
        parseKeywordStmt();
    }
    expect(LINE_END);
}

static void parseIfStmt(){
    expect(IF);
    expect(LPAREN); 
    parseExpr();
    expect(RPAREN);
    parseBlock();
    if(match(ELSE)){
        parseElseChain();
    }
}

static void parseElseChain(){
    expect(ELSE);
    if(match(IF)){
        parseIfStmt();
    } else parseBlock();
}
static void parseWhileStmt(){
    expect(WHILE);
    expect(LPAREN);
    parseExpr();
    expect(RPAREN);
    parseBlock();
}

static void parseForStmt(){
    expect(FOR);
    expect(LPAREN);
    expect(IDENTIFIER);
    if(match(Noise_MULA)) expect(Noise_MULA);
    parseExpr();
    expect(For_LIM);
    parseExpr();
    expect(RPAREN);
    parseBlock();
}

static void parseForeachStmt(){
    expect(FOREACH);
    expect(LPAREN);
    expect(IDENTIFIER);
    if(match(Noise_MULA)) expect(Noise_MULA);
    parseExpr();
    expect(RPAREN);
    parseBlock();
}

static void parseKeywordStmt(){
    if(match(RETURN)) parseReturnStmt();
    else if(match(BREAK)) parseBreakStmt();
    else if(match(CONTINUE)) parseContinueStmt();
}

static void parseReturnStmt(){
    expect(RETURN);
    if(!match(LINE_END)) parseExpr();
}

static void parseContinueStmt(){
    expect(CONTINUE);
}

static void parseBreakStmt(){
    expect(BREAK);
}

static void parseAssignStmt(){
    expect(IDENTIFIER);
    while(match(LBRACKET)){
        expect(LBRACKET);
        parseExpr();
        expect(RBRACKET);
    }
    if(match(ASSIGN)) expect(ASSIGN);
    else if(match(Noise_AY)) expect(Noise_AY);
    parseValue();
}

static void parseExpr(){
    parseOrExpr();
}

static void parseOrExpr(){
    parseAndExpr();
    while(match(Cond_OR)){
        expect(Cond_OR);
        parseAndExpr();
    }
}

static void parseAndExpr(){
    parseRelExpr();
    while(match(Cond_AND)){
        expect(Cond_AND);
        parseRelExpr();
    }
}

static void parseRelExpr(){
    parseArithExpr();
    if(match(Noise_AY)){
        expect(Noise_AY);
        parseRelOp();
        parseArithExpr();
    } else if(match(Rel_G) || match(Rel_GEQ) || 
            match(Rel_NEQ) || match(Rel_EQ) || 
            match(Rel_L) || match(Rel_LEQ)) {
        parseRelOp();
        parseArithExpr();
    }
}

static void parseRelOp(){
    if(match(Rel_G)){
        expect(Rel_G);
    } else if(match(Rel_GEQ)){
        expect(Rel_GEQ);
    } else if(match(Rel_NEQ)){
        expect(Rel_NEQ);
    } else if(match(Rel_EQ)){
        expect(Rel_EQ);
    } else if(match(Rel_L)){
        expect(Rel_L);
    } else if(match(Rel_LEQ)){
        expect(Rel_LEQ);
    }
}

static void parseArithExpr(){
    parseMul();
    int checkAdd = match(Op_ADD);
    int checkSub = match(Op_SUB); 
    while(checkAdd || checkSub){
        if(checkAdd) expect(Op_ADD);
        else if (checkSub) expect(Op_SUB);
        
        parseMul();
    }
}

static void parseMul(){
    parseUnary();
    int checkDiv = match(Op_DIV);
    int checkMult = match(Op_MULT);
    int checkDivInt = match(Op_DIV_INT);
    
    while (checkDiv || checkMult || checkDivInt){
        if(checkDiv) expect(Op_DIV);
        else if(checkMult) expect(Op_MULT);
        else if(checkDivInt) expect(Op_DIV_INT);
        parseUnary();
    }
    
}

static void parseUnary(){
    if (match(Op_SUB)){
        expect(Cond_NOT);
        parseUnary();
    } else if(match(Cond_NOT)){
        expect(Cond_NOT);
        parseUnary();
    } else {
        parsePower();
    }
}

static void parsePower(){
    parseTerm();
    if(match(Op_EXP)){
        expect(Op_EXP);
        parseUnary();
    }
}

static void parseTerm(){
    if(match(LPAREN)){
        expect(LPAREN);
        parseExpr();
        expect(RPAREN);
    } else if(match(IDENTIFIER)){
        if(matchOffset(LPAREN,1)) parseCallExpr();
        else parseIdentTerm();
    }
}

static void parseIdentTerm(){
    expect(IDENTIFIER);
    while(match(LBRACKET)){
        expect(LBRACKET);
        parseExpr();
        expect(RBRACKET);
    }
}

static void parseLiteral(){
    if(match(Lit_INT)){
        expect(Lit_INT);
    } else if(match(Lit_DEC)) {
        expect(Lit_DEC);
    } else if(match(Lit_CHAR)) {
        expect(Lit_CHAR);
    } else if(match(Lit_STRING)) {
        expect(Lit_STRING);
    } else if(match(T_TRUE)) {
        expect(T_TRUE);
    } else if(match(T_FALSE)) {
        expect(T_FALSE);
    } else if(match(T_NULL)) {
        expect(T_NULL);
    }
}

static void parseArrayLit(){
    expect(LBRACKET);
    if(!match(RBRACKET)){
        parseValue();
        while(match(COMMA)){
            expect(COMMA);
            parseValue();
        }
    }
    expect(RBRACKET);
}

static void parseCallExpr(){
    expect(IDENTIFIER);
    expect(LPAREN);
    if(!match(RPAREN)){
        parseArgList();
    }
    expect(RPAREN);
}

static void parseCallStmt(){
    if(matchOffset(Noise_NG, 1)){
        expect(IDENTIFIER);
        expect(Noise_NG);
        parseArgList();
    } else parseCallExpr();
}

static void parseArgList(){
    parseExpr();
    while(match(COMMA)){
        expect(COMMA);
        parseExpr();
    }
}

static void parseBlock(){
    expect(LBRACE);
    while(!match(RBRACE)) parseStmt();
    expect(RBRACE);
}