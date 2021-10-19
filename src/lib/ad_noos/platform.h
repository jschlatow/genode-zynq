/*
 * \brief  Platform_session-based device management for contrib code
 * \author Johannes Schlatow
 * \date   2021-10-15
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _SRC__LIB__AD_NOOS__PLATFORM_H_
#define _SRC__LIB__AD_NOOS__PLATFORM_H_

#include <base/env.h>
#include <base/heap.h>
#include <platform_session/connection.h>
#include <platform_session/device.h>
#include <drivers/gpio.h>

namespace Ad {
	struct Platform;
	struct Attached_device;

	using Device = ::Platform::Device;

	/**
	 * Returns global platform object
	 */
	Platform & platform(Genode::Env * env = nullptr);
}

struct Ad::Attached_device : Genode::List<Ad::Attached_device>::Element
{
	Device::Type      type;
	Device            device;
	Device::Mmio      mmio;

	Attached_device(::Platform::Connection & platform,
	                Device::Type             name)
	: type(name),
	  device(platform, type),
	  mmio(device)
	{ list()->insert(this); }

	static Genode::List<Ad::Attached_device> *list()
	{
		static Genode::List<Ad::Attached_device> _list;
		return &_list;
	}
};

struct Ad::Platform
{
	Genode::Env                  &env;
	::Platform::Connection       platform  { env };
	Genode::Heap                 heap      { env.ram(), env.rm() };
	Device                       gpiodev   { platform, Device::Type { "zynq-gpio" } };
	Gpio::Zynq_regs              gpio      { gpiodev };

	Platform(Genode::Env & env) : env(env) { }

	unsigned long addr_by_name(const char * name)
	{
		Device::Type  type { name };
		unsigned long addr { 0 };

		/* find device among attached devices */
		with_device(type, [&] (Attached_device &dev) {
			addr = reinterpret_cast<unsigned long>(dev.mmio.local_addr<unsigned long>());
		});

		if (addr)
			return addr;

		/* attach device if not found above */
		Attached_device *dev = new (heap) Attached_device(platform, type);
		return reinterpret_cast<unsigned long>(dev->mmio.local_addr<unsigned long>());
	}

	template <typename FUNC>
	void with_device(Device::Type const & type, FUNC && f)
	{
		for (Attached_device *dev = Attached_device::list()->first();
		     dev;
		     dev = dev->next())
		{
			if (dev->type.name == type.name) {
				f(*dev);
				break;
			}
		}
	}
};

#endif /* _SRC__LIB__AD_NOOS__PLATFORM_H_ */
