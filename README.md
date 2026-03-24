# AtomC Translator

---

## Part 1 - Lexical Analyzer

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
`COMMA` `,` - `SEMICOLON` `;` - `LPAR` `(` - `RPAR` `)` - `LBRACKET` `[` - `RBRACKET` `]` - `LACC` `{` - `RACC` `}` - `END` `\0`

**Operators**
`ADD` `+` - `SUB` `-` - `MUL` `*` - `DIV` `/` - `DOT` `.`
`AND` `&&` - `OR` `||` - `NOT` `!`
`ASSIGN` `=` - `EQUAL` `==` - `NOTEQ` `!=`
`LESS` `<` - `LESSEQ` `<=` - `GREATER` `>` - `GREATEREQ` `>=`

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
| `handleNumber(pch)` | Parses `INT` or `DOUBLE` - detects `.` and `e`/`E` exponent |
| `handleKeyword(pch)` | Reads a full word, checks against all keywords, emits keyword token or `ID` |
| `tokenize(pch)` | Main loop - `switch` on first character routes to the right handler |
| `showTokensDetailed(tokens, out)` | Writes the full token list to a `FILE*` in `line\tTYPE:value` format |

---

## Part 2 - Syntax Analyzer

### What it does

The syntax analyzer (parser) is the second phase of the compiler. It takes the linked list of tokens produced by the lexer and validates that they follow the AtomC grammar rules.

The parser is implemented as a **recursive descent parser** with backtracking. Each grammar rule is mapped to a C function returning `bool`:
- `true` → the rule matches
- `false` → the rule does not match (and tokens are restored)

On syntax errors, the parser stops execution and reports the line number.

---

### Grammar (AtomC)

- unit: ( structDef | fnDef | varDef )* END

- structDef: STRUCT ID LACC varDef* RACC SEMICOLON

- varDef: typeBase ID arrayDecl? SEMICOLON

- typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID

- arrayDecl: LBRACKET INT? RBRACKET

- fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR stmCompound

- fnParam: typeBase ID arrayDecl?

- stm: stmCompound | IF LPAR expr RPAR stm ( ELSE stm )? | WHILE LPAR expr RPAR stm | RETURN expr? SEMICOLON | expr? SEMICOLON

- stmCompound: LACC ( varDef | stm )* RACC

- expr: exprAssign

- exprAssign: exprUnary ASSIGN exprAssign | exprOr

- exprOr: exprOr OR exprAnd | exprAnd

- exprAnd: exprAnd AND exprEq | exprEq

- exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel

- exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd

- exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul

- exprMul: exprMul ( MUL | DIV ) exprCast | exprCast

- exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary

-  exprUnary: ( SUB | NOT ) exprUnary | exprPostfix

- exprPostfix: exprPostfix LBRACKET expr RBRACKET | exprPostfix DOT ID | exprPrimary

- exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )? | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
