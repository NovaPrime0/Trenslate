# Silent Accept Bugs Report

**Generated:** 2026-11-25  
**Status:** Ready for fixing (requires `else { syntaxErr(...) }` branches)

---

## HIGH Severity (Must Fix Immediately)

| Function | Line Range | Issue |
|----------|------------|-------|
| `parseParamList()` | 216-220 | No IDENTIFIER match → silent accept |
| `parseAssignStmt()` | 329-336 | Neither ASSIGN nor Noise_AY matches → silent accept |
| `parseRelOp()` | 401-412 | None of Rel_* tokens match → silent accept |
| `parseLiteral()` | 480-502 | None of literal types match → silent accept |

## MEDIUM Severity (Fix Soon)

| Function | Line Range | Issue |
|----------|------------|-------|
| `parseValue()` | 197-203 | Neither LBRACKET nor Expr matches → silent accept |
| `parseReturnStmt()` | 309-315 | LINE_END matches but Expr doesn't → silent accept |
| `parseArithExpr()` | 415-426 | Op_ADD or Op_SUB doesn't match → silent accept |
| `parseMul()` | 429-437 | Op_DIV, Op_MULT, or Op_DIV_INT doesn't match → silent accept |
| `parseTerm()` | 459-468 | Neither LPAREN nor IDENTIFIER matches → silent accept |
| `parseArrayLit()` | 504-513 | RBRACKET missing or Value fails → silent accept |
| `parseArgList()` | 530-536 | First expression fails or COMMA found but Expr fails → silent accept |

## LOW Severity (Fix Later)

| Function | Line Range | Issue |
|----------|------------|-------|
| `parseCallExpr()` | 520-527 | RPAREN matches but ArgList fails → silent accept |

---

**Total: ~14 functions with silent accept bugs to fix**

---

## Syncronization Bug (Critical)

### Issue: `syntaxErr()` does NOT restore `curr` position
After recovery, parser continues from wrong token position.

### Current Implementation:
```c
void syntaxErr(Symbol s){
    Entry *m = pop(tokenStack, NULL);  // Get marker entry
    while(!pop(tokenStack, working) && working->rule != END_RULE);
    push(tokenStack, m, NONE);  // Restore marker
}
// ❌ Does NOT restore curr!
```

### Fix Required:
Restore `curr` to the marker's saved position after rollback.

---

## Files Modified (Backup Version)
- **parser.c** - Contains all silent accept bugs + sync bug