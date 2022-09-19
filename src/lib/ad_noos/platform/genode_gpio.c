/*
 * \brief  no-OS GPIO backend
 * \author Johannes Schlatow
 * \date   2021-10-18
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* libc includes */
#include <stdint.h>
#include <stdio.h>

/* ad no-OS includes */
#include "error.h"
#include "util.h"
#include "gpio.h"

/* local includes */
#include <genode_backend.h>

#define TRACE printf("\033[32m%s\033[0m called from %p, not implemented\n", __PRETTY_FUNCTION__, __builtin_return_address(0))

/**
 * @brief Obtain the GPIO decriptor.
 * @param desc - The GPIO descriptor.
 * @param param - GPIO Initialization parameters.
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t genode_gpio_get(      struct gpio_desc      **desc,
                        const struct gpio_init_param *param)
{
	static gpio_desc _desc;
	_desc.number = param->number;

	*desc = &_desc;
	return SUCCESS;
}

/**
 * @brief Get the value of an optional GPIO.
 * @param desc - The GPIO descriptor.
 * @param param - GPIO Initialization parameters.
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t genode_gpio_get_optional(      struct gpio_desc      **desc,
                                 const struct gpio_init_param *param)
{
	if(param == NULL) {
		*desc = NULL;
		return SUCCESS;
	}

	return gpio_get(desc, param);
}

/**
 * @brief Free the resources allocated by gpio_get().
 * @param desc - The SPI descriptor.
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t genode_gpio_remove(struct gpio_desc *desc)
{
	TRACE;
	return FAILURE;
}

/**
 * @brief Enable the input direction of the specified GPIO.
 * @param desc - The GPIO descriptor.
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t genode_gpio_direction_input(struct gpio_desc *desc)
{
	TRACE;

	return FAILURE;
}

/**
 * @brief Enable the output direction of the specified GPIO.
 * @param desc - The GPIO descriptor.
 * @param value - The value.
 *                Example: GPIO_HIGH
 *                         GPIO_LOW
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t genode_gpio_direction_output(struct gpio_desc *desc,
                                     uint8_t           value)
{
	genode_gpio_direction(desc->number, false);
	return SUCCESS;
}

/**
 * @brief Get the direction of the specified GPIO.
 * @param desc - The GPIO descriptor.
 * @param direction - The direction.
 *                    Example: GPIO_OUT
 *                             GPIO_IN
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t genode_gpio_get_direction(struct gpio_desc *desc,
                                  uint8_t          *direction)
{
	TRACE;
	return FAILURE;
}

/**
 * @brief Set the value of the specified GPIO.
 * @param desc - The GPIO descriptor.
 * @param value - The value taken from the enum gpio_values members.
 *                Example: GPIO_HIGH
 *                         GPIO_LOW
 *                         GPIO_HIGH_Z
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t genode_gpio_set_value(struct gpio_desc *desc,
                              uint8_t           value)
{
	genode_gpio_write(desc->number, value);
	return SUCCESS;
}

/**
 * @brief Get the value of the specified GPIO.
 * @param desc - The GPIO descriptor.
 * @param value - The value.
 *                Example: GPIO_HIGH
 *                         GPIO_LOW
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t genode_gpio_get_value(struct gpio_desc *desc,
                              uint8_t          *value)
{
	TRACE;
	return FAILURE;
}

/**
 * @brief Generic platform GPIO ops
 */
const struct gpio_platform_ops genode_gpio_ops = {
	.gpio_ops_get = &genode_gpio_get,
	.gpio_ops_get_optional = &genode_gpio_get_optional,
	.gpio_ops_remove = &genode_gpio_remove,
	.gpio_ops_direction_input = &genode_gpio_direction_input,
	.gpio_ops_direction_output = &genode_gpio_direction_output,
	.gpio_ops_get_direction = &genode_gpio_get_direction,
	.gpio_ops_set_value = &genode_gpio_set_value,
	.gpio_ops_get_value = &genode_gpio_get_value,
};
