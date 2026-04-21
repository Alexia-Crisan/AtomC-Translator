#include <stdio.h>
#include "lexer.h"
#include "parser.h"
#include "utils.h"

#define SOURCE_FILE "C:\\Users\\alcrisan\\OneDrive - Nokia\\Desktop\\Folders\\Uni\\LFTC\\LFTC_Translator\\tests\\testparser.c"
#define OUTPUT_FILE "C:\\Users\\alcrisan\\OneDrive - Nokia\\Desktop\\Folders\\Uni\\LFTC\\LFTC_Translator\\tokens.txt"

int main()
{
    char* src = loadFile(SOURCE_FILE);

    Token* tks = tokenize(src);

    FILE* out = fopen(OUTPUT_FILE, "w");

    if (!out) 
        err("Cannot open output file");

    showTokensDetailed(tks, out);
    fclose(out);
    printf("Tokens written to tokens.txt\n");

     parse(tks);
     printf("Syntax OK\n");

    return 0;
}