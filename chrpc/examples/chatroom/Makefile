
PROJ_DIR	:=$(shell git rev-parse --show-toplevel)

CHATROOM_DIR := $(PROJ_DIR)/chrpc/examples/chatroom
BUILD_DIR := $(CHATROOM_DIR)/build

INSTALL_DIR := $(PROJ_DIR)/install

CC		:=gcc
FLAGS	:=-Wall -Wextra -Wpedantic -std=c11 -D_POSIX_C_SOURCE=200809L

$(INSTALL_DIR):
	make -C $(PROJ_DIR) headers.install
	make -C $(PROJ_DIR) lib.install

$(BUILD_DIR):
	mkdir -p $@

INCLUDES := \
			$(CHATROOM_DIR) \
			$(INSTALL_DIR)/include
	
_CLIENT_SRCS := client.c
CLIENT_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(_CLIENT_SRCS))
CLIENT_BIN := $(BUILD_DIR)/client

_SERVER_SRCS := server.c chatroom.c
SERVER_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(_SERVER_SRCS))
SERVER_BIN := $(BUILD_DIR)/server

# All objects will follow the same compilation rules.
$(BUILD_DIR)/%.o: $(CHATROOM_DIR)/%.c | $(INSTALL_DIR) $(BUILD_DIR)
	$(CC) -c -o $@ $< $(FLAGS) $(foreach inc,$(INCLUDES),-I$(inc))

LIBS := chjson chrpc chsys chutil

$(CLIENT_BIN): $(CLIENT_OBJS) | $(INSTALL_DIR) $(BUILD_DIR)
	$(CC) -o $@ -L$(INSTALL_DIR) $(foreach lib,$(LIBS),-l$(lib)) $^

$(SERVER_BIN): $(SERVER_OBJS) | $(INSTALL_DIR) $(BUILD_DIR)
	$(CC) -o $@ -L$(INSTALL_DIR) $(foreach lib,$(LIBS),-l$(lib)) $^

.PHONY: all clean clean.deep clangd

CLANGD := $(CHATROOM_DIR)/.clangd

all: $(CLIENT_BIN) $(SERVER_BIN)

clean: 
	rm -rf $(BUILD_DIR)

clean.deep: clean
	rm -f $(CLANGD)
	rm -rf $(INSTALL_DIR)


clangd: $(CLANGD)
$(CLANGD):
	echo "CompileFlags:" > $@
	echo "  Add:" >> $@
	$(foreach inc,$(INCLUDES),echo "  - -I$(inc)" >> $@;)

