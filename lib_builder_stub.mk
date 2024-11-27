
# NOTE: this Makefile is meant to be INCLUDED.
# It simply forwards the given arguments to the lib_builder.mk
# targets.

# REQUIRED
LIB_NAME 	?=

# REQUIRED
DEPS 		?=

# REQUIRED
_SRCS		?=

# REQUIRED
_TEST_SRCS	?=

PROJ_DIR	:=$(shell git rev-parse --show-toplevel)
LIB_BUILDER :=$(PROJ_DIR)/lib_builder.mk

TARGETS := lib \
		   lib.uninstall \
		   lib.install \
		   headers.uninstall \
		   headers.install \
		   test \
		   test.run \
		   clangd \
		   clean

.PHONY: $(TARGETS)
$(TARGETS):
	make -C $(PROJ_DIR) -f $(LIB_BUILDER) \
		LIB_NAME=$(LIB_NAME) \
		DEPS="$(DEPS)" \
		_SRCS="$(_SRCS)" \
		_TEST_SRCS="$(_TEST_SRCS)" \
		$@
