#pragma once

enum
{
	ID,
	// keywords
	TYPE_CHAR, TYPE_DOUBLE, TYPE_INT,
	IF, ELSE, WHILE, RETURN, VOID, STRUCT,
	// delimiters
	COMMA, SEMICOLON, LPAR, RPAR, LBRACKET, RBRACKET, LACC, RACC, END,
	// operators
	ADD, SUB, MUL, DIV, DOT,
	AND, OR, NOT,
	ASSIGN, EQUAL, NOTEQ,
	LESS, LESSEQ, GREATER, GREATEREQ,
	// constants
	INT, DOUBLE, CHAR, STRING
};

typedef struct Token
{
	int code;		// ID, TYPE_CHAR, ...
	int line;		// the line from the input file
	union
	{
		char* text;		// the chars for ID, STRING
		int i;			// the value for INT
		char c;			// the value for CHAR
		double d;		// the value for DOUBLE
	};
	struct Token* next;
} Token;

Token *tokenize(const char *pch);
void showTokens(const Token *tokens);
void showTokensDetailed(const Token* tokens, FILE* out);
const char* consumeLineComment(const char* pch);
char consumeEscape(const char** pch);
const char* handleString(const char* pch);
const char* handleChar(const char* pch);
