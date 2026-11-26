#include <stdio.h>
#include "enums.h"
#include "symtbl.h"
#include "parserUtils.h"
#include <stdbool.h>
#define sErrFile "Output/SyntaxError.txt"
/* SYNTACTIC ANALYZER 1 Utils
    Recursive Descent Parser
        - Production rule functions
        - expect & syntax error reproting
*/


SymTBL* lexxedTable;
int curr;
bool errorInProgress;  // Flag to skip expect/match on error
char* sourcefile;
int errCount; 
bool isEof;
FILE* errDump;

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
    errorInProgress = false;
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
    if(errorInProgress) return false;
    printf("====================================================\n");
    printf("SYNTAX ERROR (Unexpected EOF) detected in %s, line %d \n", sourcefile, s->line);
    printf("  -> Expected Token: %s (Type ID: %d)\n", tokenNames[expectedTok], expectedTok);
    printf("====================================================\n");
    if(!errDump) {
        errDump = fopen(sErrFile, "w+");
        if(!errDump){
            printf("[parser.c | %s: %d]: Could not create Syntax error File \n", __func__, __LINE__);
            return;
        }
    }
    if(errDump){
        printf("====================================================\n");
        printf("SYNTAX ERROR (Unexpected EOF) detected in %s, line %d \n", sourcefile, s->line);
        printf("  -> Expected Token: %s (Type ID: %d)\n", tokenNames[expectedTok], expectedTok);
        printf("====================================================\n");
    }
    errorInProgress = true;  // Set flag to skip expect/match
    errCount++;
}
void syntaxErr(Symbol *s, TokenType expectedTok){
    if(errorInProgress) return false;
    printf("====================================================\n");
    printf("SYNTAX ERROR detected in %s, line %d \n", sourcefile, s->line);
    printf("  -> Received Token: %s (Type ID: %d)\n", tokenNames[(int)s->token], s->token);
    printf("  -> Expected Token: %s (Type ID: %d)\n", tokenNames[expectedTok], expectedTok);
    printf("====================================================\n");
    if(!errDump) {
        errDump = fopen(sErrFile, "w+");
        if(!errDump){
            printf("[parser.c | %s: %d]: Could not create Syntax error File \n", __func__, __LINE__);
            return;
        }
    }
    if(errDump){
        printf("====================================================\n");
        printf("SYNTAX ERROR detected in %s, line %d \n", sourcefile, s->line);
        printf("  -> Received Token: %s (Type ID: %d)\n", tokenNames[(int)s->token], s->token);
        printf("  -> Expected Token: %s (Type ID: %d)\n", tokenNames[expectedTok], expectedTok);
        printf("====================================================\n");
    }
    errorInProgress = true;  // Set flag to skip expect/match
    errCount++;
}
void syntaxRuleErr(Symbol *s, Rule target){
    if(errorInProgress) return;
    printf("====================================================\n");
    printf("SYNTAX ERROR detected in %s, line %d \n", sourcefile, s->line);
    printf("  -> Received Token: %s (Type ID: %d)\n", tokenNames[(int)s->token], s->token);
    printf("  -> Expected Encompassing Rule: %s (Type ID: %d)\n", getName(target), target);
    printf("====================================================\n");
    if(!errDump) {
        errDump = fopen(sErrFile, "w+");
        if(!errDump){
            printf("[parser.c | %s: %d]: Could not create Syntax error File \n", __func__, __LINE__);
            return;
        }
    }
    if(errDump){
        fprintf(errDump,"====================================================\n");
        fprintf(errDump,"SYNTAX ERROR detected in %s, line %d \n", sourcefile, s->line);
        fprintf(errDump,"  -> Received Token: %s (Type ID: %d)\n", tokenNames[(int)s->token], s->token);
        fprintf(errDump,"  -> Expected Encompassing Rule: %s (Type ID: %d)\n", getName(target), target);
        fprintf(errDump,"====================================================\n");
    }

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
            if(s->token == LINE_END) { curr++; return; }
            if(curr > startIdx) return;
        }
        curr++;
    }
}

bool matchOffset(TokenType t, int offset){
    if(offset + curr > lexxedTable->curIdx || errorInProgress) return false;
    printf("[parser.c | %s: %d]: Lookahead %d Token(s)\n", __func__, __LINE__, offset);
    TokenType tok = peekTok(offset);
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
        syntaxErrEOF(peekSym(0), target);
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
        syntaxErrEOF(peekSym(0), target);
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
        if(errorInProgress) synchronize(startIdx, topDepth);
    }
    
    while(match(FUNCTION)){
        int startIdx = curr;
        parseFunctionDecl();
        if(errorInProgress) synchronize(startIdx, topDepth);
    }
    
    expect(PROG_MAIN);
    expect(LBRACE);
    
    int stmtDepth = getDepth();
    while (!match(PROG_END) && !match(END_OF_FILE) && !match(RBRACE)){
        int startIdx = curr;
        parseStmt();
        if(errorInProgress) synchronize(startIdx, stmtDepth);
        
    }
    
    expect(PROG_END);
    expect(LINE_END);
    
    while(match(FUNCTION)){
        int startIdx = curr;
        parseFunctionDecl();
        if(errorInProgress) synchronize(startIdx, topDepth);
    }
    expect(RBRACE);

    markRule(END_RULE);
}

static void parseStmt(){
    if(errorInProgress) return;
    markRule(STMT);
    //if not block then parse inline basically since block stmts start with an easy-to-check keyword
    if(match(IF) || match(WHILE) || match(FOR) || match(FOREACH)) parseBlockStmt();
    else parseInlineStmt();
    markRule(END_RULE);
}

static void parseDeclarationStmt(){
    if(errorInProgress) return;
    markRule(DECL_STMT);
    if(match(VARIABLE)){
        parseVarDecl();
    }else if(match(CONST)){
        parseConstDecl();
    } else syntaxErr(&lexxedTable->data[curr], VARIABLE);
    markRule(END_RULE);
}

static void parseVarDecl(){
    if(errorInProgress) return;
    expectRule(VARIABLE, VAR_DECL);
    expect(IDENTIFIER);
    while(match(LBRACKET)){
        expect(LBRACKET);
        if (!match(RBRACKET)) parseExpr();
        expect(RBRACKET);
    }

    //fully optional assignment
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
    if(errorInProgress) return;
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
    if(errorInProgress) return;
    expectRule(IMPORT, IMPORT_STMT);
    expect(Lit_STRING);
    expectRule(LINE_END, END_RULE);
}

static void parseValue(){
    if(errorInProgress) return;
    markRule(VALUE);
    // should be safe
    if(match(LBRACKET)){
        parseArrayLit();
    } else {
        parseExpr();
    }
    markRule(END_RULE);
}

static void parseFunctionDecl(){
    if(errorInProgress) return;
    expectRule(FUNCTION, FUNCTION_DECL);
    expect(IDENTIFIER);
    expect(LPAREN);
    parseParamList();
    expect(RPAREN);
    parseBlock();
    markRule(END_RULE);
}

static void parseParamList(){
    if(errorInProgress) return;
    markRule(PARAM_LIST);
    //optional tree
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
    if(errorInProgress) return;
    markRule(BLOCK_STMT);
    //descended from Stmt, should be safe as one of these tokens caused the function to be called
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
    if(errorInProgress) return;
    markRule(INLINE_STMT);
    if(match(IDENTIFIER)){
        if(matchOffset(ASSIGN, 1) || matchOffset(Noise_AY, 1) || matchOffset(LBRACKET, 1)) parseAssignStmt();
        else if(matchOffset(LPAREN, 1) || matchOffset(Noise_NG, 1)) parseCallStmt();
        //hard to imply which one the user wants to call, so default to assign
        else syntaxErr(&lexxedTable->data[curr], ASSIGN);
    } else if(match(VARIABLE) || match(CONST)){
        parseDeclarationStmt();
    } else {
        parseKeywordStmt();
    }
    //line_end_Rule: IF inline, then expect that its rule ends with a ;
    expectRule(LINE_END, END_RULE);
}

static void parseIfStmt(){
    if(errorInProgress) return;
    expectRule(IF, IF_STMT);
    expect(LPAREN); 
    parseExpr();
    expect(RPAREN);
    parseBlock();
    //optional block
    if(match(ELSE)){
        parseElseChain();
    }
    markRule(END_RULE);
}

static void parseElseChain(){
    if(errorInProgress) return;
    //descended from else
    expectRule(ELSE, ELSE_CHAIN);
    //optional block or IF, next is not an if, then proceed with block
    if(match(IF)){
        parseIfStmt();
    } else parseBlock();
    markRule(END_RULE);
}
static void parseWhileStmt(){
    if(errorInProgress) return;
    expectRule(WHILE, WHILE_STMT);
    expect(LPAREN);
    parseExpr();
    expect(RPAREN);
    parseBlock();
    markRule(END_RULE);
}

static void parseForStmt(){
    if(errorInProgress) return;
    expectRule(FOR, FOR_STMT);
    expect(LPAREN);
    expect(IDENTIFIER);
    //optional noise word, if exist then we can consume it
    if(match(Noise_MULA)) expect(Noise_MULA);
    parseExpr();
    expect(For_LIM);
    parseExpr();
    expect(RPAREN);
    parseBlock();
    markRule(END_RULE);
}

static void parseForeachStmt(){
    if(errorInProgress) return;
    expectRule(FOREACH, FOREACH_STMT);
    expect(LPAREN);
    expect(IDENTIFIER);
    //optional noise word, if exist then we can consume it
    if(match(Noise_SA)) expect(Noise_SA);
    parseExpr();
    expect(RPAREN);
    parseBlock();
    markRule(END_RULE);
}

static void parseKeywordStmt(){
    if(errorInProgress) return;
    markRule(KEYWORD_STMT);
    if(match(RETURN)) parseReturnStmt();
    else if(match(BREAK)) parseBreakStmt();
    else if(match(CONTINUE)) parseContinueStmt();
    //throw error as this is full default descending from inline statement (rogue <expr>, etc.)
    else  syntaxRuleErr(&lexxedTable->data[curr], KEYWORD_STMT);
    markRule(END_RULE);
}

static void parseReturnStmt(){
    if(errorInProgress) return;
    expectRule(RETURN, RETURN_STMT);
    if(!match(LINE_END)) parseExpr();
    markRule(END_RULE);
}

static void parseContinueStmt(){
    if(errorInProgress) return;
    expectRule(CONTINUE, CONTINUE_STMT);
    markRule(END_RULE);
}

static void parseBreakStmt(){
    if(errorInProgress) return;
    expectRule(BREAK, BREAK_STMT);
    markRule(END_RULE);
}

static void parseAssignStmt(){
    if(errorInProgress) return;
    expectRule(IDENTIFIER, ASSIGN_STMT);
    while(match(LBRACKET)){
        expect(LBRACKET);
        parseExpr();
        expect(RBRACKET);
    }
    if(match(ASSIGN)) expect(ASSIGN);
    else if(match(Noise_AY)) expect(Noise_AY);
    else syntaxErr(&lexxedTable->data[curr], ASSIGN);
    parseValue();
    markRule(END_RULE);
}

static void parseExpr(){
    if(errorInProgress) return;
    markRule(EXPR);
    parseOrExpr();
    markRule(END_RULE);
}

static void parseOrExpr(){
    if(errorInProgress) return;
    markRule(OR_EXPR);
    parseAndExpr();
    while(match(Cond_OR)){
        expect(Cond_OR);
        parseAndExpr();
    }
    markRule(END_RULE); 
}

static void parseAndExpr(){
    if(errorInProgress) return;
    markRule(AND_EXPR);
    parseRelExpr();
    while(match(Cond_AND)){
        expect(Cond_AND);
        parseRelExpr();
    }
    markRule(END_RULE);
}

static void parseRelExpr(){
    if(errorInProgress) return;
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
    if(errorInProgress) return;
    markRule(REL_OP);
    //lookahead from relexpr already guarantees we have one of these tokens, but just in case we print a syntax Rule Err
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
    } else syntaxRuleErr(&lexxedTable->data[curr], REL_OP);
    markRule(END_RULE);
}

static void parseArithExpr(){
    if(errorInProgress) return;
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
    if(errorInProgress) return;
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
    if(errorInProgress) return;
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
    if(errorInProgress) return;
    markRule(POWER);
    parseTerm();
    if(match(Op_EXP)){
        expect(Op_EXP);
        parseUnary();
    } 
    markRule(END_RULE);
}

static void parseTerm(){
    if(errorInProgress) return;
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
    if(errorInProgress) return;
    expectRule(IDENTIFIER, IDENT_TERM);
    while(match(LBRACKET)){
        expect(LBRACKET);
        parseExpr();
        expect(RBRACKET);
    }
    markRule(END_RULE);
}

static void parseLiteral(){
    if(errorInProgress) return;
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
    } else syntaxRuleErr(&lexxedTable->data[curr], LITERAL);
    markRule(END_RULE);
}

static void parseArrayLit(){
    if(errorInProgress) return;
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
    if(errorInProgress) return;
    expectRule(IDENTIFIER, CALL_EXPR);
    expect(LPAREN);
    //if there is something inside paren, ident(), ident(arglist); must be a valid expr
    if(!match(RPAREN)){
        parseArgList();
    }
    expectRule(RPAREN, END_RULE);
}

static void parseCallStmt(){
    if(errorInProgress) return;
    markRule(CALL_STMT);
    if(matchOffset(Noise_NG, 1)){
        expect(IDENTIFIER);
        expect(Noise_NG);
        parseArgList();
    } else parseCallExpr();
    markRule(END_RULE);
}

static void parseArgList(){
    if(errorInProgress) return;
    markRule(ARG_LIST);
    parseExpr();
    while(match(COMMA)){
        expect(COMMA);
        parseExpr();
    }
    markRule(END_RULE);
}

static void parseBlock(){
    if(errorInProgress) return;
    expectRule(LBRACE, BLOCK);
    int stmtDepth = getDepth();
    while(!match(RBRACE) && !match(END_OF_FILE) && !match(PROG_END)) {
        int startIdx = curr;
        parseStmt();
        if(errorInProgress) synchronize(startIdx, stmtDepth);
    };
    expectRule(RBRACE, END_RULE);
}

bool parse(){
    parseProgram();
    if(errCount > 0) {
        closeAll();
        freeStack();
        return false;
    }
    closeAll();
    errorInProgress = false;
    freeStack();
    return true;
}