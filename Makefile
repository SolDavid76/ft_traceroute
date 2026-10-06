SRCS_DIR	=	sources/

SRC			=	main.c \
				print.c \
				probe.c \
				receive.c \
				init.c

SRCS		=	$(addprefix $(SRCS_DIR), $(SRC))

OBJS_DIR	=	objs/

OBJ			=	$(SRC:.c=.o)

OBJS		=	$(addprefix $(OBJS_DIR), $(OBJ))

DEPS		=	$(OBJS:.o=.d)

INCS		=	-I includes/

CC			=	gcc
CFLAGS		=	-Wall -Wextra -Werror
DEPFLAGS	=	-MMD -MP

NAME		=	ft_traceroute

all			:	$(NAME)

$(NAME)		:	$(OBJS)
				$(CC) $(CFLAGS) $(OBJS) -o $(NAME) $(INCS) -lm

$(OBJS_DIR)%.o: $(SRCS_DIR)%.c | $(OBJS_DIR)
				$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@ $(INCS)

$(OBJS_DIR)	:
				mkdir -p $(OBJS_DIR)

clean		:
				rm -rf $(OBJS_DIR)

fclean		:	clean
				rm -f $(NAME)

re			:	fclean all

test		:
				CC="$(CC)" python3 tests/three_probes.py

.PHONY		:	all clean fclean re test

-include $(DEPS)
