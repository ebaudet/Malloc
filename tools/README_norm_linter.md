# Norm Linter

`norm_linter.py` is a local, dependency-free Python linter for common 42 Norm
rules on C source and header files.

It is not a replacement for the official `norminette`: it is a readable and
modular pre-checker intended to catch most mechanical issues quickly.

## Usage

From the repository root:

```sh
python3 tools/norm_linter.py
```

By default it checks:

```text
srcs
includes
Makefile
```

To check explicit paths:

```sh
python3 tools/norm_linter.py srcs includes Makefile
```

Only `.c`, `.h`, and `Makefile` files are linted. Files inside `/tests` are
skipped unless requested:

```sh
python3 tools/norm_linter.py --include-tests .
```

List available rules:

```sh
python3 tools/norm_linter.py --list-rules
```

Disable a rule:

```sh
python3 tools/norm_linter.py --disable FILE_FUNC_COUNT
```

Disable several rules:

```sh
python3 tools/norm_linter.py --disable FILE_FUNC_COUNT --disable LINE_LEN
```

Color output is automatic when stdout is a terminal:

```sh
python3 tools/norm_linter.py --color auto
python3 tools/norm_linter.py --color always
python3 tools/norm_linter.py --color never
```

The standard `NO_COLOR` environment variable disables colors:

```sh
NO_COLOR=1 python3 tools/norm_linter.py
```

Rule coverage for the requested Norm checklist is documented in
[`NORM_RULE_COVERAGE.md`](NORM_RULE_COVERAGE.md).

## Output Format

The report is grouped by file. Each issue contains:

```text
line:column  RULE_ID
    clear message
    line | offending source line
         | caret
```

Example:

```text
Norm Linter
Checked: 6 file(s)
Issues : 2

srcs/ft_malloc.c
  1:1  FILE_FUNC_COUNT
      File has 23 functions; maximum is 5.
  282:5  FORMAT_MULTI_SPACE
      Multiple consecutive spaces are not allowed in code.
      282 |     i =  5;
          |        ^
```

File-level errors, such as `FILE_FUNC_COUNT`, do not print a source snippet.
This avoids showing unrelated banner/header lines as if they were the problem.

The linter does not stop at the first error. It runs every enabled rule on
every readable file and prints every issue it can find. If one rule crashes, the
crash is reported with `LINTER_RULE_ERROR` and the next rules still run.

## Architecture

Rules are declared in the `RULES` list near the bottom of
`tools/norm_linter.py`:

```python
Rule("FUNC_LEN", "Maximum function length", check_function_line_count)
```

Each rule has:

- a stable id, such as `FUNC_LEN`;
- a short title;
- a function that receives a `SourceFile` and returns a list of `Violation`.

To add a rule:

1. Create a `check_*` function.
2. Return `Violation` objects through `make_violation(...)`.
3. Add the rule to `RULES`.

To remove or temporarily disable a rule, delete it from `RULES` or pass
`--disable RULE_ID` on the command line.

## Implemented Rule Groups

- File formatting: final newline, line length, trailing whitespace, repeated
  blank lines, repeated spaces in code.
- Indentation: tabs-only indentation.
- Functions: maximum 25 lines, maximum 5 functions per `.c` file, maximum 4
  parameters, maximum 5 local variables.
- Braces: function and control block opening braces on their own line.
- Forbidden syntax: `for`, `do`, `switch`, `case`, `goto`, ternary operator.
- Statements and declarations: one statement per line, declarations at the top,
  no declaration initialization, one declaration per line.
- Naming: functions, global variables, typedefs, defines.
- Preprocessor: includes before code, header guards.

## Known Limits

The parser is deliberately lightweight. It strips comments and strings, then
uses brace scanning and regular expressions. This keeps the tool easy to modify
but means very complex C syntax can produce false positives or false negatives.

When in doubt, the official `norminette` remains the source of truth.
