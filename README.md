# Malloc

Custom memory allocator project for the 42 curriculum.

The goal of this project is to implement a shared library that provides a
malloc-family allocator and can be loaded instead of the system allocator for
testing and debugging allocation behavior.

> Understand how malloc does work [there](https://cmmon.medium.com/chow-does-a-malloc-work-45ba5a6fbf32).

## Project Status

This repository is currently a work in progress.

Current state:

- `make` builds `libft_malloc_$(HOSTTYPE).so` and creates the
  `libft_malloc.so` symlink.
- The library exports the mandatory `malloc`, `free`, `realloc`, and
  `show_alloc_mem` symbols.
- `ft_malloc`, `ft_free`, `ft_realloc`, `ft_calloc`, and `show_alloc_mem` are
  also available for direct tests.
- TINY and SMALL allocations are grouped in page-aligned zones with at least 100
  allocation slots per zone.
- LARGE allocations are mapped independently.
- `make test` runs allocator behavior tests against libc, direct `ft_*` calls,
  and standard-symbol preload/insertion.

Current build command:

```sh
make
```

Current test command:

```sh
make test
```

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
├── author
├── auteur
├── includes
│   └── malloc.h
├── srcs
│   ├── ft_free.c
│   ├── ft_calloc.c
│   ├── ft_malloc.c
│   ├── ft_realloc.c
│   └── show_alloc_mem.c
└── tests
    ├── run_tests.sh
    └── test_malloc.c
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
$(shell uname -m)_$(shell uname -s)
```

Build command:

```sh
make
```

Cleanup commands:

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
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void show_alloc_mem();

void *ft_malloc(size_t size);
void ft_free(void *ptr);
void *ft_realloc(void *ptr, size_t size);
void *ft_calloc(size_t count, size_t size);
```

`show_alloc_mem` prints active TINY, SMALL, and LARGE allocation zones, each
allocated range, and the total number of allocated bytes.

Example format:

```text
TINY : 0xA0000
0xA0020 - 0xA004A : 42 bytes
SMALL : 0xAD000
0xAD020 - 0xADEAD : 3725 bytes
LARGE : 0xB0000
0xB0020 - 0xBBEEF : 48847 bytes
Total : 52614 bytes
```

`ft_calloc` is an extra helper API and is not required by the mandatory subject.

## Tests

Automated tests are available in the `tests/` directory.

Run them with:

```sh
make test
```

The test runner:

- Builds `libft_malloc.so`.
- Compiles `tests/test_malloc.c` into a libc reference binary.
- Runs the reference binary with `malloc`, `free`, `realloc`, and `calloc` from
  `stdlib.h`.
- Checks that `libft_malloc.so` exports `malloc`, `free`, `realloc`,
  `show_alloc_mem`, and the direct `ft_*` test helpers.
- Runs the libc reference binary again with the custom allocator preloaded.
- Compiles the same tests again with `USE_FT_ALLOC`, so the allocation calls use
  `ft_malloc`, `ft_free`, `ft_realloc`, and `ft_calloc`.
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
- `show_alloc_mem` output for TINY, SMALL, LARGE, allocation sizes, and total.
- Standard-symbol preload/insertion.
- Invalid pointer free and double-free smoke cases.
- 100+ TINY and 100+ SMALL allocation capacity.

Current expected result:

```sh
make test
```

Expected output:

```text
== libc reference run ==
SUCCESS: malloc/free/realloc/calloc: 952/952 checks passed
SUCCESS: libc reference run

== exported allocator symbols ==
found symbol: malloc
found symbol: free
found symbol: realloc
found symbol: ft_malloc
found symbol: ft_free
found symbol: ft_realloc
found symbol: ft_calloc
found symbol: show_alloc_mem
SUCCESS: exported allocator symbols

== standard malloc/free/realloc preload run ==
SUCCESS: malloc/free/realloc/calloc: 952/952 checks passed
SUCCESS: standard malloc/free/realloc preload run

== ft allocator and show_alloc_mem comparison run ==
SUCCESS: ft_malloc/ft_free/ft_realloc/ft_calloc: 1220/1220 checks passed
SUCCESS: ft allocator and show_alloc_mem comparison run

SUCCESS: all test suites passed
```

The generated test binaries are ignored by Git through `.gitignore`.

## Implementation Checklist

Remaining work:

- Run `norminette` and fix any Norm violations.
- Continue hardening fragmentation, invalid pointer, and multithreaded cases.
- Add optional bonus features only after the mandatory behavior is stable.

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
