#include <stdio.h>
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
	char* buf = (char*)safeAlloc(len + 1);
	memcpy(buf, begin, len);
	buf[len] = '\0';
	return buf;
}

const char* consumeLineComment(const char* pch)
{
	while (*pch && *pch != '\n') pch++;
	return pch;
}


const char* handleString(const char* pch)
{

}

const char* handleChar(const char* pch)
{
	char c;

}

Token *tokenize(const char *pch)
{
	const char *start;
	Token *tk;

	for(;;)
	{
		switch(*pch)
		{
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
			case '{': addTk(LACC);      pch++; break;
			case '}': addTk(RACC);      pch++; break;

			// operators
			case '+': addTk(ADD); pch++; break;
			case '-': addTk(SUB); pch++; break;
			case '*': addTk(MUL); pch++; break;

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
			case '\'': pch = handleChar(pch + 1); break;
			case '"':  pch = handleString(pch + 1);    break;
				
			// handle identifiers, keywords, numbers
			default:
				if(isalpha(*pch) || *pch == '_')
				{
					for(start = pch++; isalnum(*pch) || *pch == '_'; pch++){}
					char *text=extract(start,pch);
					if(strcmp(text,"char") == 0) addTk(TYPE_CHAR);
					else
					{
						tk = addTk(ID);
						tk->text=text;
					}
				}
				else err("invalid char: %c (%d)",*pch,*pch);
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

void showTokensDetailed(const Token* tokens)
{
	for (const Token* tk = tokens; tk; tk = tk->next) {
		switch (tk->code) {
		case ID:        printf("%d\tID:%s\n", tk->line, tk->text); break;
		case INT:       printf("%d\tINT:%d\n", tk->line, tk->i); break;
		case DOUBLE:    printf("%d\tDOUBLE:%g\n", tk->line, tk->d); break;
		case CHAR:
			if (tk->c == '\'') printf("%d\tCHAR:'\n", tk->line);
			else               printf("%d\tCHAR:%c\n", tk->line, tk->c);
			break;
		case STRING:      printf("%d\tSTRING:%s\n", tk->line, tk->text); break;
		case TYPE_INT:    printf("%d\tTYPE_INT\n", tk->line); break;
		case TYPE_CHAR:   printf("%d\tTYPE_CHAR\n", tk->line); break;
		case TYPE_DOUBLE: printf("%d\tTYPE_DOUBLE\n", tk->line); break;
		case IF:          printf("%d\tIF\n", tk->line); break;
		case ELSE:        printf("%d\tELSE\n", tk->line); break;
		case WHILE:       printf("%d\tWHILE\n", tk->line); break;
		case RETURN:      printf("%d\tRETURN\n", tk->line); break;
		case VOID:        printf("%d\tVOID\n", tk->line); break;
		case STRUCT:      printf("%d\tSTRUCT\n", tk->line); break;
		case COMMA:       printf("%d\tCOMMA\n", tk->line); break;
		case SEMICOLON:   printf("%d\tSEMICOLON\n", tk->line); break;
		case LPAR:        printf("%d\tLPAR\n", tk->line); break;
		case RPAR:        printf("%d\tRPAR\n", tk->line); break;
		case LACC:        printf("%d\tLACC\n", tk->line); break;
		case RACC:        printf("%d\tRACC\n", tk->line); break;
		case ASSIGN:      printf("%d\tASSIGN\n", tk->line); break;
		case EQUAL:       printf("%d\tEQUAL\n", tk->line); break;
		case ADD:         printf("%d\tADD\n", tk->line); break;
		case SUB:         printf("%d\tSUB\n", tk->line); break;
		case MUL:         printf("%d\tMUL\n", tk->line); break;
		case DIV:         printf("%d\tDIV\n", tk->line); break;
		case LESS:        printf("%d\tLESS\n", tk->line); break;
		case LESSEQ:      printf("%d\tLESSEQ\n", tk->line); break;
		case GREATER:     printf("%d\tGREATER\n", tk->line); break;
		case GREATEREQ:   printf("%d\tGREATEREQ\n", tk->line); break;
		case NOTEQ:       printf("%d\tNOTEQ\n", tk->line); break;
		case NOT:         printf("%d\tNOT\n", tk->line); break;
		case AND:         printf("%d\tAND\n", tk->line); break;
		case OR:          printf("%d\tOR\n", tk->line); break;
		case END:         printf("%d\tEND\n", tk->line); break;
		default:          printf("%d\t??(%d)\n", tk->line, tk->code); break;
		}
}