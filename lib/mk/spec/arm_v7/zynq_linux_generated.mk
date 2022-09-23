CUSTOM_TARGET_DEPS := kernel_build.phony

LX_DIR := $(addsuffix /src/linux, $(call select_from_ports,linux))
PWD    := $(shell pwd)

LX_MK_ARGS = ARCH=arm CROSS_COMPILE=$(CROSS_DEV_PREFIX)

#
# Linux kernel configuration
#

# define 'LX_ENABLE' and 'LX_DISABLE'
include $(REP_DIR)/src/zynq_linux/common.inc

# filter for make output of kernel build system
BUILD_OUTPUT_FILTER = 2>&1 | sed "s/^/      [Linux]  /"

kernel_config.tag:
	$(MSG_CONFIG)Linux
	$(VERBOSE)$(MAKE) -C $(LX_DIR) O=$(PWD) $(LX_MK_ARGS) tinyconfig $(BUILD_OUTPUT_FILTER)
	$(VERBOSE)$(LX_DIR)/scripts/config $(addprefix --enable ,$(LX_ENABLE))
	$(VERBOSE)$(LX_DIR)/scripts/config $(addprefix --disable ,$(LX_DISABLE))
	$(VERBOSE)$(LX_DIR)/scripts/config --set-val NR_CPUS 2
	$(VERBOSE)$(MAKE) $(LX_MK_ARGS) olddefconfig $(BUILD_OUTPUT_FILTER)
	$(VERBOSE)$(MAKE) $(LX_MK_ARGS) prepare      $(BUILD_OUTPUT_FILTER)
	$(VERBOSE)touch $@

# update Linux kernel config on makefile changes
kernel_config.tag: $(MAKEFILE_LIST)

kernel_build.phony: kernel_config.tag
