/*
 * \brief  Component for controlling the RGB LEDs of the Zybo board
 * \author Johannes Schlatow
 * \date   2022-09-15
 */

/* Genode includes */
#include <base/component.h>
#include <base/attached_rom_dataspace.h>
#include <irq_session/connection.h>
#include <pin_state_session/connection.h>
#include <pin_control_session/connection.h>
#include <platform_session/connection.h>
#include <platform_session/device.h>
#include <util/register.h>

namespace Demo {

	using namespace Genode;

	template <unsigned, unsigned, unsigned, unsigned>
	struct Value_cycle;
	struct Color_cycle;
	struct Rgb_leds;
	struct Main;
}


template <unsigned MAX, unsigned MIN, unsigned INCREMENT=1, unsigned START=MIN>
struct Demo::Value_cycle
{
	unsigned _value { START };

	unsigned next_value()
	{
		if (_value < MAX)
			_value += INCREMENT;
		else
			_value = MIN;

		return _value;
	}
};


struct Demo::Color_cycle
{
	uint32_t _colors[7] = {
		0xff0000, /* red   */
		0x00ff00, /* green */
		0x0000ff, /* blue */
		0xffff00, /* yellow */
		0xff00ff, /* purple */
		0x00ffff, /* cyan */
		0xff2000, /* orange */
	};

	Value_cycle<6, 0> _cycle;

	uint32_t next_color() {
		return _colors[_cycle.next_value()]; }
};


class Demo::Rgb_leds : public Platform::Device::Mmio
{
	private:

		struct Led : Genode::Mmio {
			struct Color      : Register<0x0, 32> { };
			struct Brightness : Register<0x4, 32> { };

			void color(uint32_t c) {
				write<Color>(c); }

			void brightness(uint32_t b) {
				write<Brightness>(b); }
		};

		Led led0 { (addr_t)local_addr<void>() };
		Led led1 { (addr_t)local_addr<void>() + 0x8 };

	public:

		Rgb_leds(Platform::Device &device)
		: Platform::Device::Mmio(device)
		{ }

		void led0_color(uint32_t c) {
			led0.color(c); }

		void led0_brightness(uint32_t b) {
			led0.brightness(b); }

		void led1_color(uint32_t c) {
			led1.color(c); }

		void led1_brightness(uint32_t b) {
			led1.brightness(b); }
};


struct Demo::Main
{
	Env &_env;

	Pin_state::Connection   _btn0 { _env, "Btn0" };
	Pin_state::Connection   _btn1 { _env, "Btn1" };
	Pin_state::Connection   _btn2 { _env, "Btn2" };
	Pin_state::Connection   _btn3 { _env, "Btn3" };

	Irq_connection _irq0 { _env, "Btn0" };
	Irq_connection _irq1 { _env, "Btn1" };
	Irq_connection _irq2 { _env, "Btn2" };
	Irq_connection _irq3 { _env, "Btn3" };

	using Brightness_cycle = Value_cycle<0x19, 0x0, 1, 0x5>;
	Brightness_cycle _led5_brightness_cycle { };
	Color_cycle      _led5_color_cycle      { };
	Brightness_cycle _led6_brightness_cycle { };
	Color_cycle      _led6_color_cycle      { };

	Platform::Connection _platform       { _env };
	Platform::Device     _rgbleds_device { _platform };
	Rgb_leds             _rgbleds        { _rgbleds_device };

	Signal_handler<Main> _irq_handler {
		_env.ep(), *this, &Main::_handle_irq };

	void _handle_irq()
	{
		_irq0.ack_irq();
		_irq1.ack_irq();
		_irq2.ack_irq();
		_irq3.ack_irq();

		if (_btn0.state())
			_rgbleds.led0_color(_led5_color_cycle.next_color());

		if (_btn1.state())
			_rgbleds.led0_brightness(_led5_brightness_cycle.next_value());

		if (_btn2.state())
			_rgbleds.led1_color(_led6_color_cycle.next_color());

		if (_btn3.state())
			_rgbleds.led1_brightness(_led6_brightness_cycle.next_value());
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
		log("Hello");
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

