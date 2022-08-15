/*
 * \brief  Driver for ad9361 subsystem based on AD's no-OS drivers
 * \author Johannes Schlatow
 * \date   2021-10-26
 *
 * Uses AD's no-OS drivers for initialisation and provides a Genode-style
 * API for Tx and Rx.
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__AD9361__AD9361_H_
#define _INCLUDE__AD9361__AD9361_H_

#include <util/xml_node.h>
#include <platform_session/device.h>
#include <drivers/dmac.h>
#include <drivers/gpio.h>

namespace Ad {
	using namespace Genode;

	struct Ad9361_config;
	class Ad9361;
}

class Ad::Ad9361
{
	public:

		enum State {
			STOPPED,
			STARTED
		};

		enum Loopback_mode {
			NONE,             /* no loopback */
			TXRX,             /* FGPA-internal TX -> RX loopback */
			RXTX,             /* FPGA-internal RX -> TX loopback */
			RF                /* TX -> RX loopback on rf side */
		};

	protected:
		using Device = Platform::Device;
		using Type   = Platform::Device::Type;

		Genode::Env          &_env;
		Platform::Connection &_platform;
		State                _state      { STOPPED };

		Platform::Device     _device_rx { _platform, Type { "rx_dmac" } };
		Platform::Device     _device_tx { _platform, Type { "tx_dmac" } };

		Constructible<Ad::Axi_dmac_rx> _dmac_rx { };
		Constructible<Ad::Axi_dmac_tx> _dmac_tx { };

		void _update_init_params(Xml_node const &);

		/* accessor to static Ad9361_config object to hide implementation */
		Ad9361_config &_ad9361_config();

		void _restart_driver(Xml_node const & config);

	public:

		Ad9361(Genode::Env &env);

		/**
		 * (Re)start driver, acquiring devices and applying the provided config.
		 *
		 * Returns driver state
		 */
		State update_config(Xml_node const & config);

		/**
		 * Allocate and initialise RX/TX buffers.
		 */
		void allocate_buffers(size_t rx_bytes, size_t tx_bytes);

		Ad::Axi_dmac_tx & tx() { return *_dmac_tx; }
		Ad::Axi_dmac_rx & rx() { return *_dmac_rx; }

		void rx_config(unsigned bw_hz, unsigned fs_hz, unsigned lo_hz);
		void tx_config(unsigned bw_hz, unsigned fs_hz, unsigned lo_hz);

		void rx_gain(const char *gain, unsigned ch);

		void loopback_mode(Loopback_mode mode);

		Gpio::Zynq_regs &gpio();
};

#endif /* _INCLUDE__AD9361__AD9361_H_ */
