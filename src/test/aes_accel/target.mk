TARGET = test-aes_accel

REQUIRES := arm_v7a
LIBS = base libc xilinx_axidma crypto_algorithms

SRC_C += aes.c

SRC_CC += main.cc

vpath %.cc $(PRG_DIR)
