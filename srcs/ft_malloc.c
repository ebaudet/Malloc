/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_malloc.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2014/04/16 19:00:31 by ebaudet           #+#    #+#             */
/*   Updated: 2014/04/16 22:55:09 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif

void			ft_bzero(void *s, size_t len)
{
	size_t			i;
	unsigned char	*ptr;

	ptr = (unsigned char *)s;
	i = 0;
	while (i < len)
	{
		ptr[i] = 0;
		++i;
	}
}

t_block			*get_malloc(void)
{
	static t_block		*ptr = NULL;


	if (!ptr)
		ptr = mmap(0, PAGE, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	return (ptr);
}

void			*new_block(t_block *ptr, size_t type, size_t size)
{
	(void)ptr;
	ptr = (t_block *)mmap(0, size, PROT_WRITE | PROT_READ,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ptr == MAP_FAILED)
		return (NULL);
	ft_bzero(ptr, size);
	ptr->type = type;
	return (ptr);
}

static int		index_val(t_block *ptr)
{
	size_t		i;

	i = 0;
	while (i < MAX_ALLOC)
	{
		if (ptr->size[i] == 0)
			return (i);
		++i;
	}
	return (i);
}

static void		*malloc_tiny(t_block *ptr, size_t size)
{
	t_block		*current;
	size_t		i;

	current = ptr;
	while (current->type != TINY || current->index == MAX_ALLOC)
	{
		if (!current->next)
			current->next = new_block(current->next, TINY, TINY_BLOCK * PAGE);
		if (!current->next)
			return (NULL);
		current = current->next;
	}
	i = 0;
	while (current->size[i])
		++i;
	current->size[i] = size;
	current->index = index_val(current);
	return ((void *)current + sizeof(t_block) + (i * SIZE_N));
}

static void		*malloc_small(t_block *ptr, size_t size)
{
	(void)ptr;
	return (new_block(NULL, SMALL, size + sizeof(t_block)));
}

static void		*malloc_large(t_block *ptr, size_t size)
{
	(void)ptr;
	return (new_block(NULL, LARGE, size + sizeof(t_block)));
}

void			*ft_malloc(size_t size)
{
	static t_block		*ptr = NULL;
	struct rlimit		rlp;

	if (!ptr)
	{
		ptr = get_malloc();
		ft_bzero(ptr, PAGE);
		ptr->type = TINY;
	}
	if (size <= 0)
		return (NULL);
	getrlimit(RLIMIT_MEMLOCK, &rlp);
	if (ptr->total + size > rlp.rlim_cur)
		return (NULL);
	if (size < SIZE_N)
		return (malloc_tiny(ptr, size));
	else if (size < SIZE_M)
		return (malloc_small(ptr, size));
	else
		return (malloc_large(ptr, size));
	return (NULL);

}


void	show_alloc_mem() {

}
