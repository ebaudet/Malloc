/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_calloc.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 19:45:00 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 19:45:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

static void	zero_bytes(void *ptr, size_t size)
{
	unsigned char	*bytes;
	size_t			i;

	bytes = (unsigned char *)ptr;
	i = 0;
	while (i < size)
	{
		bytes[i] = 0;
		++i;
	}
}

void		*ft_calloc(size_t count, size_t size)
{
	void	*ptr;
	size_t	total;

	if (size != 0 && count > ((size_t)-1 / size))
		return (NULL);
	total = count * size;
	ptr = ft_malloc(total);
	if (!ptr)
		return (NULL);
	zero_bytes(ptr, total);
	return (ptr);
}
