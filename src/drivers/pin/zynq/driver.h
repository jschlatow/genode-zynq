/*
 * \brief  Device interface of the Zynq-7000 pin driver
 * \author Johannes Schlatow
 * \date   2022-09-09
 */

#ifndef _DRIVER_H_
#define _DRIVER_H_

/* Genode includes */
#include <platform_session/device.h>

/* local includes */
#include <types.h>

namespace Pin_driver { struct Driver; }


struct Pin_driver::Driver
{
	Platform::Device::Mmio _mmio;

	/**
	 * Data registeres are offset 0x4 per bank
	 */
	struct Io_bank_data : Genode::Mmio
	{
		using Genode::Mmio::Mmio;

		struct Out   : Register_array<0x040, 32, 32, 1> { };
		struct In    : Register_array<0x060, 32, 32, 1> { };

		bool state(Index index) const
		{
			return read<In>(index.value);
		}

		void state(Index index, bool enabled)
		{
			write<Out>(enabled, index.value);
		}
	};

	/**
	 * Control registers are offset 0x40 per bank
	 */
	struct Io_bank_ctrl : Genode::Mmio
	{
		using Genode::Mmio::Mmio;

		struct Dir     : Register_array<0x204, 32, 32, 1> { };
		struct Outen   : Register_array<0x208, 32, 32, 1> { };

		struct Irqen   : Register_array<0x210, 32, 32, 1> { };
		struct Irqdis  : Register_array<0x214, 32, 32, 1> { };
		struct Irqstat : Register_array<0x218, 32, 32, 1> { };
		struct Irqtype : Register_array<0x21C, 32, 32, 1> { enum { LEVEL = 0, EDGE}; };
		struct Irqpol  : Register_array<0x220, 32, 32, 1> { };
		struct Irqany  : Register_array<0x224, 32, 32, 1> { };

		void configure(Index index, Attr const &attr)
		{
			switch (attr.function.value)
			{
				case Function::Value::IRQ:
				case Function::Value::INPUT:
					write<Outen>(0, index.value);
					write<Dir>  (0, index.value);
					break;
				case Function::Value::OUTPUT:
					write<Outen>(1, index.value); /* only effective if tri-state is disabled */
					write<Dir>  (1, index.value);
					break;
				default:
					warning("function ", attr.function, " not supported");
			}

			if (attr.pull.value != Pull::Value::DISABLE)
				warning("pull-up/pull-down not supported, must be done via SLCR");

			switch (attr.irq_trigger.value)
			{
				case Irq_trigger::Value::RISING:
					write<Irqtype>(Irqtype::EDGE, index.value);
					write<Irqpol> (1, index.value);
					write<Irqany> (0, index.value);
					break;
				case Irq_trigger::Value::FALLING:
					write<Irqtype>(Irqtype::EDGE, index.value);
					write<Irqpol> (0, index.value);
					write<Irqany> (0, index.value);
					break;
				case Irq_trigger::Value::HIGH:
					write<Irqtype>(Irqtype::LEVEL, index.value);
					write<Irqpol> (1, index.value);
					write<Irqany> (0, index.value);
					break;
				case Irq_trigger::Value::LOW:
					write<Irqtype>(Irqtype::LEVEL, index.value);
					write<Irqpol> (0, index.value);
					write<Irqany> (0, index.value);
					break;
				case Irq_trigger::Value::EDGES:
					write<Irqtype>(Irqtype::EDGE, index.value);
					write<Irqany> (1, index.value);
					break;
			}
		}

		void clear_irq_status(Index index)
		{
			write<Irqstat>(1, index.value);
		}

		bool irq_pending(Index index) const
		{
			return read<Irqstat>(index.value);
		}

		/**
		 * Enable/disable IRQ
		 */
		void irq_enabled(Index index, bool enabled)
		{
			if (enabled)
				write<Irqen> (1, index.value);
			else
				write<Irqdis>(1, index.value);
		}
	};

	addr_t const _base { (addr_t)  _mmio.local_addr<void>() };

	Constructible<Io_bank_data> _io_banks_data [Bank::NUM];
	Constructible<Io_bank_ctrl> _io_banks_ctrl [Bank::NUM];

	Driver(Platform::Device &device)
	:
		_mmio(device)
	{

		for (unsigned i = Bank::BANK0; i < Bank::NUM; i++) {
			Bank const bank { Bank::Value(i) };
			_io_banks_data[i].construct(_base + bank.value*0x4);
			_io_banks_ctrl[i].construct(_base + bank.value*0x40);
		}
	}

	void configure(Pin_id id, Attr const &attr)
	{
		_io_banks_ctrl[id.bank.value]->configure(id.index, attr);
	}

	bool state(Pin_id id) const
	{
		return _io_banks_data[id.bank.value]->state(id.index);
	}

	void state(Pin_id id, bool enabled)
	{
		_io_banks_data[id.bank.value]->state(id.index, enabled);
	}

	void clear_irq_status(Pin_id id)
	{
		_io_banks_ctrl[id.bank.value]->clear_irq_status(id.index);
	}

	void irq_enabled(Pin_id id, bool enabled)
	{
		_io_banks_ctrl[id.bank.value]->irq_enabled(id.index, enabled);
	}

	bool irq_pending(Pin_id id) const
	{
		return _io_banks_ctrl[id.bank.value]->irq_pending(id.index);
	}
};

#endif /* _DRIVER_H_ */
