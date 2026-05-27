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

void		*malloc_alloc(size_t size)
{
	size_t		type;

	if (size == 0)
		return (NULL);
	type = malloc_zone_type_for_size(size);
	if (type == LARGE)
		return (malloc_alloc_large(size));
	return (malloc_alloc_from_zone(size, type));
}

void		*ft_malloc(size_t size)
{
	void		*ptr;

	malloc_lock();
	ptr = malloc_alloc(size);
	malloc_unlock();
	return (ptr);
}

void		*malloc(size_t size)
{
	return (ft_malloc(size));
}
