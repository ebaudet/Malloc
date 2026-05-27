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

# include <stddef.h>
# include <sys/resource.h>
# include <unistd.h>

# define PAGE ((size_t)getpagesize())
# define SIZE_N (PAGE / 8)
# define SIZE_M (PAGE)
# define TINY_BLOCK 100
# define SMALL_BLOCK 800
# define MAX_ALLOC 100

enum				e_alloc_type
{
	TINY = 1,
	SMALL,
	LARGE
};

typedef struct		s_block
{
	size_t			type;
	rlim_t			total;
	size_t			size[MAX_ALLOC];
	size_t			index;
	struct s_block	*next;
	void			*data;
	int				free;
}					t_block;

void				*ft_malloc(size_t size);
void				ft_free(void *ptr);
t_block				*get_malloc(void);
void				show_alloc_mem(void);

#endif
