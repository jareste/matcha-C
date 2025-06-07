NAME = matCha

#########
RM = rm -rf
CC = cc
CFLAGS = -Werror -Wextra -Wall -g -O3 -I$(OPENSSL_BUILD_DIR)/include -I/usr/include/postgresql -Ithird_party/uthash-master/src/. -Iinc -DUSE_SSL -lpthread -lpq #-DDEBUG
# CFLAGS = -Werror -Wextra -Wall -g -fsanitize=address -O3 -I$(OPENSSL_BUILD_DIR)/include -Iinc -DUSE_SSL #-DDEBUG
# CFLAGS = -Werror -Wextra -Wall -g -fsanitize=thread -O3 -I$(OPENSSL_BUILD_DIR)/include -Iinc -DUSE_SSL #-DDEBUG
LDFLAGS =	-Lsrcs/log -llog \
			-Lsrcs/parse -lparse \
			-Lsrcs/server -lserver \
			-Lsrcs/router -lrouter \
			-Lsrcs/mail -lmail \
			-Lsrcs/db -ldb \
			-L$(OPENSSL_BUILD_DIR)/lib -lssl -lcrypto -lpthread -lm -ldl -lpq
RELEASE_CFLAGS = -Werror -Wextra -Wall -g -O3

LIB_DIRS := log parse server router mail db
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
	@make --silent -C srcs/mail fclean
	@make --silent -C srcs/db fclean
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

.PHONY: all clean fclean re release .gitignore compile_ssl create_cert  db_up db_down db_reset

create_cert:
	@if [ ! -f certs/cert.pem ]; then \
		mkdir -p certs; \
		openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout certs/key.pem -out certs/cert.pem -subj "/C=CA/ST=Catalonia/L=Mikasa/O=42/OU=42/CN=localhost"; \
	else \
		echo "Certificate already exists."; \
	fi

db_up:
	@service docker start || true
	@echo "Starting the database using Docker Compose..."
	@docker compose -f ../BBDD/docker-compose.yml up -d
	@echo "Database is up and running."

db_down:
	@echo "Stopping the database using Docker Compose..."
	@docker compose -f ../BBDD/docker-compose.yml down
	@echo "Database has been stopped."

db_clean:
	@docker volume rm bbdd_postgres_data

db_reset: db_down db_clean db_up
	@echo "Database has been reset."

db_connect:
	@psql -h localhost -p 5432 -U admin -d matcha_db

docker_start:
	@echo "Starting Docker..."
	@service docker start || true
	@echo "Docker is running."

-include $(DEP)
