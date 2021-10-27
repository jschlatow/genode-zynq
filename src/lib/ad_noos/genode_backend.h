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

/* C-interface for gpio */
extern void genode_gpio_direction(unsigned pin, bool input);
extern void genode_gpio_write(unsigned pin, unsigned value);

/* C-interfance for MMIO */
extern int32_t axi_io_write(uint32_t base, uint32_t offset, uint32_t value);
extern int32_t axi_io_read(uint32_t base, uint32_t offset, uint32_t * value);

/* C-interface for spi */
extern unsigned genode_spi_transfer(unsigned char *, unsigned);

#endif /* _SRC__LIB__AD_NOOS__GENODE_BACKEND_H_ */
