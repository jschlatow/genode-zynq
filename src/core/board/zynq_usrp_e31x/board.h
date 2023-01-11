/*
 * \brief  Board driver for core on Zynq
 * \author Johannes Schlatow
 * \author Stefan Kalkowski
 * \author Martin Stein
 * \date   2014-06-02
 */

/*
 * Copyright (C) 2014-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__BOARD__ZYNQ_USRP_E31x__BOARD_H_
#define _CORE__BOARD__ZYNQ_USRP_E31x__BOARD_H_

/* base-hw internal includes */
#include <hw/spec/arm/gicv2.h>
#include <hw/spec/arm/zynq_usrp_e31x_board.h>

/* base-hw Core includes */
#include <spec/arm/cortex_a9_global_timer.h>
#include <spec/cortex_a9/cpu.h>
#include <hw/spec/arm/pl310.h>

namespace Board {
	using namespace Hw::Zynq_usrp_e31x_board;
	using L2_cache = Hw::Pl310;

	class Global_interrupt_controller { };
	class Pic : public Hw::Gicv2 { public: Pic(Global_interrupt_controller &) { } };

	L2_cache & l2_cache();
}

#endif /* _CORE__BOARD__ZYNQ_USRP_E31x__BOARD_H_ */
