include $(REP_DIR)/lib/import/import-ad9361.mk

LIBS += libc ad9361_c

SRC_CC := ad9361.cc

vpath %.cc $(REP_DIR)/src/lib/ad_noos
