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
#include <platform_session/volatile_device.h>
#include <drivers/dmac.h>

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

	protected:
		using Device = Platform::Device;
		using Type   = Platform::Device::Type;

		Genode::Env          &_env;
		Platform::Connection &_platform;
		State                _state      { STOPPED };

		Platform::Volatile_driver<Ad::Axi_dmac_rx, Type> _dmac_rx { _platform, Type { "rx_dmac" } };
		Platform::Volatile_driver<Ad::Axi_dmac_tx, Type> _dmac_tx { _platform, Type { "tx_dmac" } };

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
		 * Refresh device availability.
		 *
		 * If driver is started and devices are missing, driver will be stopped.
		 * If driver is stopped and devices are available, try starting the
		 * driver.
		 *
		 * Returns driver state
		 */
		State update_devices(Xml_node const & config);

		Ad::Axi_dmac_tx & tx() { return _dmac_tx.driver(); }
		Ad::Axi_dmac_rx & rx() { return _dmac_rx.driver(); }
};

#endif /* _INCLUDE__AD9361__AD9361_H_ */
