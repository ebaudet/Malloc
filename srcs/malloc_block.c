/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc_block.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 21:35:00 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 21:35:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

void	malloc_init_block(t_zone *zone, t_alloc *block, size_t slot)
{
	block->size = 0;
	block->capacity = slot;
	block->free = 1;
	block->zone = zone;
	block->next = NULL;
}
