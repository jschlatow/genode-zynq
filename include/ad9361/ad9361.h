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

namespace Ad {
	using namespace Genode;
	using Device = Platform::Device;

	struct Ad9361_config;
	class Ad9361;
}

class Ad::Ad9361
{
	private:

	protected:
		Genode::Env          &_env;
		Platform::Connection &_platform;

		Device                _device_rx { _platform, Device::Type { "rx_dmac" } };
		Ad::Axi_dmac_rx       _dmac_rx   { _env, _platform, _device_rx, 1500 };
		Device                _device_tx { _platform, Device::Type { "tx_dmac" } };
		Ad::Axi_dmac_tx       _dmac_tx   { _env, _platform, _device_tx, 1500 };

		void _update_init_params(Ad9361_config &cfg, Xml_node const &);

	public:

		Ad9361(Genode::Env &env);

		void apply_config(Xml_node const & config);

		Ad::Axi_dmac_tx & tx() { return _dmac_tx; }
		Ad::Axi_dmac_rx & rx() { return _dmac_rx; }
};

#endif /* _INCLUDE__AD9361__AD9361_H_ */
