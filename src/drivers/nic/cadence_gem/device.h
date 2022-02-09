/*
 * \brief  Base EMAC driver for the Xilinx EMAC PS used on Zynq devices
 * \author Timo Wischer
 * \author Johannes Schlatow
 * \date   2015-03-10
 */

/*
 * Copyright (C) 2015-2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__DRIVERS__NIC__CADENCE_GEM__DEVICE_H_
#define _INCLUDE__DRIVERS__NIC__CADENCE_GEM__DEVICE_H_

/* Genode includes */
#include <timer_session/connection.h>
#include <platform_session/device.h>

/* local includes */
#include "marvell_phy.h"

namespace Cadence_gem
{
	using namespace Genode;

	class Device;
}

class Cadence_gem::Device
:
	protected Platform::Device::Mmio,
	public Phyio
{
	private:

		/**
		* Control register
		*/
		struct Control : Register<0x00, 32>
		{
			struct Local_loopback  : Bitfield<1, 1> {};
			struct Rx_en  : Bitfield<2, 1> {};
			struct Tx_en  : Bitfield<3, 1> {};
			struct Mgmt_port_en  : Bitfield<4, 1> {};
			struct Clear_statistics  : Bitfield<5, 1> {};
			struct Start_tx  : Bitfield<9, 1> {};
			struct Tx_pause  : Bitfield<11, 1> {};

			static access_t init() {
				return Mgmt_port_en::bits(1);
			}
		};

		/**
		* Config register
		*/
		struct Config : Register<0x04, 32>
		{
			struct Speed_100  : Bitfield<0, 1> {};
			struct Full_duplex  : Bitfield<1, 1> {};
			struct Copy_all  : Bitfield<4, 1> {};
			struct No_broadcast  : Bitfield<5, 1> {};
			struct Multi_hash_en  : Bitfield<6, 1> {};
			struct Gige_en  : Bitfield<10, 1> {};
			struct Pause_en  : Bitfield<13, 1> {};
			struct Fcs_remove  : Bitfield<17, 1> {};
			struct Mdc_clk_div  : Bitfield<18, 3> {
				enum {
					DIV_32 = 0b010,
					DIV_224 = 0b111,
				};
			};
			struct Dis_cp_pause   : Bitfield<23, 1> {};
			struct Rx_chksum_en   : Bitfield<24, 1> {};
			struct Ignore_rx_fcs  : Bitfield<26, 1> {};
		};

		/**
		* Status register
		*/
		struct Status : Register<0x08, 32>
		{
			struct Phy_mgmt_idle  : Bitfield<2, 1> {};
		};

		/**
		* DMA Config register
		*/
		struct Dma_config : Register<0x10, 32>
		{
			struct Disc_when_no_ahb : Bitfield<24,1> {};
			struct Rx_pktbuf_memsz_sel  : Bitfield<8, 2> {
				enum {
					SPACE_8KB = 0x3,
				};
			};
			struct Tx_pktbuf_memsz_sel  : Bitfield<10, 1> {
				enum {
					SPACE_4KB = 0x1,
				};
			};
			struct Ahb_mem_rx_buf_size  : Bitfield<16, 8> {
				enum {
					BUFFER_1600B = 0x19,
				};
			};
			struct Csum_gen_en : Bitfield<11, 1> { };
			struct Burst_len : Bitfield<0, 5> {
				enum {
					INCR16 = 0x10,
					INCR8  = 0x08,
					INCR4  = 0x04,
					SINGLE = 0x01
				};
			};

			static access_t init()
			{
				return Ahb_mem_rx_buf_size::bits(Ahb_mem_rx_buf_size::BUFFER_1600B) |
					Rx_pktbuf_memsz_sel::bits(Rx_pktbuf_memsz_sel::SPACE_8KB) |
					Tx_pktbuf_memsz_sel::bits(Tx_pktbuf_memsz_sel::SPACE_4KB) |
					Disc_when_no_ahb::bits(1) |
					Csum_gen_en::bits(0) |
					Burst_len::bits(Burst_len::INCR16);
			}
		};

		/**
		* Tx_Status register
		*/
		struct Tx_status : Register<0x14, 32>
		{
			struct Tx_hresp_nok    : Bitfield<8, 1> {};
			struct Late_collision  : Bitfield<7, 1> {};
			struct Tx_err_underrun : Bitfield<6, 1> {};
			struct Tx_complete     : Bitfield<5, 1> {};
			struct Tx_err_bufexh   : Bitfield<4, 1> {};
			struct Tx_go           : Bitfield<3, 1> {};
			struct Tx_retry_limit  : Bitfield<2, 1> {};
			struct Collision       : Bitfield<1, 1> {};
			struct Used_bit_read   : Bitfield<0, 1> {};
		};

		/**
		* Receiving queue base addresse register
		*/
		struct Rx_qbar : Register<0x18, 32>
		{
			struct Addr : Bitfield<0, 32> {};
		};

		/**
		* Transmition queue base addresse register
		*/
		struct Tx_qbar : Register<0x1C, 32>
		{
			struct Addr : Bitfield<0, 32> {};
		};

		/**
		* Receive status register
		*/
		struct Rx_status : Register<0x20, 32>
		{
			struct Rx_hresp_nok   : Bitfield<3, 1> {};
			struct Rx_overrun     : Bitfield<2, 1> {};
			struct Frame_received : Bitfield<1, 1> {};
			struct Buffer_not_available : Bitfield<0, 1> {};
		};

		/**
		* Interrupt status register
		*/
		struct Interrupt_status : Register<0x24, 32>
		{
			struct Rx_used_read   : Bitfield<2, 1> {};
			struct Rx_complete    : Bitfield<1, 1> {};
			struct Tx_complete    : Bitfield<7, 1> {};
			struct Pause_zero     : Bitfield<13,1> {};
			struct Pause_received : Bitfield<12,1> {};
			struct Rx_overrun     : Bitfield<10,1> {};
		};

		/**
		* Interrupt enable register
		*/
		struct Interrupt_enable : Register<0x28, 32>
		{
			struct Rx_used_read   : Bitfield<2, 1> {};
			struct Rx_complete    : Bitfield<1, 1> {};
			struct Pause_zero     : Bitfield<13,1> {};
			struct Pause_received : Bitfield<12,1> {};
			struct Rx_overrun     : Bitfield<10,1> {};
		};

		/**
		* Interrupt disable register
		*/
		struct Interrupt_disable : Register<0x2C, 32>
		{
			struct Rx_complete : Bitfield<1, 1> {};
		};

		/**
		* PHY maintenance register
		*/
		struct Phy_maintenance : Register<0x34, 32>
		{
			struct Clause_22 : Bitfield<30, 1> {};
			struct Operation : Bitfield<28, 2> {
				enum Type {
					READ = 0b10,
					WRITE = 0b01
				};
			};
			struct Phy_addr : Bitfield<23, 5> {};
			struct Reg_addr : Bitfield<18, 5> {};
			struct Must_10 : Bitfield<16, 2> {
				enum { INIT = 0b10 };
			};
			struct Data : Bitfield<0, 16> {};
		};

		/**
		* MAC hash register
		*/
		struct Hash_register : Register<0x80, 64>
		{
			struct Low_hash   : Bitfield<0, 32> { };
			struct High_hash   : Bitfield<32, 16> { };
		};

		/**
		* MAC Addresse
		*/
		struct Mac_addr_1 : Register<0x88, 64>
		{
			struct Low_addr   : Bitfield<0, 32> { };
			struct High_addr   : Bitfield<32, 16> { };
		};

		/**
		* Counter for the successfully transmitted frames
		*/
		struct Frames_transmitted : Register<0x108, 32>
		{
			struct Counter : Bitfield<0, 32> { };
		};

		/**
		* Counter for the transmitted pause frames
		*/
		struct Pause_transmitted : Register<0x114, 32>
		{
			struct Counter : Bitfield<0, 16> { };
		};

		/**
		* Counter for tx underrun errors
		*/
		struct Tx_underrun : Register<0x134, 32>
		{
			struct Counter : Bitfield<0, 10> { };
		};

		/**
		* Counter for deferred transmission frames
		*/
		struct Tx_deferred: Register<0x148, 32>
		{
			struct Counter : Bitfield<0, 18> { };
		};

		/**
		* Counter for the successfully received frames
		*/
		struct Frames_received : Register<0x158, 32>
		{
			struct Counter : Bitfield<0, 32> { };
		};

		/**
		* Counter for resource error statistics
		*/
		struct Rx_resource_errors : Register<0x1A0, 32>
		{
			struct Counter : Bitfield<0, 18> { };
		};

		/**
		* Counter for overrun statistics
		*/
		struct Rx_overrun_errors : Register<0x1A4, 32>
		{
			struct Counter : Bitfield<0, 10> { };
		};

		/**
		* Counter for IP checksum errors
		*/
		struct Rx_ip_chksum_errors : Register<0x1A8, 32>
		{
			struct Counter : Bitfield<0, 8> { };
		};

		/**
		* Counter for TCP checksum errors
		*/
		struct Rx_tcp_chksum_errors : Register<0x1AC, 32>
		{
			struct Counter : Bitfield<0, 8> { };
		};

		/**
		* Counter for UDP checksum errors
		*/
		struct Rx_udp_chksum_errors : Register<0x1B0, 32>
		{
			struct Counter : Bitfield<0, 8> { };
		};

		/**
		* Counter for FCS errors
		*/
		struct Rx_fcs_errors : Register<0x190, 32>
		{
			struct Counter : Bitfield<0, 10> { };
		};

		/**
		* Counter for pause frames received
		*/
		struct Pause_received : Register<0x164, 32>
		{
			struct Counter : Bitfield<0, 16> { };
		};

		/**
		 * These two structs help avoiding the following compile errors in
		 * places where the driver has to convert MAC addresses pointers
		 * to integer pointers:
		 *
		 * error: taking address of packed member of ‘Net::Mac_address’
		 *        may result in an unaligned pointer value
		 *
		 * As the MAC address type is packed and therefore has alignment 1,
		 * we have to ensure that the pointer type we convert it to also
		 * has alignment 1, i.e., that it is also packed.
		 */
		struct Packed_uint16 { uint16_t value; } __attribute__((packed));
		struct Packed_uint32 { uint32_t value; } __attribute__((packed));

		class Phy_timeout_for_idle : public Genode::Exception {};
		class Unkown_ethernet_speed : public Genode::Exception {};

		Timer::Connection       _timer;
		Platform::Device::Irq   _irq;
		Marvel_phy              _phy;

		void _mdio_wait()
		{
			int timeout = 200;

			/* Wait till MDIO interface is ready to accept a new transaction. */
			while (!read<Status::Phy_mgmt_idle>()) {
				if (timeout <= 0) {
					warning(__func__, ": Timeout");
					throw Phy_timeout_for_idle();
				}

				_timer.msleep(1);
				timeout--;
			}
		}

		void _phy_setup_op(const uint8_t phyaddr, const uint8_t regnum, const uint16_t data, const Phy_maintenance::Operation::Type op)
		{
			_mdio_wait();

			/* Write mgtcr and wait for completion */
			write<Phy_maintenance>(
						Phy_maintenance::Clause_22::bits(1) |
						Phy_maintenance::Operation::bits(op) |
						Phy_maintenance::Phy_addr::bits(phyaddr) |
						Phy_maintenance::Reg_addr::bits(regnum) |
						Phy_maintenance::Must_10::bits(Phy_maintenance::Must_10::INIT) |
						Phy_maintenance::Data::bits(data) );

			_mdio_wait();
		}


		/***********
		 ** Phyio **
		 ***********/

		void phy_write(const uint8_t phyaddr, const uint8_t regnum, const uint16_t data) override
		{
			_phy_setup_op(phyaddr, regnum, data, Phy_maintenance::Operation::WRITE);
		}

		void phy_read(const uint8_t phyaddr, const uint8_t regnum, uint16_t& data) override
		{
			_phy_setup_op(phyaddr, regnum, 0, Phy_maintenance::Operation::READ);

			/* the 'Data' field has 16 bits */
			data = (uint16_t)read<Phy_maintenance::Data>();
		}

	public:

		/**
		 * Constructor
		 */
		Device(Genode::Env      &env,
		       Platform::Device &device)
		:
			Platform::Device::Mmio(device),
			_timer(env),
			_irq(device),
			_phy(*this)
		{
			deinit();
			init();
		}

		void transmit_start()
		{
			write<Control::Start_tx>(1);
		}

		Nic::Mac_address read_mac_address()
		{
			/* TODO remove packed workaround */

			Nic::Mac_address mac;
			Packed_uint32 * const low_addr_pointer  = reinterpret_cast<Packed_uint32 *>(&mac.addr[0]);
			Packed_uint16 * const high_addr_pointer = reinterpret_cast<Packed_uint16 *>(&mac.addr[4]);

			/* the 'Low_addr' field is 32 bit, the 'High_addr' is 16 bit wide */
			low_addr_pointer->value  = (uint32_t)read<Mac_addr_1::Low_addr>();
			high_addr_pointer->value = (uint16_t)read<Mac_addr_1::High_addr>();

			return mac;
		}

		template <typename RX,
		          typename TX,
		          typename RECEIVE_PKTS,
		          typename TRANSMIT_PKTS>
		void handle_irq(RX &rx, TX &tx,
		                RECEIVE_PKTS  && receive_pkts,
		                TRANSMIT_PKTS && transmit_pkts)
		{
			/* 16.3.9 Receiving Frames */
			/* read interrupt status, to detect the interrupt reason */
			const Interrupt_status::access_t status = read<Interrupt_status>();
			const Rx_status::access_t rxStatus = read<Rx_status>();
			const Tx_status::access_t txStatus = read<Tx_status>();

			if ( Interrupt_status::Rx_complete::get(status) ) {
				receive_pkts();

				/* reset receive complete interrupt */
				write<Rx_status>(Rx_status::Frame_received::bits(1));
				write<Interrupt_status>(Interrupt_status::Rx_complete::bits(1));
			}

			if (Interrupt_status::Tx_complete::get(status)
			 || Tx_status::Tx_complete::get(txStatus)) {

				/* reset interrupt status */
				write<Tx_status>(Tx_status::Tx_complete::bits(1));
				write<Interrupt_status>(Interrupt_status::Tx_complete::bits(1));
				
				/* continue sending */
				transmit_pkts();
			}

			/* handle Rx/Tx errors */
			if ( Tx_status::Tx_hresp_nok::get(txStatus)
			  || Rx_status::Rx_hresp_nok::get(rxStatus)) {

				disable();

				tx.reset();
				rx.reset();

				enable();

				write<Tx_status>(Tx_status::Tx_hresp_nok::bits(1));
				write<Rx_status>(Rx_status::Rx_hresp_nok::bits(1));
				Genode::error("Rx/Tx error: resetting both");
			}

			/* handle Tx errors */
			if ( Tx_status::Tx_err_underrun::get(txStatus)
			  || Tx_status::Tx_err_bufexh::get(txStatus)) {

				write<Control::Tx_en>(0);
				tx.reset();
				write<Control::Tx_en>(1);

				Genode::error("Tx error: resetting transceiver");
			}

			/* handle Rx error */
			bool print_stats = false;
			if (Interrupt_status::Rx_overrun::get(status)) {
				write<Control::Tx_pause>(1);
				write<Interrupt_status>(Interrupt_status::Rx_overrun::bits(1));
				write<Rx_status>(Rx_status::Rx_overrun::bits(1));

				/* reset the receiver because this may lead to a deadlock */
				write<Control::Rx_en>(0);
				rx.reset();
				write<Control::Rx_en>(1);

				print_stats = true;
				Genode::error("Rx overrun - packet buffer overflow");
			}
			if (Interrupt_status::Rx_used_read::get(status)
			 && Rx_status::Buffer_not_available::get(rxStatus)) {
				/* tried to use buffer descriptor with used bit set */
				/* we sent a pause frame because the buffer appears to
				 * be full
				 */
				write<Control::Tx_pause>(1);
				write<Interrupt_status>(Interrupt_status::Rx_used_read::bits(1));
				write<Rx_status>(Rx_status::Buffer_not_available::bits(1));

				print_stats = true;
				Genode::error("Rx used - the Nic client is not fast enough");
			}
			if (Interrupt_status::Pause_zero::get(status)) {
				Genode::warning("Pause ended.");
				write<Interrupt_status>(Interrupt_status::Pause_zero::bits(1));
				print_stats = true;
			}
			if (Interrupt_status::Pause_received::get(status)) {
				Genode::warning("Pause frame received.");
				write<Interrupt_status>(Interrupt_status::Pause_received::bits(1));
				print_stats = true;
			}

			if (print_stats) {
				/* check, if there was lost some packages */
				const uint32_t received  = read<Frames_received>();
				const uint32_t pause_rx  = read<Pause_received::Counter>();
				const uint32_t res_err   = read<Rx_resource_errors::Counter>();
				const uint32_t overrun   = read<Rx_overrun_errors::Counter>();
				const uint32_t fcs_err   = read<Rx_fcs_errors::Counter>();
				const uint32_t ip_chk    = read<Rx_ip_chksum_errors::Counter>();
				const uint32_t udp_chk   = read<Rx_udp_chksum_errors::Counter>();
				const uint32_t tcp_chk   = read<Rx_tcp_chksum_errors::Counter>();
				const uint32_t transmit  = read<Frames_transmitted>();
				const uint32_t pause_tx  = read<Pause_transmitted::Counter>();
				const uint32_t underrun  = read<Tx_underrun::Counter>();
				const uint32_t deferred  = read<Tx_deferred::Counter>();

				Genode::warning("Received:          ", received);
				Genode::warning("  pause frames:    ", pause_rx);
				Genode::warning("  resource errors: ", res_err);
				Genode::warning("  overrun errors:  ", overrun);
				Genode::warning("  FCS errors:      ", fcs_err);
				Genode::warning("  IP chk failed:   ", ip_chk);
				Genode::warning("  UDP chk failed:  ", udp_chk);
				Genode::warning("  TCP chk failed:  ", tcp_chk);
				Genode::warning("Transmitted:       ", transmit);
				Genode::warning("  pause frames:    ", pause_tx);
				Genode::warning("  underrun:        ", underrun);
				Genode::warning("  deferred:        ", deferred);
			}
		}

		void irq_sigh(Signal_context_capability cap) {
			_irq.sigh(cap); }
		
		void irq_ack() { _irq.ack(); }

		void enable(addr_t rx_base=0, addr_t tx_base=0)
		{
			/* set base addresses of rx and tx buffers */
			if (rx_base)
				write<Rx_qbar>(rx_base);

			if (tx_base)
				write<Tx_qbar>(tx_base);

			/* enable */
			write<Control::Rx_en>(1);
			write<Control::Tx_en>(1);
		}

		void disable()
		{
			write<Control::Rx_en>(0);
			write<Control::Tx_en>(0);
		}

		void init()
		{
			/* see 16.3.2 Configure the Controller */

			/* 1. Program the Network Configuration register (gem.net_cfg) */
			write<Config>(
				Config::Gige_en::bits(1) |
				Config::Speed_100::bits(1) |
				Config::Pause_en::bits(1) |
				Config::Full_duplex::bits(1) |
				Config::Multi_hash_en::bits(1) |
				Config::Mdc_clk_div::bits(Config::Mdc_clk_div::DIV_32) |
				Config::Dis_cp_pause::bits(1) |
				Config::Rx_chksum_en::bits(1) |
				Config::Fcs_remove::bits(1)
			);


			/* 3. Program the DMA Configuration register (gem.dma_cfg) */
			write<Dma_config>( Dma_config::init() );

			/*
			 * 4. Program the Network Control Register (gem.net_ctrl)
			 * Enable MDIO, transmitter and receiver
			 */
			write<Control>(Control::init());

			switch (_phy.eth_speed()) {
			case SPEED_1000:
				write<Config::Gige_en>(1);
				log("Autonegotiation result: 1Gbit/s");
				break;
			case SPEED_100:
				write<Config::Gige_en>(0);
				write<Config::Speed_100>(1);
				log("Autonegotiation result: 100Mbit/s");
				break;
			case SPEED_10:
				write<Config::Gige_en>(0);
				write<Config::Speed_100>(0);
				log("Autonegotiation result: 10Mbit/s");
				break;
			default:
				throw Unkown_ethernet_speed();
			}


			/* 16.3.6 Configure Interrupts */
			write<Interrupt_enable>(Interrupt_enable::Rx_complete::bits(1) |
				                     Interrupt_enable::Rx_overrun::bits(1) |
				                     Interrupt_enable::Pause_received::bits(1) |
				                     Interrupt_enable::Pause_zero::bits(1) |
				                     Interrupt_enable::Rx_used_read::bits(1));
		}

		void deinit()
		{
			/* 16.3.1 Initialize the Controller */

			/* Disable all interrupts */
			write<Interrupt_disable>(0x7FFFEFF);

			/* Disable the receiver & transmitter */
			write<Control>(0);
			write<Control>(Control::Clear_statistics::bits(1));

			write<Tx_status>(0xFF);
			write<Rx_status>(0x0F);
			write<Phy_maintenance>(0);

			write<Rx_qbar>(0);
			write<Tx_qbar>(0);

			/* Clear the Hash registers for the mac address
			 * pointed by AddressPtr
			 */
			write<Hash_register>(0);
		}

		void write_mac_address(const Nic::Mac_address &mac)
		{
			Packed_uint32 const * const low_addr_pointer  = reinterpret_cast<Packed_uint32 const *>(&mac.addr[0]);
			Packed_uint16 const * const high_addr_pointer = reinterpret_cast<Packed_uint16 const *>(&mac.addr[4]);

			write<Mac_addr_1::Low_addr>(low_addr_pointer->value);
			write<Mac_addr_1::High_addr>(high_addr_pointer->value);
		}
};

#endif /* _INCLUDE__DRIVERS__NIC__CADENCE_GEM__CADENCE_GEM_H_ */

