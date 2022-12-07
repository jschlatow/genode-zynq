/*
 * \brief  Driver for Zynq AXI DMAC
 * \author Johannes Schlatow
 * \date   2022-12-07
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _SRC__DRIVERS__PLATFORM__ZYNQ__AXIDMAC_H_
#define _SRC__DRIVERS__PLATFORM__ZYNQ__AXIDMAC_H_

#include <os/attached_mmio.h>
#include <power.h>

namespace Driver { struct Axidmac; }


struct Driver::Axidmac
{
	Genode::Env &_env;
	Powers      &_powers;

	struct Dmac_regs : Attached_mmio
	{
		Dmac_regs(Genode::Env &env, Genode::addr_t mmio_base)
		: Attached_mmio(env, mmio_base, 0x1000)
		{ }

		struct Control : Register<0x400, 32>
		{
			struct Enable : Bitfield<0,1> { };
		};
	};

	template <typename MMIO, typename REG, typename BITFIELD>
	struct Power_switch : Driver::Power
	{
		Genode::Env           &_env;
		Genode::addr_t         _mmio_base;
		typename REG::access_t _off_val;

		Power_switch(Genode::Env           &env,
		             Powers                &powers,
		             Driver::Power::Name    name,
		             Genode::addr_t         mmio_base,
		             typename REG::access_t off)
		:
		  Power(powers, name),
		  _env(env),
		  _mmio_base(mmio_base),
		  _off_val(off)
		{ }

		void _on() override { }

		void _off() override
		{
			MMIO regs { _env, _mmio_base };
			((Attached_mmio*)&regs)->write<BITFIELD>(_off_val);
		}
	};

	Power_switch<Dmac_regs, Dmac_regs::Control, Dmac_regs::Control::Enable>
		_dmac_rx { _env, _powers, "dmac_rx", 0x7c400000, 0 };

	Power_switch<Dmac_regs, Dmac_regs::Control, Dmac_regs::Control::Enable>
		_dmac_tx { _env, _powers, "dmac_tx", 0x7c420000, 0 };

	Axidmac(Genode::Env &env, Powers &powers)
	:
		_env(env), _powers(powers)
	{ }
};

#endif /* _SRC__DRIVERS__PLATFORM__ZYNQ__AXIDMAC_H_ */
