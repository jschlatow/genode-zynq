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
#include <base/component.h>
#include <base/heap.h>

/* NIC driver includes */
#include <drivers/nic/mode.h>

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

	Device                        device        { env, heap };
	Uplink_client                 uplink_client { env, heap, device, device.mac() };

	Main(Env &env) : env(env)
	{ }
};


void Component::construct(Genode::Env &env) { static Rf::Main main(env); }
