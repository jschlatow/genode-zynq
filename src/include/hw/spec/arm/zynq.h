/*
 * \brief  MMIO and IRQ definitions common to Xilinx Zynq platforms
 * \author Mark Albers
 * \author Timo Wischer
 * \author Johannes Schlatow
 * \date   2014-12-15
 */

/*
 * Copyright (C) 2014-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _SRC__INCLUDE__HW__SPEC__ARM__ZYNQ_H_
#define _SRC__INCLUDE__HW__SPEC__ARM__ZYNQ_H_

#include <hw/spec/arm/cortex_a9.h>
#include <hw/spec/arm/pl310.h>

namespace Zynq {
	enum {
		/* device IO memory */
		MMIO_0_BASE    = 0xe0000000, /* IOP devices */
		MMIO_0_SIZE    = 0x10000000,

		/* normal RAM */
		RAM_0_BASE = 0x00000000,

		/* UART controllers */
		UART_0_MMIO_BASE = MMIO_0_BASE,
		UART_1_MMIO_BASE = MMIO_0_BASE + 0x1000,
		UART_SIZE        = 0x1000,

		/* CPU */
		CORTEX_A9_PRIVATE_MEM_BASE  = 0xf8f00000,
		CORTEX_A9_PRIVATE_MEM_SIZE  = 0x00002000,

		/* entrypoint address of secondary cpu */
		CORE1_ENTRY = 0xfffffff0,

		/* CPU cache */
		PL310_MMIO_BASE      = 0xF8F02000,
		PL310_MMIO_SIZE      = 0x1000,
	};

	using Cpu_mmio = Hw::Cortex_a9_mmio<CORTEX_A9_PRIVATE_MEM_BASE>;

	struct L2_cache;
};

struct Zynq::L2_cache : Hw::Pl310
{
	L2_cache(Genode::addr_t mmio) : Hw::Pl310(mmio)
	{
		Aux::access_t aux = 0;
		Aux::Full_line_of_zero::set(aux, true);
		Aux::Force_write_alloc::set(aux, Aux::Force_write_alloc::NO_ALLOC);
		Aux::Associativity::set(aux, Aux::Associativity::WAY_8);
		Aux::Way_size::set(aux, Aux::Way_size::KB_64);
		Aux::Share_override::set(aux, true);
		Aux::Replacement_policy::set(aux, Aux::Replacement_policy::PRAND);
		Aux::Ns_lockdown::set(aux, true);
		Aux::Data_prefetch::set(aux, true);
		Aux::Inst_prefetch::set(aux, true);
		Aux::Early_bresp::set(aux, true);
		write<Aux>(aux);

		Prefetch_ctrl::access_t prefetch = 0;
		Prefetch_ctrl::Data_prefetch::set(prefetch, 1);
		Prefetch_ctrl::Inst_prefetch::set(prefetch, 1);
		Prefetch_ctrl::Double_linefill::set(prefetch, true);
		write<Prefetch_ctrl>(prefetch | 0x10);

		write<Tag_ram>(Tag_ram::Setup_latency::bits(1)
		             | Tag_ram::Write_latency::bits(1)
		             | Tag_ram::Read_latency::bits(1));

		write<Data_ram>(Tag_ram::Setup_latency::bits(1)
		              | Tag_ram::Write_latency::bits(1)
		              | Tag_ram::Read_latency::bits(2));
	}

	using Hw::Pl310::invalidate;

	void enable()
	{
		Pl310::mask_interrupts();
		write<Control::Enable>(1);
	}

	void disable() {
		write<Control::Enable>(0);
	}
};

#endif /* _SRC__INCLUDE__HW__SPEC__ARM__ZYNQ_H_ */
