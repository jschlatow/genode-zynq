TARGET  := zynq_pin_drv
SRC_CC  := main.cc
INC_DIR += $(PRG_DIR)
INC_DIR += $(REP_DIR)/src/drivers/pin/include
LIBS    += base

vpath main.cc $(REP_DIR)/src/drivers/pin
