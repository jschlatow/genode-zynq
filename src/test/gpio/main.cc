/*
 * \brief  Test for Zynq GPIO driver
 * \author Johannes Schlatow
 * \date   2021-10-19
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <timer_session/connection.h>
#include <platform_session/connection.h>
#include <platform_session/device.h>

#include <drivers/gpio.h>

using namespace Genode;
using Type = Platform::Device::Type;

struct Main
{
	Env                 &env;
	Timer::Connection    timer     { env };
	Platform::Connection platform  { env };
	Platform::Device     device    { platform, Type { "zynq-gpio" } };
	Gpio::Zynq_regs      gpio      { device };

	Main(Env &env) : env(env)
	{
		log("--- Zynq GPIO test started ---");

		enum {
			LED1 = 0x6d,
			LED2 = 0x6b,
			LED3 = 0x69,
			LED4 = 0x68
		};

		gpio.direction(LED1, false);
		gpio.direction(LED2, false);
		gpio.direction(LED3, false);
		gpio.direction(LED4, false);

		for (char i = 1; ; ++i) {
			gpio.set_output_pin(LED1, true);
			gpio.set_output_pin(LED2, true);
			gpio.set_output_pin(LED3, true);
			gpio.set_output_pin(LED4, true);
			timer.msleep(2000);
			gpio.set_output_pin(LED1, false);
			gpio.set_output_pin(LED2, false);
			gpio.set_output_pin(LED3, false);
			gpio.set_output_pin(LED4, false);
			timer.msleep(2000);
		}

	}
};

void Component::construct(Env &env) { static Main main(env); }
