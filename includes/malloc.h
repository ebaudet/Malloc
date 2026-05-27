/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2014/04/16 17:23:51 by ebaudet           #+#    #+#             */
/*   Updated: 2026/05/27 20:30:00 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_H
# define MALLOC_H

# include <pthread.h>
# include <stddef.h>
# include <stdint.h>
# include <unistd.h>

# define MALLOC_API __attribute__((visibility("default")))
# define ALIGNMENT 16UL
# define MIN_ZONE_ALLOCS 100UL
# define TINY_MAX 512UL
# define SMALL_MAX 4096UL

enum	e_zone_type
{
	TINY = 1,
	SMALL,
	LARGE
};

typedef struct s_zone	t_zone;

typedef struct s_alloc
{
	size_t			size;
	size_t			capacity;
	int				free;
	t_zone			*zone;
	struct s_alloc	*next;
}	t_alloc;

struct s_zone
{
	size_t	type;
	size_t	total;
	t_alloc	*allocs;
	t_zone	*next;
};

MALLOC_API void		*ft_malloc(size_t size);
MALLOC_API void		ft_free(void *ptr);
MALLOC_API void		*ft_realloc(void *ptr, size_t size);
MALLOC_API void		*ft_calloc(size_t count, size_t size);
MALLOC_API void		*malloc(size_t size);
MALLOC_API void		free(void *ptr);
MALLOC_API void		*realloc(void *ptr, size_t size);
MALLOC_API void		show_alloc_mem(void);

size_t		malloc_align(size_t size);
size_t		malloc_page_align(size_t size);
size_t		malloc_slot_size(size_t type);
size_t		malloc_zone_size(size_t type);
void		malloc_bzero(void *ptr, size_t size);
void		malloc_copy(void *dst, const void *src, size_t size);
void		malloc_lock(void);
void		malloc_unlock(void);
t_zone		**malloc_zones(void);
t_alloc		*malloc_find_free_block(size_t type);
size_t		malloc_zone_type_for_size(size_t size);
t_zone		*malloc_new_zone(size_t type, size_t requested);
void		*malloc_alloc_large(size_t size);
void		*malloc_alloc_from_zone(size_t size, size_t type);
t_alloc		*malloc_find_alloc(void *ptr);
void		malloc_insert_zone(t_zone *zone);
void		malloc_init_block(t_zone *zone, t_alloc *block, size_t slot);
void		malloc_init_fixed_zone(t_zone *zone, size_t type);
size_t		malloc_large_zone_size(size_t requested);
void		malloc_init_large_zone(t_zone *zone, size_t requested);
void		*malloc_alloc(size_t size);
void		malloc_free_unlocked(void *ptr);
void		malloc_putstr(const char *str);
void		malloc_putnbr(size_t value);
void		malloc_puthex(uintptr_t value);
int			malloc_zone_has_allocs(t_zone *zone);
void		malloc_print_header(t_zone *zone);

#endif
