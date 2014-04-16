# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ebaudet <ebaudet@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2014/04/16 17:24:02 by ebaudet           #+#    #+#              #
#    Updated: 2014/04/16 17:38:39 by ebaudet          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)
endif

NAME	= libft_malloc_$(HOSTTYPE).so

LINK	= libft_malloc.so

FILES	= ft_malloc.c

SRCS	= $(addprefix srcs/, $(FILES))
OBJS	= $(SRCS: srcs/%.c=.obj/%.o)
INC		= -I includes
FLAGS	=