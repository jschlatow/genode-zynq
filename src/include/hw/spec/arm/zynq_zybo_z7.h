/*
 * \brief  Base driver for Parallella Board
 * \author Johannes Schlatow
 * \date   2021-08-19
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _SRC__INCLUDE__HW__SPEC__ARM__ZYNQ_ZYBO_Z7_H_
#define _SRC__INCLUDE__HW__SPEC__ARM__ZYNQ_ZYBO_Z7_H_

#include <hw/spec/arm/zynq.h>

namespace Zynq_zybo_z7 {

	using namespace Zynq;

	enum {
		/* clocks (assuming 6:2:1 mode) with
		 *   - 33.33333Mhz PS_CLK
		 *   - PLL = 40 * PS_CLK
		 *   - CPU_6x4x = PLL / 2
		 */
		CPU_1X_CLOCK   = 111111100,
		CPU_3X2X_CLOCK = 3*CPU_1X_CLOCK,
		CPU_6X4X_CLOCK = 6*CPU_1X_CLOCK,

		UART_CLOCK = 100*1000*1000,
		UART_BASE  = UART_1_MMIO_BASE,

		RAM_0_SIZE = 0x3FF00000, /* 1GiB */

		CORTEX_A9_GLOBAL_TIMER_CLK = CPU_3X2X_CLOCK,
		CORTEX_A9_GLOBAL_TIMER_DIV = 100,
	};
};

#endif /* _SRC__INCLUDE__HW__SPEC__ARM__ZYNQ_ZYBO_Z7_H_ */
