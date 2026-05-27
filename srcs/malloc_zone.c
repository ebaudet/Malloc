/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc_zone.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 21:20:00 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 21:20:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <stdint.h>

size_t		malloc_zone_size(size_t type)
{
	size_t		slot;
	size_t		size;

	slot = malloc_slot_size(type);
	size = sizeof(t_zone) + (MIN_ZONE_ALLOCS * (sizeof(t_alloc) + slot));
	return (malloc_page_align(size));
}

void				malloc_insert_zone(t_zone *zone)
{
	t_zone		**head;
	t_zone		*current;

	head = malloc_zones();
	if (!*head || (uintptr_t) zone < (uintptr_t) * head)
	{
		zone->next = *head;
		*head = zone;
		return ;
	}
	current = *head;
	while (current->next && (uintptr_t) current->next < (uintptr_t) zone)
		current = current->next;
	zone->next = current->next;
	current->next = zone;
}

void				malloc_init_fixed_zone(t_zone *zone, size_t type)
{
	t_alloc		*block;
	t_alloc		*previous;
	char		*cursor;
	size_t		i;
	size_t		slot;

	slot = malloc_slot_size(type);
	cursor = (char *)zone + sizeof(t_zone);
	previous = NULL;
	i = 0;
	while (i < MIN_ZONE_ALLOCS)
	{
		block = (t_alloc *)cursor;
		malloc_init_block(zone, block, slot);
			if (!zone->allocs)
				zone->allocs = block;
			else
				previous->next = block;
		previous = block;
		cursor += sizeof(t_alloc) + slot;
		++i;
	}
}

size_t		malloc_large_zone_size(size_t requested)
{
	return (malloc_page_align(sizeof(t_zone) + sizeof(t_alloc)
			+ malloc_align(requested)));
}

void				malloc_init_large_zone(t_zone *zone, size_t requested)
{
	zone->allocs = (t_alloc *)((char *)zone + sizeof(t_zone));
	zone->allocs->size = requested;
	zone->allocs->capacity = requested;
	zone->allocs->free = 0;
	zone->allocs->zone = zone;
}
