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
		ptr = mmap(0, TINY_BLOCK * PAGE, PROT_WRITE | PROT_READ,
				MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
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
	ptr->total = size;
	ptr->data = (char *)ptr + sizeof(t_block);
	return (ptr);
}

static t_block	*add_block(t_block *ptr, size_t type, size_t size)
{
	t_block		*current;
	t_block		*block;

	block = new_block(NULL, type, size);
	if (!block)
		return (NULL);
	current = ptr;
	while (current->next)
		current = current->next;
	current->next = block;
	return (block);
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
	t_block		*block;

	block = add_block(ptr, SMALL, size + sizeof(t_block));
	if (!block)
		return (NULL);
	block->size[0] = size;
	return (block->data);
}

static void		*malloc_large(t_block *ptr, size_t size)
{
	t_block		*block;

	block = add_block(ptr, LARGE, size + sizeof(t_block));
	if (!block)
		return (NULL);
	block->size[0] = size;
	return (block->data);
}

void			*ft_malloc(size_t size)
{
	static t_block		*ptr = NULL;
	struct rlimit		rlp;

	if (!ptr)
	{
		ptr = get_malloc();
		if (ptr == MAP_FAILED)
			return (NULL);
		ft_bzero(ptr, TINY_BLOCK * PAGE);
		ptr->type = TINY;
		ptr->total = TINY_BLOCK * PAGE;
		ptr->data = (char *)ptr + sizeof(t_block);
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
