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
	Heap                          heap            { env.ram(), env.rm() };

	Attached_rom_dataspace        config_rom      { env, "config" };
	Device                        device          { env, heap };
	Constructible<Uplink_client>  uplink_client   { };

	Signal_handler<Main>          config_handler  { env.ep(), *this, &Main::handle_config };
	Signal_handler<Main>          devices_handler { env.ep(), *this, &Main::handle_devices };
	Device::State                 state           { Device::State::STOPPED };

	Net::Mac_address mac_address()
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

		return mac_addr;
	}

	void update_state(Device::State new_state)
	{
		if (new_state == state)
			return;

		switch (new_state)
		{
			case Device::State::STOPPED:
				uplink_client.destruct();
				break;
			case Device::State::STARTED:
				uplink_client.construct(env, heap, device, mac_address());
				break;
		}

		state = new_state;
	}

	void handle_config()
	{
		config_rom.update();

		Device::State new_state = device.update_config(config_rom.xml());
		update_state(new_state);
	}

	void handle_devices()
	{
		device.platform().update();

		Device::State new_state = device.update_devices(config_rom.xml());
		update_state(new_state);
	}

	Main(Env &env) : env(env)
	{
		Device::State new_state = device.update_config(config_rom.xml());
		update_state(new_state);
		if (new_state != Device::State::STARTED)
			warning("waiting for devices to become available");

		config_rom.sigh(config_handler);
		device.platform().sigh(devices_handler);
	}
};


void Libc::Component::construct(Libc::Env &env) { static Rf::Main main(env); }
