# Parser Issues Checklist

## Pre-Testing Fixes (Items 1-6)

### ✅ Issue 1: parseProgram loop doesn't stop on RBRACE
**Location:** `parser.c` line ~328-340  
**Problem:** A stray `}` in the main body fails `parseStmt`, the skip returns at `}` without consuming it, and the same token fails forever.

**Fix Required:** Add `&& !match(RBRACE)` guard to stop parsing when RBRACE is encountered.

```c
// Current (line 328-340):
while (!match(PROG_END) && !match(END_OF_FILE) && !match(RBRACE)){
    int startIdx = curr;
    parseStmt();
    if(errorInProgress) synchronize(startIdx, stmtDepth);
}

// Fix: The guard already exists but the issue is that parseStmt fails on stray }
// Need to ensure RBRACE is consumed after loop or handle it in parseProgram
```

**Files Affected:** `parser.c`

---

### ✅ Issue 2: Import loop passes curr (should be startIdx)
**Location:** `parser.c` line ~310-318  
**Problem:** The import loop passes `curr` but should pass `startIdx`. With `curr`, the progress guard never works.

**Fix Required:** Change `synchronize(curr, topDepth)` to `synchronize(startIdx, topDepth)`.

```c
// Current (line 310-318):
while(match(IMPORT)){
    int startIdx = curr;
    parseImport();
    if(errorInProgress) synchronize(curr, topDepth);  // BUG: should be startIdx
}

// Fix:
if(errorInProgress) synchronize(startIdx, topDepth);
```

**Files Affected:** `parser.c`

---

### ✅ Issue 3: Early return after expect(LBRACE) leaves PROGRAM open
**Location:** `parser.c` line ~342-345  
**Problem:** After `expect(LBRACE)` in parseProgram, if errorInProgress is set, it returns early leaving PROGRAM marked but not cleaned up.

**Fix Required:** Remove the early return or ensure parse() cleans up properly.

```c
// Current (line 342-345):
expect(PROG_MAIN);
expect(LBRACE);
int stmtDepth = getDepth();
if(errorInProgress) return;  // BUG: leaves PROGRAM open

// Fix Option A - Remove early return:
expect(PROG_MAIN);
expect(LBRACE);
int stmtDepth = getDepth();
// No early return, let parse() handle cleanup

// Fix Option B - Make sure parse() cleans up (preferred)
```

**Files Affected:** `parser.c`

---

### ✅ Issue 4: parse() doesn't close the log
**Location:** `parser.c` line ~378-381  
**Problem:** If expect(PROG_END), expect(LINE_END) or expect(RBRACE) fails, the final markRule(END_RULE) is suppressed.

**Fix Required:** Make parse() clear errorInProgress, then call a closeAll() in parserUtils that pushes END_RULE while curDepth > 0.

```c
// Current (line 378-381):
bool parse(){
    parseProgram();
    if(errCount > 0) return false;
    return true;
}

// Fix:
void closeAll() {
    // Push END_RULE while curDepth > 0
    while(curDepth > 0) {
        pushToken(NULL, END_RULE);
    }
}

bool parse(){
    parseProgram();
    if(errCount > 0) return false;
    
    // Close the log
    closeAll();
    errorInProgress = false;  // Clear error flag
    
    return true;
}
```

**Files Affected:** `parser.c`, `parserUtils.h` (add closeAll function)

---

### ✅ Issue 5: initParser doesn't reset errorInProgress
**Location:** `parser.c` line ~26-37  
**Problem:** initParser doesn't reset errorInProgress for repeat parses.

**Fix Required:** Reset errorInProgress in initParser.

```c
// Current (line 26-37):
bool initParser(SymTBL* tbl, char* sf){
    if(!tbl) { ... }
    if(!startTokenStack()) { ... }
    lexxedTable = tbl;
    curr = 0;
    sourcefile = sf;
    errCount = 0;
    return true;
}

// Fix:
bool initParser(SymTBL* tbl, char* sf){
    if(!tbl) { ... }
    if(!startTokenStack()) { ... }
    lexxedTable = tbl;
    curr = 0;
    sourcefile = sf;
    errCount = 0;
    errorInProgress = false;  // ADD THIS LINE
    return true;
}
```

**Files Affected:** `parser.c`

---

### ✅ Issue 6: Recovery gaps - No top-level stops in skip
**Location:** `parser.c` line ~78-95 (consumeToNextSync function)  
**Problem:** After a broken import, the skip doesn't stop at IMPORT, FUNCTION or PROG_MAIN. The variable isTopLevelSync is declared but nothing uses it.

**Fix Required:** Either pass a topLevel flag through synchronize and consumeToNextSync, or drop the unused variable and add proper sync points for top-level constructs.

```c
// Current (line 78-95):
bool consumeToNextSync(int startIdx){
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

// Fix: Add sync points for IMPORT, FUNCTION, PROG_MAIN at top level
bool consumeToNextSync(int startIdx, bool topLevel){
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
        // ADD TOP-LEVEL SYNC POINTS:
        else if(depth == 0 && topLevel){
            if(s->token == IMPORT || s->token == FUNCTION || s->token == PROG_MAIN){
                if(curr > startIdx) return;
            }
        }
        else if(depth == 0 && isSync(s, next)){
            if(s->token == LINE_END){ curr++; return; }
            if(curr > startIdx) return;        
        }
        curr++;
    }
}

// Update callers to pass topLevel flag
```

**Files Affected:** `parser.c` (consumeToNextSync function and callers)

---

## Additional Issues Found During Review

### Issue 7: Block statements lack sync points before parseBlock()
**Location:** Various block statement functions (parseIfStmt, parseWhileStmt, parseForStmt, parseForeachStmt, parseFunctionDecl)  
**Problem:** A header error hides the body's errors. Need syncToBlock() before parseBlock().

**Fix Required:** Add `syncToBlock()` before each call to `parseBlock()`.

```c
// Example for parseIfStmt (line ~280):
static void parseIfStmt(){
    expectRule(IF, IF_STMT);
    expect(LPAREN); 
    parseExpr();
    expect(RPAREN);
    syncToBlock();  // ADD THIS LINE - sync before block
    parseBlock();
    if(match(ELSE)){
        parseElseChain();
    }
    markRule(END_RULE);
}

// Define syncToBlock in parser.c:
void syncToBlock(){
    int depth = getDepth();
    while(curr < lexxedTable->curIdx){
        Symbol *s = &lexxedTable->data[curr];
        TokenType next = lexxedTable->data[curr + 1].token;

        if(s->token == LBRACE) {
            curr++;
            // Consume block until RBRACE at same depth
            while(curr < lexxedTable->curIdx){
                Symbol *innerS = &lexxedTable->data[curr];
                TokenType innerNext = lexxedTable->data[curr + 1].token;
                
                if(innerS->token == RBRACE) break;
                curr++;
            }
        } else if(depth == 0 && isSync(s, next)){
            if(s->token == LINE_END){ curr++; return; }
            if(curr > startIdx) return;        
        }
    }
}
```

**Files Affected:** `parser.c` (all functions calling parseBlock())

---

## Summary of Required Changes

| # | Issue | File(s) | Priority |
|---|-------|---------|----------|
| 1 | parseProgram RBRACE handling | parser.c | High |
| 2 | Import loop curr vs startIdx | parser.c | High |
| 3 | Early return after expect(LBRACE) | parser.c | Medium |
| 4 | parse() doesn't close log | parser.c, parserUtils.h | High |
| 5 | initParser missing errorInProgress reset | parser.c | Medium |
| 6 | consumeToNextSync lacks top-level sync points | parser.c | High |
| 7 | Block statements lack syncToBlock() | parser.c | Medium |

---

## Testing Plan

1. **Run a valid program** - Verify normal parsing works after fixes
2. **Test stray `}` in main body** - Should not cause infinite loop
3. **Test missing PROG_END** - Should report error and recover
4. **Test broken first import** - Should stop at next top-level construct

---

*Generated: 2026-11-25*  
*Status: Ready for implementation*
