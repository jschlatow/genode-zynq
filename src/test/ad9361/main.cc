/*
 * \brief  Main component for ad9361 contrib code
 * \author Johannes Schlatow
 * \date   2021-10-15
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <libc/component.h>
#include <base/attached_rom_dataspace.h>
#include <timer_session/connection.h>
#include <base/attached_rom_dataspace.h>

#include <ad9361/ad9361.h>

using namespace Genode;

struct Main : Ad::Ad9361
{
	enum {
		TIMEOUT_US   = 1000*1000,
		MESSAGE_SIZE = 1024,
	};

	Attached_rom_dataspace        config      { _env, "config" };
	Timer::Connection             timer       { _env };
	Timer::One_shot_timeout<Main> timeout     { timer, *this, &Main::handle_timeout };
	Signal_handler<Main>          irq_handler { _env.ep(), *this, &Main::handle_irq };

	unsigned                      msgnum  { 1 };

	Main(Genode::Env &env)
	: Ad::Ad9361(env)
	{
		/* init ad9361 */
		apply_config(config.xml());

		/* register and enable RX irq handling */
		rx().irq_sigh(irq_handler);
		rx().irq_ack();
		rx().enable_irq(MESSAGE_SIZE);

		/* schedule Tx timeout */
		timeout.schedule(Microseconds(TIMEOUT_US));
	}

	void handle_irq()
	{
		rx().handle_irq([&](void * ptr, size_t) {
			log("Received message with content:", Hex(reinterpret_cast<unsigned*>(ptr)[0]));
		});
		rx().irq_ack();
	};

	void handle_timeout(Duration)
	{
		log("Test message ", msgnum);
		tx().write_transfer([&] (void *ptr, size_t max_size) {
			memset(ptr, msgnum, max_size);
			return max_size;
		});
		msgnum++;
		timeout.schedule(Microseconds(TIMEOUT_US));
	}
};

void Libc::Component::construct(Libc::Env &env)
{
	static Main main { env };
}
