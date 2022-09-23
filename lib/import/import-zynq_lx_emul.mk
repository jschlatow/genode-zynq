LIBS += zynq_linux_generated

INC_DIR += $(PRG_DIR)

SRC_C   += dummies.c lx_emul.c
SRC_C   += $(notdir $(wildcard $(PRG_DIR)/generated_dummies.c))

# lx_emul/initcall_order.h
INC_DIR += $(REP_DIR)/src/include

LX_SRC_DIR := $(addsuffix /src/linux, $(call select_from_ports,linux))
ifeq ($(wildcard $(LX_SRC_DIR)),)
LX_SRC_DIR := $(call select_from_repositories,src/linux)
endif

LX_GEN_DIR := $(LIB_CACHE_DIR)/zynq_linux_generated

include $(call select_from_repositories,lib/import/import-lx_emul_common.inc)

INC_DIR += $(LX_SRC_DIR)/scripts/dtc/libfdt
#
# Additional Lx_emul + Lx_kit definitions
#

SRC_CC  += lx_emul/clock.cc
SRC_CC  += lx_emul/io_mem.cc
SRC_CC  += lx_emul/io_port.cc
SRC_CC  += lx_emul/irq.cc
SRC_C   += lx_emul/shadow/kernel/dma/mapping.c
SRC_C   += lx_emul/shadow/kernel/irq/spurious.c
SRC_C   += lx_emul/shadow/kernel/rcu/srcutree.c
SRC_C   += lx_emul/shadow/kernel/rcu/tree.c
SRC_C   += lx_emul/shadow/kernel/smp.c
SRC_C   += lx_emul/shadow/kernel/locking/spinlock.c
SRC_C   += lx_emul/shadow/kernel/stop_machine.c
SRC_C   += lx_emul/shadow/drivers/clk/clk.c
SRC_C   += lx_emul/shadow/drivers/clk/clkdev.c
SRC_C   += lx_emul/shadow/lib/devres.c
SRC_C   += lx_emul/shadow/lib/smp_processor_id.c
SRC_C   += lx_emul/shadow/mm/memblock.c
SRC_C   += lx_emul/shadow/mm/page_alloc.c
SRC_CC  += lx_kit/device.cc
SRC_CC  += lx_kit/memory_dma.cc

CC_OPT_drivers/base/regmap/regmap  += -I$(LX_SRC_DIR)/drivers/base/regmap

DTS_PATH := zynq-7000.dts

CUSTOM_TARGET_DEPS += $(INSTALL_DIR)/$(DRIVER).dtb

$(INSTALL_DIR)/%.dtb: %.dtb
	$(VERBOSE)cp -f $< $@

%.dtb: $(PRG_DIR)/$(DTS_PATH)
	$(VERBOSE)dtc -Idts $< > $@


