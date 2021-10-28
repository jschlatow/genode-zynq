ADNOOS_PORT_DIR := $(call select_from_ports,ad_noos)/src

TARGET = test-dmac_loopback
LIBS   = base libc ad9361

INC_DIR += $(PRG_DIR)

SRC_CC = main.cc
