# AtomC Translator

A compiler for the AtomC language — a simplified subset of C — implemented in six lab stages across seven logical components: lexical analysis, syntax analysis, domain analysis, type analysis, virtual machine, and code generation.

---

## Part 1 — Lexical Analyzer

### What it does

The lexical analyzer (lexer) is the first phase of the compiler. It reads the raw source code as a string of characters and groups them into a flat linked list of tokens. Each token has:
- a **code** identifying its type (e.g. `IF`, `INT`, `ID`)
- a **line number** from the source file
- an optional **value** (identifier name, numeric value, character value, etc.)

Comments and whitespace are discarded at this stage and never appear in the token list.

---

### Token types

**Identifiers & keywords**
| Token | Source |
|---|---|
| `ID` | any user-defined name: variables, functions, struct names |
| `TYPE_INT` | `int` |
| `TYPE_CHAR` | `char` |
| `TYPE_DOUBLE` | `double` |
| `IF` `ELSE` `WHILE` `RETURN` `VOID` `STRUCT` | control flow and structure keywords |

**Constants**
| Token | Description |
|---|---|
| `INT` | integer literals: `0`, `108` |
| `DOUBLE` | real literals with decimal or scientific notation: `4.9`, `49e-1`, `0.49E1` |
| `CHAR` | character literals with escape support: `'a'`, `'\''`, `'\n'` |
| `STRING` | string literals with escape support: `"hello\n"`, `"say \""` |

**Delimiters**
`COMMA` `,` — `SEMICOLON` `;` — `LPAR` `(` — `RPAR` `)` — `LBRACKET` `[` — `RBRACKET` `]` — `LACC` `{` — `RACC` `}` — `END` `\0`

**Operators**
`ADD` `+` — `SUB` `-` — `MUL` `*` — `DIV` `/` — `DOT` `.`
`AND` `&&` — `OR` `||` — `NOT` `!`
`ASSIGN` `=` — `EQUAL` `==` — `NOTEQ` `!=`
`LESS` `<` — `LESSEQ` `<=` — `GREATER` `>` — `GREATEREQ` `>=`

---

### DOUBLE grammar (from AtomC spec)

```
Form 1: [0-9]+ '.' [0-9]+ ( [eE] [+-]? [0-9]+ )?    →  4.9   3.14e-2
Form 2: [0-9]+               [eE] [+-]? [0-9]+        →  49e-1  2E10
```

---

### Key functions

| Function | Purpose |
|---|---|
| `addTk(code)` | Creates a token, sets code and line, appends to the linked list |
| `extract(begin, end)` | Copies a substring from source into newly allocated memory |
| `consumeEscape(&pch)` | Decodes a single escape sequence (`\n`, `\t`, `\'`, etc.) into its ASCII value |
| `consumeLineComment(pch)` | Skips all characters from `//` to end of line |
| `handleChar(pch)` | Parses a character constant `'...'` including escape sequences |
| `handleString(pch)` | Parses a string literal `"..."` with a growable buffer and escape handling |
| `handleNumber(pch)` | Parses `INT` or `DOUBLE` — detects `.` and `e`/`E` exponent |
| `handleKeyword(pch)` | Reads a full word, checks against all keywords, emits keyword token or `ID` |
| `tokenize(pch)` | Main loop — `switch` on first character routes to the right handler |
| `showTokensDetailed(tokens, out)` | Writes the full token list to a `FILE*` in `line\tTYPE:value` format |

---

## Part 2 — Syntax Analyzer

### What it does

The syntax analyzer (parser) takes the linked list of tokens produced by the lexer and validates that they follow the AtomC grammar rules.

The parser is implemented as a **recursive descent parser** with backtracking. Each grammar rule is mapped to a C function returning `bool`:
- `true` → the rule matches and all its tokens are consumed
- `false` → the rule does not match and the token iterator is restored (backtracking)

On syntax errors the parser calls `tkerr`, which prints the line number and exits.

---

### Grammar (AtomC)

```
unit:        ( structDef | fnDef | varDef )* END
structDef:   STRUCT ID LACC varDef* RACC SEMICOLON
varDef:      typeBase ID arrayDecl? SEMICOLON
typeBase:    TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
arrayDecl:   LBRACKET INT? RBRACKET
fnDef:       ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR stmCompound
fnParam:     typeBase ID arrayDecl?
stm:         stmCompound
           | IF LPAR expr RPAR stm ( ELSE stm )?
           | WHILE LPAR expr RPAR stm
           | RETURN expr? SEMICOLON
           | expr? SEMICOLON
stmCompound: LACC ( varDef | stm )* RACC
expr:        exprAssign
exprAssign:  exprUnary ASSIGN exprAssign | exprOr
exprOr:      exprOr OR exprAnd | exprAnd
exprAnd:     exprAnd AND exprEq | exprEq
exprEq:      exprEq ( EQUAL | NOTEQ ) exprRel | exprRel
exprRel:     exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
exprAdd:     exprAdd ( ADD | SUB ) exprMul | exprMul
exprMul:     exprMul ( MUL | DIV ) exprCast | exprCast
exprCast:    LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
exprUnary:   ( SUB | NOT ) exprUnary | exprPostfix
exprPostfix: exprPostfix LBRACKET expr RBRACKET
           | exprPostfix DOT ID
           | exprPrimary
exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
           | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
```

Left-recursive rules (`exprOr`, `exprAnd`, etc.) are transformed using the standard `A → β A'` / `A' → α A' | ε` pattern before implementation.

---

## Part 3 — Domain Analysis (AD)

### What it does

Domain analysis runs simultaneously with parsing. As each declaration is parsed, a corresponding `Symbol` is created and inserted into the **symbol table** — a stack of `Domain` structures. Each domain holds the symbols declared at that scope level.

- **Global scope**: variables, functions, structs declared at the top level
- **Function scope**: opened after `LPAR`, closed after the function body
- **Block scope**: each `stmCompound(true)` (if/while bodies) creates a nested domain

Redefinitions within the same domain are detected and reported as errors.

---

### Symbol kinds

| Kind | Used for |
|---|---|
| `SK_VAR` | variables (local, global, struct members) |
| `SK_PARAM` | function parameters |
| `SK_FN` | functions (user-defined and external) |
| `SK_STRUCT` | struct type definitions |

---

### Key data structures

- `Symbol` — name, kind, type (`TypeBase` + array dimension `n` + struct pointer `s`), owner, indices
- `Domain` — singly-linked list of `Symbol*` + pointer to parent domain
- `symTable` — global pointer to the top of the domain stack
- `owner` — global pointer to the current enclosing function or struct (`NULL` at global scope)

---

### Key functions

| Function | Purpose |
|---|---|
| `pushDomain()` | Creates and pushes a new domain onto the stack |
| `dropDomain()` | Pops and frees the top domain with all its symbols |
| `findSymbolInDomain(d, name)` | Searches only domain `d` — used for redefinition checks |
| `findSymbol(name)` | Searches all domains from current to global |
| `addSymbolToDomain(d, s)` | Appends symbol `s` to domain `d` |
| `addExtFn(name, ptr, ret)` | Registers an external (host) function in the global domain |
| `addFnParam(fn, name, type)` | Appends a parameter to a function's parameter list |

---

### Errors detected

- Symbol redefinition within the same domain
- Array variable declared without a dimension (`int v[]` in a `varDef`)
- Use of an undefined struct type

---

## Part 4 — Type Analysis (AT)

### What it does

Type analysis runs as semantic actions inside the parser. Every expression function gains a `Ret*` out-parameter that carries type information upward through the parse tree.

```c
typedef struct {
    Type type;   // the expression's result type
    bool lval;   // true if the expression has an address (can appear left of =)
    bool ct;     // true if constant (cannot be assigned to)
} Ret;
```

**lval (left-value)**: an expression with a memory address — e.g. variable `x` is an lval. Literals and computed results are not lvals.

**ct (constant)**: even if addressable, cannot be written to — e.g. array names have `lval=true` but `ct=true`.

---

### Key helper functions (`at.c`)

| Function | Purpose |
|---|---|
| `canBeScalar(r)` | Returns true if `r` is a non-array, non-void, non-struct type |
| `convTo(src, dst)` | Returns true if `src` type can be implicitly converted to `dst` |
| `arithTypeTo(t1, t2, dst)` | Computes the arithmetic result type of `t1 op t2`; returns false for structs/arrays |
| `findSymbolInList(list, name)` | Searches a singly-linked symbol list by name (used for struct fields) |

---

### Errors detected

| Location | Rule |
|---|---|
| `stm` IF/WHILE | condition must be scalar |
| `stm` RETURN | void functions cannot return a value; non-void must return; types must be compatible |
| `exprAssign` | destination must be lval, non-constant, scalar; source must be scalar and convertible |
| `exprOr/And/Eq/Rel` | both operands must be scalar non-struct |
| `exprAdd/Mul` | both operands must be scalar non-struct; result type follows arithmetic promotion |
| `exprCast` | cannot cast to/from struct; array↔scalar casts forbidden |
| `exprUnary` | operand must be scalar |
| `exprPostfix []` | only arrays can be indexed; index must be convertible to int |
| `exprPostfix .` | only structs have fields; field must exist |
| `exprPrimary` | identifier must exist; functions must be called; argument count and types must match |

---

## Part 5 — Virtual Machine (VM)

### What it does

The VM is a **stack-based** interpreter that executes the instruction sequences generated by the code generator. It has:
- A stack `Val stack[10000]` where each cell is a `Val` union (can hold `int`, `double`, `void*`, function pointer, or `Instr*`)
- **SP** (Stack Pointer) — points to the topmost occupied stack cell
- **FP** (Frame Pointer) — points to the base of the current function's stack frame
- **IP** (Instruction Pointer) — pointer walking the `Instr` linked list

---

### Stack frame layout

When `f(arg)` is called, the stack looks like:

```
FP[-2]  =  arg          (pushed by caller before CALL)
FP[-1]  =  return addr  (pushed by OP_CALL)
FP[ 0]  =  old FP       (pushed by OP_ENTER)
FP[ 1]  =  local_0      (allocated by OP_ENTER n)
FP[ 2]  =  local_1
...
```

---

### Instruction set

| Instruction | Effect |
|---|---|
| `OP_HALT` | stops execution |
| `OP_PUSH_I [n]` | pushes int constant `n` |
| `OP_PUSH_F [f]` | pushes double constant `f` |
| `OP_CALL [instr]` | pushes return address, jumps to `instr` |
| `OP_CALL_EXT [ptr]` | calls external C function at `ptr` (args/ret via stack) |
| `OP_ENTER [n]` | saves FP, sets FP=SP, allocates `n` local slots |
| `OP_RET [n]` | returns value; restores SP/FP/IP; `n` = number of params |
| `OP_RET_VOID [n]` | returns without value; same cleanup |
| `OP_JMP [instr]` | unconditional jump |
| `OP_JF [instr]` | jumps if top of stack is 0 (false) |
| `OP_JT [instr]` | jumps if top of stack is non-zero (true) |
| `OP_FPLOAD [idx]` | pushes `FP[idx]` (raw Val) |
| `OP_FPSTORE [idx]` | pops and stores to `FP[idx]` |
| `OP_FPADDR_I [idx]` | pushes `&FP[idx].i` |
| `OP_FPADDR_F [idx]` | pushes `&FP[idx].f` |
| `OP_ADDR [p]` | pushes pointer `p` (used for global variables) |
| `OP_LOAD_I` | pops address, pushes `*(int*)addr` |
| `OP_LOAD_F` | pops address, pushes `*(double*)addr` |
| `OP_STORE_I` | pops int value + address, writes value, leaves value on stack |
| `OP_STORE_F` | pops double value + address, writes value, leaves value on stack |
| `OP_ADD_I/F` | pops two values, pushes sum |
| `OP_SUB_I/F` | pops two values, pushes difference |
| `OP_MUL_I/F` | pops two values, pushes product |
| `OP_DIV_I/F` | pops two values, pushes quotient |
| `OP_LESS_I/F` | pops two values, pushes int 0 or 1 |
| `OP_CONV_I_F` | pops int, pushes double |
| `OP_CONV_F_I` | pops double, pushes int (truncates) |
| `OP_DROP` | discards top of stack |
| `OP_NOP` | no operation (used as jump target) |

---

### External functions

Functions like `put_i` and `put_d` are implemented in C inside the compiler. They receive arguments and leave results on the stack. They are registered before parsing via `vmInit()` so the type analyser can find them.

```c
void put_i() { printf("=> %d", popi()); }
void put_d() { printf("=> %g", popf()); }
```

---

### Integration order in `main.c`

```c
pushDomain();          // 1. create global symbol table domain
vmInit();              // 2. register put_i, put_d BEFORE parsing
parse(tks);            // 3. parse + AD + AT + code generation
Symbol *symMain = findSymbolInDomain(symTable, "main");
Instr *entry = NULL;
addInstr(&entry, OP_CALL)->arg.instr = symMain->fn.instr;
addInstr(&entry, OP_HALT);
run(entry);            // 4. execute compiled code
dropDomain();          // 5. cleanup
```

---

## Part 6 — VM Homework (double support)

### What it does

Extends the VM with `double` arithmetic instructions and the `put_d` external function, then generates VM instructions manually for the homework program:

```c
void f(double n) {
    double i;
    i = 0.0;
    while (i < n) {
        put_d(i);
        i = i + 0.5;
    }
}
```

Called as `f(2.0)` which produces: `=> 0`, `=> 0.5`, `=> 1`, `=> 1.5`

### Stack frame for `f(double n)`

```
FP[-2] = n          (double argument)
FP[-1] = ret addr
FP[ 0] = old FP
FP[ 1] = i          (double local, 1 slot — ENTER 1)
```

### Generated instruction sequence

```
PUSH.f 2.0          <- argument n=2.0
CALL f              <- push ret addr (HALT), jump to ENTER
HALT

f: ENTER 1          <- push old_FP, FP=SP, SP+=1 (slot for i)
   PUSH.f 0.0       <- initial value for i
   FPSTORE 1        <- FP[1] = 0.0

F1: FPLOAD 1        <- push i (for comparison)
    FPLOAD -2       <- push n (for comparison)
    LESS.f          <- i < n -> int 0 or 1
    JF F2           <- if false, exit loop

    FPLOAD 1        <- push i (argument for put_d)
    CALL_EXT put_d  <- print i, pops double

    FPLOAD 1        <- push i (for addition)
    PUSH.f 0.5
    ADD.f           <- i + 0.5
    FPSTORE 1       <- FP[1] = i + 0.5

    JMP F1

F2: RET_VOID 1      <- return, clean up 1 parameter (n)
```

---

## Part 7 — Code Generation

### What it does

Code generation adds semantic actions inside the parser rules so that as each construct is parsed, VM instructions are emitted directly into the current function's instruction list (`owner->fn.instr`). After `parse()` completes, every function symbol has a fully populated `fn.instr` chain ready to run.

---

### Key helpers (`gc.c`)

| Function | Purpose |
|---|---|
| `addRVal(list, lval, type)` | If `lval` is true (address on stack), emits `LOAD_I` or `LOAD_F` to get the value |
| `insertConvIfNeeded(last, src, dst)` | Inserts `CONV_I_F` or `CONV_F_I` after `last` if types differ |

---

### Instruction backtracking

When the parser backtracks (resets `iTk = start`), any instructions already emitted into `fn.instr` must also be undone. This is done by saving `startInstr = lastInstr(owner->fn.instr)` before potentially-failing code and calling `delInstrAfter(startInstr)` alongside the token backtrack.

Critical locations:
- **`exprAssign`**: saves `startInstr` before `exprUnary`, calls `delInstrAfter` when ASSIGN is not found
- **`exprCast`**: saves `startInstr` after LPAR, calls `delInstrAfter` when `typeBase` fails
- **`stm`**: saves `startInstr` at entry, calls `delInstrAfter` when all alternatives fail

---

### What is generated per construct

**`fnDef`**
- Emits `ENTER` with placeholder; patches argument after body (now know locals count)
- Void functions automatically get `RET_VOID nparams` appended

**`stm` — IF/ELSE**
```
[condition code]
addRVal + insertConvIfNeeded
JF -> else_start (or end if no else)
[then body]
JMP -> end          <- only if ELSE present
NOP (else_start)
[else body]         <- only if ELSE present
NOP (end)
```

**`stm` — WHILE**
```
NOP (loop_start = beforeWhileCond->next)
[condition code]
addRVal + insertConvIfNeeded
JF -> after
[body]
JMP -> loop_start
NOP (after)
```

**`stm` — RETURN**
- With value: `addRVal` + `insertConvIfNeeded` + `RET nparams`
- Without value: `RET_VOID`

**`exprAssign`**
- Left side emits address (lval); right side: `addRVal` + `insertConvIfNeeded` + `STORE_I/F`

**`exprRel/Add/Mul` (Prim variants)**
- `lastLeft = lastInstr(...)` before left load
- `addRVal` for left, parse right, `addRVal` for right
- `insertConvIfNeeded` on both sides to `tDst`
- Typed opcode: `LESS_I/F`, `ADD_I/F`, `SUB_I/F`, `MUL_I/F`, `DIV_I/F`

**`exprPrimary` — variable access**
- Global: `ADDR varMem`
- Local: `FPADDR_I/F varIdx+1`
- Parameter: `FPADDR_I/F paramIdx-nparams-1`

**`exprPrimary` — function call**
- Per argument: `addRVal` + `insertConvIfNeeded` to param type
- External: `CALL_EXT extFnPtr`; VM function: `CALL fn.instr`

**`exprPrimary` — literals**
- `INT`/`CHAR` → `PUSH_I value`; `DOUBLE` → `PUSH_F value`

---

### Test program (`testgc.c`) walkthrough

```c
int fact(int n) {
    if (n < 3) return n;
    return n * fact(n - 1);
}

void main() {
    put_i(4.9);       // CONV.f.i -> prints 4
    put_i(fact(3));   // recursive: 3*2 = 6
    int r; r = 1;
    int i; i = 2;
    while (i < 5) { r = r * i; i = i + 1; }
    put_i(r);         // 1*2*3*4 = 24
}
```

**Stack frame for `fact(int n)`** — 0 locals, 1 parameter:
```
FP[-2] = n    FP[-1] = ret addr    FP[0] = old FP
```

**Stack frame for `void main()`** — 2 locals (`r` at FP[1], `i` at FP[2]), 0 parameters:
```
FP[-1] = ret addr    FP[0] = old FP    FP[1] = r    FP[2] = i
```

**Recursive call trace for `fact(3)`:**
```
fact(3): n=3, 3<3 -> false -> compute 3 * fact(2)
  fact(2): n=2, 2<3 -> true -> return 2
3 * 2 = 6
```

Expected output: `=> 4`, `=> 6`, `=> 24`

---

## File overview

| File | Role |
|---|---|
| `lexer.h` / `lexer.c` | Lexical analysis — tokenizer |
| `parser.h` / `parser.c` | Syntax analysis + AD + AT + code generation |
| `ad.h` / `ad.c` | Symbol table — domain analysis data structures and functions |
| `at.h` / `at.c` | Type analysis helpers (`canBeScalar`, `convTo`, `arithTypeTo`) |
| `vm.h` / `vm.c` | Virtual machine — opcodes, stack, `run()`, `vmInit()`, test programs |
| `gc.h` / `gc.c` | Code generation helpers (`addRVal`, `insertConvIfNeeded`) |
| `utils.h` / `utils.c` | Error reporting (`err`), memory (`safeAlloc`), file loading (`loadFile`) |
| `main.c` | Entry point — ties all stages together |
| `tests/testgc.c` | Code generation test (factorial, expected: 4, 6, 24) |
| `tests/testvm.c` | VM homework test (double loop, expected: 0, 0.5, 1, 1.5) |
| `tests/testat.c` | Type analysis test (should parse with no errors) |
| `tests/testad.c` | Domain analysis test |