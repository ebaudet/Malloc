/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc_state.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 21:20:00 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 21:20:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

static t_zone			*g_zones = NULL;
static pthread_mutex_t	g_malloc_mutex = PTHREAD_MUTEX_INITIALIZER;

void					malloc_lock(void)
{
	pthread_mutex_lock(&g_malloc_mutex);
}

void					malloc_unlock(void)
{
	pthread_mutex_unlock(&g_malloc_mutex);
}

t_zone				**malloc_zones(void)
{
	return (&g_zones);
}
