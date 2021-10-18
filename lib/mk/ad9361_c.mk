include $(REP_DIR)/lib/import/import-ad9361.mk

PLATFORM_DIR := $(REP_DIR)/src/lib/ad_noos/platform

LIBS += libc

SRC_CC := genode_backend.cc

SRC_C := ad9361_api.c \
         ad9361.c \
         ad9361_conv.c \
         ad9361_util.c \
         axi_adc_core.c \
         axi_dac_core.c \
         axi_dmac.c \
         spi.c \
         gpio.c \
         util.c \
         genode_spi.c \
         genode_gpio.c \
         linux_delay.c

vpath ad9361_api.c      $(AD9361_DIR)
vpath ad9361.c          $(AD9361_DIR)
vpath ad9361_util.c     $(AD9361_DIR)
vpath ad9361_conv.c     $(AD9361_DIR)
vpath axi_adc_core.c    $(AXICORE_DIR)/axi_adc_core
vpath axi_dac_core.c    $(AXICORE_DIR)/axi_dac_core
vpath axi_dmac.c        $(AXICORE_DIR)/axi_dmac
vpath spi.c             $(ADNOOS_PORT_DIR)/drivers/spi
vpath gpio.c            $(ADNOOS_PORT_DIR)/drivers/gpio
vpath util.c            $(ADNOOS_PORT_DIR)/util
vpath genode_spi.c      $(PLATFORM_DIR)
vpath genode_gpio.c     $(PLATFORM_DIR)
vpath linux_delay.c     $(ADNOOS_PORT_DIR)/drivers/platform/linux
vpath %.cc              $(REP_DIR)/src/lib/ad_noos

