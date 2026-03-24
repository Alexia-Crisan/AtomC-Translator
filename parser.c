#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token* iTk;			// the iterator in the tokens list
Token* consumedTk;	// the last consumed token

void tkerr(const char* fmt, ...) 
{
	fprintf(stderr, "error in line %d: ", iTk->line);
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

bool consume(int code) 
{
	if (iTk->code == code) 
	{
		consumedTk = iTk;
		iTk = iTk->next;
		return true;
	}
	return false;
}

void parse(Token* tokens)
{
	iTk = tokens;
	if (!unit())
		tkerr("Syntax error");
}


// unit: ( structDef | fnDef | varDef )* END
bool unit() 
{
	for (;;) 
	{
		if (structDef()) {}
		else if (fnDef()) {}
		else if (varDef()) {}
		else break;
	}
	if (consume(END)) 
	{
		return true;
	}
	return false;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef
{
	Token* start = iTk;

	if (consume(STRUCT))
	{
		if (consume(ID))
		{
			if (consume(LACC))
			{
				while (varDef()) {} // optional

				if (consume(RACC))
				{
					if (consume(SEMICOLON))
						return true;

					tkerr("Missing ; after struct definition");

				}

				tkerr("Missing } in struct definition");
			}
		}
	}

	iTk = start;
	return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef
{
	Token* start = iTk;

	if (typeBase())
	{
		if (consume(ID))
		{
			bool arrayDeclOptional = arrayDecl();

			if (consume(SEMICOLON))
				return true;

			tkerr("Missing ; after variable definition");
		}
	}

	iTk = start;
	return false;
}


// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase()
{
	if (consume(TYPE_INT))
	{
		return true;
	}
	if (consume(TYPE_DOUBLE))
	{
		return true;
	}
	if (consume(TYPE_CHAR))
	{
		return true;
	}
	if (consume(STRUCT))
	{
		if (consume(ID))
		{
			return true;
		}
	}
	return false;
}

// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl()
{
	Token* start = iTk;

	if (consume(LBRACKET))
	{
		consume(INT);

		if (consume(RBRACKET))
			return true;

		tkerr("Missing ] in array declaration");
	}

	iTk = start;
	return false;
}

// fnDef: ( typeBase | VOID ) ID LPAR(fnParam(COMMA fnParam)*) ? RPAR stmCompound
bool fnDef()
{
	Token* start = iTk;
	bool hasType = false;

	if (typeBase() || consume(VOID))
	{
		if (consume(ID))
		{
			if (consume(LPAR))
			{
				if (fnParam())
				{
					while (consume(COMMA))
					{
						if (!fnParam())
							tkerr("Expected parameter after ,");
					}
				}

				if (consume(RPAR)
				{
					if (stmCompound())
						return true;

						tkerr("Missing function body");
				}

				tkerr("Missing ) in function definition");
			}
		}
	}

	iTk = start;
	return false;
}

// fnParam: typeBase ID arrayDecl?
bool fnParam()
{
	Token* start = iTk;

	if (typeBase())
	{
		if (consume(ID))
		{
			bool arrayDeclOptional = arrayDecl();

			return true;
		}
	}

	iTk = start;
	return false;
}

// stm: stmCompound | IF LPAR expr RPAR stm(ELSE stm) ? | WHILE LPAR expr RPAR stm | RETURN expr ? SEMICOLON | expr ? SEMICOLON
bool stm()
{

}

// stmCompound: LACC(varDef | stm)* RACC
bool stmCompound()
{
	Token* start = iTk;

	if (consume(LACC))
	{
		for (;;)
		{
			if (varDef() || stm()) {}
			else break;
		}

		if (consume(RACC))
			return true;

		tkerr("Missing } at end of block");
	}

	iTk = start;
	return false;
}

// expr: exprAssign
bool expr()
{
	Token* start = iTk;

	if (exprAssign()) 
		return true;

	iTk = start;
	return false;
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign()
{

}

// exprOr: exprOr OR exprAnd | exprAnd
bool exprOr()
{
	Token* start = iTk;

	if (exprAnd())
	{
		exprOrPrim();
		return true;
	}

	iTk = start;
	return false;
}

bool exprOrPrim()
{
	if (consume(OR))
	{
		if (exprAnd())
		{
			exprOrPrim();
			return true;
		}

		tkerr("Expected expression after ||");
	}

	return true;
}

bool exprAnd()
{

}

bool exprAndPrim()
{

}

bool exprEq()
{

}

bool exprEqPrim()
{

}

bool exprRel()
{

}

bool exprRelPrim()
{

}

bool exprAdd()
{

}

bool exprAddPrim()
{

}

bool exprMul();
{

}

bool exprMulPrim()
{

}

bool exprCast();
{

}

bool exprUnary()
{

}

bool exprPostfix()
{

}

bool exprPostfixPrim()
{

}

bool exprPrimary()
{

}