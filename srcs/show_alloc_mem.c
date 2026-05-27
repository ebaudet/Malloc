/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_alloc_mem.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 20:10:00 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 20:30:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
static void	print_alloc(t_alloc *block)
{
	void		*start;

	start = (char *)block + sizeof(t_alloc);
	malloc_puthex((uintptr_t)start);
	malloc_putstr(" - ");
	malloc_puthex((uintptr_t)((char *)start + block->size));
	malloc_putstr(" : ");
	malloc_putnbr(block->size);
	malloc_putstr(" bytes\n");
}

static size_t	print_zone(t_zone *zone)
{
	t_alloc		*block;
	size_t		total;

	total = 0;
	if (!malloc_zone_has_allocs(zone))
		return (0);
	malloc_print_header(zone);
	block = zone->allocs;
	while (block)
	{
		if (!block->free)
		{
			print_alloc(block);
			total += block->size;
		}
		block = block->next;
	}
	return (total);
}

static size_t	print_type(size_t type)
{
	t_zone		*zone;
	size_t		total;

	total = 0;
	zone = *malloc_zones();
	while (zone)
	{
		if (zone->type == type)
			total += print_zone(zone);
		zone = zone->next;
	}
	return (total);
}

void	show_alloc_mem(void)
{
	size_t		total;

	malloc_lock();
	total = 0;
	total += print_type(TINY);
	total += print_type(SMALL);
	total += print_type(LARGE);
	malloc_putstr("Total : ");
	malloc_putnbr(total);
	malloc_putstr(" bytes\n");
	malloc_unlock();
}
