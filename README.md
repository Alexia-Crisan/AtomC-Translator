# AtomC Translator

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
