/*
 * \brief  Zynq DMA guard implementation
 * \author Johannes Schlatow
 * \date   2022-12-20
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _SRC__DRIVERS__PLATFORM__DMA_GUARD_H_
#define _SRC__DRIVERS__PLATFORM__DMA_GUARD_H_

#include <os/attached_mmio.h>
#include <util/register_set.h>
#include <util/misc_math.h>
#include <base/allocator.h>

#include <device.h>

namespace Driver {
	using namespace Genode;

	class Dma_guard;
	class Dma_guard_factory;
}


class Driver::Dma_guard : private Attached_mmio,
                          public  Driver::Control_device
{
	private:

		enum {
			NUM_SEGMENTS = 10
		};

		struct Ctrl : Register<0x0, 32>
		{
			struct Enable : Bitfield<0,2>
			{
				enum {
					READ_WRITE = 0x0,
					WRITE_ONLY = 0x1,
					READ_ONLY  = 0x2,
					DENY       = 0x3
				};
			};
		};

		struct Segments : Register_array<0x4, 32, NUM_SEGMENTS, 32>
		{
			struct Valid     : Bitfield< 0, 1> { };
			struct Writeable : Bitfield< 1, 1> { };
			struct Size      : Bitfield< 4, 8> { };
			struct Addr      : Bitfield<12,20> { };
		};

		/**
		 * Control_device interface
		 */

		void _enable() override {
			write<Ctrl::Enable>(Ctrl::Enable::READ_WRITE); }

		void _disable() override {
			write<Ctrl::Enable>(Ctrl::Enable::DENY); }

		void _add_range(Range) override;
		void _remove_range(Range) override;

	public:

		/**
		 * Control_device interface
		 */


		Dma_guard(Env                      & env,
		          Control_devices          & control_devices,
		          Device::Name       const & name,
		          Device::Io_mem::Range      range)
		: Attached_mmio(env, range.start, range.size),
		  Control_device(control_devices, name)
		{ };

		~Dma_guard() { _destroy_domains(); }
};


class Driver::Dma_guard_factory : public Driver::Control_device_factory
{
	private:

		Genode::Env  & _env;

	public:

		Dma_guard_factory(Genode::Env & env, Common & common)
		: Control_device_factory(common.control_device_factories(), Device::Type { "dma_guard" }),
		  _env(env)
		{ }

		void create(Allocator & alloc, Control_devices & control_devices, Device const & device) override
		{
			using Range = Device::Io_mem::Range;

			device.for_each_io_mem([&] (unsigned idx, Range range, Device::Pci_bar, bool)
			{
				if (idx == 0)
					new (alloc) Dma_guard(_env, control_devices, device.name(), range);
			});
		}
};


void Driver::Dma_guard::_add_range(Range range)
{
	bool found = false;

	/* calculate log2 size */
	size_t size_log2 = log2(range.size);
	if ((1U << size_log2) < range.size) size_log2++;

	for (size_t i=0; !found && i < NUM_SEGMENTS; i++) {
		if (!read<Segments::Valid>(i)) {
			write<Segments>(
				Segments::Valid::bits(1) |
				Segments::Writeable::bits(1) |
				Segments::Addr::masked(range.start) |
				Segments::Size::bits(size_log2-2),
				i);
			found = true;
		}
	}

	if (!found)
		error(__func__, "() all segment registers are already in use");
}


void Driver::Dma_guard::_remove_range(Range range)
{
	bool found = false;
	uint32_t addr_field = Segments::Addr::get(range.start);

	/* find segment for this address and invalidate */
	size_t i=0;
	for (; !found && i < NUM_SEGMENTS; i++) {
		if (read<Segments::Valid>(i) && read<Segments::Addr>(i) == addr_field) {
			write<Segments::Valid>(0, i);
			found = true;
		}
	}

	if (!found)
		warning(__func__, "() unable to find segment");

	/* move remaining segment registers to fill the space of the invalidated segment */
	for (; i < NUM_SEGMENTS; i++) {
		if (!read<Segments::Valid>(i))
			break;

		write<Segments>(read<Segments>(i), i-1);
	}
}

#endif /* _SRC__DRIVERS__PLATFORM__DMA_GUARD_H_ */
