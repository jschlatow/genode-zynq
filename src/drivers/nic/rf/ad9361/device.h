/*
 * \brief  Drives the packet-oriented interface of ad9361-based design
 * \author Johannes Schlatow
 * \date   2021-11-05
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__DRIVERS__NIC__RF__AD9361__DEVICE_H_
#define _INCLUDE__DRIVERS__NIC__RF__AD9361__DEVICE_H_

/* Genode includes */
#include <nic/packet_allocator.h>
#include <timer_session/connection.h>

/* ad9361 includes */
#include <ad9361/ad9361.h>

/* local includes */
#include "fragment.h"


namespace Genode {
	class Device;
}

class Genode::Device : protected Ad::Ad9361
{
	private:

		enum { MESSAGE_SIZE = 512 };
		enum { PACKET_SIZE  = Nic::Packet_allocator::DEFAULT_PACKET_SIZE };

		static_assert(MESSAGE_SIZE > sizeof(Rf::Fragment),
		              "Message size must larger than fragment header");

		using Packet_writer = Rf::Packet_writer<PACKET_SIZE>;

		Env              &_env;
		Allocator        &_alloc;
		Packet_writer     _packet_writer { };

	public:

		Device(Env                    &env,
		       Allocator              &alloc,
		       Attached_rom_dataspace &config)
		: Ad::Ad9361(env),
		  _env(env),
		  _alloc(alloc)
		{
			/* init ad9361 */
			apply_config(config.xml());
		}

		void rx_sigh(Signal_context_capability cap)
		{
			/* register and enable RX irq handling */
			rx().irq_sigh(cap);
			rx().irq_ack();
			rx().enable_rx(MESSAGE_SIZE);
		}

		void tx_sigh(Signal_context_capability) { }

		template <typename READY_TO_RECEIVE,
		          typename RECEIVE_PKT>
		void handle_rx(READY_TO_RECEIVE && ready,
		               RECEIVE_PKT      && receive)
		{
			rx().handle_irq([&] () {
				rx().read_completed_transfers([&] (void * ptr, size_t size) {
					using Consume_result = Packet_writer::Consume_result;

					Net::Size_guard fragment_guard(size);
					Rf::Fragment const &fragment = Rf::Fragment::cast_from(ptr, fragment_guard);

					/**
					 * TODO extend Uplink_client_base with partial write interface in
					 *      order to directly copy fragment data into packet
					 */

					bool finished = false;

					Consume_result result = _packet_writer.try_consume(fragment);
					switch (result) {
					case Consume_result::PACKET_INVALID:
					case Consume_result::NEW_PACKET:
						_packet_writer = Packet_writer(fragment);
						[[fallthrough]];
					case Consume_result::SUCCEEDED:
						/* store in packet buffer */
						finished = _packet_writer.consume_and_try_finish(fragment, fragment_guard);
						break;
					case Consume_result::FRAGMENT_LOST:
						error("Received unexpected fragment, dropping packet.");
						_packet_writer.discard();
						break;
					case Consume_result::BUFFER_EXCEEDED:
						error("Buffer exceeded");
						_packet_writer.discard();
						break;
					case Consume_result::FRAGMENT_INVALID:
						error("Invalid fragment received");
						break;
					}

					if (finished) {
						if (ready())
							receive(_packet_writer.data(), _packet_writer.size());
						else
							warning("Rx queue full - dropping packet");
					}
				});
			});

			rx().irq_ack();
		}

		template <typename TRANSMIT_PKTS>
		void handle_tx(TRANSMIT_PKTS && transmit) { transmit(); }

		bool ready_to_transmit() { return true; }

		void transmit_pkt(char const *buffer, size_t size)
		{
			using Result = Ad::Axi_dmac_tx::Result;

			Net::Size_guard                   size_guard(size);
			Rf::Fragment_writer<MESSAGE_SIZE> writer(buffer, size_guard);

			Result res = Result::OKAY;
			do {
				/* transmit fragment */
				res = tx().write_transfer([&] (void * ptr, size_t) {
					Rf::Fragment &hdr = writer.construct_header(ptr);
					try {
						writer.copy_payload(hdr);
					}
					catch (...) { error("fragmentation failed"); }

					/* XXX should return hdr.size but requires hardware to inflate
					 *     fragments to MESSAGE_SIZE on rx path
					 */
					return MESSAGE_SIZE;
				});

				switch (res) {
					case Result::NOT_SUPPORTED:
					case Result::DEVICE_ERROR:
					case Result::BUFFER_EXCEEDED:
						error("Unexpected error during write_transfer()");
						break;
					case Result::QUEUE_FULL:
						/**
						 * Should this occur regularly, we must enable Tx irqs and
						 * postpone further transfers. In this case, storing
						 * 'size_guard' and 'writer' as members, and returning false
						 * for 'ready_to_transmit()' until all fragments have been
						 * queued should do the trick.
						 */
						warning("Tx DMA queue full, dropping packet");
					case Result::OKAY:
						break;
				};

			} while (writer.more() && res == Result::OKAY);
		}
};

#endif /* _INCLUDE__DRIVERS__NIC__RF__AD9361__DEVICE_H_ */
