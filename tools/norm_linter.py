#!/usr/bin/env python3
"""
Lightweight, dependency-free linter for the 42 C Norm.

This tool is intentionally rule-oriented:
- every rule has a stable id, a title, and one check function;
- rules are registered in the RULES list near the bottom of the file;
- adding/removing a rule should only require editing that list and, when
  needed, adding a small check_* function.

The checks are conservative heuristics, not a full C compiler. They are useful
for catching common Norm failures locally before running the official
`norminette`. When a rule is difficult to validate perfectly without a full C
parser, the error message says what concrete pattern was found.
"""

from __future__ import annotations

import argparse
import dataclasses
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Callable, Iterable


MAX_LINE_LENGTH = 80
MAX_FUNCTION_LINES = 25
MAX_FUNCTIONS_PER_FILE = 5
MAX_FUNCTION_PARAMS = 4
MAX_LOCAL_VARIABLES = 5

CONTROL_KEYWORDS = {
    "if",
    "while",
    "return",
    "sizeof",
}

FORBIDDEN_KEYWORDS = {
    "for": "The Norm forbids for loops; use while.",
    "do": "The Norm forbids do/while loops.",
    "switch": "The Norm forbids switch statements.",
    "case": "The Norm forbids case labels.",
    "goto": "The Norm forbids goto.",
}

FILE_LEVEL_RULES = {
    "FILE_ASCII",
    "FILE_COMPILE",
    "FILE_FUNC_COUNT",
    "FILE_NAME_CASE",
    "FILE_42_HEADER",
    "MAKEFILE_MANDATORY_RULES",
    "MAKEFILE_WILDCARD",
    "LINTER_FILE_READ",
    "LINTER_FILE_ENCODING",
}

COLORS = {
    "reset": "\033[0m",
    "bold": "\033[1m",
    "red": "\033[31m",
    "green": "\033[32m",
    "yellow": "\033[33m",
    "blue": "\033[34m",
    "magenta": "\033[35m",
    "cyan": "\033[36m",
    "dim": "\033[2m",
}

MAKEFILE_RULES = {
    "FILE_ASCII",
    "FILE_NAME_CASE",
    "FILE_42_HEADER",
    "FILE_FINAL_NL",
    "LINE_LEN",
    "LINE_TRAILING_WS",
    "LINE_MULTI_BLANK",
    "MAKEFILE_MANDATORY_RULES",
    "MAKEFILE_WILDCARD",
}


@dataclasses.dataclass(frozen=True)
class Violation:
    """A single linter error."""

    path: Path
    line: int
    column: int
    rule_id: str
    message: str
    snippet: str = ""

    def format(self) -> str:
        location = f"{self.path}:{self.line}:{self.column}"
        text = f"{location}: {self.rule_id}: {self.message}"
        if self.snippet:
            return f"{text}\n    {self.snippet}"
        return text


@dataclasses.dataclass(frozen=True)
class Rule:
    """A named, independently removable Norm rule."""

    rule_id: str
    title: str
    check: Callable[["SourceFile"], list[Violation]]


@dataclasses.dataclass
class FunctionInfo:
    """Approximate function span and metadata."""

    name: str
    start_line: int
    open_line: int
    end_line: int
    signature: str
    body_lines: list[tuple[int, str]]


class SourceFile:
    """Parsed view of a C/header file used by all rules."""

    def __init__(self, path: Path):
        self.path = path
        self.text = path.read_text(encoding="utf-8", errors="replace")
        self.lines = self.text.splitlines(keepends=True)
        self.plain_lines = [line.rstrip("\n") for line in self.lines]
        self.code_lines = strip_comments_and_strings(self.plain_lines)
        self.functions = find_functions(self)


def make_violation(
    src: SourceFile,
    line: int,
    column: int,
    rule_id: str,
    message: str,
) -> Violation:
    snippet = ""
    if rule_id not in FILE_LEVEL_RULES and 1 <= line <= len(src.plain_lines):
        snippet = src.plain_lines[line - 1].rstrip()
    return Violation(src.path, line, column, rule_id, message, snippet)


def strip_comments_and_strings(lines: list[str]) -> list[str]:
    """Remove comments and string/char contents while preserving columns."""

    result: list[str] = []
    in_block = False
    for line in lines:
        cleaned: list[str] = []
        i = 0
        in_string = False
        in_char = False
        while i < len(line):
            pair = line[i : i + 2]
            char = line[i]
            if in_block:
                if pair == "*/":
                    cleaned.extend("  ")
                    in_block = False
                    i += 2
                else:
                    cleaned.append(" ")
                    i += 1
            elif in_string or in_char:
                quote = '"' if in_string else "'"
                if char == "\\":
                    cleaned.extend("  ")
                    i += 2
                elif char == quote:
                    cleaned.append(" ")
                    in_string = False
                    in_char = False
                    i += 1
                else:
                    cleaned.append(" ")
                    i += 1
            elif pair == "/*":
                cleaned.extend("  ")
                in_block = True
                i += 2
            elif pair == "//":
                cleaned.extend(" " * (len(line) - i))
                break
            elif char == '"':
                cleaned.append(" ")
                in_string = True
                i += 1
            elif char == "'":
                cleaned.append(" ")
                in_char = True
                i += 1
            else:
                cleaned.append(char)
                i += 1
        result.append("".join(cleaned))
    return result


def brace_delta(line: str) -> int:
    return line.count("{") - line.count("}")


def function_name_from_signature(signature: str) -> str | None:
    match = re.search(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\([^;]*\)\s*$", signature)
    if not match:
        return None
    name = match.group(1)
    if name in CONTROL_KEYWORDS:
        return None
    return name


def collect_signature(src: SourceFile, line_index: int) -> tuple[int, str]:
    """Collect a possibly multi-line function signature ending before `{`."""

    parts: list[str] = []
    start = line_index
    depth = 0
    while start >= 0:
        current = src.code_lines[start].strip()
        if current:
            parts.insert(0, current)
            depth += current.count(")") - current.count("(")
            if depth >= 0 and not current.startswith("*"):
                break
        start -= 1
    return start + 1, " ".join(parts)


def collect_open_signature(src: SourceFile, index: int) -> tuple[int, str]:
    """Collect a function signature for a line containing the opening `{`."""

    before_brace = src.code_lines[index].split("{", 1)[0].strip()
    if before_brace:
        return index + 1, before_brace
    return collect_signature(src, index - 1)


def is_function_open(src: SourceFile, index: int, depth: int) -> bool:
    line = src.code_lines[index].strip()
    if depth != 0 or "{" not in line:
        return False
    start, signature = collect_open_signature(src, index)
    if start <= 0 and not signature:
        return False
    if ";" in signature or "=" in signature:
        return False
    return function_name_from_signature(signature) is not None


def find_functions(src: SourceFile) -> list[FunctionInfo]:
    """Find top-level function bodies with a simple brace scanner."""

    functions: list[FunctionInfo] = []
    depth = 0
    index = 0
    while index < len(src.code_lines):
        if is_function_open(src, index, depth):
            start, signature = collect_open_signature(src, index)
            name = function_name_from_signature(signature)
            end = find_matching_close(src.code_lines, index)
            if name and end is not None:
                body = list(enumerate(src.plain_lines[index + 1 : end], index + 2))
                functions.append(FunctionInfo(name, start, index + 1,
                                              end + 1, signature, body))
                index = end + 1
                continue
        depth += brace_delta(src.code_lines[index])
        index += 1
    return functions


def find_matching_close(lines: list[str], open_index: int) -> int | None:
    depth = 0
    for index in range(open_index, len(lines)):
        depth += brace_delta(lines[index])
        if depth == 0:
            return index
    return None


def iter_source_files(paths: Iterable[Path], include_tests: bool) -> list[Path]:
    files: list[Path] = []
    for path in paths:
        if path.is_dir():
            for child in path.rglob("*"):
                add_source_file(files, child, include_tests)
        else:
            add_source_file(files, path, include_tests)
    return sorted(dict.fromkeys(files))


def add_source_file(files: list[Path], path: Path, include_tests: bool) -> None:
    if path.suffix not in {".c", ".h"} and path.name != "Makefile":
        return
    if not include_tests and "tests" in path.parts:
        return
    files.append(path)


def check_final_newline(src: SourceFile) -> list[Violation]:
    if src.text and not src.text.endswith("\n"):
        return [make_violation(src, len(src.plain_lines), 1, "FILE_FINAL_NL",
                               "File must end with a newline.")]
    return []


def check_ascii(src: SourceFile) -> list[Violation]:
    for number, line in enumerate(src.plain_lines, 1):
        for column, char in enumerate(line, 1):
            if ord(char) > 127:
                return [make_violation(src, number, column, "FILE_ASCII",
                                       "Non-ASCII characters are forbidden.")]
    return []


def check_file_name_case(src: SourceFile) -> list[Violation]:
    pattern = re.compile(r"^[a-z0-9_]+(\.[a-z0-9_]+)?$")
    for part in src.path.parts:
        if part in {".", ".."} or part == "Makefile":
            continue
        if not pattern.match(part):
            return [make_violation(src, 1, 1, "FILE_NAME_CASE",
                                   f"`{part}` must use lower snake_case.")]
    return []


def check_42_header(src: SourceFile) -> list[Violation]:
    if src.path.suffix not in {".c", ".h"} and src.path.name != "Makefile":
        return []
    if not src.plain_lines:
        return [make_violation(src, 1, 1, "FILE_42_HEADER",
                               "File must start with the standard 42 header.")]
    if not src.plain_lines[0].startswith("/* ********") \
            and not src.plain_lines[0].startswith("# ********"):
        return [make_violation(src, 1, 1, "FILE_42_HEADER",
                               "File must start with the standard 42 header.")]
    return []


def check_line_length(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.plain_lines, 1):
        if len(line.expandtabs(4)) > MAX_LINE_LENGTH:
            errors.append(make_violation(src, number, MAX_LINE_LENGTH + 1,
                                         "LINE_LEN",
                                         f"Line exceeds {MAX_LINE_LENGTH} columns."))
    return errors


def check_trailing_space(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.plain_lines, 1):
        stripped = line.rstrip("\r\n")
        if stripped.endswith(" ") or stripped.endswith("\t"):
            errors.append(make_violation(src, number, len(stripped),
                                         "LINE_TRAILING_WS",
                                         "Line has trailing whitespace."))
    return errors


def check_consecutive_blank_lines(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    previous_blank = False
    for number, line in enumerate(src.plain_lines, 1):
        blank = line.strip() == ""
        if blank and previous_blank:
            errors.append(make_violation(src, number, 1, "LINE_MULTI_BLANK",
                                         "Only one consecutive blank line is allowed."))
        previous_blank = blank
    return errors


def check_indentation(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.plain_lines, 1):
        if not line or line.lstrip() == "":
            continue
        leading = line[: len(line) - len(line.lstrip(" \t"))]
        if " " in leading:
            errors.append(make_violation(src, number, 1, "INDENT_TABS",
                                         "Indentation must use tabs, not spaces."))
    return errors


def check_function_line_count(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for function in src.functions:
        count = max(0, function.end_line - function.open_line - 1)
        if count > MAX_FUNCTION_LINES:
            errors.append(make_violation(
                src,
                function.start_line,
                1,
                "FUNC_LEN",
                f"Function `{function.name}` has {count} body lines; maximum "
                f"is {MAX_FUNCTION_LINES}.",
            ))
    return errors


def check_function_count(src: SourceFile) -> list[Violation]:
    if src.path.suffix != ".c" or len(src.functions) <= MAX_FUNCTIONS_PER_FILE:
        return []
    return [make_violation(
        src,
        1,
        1,
        "FILE_FUNC_COUNT",
        f"File has {len(src.functions)} functions; maximum is "
        f"{MAX_FUNCTIONS_PER_FILE}.",
    )]


def split_params(signature: str) -> list[str]:
    inside = signature[signature.find("(") + 1 : signature.rfind(")")].strip()
    if not inside or inside == "void":
        return []
    params: list[str] = []
    current: list[str] = []
    depth = 0
    for char in inside:
        if char == "," and depth == 0:
            params.append("".join(current).strip())
            current = []
            continue
        if char in "([{":
            depth += 1
        elif char in ")]}":
            depth -= 1
        current.append(char)
    params.append("".join(current).strip())
    return params


def check_function_params(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for function in src.functions:
        count = len(split_params(function.signature))
        if count > MAX_FUNCTION_PARAMS:
            errors.append(make_violation(
                src,
                function.start_line,
                1,
                "FUNC_PARAM_COUNT",
                f"Function `{function.name}` has {count} parameters; maximum "
                f"is {MAX_FUNCTION_PARAMS}.",
            ))
    return errors


def check_void_no_params(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for function in src.functions:
        if "()" in function.signature:
            errors.append(make_violation(src, function.start_line, 1,
                                         "FUNC_VOID_PARAM",
                                         "No-argument functions must use "
                                         "`void` explicitly."))
    if src.path.suffix != ".h":
        return errors
    for number, line in enumerate(src.code_lines, 1):
        stripped = line.strip()
        if looks_like_prototype(stripped) and "()" in stripped:
            errors.append(make_violation(src, number, 1, "FUNC_VOID_PARAM",
                                         "No-argument prototypes must use "
                                         "`void` explicitly."))
    return errors


def check_prototype_param_names(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    if src.path.suffix != ".h":
        return errors
    for number, line in enumerate(src.code_lines, 1):
        stripped = line.strip()
        if not looks_like_prototype(stripped):
            continue
        for param in split_params(stripped):
            if not parameter_has_name(param):
                errors.append(make_violation(src, number, 1,
                                             "PROTO_PARAM_NAME",
                                             "Function prototypes must include "
                                             "parameter names."))
    return errors


def looks_like_prototype(stripped: str) -> bool:
    return bool(stripped.endswith(";")
                and "(" in stripped
                and ")" in stripped
                and not stripped.startswith(("#", "typedef")))


def parameter_has_name(param: str) -> bool:
    if param == "void":
        return True
    param = re.sub(r"\[[^\]]*\]", "", param).strip()
    return bool(re.search(r"[\*\s]([a-z_][a-z0-9_]*)$", param))


def check_function_separation(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for current, following in zip(src.functions, src.functions[1:]):
        if following.start_line - current.end_line < 2:
            errors.append(make_violation(src, following.start_line, 1,
                                         "FUNC_SEPARATION",
                                         "Function definitions must be "
                                         "separated by one blank line."))
    return errors


def check_function_brace_style(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for function in src.functions:
        line = src.plain_lines[function.open_line - 1].strip()
        if line != "{":
            errors.append(make_violation(src, function.open_line, 1,
                                         "BRACE_FUNC_OPEN",
                                         "Function opening brace must be alone."))
    return errors


def check_control_brace_style(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    pattern = re.compile(r"\b(if|else|while)\b.*\{")
    for number, line in enumerate(src.code_lines, 1):
        if pattern.search(line):
            errors.append(make_violation(src, number, line.find("{") + 1,
                                         "BRACE_CONTROL_OPEN",
                                         "Control block opening brace must be "
                                         "on the next line."))
    return errors


def check_brace_alone(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.code_lines, 1):
        stripped = line.strip()
        if "{" in stripped and stripped != "{":
            if not is_initializer_or_type_block(stripped):
                errors.append(make_violation(src, number, line.find("{") + 1,
                                             "BRACE_ALONE",
                                             "Opening braces must be alone on "
                                             "their line."))
        if "}" in stripped and stripped not in {"}", "};"}:
            if not re.match(r"}\s*t_[a-z0-9_]+\s*;", stripped):
                errors.append(make_violation(src, number, line.find("}") + 1,
                                             "BRACE_ALONE",
                                             "Closing braces must be alone on "
                                             "their line."))
    return errors


def is_initializer_or_type_block(stripped: str) -> bool:
    return bool(stripped.startswith(("typedef struct", "typedef enum",
                                    "typedef union", "struct ", "enum ",
                                    "union "))
                or "=" in stripped)


def check_forbidden_keywords(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.code_lines, 1):
        for keyword, message in FORBIDDEN_KEYWORDS.items():
            match = re.search(rf"\b{keyword}\b", line)
            if match:
                errors.append(make_violation(src, number, match.start() + 1,
                                             "FORBIDDEN_KEYWORD", message))
    return errors


def check_ternary(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.code_lines, 1):
        if "?" in line and ":" in line:
            errors.append(make_violation(src, number, line.find("?") + 1,
                                         "FORBIDDEN_TERNARY",
                                         "The ternary operator is forbidden."))
    return errors


def check_nested_ternary(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.code_lines, 1):
        if line.count("?") > 1:
            errors.append(make_violation(src, number, line.find("?") + 1,
                                         "FORBIDDEN_NESTED_TERNARY",
                                         "Nested ternary operators are forbidden."))
    return errors


def check_multiple_statements(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.code_lines, 1):
        stripped = line.strip()
        if stripped.startswith("#") or stripped.startswith("for"):
            continue
        if line.count(";") > 1:
            errors.append(make_violation(src, number, line.find(";") + 1,
                                         "STMT_ONE_PER_LINE",
                                         "Only one statement is allowed per line."))
    return errors


def is_declaration(line: str) -> bool:
    types = (
        "char", "short", "int", "long", "float", "double", "void", "size_t",
        "ssize_t", "t_", "struct ", "enum ", "unsigned", "signed", "const ",
        "static ",
    )
    stripped = line.strip()
    if not stripped.endswith(";") or "(" in stripped:
        return False
    return stripped.startswith(types) or re.match(r"^[A-Za-z_]\w*\s+\**\w+", stripped)


def check_declarations_at_top(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for function in src.functions:
        seen_statement = False
        for number, line in function.body_lines:
            stripped = src.code_lines[number - 1].strip()
            if not stripped or stripped in {"{", "}"}:
                continue
            if is_declaration(stripped):
                if seen_statement:
                    errors.append(make_violation(
                        src, number, 1, "DECL_AT_TOP",
                        "Variable declarations must be at the beginning of "
                        f"`{function.name}`.",
                    ))
            elif not stripped.startswith("#"):
                seen_statement = True
    return errors


def check_declaration_blank_line(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for function in src.functions:
        declarations = declaration_lines(src, function)
        if not declarations:
            continue
        last_decl = declarations[-1]
        next_line = first_non_empty_after(src, last_decl)
        if next_line and next_line == last_decl + 1:
            errors.append(make_violation(src, next_line, 1,
                                         "DECL_BLANK_LINE",
                                         "Declarations must be separated from "
                                         "implementation by one blank line."))
    return errors


def declaration_lines(src: SourceFile, function: FunctionInfo) -> list[int]:
    lines: list[int] = []
    for number, _line in function.body_lines:
        stripped = src.code_lines[number - 1].strip()
        if not stripped:
            continue
        if is_declaration(stripped):
            lines.append(number)
        else:
            break
    return lines


def first_non_empty_after(src: SourceFile, number: int) -> int | None:
    index = number
    while index < len(src.code_lines):
        if src.code_lines[index].strip():
            return index + 1
        index += 1
    return None


def check_declaration_alignment(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for function in src.functions:
        columns = []
        for number in declaration_lines(src, function):
            line = src.plain_lines[number - 1]
            name_col = declaration_name_column(line)
            if name_col is not None:
                columns.append((number, name_col))
        if columns and len({column for _number, column in columns}) > 1:
            for number, _column in columns:
                errors.append(make_violation(src, number, 1,
                                             "DECL_ALIGNMENT",
                                             "Variable names in declarations "
                                             "must be aligned."))
    return errors


def declaration_name_column(line: str) -> int | None:
    names = declared_names(line.strip())
    if names:
        return line.find(names[-1]) + 1
    return None


def check_declaration_initialization(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for function in src.functions:
        for number, _line in function.body_lines:
            stripped = src.code_lines[number - 1].strip()
            if is_declaration(stripped) and "=" in stripped:
                errors.append(make_violation(src, number, stripped.find("=") + 1,
                                             "DECL_INIT",
                                             "Declaration and initialization "
                                             "must be separate statements."))
    return errors


def check_multiple_assignment(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.code_lines, 1):
        stripped = line.strip()
        if stripped.startswith("#"):
            continue
        if len(re.findall(r"(?<![!<>=])=(?!=)", line)) > 1:
            errors.append(make_violation(src, number, line.find("=") + 1,
                                         "ASSIGN_MULTIPLE",
                                         "Multiple assignment is forbidden."))
    return errors


def check_local_variable_count(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for function in src.functions:
        count = 0
        for number, _line in function.body_lines:
            stripped = src.code_lines[number - 1].strip()
            if is_declaration(stripped):
                count += count_declared_names(stripped)
        if count > MAX_LOCAL_VARIABLES:
            errors.append(make_violation(
                src,
                function.start_line,
                1,
                "FUNC_VAR_COUNT",
                f"Function `{function.name}` declares {count} local variables; "
                f"maximum is {MAX_LOCAL_VARIABLES}.",
            ))
    return errors


def count_declared_names(declaration: str) -> int:
    declaration = declaration.rstrip(";")
    if "," not in declaration:
        return 1
    return len([part for part in declaration.split(",") if part.strip()])


def check_comma_declarations(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.code_lines, 1):
        stripped = line.strip()
        if is_declaration(stripped) and "," in stripped:
            errors.append(make_violation(src, number, stripped.find(",") + 1,
                                         "DECL_ONE_PER_LINE",
                                         "Declare only one variable per line."))
    return errors


def check_global_names(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    depth = 0
    for number, line in enumerate(src.code_lines, 1):
        stripped = line.strip()
        if depth == 0 and is_global_variable_line(stripped):
            name = global_name(stripped)
            if name and not name.startswith("g_"):
                errors.append(make_violation(src, number, 1, "NAME_GLOBAL",
                                             f"Global variable `{name}` must "
                                             "start with `g_`."))
        depth += brace_delta(line)
    return errors


def is_global_variable_line(stripped: str) -> bool:
    if not stripped.endswith(";") or "(" in stripped:
        return False
    if stripped.startswith(("typedef", "struct", "enum", "#")):
        return False
    return bool(re.match(r"^(static\s+)?(const\s+)?[A-Za-z_]\w*", stripped))


def global_name(stripped: str) -> str | None:
    stripped = stripped.rstrip(";").split("=")[0].strip()
    match = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[[^\]]*\])?$", stripped)
    if match:
        return match.group(1)
    return None


def check_function_names(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    pattern = re.compile(r"^[a-z][a-z0-9_]*$")
    for function in src.functions:
        if not pattern.match(function.name):
            errors.append(make_violation(src, function.start_line, 1,
                                         "NAME_FUNCTION",
                                         f"Function `{function.name}` must use "
                                         "lower snake_case."))
    return errors


def check_variable_names(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    pattern = re.compile(r"^[a-z][a-z0-9_]*$")
    for number, line in enumerate(src.code_lines, 1):
        stripped = line.strip()
        if not is_declaration(stripped):
            continue
        for name in declared_names(stripped):
            if not pattern.match(name):
                errors.append(make_violation(src, number, 1,
                                             "NAME_VARIABLE",
                                             f"Variable `{name}` must use lower "
                                             "snake_case."))
    return errors


def declared_names(declaration: str) -> list[str]:
    names: list[str] = []
    declaration = declaration.rstrip(";").split("=", 1)[0]
    for part in declaration.split(","):
        match = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[[^\]]*\])?$",
                          part.strip())
        if match:
            names.append(match.group(1))
    return names


def check_type_names(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    checks = [
        (r"\bstruct\s+([A-Za-z_][A-Za-z0-9_]*)", "TYPE_STRUCT",
         "Struct tags must start with `s_`."),
        (r"\benum\s+([A-Za-z_][A-Za-z0-9_]*)", "TYPE_ENUM",
         "Enum tags must start with `e_`."),
        (r"\bunion\s+([A-Za-z_][A-Za-z0-9_]*)", "TYPE_UNION",
         "Union tags must start with `u_`."),
        (r"}\s*([A-Za-z_][A-Za-z0-9_]*)\s*;", "TYPE_TYPEDEF",
         "Typedef names must start with `t_`."),
    ]
    for number, line in enumerate(src.code_lines, 1):
        for pattern, rule_id, message in checks:
            match = re.search(pattern, line)
            if match and match.lastindex and match.group(match.lastindex):
                name = match.group(match.lastindex)
                if type_name_invalid(rule_id, name):
                    errors.append(make_violation(src, number, match.start() + 1,
                                                 rule_id, message))
    return errors


def type_name_invalid(rule_id: str, name: str) -> bool:
    prefixes = {
        "TYPE_STRUCT": "s_",
        "TYPE_ENUM": "e_",
        "TYPE_UNION": "u_",
        "TYPE_TYPEDEF": "t_",
    }
    return not name.startswith(prefixes[rule_id])


def check_struct_in_c(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    if src.path.suffix != ".c":
        return errors
    for number, line in enumerate(src.code_lines, 1):
        if re.search(r"\b(struct|enum|union)\s+[A-Za-z_][A-Za-z0-9_]*\s*{",
                     line):
            errors.append(make_violation(src, number, 1, "TYPE_IN_C",
                                         "Struct, enum and union declarations "
                                         "are forbidden in .c files."))
    return errors


def check_define_names(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.code_lines, 1):
        match = re.match(r"\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)", line)
        if match and not re.match(r"^[A-Z][A-Z0-9_]*$", match.group(1)):
            errors.append(make_violation(src, number, match.start(1) + 1,
                                         "NAME_DEFINE",
                                         f"Define `{match.group(1)}` must be "
                                         "uppercase snake_case."))
    return errors


def check_include_location(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    seen_code = False
    for number, line in enumerate(src.code_lines, 1):
        stripped = line.strip()
        if not stripped:
            continue
        if stripped.startswith("# include") or stripped.startswith("#include"):
            if seen_code:
                errors.append(make_violation(src, number, 1, "PREPROC_INCLUDE_TOP",
                                             "Includes must appear before code."))
        elif not stripped.startswith("#") and stripped not in {"{", "}"}:
            seen_code = True
    return errors


def check_include_c(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.code_lines, 1):
        if re.search(r"#\s*include\s+[<\"].*\.c[>\"]", line):
            errors.append(make_violation(src, number, 1, "INCLUDE_C_FILE",
                                         "Including .c files is forbidden."))
    return errors


def check_header_content(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    if src.path.suffix != ".h":
        return errors
    depth = 0
    for number, line in enumerate(src.code_lines, 1):
        stripped = line.strip()
        if not stripped or stripped.startswith(("#", "/*", "*")):
            continue
        if depth == 0 and not allowed_header_line(stripped):
            errors.append(make_violation(src, number, 1, "HEADER_CONTENT",
                                         "Headers may only contain includes, "
                                         "defines, types, macros and prototypes."))
        depth += brace_delta(line)
    return errors


def allowed_header_line(stripped: str) -> bool:
    return bool(stripped in {"{", "}", "};"}
                or stripped.startswith(("typedef", "struct", "enum", "union"))
                or looks_like_prototype(stripped))


def check_header_guard(src: SourceFile) -> list[Violation]:
    if src.path.suffix != ".h":
        return []
    stem = re.sub(r"[^A-Za-z0-9]", "_", src.path.name).upper()
    expected = stem
    text = "\n".join(src.code_lines)
    if f"#ifndef {expected}" not in text or f"# define {expected}" not in text:
        return [make_violation(src, 1, 1, "HEADER_GUARD",
                               f"Header guard should use `{expected}`.")]
    return []


def check_tabs_after_type(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.plain_lines, 1):
        if re.match(r"^\w[\w\s\*]* [A-Za-z_]\w*\s*\(", line):
            errors.append(make_violation(src, number, 1, "FORMAT_PROTO_TAB",
                                         "Function return type and name should "
                                         "be separated with a tab."))
    return errors


def check_function_name_alignment(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    columns: list[tuple[int, int]] = []
    for function in src.functions:
        line = src.plain_lines[function.start_line - 1]
        column = line.find(function.name)
        if column >= 0:
            columns.append((function.start_line, column + 1))
    if len({column for _line, column in columns}) > 1:
        for number, _column in columns:
            errors.append(make_violation(src, number, 1,
                                         "FUNC_NAME_ALIGNMENT",
                                         "Function names must be aligned in "
                                         "the same file."))
    return errors


def check_double_space_in_code(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, original in enumerate(src.plain_lines, 1):
        line = remove_string_char_literals(original)
        stripped = line.strip()
        if not stripped or stripped.startswith(("#", "*", "/*", "//")):
            continue
        body = line.lstrip(" \t")
        column_offset = len(line) - len(body)
        match = re.search(r" {2,}", body)
        if match:
            errors.append(make_violation(src, number,
                                         column_offset + match.start() + 1,
                                         "FORMAT_MULTI_SPACE",
                                         "Multiple consecutive spaces are not "
                                         "allowed in code."))
    return errors


def check_punctuation_spacing(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.code_lines, 1):
        for match in re.finditer(r"[,;](?=\S)", line):
            errors.append(make_violation(src, number, match.start() + 1,
                                         "FORMAT_PUNCT_SPACE",
                                         "Comma and semicolon must be followed "
                                         "by a space unless at end of line."))
    return errors


def check_keyword_spacing(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    keywords = ("if", "while", "return", "else")
    for number, line in enumerate(src.code_lines, 1):
        if line.strip().startswith("#"):
            continue
        for keyword in keywords:
            match = re.search(rf"\b{keyword}(?=\S)", line)
            if match:
                errors.append(make_violation(src, number, match.start() + 1,
                                             "FORMAT_KEYWORD_SPACE",
                                             f"`{keyword}` must be followed by "
                                             "a space."))
    return errors


def check_operator_spacing(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    operators = [
        "==", "!=", "<=", ">=", "||", "&&", "<<", ">>", "+=", "-=", "*=",
        "/=", "%=", "+", "-", "*", "/", "%", "<", ">", "|", "&", "^", "=",
    ]
    signature_lines = {function.start_line for function in src.functions}
    for number, line in enumerate(src.code_lines, 1):
        stripped = line.strip()
        if stripped.startswith("#") or is_declaration(stripped) \
                or looks_like_prototype(stripped) or number in signature_lines:
            continue
        for operator in operators:
            errors.extend(operator_errors(src, number, line, operator))
    return errors


def operator_errors(
    src: SourceFile,
    number: int,
    line: str,
    operator: str,
) -> list[Violation]:
    errors: list[Violation] = []
    for match in re.finditer(re.escape(operator), line):
        if skip_operator(line, match.start(), match.group(0)):
            continue
        before = line[match.start() - 1] if match.start() > 0 else ""
        after_i = match.end()
        after = line[after_i] if after_i < len(line) else ""
        if before != " " or after != " ":
            errors.append(make_violation(src, number, match.start() + 1,
                                         "FORMAT_OPERATOR_SPACE",
                                         "Binary operators must be surrounded "
                                         "by exactly one space."))
    return errors


def skip_operator(line: str, index: int, operator: str) -> bool:
    pair = line[index:index + 2]
    before_pair = line[index - 1:index + 1] if index > 0 else ""
    compounds = {"==", "!=", "<=", ">=", "&&", "||", "<<", ">>", "+=", "-=",
                 "*=", "/=", "%="}
    if pair in compounds or before_pair in compounds:
        return True
    if pair == "->" or before_pair == "->":
        return True
    if pair in {"++", "--"} or before_pair in {"++", "--"}:
        return True
    if pair in {"++", "--", "->", "::"}:
        return True
    if operator in {"*", "&"} and re.search(r"[\w\)]\s*$", line[:index]) is None:
        return True
    if operator in {"+", "-"} and re.search(r"[\w\)]\s*$", line[:index]) is None:
        return True
    if operator == "=" and line[index:index + 2] in {"==", ">=", "<=", "!="}:
        return True
    if operator in {"<", ">"} and line[index:index + 2] in {"<<", ">>", "<=", ">="}:
        return True
    return False


def check_pointer_stars(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.code_lines, 1):
        stripped = line.strip()
        if is_declaration(stripped) and re.search(r"\*\s+[a-zA-Z_]", stripped):
            errors.append(make_violation(src, number, line.find("*") + 1,
                                         "DECL_POINTER_STAR",
                                         "Pointer stars must be glued to the "
                                         "variable name."))
    return errors


def check_comments(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    in_function = function_line_set(src)
    for number, line in enumerate(src.plain_lines, 1):
        if "//" in line:
            errors.append(make_violation(src, number, line.find("//") + 1,
                                         "COMMENT_CPP",
                                         "`//` comments are forbidden."))
        if number in in_function and ("/*" in line or "*/" in line):
            errors.append(make_violation(src, number, 1, "COMMENT_IN_FUNC",
                                         "Comments are forbidden inside "
                                         "function bodies."))
    return errors


def function_line_set(src: SourceFile) -> set[int]:
    lines: set[int] = set()
    for function in src.functions:
        lines.update(range(function.open_line + 1, function.end_line))
    return lines


def check_comment_blocks(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    in_block = False
    for number, line in enumerate(src.plain_lines, 1):
        stripped = line.strip()
        if is_single_line_c_comment(stripped):
            continue
        if "/*" in stripped and stripped != "/*":
            if not stripped.startswith("/* ********"):
                errors.append(make_violation(src, number, 1,
                                             "COMMENT_BLOCK_STYLE",
                                             "Block comments must start on "
                                             "their own line."))
            in_block = True
        elif in_block and stripped and not stripped.startswith(("**", "*/", "*")):
            errors.append(make_violation(src, number, 1,
                                         "COMMENT_BLOCK_STYLE",
                                         "Intermediate comment lines must be "
                                         "aligned and start with `**`."))
        if "*/" in stripped:
            in_block = False
    return errors


def is_single_line_c_comment(stripped: str) -> bool:
    return stripped.startswith("/*") and stripped.endswith("*/")


def check_macro_rules(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    for number, line in enumerate(src.plain_lines, 1):
        stripped = line.strip()
        if not stripped.startswith("#"):
            continue
        if re.match(r"#\s*define\s+[A-Z0-9_]+\s*\(", stripped):
            errors.append(make_violation(src, number, 1, "MACRO_CODE",
                                         "Function-like macros are forbidden."))
        if stripped.endswith("\\"):
            errors.append(make_violation(src, number, len(line), "MACRO_MULTILINE",
                                         "Multiline macros are forbidden."))
        if re.match(r"#\s*(if|ifdef|ifndef)", stripped):
            check_preprocessor_indent(src, number, errors)
    return errors


def check_preprocessor_indent(
    src: SourceFile,
    number: int,
    errors: list[Violation],
) -> None:
    index = number
    while index < len(src.plain_lines):
        stripped = src.plain_lines[index].strip()
        if re.match(r"#\s*endif", stripped):
            return
        if stripped.startswith("#") and index + 1 != number:
            if not src.plain_lines[index].startswith("# "):
                errors.append(make_violation(src, index + 1, 1,
                                             "PREPROC_IF_INDENT",
                                             "Preprocessor lines inside #if "
                                             "blocks must be indented."))
        index += 1


def check_compileable(src: SourceFile) -> list[Violation]:
    if src.path.suffix not in {".c", ".h"}:
        return []
    command = ["cc", "-fsyntax-only", "-I", "includes"]
    if src.path.suffix == ".h":
        command.extend(["-x", "c-header"])
    command.append(str(src.path))
    try:
        result = subprocess.run(command, capture_output=True, text=True,
                                timeout=5, check=False)
    except (OSError, subprocess.TimeoutExpired) as exc:
        return [make_violation(src, 1, 1, "FILE_COMPILE",
                               f"Could not verify compilation: {exc}")]
    if result.returncode != 0:
        detail = first_compiler_line(result.stderr)
        return [make_violation(src, 1, 1, "FILE_COMPILE",
                               f"File is not compilable: {detail}")]
    return []


def first_compiler_line(stderr: str) -> str:
    for line in stderr.splitlines():
        if line.strip():
            return line.strip()
    return "compiler returned an error"


def check_makefile_rules(src: SourceFile) -> list[Violation]:
    if src.path.name != "Makefile":
        return []
    errors: list[Violation] = []
    text = "\n".join(src.plain_lines)
    missing = [rule for rule in ("$(NAME)", "clean", "fclean", "re", "all")
               if not re.search(rf"^{re.escape(rule)}\s*:", text, re.MULTILINE)]
    if missing:
        errors.append(make_violation(src, 1, 1, "MAKEFILE_MANDATORY_RULES",
                                     "Missing mandatory rule(s): "
                                     + ", ".join(missing)))
    return errors


def check_makefile_wildcard(src: SourceFile) -> list[Violation]:
    errors: list[Violation] = []
    if src.path.name != "Makefile":
        return errors
    for number, line in enumerate(src.plain_lines, 1):
        if "*.c" in line or "wildcard" in line:
            errors.append(make_violation(src, number, 1, "MAKEFILE_WILDCARD",
                                         "Wildcard source discovery is forbidden."))
    return errors


def remove_string_char_literals(line: str) -> str:
    result: list[str] = []
    i = 0
    in_string = False
    in_char = False
    while i < len(line):
        char = line[i]
        if in_string or in_char:
            quote = '"' if in_string else "'"
            if char == "\\":
                i += 2
            elif char == quote:
                in_string = False
                in_char = False
                result.append(char)
                i += 1
            else:
                i += 1
        elif char == '"':
            in_string = True
            result.append(char)
            i += 1
        elif char == "'":
            in_char = True
            result.append(char)
            i += 1
        else:
            result.append(char)
            i += 1
    return "".join(result)


RULES: list[Rule] = [
    Rule("FILE_ASCII", "ASCII-only source", check_ascii),
    Rule("FILE_NAME_CASE", "File and directory naming", check_file_name_case),
    Rule("FILE_42_HEADER", "Standard 42 header", check_42_header),
    Rule("FILE_FINAL_NL", "File ends with newline", check_final_newline),
    Rule("FILE_COMPILE", "File is syntactically compilable", check_compileable),
    Rule("LINE_LEN", "Maximum line length", check_line_length),
    Rule("LINE_TRAILING_WS", "No trailing whitespace", check_trailing_space),
    Rule("LINE_MULTI_BLANK", "No repeated blank lines", check_consecutive_blank_lines),
    Rule("INDENT_TABS", "Indentation uses tabs", check_indentation),
    Rule("FUNC_LEN", "Maximum function length", check_function_line_count),
    Rule("FILE_FUNC_COUNT", "Maximum functions per file", check_function_count),
    Rule("FUNC_PARAM_COUNT", "Maximum parameters", check_function_params),
    Rule("FUNC_VOID_PARAM", "Explicit void parameter lists", check_void_no_params),
    Rule("PROTO_PARAM_NAME", "Prototype parameter names", check_prototype_param_names),
    Rule("FUNC_SEPARATION", "Function blank-line separation", check_function_separation),
    Rule("FUNC_VAR_COUNT", "Maximum local variables", check_local_variable_count),
    Rule("BRACE_FUNC_OPEN", "Function brace style", check_function_brace_style),
    Rule("BRACE_CONTROL_OPEN", "Control brace style", check_control_brace_style),
    Rule("BRACE_ALONE", "Braces alone on lines", check_brace_alone),
    Rule("FORBIDDEN_KEYWORD", "Forbidden C keywords", check_forbidden_keywords),
    Rule("FORBIDDEN_TERNARY", "Forbidden ternary operator", check_ternary),
    Rule("FORBIDDEN_NESTED_TERNARY", "Forbidden nested ternary", check_nested_ternary),
    Rule("STMT_ONE_PER_LINE", "One statement per line", check_multiple_statements),
    Rule("DECL_AT_TOP", "Declarations at function top", check_declarations_at_top),
    Rule("DECL_BLANK_LINE", "Blank line after declarations", check_declaration_blank_line),
    Rule("DECL_ALIGNMENT", "Declaration name alignment", check_declaration_alignment),
    Rule("DECL_INIT", "No declaration initialization", check_declaration_initialization),
    Rule("DECL_ONE_PER_LINE", "One declaration per line", check_comma_declarations),
    Rule("DECL_POINTER_STAR", "Pointer star placement", check_pointer_stars),
    Rule("ASSIGN_MULTIPLE", "No multiple assignment", check_multiple_assignment),
    Rule("NAME_GLOBAL", "Global variable prefix", check_global_names),
    Rule("NAME_FUNCTION", "Function naming", check_function_names),
    Rule("NAME_VARIABLE", "Variable naming", check_variable_names),
    Rule("TYPE_NAMES", "Type naming", check_type_names),
    Rule("TYPE_IN_C", "No type declarations in .c files", check_struct_in_c),
    Rule("NAME_DEFINE", "Define naming", check_define_names),
    Rule("PREPROC_INCLUDE_TOP", "Includes before code", check_include_location),
    Rule("INCLUDE_C_FILE", "No .c includes", check_include_c),
    Rule("HEADER_CONTENT", "Header content restrictions", check_header_content),
    Rule("HEADER_GUARD", "Header guard naming", check_header_guard),
    Rule("MACRO_RULES", "Macro and preprocessor rules", check_macro_rules),
    Rule("FORMAT_PROTO_TAB", "Prototype/function tab formatting", check_tabs_after_type),
    Rule("FUNC_NAME_ALIGNMENT", "Function name alignment", check_function_name_alignment),
    Rule("FORMAT_MULTI_SPACE", "No repeated spaces in code", check_double_space_in_code),
    Rule("FORMAT_PUNCT_SPACE", "Punctuation spacing", check_punctuation_spacing),
    Rule("FORMAT_KEYWORD_SPACE", "Keyword spacing", check_keyword_spacing),
    Rule("FORMAT_OPERATOR_SPACE", "Operator spacing", check_operator_spacing),
    Rule("COMMENT_RULES", "Comment placement rules", check_comments),
    Rule("COMMENT_BLOCK_STYLE", "Block comment style", check_comment_blocks),
    Rule("MAKEFILE_MANDATORY_RULES", "Mandatory Makefile rules", check_makefile_rules),
    Rule("MAKEFILE_WILDCARD", "No Makefile wildcards", check_makefile_wildcard),
]


def enabled_rules(disabled: set[str]) -> list[Rule]:
    return [rule for rule in RULES if rule.rule_id not in disabled]


def internal_error(path: Path, rule_id: str, message: str) -> Violation:
    return Violation(path, 1, 1, rule_id, message)


def lint_file(path: Path, rules: list[Rule]) -> list[Violation]:
    """Run every possible check on one file.

    A broken rule must not hide errors from later rules. If a rule crashes, the
    crash is reported as a linter issue and the remaining rules keep running.
    """

    errors: list[Violation] = []
    try:
        src = SourceFile(path)
    except OSError as exc:
        return [internal_error(path, "LINTER_FILE_READ",
                               f"Cannot read file: {exc}")]
    except UnicodeError as exc:
        return [internal_error(path, "LINTER_FILE_ENCODING",
                               f"Cannot decode file: {exc}")]
    for rule in rules:
        if path.name == "Makefile" and rule.rule_id not in MAKEFILE_RULES:
            continue
        try:
            errors.extend(rule.check(src))
        except Exception as exc:  # noqa: BLE001 - keep linting other rules.
            errors.append(internal_error(
                path,
                "LINTER_RULE_ERROR",
                f"Rule `{rule.rule_id}` crashed: {exc.__class__.__name__}: "
                f"{exc}",
            ))
    return errors


def print_rules() -> None:
    for rule in RULES:
        print(f"{rule.rule_id:22} {rule.title}")


def sort_errors(errors: list[Violation]) -> list[Violation]:
    return sorted(errors, key=lambda item: (str(item.path), item.line,
                                            item.column, item.rule_id))


def errors_by_file(errors: list[Violation]) -> dict[Path, list[Violation]]:
    grouped: dict[Path, list[Violation]] = {}
    for error in sort_errors(errors):
        grouped.setdefault(error.path, []).append(error)
    return grouped


def use_color(mode: str) -> bool:
    if mode == "always":
        return True
    if mode == "never" or os.environ.get("NO_COLOR") is not None:
        return False
    return sys.stdout.isatty()


def colorize(text: str, color: str, enabled: bool) -> str:
    if not enabled:
        return text
    return f"{COLORS[color]}{text}{COLORS['reset']}"


def print_report(files: list[Path], errors: list[Violation], color: bool) -> None:
    print(colorize("Norm Linter", "bold", color))
    print(f"{colorize('Checked:', 'cyan', color)} {len(files)} file(s)")
    issue_color = "red" if errors else "green"
    print(f"{colorize('Issues :', issue_color, color)} {len(errors)}")
    if not errors:
        print(f"\n{colorize('SUCCESS:', 'green', color)} no norm issues found.")
        return
    for path, file_errors in errors_by_file(errors).items():
        print(f"\n{colorize(str(path), 'blue', color)}")
        for error in file_errors:
            print_error(error, color)
    print(f"\n{colorize('ERROR:', 'red', color)} {len(errors)} norm issue(s) found.")


def print_error(error: Violation, color: bool) -> None:
    location = colorize(f"{error.line}:{error.column}", "yellow", color)
    rule = colorize(error.rule_id, "magenta", color)
    print(f"  {location}  {rule}")
    print(f"      {error.message}")
    if error.snippet:
        print_snippet(error, color)


def print_snippet(error: Violation, color: bool) -> None:
    line_no = str(error.line)
    code = error.snippet.expandtabs(4)
    caret = caret_for_column(error.snippet, error.column)
    line_no = colorize(line_no, "dim", color)
    pipe = colorize("|", "dim", color)
    caret = colorize(caret, "red", color)
    print(f"      {line_no} {pipe} {code}")
    print(f"      {' ' * len(str(error.line))} {pipe} {caret}")


def caret_for_column(snippet: str, column: int) -> str:
    if column <= 1:
        return "^"
    visual_column = len(snippet[:column - 1].expandtabs(4))
    return (" " * visual_column) + "^"


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Modular 42 Norm linter for C source/header files.",
    )
    parser.add_argument("paths", nargs="*", default=["srcs", "includes", "Makefile"],
                        help="Files or directories to lint.")
    parser.add_argument("--include-tests", action="store_true",
                        help="Also lint files below tests/.")
    parser.add_argument("--disable", action="append", default=[],
                        help="Disable one rule id. Can be passed multiple times.")
    parser.add_argument("--list-rules", action="store_true",
                        help="Print available rules and exit.")
    parser.add_argument("--color", choices=("auto", "always", "never"),
                        default="auto",
                        help="Color output. Defaults to auto.")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    if args.list_rules:
        print_rules()
        return 0
    disabled = set(args.disable)
    rules = enabled_rules(disabled)
    paths = [Path(path) for path in args.paths]
    files = iter_source_files(paths, args.include_tests)
    errors: list[Violation] = []
    for path in files:
        errors.extend(lint_file(path, rules))
    print_report(files, errors, use_color(args.color))
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
