/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc_alloc_helpers.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 21:20:00 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 21:20:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif

t_zone			*malloc_new_zone(size_t type, size_t requested)
{
	t_zone			*zone;
	size_t			total;

	if (type == LARGE)
		total = malloc_large_zone_size(requested);
	else
		total = malloc_zone_size(type);
	zone = mmap(0, total, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (zone == MAP_FAILED)
		return (NULL);
	malloc_bzero(zone, total);
	zone->type = type;
	zone->total = total;
	if (type == LARGE)
		malloc_init_large_zone(zone, requested);
	else
		malloc_init_fixed_zone(zone, type);
	malloc_insert_zone(zone);
	return (zone);
}

t_alloc			*malloc_find_free_block(size_t type)
{
	t_zone			*zone;
	t_alloc			*block;

	zone = *malloc_zones();
	while (zone)
	{
		if (zone->type == type)
		{
			block = zone->allocs;
			while (block)
			{
				if (block->free)
					return (block);
				block = block->next;
			}
		}
		zone = zone->next;
	}
	return (NULL);
}

size_t				malloc_zone_type_for_size(size_t size)
{
	if (size <= TINY_MAX)
		return (TINY);
	if (size <= SMALL_MAX)
		return (SMALL);
	return (LARGE);
}

void					*malloc_alloc_large(size_t size)
{
	t_zone			*zone;

	zone = malloc_new_zone(LARGE, size);
	if (!zone)
		return (NULL);
	return ((char *)zone->allocs + sizeof(t_alloc));
}

void					*malloc_alloc_from_zone(size_t size, size_t type)
{
	t_alloc			*block;
	t_zone			*zone;

	block = malloc_find_free_block(type);
	if (!block)
	{
		zone = malloc_new_zone(type, size);
		if (!zone)
			return (NULL);
		block = malloc_find_free_block(type);
	}
	if (!block)
		return (NULL);
	block->size = size;
	block->free = 0;
	return ((char *)block + sizeof(t_alloc));
}
