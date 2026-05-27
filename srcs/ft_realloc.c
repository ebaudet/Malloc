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

void	*ft_realloc(void *ptr, size_t size)
{
	t_alloc	*block;
	void	*new_ptr;
	size_t	copy_size;

	if (!ptr)
		return (ft_malloc(size));
	if (size == 0)
	{
		ft_free(ptr);
		return (NULL);
	}
	malloc_lock();
	block = malloc_find_alloc(ptr);
	if (!block || block->free)
	{
		malloc_unlock();
		return (NULL);
	}
	if (size <= block->capacity)
	{
		block->size = size;
		malloc_unlock();
		return (ptr);
	}
	new_ptr = malloc_alloc(size);
	if (!new_ptr)
	{
		malloc_unlock();
		return (NULL);
	}
	copy_size = block->size;
	if (copy_size > size)
		copy_size = size;
	malloc_copy(new_ptr, ptr, copy_size);
	malloc_free_unlocked(ptr);
	malloc_unlock();
	return (new_ptr);
}

void	*realloc(void *ptr, size_t size)
{
	return (ft_realloc(ptr, size));
}
