
PROJ_DIR	:=$(shell git rev-parse --show-toplevel)

# Add Libraries here.
LIBS		:=chutil chjson chsys

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
	$(CC) -c $(UNITY_SRC_DIR)/unity.c -I$(UNITY_SRC_DIR) -o $(UNITY_SRC_DIR)/unity.o
	ar rcs $(UNITY_SRC_DIR)/libunity.a $(UNITY_SRC_DIR)/unity.o
	# Copy headers and lib.
	mkdir -p $(INSTALL_DIR)/include/unity
	cp $(UNITY_SRC_DIR)/*.h $(INSTALL_DIR)/include/unity
	cp $(UNITY_SRC_DIR)/libunity.a $(INSTALL_DIR)
	# Finally, remove the temporary directory.
	rm -rf $(UNITY_DIR)

	



