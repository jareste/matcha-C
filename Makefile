NAME = truco

#########
RM = rm -rf
CC = cc
CFLAGS = -Werror -Wextra -Wall -g -O3 -I$(OPENSSL_BUILD_DIR)/include -Iinc -DUSE_SSL -lpthread #-DDEBUG
# CFLAGS = -Werror -Wextra -Wall -g -fsanitize=address -O3 -I$(OPENSSL_BUILD_DIR)/include -Iinc -DUSE_SSL #-DDEBUG
# CFLAGS = -Werror -Wextra -Wall -g -fsanitize=thread -O3 -I$(OPENSSL_BUILD_DIR)/include -Iinc -DUSE_SSL #-DDEBUG
LDFLAGS =	-Lsrcs/log -llog \
			-Lsrcs/parse -lparse \
			-Lsrcs/server -lserver \
			-Lsrcs/router -lrouter \
			-L$(OPENSSL_BUILD_DIR)/lib -lssl -lcrypto -lpthread -lm -ldl
RELEASE_CFLAGS = -Werror -Wextra -Wall -g -O3

LIB_DIRS := log parse server router
LIB_PATHS := $(addprefix srcs/, $(LIB_DIRS))
LIBS := $(addprefix -l, $(LIB_DIRS))
LIBFLAGS := $(addprefix -Lsrcs/, $(LIB_DIRS))
LIB_NAMES := $(foreach dir,$(LIB_DIRS),srcs/$(dir)/lib$(dir).a)

SUBMAKE = $(MAKE) --silent -C
#########

#########
FILES = main  ft_list ft_malloc

SRC = $(addsuffix .c, $(FILES))

vpath %.c srcs inc 
#########

OBJ_DIR = objs

#########
OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))
DEP = $(addsuffix .d, $(basename $(OBJ)))
#########

THIRD_PARTY_PATH = third_party

OPENSSL_SRC_DIR := third_party/openssl
OPENSSL_INSTALL_DIR := third_party/openssl_install

OPENSSL_BUILD_DIR := third_party/openssl_build
OPENSSL_BUILD_DONE := $(OPENSSL_INSTALL_DIR)/lib/libssl.a


#########
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	${CC} -MMD $(CFLAGS) -c $< -o $@


all: .gitignore create_cert build_libs
	@echo "\033[1;32mBuilding $(NAME)...\033[0m"
	@$(MAKE) $(NAME)

$(NAME): $(OBJ) $(LIB_NAMES)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME) $(LDFLAGS) $(LIBFLAGS) $(LIBS)
	@echo "EVERYTHING DONE  "
#	@./.add_path.sh

build_libs:
	@for dir in $(LIB_PATHS); do \
		echo "\033[1;32mBuilding library in $$dir...\033[0m"; \
		$(SUBMAKE) $$dir; \
	done

release: CFLAGS = $(RELEASE_CFLAGS)
release: re
	@echo "RELEASE BUILD DONE  "

clean:
	@$(RM) $(OBJ) $(DEP)
	@$(RM) -r $(OBJ_DIR)
	@echo "OBJECTS REMOVED   "

fclean: clean
	@make --silent -C srcs/log fclean
	@make --silent -C srcs/parse fclean
	@make --silent -C srcs/server fclean
	@make --silent -C srcs/router fclean
	@$(RM) $(NAME)
	@cd $(OPENSSL_SRC_DIR) 2>/dev/null && [ -f Makefile ] && make clean || true
	@rm -rf $(OPENSSL_INSTALL_DIR)
	@echo "EVERYTHING REMOVED   "

re: fclean all

.gitignore:
	@if [ ! -f .gitignore ]; then \
		echo ".gitignore not found, creating it..."; \
		echo ".gitignore" >> .gitignore; \
		echo "$(NAME)" >> .gitignore; \
		echo "$(OBJ_DIR)/" >> .gitignore; \
		echo "$(OPENSSL_SRC_DIR)/" >> .gitignore; \
		echo "*.a" >> .gitignore; \
		echo "*.pem" >> .gitignore; \
		echo ".vscode/" >> .gitignore; \
		echo ".gitignore" >> .gitignore; \
		echo "log.txt" >> .gitignore; \
		echo "*.o" >> .gitignore; \
		echo ".gitignore created and updated with entries."; \
	else \
		echo ".gitignore already exists."; \
	fi

.PHONY: all clean fclean re release .gitignore compile_ssl create_cert

create_cert:
	@if [ ! -f certs/cert.pem ]; then \
		mkdir -p certs; \
		openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout certs/key.pem -out certs/cert.pem -subj "/C=CA/ST=Catalonia/L=Mikasa/O=42/OU=42/CN=localhost"; \
	else \
		echo "Certificate already exists."; \
	fi

-include $(DEP)
