ADNOOS_PORT_DIR := $(call select_from_ports,ad_noos)/src

TARGET = test-ad9361
LIBS   = base libc ad9361_c

INC_DIR += $(PRG_DIR)

SRC_CC = component.cc
SRC_C = main.c

vpath main.c $(ADNOOS_PORT_DIR)/projects/ad9361/src
