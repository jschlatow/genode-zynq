TARGET = test-dma_loopback

REQUIRES := arm_v7a
LIBS = base libc xilinx_axidma

SRC_CC += main.cc
