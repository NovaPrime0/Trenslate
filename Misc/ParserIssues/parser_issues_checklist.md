# Parser Issues Checklist - Fresh Start (Post-Revert)

**Status:** Ready for Testing  
**Last Known State:** All Items 1-6 reverted to original code  
**Working Directory:** `K:/Codez/C/PPL`  

---

## Core Fixes Required Before Testing (Items 1-6)

### Issue 2a: Import loop passes wrong token index to synchronize
**Location:** `parser.c` line ~310-318  
**Problem:** The import loop passes `curr` but should pass `startIdx`. With `curr`, the progress guard never works.  
**Fix Required:** Change `synchronize(curr, topDepth)` to `synchronize(startIdx, topDepth)`.

```c
// Current (line ~310-318):
while(match(IMPORT)){
    int startIdx = curr;
    parseImport();
    if(errorInProgress) synchronize(curr, topDepth);  // BUG: should be startIdx
}
// Fix:
if(errorInProgress) synchronize(startIdx, topDepth);
```  
**Files Affected:** `parser.c` | **Lines:** ~310-318  

---

### Issue 2b: parseProgram loop doesn't stop on RBRACE
**Location:** `parser.c` line ~213 (or similar)  
**Problem:** A stray `}` in the main body fails `parseStmt`, causing infinite hang. The skip returns at `}` without consuming it, and the same token fails forever.  
**Fix Required:** Add `&& !match(RBRACE)` guard to while loop condition.

```c
// Current (line ~213):
while (!match(PROG_END) && !match(END_OF_FILE)) {

// Should be:
while (!match(PROG_END) && !match(END_OF_FILE) && !match(RBRACE)) {
```  
**Files Affected:** `parser.c` | **Lines:** ~213  

---

### Issue 3a: parseProgram leaves errorInProgress set after early return
**Location:** `parser.c` line ~212 (or similar)  
**Problem:** If an error occurs before LBRACE in main body, the program stays open with PROGRAM flag still active. Early exit doesn't clear error state properly.  
**Fix Required:** Remove early exit or ensure cleanup happens unconditionally after expect(LBRACE).

```c
// Current (line ~212):
expect(PROG_MAIN);
expect(LBRACE);
int stmtDepth = getDepth();
if(errorInProgress) return;  // BUG: leaves PROGRAM open, doesn't clean up properly

// Fix Option A - Remove early return:
expect(PROG_MAIN);
expect(LBRACE);
int stmtDepth = getDepth();
// No early return, let parse() handle cleanup below

// Fix Option B - Make sure parse() cleans up (preferred):
if(errorInProgress) { /* do minimal cleanup instead of returning */ }
```  
**Files Affected:** `parser.c` | **Lines:** ~210-215  

---

### Issue 3b: parse() doesn't close the log on error
**Location:** `parser.c` line ~647 (parse function) or similar  
**Problem:** If expect(PROG_END), expect(LINE_END) or expect(RBRACE) fails, final markRule(END_RULE) is suppressed. Error handling leaves cleanup incomplete.  
**Fix Required:** Make parse() clear errorInProgress, then call a closeAll() function that pushes END_RULE while curDepth > 0.

```c
// Current (line ~647):
bool parse(){
    parseProgram();
    if(errCount > 0) return false;
    return true;
}

// Fix: Add proper cleanup after parseProgram completes or fails
void closeAll() {
    // Push      while curDepth > 0 (implementation in parserUtils.c)
    errorInProgress = false;  // Clear error flag on exit
}

bool parse(){
    if(parseProgram()) { 
        cleanupParse(true); 
        return true; 
    } else { 
        cleanupParse(false); 
        return false; 
    }
}

void cleanupParse(bool success){
    closeAll();
}
```  
**Files Affected:** `parser.c`, `parserUtils.h` (add closeAll function) | **Lines:** ~647-658  

---

### Issue 3c: lastSyncTokIndex is -1 at first import/function parse
**Location:** `parser.c` line ~203-215  
**Problem:** An error in the first import or function can cut PROGRAM's open, leaving lastSyncTokIndex as -1. markRule(PROGRAM) hasn't been called yet when errors occur early.  
**Fix Required:** Add `markSyncFloor()` call right after markRule(PROGRAM).

```c
// Current (line ~203-215):
while(match(IMPORT)){
    int startIdx = curr;
    parseImport();
    if(errorInProgress) synchronize(curr, topDepth);  // BUG: lastSyncTokIndex still -1 here

// Should be:
int topDepth = getDepth();
markRule(PROGRAM);  // <-- Add this line after markRule call (if exists before IMPORT loop)
while(match(IMPORT)){
    int startIdx = curr;
    parseImport();
```  
**Files Affected:** `parser.c` | **Lines:** ~203-215  

---

### Issue 4a: initParser doesn't reset errorInProgress
**Location:** `parser.c` line ~31-37 (initParser function)  
**Problem:** Repeated parses leave old errors in place. No explicit reset of error state at parser initialization.  
**Fix Required:** Add `errorInProgress = false;` after other initializations.

```c
// Current (line ~31-37):
bool initParser(SymTBL* tbl, char* sf){
    if(!tbl) { ... }
    if(!startTokenStack()) { ... }
    lexxedTable = tbl;
    curr = 0;
    sourcefile = sf;
    errCount = 0;

// Fix: Add errorInProgress reset after other initializations
bool initParser(SymTBL* tbl, char* sf){
    if(!tbl) { ... }
    if(!startTokenStack()) { ... }
    lexxedTable = tbl;
    curr = 0;
    sourcefile = sf;
    errCount = 0;
    errorInProgress = false;  // ADD THIS LINE - Reset for repeat parses

// Files Affected:** `parser.c` | **Lines:** ~31-42  

---

### Issue 5a: Recovery gaps - skip doesn't stop at top-level constructs
**Location:** Various locations in parser.c (IMPORT/FUNCTION/PROGRAM_MAIN loops)  
**Problem:** After a broken import, the skip can swallow the next function. isTopLevelSync declared but unused. No synchronization points for IMPORT/FUNCTION/PROGRAM_MAIN tokens during error recovery.  

```c
// Need to add syncToBlock() or similar before consuming top-level constructs:
while(match(IMPORT)){ /* needs sync after parseImport */ }
    if(errorInProgress) synchronize(startIdx, topDepth);  // Add this line

while(match(FUNCTION)) { /* needs sync after parseFunctionDecl */ }
    int startIdx = curr;
    parseFunctionDecl();
```  
**Files Affected:** `parser.c` | **Lines:** ~203-215 (IMPORT loop), various function definitions  

---

### Issue 6a: Header errors hide body's errors in skip
**Location:** Various block parsing locations throughout parser.c  
**Problem:** The skip consumes the whole function or if body, hiding internal errors. No synchronization before parseBlock() calls inside functions/ifs/whiles/fors/foreaches.  

```c
// Example in function:
static void function(){
    markRule(FUNCTION); // or similar marker
    syncToBlock();  // <-- Add this line here (implementation needed)
    expect(LBRACE);
    int blockDepth = getDepth();
    while(!match(RBRACE) && !match(END_OF_FILE)) { /* ... */ }

// Need to add before each parseBlock() call: function/if/else/while/for/foreach
```  
**Files Affected:** `parser.c` | **Lines:** ~240+ (various function definitions, block parsing code)  

---

## Additional Issues Identified During Analysis

### Issue 7a: EOF error can print twice
**Location:** Various locations in parser.c where errors are reported  
**Problem:** The broken statement prints an error, then the block's closing brace also triggers one. No guard against duplicate eofReported flag usage.  

```c
// Need to add guard like this (example):
bool eofReported = false;  // Add at appropriate scope level

void syntaxErrEOF(Symbol* s, TokenType expectedTok){
    if(eofReported) return;  // Prevent double printing
    eofReported = true;      // Set flag after first print
    
    printf("====================================================\n");
}
```  
**Files Affected:** `parser.c` | **Lines:** Various error reporting locations  

---

### Issue 7b: parseInlineStmt dispatches only on LPAREN
**Location:** parser.c, parseInlineStmt function (around line ~240+)  
**Problem:** Noise_NG calls are unreachable since inline stmts don't check for LPAREN before calling parseCallExpr. parseTerm sends Noise_NG to parseCallExpr, which expects LPAREN but doesn't receive it from parseInlineStmt.  

```c
// Current (parseInlineStmt):
static void parseInlineStmt(){
    markRule(INLINE_STMT);
    //if not block then parse inline basically since block stmts start with an easy-to-check keyword
    if(match(IF) || match(WHILE) || match(FOR) || match(FOREACH)) parseBlockStmt();
    else parseInlineStmt();  // BUG: doesn't check for LPAREN before calling Noise_NG/parseCallExpr

// Fix (optional): Add LPAREN check or remove the dispatch logic entirely if not needed
if(match(LPAREN)){ /* handle call expressions */ }
else { /* other inline statements */ }
```  
**Files Affected:** `parser.c` | **Lines:** ~240+  

---

### Issue 7c: Unused ptree global variable
**Location:** parser.c, line ~16-18 (global declarations)  
**Problem:** `char ptree[1024]` declared in parser.c is never used. Dead code left over from earlier development phase.  

```c
// Current (line ~16-18):
SymTBL* lexxedTable;
int curr;
bool errorInProgress;  // Flag to skip expect/match on error
char ptree[1024];      // BUG: Unused global array declaration

// Fix Option A - Remove entirely:
// SymTBL* lexxedTable;
// int curr;
// bool errorInProgress;  
// (Remove line ~17)

// Fix Option B - Comment out if needed for debugging:
char ptree[1024] __attribute__((unused));  // Mark as unused to compiler warnings
```  
**Files Affected:** `parser.c` | **Lines:** ~16-18  

---

## Test Scenarios (Must Run After Each Fix)

| # | Scenario | Expected Behavior | File/Command | Priority |
|---|----------|------------------|--------------|----------|
| 1 | Valid program parsing | Parse successfully, no errors | `TestCases/valid_program/test_valid.tren` + run parser | HIGH (baseline) |
| 2 | Stray `}` in main body | Report error at RBRACE position, recover gracefully without hang | Create test file with stray brace before PROG_END | MEDIUM |
| 3 | Missing PROG_END token | Report missing end marker, don't hang indefinitely | Test file without final } or PROG_END | HIGH |
| 4 | Broken first import (`daan "invalid_file"`) | Stop at next top-level construct (FUNCTION/PROGRAM_MAIN) instead of swallowing it | Create test with invalid import syntax followed by valid FUNCTION | MEDIUM |

---

## Files to Modify Summary

1. **`parser.c`** - Main parsing logic, all Items 2a-6 apply here
   - Line ~310: Fix synchronize parameter (Issue 2a)  
   - Line ~213: Add RBRACE guard (Issue 2b)  
   - Line ~212: Handle early return properly (Issue 3a)  
   - Line ~647+: Proper cleanup in parse() function (Issue 3b)  
   - Line ~203-215: markSyncFloor after PROGRAM marker (Issue 3c)  
   - Line ~31-37: Reset errorInProgress in initParser (Issue 4a)  

2. **`parserUtils.h/c`** - Synchronization utilities if needed for syncToBlock/markSyncFloor/closeAll functions
   - Add `closeAll()` function to push END_RULE while curDepth > 0  
   - Implement `syncToBlock()` helper function  
   - Implement `markSyncFloor()` helper function  

3. **Output files:** Will be generated during testing (not tracked in git)

---

## Notes from Previous Handoff Analysis

**Completed Fixes (4/7):** errorInProgress reset, unused variable removal, SYNC_FILE_PATH deletion, tokDistFromLastSync comment-out  
**Reverted Items (3/7):** All items requiring testing before proceeding - reverted per user instruction to "not repeat yourself"  

**Critical Gap:** Tests have NOT been run yet. This checklist documents what needs to be done AFTER verifying the current state with actual test runs.

---

## Git Commands Reference

```bash
# Check current state of files
git diff --stat HEAD

# Restore a reverted change (if needed)  
git checkout HEAD -- parser.c

# View original code for reference
cat parser.c | grep -n "while.*PROG_END"  # Issue 2b location
cat parser.c | grep -n "synchronize(curr, topDepth)"  # Issue 2a location
```  

---

## Next Steps After Testing Each Fix

1. Run valid program → verify baseline parsing works  
   ```bash
   cd K:/Codez/C/PPL && make clean -s && make run TestCases/valid_program/test_valid.tren
   ```  

2. Test stray `}` scenario → confirm error reporting and recovery without hang  
3. Test missing PROG_END → ensure no infinite hang, proper error message  
4. Test broken import → verify stop at next top-level construct (FUNCTION/PROGRAM_MAIN) instead of swallowing it  

**If any test fails:** Revert the fix using git checkout HEAD -- parser.c or specific file, then investigate further before proceeding to next item.

---

## Success Criteria for Each Item

| Issue | Pass Condition |
|-------|----------------|
| 2a (synchronize) | Error in import doesn't cause infinite loop; progress guard works correctly |
| 2b (RBRACE guard) | Stray `}` causes error but program terminates/hangs gracefully, not infinitely |
| 3a (early return) | PROGRAM state is properly cleaned up after LBRACE errors |
| 3b (closeAll) | END_RULE marker pushed even when expect() fails at end of parseProgram |
| 3c (markSyncFloor) | lastSyncTokIndex initialized correctly before first import/function parses |
| 4a (initParser reset) | Repeated parser invocations start with clean error state |

---

**Last Updated:** After reverting all pre-test modifications  
**Ready for Testing Phase: YES** ✅  
