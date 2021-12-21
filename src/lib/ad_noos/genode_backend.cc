/*
 * \brief  C-interface for platform service
 * \author Johannes Schlatow
 * \date   2021-10-18
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/log.h>

#include "platform.h"
#include "error.h"

using namespace Genode;

Ad::Platform & Ad::platform(Genode::Env * env)
{
	static Ad::Platform platform(*env);
	return platform;
}

extern "C"
int32_t axi_io_write(uint32_t base, uint32_t offset, uint32_t value)
{
	uint32_t volatile * dst = (uint32_t volatile *)(base + offset);
	*dst = value;

	return SUCCESS;
}

extern "C"
int32_t axi_io_read(uint32_t base, uint32_t offset, uint32_t * value)
{
	uint32_t volatile * dst = (uint32_t volatile *)(base + offset);
	*value = *dst;

	return SUCCESS;
}

extern "C"
void genode_gpio_direction(unsigned pin, bool input)
{
	try {
		Ad::platform().gpio().direction(pin, input);
	} catch (...) {
		error("GPIO device access failed");
	}
}

extern "C"
void genode_gpio_write(unsigned pin, unsigned value)
{
	try {
		Ad::platform().gpio().set_output_pin(pin, value);
	} catch (...) {
		error("GPIO device access failed");
	}
}

extern "C"
unsigned genode_spi_transfer(unsigned char *buf, unsigned bytes)
{
	try {
		return Ad::platform().spi().write_and_read(buf, bytes);
	} catch (...) {
		error("SPI device access failed");
	}
	return 0;
}
