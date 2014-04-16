/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_malloc.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2014/04/16 19:00:31 by ebaudet           #+#    #+#             */
/*   Updated: 2014/04/16 22:55:09 by ebaudet          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

void			ft_bzero(void *s, size_t len)
{
	size_t			i;
	unsigned char	*ptr;

	ptr = (unsigned char *)b;
	i = 0;
	while (i < len)
	{
		ptr[i] = 0;
		++i;
	}
}

t_block			*get_malloc(void)
{
	static t_block		*ptr = NULL;


	if (!ptr)
		ptr = mmap(0, PAGE, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	return (ptr);
}

void			*new_block(t_block *ptr, size_t type, size_t size)
{
	size_t		i;

	ptr = (t_block *)mmap(0, size, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0)
	i = 0;
	while (i < MAX_ALLOC)
	{
		ptr->size[i] = 0;
		++i;
	}
	ft_bzero(ptr, size);
	ptr->type = type;
	return (ptr);
}

static int		index_val(t_block *ptr)
{
	size_t		i;

	i = 0;
	while (i < MAX_ALLOC)
	{
		if (ptr->size[i] == 0)
			return (i);
		++i;
	}
	return (i);
}

static void		*malloc_tiny(t_block *ptr, size_t size)
{
	t_block		*current;
	size_t		i;

	current = ptr;
	while (current->type != TINY || current->index == MAX_ALLOC)
	{
		if (!current->next)
			current->next = new_block(current->next, TINY, TINY_BLOCK * PAGE);
		current = current->next;
	}
	i = 0;
	while (current->size[i])
		++i;
	current->size[i] = size;
	current->index = index_val(current);
}

static void		*malloc_small(t_block *ptr, size_t size)
{

}

static void		*malloc_large(t_block *ptr, size_t size)
{

}

void			*ft_malloc(size_t size)
{
	static t_block		*ptr = NULL;
	struct rlimit		rlp;

	if (!ptr)
	{
		ptr = get_malloc();
		ft_bzero(ptr, PAGE);
		ptr->type = TINY;
	}
	if (size <= 0)
		return (NULL);
	getrlimit(RLIMIT_MEMLOCK, &rlp);
	if (ptr->total + size > rlp.rlim_cur)
		return (NULL);
	if (size < SIZE_N)
		return (malloc_tiny(ptr, size));
	else if (size < SIZE_M)
		return (malloc_small(ptr, size));
	else
		return (malloc_large(ptr, size));
	return (NULL);

}


void	show_alloc_mem() {

}