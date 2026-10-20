Changelogs on file:
    9/29/26: basic ds for lexical analyzer
    9/30/26: separation to c and h files + makefile, completion of lexStates.txt
    10/13/26: file and states revisions
        - ** = Op_EXP
        - ',', '.' = added as delimiters COMMA, DOT respectively
        - start of analysis phase (readFile helper function):
            - reads entire file and parses into char until EOF:\0*
    10/20/2026: new NFA state tree "newStates.txt"
        - changed because readapting lexStates would be more difficult
        - added State* next[255] to the State structure for traversal
            - Transition[255] stays as a placeholder to reduce complexity in expanding stateList (checking if transition is larger than the list and reallocating accordingly)
        - added missing tokens
            - For_LIM
            - TRUE
            - FALSE
            - all states Cond_ -> Rel_, all states Bool_ -> Cond_
            - CONST
            - IMPORT
            - Op_EXP