/*
 * \brief  Component for testing the on-board LEDs and Buttons of the Zybo board
 * \author Johannes Schlatow
 * \date   2022-09-09
 */

/* Genode includes */
#include <base/component.h>
#include <base/attached_rom_dataspace.h>
#include <irq_session/connection.h>
#include <pin_state_session/connection.h>
#include <pin_control_session/connection.h>

namespace Test {

	using namespace Genode;

	struct Main;
}


struct Test::Main
{
	Env &_env;

	Pin_state::Connection   _btn4 { _env, "Btn4" };
	Pin_state::Connection   _btn5 { _env, "Btn5" };
	Pin_control::Connection _led4 { _env, "Led4" };

	Irq_connection _irq4 { _env, "Btn4" };
	Irq_connection _irq5 { _env, "Btn5" };

	Signal_handler<Main> _irq_handler {
		_env.ep(), *this, &Main::_handle_irq };

	void _handle_irq()
	{
		_irq4.ack_irq();
		_irq5.ack_irq();

		if (_btn4.state()) {
			log("LED4 ON");
			_led4.state(1);
		} else if (_btn5.state()) {
			log("LED4 OFF");
			_led4.state(0);
		}
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

		_irq4.sigh(_irq_handler);
		_irq5.sigh(_irq_handler);
		_irq4.ack_irq();
		_irq5.ack_irq();
	}
};


void Component::construct(Genode::Env &env)
{
	static Test::Main main(env);
}

