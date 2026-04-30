#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token* iTk;			// the iterator in the tokens list
Token* consumedTk;	// the last consumed token

void tkerr(const char* fmt, ...) 
{
	fprintf(stderr, "Error in line %d: ", iTk->line);
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

	tkerr("Unexpected token at top level; expected a struct definition, function definition, or variable declaration");
	return false;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef()
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
bool varDef()
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
		
		tkerr("Expected struct name after keyword 'struct'");
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
							tkerr("Missing or invalid parameter after , in function parameter list");
					}
				}

				if (consume(RPAR))
				{
					if (stmCompound())
						return true;

					else tkerr("Missing function body");
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
	Token* start = iTk;

	if (stmCompound()) return true;

	if (consume(IF))
	{
		if (consume(LPAR))
		{
			if (expr())
			{
				if (consume(RPAR))
				{
					if (stm())
					{
						if (consume(ELSE))
						{
							if (!stm()) 
								tkerr("Expected statement after else");
						}
						return true;
					}

					tkerr("Expected statement after if(...)");
				}
				
				tkerr("Missing ) after if condition");
			}

			tkerr("Invalid or missing if condition");
		}

		tkerr("Missing ( after if");
	}

	if (consume(WHILE))
	{
		if (consume(LPAR))
		{
			if (expr())
			{
				if (consume(RPAR))
				{
					if (stm()) 
						return true;

					tkerr("Expected statement after while(...)");
				}

				tkerr("Missing ) after while condition");
			}

			tkerr("Invalid or missing while condition");
		}

		tkerr("Missing ( after while");
	}

	if (consume(RETURN))
	{
		if (expr()) {}	// optional

		if (consume(SEMICOLON))
			return true;

		tkerr("Missing ; after return");
	}

	if (expr())
	{
		if (!consume(SEMICOLON))
			tkerr("Missing ; after expression statement");

		return true;
	}

	if (consume(SEMICOLON)) return true;  // empty statement: just ;

	iTk = start;
	return false;
}

// stmCompound: LACC(varDef | stm)* RACC
bool stmCompound()
{
	Token* start = iTk;

	if (consume(LACC))
	{
		for (;;)
		{
			if (varDef()) {}
			else if (stm()) {}
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
	Token* start = iTk;

	if (exprUnary())
	{
		if (consume(ASSIGN))
		{
			if (exprAssign())
				return true;

			tkerr("Expected expression after =");
		}

		iTk = start; // backtrack
	}

	if (exprOr()) 
		return true;

	iTk = start;
	return false;
}

// exprOr: exprOr OR exprAnd | exprAnd
// exprOr = exprAnd exprOrPrim
// exprOrPrim = OR exprAnd exprOrPrim | ε
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
		if (!exprAnd())
			tkerr("Expected expression after ||");

		exprOrPrim();
		return true;
	}

	return true;
}

// exprAnd: exprAnd AND exprEq | exprEq
// exprAnd = exprEq exprAndPrim
// exprAndPrim = AND exprEq exprAndPrim | ε
bool exprAnd()
{
	Token* start = iTk;

	if (exprEq())
	{
		exprAndPrim();
		return true;
	}

	iTk = start;
	return false;

}

bool exprAndPrim()
{
	if (consume(AND))
	{
		if (!exprEq())
			tkerr("Expected expression after &&");

		exprAndPrim();
		return true;
	}

	return true;
}

// exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel
// exprEq = exprRel exprEqPrim
// exprEqPrim = (EQUAL | NOTEQ) exprRel exprEqPrim | ε
bool exprEq()
{
	Token* start = iTk;

	if (exprRel())
	{
		exprEqPrim();
		return true;
	}

	iTk = start;
	return false;
}

bool exprEqPrim()
{
	if (consume(EQUAL))
	{
		if (!exprRel())
		{
			tkerr("Expected expression after ==");
		}

		exprEqPrim();
		return true;
	}

	if (consume(NOTEQ))
	{
		if (!exprRel())
		{
			tkerr("Expected expression after !=");
		}

		exprEqPrim();
		return true;
	}

	return true;
}

// exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
// exprRel = exprAdd exprRelPrim
// exprRelPrim = (LESS | LESSEQ | GREATER | GREATEREQ) exprAdd exprRelPrim | ε
bool exprRel()
{
	Token* start = iTk;

	if (exprAdd())
	{
		exprRelPrim();
		return true;
	}

	iTk = start;
	return false;
}

bool exprRelPrim()
{
	if (consume(LESS))
	{
		if (!exprAdd())
			tkerr("Missing or invalid operand after <");

		exprRelPrim();
		return true;
	}

	if (consume(LESSEQ))
	{
		if (!exprAdd())
			tkerr("Missing or invalid operand after <=");

		exprRelPrim();
		return true;
	}

	if (consume(GREATER))
	{
		if (!exprAdd())
			tkerr("Missing or invalid operand after >");

		exprRelPrim();
		return true;
	}

	if (consume(GREATEREQ))
	{
		if (!exprAdd())
			tkerr("Missing or invalid operand after >=");

		exprRelPrim();
		return true;
	}

	return true; // epsilon
}

// exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul
// exprAdd = exprMul exprAddPrim
// exprAddPrim = ( ADD | SUB ) exprMul exprAddPrim | ε
bool exprAdd()
{
	Token* start = iTk;

	if (exprMul())
	{
		exprAddPrim();
		return true;
	}

	iTk = start;
	return false;
}

bool exprAddPrim()
{
	if (consume(ADD))
	{
		if (!exprMul())
			tkerr("Missing or invalid operand after +");

		exprAddPrim();
		return true;
	}

	if (consume(SUB))
	{
		if (!exprMul())
			tkerr("Missing or invalid operand after -");

		exprAddPrim();
		return true;
	}

	return true;
}

// exprMul: exprMul ( MUL | DIV ) exprCast | exprCast
// exprMul = exprCast exprMulPrim
// exprMulPrim = ( MUL | DIV ) exprCast exprMulPrim | ε
bool exprMul()
{
	Token* start = iTk;

	if (exprCast())
	{
		exprMulPrim();
		return true;
	}

	iTk = start;
	return false;
}

bool exprMulPrim()
{
	if (consume(MUL))
	{
		if (!exprCast())
			tkerr("Missing or invalid operand after *");

		exprMulPrim();
		return true;
	}

	if (consume(DIV))
	{
		if (!exprCast())
			tkerr("Missing or invalid operand after /");

		exprMulPrim();
		return true;
	}

	return true;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast()
{
	Token* start = iTk;

	if (consume(LPAR))
	{
		if (typeBase())
		{
			arrayDecl();

			if (!consume(RPAR))
				tkerr("Missing ')' in cast expression");

			if (!exprCast())
				tkerr("Missing expression after cast");

			return true;
		}

		iTk = start; // LPAR seen but no typeBase - not a cast
	}

	if (exprUnary()) return true;

	iTk = start;
	return false;
}

// exprUnary: (SUB | NOT) exprUnary | exprPostfix
bool exprUnary()
{
	Token* start = iTk;

	if (consume(SUB))
	{
		if (!exprUnary())
			tkerr("Expected expression after unary -");

		return true;
	}

	if (consume(NOT))
	{
		if (!exprUnary())
			tkerr("Expected expression after unary !");

		return true;
	}

	if (exprPostfix()) 
		return true;

	iTk = start;
	return false;
}

// exprPostfix: exprPostfix LBRACKET expr RBRACKET | exprPostfix DOT ID | exprPrimary
// exprPostfix = exprPrimary exprPostfixPrim
// exprPostfixPrim = LBRACKET expr RBRACKET exprPostfixPrim | DOT ID exprPostfixPrim | ε
bool exprPostfix()
{
	Token* start = iTk;

	if (exprPrimary())
	{
		exprPostfixPrim();
		return true;
	}

	iTk = start;
	return false;
}

bool exprPostfixPrim()
{
	if (consume(LBRACKET))
	{
		if (expr())
		{
			if (consume(RBRACKET))
			{
				exprPostfixPrim();
				return true;
			}

			tkerr("Missing ] in index expression");
		}

		tkerr("Expected expression inside []");
	}

	if (consume(DOT))
	{
		if (consume(ID))
		{
			exprPostfixPrim();
			return true;
		}

		tkerr("Expected field name after .");
	}
	return true;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )? | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary()
{
	Token* start = iTk;

	if (consume(ID))
	{
		if (consume(LPAR))
		{
			if (expr())
			{
				while (consume(COMMA))
				{
					if (!expr()) 
						tkerr("Expected expression after ,");
				}
			}

			if (consume(RPAR))
				return true;

			tkerr("Missing ) in function call");
		}

		return true;
	}

	if (consume(INT))    return true;
	if (consume(DOUBLE)) return true;
	if (consume(CHAR))   return true;
	if (consume(STRING)) return true;

	if (consume(LPAR))
	{
		if (expr())
		{
			if (consume(RPAR)) 
				return true;

			tkerr("Missing ) after expression");
		}
		tkerr("Expected expression after (");
	}

	iTk = start;
	return false;
}