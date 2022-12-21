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

#include <base/registry.h>
#include <os/attached_mmio.h>
#include <util/register_set.h>
#include <util/misc_math.h>
#include <base/allocator.h>

#include <device.h>

namespace Driver {
	using namespace Genode;

	struct Guard_device;

	template <typename BUFFER>
	class Dma_guard;
}


struct Driver::Guard_device : Attached_mmio
{
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

	Guard_device(Env & env, addr_t addr, size_t size)
	: Attached_mmio(env, addr, size)
	{ };
};


template <typename BUFFER>
class Driver::Dma_guard
{
	private:

		Env                    & _env;
		Device_model           & _devices;
		Registry<BUFFER> const & _dma_buffers;
		Device::Owner            _owner_id;

		template <typename FUNC>
		void _with_guard_device(Device const & device, FUNC && fn)
		{
			/* interpret reserved_memory as guard devices */
			device.for_each_reserved_memory([&](unsigned, Platform::Device_interface::Range range) {
				Guard_device regs(_env, range.start, range.size);
				fn(regs);
			});
		}

	public:

		Dma_guard(Env                    & env,
		          Device_model           & devices,
		          Registry<BUFFER> const & dma_buffers,
		          Device::Owner            owner_id)
		:
		  _env(env),
		  _devices(devices),
		  _dma_buffers(dma_buffers),
		  _owner_id(owner_id)
		{ }

		void update()
		{
			/* apply all owned and attached devices */
			_devices.for_each([&] (Device const & dev) {
				if (!(dev.owner() == _owner_id))
					return;

				_with_guard_device(dev, [&] (Guard_device & regs) {
					size_t i=0;
					_dma_buffers.for_each([&] (BUFFER const &buf) {
						if (i >= Guard_device::NUM_SEGMENTS) {
							error("Too many DMA buffers for DMA guard");
							return;
						}

						Dataspace_client ds_client(buf.cap);
						size_t size      = ds_client.size();
						size_t size_log2 = log2(size);
						if ((1U << size_log2) < size) size_log2++;

						regs.write<Guard_device::Segments>(
							Guard_device::Segments::Valid::bits(1) |
							Guard_device::Segments::Writeable::bits(1) |
							(Guard_device::Segments::Addr::reg_mask() & buf.dma_addr) |
							Guard_device::Segments::Size::bits(size_log2-2),
							i);
						i++;
					});

					/* invalidate remaining segments */
					for (; i < Guard_device::NUM_SEGMENTS; ++i) {
						regs.write<Guard_device::Segments::Valid>(0, i);
					}
				});
			});
		}

		void enable(Device const & device)
		{
			_with_guard_device(device, [&] (Guard_device & regs) {
				regs.write<Guard_device::Ctrl::Enable>(Guard_device::Ctrl::Enable::READ_WRITE);
			});
		}

		void disable(Device const & device)
		{
			_with_guard_device(device, [&] (Guard_device & regs) {
				regs.write<Guard_device::Ctrl::Enable>(Guard_device::Ctrl::Enable::DENY);
			});
		}
};

#endif /* _SRC__DRIVERS__PLATFORM__DMA_GUARD_H_ */
