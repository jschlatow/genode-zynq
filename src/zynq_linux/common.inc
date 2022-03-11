#
# Linux kernel configuration (essentials)
#
# bare minimum to boot the hardware into a shell
#

# kernel fundamentals
LX_ENABLE += TTY SERIAL_CORE SERIAL_CORE_CONSOLE PRINTK HAS_IOMEM

# target fundamentals
LX_ENABLE += MMU CPU_V7 ARCH_ZYNQ SERIAL_XILINX_PS_UART SERIAL_XILINX_PS_UART_CONSOLE SERIAL_EARLYCON

# ARM errata
LX_ENABLE += PL310_ERRATA_588369 \
             PL310_ERRATA_727915 \
             PL310_ERRATA_753970 \
             PL310_ERRATA_769419 \
             ARM_ERRATA_643719 \
             ARM_ERRATA_720789 \
             ARM_ERRATA_754322 \
             ARM_ERRATA_754327 \
             ARM_ERRATA_764369 \
             ARM_ERRATA_775420

# block device infrastructure
LX_ENABLE += BLOCK MMC MMC_BLOCK MMC_SDHCI MMC_SDHCI_PLTFM MMC_SDHCI_OF_ARASAN

# file systems
LX_ENABLE += MSDOS_FS FAT_FS EXT3_FS EXT4_FS

# network infrastructure
LX_ENABLE += NET NETDEVICES MACB

# ethernet PHY
LX_ENABLE += MARVELL_PHY

# network protocols
LX_ENABLE += INET IP_PNP IP_PNP_DHCP PACKET FILTER


# not strictly necessary but convenient
LX_ENABLE += CMDLINE_FROM_BOOTLOADER IKCONFIG IKCONFIG_PROC

# disable network drivers
LX_DISABLE += $(addprefix NET_VENDOR_, ALACRITECH AMAZON AQUANTIA ARC \
                                       BROADCOM CAVIUM CIRRUS CORTINA EZCHIP \
                                       FARADAY GOOGLE HISILICON HUAWEI I825XX \
                                       INTEL MARVELL MICROSOFT MICREL \
                                       MICROCHIP MICROSEMI NATSEMI \
                                       NETRONOME NI 8390 PENSANDO QUALCOMM \
                                       RENESAS ROCKER SAMSUNG SOLARFLARE \
                                       SMSC SOCIONEXT STMICRO SYNOPSYS VIA \
                                       WIZNET XILINX)

LX_DISABLE += WLAN NETWORK_FILESYSTEMS WIRELESS IPV6

# no iommu
LX_DISABLE += IOMMU_SUPPORT

# no thumb kernel and virt extension
LX_DISABLE += THUMB2_KERNEL ARM_VIRT_EXT
