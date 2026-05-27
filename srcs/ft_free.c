/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_free.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 18:58:00 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 20:30:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <sys/mman.h>

t_alloc	*malloc_find_alloc(void *ptr)
{
	t_zone	*zone;
	t_alloc	*block;
	void	*data;

	if (!ptr)
		return (NULL);
	zone = *malloc_zones();
	while (zone)
	{
		block = zone->allocs;
		while (block)
		{
			data = (char *)block + sizeof(t_alloc);
			if (data == ptr)
				return (block);
			block = block->next;
		}
		zone = zone->next;
	}
	return (NULL);
}

static void	remove_zone(t_zone *zone)
{
	t_zone	**head;
	t_zone	*current;

	head = malloc_zones();
	if (*head == zone)
	{
		*head = zone->next;
		return ;
	}
	current = *head;
	while (current && current->next != zone)
		current = current->next;
	if (current)
		current->next = zone->next;
}

void	malloc_free_unlocked(void *ptr)
{
	t_alloc	*block;
	t_zone	*zone;

	block = malloc_find_alloc(ptr);
	if (!block || block->free)
		return ;
	zone = block->zone;
	if (zone->type == LARGE)
	{
		remove_zone(zone);
		munmap(zone, zone->total);
		return ;
	}
	block->size = 0;
	block->free = 1;
}

void	ft_free(void *ptr)
{
	malloc_lock();
	malloc_free_unlocked(ptr);
	malloc_unlock();
}

void	free(void *ptr)
{
	ft_free(ptr);
}
