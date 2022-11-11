include $(call select_from_repositories,lib/import/import-xilinx_common.inc)

LIBS += libc

INC_DIR += $(REP_DIR)/src/include/xilinx_axidma
INC_DIR += $(XIL_SRC_DIR)/XilinxProcessorIPLib/drivers/axidma/src/

SRC_C += xaxidma.c

SRC_CC += xilinx_axidma.cc

vpath xilinx_axidma.cc $(REP_DIR)/src/lib/xilinx_axidma
vpath xaxidma.c        $(XIL_SRC_DIR)/XilinxProcessorIPLib/drivers/axidma/src

#CC_CXX_WARN_STRICT =
