/*
 * \brief  Uplink client implementation
 * \author Johannes Schlatow
 * \date   2021-11-04
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__DRIVERS__NIC__RF__UPLINK_CLIENT_H_
#define _INCLUDE__DRIVERS__NIC__RF__UPLINK_CLIENT_H_

/* NIC driver includes */
#include <drivers/nic/uplink_client_base.h>

#include "device.h"

namespace Genode {

	class Uplink_client;
}

class Genode::Uplink_client : public Uplink_client_base
{
	private:

		Signal_handler<Uplink_client>   _rx_handler;
		Signal_handler<Uplink_client>   _tx_handler;
		Device                         &_device;

		void _handle_tx()
		{
			_device.handle_tx(
				[&] () { _conn_rx_handle_packet_avail(); });
		}

		void _handle_rx()
		{
			_device.handle_rx(
				/* are we ready for receiving a packet? */
				[&] () { return _conn->tx()->ready_to_submit(); },

				/* receive packet */
				[&] (void * src, size_t size ) {
					_drv_rx_handle_pkt(size, [&] (void * dst, size_t dst_size) {
						memcpy(dst, src, dst_size);
						return Write_result::WRITE_SUCCEEDED;
					});
				});
		}


		/************************
		 ** Uplink_client_base **
		 ************************/

		Transmit_result
		_drv_transmit_pkt(const char *buf,
		                  size_t      size) override
		{
			if (!_device.ready_to_transmit()) { return Transmit_result::RETRY; }

			try { _device.transmit_pkt(buf, size); }
			catch (...) { return Transmit_result::RETRY; }

			return Transmit_result::ACCEPTED;
		}

	public:

		Uplink_client(Env                    &env,
		              Allocator              &alloc,
		              Device                 &device,
		              Net::Mac_address const  mac_addr)
		:
			Uplink_client_base { env, alloc, mac_addr },
			_rx_handler        { env.ep(), *this, &Uplink_client::_handle_rx },
			_tx_handler        { env.ep(), *this, &Uplink_client::_handle_tx },
			_device            { device }
		{
			_drv_handle_link_state(true);

			_device.rx_sigh(_rx_handler);
			_device.tx_sigh(_tx_handler);
		}
};

#endif /* _INCLUDE__DRIVERS__NIC__RF__UPLINK_CLIENT_H_ */
