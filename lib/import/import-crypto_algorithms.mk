CRYPTALG_SRC_DIR := $(addsuffix /src,$(call select_from_ports,crypto_algorithms))

INC_DIR += $(CRYPTALG_SRC_DIR)

vpath %.c $(CRYPTALG_SRC_DIR)
