#include <stdio.h>
#include "lexer.h"
#include "parser.h"
#include "utils.h"
#include "vm.h"

#define SOURCE_FILE "C:\\Users\\alcrisan\\OneDrive - Nokia\\Desktop\\Folders\\Uni\\LFTC\\LFTC_Translator\\tests\\testgc.c"
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

    pushDomain();   // 1. create global domain
    vmInit();       // 2. register put_i, put_d before parsing
    parse(tks);     // 3. parse + AD + AT + code generation
    printf("Syntax OK\n");

    // 4. find main and run the compiled code
    Symbol* symMain = findSymbolInDomain(symTable, "main");
    if (!symMain) err("missing main function");

    Instr* entryCode = NULL;
    addInstr(&entryCode, OP_CALL)->arg.instr = symMain->fn.instr;
    addInstr(&entryCode, OP_HALT);
    run(entryCode);

    dropDomain();   // 5. cleanup

    return 0;
}