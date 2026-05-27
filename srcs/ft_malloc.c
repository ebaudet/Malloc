/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_malloc.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2014/04/16 19:00:31 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 20:30:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <stdint.h>
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif

static t_zone			*g_zones = NULL;
static pthread_mutex_t	g_malloc_mutex = PTHREAD_MUTEX_INITIALIZER;

size_t			malloc_align(size_t size)
{
	return ((size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1));
}

size_t			malloc_page_align(size_t size)
{
	size_t	page;

	page = (size_t)getpagesize();
	return (((size + page - 1) / page) * page);
}

void			malloc_bzero(void *ptr, size_t size)
{
	unsigned char	*bytes;
	size_t			i;

	bytes = (unsigned char *)ptr;
	i = 0;
	while (i < size)
		bytes[i++] = 0;
}

void			malloc_copy(void *dst, const void *src, size_t size)
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

void			malloc_lock(void)
{
	pthread_mutex_lock(&g_malloc_mutex);
}

void			malloc_unlock(void)
{
	pthread_mutex_unlock(&g_malloc_mutex);
}

t_zone			**malloc_zones(void)
{
	return (&g_zones);
}

size_t			malloc_slot_size(size_t type)
{
	if (type == TINY)
		return (malloc_align(TINY_MAX));
	if (type == SMALL)
		return (malloc_align(SMALL_MAX));
	return (0);
}

size_t			malloc_zone_size(size_t type)
{
	size_t	slot;
	size_t	size;

	slot = malloc_slot_size(type);
	size = sizeof(t_zone) + (MIN_ZONE_ALLOCS * (sizeof(t_alloc) + slot));
	return (malloc_page_align(size));
}

static void		insert_zone(t_zone *zone)
{
	t_zone	**head;
	t_zone	*current;

	head = malloc_zones();
	if (!*head || (uintptr_t)zone < (uintptr_t)*head)
	{
		zone->next = *head;
		*head = zone;
		return ;
	}
	current = *head;
	while (current->next && (uintptr_t)current->next < (uintptr_t)zone)
		current = current->next;
	zone->next = current->next;
	current->next = zone;
}

static void		init_block(t_zone *zone, t_alloc *block, size_t slot)
{
	block->size = 0;
	block->capacity = slot;
	block->free = 1;
	block->zone = zone;
	block->next = NULL;
}

static void		init_fixed_zone(t_zone *zone, size_t type)
{
	t_alloc	*block;
	t_alloc	*previous;
	char	*cursor;
	size_t	i;
	size_t	slot;

	slot = malloc_slot_size(type);
	cursor = (char *)zone + sizeof(t_zone);
	previous = NULL;
	i = 0;
	while (i < MIN_ZONE_ALLOCS)
	{
		block = (t_alloc *)cursor;
		init_block(zone, block, slot);
		if (!zone->allocs)
			zone->allocs = block;
		else
			previous->next = block;
		previous = block;
		cursor += sizeof(t_alloc) + slot;
		++i;
	}
}

static size_t	large_zone_size(size_t requested)
{
	return (malloc_page_align(sizeof(t_zone) + sizeof(t_alloc)
			+ malloc_align(requested)));
}

static void		init_large_zone(t_zone *zone, size_t requested)
{
	zone->allocs = (t_alloc *)((char *)zone + sizeof(t_zone));
	zone->allocs->size = requested;
	zone->allocs->capacity = requested;
	zone->allocs->free = 0;
	zone->allocs->zone = zone;
}

static t_zone	*new_zone(size_t type, size_t requested)
{
	t_zone	*zone;
	size_t	total;

	if (type == LARGE)
		total = large_zone_size(requested);
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
		init_large_zone(zone, requested);
	else
		init_fixed_zone(zone, type);
	insert_zone(zone);
	return (zone);
}

static t_alloc	*find_free_block(size_t type)
{
	t_zone	*zone;
	t_alloc	*block;

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

static size_t	zone_type_for_size(size_t size)
{
	if (size <= TINY_MAX)
		return (TINY);
	if (size <= SMALL_MAX)
		return (SMALL);
	return (LARGE);
}

static void		*alloc_large(size_t size)
{
	t_zone	*zone;

	zone = new_zone(LARGE, size);
	if (!zone)
		return (NULL);
	return ((char *)zone->allocs + sizeof(t_alloc));
}

static void		*alloc_from_zone(size_t size, size_t type)
{
	t_alloc	*block;
	t_zone	*zone;

	block = find_free_block(type);
	if (!block)
	{
		zone = new_zone(type, size);
		if (!zone)
			return (NULL);
		block = find_free_block(type);
	}
	if (!block)
		return (NULL);
	block->size = size;
	block->free = 0;
	return ((char *)block + sizeof(t_alloc));
}

void			*malloc_alloc(size_t size)
{
	size_t	type;

	if (size == 0)
		return (NULL);
	type = zone_type_for_size(size);
	if (type == LARGE)
		return (alloc_large(size));
	return (alloc_from_zone(size, type));
}

void			*ft_malloc(size_t size)
{
	void	*ptr;

	malloc_lock();
	ptr = malloc_alloc(size);
	malloc_unlock();
	return (ptr);
}

void			*malloc(size_t size)
{
	return (ft_malloc(size));
}
