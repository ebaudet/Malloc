# Malloc

Custom memory allocator project for the 42 curriculum.

The goal of this project is to implement a shared library that provides a
malloc-family allocator and can be loaded instead of the system allocator for
testing and debugging allocation behavior.

## Project Status

This repository is currently a work in progress.

At the moment it contains:

- `srcs/ft_malloc.c`: initial allocator implementation.
- `includes/malloc.h`: allocator constants and block metadata structure.
- `Makefile`: library naming variables, but no build targets yet.
- `auteur`: 42 author file.

The project does not currently compile with `make` because the `Makefile` has
no targets:

```sh
make
```

Current result:

```text
make: *** No targets. Stop.
```

The C implementation is also incomplete: several allocator paths are empty,
some symbols are missing or inconsistent, and the public malloc API is not fully
implemented yet.

## Purpose

The intended allocator should manage memory allocations without directly relying
on the standard `malloc` implementation.

A typical 42 malloc project provides replacements for:

- `malloc`
- `free`
- `realloc`
- optionally `calloc`
- `show_alloc_mem`

Memory is usually requested from the operating system with `mmap(2)`, split into
allocation zones, and tracked with custom metadata.

Common allocation classes are:

- **TINY**: small allocations grouped in pages.
- **SMALL**: medium allocations grouped in larger zones.
- **LARGE**: large allocations mapped independently.

## Repository Layout

```text
.
├── Makefile
├── auteur
├── includes
│   └── malloc.h
└── srcs
    └── ft_malloc.c
```

## Build

The intended build output is defined in the `Makefile` as:

```text
libft_malloc_$(HOSTTYPE).so
```

The Makefile also defines the convenience symlink:

```text
libft_malloc.so
```

`HOSTTYPE` is taken from the environment when available. If it is unset, the
Makefile falls back to:

```sh
uname -m
```

Expected build command once Makefile rules are implemented:

```sh
make
```

Expected cleanup commands once implemented:

```sh
make clean
make fclean
make re
```

## Run

Once the shared library exports the standard allocator symbols, it can be tested
by preloading it into another program.

On Linux:

```sh
LD_PRELOAD=./libft_malloc.so ./your_program
```

On macOS:

```sh
DYLD_LIBRARY_PATH=. DYLD_INSERT_LIBRARIES=./libft_malloc.so ./your_program
```

For a small local test program:

```c
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *ptr = malloc(32);
    if (!ptr)
        return 1;
    strcpy(ptr, "hello malloc");
    free(ptr);
    return 0;
}
```

Compile it with:

```sh
cc test.c -o test_malloc
```

Then run it with the allocator preloaded as shown above.

## Current API Notes

The current source contains:

```c
void *ft_malloc(size_t size);
void ft_free(void *ptr);
void *ft_realloc(void *ptr, size_t size);
void show_alloc_mem(void);
```

`show_alloc_mem` is declared in the source but has no implementation yet.

To behave as a drop-in allocator, the project still needs exported functions
with the standard names:

```c
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
```

## Tests

Automated tests are available in the `tests/` directory.

Run them with:

```sh
make test
```

The test runner:

- Builds `libft_malloc.so`.
- Compiles `tests/test_malloc.c` into a libc reference binary.
- Runs the reference binary with `malloc`, `free`, and `realloc` from
  `stdlib.h`.
- Checks that `libft_malloc.so` exports `ft_malloc`, `ft_free`, and
  `ft_realloc`.
- Compiles the same tests again with `USE_FT_ALLOC`, so the allocation calls use
  `ft_malloc`, `ft_free`, and `ft_realloc`.
- Runs the project allocator binary and compares the same allocation behavior.

The current tests cover:

- Basic allocation, write, read, and free behavior.
- `malloc(0)`.
- `free(NULL)`.
- Pointer alignment.
- Many mixed-size allocations kept alive at the same time.
- Content preservation across many allocations.
- Freeing alternating blocks and allocating again afterward.
- `realloc` growth.
- `realloc` shrink.
- `realloc(NULL, size)`.
- `calloc` zero initialization.
- `calloc` multiplication overflow detection.
- Large allocation writes at the beginning, middle, and end of the block.

The `calloc` cases currently run only against libc because `ft_calloc` is not
implemented yet.

Current expected result:

```sh
make test
```

Expected output:

```text
== libc reference run ==
SUCCESS: malloc/free/realloc: 952/952 checks passed
SUCCESS: libc reference run

== exported allocator symbols ==
found symbol: ft_malloc
found symbol: ft_free
found symbol: ft_realloc
SUCCESS: exported allocator symbols

== ft_malloc/ft_free/ft_realloc comparison run ==
SUCCESS: ft_malloc/ft_free/ft_realloc: 822/822 checks passed
SUCCESS: ft_malloc/ft_free/ft_realloc comparison run

SUCCESS: all test suites passed
```

The generated test binaries are ignored by Git through `.gitignore`.

## Implementation Checklist

Before the project can be considered usable, the following items should be
completed:

- Add full Makefile rules for `all`, `clean`, `fclean`, and `re`.
- Compile the shared object with `-shared` and position-independent code.
- Fix header includes and type declarations in `includes/malloc.h`.
- Define missing allocation type constants such as `TINY`, `SMALL`, and
  `LARGE`.
- Resolve duplicate and inconsistent macros.
- Implement tiny, small, and large allocation paths.
- Export standard `malloc`, `free`, and `realloc` symbols.
- Implement `calloc` or `ft_calloc` if required.
- Implement `show_alloc_mem`.
- Expand tests as new allocator APIs are added.

## Useful Commands

Inspect tracked files:

```sh
git status
```

List project files:

```sh
find . -maxdepth 2 -type f
```

Check the Makefile:

```sh
make
```

Compile a source file manually while developing:

```sh
cc -Wall -Wextra -Werror -I includes -c srcs/ft_malloc.c
```

## Author

```text
ebaudet
```
