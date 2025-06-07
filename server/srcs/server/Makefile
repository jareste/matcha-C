include ../../config.mk

NAME = libserver.a
SRC = server.c 

OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))
INCLUDES = -I../../inc -I../log
OBJ_DIR = objs
all: $(NAME)

$(NAME): $(OBJ)
	$(AR) $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	echo "Compiling $< to $@"
	@$(CC) $(CFLAGS) $(INCLUDES)  -c $< -o $@

clean:
	$(RM) -rf $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all
