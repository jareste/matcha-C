include ../../config.mk

NAME = libdb.a
SRC = db.c \
	  db_gen.c \
	  tables/db_table_user.c \
	  tables/db_table_tag.c \
	  tables/db_table_pic.c \
	  tables/db_table_visit.c \
	  tables/db_table_like.c \
	  tables/db_table_message.c \
	  tables/db_table_notification.c \
	  tables/db_table_session.c \

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
