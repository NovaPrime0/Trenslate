#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "enums.h"
#include "utils.h"

char* readFile(char* filepath){
    FILE* f = fopen(filepath, "r+");
    if(!f){
        return NULL;
    } 
}