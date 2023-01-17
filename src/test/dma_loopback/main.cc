/*
 * \brief  Test component for xilinx_axidma
 * \author Johannes Schlatow
 * \date   2022-11-11
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <libc/component.h>
#include <timer_session/connection.h>
#include <base/attached_rom_dataspace.h>

/* Xilinx port includes */
#include <xilinx_axidma.h>

/* local includes */
#include <dma_ring_buffer.h>

using namespace Genode;

struct Main {

	Env            &env;

	Xilinx::Axidma  axidma  { env, Xilinx::Axidma::Mode::NORMAL };

	Xilinx::Axidma::Transfer_complete_handler<Main> rx_handler {
		*this, &Main::handle_rx_complete };

	Attached_rom_dataspace config { env, "config" };

	Cache         cache           { cache_from_xml() };
	size_t        max_buffer_size { config.xml().attribute_value("max_size", Number_of_bytes { 32*1024*1024 }) };
	size_t        cur_buffer_size { 8*1024 };

	size_t        max_access_size { 64*1024 };
	size_t        min_access_size { 64 };
	size_t        cur_access_size { min_access_size };

	/* timer for throughput reporting */
	using Periodic_timeout = Timer::Periodic_timeout<Main>;
	Timer::Connection               timer   { env };
	Constructible<Periodic_timeout> timeout { };

	/* state for throughput test */
	unsigned                        last_counter { 0 };
	unsigned                        counter      { 0 };
	unsigned                        rx_counter   { 0 };
	Dma_ring_buffer                 buffers      { axidma.platform(),
	                                               max_buffer_size,
	                                               cache };

	Cache cache_from_xml()
	{
		bool cached = config.xml().attribute_value("cached", false);
		return cached ? CACHED : UNCACHED;
	}

	/* simple transfer test */
	void test_simple_transfer(size_t, uint8_t);

	/* methods for throughput test */
	void handle_rx_complete();
	void fill_transfers();
	void queue_next_transfer();

	Main(Env & env) : env(env)
	{
		test_simple_transfer(8192,  0x21);

		/* prepare throughput test */
		axidma.rx_complete_handler(rx_handler);
		fill_transfers();

		/* start periodic timer for throughput logging */
		timeout.construct(timer, *this, &Main::handle_timeout, Microseconds { 1000 * 2000U });

		/* start throughput test */
		queue_next_transfer();
	}

	void handle_timeout(Duration)
	{
		unsigned long transmitted = counter - last_counter;
		last_counter = counter;

		if (transmitted == last_counter)
			return;

		log("Current loopback throughput: ", ((transmitted * cur_buffer_size) / 2000000UL),
		    "MB/s with ", cur_buffer_size/1024,
		    "KB buffers and access size ", cur_access_size);

		cur_access_size *= 4;
		if (cur_access_size > max_access_size || cur_access_size > cur_buffer_size) {
			cur_access_size = min_access_size;

			log("");
			cur_buffer_size *= 4;
			if (cur_buffer_size > max_buffer_size)
				env.parent().exit(0);
		}
	}
};


void Main::test_simple_transfer(size_t size, uint8_t value)
{
	Platform::Dma_buffer src_buffer { axidma.platform(), size, cache };
	Platform::Dma_buffer dst_buffer { axidma.platform(), size, cache };

	/* initialise src buffer */
	Genode::memset(src_buffer.local_addr<void>(), value, size);
	Genode::memset(dst_buffer.local_addr<void>(), value != 0 ? 0 : -1, size);

	log("initiating simple transfer of size ", (unsigned)size);

	/* perform DMA transfer */
	if (axidma.simple_transfer(src_buffer, size, dst_buffer, size) != Xilinx::Axidma::Result::OKAY) {
		error("DMA transfer failed");
		env.parent().exit(1);
		return;
	}

	/* compare buffers */
	if (Genode::memcmp(src_buffer.local_addr<void>(), dst_buffer.local_addr<void>(), size)) {
		error("DMA transfer failed - Data error");
		env.parent().exit(1);
	} else
		log("DMA transfer succeeded");
}


void Main::handle_rx_complete()
{
	if (!axidma.rx_transfer_complete())
		return;

	/* compare the first cur_access_size bytes of src and dst buffers */
	Dma_ring_buffer::Dma_buffer_pair bufs = buffers.tail();
	if (Genode::memcmp(bufs.tx.local_addr<void>(),
		                bufs.rx.local_addr<void>(), cur_access_size)) {
		error("DMA failed - Data error");
		env.parent().exit(1);
	}
	/* check whether memory content has the expected value */
	else if (*bufs.tx.local_addr<unsigned>() != rx_counter++) {
		error("Expected ", rx_counter-1, " but got ", *bufs.tx.local_addr<unsigned>());
		env.parent().exit(1);
	}

	/* advance tail and initiate next transfer */
	buffers.advance_tail();
	queue_next_transfer();
	fill_transfers();
}


void Main::fill_transfers()
{
	/* fill all buffers */
	while (true) {
		Genode::memset(buffers.head().tx.local_addr<void>(), (uint8_t)counter, cur_access_size);
		*buffers.head().tx.local_addr<unsigned>() = counter;

		if (buffers.advance_head())
			counter++;
		else
			break;
	}
}


void Main::queue_next_transfer()
{
	/* start transfer */
	if (buffers.empty()) {
		warning("unable to queue transfer from empty ring buffer");
		return;
	}

	Dma_ring_buffer::Dma_buffer_pair bufs = buffers.tail();
	if (axidma.start_rx_transfer(bufs.rx, cur_buffer_size) != Xilinx::Axidma::Result::OKAY) {
		error("DMA rx transfer failed");
		env.parent().exit(1);
	}

	if (axidma.start_tx_transfer(bufs.tx, cur_buffer_size) != Xilinx::Axidma::Result::OKAY) {
		error("DMA tx transfer failed");
		env.parent().exit(1);
	}
}


void Libc::Component::construct(Env &env) {
	static Main main(env);
}
