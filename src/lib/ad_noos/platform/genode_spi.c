/*
 * \brief  no-OS SPI backend
 * \author Johannes Schlatow
 * \date   2021-10-18
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <stdio.h>
#include "error.h"
#include "spi.h"
#include "util.h"

#include "genode_backend.h"

#define TRACE printf("\033[32m%s\033[0m called from %p, not implemented\n", __PRETTY_FUNCTION__, __builtin_return_address(0))


const struct spi_platform_ops genode_spi_ops;

/**
 * @brief Initialize the SPI communication peripheral.
 * @param desc - The SPI descriptor.
 * @param param - The structure that contains the SPI parameters.
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t genode_spi_init(      struct spi_desc      **desc,
                        const struct spi_init_param *param)
{
	static spi_desc _desc;
	_desc.device_id     = param->device_id;
	_desc.max_speed_hz  = param->max_speed_hz;
	_desc.chip_select   = param->chip_select;
	_desc.mode          = param->mode;
	_desc.bit_order     = param->bit_order;

	*desc = &_desc;
	return SUCCESS;
}

/**
 * @brief Free the resources allocated by spi_init().
 * @param desc - The SPI descriptor.
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t genode_spi_remove(struct spi_desc *desc)
{
	TRACE;
	return FAILURE;
}

/**
 * @brief Write and read data to/from SPI.
 * @param desc - The SPI descriptor.
 * @param data - The buffer with the transmitted/received data.
 * @param bytes_number - Number of bytes to write/read.
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t genode_spi_write_and_read(struct spi_desc *desc,
                                  uint8_t         *data,
                                  uint16_t         bytes_number)
{
	if (genode_spi_transfer(data, bytes_number) != bytes_number)
		return FAILURE;

	return SUCCESS;
}

/**
 * @brief Generic platform SPI ops
 */
const struct spi_platform_ops genode_spi_ops = {
	.init = &genode_spi_init,
	.write_and_read = &genode_spi_write_and_read,
	.remove = &genode_spi_remove
};
