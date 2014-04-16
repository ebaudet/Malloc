/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2014/04/16 17:23:51 by ebaudet           #+#    #+#             */
/*   Updated: 2014/04/16 22:24:16 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_H
# define MALLOC_H

# define PAGE getpagesize()
# define SIZE_N (PAGE / 8)
# define SIZE_M (PAGE)
# define TINY_BLOCK 100
# define SMALL_BLOCK 800
# define TINY_BLOCK 1
# define MAX_ALLOC 100

typedef struct		s_block
{
	size_t			type;
	size_t			size;
	rlim_t			total;
	size_t			size[MAX_ALLOC];
	s_block			*next;
	void			*data;
	int				free;
}					t_block;

#endif
