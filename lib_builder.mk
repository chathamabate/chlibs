
PROJ_DIR:=$(shell git rev-parse --show-toplevel)

# NOTE: This Makefile is meant to be INVOKED.
# A Library is supposed to be in a directory which shares its name.
# It must have  src, include, and test folders.

# REQUIRED
LIB_NAME 	?=

# REQUIRED
DEPS 		?=

# REQUIRED
_SRCS		?=

# REQUIRED
_TEST_SRCS	?=

# OPTIONALS
INSTALL_DIR	?=$(PROJ_DIR)/install
BUILD_DIR	?=$(PROJ_DIR)/build/$(LIB_NAME)

CC			:=gcc
FLAGS:=-Wall -Wextra -Wpedantic -std=c11 -D_POSIX_C_SOURCE=200809L

# Where to search for static libraries and header files
# of other modules.

# I like absolute paths tbh.
LIB_DIR		:=$(PROJ_DIR)/$(LIB_NAME)
INSTALL_INCLUDE_DIR := $(INSTALL_DIR)/include

INCLUDE_DIR	:=$(LIB_DIR)/include
SRC_DIR		:=$(LIB_DIR)/src
TEST_DIR	:=$(LIB_DIR)/test

SRCS :=$(addprefix $(SRC_DIR)/,$(_SRCS))
TEST_SRCS :=$(addprefix $(TEST_DIR)/,$(_TEST_SRCS))

OBJS_DIR	  := $(BUILD_DIR)/objs
TEST_OBJS_DIR := $(BUILD_DIR)/test-objs

_LIB_FILE		:=lib$(LIB_NAME).a
LIB_FILE		:=$(BUILD_DIR)/$(_LIB_FILE)
INSTALLED_LIB_FILE := $(INSTALL_DIR)/$(_LIB_FILE)

# Each library will be built entirely independently to its
# dependencies. It will search for libraries in the install path.

HEADERS			:=$(wildcard $(INCLUDE_DIR)/$(LIB_NAME)/*.h)
PRIVATE_HEADERS	:=$(wildcard $(SRC_DIR)/*.h)
OBJS			:=$(patsubst %.c,$(OBJS_DIR)/%.o,$(_SRCS))

TEST_HEADERS	:=$(wildcard $(TEST_DIR)/*.h)
TEST_OBJS		:=$(patsubst %.c,$(TEST_OBJS_DIR)/%.o,$(_TEST_SRCS))
TEST_EXEC		:=$(BUILD_DIR)/test

# Headers accessible within the include directory.
# Only really used for clangd generation.
INCLUDE_INCLUDE_PATHS :=$(INCLUDE_DIR) $(INSTALL_DIR)/include
INCLUDE_INCLUDE_FLAGS :=$(addprefix -I,$(INCLUDE_INCLUDE_PATHS))

# It is important our local include is first.
# This should be searched first.
SRC_INCLUDE_PATHS	:=$(INCLUDE_DIR) $(SRC_DIR) $(INSTALL_DIR)/include
SRC_INCLUDE_FLAGS	:=$(addprefix -I,$(SRC_INCLUDE_PATHS))

TEST_INCLUDE_PATHS :=$(INCLUDE_DIR) $(TEST_DIR) $(INSTALL_DIR)/include
TEST_INCLUDE_FLAGS :=$(addprefix -I,$(TEST_INCLUDE_PATHS))

# Where to look for static library dependencies.
# Again, it is important our local build is first.
DEPS_PATHS 	:=$(BUILD_DIR) $(INSTALL_DIR)
DEPS_FLAGS	:=$(addprefix -L,$(DEPS_PATHS)) $(foreach dep, $(DEPS),-l$(dep))

# New Targets....

$(BUILD_DIR) $(OBJS_DIR) $(TEST_OBJS_DIR) $(INSTALL_DIR) $(INSTALL_INCLUDE_DIR):
	mkdir -p $@

$(OBJS): $(OBJS_DIR)/%.o: $(SRC_DIR)/%.c $(PRIVATE_HEADERS) $(HEADERS) | $(OBJS_DIR)
	$(CC) $< -c -o $@ $(FLAGS) $(SRC_INCLUDE_FLAGS)

$(LIB_FILE): $(OBJS)
	ar rcs $@ $^

.PHONY: lib lib.uninstall lib.install
lib: $(LIB_FILE)

lib.uninstall:
	rm -f $(INSTALLED_LIB_FILE)
lib.install: $(LIB_FILE) | $(INSTALL_DIR)
	mkdir -p $(INCLUDE_DIR)
	cp $< $(INSTALLED_LIB_FILE)

.PHONY: headers.uninstall headers.install
headers.uninstall:
	rm -rf $(INSTALL_INCLUDE_DIR)/$(LIB_NAME)
headers.install: headers.uninstall | $(INSTALL_INCLUDE_DIR)
	cp -r $(INCLUDE_DIR)/$(LIB_NAME) $(INSTALL_INCLUDE_DIR)

$(TEST_OBJS): $(TEST_OBJS_DIR)/%.o: $(TEST_DIR)/%.c $(TEST_HEADERS) $(HEADERS) | $(TEST_OBJS_DIR)
	$(CC) $< -c -o $@ $(FLAGS) $(TEST_INCLUDE_FLAGS)

.PHONY: test test.run
test: $(TEST_EXEC)
$(TEST_EXEC): $(TEST_OBJS) $(LIB_FILE) | $(BUILD_DIR)
	$(CC) $(TEST_OBJS) $(DEPS_FLAGS) -lunity -l$(LIB_NAME) -o $@

test.run: $(TEST_EXEC)
	cd $(BUILD_DIR) && ./test

.PHONY: clangd
clangd:
	cp $(PROJ_DIR)/clangd_template.yml $(INCLUDE_DIR)/.clangd
	$(foreach flag,$(INCLUDE_INCLUDE_FLAGS),echo "    - $(flag)" >> $(INCLUDE_DIR)/.clangd;) 
	cp $(PROJ_DIR)/clangd_template.yml $(SRC_DIR)/.clangd
	$(foreach flag,$(SRC_INCLUDE_FLAGS),echo "    - $(flag)" >> $(SRC_DIR)/.clangd;) 
	cp $(PROJ_DIR)/clangd_template.yml $(TEST_DIR)/.clangd
	$(foreach flag,$(TEST_INCLUDE_FLAGS),echo "    - $(flag)" >> $(TEST_DIR)/.clangd;) 

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

