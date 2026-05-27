/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc_utils.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 21:20:00 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 21:20:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

size_t		malloc_align(size_t size)
{
	return ((size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1));
}

size_t		malloc_page_align(size_t size)
{
	size_t		page;

	page = (size_t)getpagesize();
	return (((size + page - 1) / page) * page);
}

void				malloc_bzero(void *ptr, size_t size)
{
	unsigned char	*bytes;
	size_t			i;

	bytes = (unsigned char *)ptr;
	i = 0;
	while (i < size)
		bytes[i++] = 0;
}

void				malloc_copy(void *dst, const void *src, size_t size)
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

size_t		malloc_slot_size(size_t type)
{
	if (type == TINY)
		return (malloc_align(TINY_MAX));
	if (type == SMALL)
		return (malloc_align(SMALL_MAX));
	return (0);
}
