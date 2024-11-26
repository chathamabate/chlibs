
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
CC			?=gcc
INSTALL_DIR	?=$(PROJ_DIR)/install

FLAGS:=-Wall -Wextra -Wpedantic -std=c11 -D_POSIX_C_SOURCE=200809L

# Where to search for static libraries and header files
# of other modules.

# I like absolute paths tbh.
LIB_DIR		:=$(PROJ_DIR)/$(LIB_NAME)

INCLUDE_DIR	:=$(LIB_DIR)/include
SRC_DIR		:=$(LIB_DIR)/src
TEST_DIR	:=$(LIB_DIR)/test

SRCS :=$(addprefix $(SRC_DIR)/,$(_SRCS))
TEST_SRCS :=$(addprefix $(TEST_DIR)/,$(_TEST_SRCS))

BUILD_DIR		:=$(PROJ_DIR)/build/$(LIB_NAME)
BUILD_TEST_DIR	:=$(BUILD_DIR)/test

_LIB_FILE		:=lib$(LIB_NAME).a
LIB_FILE		:=$(BUILD_DIR)/$(_LIB_FILE)

# Each library will be built entirely independently to its
# dependencies. It will search for libraries in the install path.

HEADERS			:=$(wildcard $(INCLUDE_DIR)/$(LIB_NAME)/*.h)
PRIVATE_HEADERS	:=$(wildcard $(SRC_DIR)/*.h)
OBJS			:=$(patsubst %.c,$(BUILD_DIR)/%.o,$(_SRCS))

TEST_HEADERS	:=$(wildcard $(TEST_DIR)/*.h)
TEST_OBJS		:=$(patsubst %.c,$(BUILD_TEST_DIR)/%.o,$(_TEST_SRCS))

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
DEPS_FLAGS	:=$(addprefix -L,$(DEPS_PATHS)) $(foreach dep,$(DEPS),-l$(dep))

.PHONY: all lib test run_tests
.PHONY: uninstall_headers install_headser uninstall_lib install_lib
.PHONY: clean clangd clean_clangd

all: lib test

lib: $(LIB_FILE)

test: $(BUILD_TEST_DIR)/test

run_tests: test
	$(BUILD_TEST_DIR)/test	

$(INSTALL_DIR) $(INSTALL_DIR)/include:
	mkdir -p $@

uninstall_headers:
	rm -rf $(INSTALL_DIR)/include/$(LIB_NAME)

install_headers: uninstall_headers | $(INSTALL_DIR)/include
	cp -r $(INCLUDE_DIR)/$(LIB_NAME) $(INSTALL_DIR)/include

uninstall_lib:
	rm -f $(INSTALL_DIR)/$(LIB_FILE_NAME)

install_lib: uninstall_lib $(LIB_FILE) | $(INSTALL_DIR)
	cp $(LIB_FILE) $(INSTALL_DIR)

uninstall: uninstall_lib uninstall_headers

install: install_lib install_headers

clean:
	rm -rf $(BUILD_DIR)

clangd:
	cp $(PROJ_DIR)/clangd_template.yml $(INCLUDE_DIR)/.clangd
	$(foreach flag,$(INCLUDE_INCLUDE_FLAGS),echo "    - $(flag)" >> $(INCLUDE_DIR)/.clangd;) 
	cp $(PROJ_DIR)/clangd_template.yml $(SRC_DIR)/.clangd
	$(foreach flag,$(SRC_INCLUDE_FLAGS),echo "    - $(flag)" >> $(SRC_DIR)/.clangd;) 
	cp $(PROJ_DIR)/clangd_template.yml $(TEST_DIR)/.clangd
	$(foreach flag,$(TEST_INCLUDE_FLAGS),echo "    - $(flag)" >> $(TEST_DIR)/.clangd;) 

clean_clangd:
	rm -f $(INCLUDE_DIR)/.clangd
	rm -f $(SRC_DIR)/.clangd
	rm -f $(TEST_DIR)/.clangd

$(BUILD_DIR) $(BUILD_TEST_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(PRIVATE_HEADERS) $(HEADERS) | $(BUILD_DIR)
	$(CC) -c $(FLAGS) $(SRC_INCLUDE_FLAGS) $< -o $@

# Remember, the lib file doesn't actually care about dependencies.
$(LIB_FILE): $(FULL_OBJS)
	ar rcs $@ $^

$(BUILD_TEST_DIR)/%.o: $(TEST_DIR)/%.c $(HEADERS) $(TEST_HEADERS) | $(BUILD_TEST_DIR)
	$(CC) -c $(FLAGS) $(TEST_INCLUDE_FLAGS) $< -o $@

# Always include unity!
$(BUILD_TEST_DIR)/test: $(FULL_TEST_OBJS) $(LIB_FILE)
	$(CC) $^ $(DEPS_FLAGS) -lunity -o $@

