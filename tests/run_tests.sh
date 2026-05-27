#!/bin/sh

set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TEST_BIN="$ROOT_DIR/tests/test_malloc"
LIB="$ROOT_DIR/libft_malloc.so"

cd "$ROOT_DIR"

make

cc -Wall -Wextra -Werror -std=c11 tests/test_malloc.c -o "$TEST_BIN"

printf '%s\n' "== libc reference run =="
"$TEST_BIN"

printf '%s\n' "== exported allocator symbols =="
missing=0
for symbol in ft_malloc ft_free ft_realloc
do
	if ! nm -g "$LIB" | grep -E "[[:space:]]_?$symbol$" >/dev/null 2>&1
	then
		printf 'missing symbol: %s\n' "$symbol"
		missing=1
	fi
done
if [ "$missing" -ne 0 ]
then
	printf '%s\n' "libft_malloc.so cannot replace the stdlib allocator yet."
	exit 1
fi

printf '%s\n' "== libft_malloc preload run =="
case "$(uname -s)" in
	Darwin)
		DYLD_LIBRARY_PATH="$ROOT_DIR" \
		DYLD_INSERT_LIBRARIES="$LIB" \
		"$TEST_BIN"
		;;
	*)
		LD_PRELOAD="$LIB" "$TEST_BIN"
		;;
esac
