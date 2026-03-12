#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "utils.h"

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
		return 1;
	}

	char* src = loadFile(argv[1]);

	Token* tks = tokenize(src);

	FILE* out = fopen("tokens.txt", "w");
	if (!out) err("Cannot open tokens.txt");

	showTokensDetailed(tks, out);

	fclose(out);
	printf("Tokens written to: tokens.txt\n");
	return 0;
}