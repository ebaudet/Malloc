# Norm Rule Coverage

This file maps the requested Norm rules to `tools/norm_linter.py` rule ids.

The linter is intentionally heuristic. It catches many mechanical violations,
but the official `norminette` remains the source of truth for edge cases that
need a full parser.

## Naming

| Norm rule | Linter rule |
| --- | --- |
| Struct names start with `s_` | `TYPE_STRUCT` via `TYPE_NAMES` |
| Typedef names start with `t_` | `TYPE_TYPEDEF` via `TYPE_NAMES` |
| Union names start with `u_` | `TYPE_UNION` via `TYPE_NAMES` |
| Enum names start with `e_` | `TYPE_ENUM` via `TYPE_NAMES` |
| Global names start with `g_` | `NAME_GLOBAL` |
| Variable and function names use lower snake_case | `NAME_VARIABLE`, `NAME_FUNCTION` |
| File and directory names use lower snake_case | `FILE_NAME_CASE` |
| File is compilable | `FILE_COMPILE` |
| ASCII-only files | `FILE_ASCII` |

## Formatting

| Norm rule | Linter rule |
| --- | --- |
| Standard 42 header starts at line 1 | `FILE_42_HEADER` |
| Function body max 25 lines, excluding braces | `FUNC_LEN` |
| Max 80 columns | `LINE_LEN` |
| One statement per line | `STMT_ONE_PER_LINE` |
| Empty lines contain no whitespace | `LINE_TRAILING_WS` |
| No trailing spaces/tabs | `LINE_TRAILING_WS` |
| Braces and control-structure ends on their own line | `BRACE_FUNC_OPEN`, `BRACE_CONTROL_OPEN`, `BRACE_ALONE` |
| Indentation uses tabs | `INDENT_TABS` |
| Comma/semicolon followed by space unless EOL | `FORMAT_PUNCT_SPACE` |
| Operators surrounded by one space | `FORMAT_OPERATOR_SPACE`, `FORMAT_MULTI_SPACE` |
| C keywords followed by space | `FORMAT_KEYWORD_SPACE` |
| Variable declarations aligned | `DECL_ALIGNMENT` |
| Pointer stars glued to variable name | `DECL_POINTER_STAR` |
| One variable declaration per line | `DECL_ONE_PER_LINE` |
| No declaration plus initialization, except globals/statics | `DECL_INIT` |
| Declarations at start and separated by blank line | `DECL_AT_TOP`, `DECL_BLANK_LINE` |
| No blank line inside declarations or implementation | `LINE_MULTI_BLANK` |
| Continued lines indented; operators at line start | Partially covered by `INDENT_TABS`, `FORMAT_OPERATOR_SPACE` |
| Multiple assignment forbidden | `ASSIGN_MULTIPLE` |

## Functions

| Norm rule | Linter rule |
| --- | --- |
| Max 4 named parameters | `FUNC_PARAM_COUNT` |
| No-argument functions use `void` | `FUNC_VOID_PARAM` |
| Prototype parameters have names | `PROTO_PARAM_NAME` |
| Function definitions separated by blank line | `FUNC_SEPARATION` |
| Function names aligned in one file/header | `FUNC_NAME_ALIGNMENT` |
| Max 5 function definitions in a `.c` file | `FILE_FUNC_COUNT` |

## Types And Headers

| Norm rule | Linter rule |
| --- | --- |
| Tabs in struct/enum/union declarations | Partially covered by `INDENT_TABS` |
| Struct/enum/union variable type spacing | Partially covered by `FORMAT_MULTI_SPACE` |
| Tab between typedef parameters | Partially covered by `FORMAT_PROTO_TAB`, `TYPE_NAMES` |
| Typedef name aligned with type name | Partially covered by `TYPE_NAMES` |
| No struct/enum/union declaration in `.c` | `TYPE_IN_C` |
| Header content restricted | `HEADER_CONTENT` |
| Includes at file beginning | `PREPROC_INCLUDE_TOP` |
| Header guard matches filename | `HEADER_GUARD` |
| Prototypes only in `.h` | Partially covered by `HEADER_CONTENT` |
| Unused includes forbidden | Not reliably covered without semantic analysis |
| `.c` includes forbidden | `INCLUDE_C_FILE` |

## Macros And Preprocessor

| Norm rule | Linter rule |
| --- | --- |
| Defines that define code forbidden | `MACRO_CODE` via `MACRO_RULES` |
| Multiline macros forbidden | `MACRO_MULTILINE` via `MACRO_RULES` |
| Macro names uppercase | `NAME_DEFINE` |
| Indent after `#if`, `#ifdef`, `#ifndef` | `PREPROC_IF_INDENT` via `MACRO_RULES` |

## Forbidden Syntax

| Norm rule | Linter rule |
| --- | --- |
| `for` forbidden | `FORBIDDEN_KEYWORD` |
| `do...while` forbidden | `FORBIDDEN_KEYWORD` |
| `switch` forbidden | `FORBIDDEN_KEYWORD` |
| `case` forbidden | `FORBIDDEN_KEYWORD` |
| `goto` forbidden | `FORBIDDEN_KEYWORD` |
| Nested ternary forbidden | `FORBIDDEN_NESTED_TERNARY` |
| Ternary operator forbidden by stricter projects | `FORBIDDEN_TERNARY` |

## Comments

| Norm rule | Linter rule |
| --- | --- |
| Comments allowed in source files | No blocking rule |
| No comments inside function bodies | `COMMENT_IN_FUNC` via `COMMENT_RULES` |
| Block comment shape | `COMMENT_BLOCK_STYLE` |
| No `//` comments | `COMMENT_CPP` via `COMMENT_RULES` |

## Makefile

| Norm rule | Linter rule |
| --- | --- |
| Mandatory `$(NAME)`, `clean`, `fclean`, `re`, `all` | `MAKEFILE_MANDATORY_RULES` |
| Makefile must not relink | Not reliably covered yet |
| Multibinary specific rules | Not covered generically |
| Automatic library compilation | Project-specific, not covered generically |
| Wildcard source discovery forbidden | `MAKEFILE_WILDCARD` |
