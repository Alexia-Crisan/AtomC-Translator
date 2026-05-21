#include <stdio.h>
#include "lexer.h"
#include "parser.h"
#include "utils.h"
#include "vm.h"

#define SOURCE_FILE "C:\\Users\\alcrisan\\OneDrive - Nokia\\Desktop\\Folders\\Uni\\LFTC\\LFTC_Translator\\tests\\testvm.c"
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

    pushDomain();    // 1. create the global domain in the symbol table
    vmInit();        // 2. register built-in functions (put_i, put_d)
    parse(tks);      // 3. parse + domain analysis + type analysis
    printf("Syntax OK\n");

    printf("\n=== genTestProgram (int) ===\n");
    Instr* testCode = genTestProgram();
    run(testCode);

    printf("\n\n=== genTestProgramDouble (double, homework) ===\n");
    Instr* testCodeDouble = genTestProgramDouble();
    run(testCodeDouble);

    dropDomain();     // 4. release global domain

    return 0;
}