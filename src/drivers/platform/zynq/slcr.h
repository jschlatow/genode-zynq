/*
 * \brief  Driver for Zynq SLCR
 * \author Johannes Schlatow
 * \date   2022-04-05
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _SRC__DRIVERS__PLATFORM__ZYNQ__SLCR_H_
#define _SRC__DRIVERS__PLATFORM__ZYNQ__SLCR_H_

#include <os/attached_mmio.h>
#include <clock.h>
#include <reset.h>
#include <common.h>

namespace Driver {
	struct Slcr;
	struct Slcr_factory;
}


struct Driver::Slcr : private Attached_mmio,
                      public  Driver::Control_device
{
	Genode::Env &_env;
	Clocks      &_clocks;
	Resets      &_resets;
	Clock       &_ps_clk;

	void *_regs() { return local_addr<void>(); }

	struct Unlock : Register<0x8, 32> { };

	struct Cpu_ratio_mode : Register<0x1C4, 32>
	{
		struct Ratio621 : Bitfield<0,1> { };
	};

	const bool     _621_enabled { (bool)read<Cpu_ratio_mode::Ratio621>() };
	const unsigned _cpu_1x_div  { _621_enabled ? 6U : 4U };

	/* Individual FPGA reset domains */
	struct Fpga_reset : Register<0x240, 32> {
		struct Fpga0 : Bitfield<0,1> { };
		struct Fpga1 : Bitfield<1,1> { };
		struct Fpga2 : Bitfield<2,1> { };
		struct Fpga3 : Bitfield<3,1> { };
	};

	/*
	 * Inverted reset domain to be used when devcfg device is acquired in
	 * order to reprogram the FPGA.
	 */
	struct Fpga_nreset : Reset {

		struct Fpga_rst_reg : Register<0x240, 32> {
			struct All : Bitfield<0,4> {
				enum { RUN  = 0x0, HALT = 0xF };
			};
		};

		/* Voltage level shifters */
		struct Lvl_shifter : Register<0x900, 32> {
			struct Enable : Bitfield<0,4> {
				enum { DISABLE = 0x0,
				       PSPL    = 0xA,
				       ALL     = 0xF
				};
			};
		};

		Slcr & slcr;

		Fpga_nreset(Slcr              & slcr,
		            Reset::Name const & name)
		:
			Reset(slcr._resets, name),
			slcr(slcr) {}

		void _deassert() override
		{
			/* halt fpga when device is acquired */
			slcr.write<Fpga_rst_reg::All>(Fpga_rst_reg::All::HALT);

			/* disable all voltage level shifters */
			slcr.write<Lvl_shifter::Enable>(Lvl_shifter::Enable::DISABLE);

			/* enable only PS-to-OL voltage level shifters */
			slcr.write<Lvl_shifter::Enable>(Lvl_shifter::Enable::PSPL);
		}

		void _assert() override
		{
			/* enable all voltage level shifters */
			slcr.write<Lvl_shifter::Enable>(Lvl_shifter::Enable::ALL);

			/* reset fpga when device is released */
			slcr.write<Fpga_rst_reg::All>(Fpga_rst_reg::All::RUN);
		}
	};

	template <typename REG, typename BITFIELD>
	struct Reset_switch : Reset
	{
		Slcr                 & slcr;
		typename REG::access_t on;
		typename REG::access_t off;

		Reset_switch(Slcr                 & slcr,
		             Reset::Name const    & name,
		             typename REG::access_t on,
		             typename REG::access_t off)
		:
			Reset(slcr._resets, name),
			slcr(slcr), on(on), off(off) {}

		void _deassert() override {
			slcr.write<BITFIELD>(on); }

		void _assert() override {
			slcr.write<BITFIELD>(off); }
	};


	/*
	 * PLL clocks
	 */

	struct Pll : Clock, private Mmio
	{
		Clock &_parent;

		struct Ctrl_reg : Register<0, 32>
		{
			struct Fdiv : Bitfield<12, 7> { };
		};

		Pll(Clocks     &clocks,
		    Name const &name,
		    Clock      &parent,
		    void       *regs,
		    unsigned    reg_offset)
		: Clock(clocks, name),
		  Mmio((addr_t)regs + reg_offset),
		  _parent(parent)
		{ }

		void rate(Rate)   override { Genode::warning("PLL clock rate setting ignored"); }
		Rate rate() const override { return Rate { read<Ctrl_reg::Fdiv>() * _parent.rate().value }; }
	};

	Pll _arm_pll { _clocks, "armpll", _ps_clk, _regs(), 0x100 };
	Pll _ddr_pll { _clocks, "ddrpll", _ps_clk, _regs(), 0x104 };
	Pll _io_pll  { _clocks, "iopll",  _ps_clk, _regs(), 0x108 };

	/*
	 * CPU clocks
	 */

	struct Cpu_6or4x : Clock, private Mmio
	{
		Clocks &_clocks;

		struct Reg : Register<0x120, 32>
		{
			struct Divisor      : Bitfield<8,6> { };
			struct Src_sel      : Bitfield<4,2> { enum { DDR_PLL = 2, IO_PLL = 3 }; };
		};

		Cpu_6or4x(Clocks &clocks,
		          void   *regs)
		: Clock(clocks, "cpu_6or4x"),
		  Mmio((addr_t)regs),
		  _clocks(clocks)
		{ }

		Rate _parent_rate() const
		{
			Rate rate { 0 };
			Name name { "armpll" };

			switch (read<Reg::Src_sel>())
			{
				case Reg::Src_sel::DDR_PLL:
					name = "ddrpll";
					break;
				case Reg::Src_sel::IO_PLL:
					name = "iopll";
					break;
			}

			_clocks.apply(name, [&] (Clock & parent) {
				rate = parent.rate();
			});

			return rate;
		}

		void rate(Rate)   override { Genode::warning("CPU clock rate setting ignored"); }
		Rate rate() const override { return Rate { _parent_rate().value / Genode::max(1U, read<Reg::Divisor>()) }; }

	} _cpu_6or4x { _clocks, _regs() };

	Fixed_divider _cpu_1x    { _clocks, "cpu_1x",    _cpu_6or4x, _cpu_1x_div };
	Fixed_divider _cpu_2x    { _clocks, "cpu_2x",    _cpu_6or4x, _cpu_1x_div / 2 };
	Fixed_divider _cpu_3or2x { _clocks, "cpu_3or2x", _cpu_6or4x,  2 };

	/*
	 * Bus clocks
	 */

	Fixed_divider _dma        { _clocks, "dma",        _cpu_2x, 1 };
	Fixed_divider _usb0_aper  { _clocks, "usb0_aper",  _cpu_1x, 1 };
	Fixed_divider _usb1_aper  { _clocks, "usb1_aper",  _cpu_1x, 1 };
	Fixed_divider _gem0_aper  { _clocks, "gem0_aper",  _cpu_1x, 1 };
	Fixed_divider _gem1_aper  { _clocks, "gem1_aper",  _cpu_1x, 1 };
	Fixed_divider _sdio0_aper { _clocks, "sdio0_aper", _cpu_1x, 1 };
	Fixed_divider _sdio1_aper { _clocks, "sdio1_aper", _cpu_1x, 1 };
	Fixed_divider _spi0_aper  { _clocks, "spi0_aper",  _cpu_1x, 1 };
	Fixed_divider _spi1_aper  { _clocks, "spi1_aper",  _cpu_1x, 1 };
	Fixed_divider _can0_aper  { _clocks, "can0_aper",  _cpu_1x, 1 };
	Fixed_divider _can1_aper  { _clocks, "can1_aper",  _cpu_1x, 1 };
	Fixed_divider _i2c0_aper  { _clocks, "i2c0_aper",  _cpu_1x, 1 };
	Fixed_divider _i2c1_aper  { _clocks, "i2c1_aper",  _cpu_1x, 1 };
	Fixed_divider _uart0_aper { _clocks, "uart0_aper", _cpu_1x, 1 };
	Fixed_divider _uart1_aper { _clocks, "uart1_aper", _cpu_1x, 1 };
	Fixed_divider _gpio_aper  { _clocks, "gpio_aper",  _cpu_1x, 1 };
	Fixed_divider _lqspi_aper { _clocks, "lqspi_aper", _cpu_1x, 1 };
	Fixed_divider _smc_aper   { _clocks, "smc_aper",   _cpu_1x, 1 };
	Fixed_divider _swdt       { _clocks, "swdt",       _cpu_1x, 1 };

	/*
	 * DDR clocks
	 */

	struct Ddr_2x : Clock, private Mmio
	{
		Clock &_parent;

		struct Reg : Register<0x124, 32>
		{
			struct Divisor_2x : Bitfield<26,6> { };
		};

		Ddr_2x(Clocks &clocks,
		       Clock  &parent,
		       void   *regs)
		: Clock(clocks, "ddr2x"),
		  Mmio((addr_t)regs),
		  _parent(parent)
		{ }

		void rate(Rate)   override { Genode::warning("DDR clock rate setting ignored"); }
		Rate rate() const override { return Rate { _parent.rate().value / Genode::max(1U, read<Reg::Divisor_2x>()) }; }
	} _ddr_2x { _clocks, _ddr_pll, _regs() };


	struct Ddr_3x : Clock, private Mmio
	{
		Clock &_parent;

		struct Reg : Register<0x124, 32>
		{
			struct Divisor_3x : Bitfield<20,6> { };
		};

		Ddr_3x(Clocks &clocks,
		       Clock  &parent,
		       void   *regs)
		: Clock(clocks, "ddr3x"),
		  Mmio((addr_t)regs),
		  _parent(parent)
		{ }

		void rate(Rate)   override { Genode::warning("DDR clock rate setting ignored"); }
		Rate rate() const override { return Rate { _parent.rate().value / Genode::max(1U, read<Reg::Divisor_3x>()) }; }
	} _ddr_3x { _clocks, _ddr_pll, _regs() };


	/*
	 * Peripheral clocks
	 */

	struct Io_clk : Clock, protected Mmio
	{
		Clocks &_clocks;

		struct Reg : Register<0, 32>
		{
			struct Src_sel  : Bitfield< 4,3> { enum { ARM_PLL = 2, DDR_PLL = 3 }; };
			struct Divisor0 : Bitfield< 8,6> { };
		};

		Io_clk(Clocks     &clocks,
		       Name const &name,
		       void       *regs,
		       unsigned    reg_offset)
		: Clock(clocks, name),
		  Mmio((addr_t)regs + reg_offset),
		  _clocks(clocks)
		{ }

		Rate _parent_rate() const
		{
			Rate rate { 0 };
			Name name { "iopll" };

			switch (read<Reg::Src_sel>())
			{
				case Reg::Src_sel::DDR_PLL:
					name = "ddrpll";
					break;
				case Reg::Src_sel::ARM_PLL:
					name = "armpll";
					break;
			}

			_clocks.apply(name, [&] (Clock & parent) {
				rate = parent.rate();
			});

			return rate;
		}

		void rate(Rate)   override { Genode::warning("I/O clock rate setting ignored"); }
		Rate rate() const override { return Rate { _parent_rate().value / Genode::max(1U, read<Reg::Divisor0>()) }; }
	};

	Io_clk _sdio0_clk { _clocks, "sdio0", _regs(), 0x150 };
	Io_clk _sdio1_clk { _clocks, "sdio1", _regs(), 0x150 };

	Io_clk _uart0_clk { _clocks, "uart0", _regs(), 0x154 };
	Io_clk _uart1_clk { _clocks, "uart1", _regs(), 0x154 };

	Io_clk _spi0_clk  { _clocks, "spi0",  _regs(), 0x158 };
	Io_clk _spi1_clk  { _clocks, "spi1",  _regs(), 0x158 };

	Io_clk _smc_clk   { _clocks, "smc",   _regs(), 0x148 };
	Io_clk _lqspi_clk { _clocks, "lqspi", _regs(), 0x14C };


	struct Gem_clk : Io_clk
	{
		struct Reg : Register<0, 32>
		{
			struct Divisor1 : Bitfield<20,6> { };
		};

		Gem_clk(Clocks     &clocks,
		        Name const &name,
		        void       *regs,
		        unsigned    reg_offset)
		: Io_clk(clocks, name, regs, reg_offset)
		{ }

		Rate rate() const override { return Rate { Io_clk::rate().value / Genode::max(1U, read<Reg::Divisor1>()) }; }
	};

	Gem_clk _gem0_clk { _clocks, "gem0", _regs(), 0x140 };
	Gem_clk _gem1_clk { _clocks, "gem1", _regs(), 0x144 };


	struct Can_clk : Io_clk
	{
		struct Reg : Register<0, 32>
		{
			struct Divisor1 : Bitfield<20,6> { };
		};

		Can_clk(Clocks     &clocks,
		        Name const &name,
		        void       *regs,
		        unsigned    reg_offset)
		: Io_clk(clocks, name, regs, reg_offset)
		{ }

		Rate rate() const override { return Rate { Io_clk::rate().value / Genode::max(1U, read<Reg::Divisor1>()) }; }
	};

	Can_clk _can0_clk { _clocks, "can0", _regs(), 0x15C };
	Can_clk _can1_clk { _clocks, "can1", _regs(), 0x15C };


	struct Fpga_clk : Io_clk
	{
		struct Reg : Register<0, 32>
		{
			struct Divisor1 : Bitfield<20,6> { };
		};

		Fpga_clk(Clocks     &clocks,
		         Name const &name,
		         void       *regs,
		         unsigned    reg_offset)
		: Io_clk(clocks, name, regs, reg_offset)
		{ }

		static unsigned _find_divisor(unsigned target_div) {
			unsigned result = 1;
			unsigned primes[11] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31 };

			for (size_t i=0; i < 11; i++) {
				while (target_div % primes[i] == 0 && (result * primes[i] < 0x3f)) {
					target_div /= primes[i];
					result *= primes[i];
				}
				if (result >= target_div)
					break;
			}

			return result;
		}

		void rate(Rate r) override {
			unsigned long parent     = _parent_rate().value;
			unsigned long target     = r.value;
			unsigned long max_target = 250UL*1000UL*1000UL;
			if (target > max_target) {
				Genode::warning("Clock rate exceeds maximum.");
				target = max_target;
			}

			unsigned long target_div = parent / target + (parent % target >= target/2);
			unsigned long div1 = _find_divisor(target_div);
			unsigned long div0 = target_div / div1;

			/* first set divisors to max to avoid temporary overclocking */
			write<Io_clk::Reg::Divisor0>(0x3f);
			write<Reg::Divisor1>(0x3f);
			write<Io_clk::Reg::Divisor0>(div0);
			write<Reg::Divisor1>(div1);
		}
		Rate rate() const override { return Rate { Io_clk::rate().value / Genode::max(1U, read<Reg::Divisor1>()) }; }
	};

	Fpga_clk _fpga0_clk { _clocks, "fpga0", _regs(), 0x170 };
	Fpga_clk _fpga1_clk { _clocks, "fpga1", _regs(), 0x180 };
	Fpga_clk _fpga2_clk { _clocks, "fpga2", _regs(), 0x190 };
	Fpga_clk _fpga3_clk { _clocks, "fpga3", _regs(), 0x1a0 };

	Reset_switch<Fpga_reset, Fpga_reset::Fpga0> _fpga0_rst { *this, "fpga0", 0, 1 };
	Reset_switch<Fpga_reset, Fpga_reset::Fpga1> _fpga1_rst { *this, "fpga1", 0, 1 };
	Reset_switch<Fpga_reset, Fpga_reset::Fpga2> _fpga2_rst { *this, "fpga2", 0, 1 };
	Reset_switch<Fpga_reset, Fpga_reset::Fpga3> _fpga3_rst { *this, "fpga3", 0, 1 };

	Fpga_nreset _fpga_nreset { *this, "fpga_nreset" };

	Slcr(Genode::Env              & env,
	     Control_devices          & control_devices,
	     Device::Name       const & name,
	     Device::Io_mem::Range      range,
	     Clocks                   & clocks,
	     Resets                   & resets,
	     Clock                    & ps_clk)
	:
		Attached_mmio(env, range.start, range.size),
		Control_device(control_devices, name),
		_env(env), _clocks(clocks), _resets(resets), _ps_clk(ps_clk)
	{
		/* unlock write access to clock registers */
		write<Unlock>(0xdf0d);
	}
};


class Driver::Slcr_factory : public Driver::Control_device_factory
{
	private:

		Genode::Env  & _env;
		Device_model & _devices;

		/**
		 * PS_CLK is either 33.33333Mhz or 50MHz. The latter is very rare though
		 * and practically not in use, hence we stick to 33.33333Mhz.
		 */
		Fixed_clock _ps_clk { _devices.clocks(), "ps_clk", Clock::Rate { 33333333 } };

	public:

		Slcr_factory(Genode::Env & env, Common & common)
		: Control_device_factory(common.control_device_factories(), Device::Type { "slcr" }),
		  _env(env),
		  _devices(common.devices())
		{ }

		void create(Allocator & alloc, Control_devices & control_devices, Device const & device) override
		{
			using Range = Device::Io_mem::Range;

			device.for_each_io_mem([&] (unsigned idx, Range range, Device::Pci_bar, bool)
			{
				if (idx == 0)
					new (alloc) Slcr(_env, control_devices, device.name(), range,
					                 _devices.clocks(), _devices.resets(), _ps_clk);
			});
		}
};

#endif /* _SRC__DRIVERS__PLATFORM__ZYNQ__SLCR_H_ */
