/*
 * \brief  Zynq GPIO driver
 * \author Johannes Schlatow
 * \date   2021-10-19
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__DRIVERS__GPIO_H_
#define _INCLUDE__DRIVERS__GPIO_H_

#include <platform_session/device.h>

namespace Gpio {
	using namespace Genode;

	class Zynq_regs;
}

class Gpio::Zynq_regs : public Platform::Device::Mmio
{
	private:

		enum {
			INPUT = 0,
			OUTPUT = 1
		};

		/*
		 * Registers
		 */

		/* output data register bank 0-3 */
		struct Out0 : Register_array<0x040, 32, 32, 1> { };
		struct Out1 : Register_array<0x044, 32, 22, 1> { };
		struct Out2 : Register_array<0x048, 32, 32, 1> { };
		struct Out3 : Register_array<0x04c, 32, 32, 1> { };

		/* input data register bank 0-3 */
		struct In0 : Register_array<0x060, 32, 32, 1> { };
		struct In1 : Register_array<0x064, 32, 22, 1> { };
		struct In2 : Register_array<0x068, 32, 32, 1> { };
		struct In3 : Register_array<0x06c, 32, 32, 1> { };

		/* direction registers bank 0-3 */
		struct Dirm0 : Register_array<0x204, 32, 32, 1> { };
		struct Dirm1 : Register_array<0x244, 32, 22, 1> { };
		struct Dirm2 : Register_array<0x284, 32, 32, 1> { };
		struct Dirm3 : Register_array<0x2C4, 32, 32, 1> { };

		/* output enable registers bank 0-3 */
		struct Oen0 : Register_array<0x208, 32, 32, 1> { };
		struct Oen1 : Register_array<0x248, 32, 22, 1> { };
		struct Oen2 : Register_array<0x288, 32, 32, 1> { };
		struct Oen3 : Register_array<0x2C8, 32, 32, 1> { };

	public:

		Zynq_regs(Platform::Device &device)
		: Platform::Device::Mmio(device)
		{ }

		void direction(unsigned pin, bool input)
		{
			if (pin < 32) {
				if (input)
					write<Dirm0>(INPUT,  pin);
				else {
					write<Dirm0>(OUTPUT, pin);
					write<Oen0> (1,      pin);
				}
			} else if (pin < 54) {
				if (input)
					write<Dirm1>(INPUT,  pin-32);
				else {
					write<Dirm1>(OUTPUT, pin-32);
					write<Oen1> (1,      pin-32);
				}
			} else if (pin < 86) {
				if (input)
					write<Dirm2>(INPUT,  pin-54);
				else {
					write<Dirm2>(OUTPUT, pin-54);
					write<Oen2> (1,      pin-54);
				}
			} else {
				if (input)
					write<Dirm3>(INPUT,  pin-86);
				else {
					write<Dirm3>(OUTPUT, pin-86);
					write<Oen3> (1,      pin-86);
				}
			}
		}

		void set_output_pin(unsigned pin, bool level)
		{
			if      (pin < 32)  write<Out0>(level, pin);
			else if (pin < 54)  write<Out1>(level, pin-32);
			else if (pin < 86)  write<Out2>(level, pin-54);
			else                write<Out3>(level, pin-86);
		}

		bool get_input_pin(unsigned pin)
		{
			if      (pin < 32)  return read<In0>(pin);
			else if (pin < 54)  return read<In1>(pin-32);
			else if (pin < 86)  return read<In2>(pin-54);
			else                return read<In3>(pin-86);
		}
};

#endif /* _INCLUDE__DRIVERS__GPIO_H_ */
