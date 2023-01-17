/*
 * \brief  Ring buffer of DMA buffers
 * \author Johannes Schlatow
 * \date   2022-11-11
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _DMA_RING_BUFFER_H_
#define _DMA_RING_BUFFER_H_

/* Genode includes */
#include <util/lazy_array.h>
#include <platform_session/connection.h>

struct Dma_ring_buffer {

	struct Dma_buffer_pair {
		Platform::Dma_buffer &tx;
		Platform::Dma_buffer &rx;
	};

	unsigned _tail { 0 };
	unsigned _head { 0 };

	Genode::Lazy_array<Platform::Dma_buffer, 3> _tx_buffers;
	Genode::Lazy_array<Platform::Dma_buffer, 3> _rx_buffers;

	Dma_ring_buffer(Platform::Connection &platform, Genode::size_t element_size, Genode::Cache cache)
	: _tx_buffers(3, platform, element_size, cache),
	  _rx_buffers(3, platform, element_size, cache)
	{ }

	bool advance_head() {
		if ((_head+1) % _rx_buffers.count() != _tail) {
			_head = (_head+1) % _rx_buffers.count();
			return true;
		}

		return false;
	}

	bool advance_tail() {
		if (!empty()) {
			_tail = (_tail+1) % _rx_buffers.count();
			return true;
		}

		return false;
	}

	Dma_buffer_pair head() {
		return Dma_buffer_pair { _tx_buffers.value(_head), _rx_buffers.value(_head) };
	}

	Dma_buffer_pair tail() {
		return Dma_buffer_pair { _tx_buffers.value(_tail), _rx_buffers.value(_tail) };
	}

	bool empty() { return _head == _tail; }
};

#endif /* _DMA_RING_BUFFER_H_ */
