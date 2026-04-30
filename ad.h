#pragma once

// Forward declaration for the VM instruction type (defined in a later lab stage).
// Needed here because struct Symbol's fn union contains an Instr* field.
typedef struct Instr Instr;

struct Symbol; typedef struct Symbol Symbol;

typedef enum
{	// base type
	TB_INT, TB_DOUBLE, TB_CHAR, TB_VOID, TB_STRUCT
} TypeBase;

typedef struct
{	// the type of a symbol
	TypeBase tb;
	Symbol* s;		// for TB_STRUCT, the struct's symbol
	// n - the dimension for an array
	//		n<0  - no array
	//		n==0 - array without specified dimension: int v[]
	//		n>0  - array with specified dimension: double v[10]
	int n;
} Type;

// returns the size of type t in bytes
int typeSize(Type* t);

typedef enum
{
	// symbol's kind
	SK_VAR, SK_PARAM, SK_FN, SK_STRUCT
} SymKind;

struct Symbol
{
	const char* name;	// symbol's name — NOT owned by Symbol (points into token text)
	SymKind kind;
	Type type;
	// owner:
	//		- NULL for global symbols
	//		- a struct for variables defined in that struct
	//		- a function for parameters/variables local to that function
	Symbol* owner;
	Symbol* next;		// the link to the next symbol in list
	union
	{	// specific data for each kind of symbol
		// the index in fn.locals for local vars
		// the index in struct for struct members
		int varIdx;
		// the variable memory for global vars (dynamically allocated)
		void* varMem;
		// the index in fn.params for parameters
		int paramIdx;
		// the members of a struct
		Symbol* structMembers;
		struct
		{
			Symbol* params;		// the parameters of a function
			Symbol* locals;		// all local vars of a function, including ones from inner domains
			void (*extFnPtr)();	// !=NULL for extern functions
			Instr* instr;		// used if extFnPtr==NULL (VM lab stage)
		} fn;
	};
};

// dynamically allocates a new symbol
Symbol* newSymbol(const char* name, SymKind kind);
// duplicates the given symbol
Symbol* dupSymbol(Symbol* symbol);
// adds the symbol to the end of the list
Symbol* addSymbolToList(Symbol** list, Symbol* s);
// the number of symbols in list
int symbolsLen(Symbol* list);
// frees the memory of a symbol
void freeSymbol(Symbol* s);
// displays a symbol
void showSymbol(Symbol* s);

typedef struct _Domain
{
	struct _Domain* parent;		// the parent domain
	Symbol* symbols;			// the symbols from this domain (single linked list)
} Domain;

// the current domain (top of the domain stack)
extern Domain* symTable;
// the current enclosing function or struct symbol (NULL at global scope)
extern Symbol* owner;

// adds a domain to the top of the domain stack
Domain* pushDomain();
// deletes the domain from the top of the domain stack
void dropDomain();
// shows the content of the given domain
void showDomain(Domain* d, const char* name);
// searches for a symbol by name in domain d only; returns NULL if not found
Symbol* findSymbolInDomain(Domain* d, const char* name);
// searches all domains from current to global; returns NULL if not found
Symbol* findSymbol(const char* name);
// adds a symbol to domain d
Symbol* addSymbolToDomain(Domain* d, Symbol* s);

// adds an externally-linked function to the current domain
Symbol* addExtFn(const char* name, void(*extFnPtr)(), Type ret);
// appends a parameter to fn (no redefinition check)
Symbol* addFnParam(Symbol* fn, const char* name, Type type);