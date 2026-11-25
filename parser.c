#include <stdio.h>
#include "enums.h"
#include "symtbl.h"
#include "parserUtils.h"
#include <stdbool.h>

/* SYNTACTIC ANALYZER 1 Utils
    Recursive Descent Parser
        - Production rule functions
        - expect & syntax error reproting
*/


SymTBL* lexxedTable;
int curr;
bool errorInProgress;  // Flag to skip expect/match on error
char ptree[1024];
char* sourcefile;
int errCount; 
bool isEof;

bool initParser(SymTBL* tbl, char* sf){
    if(!tbl){
        printf("[parser.c | %s: %d]: Received Null Symbol Table \n", __func__, __LINE__);
        return false;
    }
    if(!startTokenStack()){
        printf("[parser.c | %s: %d]: Failed to create Token Stack \n", __func__, __LINE__);
        return false;
    }
    lexxedTable = tbl;
    curr = 0;
    sourcefile = sf;
    errCount = 0;
    return true;
}

bool parse(){
    parseProgram();
    if(errCount > 0) return false;
    return true;
}

//add a way to safeguard end-of-table
TokenType peekTok(int i){
    return lexxedTable->data[curr+i].token; 
}
static Symbol *peekSym(int i){
    return &lexxedTable->data[curr+i]; 
}

bool match(TokenType t){
    if(errorInProgress || curr > lexxedTable->curIdx) {
        // Auto-fail and attempt recovery
        return false;
    }
    return peekTok(0) == t;
}
void syntaxErrEOF(Symbol* s, TokenType expectedTok){
    printf("====================================================\n");
    printf("SYNTAX ERROR (Unexpected EOF) detected in %s, line %d \n", sourcefile, s->line);
    printf("  -> Expected Token: %s (Type ID: %d)\n", tokenNames[expectedTok], expectedTok);
    printf("====================================================\n");
    errorInProgress = true;  // Set flag to skip expect/match
    isEof = true;  // Set flag to skip expect/match
    errCount++;
}
void syntaxErr(Symbol *s, TokenType expectedTok){
    printf("====================================================\n");
    printf("SYNTAX ERROR detected in %s, line %d \n", sourcefile, s->line);
    printf("  -> Received Token: %s (Type ID: %d)\n", tokenNames[(int)s->token], s->token);
    printf("  -> Expected Token: %s (Type ID: %d)\n", tokenNames[expectedTok], expectedTok);
    printf("====================================================\n");
    errorInProgress = true;  // Set flag to skip expect/match
    errCount++;
}

void consumeToNextSync(int startIdx){
    int depth = 0;
    while(curr < lexxedTable->curIdx){
        Symbol *s = &lexxedTable->data[curr];
        TokenType next = lexxedTable->data[curr + 1].token;  

        if(s->token == LBRACE) depth++;
        else if(s->token == RBRACE){
            if(depth == 0) return;              
            curr++;
            if(--depth == 0) return;            
            continue;
        }
        else if(depth == 0 && isSync(s, next)){
            if(s->token == LINE_END){ curr++; return; }
            if(curr > startIdx) return;        
        }
        curr++;
    }
}

bool matchOffset(TokenType t, int offset){

    if(offset + curr > lexxedTable->curIdx) return false;
    printf("[parser.c | %s: %d]: Lookahead %d Token(s)\n", __func__, __LINE__, offset);
    TokenType tok = peekTok(curr);
    printf("[parser.c | %s: %d]: Found %s \n", __func__, __LINE__, tokenNames[tok]);
    
    return tok == t;
}

// Sync point restoration - pops stack until sync token (LINE_END or RBRACE)

//attach parserutils methods
void expectRule(TokenType target, Rule r){
    if(errorInProgress) {
        return;
    }
    TokenType tok = peekTok(0);
    printf("[parser.c | %s: %d]: Expecting %s (terminal) to attach <%s> rule\n", __func__, __LINE__, tokenNames[target], getName(r));
    if (tok == END_OF_FILE && target != END_OF_FILE) {
        syntaxErrorEOF(peekSym(0), target);
        return;
    }
    if(tok == target) {
        printf("[parser.c | %s: %d]: Token matched with current pointer \n", __func__, __LINE__);
        pushToken(&lexxedTable->data[curr++], r);
    }
    else syntaxErr(peekSym(0), target);
}
bool markRule(Rule r){
    if(errorInProgress) return false;
    printf("[parser.c | %s: %d]: Marking Rule Start for <%s>: \n", __func__, __LINE__, getName(r));
    if(!pushToken(NULL, r)) return false;
    return true;
}

void expect(TokenType target){
    if(errorInProgress) {
        return;
    }
    TokenType tok = peekTok(0);
    printf("[parser.c | %s: %d]: Expecting %s \n", __func__, __LINE__, tokenNames[target]);
    if (tok == END_OF_FILE && target != END_OF_FILE) {
        syntaxErrorEOF(peekSym(0), target);
        return;
    }
    if(tok == target) {
        printf("[parser.c | %s: %d]: Token matched with current pointer \n", __func__, __LINE__);
        pushToken(&lexxedTable->data[curr++], NONE);
    }  else syntaxErr(peekSym(0), target);
}

void synchronize(int startIdx, int blockDepth){
    unwindToLastSync(blockDepth);
    consumeToNextSync(startIdx);
    errorInProgress = false;
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
    markRule(PROGRAM);
    int topDepth = getDepth();
    while(match(IMPORT)){
        int startIdx = curr;
        parseImport();
        if(errorInProgress) synchronize(curr, topDepth);
    }
    
    while(match(FUNCTION)){
        int startIdx = curr;
        parseFunctionDecl();
        if(errorInProgress) synchronize(startIdx, topDepth);
    }
    
    expect(PROG_MAIN);
    expect(LBRACE);
    int stmtDepth = getDepth();
    if(errorInProgress) return;
    while (!match(PROG_END) && !match(END_OF_FILE)){
        int startIdx = curr;
        parseStmt();
        if(errorInProgress) synchronize(startIdx, stmtDepth);
        
    }
    
    expect(PROG_END);
    expect(LINE_END);
    expect(RBRACE);
    while(match(FUNCTION)){
        int startIdx = curr;
        parseFunctionDecl();
        if(errorInProgress) synchronize(startIdx, topDepth);
    }
    markRule(END_RULE);
}

static void parseStmt(){
    markRule(STMT);
    //if not block then parse inline basically since block stmts start with an easy-to-check keyword
    if(match(IF) || match(WHILE) || match(FOR) || match(FOREACH)) parseBlockStmt();
    else parseInlineStmt();
    markRule(END_RULE);
}

static void parseDeclarationStmt(){
    markRule(DECL_STMT);
    if(match(VARIABLE)){
        parseVarDecl();
    }else if(match(CONST)){
        parseConstDecl();
    }
    markRule(END_RULE);
}

static void parseVarDecl(){
    expectRule(VARIABLE, VAR_DECL);
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
    markRule(END_RULE);
}

static void parseConstDecl(){
    expectRule(CONST, CONST_DECL);
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
    markRule(END_RULE);
}

static void parseImport(){
    expectRule(IMPORT, IMPORT_STMT);
    expect(Lit_STRING);
    expectRule(LINE_END, END_RULE);
}

static void parseValue(){
    markRule(VALUE);
    if(match(LBRACKET)){
        parseArrayLit();
    } else {
        parseExpr();
    }
    markRule(END_RULE);
}

static void parseFunctionDecl(){
    expectRule(FUNCTION, FUNCTION_DECL);
    expect(IDENTIFIER);
    expect(LPAREN);
    parseParamList();
    expect(RPAREN);
    parseBlock();
    markRule(END_RULE);
}

static void parseParamList(){
    markRule(PARAM_LIST);
    if(match(IDENTIFIER)){
        expect(IDENTIFIER);
        while(match(COMMA)){
            expect(COMMA);
            expect(IDENTIFIER);
        }
    }
    markRule(END_RULE);
}

static void parseBlockStmt(){
    markRule(BLOCK_STMT);
    if(match(IF)){
        parseIfStmt();
    } else if(match(WHILE)){
        parseWhileStmt();
    } else if(match(FOR)){
        parseForStmt();
    } else if(match(FOREACH)){
        parseForeachStmt();
    }
    markRule(END_RULE);
}

static void parseInlineStmt(){
    markRule(INLINE_STMT);
    if(match(IDENTIFIER)){
        if(matchOffset(ASSIGN, 1) || matchOffset(Noise_AY, 1) || matchOffset(LBRACKET, 1)) parseAssignStmt();
        else if(matchOffset(LPAREN, 1)) parseCallStmt();
    } else if(match(VARIABLE) || match(CONST)){
        parseDeclarationStmt();
    } else {
        parseKeywordStmt();
    }
    expectRule(LINE_END, END_RULE);
}

static void parseIfStmt(){
    expectRule(IF, IF_STMT);
    expect(LPAREN); 
    parseExpr();
    expect(RPAREN);
    parseBlock();
    if(match(ELSE)){
        parseElseChain();
    }
    markRule(END_RULE);
}

static void parseElseChain(){
    expectRule(ELSE, ELSE_CHAIN);
    if(match(IF)){
        parseIfStmt();
    } else parseBlock();
    markRule(END_RULE);
}
static void parseWhileStmt(){
    expectRule(WHILE, WHILE_STMT);
    expect(LPAREN);
    parseExpr();
    expect(RPAREN);
    parseBlock();
    markRule(END_RULE);
}

static void parseForStmt(){
    expectRule(FOR, FOR_STMT);
    expect(LPAREN);
    expect(IDENTIFIER);
    if(match(Noise_MULA)) expect(Noise_MULA);
    parseExpr();
    expect(For_LIM);
    parseExpr();
    expect(RPAREN);
    parseBlock();
    markRule(END_RULE);
}

static void parseForeachStmt(){
    expectRule(FOREACH, FOREACH_STMT);
    expect(LPAREN);
    expect(IDENTIFIER);
    if(match(Noise_MULA)) expect(Noise_MULA);
    parseExpr();
    expect(RPAREN);
    parseBlock();
    markRule(END_RULE);
}

static void parseKeywordStmt(){
    markRule(KEYWORD_STMT);
    if(match(RETURN)) parseReturnStmt();
    else if(match(BREAK)) parseBreakStmt();
    else if(match(CONTINUE)) parseContinueStmt();
    markRule(END_RULE);
}

static void parseReturnStmt(){
    expectRule(RETURN, RETURN_STMT);
    if(!match(LINE_END)) parseExpr();
    markRule(END_RULE);
}

static void parseContinueStmt(){
    expectRule(CONTINUE, CONTINUE_STMT);
    markRule(END_RULE);
}

static void parseBreakStmt(){
    expectRule(BREAK, BREAK_STMT);
    markRule(END_RULE);
}

static void parseAssignStmt(){
    expectRule(IDENTIFIER, ASSIGN_STMT);
    while(match(LBRACKET)){
        expect(LBRACKET);
        parseExpr();
        expect(RBRACKET);
    }
    if(match(ASSIGN)) expect(ASSIGN);
    else if(match(Noise_AY)) expect(Noise_AY);
    parseValue();
    markRule(END_RULE);
}

static void parseExpr(){
    markRule(EXPR);
    parseOrExpr();
    markRule(END_RULE);
}

static void parseOrExpr(){
    markRule(OR_EXPR);
    parseAndExpr();
    while(match(Cond_OR)){
        expect(Cond_OR);
        parseAndExpr();
    }
    markRule(END_RULE); 
}

static void parseAndExpr(){
    markRule(AND_EXPR);
    parseRelExpr();
    while(match(Cond_AND)){
        expect(Cond_AND);
        parseRelExpr();
    }
    markRule(END_RULE);
}

static void parseRelExpr(){
    markRule(REL_EXPR);
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
    markRule(END_RULE);
}

static void parseRelOp(){
    markRule(REL_OP);
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
    markRule(END_RULE);
}

static void parseArithExpr(){
    markRule(ARITH_EXPR);
    parseMul();

    while(match(Op_ADD) || match(Op_SUB)){
        if(match(Op_ADD)) expect(Op_ADD);
        else if (match(Op_SUB)) expect(Op_SUB);
        parseMul();
    }
    markRule(END_RULE);
}

static void parseMul(){
    markRule(MUL);
    parseUnary();

    while (match(Op_DIV) ||  match(Op_MULT) || match(Op_DIV_INT)){
        if(match(Op_DIV)) expect(Op_DIV);
        else if(match(Op_MULT)) expect(Op_MULT);
        else if(match(Op_DIV_INT)) expect(Op_DIV_INT);
        parseUnary();
    }
    markRule(END_RULE);
}

static void parseUnary(){
    markRule(UNARY);
    if (match(Op_SUB)){
        expect(Op_SUB);
        parseUnary();
    } else if(match(Cond_NOT)){
        expect(Cond_NOT);
        parseUnary();
    } else {
        parsePower();
    }
    markRule(END_RULE);
}

static void parsePower(){
    markRule(POWER);
    parseTerm();
    if(match(Op_EXP)){
        expect(Op_EXP);
        parseUnary();
    }
    markRule(END_RULE);
}

static void parseTerm(){
    markRule(TERM);
    if(match(LPAREN)){
        expect(LPAREN);
        parseExpr();
        expect(RPAREN);
    } else if(match(IDENTIFIER)){
        if(matchOffset(LPAREN,1) || matchOffset(Noise_NG,1)) parseCallExpr();
        else parseIdentTerm();
    } else{
        parseLiteral();
    }
    markRule(END_RULE);
}

static void parseIdentTerm(){
    expectRule(IDENTIFIER, IDENT_TERM);
    while(match(LBRACKET)){
        expect(LBRACKET);
        parseExpr();
        expect(RBRACKET);
    }
    markRule(END_RULE);
}

static void parseLiteral(){
    markRule(LITERAL);
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
    markRule(END_RULE);
}

static void parseArrayLit(){
    expectRule(LBRACKET, ARRAY_LIT);
    if(!match(RBRACKET)){
        parseValue();
        while(match(COMMA)){
            expect(COMMA);
            parseValue();
        }
    }
    expectRule(RBRACKET, END_RULE);
}

static void parseCallExpr(){
    expectRule(IDENTIFIER, CALL_EXPR);
    expect(LPAREN);
    if(!match(RPAREN)){
        parseArgList();
    }
    expectRule(RPAREN, END_RULE);
}

static void parseCallStmt(){
    markRule(CALL_STMT);
    if(matchOffset(Noise_NG, 1)){
        expect(IDENTIFIER);
        expect(Noise_NG);
        parseArgList();
    } else parseCallExpr();
    markRule(END_RULE);
}

static void parseArgList(){
    markRule(ARG_LIST);
    parseExpr();
    while(match(COMMA)){
        expect(COMMA);
        parseExpr();
    }
    markRule(END_RULE);
}

static void parseBlock(){
    expectRule(LBRACE, BLOCK);
    if(errorInProgress) return;
    int blockDepth = getDepth();
    while(!match(RBRACE) && !match(END_OF_FILE) && !match(PROG_END)) {
        int startIdx = curr;
        parseStmt();
        if(errorInProgress) synchronize(startIdx, blockDepth);
    };
    expectRule(RBRACE, END_RULE);
}