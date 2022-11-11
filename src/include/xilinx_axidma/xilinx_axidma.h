/*
 * \brief  axidma wrapper
 * \author Johannes Schlatow
 * \date   2022-11-11
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _XILINX_AXIDMA_H_
#define _XILINX_AXIDMA_H_

/* Genode includes */
#include <base/signal.h>
#include <util/reconstructible.h>
#include <platform_session/connection.h>
#include <platform_session/device.h>
#include <platform_session/dma_buffer.h>

/* Xilinx includes */
#include <xaxidma.h>

namespace Xilinx {
	using namespace Genode;

	using Device            = Platform::Device;

	class Axidma;
}


class Xilinx::Axidma
{
	public:
		/*
		 * The device may operate in different modes.
		 *  - SIMPLE and NORMAL refer to Direct Register Mode (single transfers)
		 *    without/with interrupt support
		 *  - SG refers to Scatter/Gather mode, which allows queueing
		 */
		enum Mode   { NORMAL, SIMPLE, SG };
		enum Result { OKAY, DEVICE_ERROR, CONFIG_ERROR };

		struct Init_error : Exception { };

		struct Handler_base : Interface, Genode::Noncopyable
		{
			virtual void handle_transfer_complete() = 0;
		};

		template <typename T>
		struct Transfer_complete_handler : Handler_base
		{
			T &_obj;
			void (T::*_member) (void);

			Transfer_complete_handler(T &obj, void (T::*member)(void))
			: _obj(obj), _member(member) { }

			void handle_transfer_complete() override
			{
				(_obj.*_member)();
			}
		};

	private:

		Env                  &_env;
		Platform::Connection  _platform;
		Device::Type          _type { "axi_dma" };
		Device                _device;
		Mode                  _mode;

		/* device has a single I/O mem */
		Device::Mmio          _mmio   { _device };

		XAxiDma               _xaxidma { };

		/* devices has two IRQs */
		Constructible<Device::Irq> _rx_irq { };
		Constructible<Device::Irq> _tx_irq { };

		Handler_base *_rx_complete_handler { nullptr };
		Handler_base *_tx_complete_handler { nullptr };

		/* irq handler must be an io signal handler to allow blocking semantics of simple_transfer() */
		Io_signal_handler<Axidma> _irq_handler {
			_env.ep(), *this, &Axidma::_handle_irq };

		/* helper methods */
		XAxiDma_Config _config();
		Result         _init();
		void           _handle_irq();

		/* Noncopyable */
		Axidma(Axidma const &) = delete;
		void operator=(Axidma const &) = delete;

	public:

		Axidma(Env &env, Mode mode)
		: _env(env),
		  _platform(env),
		  _device(_platform, _type),
		  _mode(mode)
		{
			if (_mode == Mode::SG) {
				error("Scatter/Gather mode not supported");
				throw Init_error();
			}

			if (_mode == Mode::NORMAL) {
				_rx_irq.construct(_device, Device::Irq::Index { 0 });
				_tx_irq.construct(_device, Device::Irq::Index { 1 });

				_rx_irq->sigh(_irq_handler);
				_tx_irq->sigh(_irq_handler);
			}

			Result result = _init();
			if (result != Result::OKAY)
				throw Init_error();
		}

		/* Initiate a single blocking transfer from a DMA buffer to device and vice versa. */
		Result simple_transfer(Platform::Dma_buffer const &, size_t,
		                       Platform::Dma_buffer const &, size_t);

		/* Initiate a transfer from memory to device */
		Result start_tx_transfer(Platform::Dma_buffer const &, size_t);

		/* Initiate a transfer from device to memory */
		Result start_rx_transfer(Platform::Dma_buffer const &, size_t);

		bool tx_transfer_complete();
		bool rx_transfer_complete();

		void rx_complete_handler(Handler_base &handler) {
			_rx_complete_handler = &handler; }

		void tx_complete_handler(Handler_base &handler) {
			_tx_complete_handler = &handler; }

		Platform::Connection &platform() { return _platform; }
};

#endif /* _XILINX_AXIDMA_H_ */
