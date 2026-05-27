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
#include <stdint.h>

static void	putstr(const char *str)
{
	size_t	len;

	len = 0;
	while (str[len])
		++len;
	write(1, str, len);
}

static void	putnbr(size_t value)
{
	char	buffer[32];
	size_t	i;

	i = 0;
	if (value == 0)
	{
		write(1, "0", 1);
		return ;
	}
	while (value > 0)
	{
		buffer[i++] = (char)('0' + (value % 10));
		value /= 10;
	}
	while (i > 0)
		write(1, &buffer[--i], 1);
}

static void	puthex(uintptr_t value)
{
	char	buffer[32];
	char	*base;
	size_t	i;

	base = "0123456789ABCDEF";
	putstr("0x");
	i = 0;
	if (value == 0)
	{
		write(1, "0", 1);
		return ;
	}
	while (value > 0)
	{
		buffer[i++] = base[value % 16];
		value /= 16;
	}
	while (i > 0)
		write(1, &buffer[--i], 1);
}

static int	zone_has_allocs(t_zone *zone)
{
	t_alloc	*block;

	block = zone->allocs;
	while (block)
	{
		if (!block->free)
			return (1);
		block = block->next;
	}
	return (0);
}

static void	print_header(t_zone *zone)
{
	if (zone->type == TINY)
		putstr("TINY : ");
	else if (zone->type == SMALL)
		putstr("SMALL : ");
	else
		putstr("LARGE : ");
	puthex((uintptr_t)zone);
	putstr("\n");
}

static void	print_alloc(t_alloc *block)
{
	void	*start;

	start = (char *)block + sizeof(t_alloc);
	puthex((uintptr_t)start);
	putstr(" - ");
	puthex((uintptr_t)((char *)start + block->size));
	putstr(" : ");
	putnbr(block->size);
	putstr(" bytes\n");
}

static size_t	print_zone(t_zone *zone)
{
	t_alloc	*block;
	size_t	total;

	total = 0;
	if (!zone_has_allocs(zone))
		return (0);
	print_header(zone);
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
	t_zone	*zone;
	size_t	total;

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
	size_t	total;

	malloc_lock();
	total = 0;
	total += print_type(TINY);
	total += print_type(SMALL);
	total += print_type(LARGE);
	putstr("Total : ");
	putnbr(total);
	putstr(" bytes\n");
	malloc_unlock();
}
