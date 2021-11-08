/*
 * \brief  Main component
 * \author Johannes Schlatow
 * \date   2021-11-04
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/attached_rom_dataspace.h>
#include <libc/component.h>
#include <base/heap.h>

/* local includes */
#include "device.h"
#include "uplink_client.h"

namespace Rf {
	using namespace Genode;

	struct Main;
}

struct Rf::Main
{
	Env                          &env;
	Heap                          heap          { env.ram(), env.rm() };

	Attached_rom_dataspace        config_rom    { env, "config" };
	Device                        device        { env, heap, config_rom };
	Constructible<Uplink_client>  uplink_client { };

	Main(Env &env) : env(env)
	{
		/* read MAC address from config */
		Net::Mac_address mac_addr { };
		try {
			Genode::Xml_node nic_config = config_rom.xml();
			nic_config.attribute("mac").value(mac_addr);
		} catch (...) {
			error("No MAC address provided.");
			env.parent().exit(-1);
		}

		uplink_client.construct(env, heap, device, mac_addr);
	}
};


void Libc::Component::construct(Libc::Env &env) { static Rf::Main main(env); }
