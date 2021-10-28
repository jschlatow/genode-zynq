/*
 * \brief  Dmac loopback test
 * \author Johannes Schlatow
 * \date   2021-10-27
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

enum {
	MESSAGE_SIZE = 1000,
	MESSAGES_PER_BATCH = 10000,
	MESSAGES_PER_RUN   = 5*MESSAGES_PER_BATCH,
};


struct Main : Ad::Ad9361
{

	Attached_rom_dataspace        config      { _env, "config" };
	Timer::Connection             timer       { _env };
	Signal_handler<Main>          irq_handler { _env.ep(), *this, &Main::handle_irq };

	unsigned                      cnt_recv    { 0 };
	unsigned                      cnt_sent    { 0 };
	unsigned                      cnt_lost    { 0 };
	unsigned long                 start_ts    { 0 };

	void send_more()
	{
		using Result = Ad::Axi_dmac_tx::Result;

		bool done = false;
		while (!done && cnt_sent <= MESSAGES_PER_RUN) {
			Result res = tx().write_transfer([&] (void *ptr, size_t max_size) {
				size_t size = min(max_size, (size_t)MESSAGE_SIZE);
				memset(ptr, cnt_sent, size);
				reinterpret_cast<unsigned*>(ptr)[0] = cnt_sent++;
				return size;
			});

			switch (res) {
				case Result::OKAY:
					break;
				case Result::NOT_SUPPORTED:
				case Result::BUFFER_EXCEEDED:
				case Result::DEVICE_ERROR:
					error("Unexpected error during write_transfer()");
					[[fallthrough]];
				case Result::QUEUE_FULL:
					done = true;
					break;
			}
		}

		if (cnt_sent && (cnt_sent % MESSAGES_PER_BATCH) == 0)
			log("Sending ", cnt_sent, " messages took ", timer.elapsed_ms() - start_ts, "ms");
	}

	void handle_irq()
	{
		rx().handle_irq([&]() {
			rx().read_completed_transfers([&] (void * ptr, size_t) {

				unsigned received = reinterpret_cast<unsigned*>(ptr)[0];
				if (received >= cnt_recv) {
					cnt_lost += received - cnt_recv;
					cnt_recv = received + 1;
				}
				else if (received < cnt_recv) {
					error("Receive error: got ", Hex(received), " expected ", Hex(cnt_recv));
				}

				if (cnt_recv && (cnt_recv % MESSAGES_PER_BATCH) == 0) {
					unsigned long long duration_ms = timer.elapsed_ms() - start_ts;
					unsigned long long kbit        = 8000ULL*(unsigned long long)cnt_recv*MESSAGE_SIZE
					                                 / (1024ULL*1024ULL);
					log("Receiving ", cnt_recv, " messages took ", duration_ms, "ms");
					log("   -> ", kbit / duration_ms, " Mbit/s");
					if (cnt_lost) warning("Lost ", cnt_lost, " messages on the way");
					cnt_lost = 0;
				}

			});
		});

		send_more();

		rx().irq_ack();
	};

	Main(Genode::Env &env)
	: Ad::Ad9361(env)
	{
		/* init ad9361 */
		apply_config(config.xml());

		/* register and enable RX irq handling */
		rx().irq_sigh(irq_handler);
		rx().irq_ack();
		rx().enable_rx(MESSAGE_SIZE);

		start_ts = timer.elapsed_ms();
		send_more();
	}

};

void Libc::Component::construct(Libc::Env &env)
{
	static Main main { env };
}
