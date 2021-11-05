/*
 * \brief  Nic client implementation (for testing without rf hardware)
 * \author Johannes Schlatow
 * \date   2021-11-04
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__DRIVERS__NIC__RF__DUMMY__DEVICE_H_
#define _INCLUDE__DRIVERS__NIC__RF__DUMMY__DEVICE_H_

/* NIC driver includes */
#include <nic_session/connection.h>
#include <nic/packet_allocator.h>

namespace Genode {

	class Device;
}

class Genode::Device
{
	private:
		using Signal_handler = Genode::Signal_handler<Device>;

		enum { PKT_SIZE = Nic::Packet_allocator::DEFAULT_PACKET_SIZE };
		enum { BUF_SIZE = Nic::Session::QUEUE_SIZE * PKT_SIZE };

		Env                   &_env;
		Allocator             &_alloc;
		Nic::Packet_allocator  _pkt_alloc { &_alloc };
		Nic::Connection        _nic       { _env, &_pkt_alloc, BUF_SIZE, BUF_SIZE };
		Nic::Mac_address const _mac       { _nic.mac_address() };

	public:

		Device(Env       &env,
		       Allocator &alloc)
		: _env(env),
		  _alloc(alloc)
		{ }

		void rx_sigh(Signal_context_capability cap)
		{
			_nic.rx_channel()->sigh_ready_to_ack(cap);
			_nic.rx_channel()->sigh_packet_avail(cap);
		}

		void tx_sigh(Signal_context_capability cap)
		{
			_nic.tx_channel()->sigh_ack_avail(cap);
			_nic.tx_channel()->sigh_ready_to_submit(cap);
		}

		/**
		 * acknowledge queue not full anymore
		 *   or
		 * submit queue not empty anymore
		 */
		template <typename READY_TO_RECEIVE,
		          typename RECEIVE_PKT>
		void handle_rx(READY_TO_RECEIVE && ready,
		               RECEIVE_PKT      && receive)
		{
			/* dequeue packets from submit queue and queue acks */
			while (_nic.rx()->packet_avail() &&
			       _nic.rx()->ready_to_ack() &&
			       ready())
			{
				Nic::Packet_descriptor p = _nic.rx()->get_packet();
				try {
					receive(_nic.rx()->packet_content(p), p.size());
					_nic.rx()->acknowledge_packet(p);
				} catch (...) {
					error("Could not receive packet"); }
			}
		}

		/**
		 * acknowledge queue not empty anymore
		 *   or
		 * submit queue not full anymore
		 */
		template <typename TRANSMIT_PKTS>
		void handle_tx(TRANSMIT_PKTS && transmit)
		{
			/* dequeue acks */
			while (_nic.tx()->ack_avail()) {
				_nic.tx()->release_packet(_nic.tx()->get_acked_packet()); }

			/* queue more packets */
			transmit();
		}

		bool ready_to_transmit() { return _nic.tx()->ready_to_submit(); }

		void transmit_pkt(char const *buffer, size_t size)
		{
			Nic::Packet_descriptor p = _nic.tx()->alloc_packet(size);

			memcpy(_nic.tx()->packet_content(p), buffer, size);

			_nic.tx()->submit_packet(p);
		}

		Nic::Mac_address const &mac() const { return _mac; }
};

#endif /* _INCLUDE__DRIVERS__NIC__RF__DUMMY__DEVICE_H_ */
