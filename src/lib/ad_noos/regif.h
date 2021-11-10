/*
 * \brief  Register interface for custom IP core
 * \author Johannes Schlatow
 * \date   2021-10-29
 *
 * The IP core provides a MMIO interface for setting parameters and enabling/
 * disabling DMA loopback mode.
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _SRC__LIB__AD_NOOS__REGIF_H_
#define _SRC__LIB__AD_NOOS__REGIF_H_

#include <platform_session/device.h>

namespace Ad {
	class Regif;
}

class Ad::Regif : public ::Platform::Device::Mmio
{
	private:
		struct Bypass : Register<0x00, 32>
		{
			struct Loopback : Bitfield<1,1>
			{
				enum {
					LOOPBACK_ENABLE  = 0,
					LOOPBACK_DISABLE = 1
				};
			};
		};

		struct Version : Register<0xFC, 32> { };

	public:
		Regif(::Platform::Device & device)
		: ::Platform::Device::Mmio(device)
		{
			if (read<Version>() < 0x01010042)
				error("Unable to detect register interface. Got version ", Hex(read<Version>()));
		}

		void enable_loopback()  { write<Bypass::Loopback>(Bypass::Loopback::LOOPBACK_ENABLE); }
		void disable_loopback() { write<Bypass::Loopback>(Bypass::Loopback::LOOPBACK_DISABLE); }
};

#endif /* _SRC__LIB__AD_NOOS__REGIF_H_ */
