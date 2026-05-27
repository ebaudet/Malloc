#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

static int	g_failures;

static void	check(int condition, const char *message)
{
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

	str = malloc(32);
	check(str != NULL, "malloc(32) returns a pointer");
	if (str != NULL)
	{
		strcpy(str, "hello malloc");
		check(strcmp(str, "hello malloc") == 0, "allocated memory is writable");
		free(str);
	}
	zero = malloc(0);
	free(zero);
	i = 0;
	while (i < 128)
	{
		void *ptr = malloc(i + 1);
		check(ptr != NULL, "small allocation returns a pointer");
		if (ptr != NULL)
			memset(ptr, (int)i, i + 1);
		free(ptr);
		++i;
	}
	free(NULL);
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

		ptr = malloc(sizes[i]);
		check(ptr != NULL, "malloc returns non-null for alignment test");
		addr = (uintptr_t)ptr;
		check(addr % sizeof(max_align_t) == 0, "malloc pointer is max_align_t aligned");
		free(ptr);
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
		ptrs[i] = malloc(sizes[i]);
		check(ptrs[i] != NULL, "mixed allocation returns a pointer");
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
		free(ptrs[i]);
	}
}

static void	test_realloc_behavior(void)
{
	char	*ptr;
	char	*grown;
	char	*shrunk;
	char	*from_null;
	size_t	i;

	ptr = malloc(16);
	check(ptr != NULL, "malloc before realloc returns a pointer");
	if (ptr == NULL)
		return ;
	strcpy(ptr, "abcdefghijkl");
	grown = realloc(ptr, 4096);
	check(grown != NULL, "realloc can grow an allocation");
	if (grown == NULL)
		return ;
	check(memcmp(grown, "abcdefghijkl", 12) == 0, "realloc growth preserves contents");
	i = 12;
	while (i < 4096)
		grown[i++] = 'x';
	shrunk = realloc(grown, 8);
	check(shrunk != NULL, "realloc can shrink an allocation");
	if (shrunk != NULL)
	{
		check(memcmp(shrunk, "abcdefgh", 8) == 0, "realloc shrink preserves prefix");
		free(shrunk);
	}
	from_null = realloc(NULL, 64);
	check(from_null != NULL, "realloc(NULL, size) behaves like malloc");
	free(from_null);
}

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

static void	test_large_allocation(void)
{
	size_t	size;
	char	*ptr;

	size = 1024 * 1024;
	ptr = malloc(size);
	check(ptr != NULL, "large allocation returns a pointer");
	if (ptr != NULL)
	{
		ptr[0] = 'a';
		ptr[size / 2] = 'b';
		ptr[size - 1] = 'c';
		check(ptr[0] == 'a' && ptr[size / 2] == 'b' && ptr[size - 1] == 'c',
			"large allocation is writable at beginning, middle, and end");
		free(ptr);
	}
}

int	main(void)
{
	test_basic_allocations();
	test_alignment();
	test_many_allocations_keep_contents();
	test_realloc_behavior();
	test_calloc_behavior();
	test_large_allocation();
	if (g_failures != 0)
	{
		fprintf(stderr, "%d malloc test(s) failed\n", g_failures);
		return (1);
	}
	printf("malloc behavior tests passed\n");
	return (0);
}
