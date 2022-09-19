MIRROR_FROM_PORT_DIR := src/drivers/rf-transceiver/ad9361 \
                        src/include \
                        src/drivers/axi_core \
                        src/drivers/spi \
                        src/drivers/gpio \
                        src/drivers/platform/linux/linux_delay.c \
                        src/util \
                        src/projects/ad9361/src/app_config.h
MIRROR_FROM_REP_DIR  := src/lib/ad_noos/ad9361.cc \
                        src/lib/ad_noos/genode_backend.cc \
                        src/lib/ad_noos/genode_backend.h \
                        src/lib/ad_noos/platform.h \
                        src/lib/ad_noos/platform/genode_gpio.c \
                        src/lib/ad_noos/platform/genode_spi.c \
                        lib/import/import-ad9361.mk \
                        lib/mk/ad9361.mk \
                        lib/mk/ad9361_c.mk \
                        include/drivers/gpio.h \
                        include/drivers/dmac.h \
                        include/drivers/spi.h \
                        include/util/lazy_array.h \
                        include/ad9361
content: $(MIRROR_FROM_REP_DIR) $(MIRROR_FROM_PORT_DIR) LICENSE

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/ad_noos)

$(MIRROR_FROM_PORT_DIR):
	mkdir -p $(dir $@)
	cp -r $(PORT_DIR)/$@ $(dir $@)

$(MIRROR_FROM_REP_DIR):
	$(mirror_from_rep_dir)

LICENSE:
	cp $(PORT_DIR)/src/LICENSE $@
