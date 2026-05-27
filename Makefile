# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2014/04/16 17:24:02 by ebaudet           #+#    #+#              #
#    Updated: 2019/10/15 17:05:35 by ebaudet          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

NAME	= libft_malloc_$(HOSTTYPE).so

LINK	= libft_malloc.so

CC		= cc
FILES	= ft_malloc.c ft_free.c ft_realloc.c ft_calloc.c show_alloc_mem.c \
		  malloc_utils.c malloc_state.c malloc_zone.c malloc_block.c \
		  malloc_alloc_helpers.c \
		  show_alloc_mem_utils.c

SRCS	= $(addprefix srcs/, $(FILES))
OBJS	= $(SRCS:srcs/%.c=.obj/%.o)
INC		= -I includes
CFLAGS	= -Wall -Wextra -Werror -fPIC -fvisibility=hidden -pthread
LDFLAGS	= -shared

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS = -dynamiclib
endif

.PHONY: all clean fclean re test norm-lint

all: $(NAME) $(LINK)

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) -pthread -o $@ $^

$(LINK): $(NAME)
	ln -sf $(NAME) $(LINK)

.obj/%.o: srcs/%.c includes/malloc.h
	@mkdir -p .obj
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -rf .obj

fclean: clean
	rm -f $(NAME) $(LINK) libft_malloc_*.so

re: fclean all

test: all
	sh tests/run_tests.sh

norm-lint:
	python3 tools/norm_linter.py
