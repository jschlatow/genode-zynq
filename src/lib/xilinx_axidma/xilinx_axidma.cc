/*
 * \brief  axidma wrapper
 * \author Johannes Schlatow
 * \date   2022-11-14
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <xilinx_axidma.h>

XAxiDma_Config Xilinx::Axidma::_config()
{
	XAxiDma_Config result { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	                        0, 0, 0, 0, 0, 0, 0 };

	result.BaseAddr = (UINTPTR)_mmio.local_addr<unsigned>();

	using Name = String<64>;

	_platform.update();
	_platform.with_xml([&] (Xml_node & xml) {
		xml.for_each_sub_node("device", [&] (Xml_node device) {
			if (device.attribute_value("type", Name { }) != _type.name)
				return;

			device.for_each_sub_node("property", [&] (Xml_node par) {
				Name name  = par.attribute_value("name", Name());
				int  value = par.attribute_value("value", 0);

				if (name == "XPAR_AXI_DMA__SG_INCLUDE_STSCNTRL_STRM")   result.HasStsCntrlStrm = value;
				else if (name == "XPAR_AXI_DMA__INCLUDE_MM2S")          result.HasMm2S         = value;
				else if (name == "XPAR_AXI_DMA__INCLUDE_MM2S_DRE")      result.HasMm2SDRE      = value;
				else if (name == "XPAR_AXI_DMA__M_AXI_MM2S_DATA_WIDTH") result.Mm2SDataWidth   = value;
				else if (name == "XPAR_AXI_DMA__INCLUDE_S2MM")          result.HasS2Mm         = value;
				else if (name == "XPAR_AXI_DMA__INCLUDE_S2MM_DRE")      result.HasMm2SDRE      = value;
				else if (name == "XPAR_AXI_DMA__M_AXI_S2MM_DATA_WIDTH") result.S2MmDataWidth   = value;
				else if (name == "XPAR_AXI_DMA__INCLUDE_SG")            result.HasSg           = value;
				else if (name == "XPAR_AXI_DMA__NUM_MM2S_CHANNELS")     result.Mm2sNumChannels = value;
				else if (name == "XPAR_AXI_DMA__NUM_S2MM_CHANNELS")     result.S2MmNumChannels = value;
				else if (name == "XPAR_AXI_DMA__MM2S_BURST_SIZE")       result.Mm2SBurstSize   = value;
				else if (name == "XPAR_AXI_DMA__S2MM_BURST_SIZE")       result.S2MmBurstSize   = value;
				else if (name == "XPAR_AXI_DMA__MICRO_DMA")             result.MicroDmaMode    = value;
				else if (name == "XPAR_AXI_DMA__ADDR_WIDTH")            result.AddrWidth       = value;
				else if (name == "XPAR_AXI_DMA__SG_LENGTH_WIDTH")       result.SgLengthWidth   = value;
			});
		});
	});

	if ((result.Mm2SBurstSize == 0   && result.S2MmBurstSize == 0) ||
	    (result.Mm2SDataWidth == 0   && result.S2MmDataWidth == 0) ||
	    (result.Mm2sNumChannels == 0 && result.S2MmNumChannels == 0))
	{
		warning("Invalid MM2S/S2MM configuration parameters (missing burst size, data width, or #channels)");
	}

	return result;
}


Xilinx::Axidma::Result Xilinx::Axidma::_init()
{
	XAxiDma_Config cfg = _config();

	int status = XAxiDma_CfgInitialize(&_xaxidma, &cfg);
	if (status != XST_SUCCESS) {
		error("Initialization failed: ", status);
		return Result::CONFIG_ERROR;
	}

	if (_mode != Mode::SG && XAxiDma_HasSg(&_xaxidma)) {
		error("Device configured as SG mode");
		return Result::CONFIG_ERROR;
	}

	switch (_mode) {
		case Mode::SIMPLE:
			/* disable interrupts */
			XAxiDma_IntrDisable(&_xaxidma, XAXIDMA_IRQ_ALL_MASK,
			                               XAXIDMA_DEVICE_TO_DMA);
			XAxiDma_IntrDisable(&_xaxidma, XAXIDMA_IRQ_ALL_MASK,
			                               XAXIDMA_DMA_TO_DEVICE);
			break;
		case Mode::NORMAL:
			/* enable interrupts */
			XAxiDma_IntrEnable(&_xaxidma, XAXIDMA_IRQ_ALL_MASK,
			                              XAXIDMA_DEVICE_TO_DMA);
			XAxiDma_IntrEnable(&_xaxidma, XAXIDMA_IRQ_ALL_MASK,
			                              XAXIDMA_DMA_TO_DEVICE);
		case Mode::SG:
			break;
	}

	return Result::OKAY;
}


void Xilinx::Axidma::_handle_irq()
{
	_rx_irq->ack();
	_tx_irq->ack();

	/* read pending interrupts */
	u32 tx_status = XAxiDma_IntrGetIrq(&_xaxidma, XAXIDMA_DMA_TO_DEVICE);
	u32 rx_status = XAxiDma_IntrGetIrq(&_xaxidma, XAXIDMA_DEVICE_TO_DMA);

	/* acknowledge pending interrupts */
	XAxiDma_IntrAckIrq(&_xaxidma, tx_status, XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_IntrAckIrq(&_xaxidma, rx_status, XAXIDMA_DEVICE_TO_DMA);

	/* check for errors */
	if (((tx_status|rx_status) & XAXIDMA_IRQ_ERROR_MASK)) {
		error("DMA error, resetting device for recovery");

		/* reset device */
		XAxiDma_Reset(&_xaxidma);

		for (unsigned timeout = 10000; timeout; timeout--) {
			if (XAxiDma_ResetIsDone(&_xaxidma)) {
				break;
			}
		}

		return;
	}

	if ((tx_status & XAXIDMA_IRQ_IOC_MASK)) {
		if (_tx_complete_handler)
			_tx_complete_handler->handle_transfer_complete();
	}

	if ((rx_status & XAXIDMA_IRQ_IOC_MASK)) {
		if (_rx_complete_handler)
			_rx_complete_handler->handle_transfer_complete();
	}
}


Xilinx::Axidma::Result Xilinx::Axidma::simple_transfer(Platform::Dma_buffer const &src_buf, size_t src_len,
                                                       Platform::Dma_buffer const &dst_buf, size_t dst_len)
{
	Result result { Result::OKAY };

	result = start_rx_transfer(dst_buf, dst_len);
	if (result != Result::OKAY)
		return result;

	result = start_tx_transfer(src_buf, src_len);
	if (result != Result::OKAY)
		return result;

	do {
		if (_mode == Mode::NORMAL)
			_env.ep().wait_and_dispatch_one_io_signal();

	} while (!rx_transfer_complete() || !tx_transfer_complete());

	uint32_t status_reg = XAxiDma_ReadReg(_xaxidma.RegBase, XAXIDMA_SR_OFFSET);
	if (status_reg & XAXIDMA_ERR_ALL_MASK) {
		error("Xilinx::Axidma::simple_transfer: failed (", Hex(status_reg), ")");
		return Result::DEVICE_ERROR;
	}

	return Result::OKAY;
}


Xilinx::Axidma::Result Xilinx::Axidma::start_tx_transfer(Platform::Dma_buffer const &buf, size_t len)
{
	if (_mode == Mode::SG) {
		error("Axidma device has not been initialised for simple transfers");
		return CONFIG_ERROR;
	}

	int status = XAxiDma_SimpleTransfer(&_xaxidma,
	                                    buf.dma_addr(),
	                                    len,
	                                    XAXIDMA_DMA_TO_DEVICE);

	if (status != XST_SUCCESS) {
		error("XAxiDma_SimpleTransfer() failed (DMA_TO_DEVICE)");
		return Result::DEVICE_ERROR;
	}

	return Result::OKAY;
}


Xilinx::Axidma::Result Xilinx::Axidma::start_rx_transfer(Platform::Dma_buffer const &buf, size_t len)
{
	if (_mode == Mode::SG) {
		error("Axidma device has not been initialised for simple transfers");
		return CONFIG_ERROR;
	}

	int status = XAxiDma_SimpleTransfer(&_xaxidma,
	                                    buf.dma_addr(),
	                                    len,
	                                    XAXIDMA_DEVICE_TO_DMA);

	if (status != XST_SUCCESS) {
		error("XAxiDma_SimpleTransfer() failed (DEVICE_TO_DMA)");
		return Result::DEVICE_ERROR;
	}

	return Result::OKAY;
}


bool Xilinx::Axidma::tx_transfer_complete()
{ return !XAxiDma_Busy(&_xaxidma, XAXIDMA_DMA_TO_DEVICE); }


bool Xilinx::Axidma::rx_transfer_complete()
{ return !XAxiDma_Busy(&_xaxidma, XAXIDMA_DEVICE_TO_DMA); }
