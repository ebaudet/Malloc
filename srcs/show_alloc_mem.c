/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_alloc_mem.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 20:10:00 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 20:10:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <stdint.h>
#include <sys/mman.h>

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

static void	print_header(t_block *block)
{
	if (block->type == TINY)
		putstr("TINY : ");
	else if (block->type == SMALL)
		putstr("SMALL : ");
	else
		putstr("LARGE : ");
	puthex((uintptr_t)block);
	putstr("\n");
}

static void	print_alloc(void *start, size_t size)
{
	puthex((uintptr_t)start);
	putstr(" - ");
	puthex((uintptr_t)((char *)start + size));
	putstr(" : ");
	putnbr(size);
	putstr(" bytes\n");
}

static size_t	print_tiny_block(t_block *block)
{
	size_t	total;
	size_t	i;
	char	*data;

	total = 0;
	i = 0;
	data = (char *)block + sizeof(t_block);
	print_header(block);
	while (i < MAX_ALLOC)
	{
		if (block->size[i])
		{
			print_alloc(data + (i * SIZE_N), block->size[i]);
			total += block->size[i];
		}
		++i;
	}
	return (total);
}

static size_t	print_block(t_block *block)
{
	print_header(block);
	print_alloc(block->data, block->size[0]);
	return (block->size[0]);
}

static size_t	print_type(t_block *blocks, size_t type)
{
	t_block		*current;
	size_t		total;

	total = 0;
	current = blocks;
	while (current)
	{
		if (current->type == type)
		{
			if (type == TINY)
				total += print_tiny_block(current);
			else if (current->size[0])
				total += print_block(current);
		}
		current = current->next;
	}
	return (total);
}

void		show_alloc_mem(void)
{
	t_block		*blocks;
	size_t		total;

	blocks = get_malloc();
	if (blocks == MAP_FAILED)
		return ;
	total = 0;
	total += print_type(blocks, TINY);
	total += print_type(blocks, SMALL);
	total += print_type(blocks, LARGE);
	putstr("Total : ");
	putnbr(total);
	putstr(" bytes\n");
}
