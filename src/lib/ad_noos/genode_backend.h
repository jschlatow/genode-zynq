/*
 * \brief  C-backend for no-OS contrib code
 * \author Johannes Schlatow
 * \date   2021-10-18
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _SRC__LIB__AD_NOOS__GENODE_BACKEND_H_
#define _SRC__LIB__AD_NOOS__GENODE_BACKEND_H_

#include <base/fixed_stdint.h>

/* dummy defines, addresses must be set using genode_dev_addr() */
#define RX_CORE_BASEADDR          0
#define TX_CORE_BASEADDR          0
#define CF_AD9361_RX_DMA_BASEADDR 0
#define CF_AD9361_TX_DMA_BASEADDR 0

#define GPIO_RESET_PIN            100
#define SPI_DEVICE_ID             0
#define SPI_CS                    0

/* C-interface for getting dev addresses from platform session */
extern unsigned long genode_dev_addr(const char * name);

/* C-interface for gpio */
extern void genode_gpio_direction(unsigned pin, bool input);
extern void genode_gpio_write(unsigned pin, unsigned value);

/* C-interfance for MMIO */
extern int32_t axi_io_write(uint32_t base, uint32_t offset, uint32_t value);
extern int32_t axi_io_read(uint32_t base, uint32_t offset, uint32_t * value);

/* C-interface for spi */
extern unsigned genode_spi_transfer(unsigned char *, unsigned);

/* external symbols from ad9361.lib */
extern const struct gpio_platform_ops genode_gpio_ops;
extern const struct spi_platform_ops  genode_spi_ops;

#endif /* _SRC__LIB__AD_NOOS__GENODE_BACKEND_H_ */
