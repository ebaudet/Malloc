/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_alloc_mem_utils.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 21:20:00 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 21:20:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

void				malloc_putstr(const char *str)
{
	size_t		len;

	len = 0;
	while (str[len])
		++len;
	write(1, str, len);
}

void				malloc_putnbr(size_t value)
{
	char		buffer[32];
	size_t		i;

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

void				malloc_puthex(uintptr_t value)
{
	char		buffer[32];
	char		*base;
	size_t		i;

	base = "0123456789ABCDEF";
	malloc_putstr("0x");
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

int					malloc_zone_has_allocs(t_zone *zone)
{
	t_alloc		*block;

	block = zone->allocs;
	while (block)
	{
		if (!block->free)
			return (1);
		block = block->next;
	}
	return (0);
}

void				malloc_print_header(t_zone *zone)
{
	if (zone->type == TINY)
		malloc_putstr("TINY : ");
	else if (zone->type == SMALL)
		malloc_putstr("SMALL : ");
	else
		malloc_putstr("LARGE : ");
	malloc_puthex((uintptr_t)zone);
	malloc_putstr("\n");
}
