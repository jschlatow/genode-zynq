/*
 * \brief  Component for testing the PL-controlled switches and LEDs
 * \author Johannes Schlatow
 * \date   2022-09-15
 */

/* Genode includes */
#include <base/component.h>
#include <base/attached_rom_dataspace.h>
#include <irq_session/connection.h>
#include <pin_state_session/connection.h>
#include <pin_control_session/connection.h>

namespace Demo {

	using namespace Genode;

	struct Main;
}


struct Demo::Main
{
	Env &_env;

	Pin_state::Connection   _sw0  { _env, "Sw0" };
	Pin_state::Connection   _sw1  { _env, "Sw1" };
	Pin_state::Connection   _sw2  { _env, "Sw2" };
	Pin_state::Connection   _sw3  { _env, "Sw3" };

	Pin_control::Connection _led0 { _env, "Led0" };
	Pin_control::Connection _led1 { _env, "Led1" };
	Pin_control::Connection _led2 { _env, "Led2" };
	Pin_control::Connection _led3 { _env, "Led3" };

	Irq_connection _irq0 { _env, "Sw0" };
	Irq_connection _irq1 { _env, "Sw1" };
	Irq_connection _irq2 { _env, "Sw2" };
	Irq_connection _irq3 { _env, "Sw3" };

	Signal_handler<Main> _irq_handler {
		_env.ep(), *this, &Main::_handle_irq };

	void _handle_irq()
	{
		_irq0.ack_irq();
		_irq1.ack_irq();
		_irq2.ack_irq();
		_irq3.ack_irq();

		_led0.state(_sw0.state());
		_led1.state(_sw1.state());
		_led2.state(_sw2.state());
		_led3.state(_sw3.state());
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

		_irq0.sigh(_irq_handler);
		_irq1.sigh(_irq_handler);
		_irq2.sigh(_irq_handler);
		_irq3.sigh(_irq_handler);
		_irq0.ack_irq();
		_irq1.ack_irq();
		_irq2.ack_irq();
		_irq3.ack_irq();
	}
};


void Component::construct(Genode::Env &env)
{
	static Demo::Main main(env);
}

