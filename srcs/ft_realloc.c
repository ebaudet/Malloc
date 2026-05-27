/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_realloc.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 19:20:00 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 20:30:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

static size_t	realloc_copy_size(t_alloc *block, size_t size)
{
	if (block->size < size)
		return (block->size);
	return (size);
}

static void		*realloc_locked(void *ptr, size_t size)
{
	t_alloc		*block;
	void		*new_ptr;

	block = malloc_find_alloc(ptr);
	if (!block || block->free)
		return (NULL);
	if (size <= block->capacity)
	{
		block->size = size;
		return (ptr);
	}
	new_ptr = malloc_alloc(size);
	if (!new_ptr)
		return (NULL);
	malloc_copy(new_ptr, ptr, realloc_copy_size(block, size));
	malloc_free_unlocked(ptr);
	return (new_ptr);
}

void		*ft_realloc(void *ptr, size_t size)
{
	void	*new_ptr;

	if (!ptr)
		return (ft_malloc(size));
	if (size == 0)
	{
		ft_free(ptr);
		return (NULL);
	}
	malloc_lock();
	new_ptr = realloc_locked(ptr, size);
	malloc_unlock();
	return (new_ptr);
}

void		*realloc(void *ptr, size_t size)
{
	return (ft_realloc(ptr, size));
}
