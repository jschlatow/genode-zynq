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
unsigned long genode_dev_addr(const char * name)
{
	return Ad::platform().addr_by_name(name);
}

extern "C"
void genode_gpio_direction(unsigned pin, bool input)
{
	Ad::platform().gpio.direction(pin, input);
}

extern "C"
void genode_gpio_write(unsigned pin, unsigned value)
{
	Ad::platform().gpio.set_output_pin(pin, value);
}
