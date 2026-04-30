#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "ad.h"
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
	pushDomain();	// AD: create the global domain
	if (!unit())
		tkerr("Syntax error");
	showDomain(symTable, "global"); // AD: display global domain
	dropDomain();                   // AD: release global domain
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
			Token* tkName = consumedTk; // AD: save struct name token

			if (consume(LACC))
			{
				// AD: struct name must be unique in current domain
				Symbol* s = findSymbolInDomain(symTable, tkName->text);
				if (s)
					tkerr("symbol redefinition: %s", tkName->text);

				s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
				s->type.tb = TB_STRUCT;
				s->type.s = s;
				s->type.n = -1;

				pushDomain(); // AD: open struct's domain
				owner = s;    // AD: set owner to this struct

				while (varDef()) {} // optional

				if (consume(RACC))
				{
					if (consume(SEMICOLON))
					{
						owner = NULL;  // AD: leave struct scope
						dropDomain();  // AD: close struct domain

						return true;
					}

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
	Type t;	 // AD: local type accumulator

	if (typeBase(&t))
	{
		if (consume(ID))
		{
			Token* tkName = consumedTk; // AD: save variable name token

			if (arrayDecl(&t)) // AD: optional; updates t.n
			{
				// AD: array variables must have a specified dimension
				if (t.n == 0)
					tkerr("AD: A vector variable must have a specified dimension");
			}

			if (consume(SEMICOLON))
			{
				// AD: check for redefinition in current domain
				Symbol* var = findSymbolInDomain(symTable, tkName->text);

				if (var)
					tkerr("AD: Symbol redefinition: %s", tkName->text);

				var = newSymbol(tkName->text, SK_VAR);
				var->type = t;
				var->owner = owner;
				addSymbolToDomain(symTable, var);

				if (owner)
				{
					switch (owner->kind)
					{
					case SK_FN:
						// AD: local variable of a function
						var->varIdx = symbolsLen(owner->fn.locals);
						addSymbolToList(&owner->fn.locals, dupSymbol(var));
						break;
					case SK_STRUCT:
						// AD: member variable of a struct
						var->varIdx = typeSize(&owner->type);
						addSymbolToList(&owner->structMembers, dupSymbol(var));
						break;
					default: break;
					}
				}
				else
				{
					// AD: global variable — allocate storage
					var->varMem = safeAlloc(typeSize(&t));
				}

				return true;
			}

			tkerr("Missing ; after variable definition");
		}
	}

	iTk = start;
	return false;
}


// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(Type* t)
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
			Token* tkName = consumedTk; // AD: save struct name token

			// AD: struct must already be defined
			t->tb = TB_STRUCT;
			t->s = findSymbol(tkName->text);
			if (!t->s)
				tkerr("AD: Undefined struct: %s", tkName->text);

			return true;
		}

		tkerr("Expected struct name after keyword struct");
	}

	return false;
}

// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl(Type* t)
{
	Token* start = iTk;

	if (consume(LBRACKET))
	{
		if (consume(INT))
		{
			Token* tkSize = consumedTk; // AD: save size token
			t->n = tkSize->i;           // AD: store dimension
		}
		else
		{
			t->n = 0; // AD: array without specified dimension: int v[]
		}

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
	Type t;

	bool hasType = typeBase(&t);
	if (!hasType && consume(VOID))
	{
		t.tb = TB_VOID;
		t.n = -1;
		hasType = true;
	}


	if (hasType)
	{
		if (consume(ID))
		{
			Token* tkName = consumedTk; // AD: save function name token

			if (consume(LPAR))
			{
				// AD: function name must be unique in current (global) domain
				Symbol* fn = findSymbolInDomain(symTable, tkName->text);
				if (fn)
					tkerr("AD: Symbol redefinition: %s", tkName->text);

				fn = newSymbol(tkName->text, SK_FN);
				fn->type = t;
				addSymbolToDomain(symTable, fn);

				owner = fn;   // AD: set owner to this function
				pushDomain(); // AD: open function local domain right after LPAR

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
					if (stmCompound(false))
					{
						dropDomain(); // AD: close function's domain
						owner = NULL; // AD: leave function scope
						return true;
					}

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
	Type t;

	if (typeBase(&t))
	{
		if (consume(ID))
		{
			Token* tkName = consumedTk; // AD: save parameter name token

			if (arrayDecl(&t))
			{
				t.n = 0; // AD: param arrays lose their dimension (int v[10] -> int v[])
			}

			// AD: parameter name must be unique in current (function's) domain
			Symbol* param = findSymbolInDomain(symTable, tkName->text);
			if (param)
				tkerr("symbol redefinition: %s", tkName->text);

			param = newSymbol(tkName->text, SK_PARAM);
			param->type = t;
			param->owner = owner;
			param->paramIdx = symbolsLen(owner->fn.params);

			// AD: add to current domain AND to the function's param list
			addSymbolToDomain(symTable, param);
			addSymbolToList(&owner->fn.params, dupSymbol(param));

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

	if (stmCompound(true)) return true;

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
bool stmCompound(bool newDomain)
{
	Token* start = iTk;

	if (consume(LACC))
	{
		if (newDomain) pushDomain(); // AD: open new domain only when requested

		for (;;)
		{
			if (varDef()) {}
			else if (stm()) {}
			else break;
		}

		if (consume(RACC))
		{
			if (newDomain) dropDomain(); // AD: close domain

			return true;
		}

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
	Type t;

	if (consume(LPAR))
	{
		if (typeBase(&t))
		{
			arrayDecl(&t);

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