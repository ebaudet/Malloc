/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_free.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 18:58:00 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 18:58:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <sys/mman.h>

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

static int		free_tiny(t_block *block, void *ptr)
{
	char		*data;
	size_t		i;

	data = (char *)block + sizeof(t_block);
	i = 0;
	while (i < MAX_ALLOC)
	{
		if (block->size[i] && ptr == data + (i * SIZE_N))
		{
			block->size[i] = 0;
			block->index = index_val(block);
			return (1);
		}
		++i;
	}
	return (0);
}

void			ft_free(void *ptr)
{
	t_block		*previous;
	t_block		*current;

	if (!ptr)
		return ;
	previous = NULL;
	current = get_malloc();
	if (current == MAP_FAILED)
		return ;
	while (current)
	{
		if (current->type == TINY)
		{
			if (free_tiny(current, ptr))
				return ;
		}
		else if (ptr == current->data)
		{
			if (previous)
				previous->next = current->next;
			munmap(current, current->total);
			return ;
		}
		previous = current;
		current = current->next;
	}
}
