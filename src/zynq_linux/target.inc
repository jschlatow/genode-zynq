#
# Linux kernel configuration
#

# kernel fundamentals
LX_ENABLE += TTY SERIAL_EARLYCON SERIAL_OF_PLATFORM PRINTK HAS_IOMEM

# target fundamentals
LX_ENABLE += MMU CPU_V7 ARCH_ZYNQ SERIAL_XILINX_PS_UART

# ARM errata
LX_ENABLE += PL310_ERRATA_588369 \
             PL310_ERRATA_72791 \
             PL310_ERRATA_753970 \
             PL310_ERRATA_769419 \
             ARM_ERRATA_643719 \
             ARM_ERRATA_720789 \
             ARM_ERRATA_754322 \
             ARM_ERRATA_754327 \
             ARM_ERRATA_764369 \
             ARM_ERRATA_775420

# subsystems
LX_ENABLE += NET NETDEVICES MACB MMC MMC_SDHCI MMC_SDHCI_PLTFM MMC_SDHCI_OF_ARASAN


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

LX_DISABLE += WLAN
