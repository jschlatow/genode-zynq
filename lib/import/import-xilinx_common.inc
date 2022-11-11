XIL_SRC_DIR := $(addsuffix /src/embeddedsw,$(call select_from_ports,xilinx_embeddedsw))

INC_DIR += $(PRG_DIR)
INC_DIR += $(REP_DIR)/src/include/xilinx_common
INC_DIR += $(XIL_SRC_DIR)/lib/bsp/standalone/src/common
#INC_DIR += $(XIL_SRC_DIR)/XilinxProcessorIPLib/drivers/common/src

SRC_C   += src/lib/xilinx_common/outbyte.c

vpath %.c $(REP_DIR)
#vpath % $(XIL_SRC_DIR)/lib/bsp/standalone/src/common
