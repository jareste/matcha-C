include ../../config.mk

NAME = libapi.a
SRC =	api.c \
		umgmt/umgmt.c \

OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))
AR = ar rcs
OBJ_DIR = objs

CFLAGS += -I../../inc -I/usr/include/postgresql -I$(HOME)/postgresql/include

all: $(NAME)

$(NAME): $(OBJ)
	$(AR) $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	echo "Compiling $< to $@"
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -rf $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all
