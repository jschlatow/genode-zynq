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
#include <base/registry.h>
#include <platform_session/connection.h>
#include <platform_session/device.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>

namespace Ad {
	class Platform;
	struct Attached_device;

	using Device = ::Platform::Device;

	/**
	 * Returns global platform object
	 */
	Platform & platform(Genode::Env * env = nullptr);
}

struct Ad::Attached_device : Genode::Registry<Ad::Attached_device>::Element
{
	Device::Type      type;
	Device            device;
	Device::Mmio      mmio;

	Attached_device(::Platform::Connection            & platform,
	                Device::Type                        name,
	                Genode::Registry<Attached_device> & registry)
	: Genode::Registry<Attached_device>::Element(registry, *this),
	  type(name),
	  device(platform, type),
	  mmio(device)
	{ }
};

class Ad::Platform
{
	private:
		typedef ::Platform::Volatile_driver<Gpio::Zynq_regs,Device::Type>   Gpio_driver;
		typedef Spi::Zynq_driver                                            Spi_driver;

		Genode::Env                      &_env;
		::Platform::Connection            _platform  { _env };
		Genode::Heap                      _heap      { _env.ram(), _env.rm() };
		Gpio_driver                       _gpio      { _platform, Device::Type { "zynq-gpio" } };
		Spi_driver                        _spi       { _platform, Device::Type { "zynq-spi" } };

		Genode::Registry<Attached_device> _devices   { };

	public:

		Platform(Genode::Env & env) : _env(env) { }

		unsigned long addr_by_name(const char * name)
		{
			Device::Type  type { name };
			unsigned long addr { 0 };

			/* find device among attached devices */
			_devices.for_each([&] (Attached_device &dev) {
				if (dev.type.name == type.name)
					addr = reinterpret_cast<unsigned long>(dev.mmio.local_addr<unsigned long>());
			});

			if (addr)
				return addr;

			/* attach device if not found above */
			Attached_device *dev = new (_heap) Attached_device(_platform, type, _devices);
			return reinterpret_cast<unsigned long>(dev->mmio.local_addr<unsigned long>());
		}

		::Platform::Connection &platform() { return _platform; };
		Spi_driver             &spi()      { return _spi; }

		Gpio_driver::Driver    &gpio()
		{
			if (!_gpio.available())
				_gpio.acquire();

			return _gpio.driver();
		}
};

#endif /* _SRC__LIB__AD_NOOS__PLATFORM_H_ */
