/*
 * \brief  Base driver for the Zynq (QEMU)
 * \author Johannes Schlatow
 * \date   2015-06-30
 */

/*
 * Copyright (C) 2015-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _SRC__INCLUDE__HW__SPEC__ARM__ZYNQ_QEMU_H_
#define _SRC__INCLUDE__HW__SPEC__ARM__ZYNQ_QEMU_H_

#include <hw/spec/arm/zynq.h>

namespace Zynq_qemu {

	using namespace Zynq;

	enum {
		RAM_0_SIZE = 0x40000000, /* 1GiB */

		UART_CLOCK = 50*1000*1000,
		UART_BASE  = UART_0_MMIO_BASE,

		CORTEX_A9_GLOBAL_TIMER_CLK = 100000000,
		CORTEX_A9_GLOBAL_TIMER_DIV = 100,
	};
};

#endif /* _SRC__INCLUDE__HW__SPEC__ARM__ZYNQ_QEMU_H_ */
