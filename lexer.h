#pragma once

enum
{
	ID
	// keywords
	, TYPE_CHAR, TYPE_DOUBLE, TYPE_INT
	, IF, ELSE, WHILE, RETURN, VOID, STRUCT
	// delimiters
	, COMMA, SEMICOLON, LPAR, RPAR, LACC, RACC, END
	// operators
	, ASSIGN, EQUAL, ADD, SUB, MUL, DIV, LESS, LESSEQ
	, GREATER, GREATEREQ, NOTEQ, NOT, AND, OR
	// constants
	, INT, DOUBLE, CHAR, STRING
};

typedef struct Token
{
	int code;		// ID, TYPE_CHAR, ...
	int line;		// the line from the input file
	union
	{
		char *text;		// the chars for ID, STRING (dynamically allocated)
		int i;		// the value for INT
		char c;		// the value for CHAR
		double d;		// the value for DOUBLE
	};
	struct Token *next;		// next token in a simple linked list
}Token;

Token *tokenize(const char *pch);
void showTokens(const Token *tokens);
void showTokensDetailed(const Token* tokens);
const char* consumeLineComment(const char* pch);
const char* handleString(const char* pch);
const char* handleChar(const char* pch);
