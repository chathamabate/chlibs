
PROJ_DIR	:=$(shell git rev-parse --show-toplevel)

LIBS		:=chutil chjson chsys chrpc chtest

.PHONY:  logo help

LOGO_COLOR   := \033[36m
TARGET_COLOR := \033[35m
LIB_COLOR    := \033[32m
CMD_COLOR    := \033[33m
RESET_COLOR  := \033[0m


logo:
	@echo "   $(LOGO_COLOR)       _     _ _ _          $(RESET_COLOR)"
	@echo "   $(LOGO_COLOR)      | |   | (_) |         $(RESET_COLOR)"
	@echo "   $(LOGO_COLOR)   ___| |__ | |_| |__  ___  $(RESET_COLOR)"
	@echo "   $(LOGO_COLOR)  / __| '_ \| | | '_ \/ __| $(RESET_COLOR)"
	@echo "   $(LOGO_COLOR) | (__| | | | | | |_) \__ \ $(RESET_COLOR)"
	@echo "   $(LOGO_COLOR)  \___|_| |_|_|_|_.__/|___/ $(RESET_COLOR)"

help: logo
	@echo ""
	@echo " Headers "
	@echo "   $(TARGET_COLOR)headers.uninstall$(RESET_COLOR) - Remove all headers files from the install directory"
	@echo "   $(TARGET_COLOR)headers.install$(RESET_COLOR)   - Copy all headers files to the install directory"
	@echo ""
	@echo " Libraries "
	@echo "   $(TARGET_COLOR)lib$(RESET_COLOR)               - Build all static archive files"
	@echo "   $(TARGET_COLOR)lib.uninstall$(RESET_COLOR)     - Remove all static archives from the install directory"
	@echo "   $(TARGET_COLOR)lib.install$(RESET_COLOR)       - Build and copy all static archives to the install directory"
	@echo ""
	@echo " Testing "
	@echo "   $(TARGET_COLOR)test$(RESET_COLOR)              - Build all test executables"
	@echo "   $(TARGET_COLOR)test.run$(RESET_COLOR)          - Build and run all test executables"
	@echo ""
	@echo " Misc "
	@echo "   $(TARGET_COLOR)help$(RESET_COLOR)              - Display this page"
	@echo "   $(TARGET_COLOR)clangd$(RESET_COLOR)            - Build all Clangd files"
	@echo "   $(TARGET_COLOR)clean$(RESET_COLOR)             - Delete build folder"
	@echo "   $(TARGET_COLOR)clean.deep$(RESET_COLOR)        - Delete build and install folders"
	@echo "   $(TARGET_COLOR)uninstall@unity$(RESET_COLOR)   - Delete unity static archive and headers from the install folder"
	@echo "   $(TARGET_COLOR)install@unity$(RESET_COLOR)     - Download and build unity testing framework"
	@echo ""
	@echo " Module Targeting "
	@echo "   Many of the above targets can be applied to a specific module if desired."
	@echo "   To do so add the suffix @<MODULE_NAME> to an applicable target."
	@echo ""
	@echo "   Valid values for <MODULE_NAME> are..."
	@echo "   $(LIB_COLOR)$(LIBS)$(RESET_COLOR)"
	@echo ""
	@echo " Getting Started "
	@echo "   To get tests up and running, try the following sequence of commands:"
	@echo ""
	@echo "     $(CMD_COLOR)make install@unity$(RESET_COLOR) "
	@echo "     $(CMD_COLOR)make headers.install$(RESET_COLOR) "
	@echo "     $(CMD_COLOR)make lib.install$(RESET_COLOR) "
	@echo "     $(CMD_COLOR)make test$(RESET_COLOR) "
	@echo "     $(CMD_COLOR)make test.run$(RESET_COLOR) "
	@echo ""


# Add Libraries here.

_LIB_TARGETS := \
		lib \
		lib.uninstall \
		lib.install \
		headers.uninstall \
		headers.install \
		test \
		test.run \
		clangd \
		clean

LIB_TARGETS := $(foreach lib,$(LIBS),$(foreach tar,$(_LIB_TARGETS),$(tar)@$(lib)))

.PHONY: $(LIB_TARGETS)
$(LIB_TARGETS): 
	tar=$(shell echo "$@" | sed "s/@.*//"); \
	name=$(shell echo "$@" | sed "s/.*@//"); \
	make -C $(PROJ_DIR)/$$name $$tar

TOP_TARGETS := \
		lib \
		lib.uninstall \
		lib.install \
		headers.uninstall \
		headers.install \
		test \
		test.run \
		clangd 

.PHONY: $(TOP_TARGETS)
$(TOP_TARGETS): %: $(foreach lib,$(LIBS),%@$(lib))

BUILD_DIR		:= $(PROJ_DIR)/build
INSTALL_DIR		:= $(PROJ_DIR)/install

.PHONY: clean clean.deep
clean:
	rm -rf $(BUILD_DIR)

clean.deep: clean
	rm -rf $(INSTALL_DIR)

UNITY_REPO:=https://github.com/ThrowTheSwitch/Unity.git
UNITY_HASH:=73237c5d224169c7b4d2ec8321f9ac92e8071708

UNITY_DIR		:=$(PROJ_DIR)/_temp_unity_dir
UNITY_SRC_DIR	:=$(UNITY_DIR)/src

UNINSTALL_UNITY := uninstall@unity
INSTALL_UNITY   := install@unity

.PHONY: $(UNINSTALL_UNITY) $(INSTALL_UNITY)

$(UNINSTALL_UNITY):
	rm -rf $(INSTALL_DIR)/include/unity
	rm -f $(INSTALL_DIR)/libunity.a

$(INSTALL_UNITY): $(UNINSTALL_UNITY)
	# Clone repo and build static library.
	git clone $(UNITY_REPO) $(UNITY_DIR)
	cd $(UNITY_DIR) && git checkout $(UNITY_HASH)
	$(CC) -c $(UNITY_SRC_DIR)/unity.c -I$(UNITY_SRC_DIR) -o $(UNITY_SRC_DIR)/unity.o -DUNITY_INCLUDE_DOUBLE
	ar rcs $(UNITY_SRC_DIR)/libunity.a $(UNITY_SRC_DIR)/unity.o
	# Copy headers and lib.
	mkdir -p $(INSTALL_DIR)/include/unity
	cp $(UNITY_SRC_DIR)/*.h $(INSTALL_DIR)/include/unity
	cp $(UNITY_SRC_DIR)/libunity.a $(INSTALL_DIR)
	# Finally, remove the temporary directory.
	rm -rf $(UNITY_DIR)

	



