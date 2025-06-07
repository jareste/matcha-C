include ../../config.mk

NAME = liblog.a
SRC = log.c

OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))
AR = ar rcs
OBJ_DIR = objs
all: $(NAME)

$(NAME): $(OBJ)
	$(AR) $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	echo "Compiling $< to $@"
	$(CC) $(CFLAGS) -I../../inc -c $< -o $@

clean:
	$(RM) -rf $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all
