/* Genode includes */
#include <libc/component.h>
#include <util/reconstructible.h>
#include <util/string.h>
#include <timer_session/connection.h>
#include <base/attached_rom_dataspace.h>

/* local includes */
#include "api.h"

namespace Heartbeat_test {
	using namespace Genode;

	struct Main;
};


struct Heartbeat_test::Main
{
	Env                           &env;

	Attached_rom_dataspace         config  { env, "config" };

	Constructible<Api>             api     { };

	Timer::Connection              timer { env };

	unsigned                       heartbeat_id { 0 };

	/**
	 * cannot use Timer::Periodic_timeout here because it uses Io_signal_handler
	 * and therefore gets processed by libc on blocking socket operations
	 */
	Signal_handler<Main>           timeout_handler
		{ env.ep(), *this, &Main::handle_timeout };

	Signal_handler<Main>           config_handler
		{ env.ep(), *this, &Main::handle_config };

	Main(Env &env)
	: env(env)
	{
		config.sigh(config_handler);
		timer.sigh(timeout_handler);

		handle_config();
	}

	void handle_config()
	{
		config.update();

		Xml_node xml = config.xml();

		/* construct api object with provided server URI */
		Libc::with_libc([&] () {
			api.construct(xml.attribute_value("server", Api::Server_uri { "http://127.0.0.1" }));
		});

		/* set periodic timeout */
		unsigned ms = xml.attribute_value("period_ms", 5000U);
		timer.trigger_periodic(ms * 1000U);
	}

	void handle_timeout()
	{

		Libc::with_libc([&] () {
			if (api->postHeartbeat(++heartbeat_id))
				log("Sent heartbeat ", heartbeat_id);
			else
				error("POST request failed");

			if (!api->getHeartbeat())
				error("GET request failed");
		});
	}
};


void Libc::Component::construct(Libc::Env &env) { static Heartbeat_test::Main main(env); }
