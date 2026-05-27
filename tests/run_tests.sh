#!/bin/sh

set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
LIBC_TEST_BIN="$ROOT_DIR/tests/test_malloc_libc"
FT_TEST_BIN="$ROOT_DIR/tests/test_malloc_ft"
LIB="$ROOT_DIR/libft_malloc.so"

cd "$ROOT_DIR"

run_step()
{
	name=$1
	shift
	printf '\n%s\n' "== $name =="
	if "$@"
	then
		printf '%s\n' "SUCCESS: $name"
	else
		status=$?
		printf '%s\n' "ERROR: $name failed with exit code $status"
		exit "$status"
	fi
}

make

cc -Wall -Wextra -Werror -std=c11 tests/test_malloc.c -o "$LIBC_TEST_BIN"

run_step "libc reference run" "$LIBC_TEST_BIN"

printf '\n%s\n' "== exported allocator symbols =="
missing=0
for symbol in malloc free realloc ft_malloc ft_free ft_realloc ft_calloc show_alloc_mem
do
	if ! nm -g "$LIB" | grep -E "[[:space:]]_?$symbol$" >/dev/null 2>&1
	then
		printf 'missing symbol: %s\n' "$symbol"
		missing=1
	else
		printf 'found symbol: %s\n' "$symbol"
	fi
done
if [ "$missing" -ne 0 ]
then
	printf '%s\n' "ERROR: libft_malloc.so cannot run the ft allocator tests yet."
	exit 1
fi
printf '%s\n' "SUCCESS: exported allocator symbols"

cc -Wall -Wextra -Werror -std=c11 -DUSE_FT_ALLOC -I includes \
	tests/test_malloc.c -L. -lft_malloc -Wl,-rpath,. -o "$FT_TEST_BIN"

case "$(uname -s)" in
	Darwin)
		run_step "standard malloc/free/realloc preload run" env \
			DYLD_LIBRARY_PATH="$ROOT_DIR" \
			DYLD_INSERT_LIBRARIES="$LIB" "$LIBC_TEST_BIN"
		;;
	*)
		run_step "standard malloc/free/realloc preload run" env \
			LD_PRELOAD="$LIB" "$LIBC_TEST_BIN"
		;;
esac

case "$(uname -s)" in
	Darwin)
		run_step "ft allocator and show_alloc_mem comparison run" env \
			DYLD_LIBRARY_PATH="$ROOT_DIR" "$FT_TEST_BIN"
		;;
	*)
		run_step "ft allocator and show_alloc_mem comparison run" env \
			LD_LIBRARY_PATH="$ROOT_DIR" "$FT_TEST_BIN"
		;;
esac

printf '\n%s\n' "SUCCESS: all test suites passed"
