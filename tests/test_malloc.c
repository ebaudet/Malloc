#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_FT_ALLOC
# include "malloc.h"
# define TEST_MALLOC ft_malloc
# define TEST_FREE ft_free
# define TEST_REALLOC ft_realloc
# define TEST_NAME "ft_malloc/ft_free/ft_realloc"
#else
# define TEST_MALLOC malloc
# define TEST_FREE free
# define TEST_REALLOC realloc
# define TEST_NAME "malloc/free/realloc"
#endif

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

static int	g_failures;
static int	g_checks;

static void	check(int condition, const char *message)
{
	++g_checks;
	if (!condition)
	{
		fprintf(stderr, "FAIL: %s\n", message);
		++g_failures;
	}
}

static void	test_basic_allocations(void)
{
	char	*str;
	void	*zero;
	size_t	i;

	str = TEST_MALLOC(32);
	check(str != NULL, TEST_NAME " allocates 32 bytes");
	if (str != NULL)
	{
		strcpy(str, "hello malloc");
		check(strcmp(str, "hello malloc") == 0, "allocated memory is writable");
		TEST_FREE(str);
	}
	zero = TEST_MALLOC(0);
	TEST_FREE(zero);
	i = 0;
	while (i < 128)
	{
		void *ptr = TEST_MALLOC(i + 1);
		check(ptr != NULL, TEST_NAME " returns a pointer for small allocation");
		if (ptr != NULL)
			memset(ptr, (int)i, i + 1);
		TEST_FREE(ptr);
		++i;
	}
	TEST_FREE(NULL);
}

static void	test_alignment(void)
{
	size_t	sizes[] = {1, 2, 3, 7, 8, 15, 16, 31, 32, 63, 64, 127, 128, 255};
	size_t	i;

	i = 0;
	while (i < ARRAY_LEN(sizes))
	{
		void		*ptr;
		uintptr_t	addr;

		ptr = TEST_MALLOC(sizes[i]);
		check(ptr != NULL, TEST_NAME " returns non-null for alignment test");
		addr = (uintptr_t)ptr;
		check(addr % sizeof(max_align_t) == 0,
			TEST_NAME " pointer is max_align_t aligned");
		TEST_FREE(ptr);
		++i;
	}
}

static void	test_many_allocations_keep_contents(void)
{
	enum { COUNT = 512 };
	void	*ptrs[COUNT];
	size_t	sizes[COUNT];
	size_t	i;

	i = 0;
	while (i < COUNT)
	{
		sizes[i] = ((i * 37) % 4096) + 1;
		ptrs[i] = TEST_MALLOC(sizes[i]);
		check(ptrs[i] != NULL, TEST_NAME " returns a pointer for mixed allocation");
		if (ptrs[i] != NULL)
			memset(ptrs[i], (int)(i & 0xff), sizes[i]);
		++i;
	}
	i = 0;
	while (i < COUNT)
	{
		if (ptrs[i] != NULL)
		{
			unsigned char	*bytes;
			size_t			j;

			bytes = (unsigned char *)ptrs[i];
			j = 0;
			while (j < sizes[i])
			{
				if (bytes[j] != (unsigned char)(i & 0xff))
				{
					check(0, "allocation contents were preserved");
					break ;
				}
				++j;
			}
		}
		++i;
	}
	i = COUNT;
	while (i > 0)
	{
		--i;
		TEST_FREE(ptrs[i]);
	}
}

static void	test_free_reuse_pattern(void)
{
	enum { COUNT = 96 };
	void	*ptrs[COUNT];
	size_t	i;

	i = 0;
	while (i < COUNT)
	{
		ptrs[i] = TEST_MALLOC(24 + (i % 17));
		check(ptrs[i] != NULL, TEST_NAME " allocates reusable small blocks");
		if (ptrs[i] != NULL)
			memset(ptrs[i], 0xa5, 24 + (i % 17));
		++i;
	}
	i = 0;
	while (i < COUNT)
	{
		if (i % 2 == 0)
			TEST_FREE(ptrs[i]);
		++i;
	}
	i = 0;
	while (i < COUNT / 2)
	{
		void *ptr = TEST_MALLOC(32);
		check(ptr != NULL, TEST_NAME " allocates after freeing alternating blocks");
		TEST_FREE(ptr);
		++i;
	}
	i = 0;
	while (i < COUNT)
	{
		if (i % 2 != 0)
			TEST_FREE(ptrs[i]);
		++i;
	}
}

static void	test_realloc_behavior(void)
{
	char	*ptr;
	char	*grown;
	char	*shrunk;
	char	*from_null;
	size_t	i;

	ptr = TEST_MALLOC(16);
	check(ptr != NULL, TEST_NAME " allocates before realloc");
	if (ptr == NULL)
		return ;
	strcpy(ptr, "abcdefghijkl");
	grown = TEST_REALLOC(ptr, 4096);
	check(grown != NULL, TEST_NAME " can grow an allocation");
	if (grown == NULL)
		return ;
	check(memcmp(grown, "abcdefghijkl", 12) == 0, "realloc growth preserves contents");
	i = 12;
	while (i < 4096)
		grown[i++] = 'x';
	shrunk = TEST_REALLOC(grown, 8);
	check(shrunk != NULL, TEST_NAME " can shrink an allocation");
	if (shrunk != NULL)
	{
		check(memcmp(shrunk, "abcdefgh", 8) == 0, "realloc shrink preserves prefix");
		TEST_FREE(shrunk);
	}
	from_null = TEST_REALLOC(NULL, 64);
	check(from_null != NULL, TEST_NAME " realloc(NULL, size) behaves like malloc");
	TEST_FREE(from_null);
}

#ifndef USE_FT_ALLOC
static void	test_calloc_behavior(void)
{
	size_t	*values;
	size_t	i;
	void	*overflow;

	values = calloc(128, sizeof(*values));
	check(values != NULL, "calloc returns a pointer");
	if (values != NULL)
	{
		i = 0;
		while (i < 128)
		{
			check(values[i] == 0, "calloc zero-initializes memory");
			values[i] = i + 1;
			++i;
		}
		free(values);
	}
	overflow = calloc(((size_t)-1 / 2) + 1, 2);
	check(overflow == NULL, "calloc detects multiplication overflow");
	free(overflow);
}
#endif

static void	test_large_allocation(void)
{
	size_t	size;
	char	*ptr;

	size = 1024 * 1024;
	ptr = TEST_MALLOC(size);
	check(ptr != NULL, TEST_NAME " returns a pointer for large allocation");
	if (ptr != NULL)
	{
		ptr[0] = 'a';
		ptr[size / 2] = 'b';
		ptr[size - 1] = 'c';
		check(ptr[0] == 'a' && ptr[size / 2] == 'b' && ptr[size - 1] == 'c',
			"large allocation is writable at beginning, middle, and end");
		TEST_FREE(ptr);
	}
}

int	main(void)
{
	test_basic_allocations();
	test_alignment();
	test_many_allocations_keep_contents();
	test_free_reuse_pattern();
	test_realloc_behavior();
#ifndef USE_FT_ALLOC
	test_calloc_behavior();
#endif
	test_large_allocation();
	if (g_failures != 0)
	{
		fprintf(stderr, "ERROR: %s: %d/%d checks passed, %d failed\n",
			TEST_NAME, g_checks - g_failures, g_checks, g_failures);
		return (1);
	}
	printf("SUCCESS: %s: %d/%d checks passed\n", TEST_NAME, g_checks,
		g_checks);
	return (0);
}
