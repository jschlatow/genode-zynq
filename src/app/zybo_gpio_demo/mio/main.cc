/*
 * \brief  Component for controlling the MIO LED and buttons of the Zybo board
 * \author Johannes Schlatow
 * \date   2022-09-15
 */

/* Genode includes */
#include <base/component.h>
#include <base/attached_rom_dataspace.h>
#include <irq_session/connection.h>
#include <pin_state_session/connection.h>
#include <pin_control_session/connection.h>
#include <os/reporter.h>

namespace Demo {

	using namespace Genode;

	struct Main;
}


struct Demo::Main
{
	Env &_env;

	Pin_state::Connection   _btn4 { _env, "Btn4" };
	Pin_state::Connection   _btn5 { _env, "Btn5" };

	Pin_control::Connection _led4 { _env, "Led4" };

	Irq_connection _irq4 { _env, "Btn4" };
	Irq_connection _irq5 { _env, "Btn5" };

	Signal_handler<Main> _irq_handler {
		_env.ep(), *this, &Main::_handle_irq };

	Genode::Expanding_reporter _reporter { _env, "state", "state" };

	void _update_state(bool on)
	{
		_reporter.generate([&] (Genode::Xml_generator & xml) {
			xml.attribute("value", on ? "yes" : "no");
		});

		_led4.state(on);
	}

	void _handle_irq()
	{
		_irq4.ack_irq();
		_irq5.ack_irq();

		if (_btn4.state())
			_update_state(true);
		else if (_btn5.state())
			_update_state(false);
	}

	/*
	 * Configuration
	 */

	Attached_rom_dataspace _config { _env, "config" };

	Signal_handler<Main> _config_handler {
		_env.ep(), *this, &Main::_handle_config };

	void _handle_config()
	{
		_config.update();
	}

	Main(Env &env) : _env(env)
	{
		_config.sigh(_config_handler);
		_handle_config();
		_update_state(false);

		_irq4.sigh(_irq_handler);
		_irq5.sigh(_irq_handler);
		_irq4.ack_irq();
		_irq5.ack_irq();
	}
};


void Component::construct(Genode::Env &env)
{
	static Demo::Main main(env);
}

