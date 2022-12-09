/*
 * \brief  Test component for aes co-processor
 * \author Johannes Schlatow
 * \date   2022-11-30
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <libc/component.h>
#include <base/env.h>
#include <util/lazy_array.h>
#include <timer_session/connection.h>

/* Xilinx port includes */
#include <xilinx_axidma.h>

/* Crypto algorithms includes */
extern "C" {
#include <aes.h>

int aes_decrypt_cbc(const BYTE in[],      // plaintext
                    size_t in_len,        // Must be a multiple of AES_BLOCK_SIZE
                    BYTE out[],           // Output MAC
                    const WORD key[],     // From the key setup
                    int keysize,          // Bit length of the key, 128, 192, or 256
                    const BYTE iv[]);     // IV, must be AES_BLOCK_SIZE bytes long
}


using namespace Genode;

struct Aes_control : Platform::Device::Mmio
{
	struct Ctrl : Register<0x0, 32> {
		struct Start        : Bitfield<0,1> { };
		struct Auto_restart : Bitfield<7,1> { };
	};

	struct Key : Register_array<0x38,32,8,32> { };

	struct Iv : Register_array<0x10,32,4,32> { };

	Aes_control(Platform::Device &device)
	: Mmio(device)
	{ }
};

struct Dma_ring_buffer {

	struct Dma_buffer_pair {
		Platform::Dma_buffer &tx;
		Platform::Dma_buffer &rx;
	};

	unsigned _tail { 0 };
	unsigned _head { 0 };

	Lazy_array<Platform::Dma_buffer, 3> _tx_buffers;
	Lazy_array<Platform::Dma_buffer, 3> _rx_buffers;

	Dma_ring_buffer(Platform::Connection &platform, size_t element_size)
	: _tx_buffers(3, platform, element_size, CACHED),
	  _rx_buffers(3, platform, element_size, CACHED)
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

struct Main {
	using Dma_buffer = Platform::Dma_buffer;

	enum {
		THROUGHPUT_BURST_SIZE = 1024*1024,
		ACCESS_SIZE           = 64*30
	};

	Env            &env;

	Xilinx::Axidma  axidma { env,
	                         Platform::Device::Type { "axi_dma" },
	                         Xilinx::Axidma::Mode::NORMAL };

	Platform::Device device      { axidma.platform(), Platform::Device::Type { "encrypt" } };
	Aes_control      aes_control { device };

	Xilinx::Axidma::Transfer_complete_handler<Main> rx_handler {
		*this, &Main::handle_rx_complete };

	/* state for throughput test */
	unsigned                        last_counter { 0 };
	unsigned                        counter      { 0 };
	unsigned                        rx_counter   { 0 };
	Dma_ring_buffer                 buffers      { axidma.platform(), THROUGHPUT_BURST_SIZE };

	/* timer for throughput reporting */
	using Periodic_timeout = Timer::Periodic_timeout<Main>;
	Timer::Connection               timer   { env };
	Constructible<Periodic_timeout> timeout { };

	Main(Env & env) : env(env)
	{
		test_aes(*this, &Main::encrypt_sw);
		test_aes(*this, &Main::encrypt_hw);

		test_throughput_sw();

		axidma.rx_complete_handler(rx_handler);

		test_throughput_hw();
	}

	void handle_rx_complete()
	{
		if (!axidma.rx_transfer_complete())
			return;

		/* advance tail and initiate next transfer */
		buffers.advance_tail();
		_queue_next_transfer();
		_fill_transfers();
	}

	void encrypt_sw(Dma_buffer&, size_t, Dma_buffer&, BYTE*, size_t, BYTE*);
	void encrypt_hw(Dma_buffer&, size_t, Dma_buffer&, BYTE*, size_t, BYTE*);

	template <typename T>
	void test_aes(T &, void (T::*)(Dma_buffer&, size_t, Dma_buffer&, BYTE*, size_t, BYTE*));

	/*******************
	 * throughput test *
	 *******************/

	void test_throughput_sw();

	void _fill_transfers()
	{
		/* fill all buffers */
		while (true) {
			Genode::memset(buffers.head().tx.local_addr<void>(), (uint8_t)counter, ACCESS_SIZE);
			*buffers.head().tx.local_addr<unsigned>() = counter;

			if (buffers.advance_head())
				counter++;
			else
				break;
		}
	}

	void _queue_next_transfer()
	{
		/* start transfer */
		if (buffers.empty()) {
			warning("unable to queue transfer from empty ring buffer");
			return;
		}

		Dma_ring_buffer::Dma_buffer_pair bufs = buffers.tail();
		if (axidma.start_rx_transfer(bufs.rx, THROUGHPUT_BURST_SIZE) != Xilinx::Axidma::Result::OKAY)
			error("DMA rx transfer failed");
		if (axidma.start_tx_transfer(bufs.tx, THROUGHPUT_BURST_SIZE) != Xilinx::Axidma::Result::OKAY)
			error("DMA tx transfer failed");
	}

	void test_throughput_hw()
	{
		_fill_transfers();

		timeout.construct(timer, *this, &Main::handle_timeout, Microseconds { 1000 * 2000U });

		_queue_next_transfer();
	}

	void handle_timeout(Duration)
	{
		unsigned long transmitted = counter - last_counter;
		last_counter = counter;

		log("Current loopback throughput: ", ((transmitted * THROUGHPUT_BURST_SIZE) / 2000000UL), "MB/s");
	}
};


void Main::encrypt_sw(Dma_buffer &plaintext, size_t size, Dma_buffer &cyphertext, BYTE *key, size_t key_size, BYTE* iv)
{
	WORD key_schedule[60];
	aes_key_setup(key, key_schedule, key_size);

	aes_encrypt_cbc(plaintext.local_addr<BYTE>(), size, cyphertext.local_addr<BYTE>(), key_schedule, key_size, iv);
}


void Main::encrypt_hw(Dma_buffer &plaintext, size_t size, Dma_buffer &cyphertext, BYTE *key, size_t, BYTE* iv)
{
	aes_control.write<Aes_control::Ctrl::Auto_restart>(0);
	aes_control.write<Aes_control::Ctrl::Start>(0);

	aes_control.write<Aes_control::Iv>(*(uint32_t*)&iv[0],  0);
	aes_control.write<Aes_control::Iv>(*(uint32_t*)&iv[4],  1);
	aes_control.write<Aes_control::Iv>(*(uint32_t*)&iv[8],  2);
	aes_control.write<Aes_control::Iv>(*(uint32_t*)&iv[12], 3);

	aes_control.write<Aes_control::Key>(*(uint32_t*)&key[0], 0);
	aes_control.write<Aes_control::Key>(*(uint32_t*)&key[4], 1);
	aes_control.write<Aes_control::Key>(*(uint32_t*)&key[8], 2);
	aes_control.write<Aes_control::Key>(*(uint32_t*)&key[12], 3);
	aes_control.write<Aes_control::Key>(*(uint32_t*)&key[16], 4);
	aes_control.write<Aes_control::Key>(*(uint32_t*)&key[20], 5);
	aes_control.write<Aes_control::Key>(*(uint32_t*)&key[24], 6);
	aes_control.write<Aes_control::Key>(*(uint32_t*)&key[28], 7);

	aes_control.write<Aes_control::Ctrl::Auto_restart>(1);
	aes_control.write<Aes_control::Ctrl::Start>(1);

	/* stream plaintext and receive cyphertext */
	if (axidma.simple_transfer(plaintext, size, cyphertext, size) != Xilinx::Axidma::Result::OKAY)
		error("DMA transfer failed");
}


template <typename T>
void Main::test_aes(T & obj, void (T::*fn)(Dma_buffer&, size_t, Dma_buffer&, BYTE*, size_t, BYTE*))
{
	log("running ", __func__);
	enum { BUF_SIZE = 4096 };

	static BYTE decrypted[BUF_SIZE] { };

	WORD key_schedule[60];
	BYTE iv[16]  = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
	BYTE key[32] = { 0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4};

	Dma_buffer original  { axidma.platform(), BUF_SIZE, CACHED };
	Dma_buffer encrypted { axidma.platform(), BUF_SIZE, CACHED };

	(obj.*fn)(original, BUF_SIZE, encrypted, key, 256, iv);

	aes_key_setup(key, key_schedule, 256);
	aes_decrypt_cbc(encrypted.local_addr<BYTE>(), BUF_SIZE, decrypted, key_schedule, 256, iv);

	if (Genode::memcmp(original.local_addr<char>(), decrypted, BUF_SIZE)) {
		error(__func__, " failed");
		unsigned *orig_words = original.local_addr<unsigned>();
		unsigned *encr_words = encrypted.local_addr<unsigned>();
		for (size_t i=0; i < 16; i+=2) {
			log(Hex(orig_words[i]), " ", Hex(orig_words[i+1]), " | ",
			    Hex(encr_words[i]), " ", Hex(encr_words[i+1]));
		}
	}
	else
		log(__func__, " finsished");
}


void Main::test_throughput_sw()
{
	Dma_buffer src { axidma.platform(), THROUGHPUT_BURST_SIZE, CACHED };
	Dma_buffer dst { axidma.platform(), THROUGHPUT_BURST_SIZE, CACHED };

	BYTE iv[16]  = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
	BYTE key[32] = { 0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4};

	uint64_t start = timer.elapsed_ms();
	uint64_t end   = start;
	uint64_t bytes = 0;
	log("starting ", __func__);
	while (end - start < 5000) {
		for (size_t i=0; i < 10; i++) {
			encrypt_sw(src, THROUGHPUT_BURST_SIZE, dst, key, 256, iv);
			bytes += THROUGHPUT_BURST_SIZE;
		}
		end = timer.elapsed_ms();
	};

	log("Encrypted ", bytes, " bytes in ", (double)(end - start) / 1000.0d, " seconds at ", (bytes / 1000ULL) / (end - start), " MB/s");
}


void Libc::Component::construct(Env &env) {
	static Main main(env);
}
