include ../../config.mk

NAME = libdb.a
SRC = db.c \
	  db_users.c

OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))
AR = ar rcs
OBJ_DIR = objs
all: $(NAME)

$(NAME): $(OBJ)
	$(AR) $@ $^ -lpq

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	echo "Compiling $< to $@"
	$(CC) $(CFLAGS) -I../../inc -I/usr/include/postgresql -c $< -o $@ -lpq

clean:
	$(RM) -rf $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all
