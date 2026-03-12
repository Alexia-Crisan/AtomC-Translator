#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "utils.h"

int main(int argc, char* argv[])
{
	FILE* src = fopen("../../tests/testlex.c", "r");
	
	Token* tks = tokenize(src);

	FILE* out = fopen("tokens.txt", "w");
	if (!out) err("Cannot open tokens.txt");

	showTokensDetailed(tks, out);

	fclose(out);
	printf("Tokens written to: tokens.txt\n");
	return 0;
}