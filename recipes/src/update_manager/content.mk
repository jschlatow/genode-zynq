SRC_DIR = src/app/update_manager
include $(GENODE_DIR)/repos/base/recipes/src/content.inc

MIRROR_FROM_GEMS_DIR := include/depot

content: $(MIRROR_FROM_GEMS_DIR)

$(MIRROR_FROM_GEMS_DIR):
	mkdir -p $(dir $@)
	cp -r $(GENODE_DIR)/repos/gems/$@ $@
