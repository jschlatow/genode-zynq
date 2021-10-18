ADNOOS_PORT_DIR := $(call select_from_ports,ad_noos)/src

AD9361_DIR   := $(ADNOOS_PORT_DIR)/drivers/rf-transceiver/ad9361
AXICORE_DIR  := $(ADNOOS_PORT_DIR)/drivers/axi_core

INC_DIR += $(AD9361_DIR) \
           $(AXICORE_DIR)/axi_adc_core \
           $(AXICORE_DIR)/axi_dac_core \
           $(AXICORE_DIR)/axi_dmac \
           $(ADNOOS_PORT_DIR)/drivers/spi \
           $(ADNOOS_PORT_DIR)/drivers/gpio \
           $(ADNOOS_PORT_DIR)/include \
           $(REP_DIR)/src/lib/ad_noos \
           $(REP_DIR)/src/lib/ad_noos/platform \
           $(ADNOOS_PORT_DIR)/projects/ad9361/src
