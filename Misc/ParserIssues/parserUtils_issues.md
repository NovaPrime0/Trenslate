# Parser Utils Issues Checklist

## Deferred Items from parserUtils.c and parserUtils.h

### Issue 1: unwindToLastSync - first->depth++ should be first->depth
**Location:** `parserUtils.c` line ~245-260  
**Problem:** In the unwind logic, `first->depth++` increments depth incorrectly. Should use `first->depth`.

```c
// Current (line 245-260):
void unwindToLastSync(int stmtDepth){
    int old = tokenStack -> curSize;
    int cut = lastSyncTokIndex + 1;
    
    while(cut < old 
        && tokenStack->data[cut].rule == END_RULE
        && tokenStack->data[cut].sym == NULL) cut++;
    
    if(cut < old){
        Entry* first = &tokenStack->data[cut];
        curDepth = (first->rule == END_RULE)? first->depth + 1 : first->depth;  // BUG: depth++
        for(int i = cut; i < old; i++) resetEntry(&tokenStack->data[i]);
        tokenStack->curSize = cut;
    }

    while(curDepth > stmtDepth) {
        pushToken(NULL, END_RULE);
    }
}

// Fix:
void unwindToLastSync(int stmtDepth){
    int old = tokenStack -> curSize;
    int cut = lastSyncTokIndex + 1;
    
    while(cut < old 
        && tokenStack->data[cut].rule == END_RULE
        && tokenStack->data[cut].sym == NULL) cut++;
    
    if(cut < old){
        Entry* first = &tokenStack->data[cut];
        curDepth = (first->rule == END_RULE)? first->depth : first->depth;  // FIX: remove ++
        for(int i = cut; i < old; i++) resetEntry(&tokenStack->data[i]);
        tokenStack->curSize = cut;
    }

    while(curDepth > stmtDepth) {
        pushToken(NULL, END_RULE);
    }
}
```

**Files Affected:** `parserUtils.c`

---

### Issue 2: Delete pop function
**Location:** `parserUtils.c` line ~145-178  
**Problem:** The `pop()` function is unused and should be removed.

**Fix Required:** Remove the entire `pop()` function definition.

```c
// REMOVE ENTIRE FUNCTION (lines 145-178)
bool pop(Stack *st, Entry* w){
    if(!st){
        printf("[parserUtils.c | %s: %d]: Stack not initialized \n", __func__, __LINE__);
        return false;
    }
    if(st->curSize <= 0) {
        printf("[parserUtils.c | %s: %d]: No entries in stack \n", __func__, __LINE__);
        return false;
    } else if (!w){
        printf("[parserUtils.c | %s: %d]: Null entry, resetting top node instead \n", __func__, __LINE__);
        resetEntry(&st->data[--st->curSize]);
        return true;
    }
    printf("[parserUtils.c | %s: %d]: Attaching data to passed address \n", __func__, __LINE__);
    
    Entry *poppedNode = &st->data[--st->curSize];
    w->sym = poppedNode->sym;
    w->rule = poppedNode->rule;
    w->depth = poppedNode->depth;
    // ... rest of function
}
```

**Files Affected:** `parserUtils.c` (remove entire function)

---

### Issue 3: Delete tokDistFromLastSync variable
**Location:** `parserUtils.h` line ~40, and potentially used in parserUtils.c  
**Problem:** The `tokDistFromLastSync` variable is declared but not used.

**Fix Required:** Remove the declaration from `parserUtils.h`. Check if it's used anywhere else before removing.

```c
// Current (parserUtils.h line ~40):
typedef struct{
    int curSize;
    int maxSize;
    Entry* data;
} Stack;

// Fix: Remove tokDistFromLastSync from the struct definition
typedef struct{
    int curSize;
    int maxSize;
    Entry* data;
} Stack;
```

**Files Affected:** `parserUtils.h` (remove variable declaration)

---

### Issue 4: Delete SYNC_FILE_PATH macro
**Location:** `parserUtils.c` line ~20  
**Problem:** The `SYNC_FILE_PATH` macro is defined but not used.

**Fix Required:** Remove the macro definition.

```c
// Current (line 20):
#define SYNC_FILE_PATH ".ppl_sync_state.txt"

// Fix: Remove this line entirely
```

**Files Affected:** `parserUtils.c` (remove macro)

---

### Issue 5: Don't set s->data = NULL in expand's failure path
**Location:** `parserUtils.c` line ~80-86  
**Problem:** In the expand() function, on failure it sets `s->data = NULL`. This should not happen.

```c
// Current (line 80-86):
bool expand(Stack *s){
    int newSize = s->maxSize*2;
    Entry *temp = realloc(s->data, sizeof(Entry)*newSize);
    if(!temp){
        printf("[perserUtils.c: %d]: Failed to allocate stack", __LINE__);
        s->data = NULL;  // BUG: Don't set to NULL
        return false;
    }
    s->data = temp;
    s->maxSize = newSize;
    for(int i = s->curSize; i < s->maxSize; i++) resetEntry(&s->data[i]);
    return true;
}

// Fix: Remove the NULL assignment or handle differently
bool expand(Stack *s){
    int newSize = s->maxSize*2;
    Entry *temp = realloc(s->data, sizeof(Entry)*newSize);
    if(!temp){
        printf("[perserUtils.c: %d]: Failed to allocate stack", __LINE__);
        // Don't set s->data = NULL - leave it as is or handle error differently
        return false;
    }
    s->data = temp;
    s->maxSize = newSize;
    for(int i = s->curSize; i < s->maxSize; i++) resetEntry(&s->data[i]);
    return true;
}
```

**Files Affected:** `parserUtils.c` (remove NULL assignment)

---

### Issue 6: Unused ptree variable in parser.c
**Location:** `parser.c` line ~18  
**Problem:** The global `char ptree[1024]` is declared but appears to be unused.

```c
// Current (line 18):
SymTBL* lexxedTable;
int curr;
bool errorInProgress;  // Flag to skip expect/match on error
char ptree[1024];  // BUG: Unused variable
char* sourcefile;
int errCount; 

// Fix: Remove the unused declaration or use it if needed for debugging
```

**Files Affected:** `parser.c` (remove unused variable)

---

### Issue 7: Rename blockDepth to stmtDepth (optional)
**Location:** Various places in parser.c  
**Problem:** The variable is named `blockDepth` but should be `stmtDepth` for consistency.

```c
// Current usage examples:
int blockDepth = getDepth();  // Should be stmtDepth
synchronize(startIdx, blockDepth);  // Should be stmtDepth

// Fix: Rename to stmtDepth throughout parser.c
```

**Files Affected:** `parser.c` (optional rename)

---

### Issue 8: Debug prints in matchOffset and expect* should be behind DEBUG macro
**Location:** Various places in parser.c  
**Problem:** The debug prints fire constantly. Should be behind a DEBUG macro.

```c
// Current (line ~105):
bool matchOffset(TokenType t, int offset){
    if(offset + curr > lexxedTable->curIdx) return false;
    printf("[parser.c | %s: %d]: Lookahead %d Token(s)\n", __func__, __LINE__, offset);  // Always fires
    TokenType tok = peekTok(offset);
    printf("[parser.c | %s: %d]: Found %s \n", __func__, __LINE__, tokenNames[tok]);  // Always fires
    
    return tok == t;
}

// Fix: Add DEBUG macro guard
#ifdef DEBUG
bool matchOffset(TokenType t, int offset){
    if(offset + curr > lexxedTable->curIdx) return false;
    printf("[parser.c | %s: %d]: Lookahead %d Token(s)\n", __func__, __LINE__, offset);
    TokenType tok = peekTok(offset);
    printf("[parser.c | %s: %d]: Found %s \n", __func__, __LINE__, tokenNames[tok]);
    
    return tok == t;
}
#endif
```

**Files Affected:** `parser.c` (add DEBUG macro guards)

---

## Summary of Required Changes in parserUtils

| # | Issue | File(s) | Priority | Action |
|---|-------|---------|----------|--------|
| 1 | unwindToLastSync: first->depth++ → first->depth | parserUtils.c | High | Fix bug |
| 2 | Delete pop function | parserUtils.c | Medium | Remove unused code |
| 3 | Delete tokDistFromLastSync variable | parserUtils.h | Low | Remove unused var |
| 4 | Delete SYNC_FILE_PATH macro | parserUtils.c | Low | Remove unused macro |
| 5 | Don't set s->data = NULL in expand() failure | parserUtils.c | Medium | Fix logic |

---

## Testing Plan for Parser Utils Issues

1. **Valid program parsing** - Verify normal operation after all fixes
2. **Error recovery scenarios** - Test that errors don't cause infinite loops
3. **Stack operations** - Verify push/pop/expand work correctly
4. **Sync point restoration** - Ensure unwindToLastSync works properly

---

*Generated: 2026-11-25*  
*Status: Ready for implementation*
