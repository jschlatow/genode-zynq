TARGET   = zynq_uboot
REQUIRES = arm_v7a

SUPPORTED_BOARDS := zynq_usrp_e31x zynq_zybo_z7

UBOOT_DEFCONFIG(zynq_usrp_e31x)   := xilinx_zynq_virt_defconfig
UBOOT_DEFCONFIG(zynq_zybo_z7)     := xilinx_zynq_virt_defconfig
UBOOT_DEVICE_TREE(zynq_zybo_z7)   := zynq-zybo-z7
UBOOT_DEVICE_TREE(zynq_usrp_e31x) := zynq-usrp-e31x

UBOOT_OPTION(zynq_zybo_z7)   += OF_LIST="zynq-zybo-z7"
UBOOT_OPTION(zynq_zybo_z7)   += DEFAULT_ENV_FILE="./env.txt"
UBOOT_OPTION(zynq_zybo_z7)   += SPL_PAYLOAD="u-boot.img"
UBOOT_OPTION(zynq_usrp_e31x) += OF_LIST="zynq-usrp-e31x"
UBOOT_OPTION(zynq_usrp_e31x) += DEFAULT_ENV_FILE="./env.txt"
UBOOT_OPTION(zynq_usrp_e31x) += SPL_PAYLOAD="u-boot.img"
UBOOT_ENABLE_COMMON          += CMD_ZYNQ CMD_FAT USE_DEFAULT_ENV_FILE SD_BOOT SPL_LEGACY_IMAGE_SUPPORT SPL_FIT USE_BOOTCOMMAND CMD_PING CMD_DHCP FS_GENERIC
UBOOT_DISABLE_COMMON         += USB SPL_FIT_PRINT SPL_OS_BOOT BOOTM_EFI SPL_FPGA
UBOOT_DISABLE_COMMON         += ENV_IS_IN_NAND ENV_IS_IN_SPI_FLASH BOOTM_PLAN9 BOOTM_RTEMS BOOTM_VXWORKS
UBOOT_DISABLE_COMMON         += CMD_EXT4_WRITE CMD_UBI CMD_MTDPARTS CMD_USB CMD_SF_TEST CMD_I2C CMD_GPIO CMD_DFU CMD_MEMTEST CMD_THOR_DOWNLOAD CMD_SPL CMD_NAND CMD_SF CMD_EXT2 CMD_EXT4 CMD_BOOTEFI

# do not confuse third-party sub-makes
unexport BOARD
unexport MAKEFLAGS
unexport .SHELLFLAGS


#
# U-Boot
#

UBOOT_DIR        := $(addsuffix /uboot, $(call select_from_ports,zynq_uboot))
UBOOT_BUILD_DIR   = $(PWD)/$(PRG_REL_DIR)/$(BOARD)
UBOOT_CONFIG_FILE = $(BOARD)/.config
UBOOT_IMG_FILE    = $(BOARD)/u-boot.img
UBOOT_SPL_FILE    = $(BOARD)/spl/boot.bin
UBOOT_ENV_FILE    = $(BOARD)/env.txt

UBOOT_MAKE_ARGS = $(VERBOSE_DIR) -C $(UBOOT_DIR) \
                  CROSS_COMPILE=$(CROSS_DEV_PREFIX) \
                  DEVICE_TREE=${UBOOT_DEVICE_TREE(${BOARD})} \
                  O=$(UBOOT_BUILD_DIR)

$(UBOOT_ENV_FILE): $(REP_DIR)/src/u-boot/zynq/env.txt
	$(VERBOSE) mkdir -p $(BOARD)
	$(VERBOSE) cp $< $(BOARD)/env.txt

$(UBOOT_CONFIG_FILE):
	$(VERBOSE) mkdir -p $(BOARD)
	$(VERBOSE) cp $(UBOOT_DIR)/configs/${UBOOT_DEFCONFIG(${BOARD})} $(BOARD)/.config
	$(VERBOSE) ( \
		true; \
		$(foreach I, $(UBOOT_ENABLE_COMMON)  ${UBOOT_ENABLE(${BOARD})},  echo 'CONFIG_$I=y';) \
		$(foreach I, $(UBOOT_DISABLE_COMMON) ${UBOOT_DISABLE(${BOARD})}, echo 'CONFIG_$I=n';) \
		$(foreach I, $(UBOOT_OPTION_COMMON)  ${UBOOT_OPTION(${BOARD})},  echo 'CONFIG_$I';) \
	) >> $(BOARD)/.config
	$(VERBOSE) $(MAKE) $(UBOOT_MAKE_ARGS) olddefconfig

$(UBOOT_CONFIG_FILE): $(MAKEFILE_LIST)

$(UBOOT_IMG_FILE): $(UBOOT_CONFIG_FILE) $(UBOOT_ENV_FILE)
	$(VERBOSE) $(MAKE) $(UBOOT_MAKE_ARGS)


#
# SD-card image
#

SD_CARD_IMAGE_FILE = $(BOARD).img

$(SD_CARD_IMAGE_FILE) : $(UBOOT_IMG_FILE)

# note: empirically determined minimum size for FAT32 fs that mcopy can deal with
#       is 33MB
PART_OFFSET_MB    := 1
PART_SIZE_MB      := 33
IMG_SIZE_MB       := $(shell echo "$(PART_SIZE_MB)+$(PART_OFFSET_MB)" | bc -l)
PART_SIZE_BLOCK   := $(shell echo "$(PART_SIZE_MB)*1024" | bc -l )

# mkfs.fat introduced the --offset argument with version 4.2, yet this version
# is not shipped before Ubuntu 21.04. Until the major distributions are deploying
# this version, we use a workaround with dd instead.
$(SD_CARD_IMAGE_FILE):
	$(MSG_CONVERT)SD-card image $(PRG_REL_DIR)/$@
	$(VERBOSE) dd if=/dev/zero of=$@.incomplete bs=1M count=$(PART_OFFSET_MB) 2> /dev/null
	$(VERBOSE) dd if=/dev/zero of=$@.incomplete.fatfs bs=1M count=$(PART_SIZE_MB) 2> /dev/null
	$(VERBOSE) mkfs.fat -F32 -S512 -n 'U-BOOT' \
	                     $@.incomplete.fatfs $(PART_SIZE_BLOCK)
	$(VERBOSE) dd if=$@.incomplete.fatfs of=$@.incomplete bs=1M seek=$(PART_OFFSET_MB)
	$(VERBOSE) rm $@.incomplete.fatfs
	$(VERBOSE) mcopy -i $@.incomplete@@$(PART_OFFSET_MB)M $(UBOOT_SPL_FILE) $(UBOOT_IMG_FILE) ::

	$(VERBOSE) echo -e "n\np\n1\n2048\n\nn\nw" | fdisk -b512 $@.incomplete 2> /dev/null > /dev/null
	$(VERBOSE) mv $@.incomplete $@


# trigger build only for supported boards
ifneq ($(filter $(SUPPORTED_BOARDS), $(BOARD)),)
$(TARGET) : $(SD_CARD_IMAGE_FILE)
endif

