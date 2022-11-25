/*
 * \brief  PCAP driver for FPGA configuration
 * \author Johannes Schlatow
 * \date   2021-11-12
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _DRIVERS__FPGA__PCAP_H_
#define _DRIVERS__FPGA__PCAP_H_

#include <platform_session/device.h>
#include <util/reconstructible.h>

namespace Fpga {
	using namespace Genode;

	using Device = Platform::Device;

	struct Devcfg_driver;
	struct Devcfg;

	class Pcap_loader;
}


struct Fpga::Devcfg : Device::Mmio
{
	struct Ctrl : Register<0x0, 32>
	{
		struct Efuse : Bitfield<12,1> { };

		struct Rate : Bitfield<25,1>
		{
			enum { NORMAL  = 0,
		          QUARTER = 1 };
		};

		struct Mode : Bitfield<26,2>
		{
			enum { ENABLE = 0x3 };
		};

		struct Interface : Bitfield<27,1>
		{
			enum { ICAP = 0, PCAP = 0x1 };
		};

		struct Prog_b : Bitfield<30,1> { };
	};

	struct Interrupts : Register<0x0C, 32>
	{
		struct Pfg_done : Bitfield< 2,1> { };
		struct Dma_done : Bitfield<13,1> { };

		struct Seu_err       : Bitfield< 5,1> { };
		struct Hmac_err      : Bitfield< 6,1> { };
		struct P2d_len_err   : Bitfield<11,1> { };
		struct Dma_overflow  : Bitfield<14,1> { };
		struct Dma_err       : Bitfield<15,1> { };
		struct Fifo_overflow : Bitfield<18,1> { };
		struct Axi_rd_err    : Bitfield<20,1> { };
		struct Axi_wr_err    : Bitfield<22,1> { };

		struct Errors : Bitset_3<Bitset_3<Seu_err, Hmac_err, P2d_len_err>,
	                            Bitset_3<Dma_overflow, Dma_err, Fifo_overflow>,
	                            Bitset_2<Axi_rd_err, Axi_wr_err>> { };
	};

	struct Status : Register<0x14, 32>
	{
		struct Pfg_init  : Bitfield< 4,1> { };
		struct Dma_count : Bitfield<28,2> { };
		struct Dma_empty : Bitfield<30,1> { };
		struct Dma_full  : Bitfield<31,1> { };
	};

	struct Dma_src : Register<0x18, 32>
	{
		enum { FPGA = ~0U };
		enum { WAIT_FOR_PCAP_DONE = 0x1 };
	};

	struct Dma_dst : Register<0x1C, 32>
	{
		enum { FPGA = ~0U };
	};

	struct Dma_src_len : Register<0x20, 32>
	{
		struct Words : Bitfield<0,27> { };
	};

	struct Dma_dst_len : Register<0x24, 32>
	{
		struct Words : Bitfield<0,27> { };
	};

	struct Mctrl : Register<0x80, 32>
	{
		struct Loopback : Bitfield< 4,1> { };
		struct Por      : Bitfield< 8,1> { };
		struct Version  : Bitfield<28,4> { };
	};

	Devcfg(Device & device)
	: Device::Mmio(device)
	{ }
};


struct Fpga::Devcfg_driver
{
	Device device;
	Devcfg devcfg { device };

	Devcfg_driver(Platform::Connection &platform)
	: device(platform, Device::Type { "xlnx,zynq-devcfg-1.0" })
	{ }
};


class Fpga::Pcap_loader
{
	private:
		Env                         &_env;
		Platform::Connection        &_platform;
		Timer::Connection            _timer  { _env };
		Constructible<Devcfg_driver> _driver { };

		struct Dma_buffer
		{
			Platform::Connection       &_platform;
			Ram_dataspace_capability    _ds_cap;
			Genode::Attached_dataspace  _ds;
			addr_t const                _dma_addr { _platform.dma_addr(_ds_cap) };

			Dma_buffer(Genode::Env & env, Platform::Connection & platform, size_t size)
			: _platform(platform),
			  _ds_cap(_platform.alloc_dma_buffer(size, UNCACHED)),
			  _ds(env.rm(), _ds_cap)
			{ }

			~Dma_buffer() { _platform.free_dma_buffer(_ds_cap); }

			char *local_addr() { return (char*)_ds.local_addr<addr_t>(); };
		};

		bool _device_valid() {
			if (!_driver.constructed())
				_driver.construct(_platform);

			return devcfg().read<Devcfg::Mctrl::Version>() > 0;
		}

		Devcfg::Mmio &devcfg() {
			return _driver->devcfg;
		}

	public:

		Pcap_loader(Env                    &env,
		            Platform::Connection   &platform)
		: _env(env), _platform(platform)
		{ }

		/**
		 * Reset PL by following the sequence from
		 * ug585-Zynq-7000-TRM v1.13 p.212+213.
		 */
		void reset()
		{
			if (!_device_valid()) {
				error("Invalid devcfg device");
				return;
			}

			/* uboot adds a 5ms delay between PROG_B high and low if efuse is selected */
			if (devcfg().read<Devcfg::Ctrl::Efuse>())
				warning("AES efuse selected as key source, potentially needs a delay");

			devcfg().write<Devcfg::Ctrl::Prog_b>(1);
			devcfg().write<Devcfg::Ctrl::Prog_b>(0);

			while (devcfg().read<Devcfg::Status::Pfg_init>());

			devcfg().write<Devcfg::Ctrl::Prog_b>(1);
			devcfg().write<Devcfg::Interrupts::Pfg_done>(1);

			while (!devcfg().read<Devcfg::Status::Pfg_init>());
		}
		

		template <typename TRANSFER>
		bool load_bitstream(size_t size, TRANSFER && transfer)
		{
			if (!_device_valid()) {
				error("Invalid devcfg device");
				return false;
			}

			/* allocate DMA buffer */
			Dma_buffer dma_buf { _env, _platform, size };

			/* copy bitstream into DMA buffer */
			size_t transferred = transfer(dma_buf.local_addr(), size);
			if (transferred != size) {
				error("Failed copying ", size, " bytes to DMA buffer");
				return false;
			}
			
			/* enable PCAP interface */
			devcfg().write<Devcfg::Ctrl::Mode>     (Devcfg::Ctrl::Mode::ENABLE);
			devcfg().write<Devcfg::Ctrl::Interface>(Devcfg::Ctrl::Interface::PCAP);

			/* disable PCAP loopback */
			devcfg().write<Devcfg::Mctrl::Loopback>(0);

			/* clear interrupts */
			devcfg().write<Devcfg::Interrupts>(~0U);

			/* reset the PL */
			reset();

			/* check RX fifo status */
			if (devcfg().read<Devcfg::Interrupts::Errors>()) {
				error("Fatal error: ", Hex(devcfg().read<Devcfg::Interrupts::Errors>()));
				/*
				 * note: u-boot checks for RX FIFO overflow and supposedly flushes
				 * the FIFO by writing to an undocumented Mctrl bit
				 */
			}

			/* check that there is room in the command queue */
			if (devcfg().read<Devcfg::Status::Dma_full>()) {
				error("DMA queue full");
				return false;
			}

			if (!devcfg().read<Devcfg::Status::Dma_empty>()) {
				if (!devcfg().read<Devcfg::Interrupts::Dma_done>()) {
					error("DMA busy");
					return false;
				}

				/* clear out status */
				devcfg().write<Devcfg::Interrupts::Dma_done>(1);
			}

			if (devcfg().read<Devcfg::Status::Dma_count>())
				devcfg().write<Devcfg::Status::Dma_count>(0x3);

			/* set PCAP clock divider to the rate expected in non-secure mode */
			devcfg().write<Devcfg::Ctrl::Rate>(Devcfg::Ctrl::Rate::NORMAL);

			/* set DMA source address */
			devcfg().write<Devcfg::Dma_src>(dma_buf._dma_addr | Devcfg::Dma_src::WAIT_FOR_PCAP_DONE);

			/* set DMA destination address = FPGA */
			devcfg().write<Devcfg::Dma_dst>(Devcfg::Dma_dst::FPGA);

			/* set DMA source length (32bit words */
			devcfg().write<Devcfg::Dma_src_len::Words>(size >> 2);

			/* set DMA destination length - finalises DMA command */
			devcfg().write<Devcfg::Dma_dst_len::Words>(size >> 2);

			/* wait for DMA done bit (includes PCAP done) */
			while (!devcfg().read<Devcfg::Interrupts::Dma_done>()) {};

			/* check for errors */
			bool failed = devcfg().read<Devcfg::Interrupts::Errors>();

			/* make sure the PL is done */
			if (!failed) {
				while (!devcfg().read<Devcfg::Interrupts::Pfg_done>());

				_driver.destruct();
			} else {
				error("loading failed: ", Hex(devcfg().read<Devcfg::Interrupts>()));
			}

			return !failed;
		}
};

#endif /* _DRIVERS__FPGA__PCAP_H_ */
