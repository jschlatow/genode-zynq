/*
 * \brief  Test case for dynamic reconfiguration of platform driver
 * \author Johannes Schlatow
 * \date   2022-05-23
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <base/log.h>
#include <platform_session/connection.h>
#include <platform_session/device.h>

using namespace Genode;

struct Main
{
	Env                         &_env;
	Platform::Connection         _platform { _env };
	Platform::Device             _device   { _platform, Platform::Device::Type { "dummy" }};


	Main(Genode::Env &env)
	: _env(env)
	{ log("Device acquired"); }
};

void Component::construct(Env &env)
{
	static Main main { env };
}
