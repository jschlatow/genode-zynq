/*
 * \brief  Uplink client implementation for Cadence_gem::Device
 * \author Johannes Schlatow
 * \date   2021-08-24
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__DRIVERS__NIC__CADENCE_GEM__UPLINK_CLIENT_H_
#define _INCLUDE__DRIVERS__NIC__CADENCE_GEM__UPLINK_CLIENT_H_

/* NIC driver includes */
#include <drivers/nic/uplink_client_base.h>

#include "tx_buffer_descriptor.h"
#include "rx_buffer_descriptor.h"
#include "device.h"
#include "dma_pool.h"

namespace Cadence_gem {

	using Source    = Uplink::Session::Tx::Source;
	using Sink      = Uplink::Session::Rx::Sink;
	using Rx_buffer = Rx_buffer_descriptor<Source, Buffered_dma_pool<Source>>;
	using Tx_buffer = Tx_buffer_descriptor<Sink,   Buffered_dma_pool<Sink>>;

	class Uplink_client;
}


class Cadence_gem::Uplink_client : public Uplink_client_base
{
	private:

		Signal_handler<Uplink_client>          _irq_handler;
		Constructible<Tx_buffer>               _tx_buffer        { };
		Constructible<Rx_buffer>               _rx_buffer        { };
		Device                                &_device;

		void _transmit_acks() {
			_tx_buffer->submit_acks(); }

		void _send()
		{
			while (true) {
				if (!_conn->rx()->ready_to_ack())
					break;

				if (!_conn->rx()->packet_avail())
					break;

				if (!_tx_buffer->ready_to_submit())
					break;

				Packet_descriptor packet = _conn->rx()->get_packet();
				if (!packet.size()) {
					Genode::warning("Invalid tx packet");
					continue;
				}

				_tx_buffer->add_to_queue(packet);

				_device.transmit_start();
			}
		}

		void _handle_acks()
		{
			while (_conn->tx()->ack_avail()) {
				Packet_descriptor pd = _conn->tx()->get_acked_packet();
				_rx_buffer->reset_descriptor(pd);
			}
		}

		void _recv()
		{
			while (true) {
				if (!_conn->tx()->ready_to_submit()) {
					error("Not ready to submit received packet to uplink.");
					break;
				}

				if (!_rx_buffer->next_packet())
					break;

				Nic::Packet_descriptor pkt = _rx_buffer->get_packet_descriptor();

				if (!_conn->tx()->packet_valid(pkt)) {
					error("invalid packet descriptor ", Hex(pkt.offset()),
					      " size ", Hex(pkt.size()));
					continue;
				}

				_conn->tx()->try_submit_packet(pkt);
			}

			_conn->tx()->wakeup();
		}

		void _handle_irq()
		{
			if (!_conn.constructed()) {

				class No_connection { };
				throw No_connection { };
			}

			_handle_acks();
			_transmit_acks();

			_device.handle_irq(*_rx_buffer, *_tx_buffer,
				[&] () { _recv(); },
				[&] () { _send(); });

			_device.irq_ack();

			/* wakeup if acks have been transmitted */
			_conn->rx()->wakeup();
		}


		/************************
		 ** Uplink_client_base **
		 ************************/

		void _custom_conn_rx_handle_packet_avail() override
		{
			_handle_acks();

			_transmit_acks();
			_send();

			/* wakeup if acks have been transmitted */
			_conn->rx()->wakeup();
		}

		void _custom_conn_tx_handle_ack_avail() override
		{
			_handle_acks();
			/* also indicates ready_to_submit, but we rather wait for an IRQ */
		}

		bool _custom_conn_rx_packet_avail_handler() override
		{
			return true;
		}

		bool _custom_conn_tx_ack_avail_handler() override
		{
			return true;
		}

		Transmit_result
		_drv_transmit_pkt(const char *,
		                  size_t      ) override
		{
			class Unexpected_call { };
			throw Unexpected_call { };
		}

	public:

		Uplink_client(Env                    &env,
		              Allocator              &alloc,
		              Device                 &device,
		              Platform::Connection   &platform,
		              Net::Mac_address const  mac_addr)
		:
			Uplink_client_base { env, alloc, mac_addr },
			_irq_handler       { env.ep(), *this, &Uplink_client::_handle_irq },
			_device            { device }
		{
			_drv_handle_link_state(true);

			_tx_buffer.construct(env, platform, *_conn->rx());
			_rx_buffer.construct(env, platform, *_conn->tx());

			_device.irq_sigh(_irq_handler);
			_device.irq_ack();

			/* set mac address */
			_device.write_mac_address(mac_addr);

			_device.enable(_rx_buffer->dma_addr(), _tx_buffer->dma_addr());
		}
};

#endif /* _INCLUDE__DRIVERS__NIC__CADENCE_GEM__UPLINK_CLIENT_H_ */
