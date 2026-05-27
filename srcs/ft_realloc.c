/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_realloc.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 19:20:00 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 19:20:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <sys/mman.h>

static void		copy_bytes(void *dst, const void *src, size_t size)
{
	unsigned char		*d;
	const unsigned char	*s;
	size_t				i;

	d = (unsigned char *)dst;
	s = (const unsigned char *)src;
	i = 0;
	while (i < size)
	{
		d[i] = s[i];
		++i;
	}
}

static size_t	tiny_alloc_size(t_block *block, void *ptr)
{
	char		*data;
	size_t		i;

	data = (char *)block + sizeof(t_block);
	i = 0;
	while (i < MAX_ALLOC)
	{
		if (block->size[i] && ptr == data + (i * SIZE_N))
			return (block->size[i]);
		++i;
	}
	return (0);
}

static size_t	alloc_size(void *ptr)
{
	t_block		*current;
	size_t		size;

	current = get_malloc();
	if (current == MAP_FAILED)
		return (0);
	while (current)
	{
		if (current->type == TINY)
		{
			size = tiny_alloc_size(current, ptr);
			if (size != 0)
				return (size);
		}
		else if (ptr == current->data)
			return (current->size[0]);
		current = current->next;
	}
	return (0);
}

void			*ft_realloc(void *ptr, size_t size)
{
	void		*new_ptr;
	size_t		old_size;
	size_t		copy_size;

	if (!ptr)
		return (ft_malloc(size));
	if (size == 0)
	{
		ft_free(ptr);
		return (NULL);
	}
	old_size = alloc_size(ptr);
	if (old_size == 0)
		return (NULL);
	new_ptr = ft_malloc(size);
	if (!new_ptr)
		return (NULL);
	copy_size = old_size;
	if (copy_size > size)
		copy_size = size;
	copy_bytes(new_ptr, ptr, copy_size);
	ft_free(ptr);
	return (new_ptr);
}
