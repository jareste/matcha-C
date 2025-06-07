CC := cc
CFLAGS := -Werror -Wextra -Wall -g -O3 -DUSE_SSL -I$(ROOT_DIR)/inc -I$(ROOT_DIR)/third_party/uthash-master/src/ -I$(ROOT_DIR)/$(THIRD_PARTY_PATH)/cJSON
LDFLAGS := -L$(ROOT_DIR)/$(OPENSSL_BUILD_DIR)/lib -lssl -lcrypto -lm -lpthread -ldl
ROOT_DIR := $(abspath .)
THIRD_PARTY_PATH := third_party
OPENSSL_BUILD_DIR := third_party/openssl_build
AR := ar rcs

MODE ?= DEBUG

ifeq ($(MODE), DEBUG)
CFLAGS += -g
else
CFLAGS += -O3
endif
