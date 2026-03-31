#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "utils.h"

Token *tokens;	// single linked list of tokens
Token *lastTk;  // the last token in list
int line = 1;   // the current line in the input file

// adds a token to the end of the tokens list and returns it
// sets its code and line
Token *addTk(int code)
{
	Token *tk = safeAlloc(sizeof(Token));
	tk->code = code;
	tk->line = line;
	tk->next = NULL;

	if(lastTk) lastTk->next = tk;
	else tokens = tk;

	lastTk = tk;
	return tk;
}

char *extract(const char *begin, const char *end)
{
	int len = (size_t)(end - begin);
	char* buffer = (char*)safeAlloc(len + 1);
	memcpy(buffer, begin, len);
	buffer[len] = '\0';
	return buffer;
}

const char* consumeLineComment(const char* pch)
{
	while (*pch && *pch != '\n' && *pch != '\r') pch++;
	return pch;
}

char consumeEscape(const char** pch)
{
	char c = **pch;
	(*pch)++;

	switch (c)
	{
		case 'a':  return '\a';
		case 'b':  return '\b';
		case 'f':  return '\f';
		case 'n':  return '\n';
		case 'r':  return '\r';
		case 't':  return '\t';
		case 'v':  return '\v';
		case '\\': return '\\';
		case '\'': return '\'';
		case '"':  return '"';
		case '0':  return '\0';
		default: err("Unknown escape sequence: \\%c", c);
	}
	return 0;
}

const char* handleString(const char* pch)
{
	int capacity = 64, length = 0;
	char* buffer = (char*)safeAlloc(capacity);

	while (*pch != '"')
	{
		// reached the end of the entire file and never found a closing quote "
		if (*pch == '\0') err("Unterminated string constant");

		char charVal;
		if (*pch == '\\')
		{
			pch++;
			charVal = consumeEscape(&pch);
		}
		else
		{
			charVal = *pch++;
		}

		if (length + 1 >= capacity)
		{
			capacity *= 2;
			buffer = realloc(buffer, capacity);

			if (!buffer) err("Out of memory");
		}

		buffer[length ++] = charVal;
	}

	buffer[length] = '\0';                    
	pch++; // skip closing "

	Token* tk = addTk(STRING);
	tk->text = buffer;
	return pch;
}

const char* handleChar(const char* pch)
{
	char charVal;

	if (*pch == '\\')
	{
		pch++;
		charVal = consumeEscape(&pch);
	}
	else if (*pch != '\'' && *pch != '\0')
	{
		charVal = *pch++;
	}
	else 
	{
		err("Empty char constant");
		charVal = 0;
	}

	if (*pch != '\'') err("Missing closing '");

	pch++;                    
	Token* tk = addTk(CHAR);
	tk->c = charVal;
	return pch;
}

// DOUBLE: [0-9]+ ( '.' [0-9]+ ( [eE] [+-]? [0-9]+ )? | ( '.' [0-9]+ )? [eE] [+-]? [0-9]+ )
// INT:    [0-9]+
const char* handleNumber(const char* pch)
{
	const char* start = pch;

	while (isdigit(*pch)) pch++; // [0-9]+

	int isReal = 0;

	// [0-9]+ '.' [0-9]+ ( [eE] [+-]? [0-9]+ )?
	if (*pch == '.')
	{
		isReal = 1;

		pch++;
		if (!isdigit(*pch)) err("Expected digit after decimal point");
		while (isdigit(*pch)) pch++;

		if (*pch == 'e' || *pch == 'E') // optional exponent
		{
			pch++;
			if (*pch == '+' || *pch == '-') pch++;
			if (!isdigit(*pch)) err("Expected digit in exponent");
			while (isdigit(*pch)) pch++;
		}
	}
	// [0-9]+ [eE] [+-]? [0-9]+
	else if (*pch == 'e' || *pch == 'E')
	{
		isReal = 1;
		pch++;
		if (*pch == '+' || *pch == '-') pch++;
		if (!isdigit(*pch)) err("Expected digit in exponent");
		while (isdigit(*pch)) pch++;
	}

	char* numString = extract(start, pch);

	Token* tk;
	if (isReal) 
	{ 
		tk = addTk(DOUBLE); 
		tk->d = atof(numString);
	}
	else 
	{ 
		tk = addTk(INT);    
		tk->i = atoi(numString);
	}

	free(numString);
	return pch;
}

const char* handleKeyword(const char* pch)
{ 
	const char* start = pch;
	while (isalnum(*pch) || *pch == '_') pch++; // ID: [a-zA-Z_] [a-zA-Z0-9_]*

	char* text = extract(start, pch);
	Token* tk;

	if (!strcmp(text, "char")) { free(text); addTk(TYPE_CHAR); }
	else if (!strcmp(text, "double")) { free(text); addTk(TYPE_DOUBLE); }
	else if (!strcmp(text, "else")) { free(text); addTk(ELSE); }
	else if (!strcmp(text, "if")) { free(text); addTk(IF); }
	else if (!strcmp(text, "int")) { free(text); addTk(TYPE_INT); }
	else if (!strcmp(text, "return")) { free(text); addTk(RETURN); }
	else if (!strcmp(text, "struct")) { free(text); addTk(STRUCT); }
	else if (!strcmp(text, "void")) { free(text); addTk(VOID); }
	else if (!strcmp(text, "while")) { free(text); addTk(WHILE); }
	else { tk = addTk(ID); tk->text = text; }

	return pch;
}

Token *tokenize(const char *pch)
{
	const char *start;
	Token *tk;

	for(;;)
	{
		switch(*pch)
		{
			// SPACE: [ \n\r\t]
			case ' ': case '\t': pch++; break;
			case '\r':		// handles different kinds of newlines (Windows: \r\n, Linux: \n, MacOS, OS X: \r or \n)
				if(pch[1] == '\n') pch++;
				// fallthrough to \n
			case '\n': { line++; pch++; break; }	
			case '\0': addTk(END); return tokens;
			
			// delimiters
			case ',': addTk(COMMA);     pch++; break;
			case ';': addTk(SEMICOLON); pch++; break;
			case '(': addTk(LPAR);      pch++; break;
			case ')': addTk(RPAR);      pch++; break;
			case '[': addTk(LBRACKET);  pch++; break;
			case ']': addTk(RBRACKET);  pch++; break;
			case '{': addTk(LACC);      pch++; break;
			case '}': addTk(RACC);      pch++; break;

			// operators
			case '+': addTk(ADD); pch++; break;
			case '-': addTk(SUB); pch++; break;
			case '*': addTk(MUL); pch++; break;
			case '.': addTk(DOT); pch++; break;

			case '/':
				if (pch[1] == '/') pch = consumeLineComment(pch);
				else { addTk(DIV); pch++; }
				break;

			case '=':
				if (pch[1] == '=') { addTk(EQUAL); pch += 2; }
				else { addTk(ASSIGN); pch++; }
				break;

			case '<':
				if (pch[1] == '=') { addTk(LESSEQ); pch += 2; }
				else { addTk(LESS); pch++; }
				break;


			case '>':
				if (pch[1] == '=') { addTk(GREATEREQ); pch += 2; }
				else { addTk(GREATER); pch++; }
				break;

			case '!':
				if (pch[1] == '=') { addTk(NOTEQ); pch += 2; }
				else { addTk(NOT); pch++; }
				break;

			case '&':
				if (pch[1] == '&') { addTk(AND); pch += 2; }
				else err("Invalid char: %c (%d)", *pch, *pch);
				break;

			case '|':
				if (pch[1] == '|') { addTk(OR); pch += 2; }
				else err("Invalid char: %c (%d)", *pch, *pch);
				break;

			// handle char or string
			case '\'': pch = handleChar(pch + 1);   break;
			case '"':  pch = handleString(pch + 1); break;
				
			// handle keywords, numbers
			default:
				if(isalpha(*pch) || *pch == '_')
					pch = handleKeyword(pch);
				else if (isdigit(*pch))
					pch = handleNumber(pch);
				else
					err("Invalid char: %c (%d)", *pch, *pch);
				break;
			}
		}
	}

void showTokens(const Token *tokens)
{
	for(const Token *tk = tokens; tk; tk = tk->next)
	{
		printf("%d\n",tk->code);
	}
}

void showTokensDetailed(const Token* tokens, FILE* out)
{
	for (const Token* tk = tokens; tk; tk = tk->next)
	{
		switch (tk->code)
		{
			case ID:        fprintf(out, "%d\tID:%s\n", tk->line, tk->text); break;
			case INT:       fprintf(out, "%d\tINT:%d\n", tk->line, tk->i);    break;
			case DOUBLE:    fprintf(out, "%d\tDOUBLE:%g\n", tk->line, tk->d);    break;
			case CHAR:
				if (tk->c == '\'') fprintf(out, "%d\tCHAR:'\n", tk->line);
				else              fprintf(out, "%d\tCHAR:%c\n", tk->line, tk->c);
				break;
			case STRING:      fprintf(out, "%d\tSTRING:%s\n", tk->line, tk->text); break;
			case TYPE_INT:    fprintf(out, "%d\tTYPE_INT\n", tk->line); break;
			case TYPE_CHAR:   fprintf(out, "%d\tTYPE_CHAR\n", tk->line); break;
			case TYPE_DOUBLE: fprintf(out, "%d\tTYPE_DOUBLE\n", tk->line); break;
			case IF:          fprintf(out, "%d\tIF\n", tk->line); break;
			case ELSE:        fprintf(out, "%d\tELSE\n", tk->line); break;
			case WHILE:       fprintf(out, "%d\tWHILE\n", tk->line); break;
			case RETURN:      fprintf(out, "%d\tRETURN\n", tk->line); break;
			case VOID:        fprintf(out, "%d\tVOID\n", tk->line); break;
			case STRUCT:      fprintf(out, "%d\tSTRUCT\n", tk->line); break;
			case COMMA:       fprintf(out, "%d\tCOMMA\n", tk->line); break;
			case SEMICOLON:   fprintf(out, "%d\tSEMICOLON\n", tk->line); break;
			case LPAR:        fprintf(out, "%d\tLPAR\n", tk->line); break;
			case RPAR:        fprintf(out, "%d\tRPAR\n", tk->line); break;
			case LBRACKET:    fprintf(out, "%d\tLBRACKET\n", tk->line); break;
			case RBRACKET:    fprintf(out, "%d\tRBRACKET\n", tk->line); break;
			case LACC:        fprintf(out, "%d\tLACC\n", tk->line); break;
			case RACC:        fprintf(out, "%d\tRACC\n", tk->line); break;
			case ASSIGN:      fprintf(out, "%d\tASSIGN\n", tk->line); break;
			case EQUAL:       fprintf(out, "%d\tEQUAL\n", tk->line); break;
			case ADD:         fprintf(out, "%d\tADD\n", tk->line); break;
			case SUB:         fprintf(out, "%d\tSUB\n", tk->line); break;
			case MUL:         fprintf(out, "%d\tMUL\n", tk->line); break;
			case DIV:         fprintf(out, "%d\tDIV\n", tk->line); break;
			case DOT:         fprintf(out, "%d\tDOT\n", tk->line); break;
			case LESS:        fprintf(out, "%d\tLESS\n", tk->line); break;
			case LESSEQ:      fprintf(out, "%d\tLESSEQ\n", tk->line); break;
			case GREATER:     fprintf(out, "%d\tGREATER\n", tk->line); break;
			case GREATEREQ:   fprintf(out, "%d\tGREATEREQ\n", tk->line); break;
			case NOTEQ:       fprintf(out, "%d\tNOTEQ\n", tk->line); break;
			case NOT:         fprintf(out, "%d\tNOT\n", tk->line); break;
			case AND:         fprintf(out, "%d\tAND\n", tk->line); break;
			case OR:          fprintf(out, "%d\tOR\n", tk->line); break;
			case END:         fprintf(out, "%d\tEND\n", tk->line); break;
			default:          fprintf(out, "%d\t??(%d)\n", tk->line, tk->code); break;
		}
	}
}