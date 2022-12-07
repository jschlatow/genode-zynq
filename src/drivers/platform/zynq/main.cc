/*
 * \brief  Platform driver for Zynq
 * \author Johannes Schlatow
 * \date   2022-04-05
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/component.h>

#include <slcr.h>
#include <axidmac.h>
#include <common.h>

namespace Driver { struct Main; };

struct Driver::Main
{
	Env                  & _env;
	Attached_rom_dataspace _config_rom     { _env, "config"        };
	Common                 _common         { _env, _config_rom     };
	Signal_handler<Main>   _config_handler { _env.ep(), *this,
	                                         &Main::_handle_config };

	/**
	 * PS_CLK is either 33.33333Mhz or 50MHz. The latter is very rare though
	 * and practically not in use, hence we stick to 33.33333Mhz.
	 */
	Fixed_clock _ps_clk { _common.devices().clocks(), "ps_clk",
	                      Clock::Rate { 33333333 } };

	Slcr  _slcr { _env, _common.devices().clocks(), _common.devices().resets(), _ps_clk };

	Axidmac _axidmac { _env, _common.devices().powers() };

	void _handle_config();

	Main(Genode::Env & e)
	: _env(e)
	{
		_config_rom.sigh(_config_handler);
		_handle_config();
		_common.announce_service();
	}
};


void Driver::Main::_handle_config()
{
	_config_rom.update();
	_common.handle_config(_config_rom.xml());
}


void Component::construct(Genode::Env &env) {
	static Driver::Main main(env); }
