/*
 * \brief  Ad9361 driver interface for Genode components
 * \author Johannes Schlatow
 * \date   2021-10-26
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* genode includes */
#include <libc/component.h>
#include <ad9361/ad9361.h>

/* no-OS includes */
extern "C" {
/* contrib code checks __STDC_VERSION__ to determine whether to include stdbool */
#define __STDC_VERSION__ 201112L
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <ad9361_api.h>
#include <axi_adc_core.h>
#include <axi_dac_core.h>
#pragma GCC diagnostic pop
}

/* static platform object */
#include "platform.h"

/* custom register interface */
#include "regif.h"

extern const struct gpio_platform_ops genode_gpio_ops;
extern const struct spi_platform_ops  genode_spi_ops;

/* container for required structs from contrib code */
struct Ad::Ad9361_config
{
	ad9361_rf_phy *ad9361_phy { nullptr };

	struct axi_adc_init rx_adc_init = {
		"cf-ad9361-lpc",          /* name */
		0,                        /* base */
		2                         /* channels */
	};
	struct axi_dac_init tx_dac_init = {
		"cf-ad9361-dds-core-lpc", /* name */
		0,                        /* base */
		2,                        /* channels */
		NULL                      /* manual channel config */
	};

	AD9361_InitParam default_init_param = {
		/* Device selection */
		ID_AD9361,  // dev_sel
		/* Identification number */
		0,          //id_no
		/* Reference Clock */
		40000000UL, //reference_clk_rate
		/* Base Configuration */
		0,    //two_rx_two_tx_mode_enable *** adi,2rx-2tx-mode-enable
		0,    //one_rx_one_tx_mode_use_rx_num *** adi,1rx-1tx-mode-use-rx-num
		0,    //one_rx_one_tx_mode_use_tx_num *** adi,1rx-1tx-mode-use-tx-num
		1,    //frequency_division_duplex_mode_enable *** adi,frequency-division-duplex-mode-enable
		0,    //frequency_division_duplex_independent_mode_enable *** adi,frequency-division-duplex-independent-mode-enable
		0,    //tdd_use_dual_synth_mode_enable *** adi,tdd-use-dual-synth-mode-enable
		0,    //tdd_skip_vco_cal_enable *** adi,tdd-skip-vco-cal-enable
		0,    //tx_fastlock_delay_ns *** adi,tx-fastlock-delay-ns
		0,    //rx_fastlock_delay_ns *** adi,rx-fastlock-delay-ns
		0,    //rx_fastlock_pincontrol_enable *** adi,rx-fastlock-pincontrol-enable
		0,    //tx_fastlock_pincontrol_enable *** adi,tx-fastlock-pincontrol-enable
		0,    //external_rx_lo_enable *** adi,external-rx-lo-enable
		0,    //external_tx_lo_enable *** adi,external-tx-lo-enable
		5,    //dc_offset_tracking_update_event_mask *** adi,dc-offset-tracking-update-event-mask
		6,    //dc_offset_attenuation_high_range *** adi,dc-offset-attenuation-high-range
		5,    //dc_offset_attenuation_low_range *** adi,dc-offset-attenuation-low-range
		0x28, //dc_offset_count_high_range *** adi,dc-offset-count-high-range
		0x32, //dc_offset_count_low_range *** adi,dc-offset-count-low-range
		0,    //split_gain_table_mode_enable *** adi,split-gain-table-mode-enable
		MAX_SYNTH_FREF,//trx_synthesizer_target_fref_overwrite_hz *** adi,trx-synthesizer-target-fref-overwrite-hz
		0,             // qec_tracking_slow_mode_enable *** adi,qec-tracking-slow-mode-enable
		/* ENSM Control */
		0,             //ensm_enable_pin_pulse_mode_enable *** adi,ensm-enable-pin-pulse-mode-enable
		0,             //ensm_enable_txnrx_control_enable *** adi,ensm-enable-txnrx-control-enable
		/* LO Control */
		2400000000UL,  //rx_synthesizer_frequency_hz *** adi,rx-synthesizer-frequency-hz
		2450000000UL,  //tx_synthesizer_frequency_hz *** adi,tx-synthesizer-frequency-hz
		1,             //tx_lo_powerdown_managed_enable *** adi,tx-lo-powerdown-managed-enable
		/* Rate & BW Control */
		{983040000, 245760000, 122880000, 61440000, 30720000, 30720000},// rx_path_clock_frequencies[6] *** adi,rx-path-clock-frequencies
		{983040000, 122880000, 122880000, 61440000, 30720000, 30720000},// tx_path_clock_frequencies[6] *** adi,tx-path-clock-frequencies
		18000000, //rf_rx_bandwidth_hz *** adi,rf-rx-bandwidth-hz
		18000000, //rf_tx_bandwidth_hz *** adi,rf-tx-bandwidth-hz
		/* RF Port Control */
		0,        //rx_rf_port_input_select *** adi,rx-rf-port-input-select
		0,        //tx_rf_port_input_select *** adi,tx-rf-port-input-select
		/* TX Attenuation Control */
		10000,    //tx_attenuation_mdB *** adi,tx-attenuation-mdB
		0,        //update_tx_gain_in_alert_enable *** adi,update-tx-gain-in-alert-enable
		/* Reference Clock Control */
		1,               //xo_disable_use_ext_refclk_enable *** adi,xo-disable-use-ext-refclk-enable
		{8, 5920},       //dcxo_coarse_and_fine_tune[2] *** adi,dcxo-coarse-and-fine-tune
		CLKOUT_DISABLE,  //clk_output_mode_select *** adi,clk-output-mode-select
		/* Gain Control */
		2,      //gc_rx1_mode *** adi,gc-rx1-mode
		2,      //gc_rx2_mode *** adi,gc-rx2-mode
		58,     //gc_adc_large_overload_thresh *** adi,gc-adc-large-overload-thresh
		4,      //gc_adc_ovr_sample_size *** adi,gc-adc-ovr-sample-size
		47,     //gc_adc_small_overload_thresh *** adi,gc-adc-small-overload-thresh
		8192,   //gc_dec_pow_measurement_duration *** adi,gc-dec-pow-measurement-duration
		0,      //gc_dig_gain_enable *** adi,gc-dig-gain-enable
		800,    //gc_lmt_overload_high_thresh *** adi,gc-lmt-overload-high-thresh
		704,    //gc_lmt_overload_low_thresh *** adi,gc-lmt-overload-low-thresh
		24,     //gc_low_power_thresh *** adi,gc-low-power-thresh
		15,     //gc_max_dig_gain *** adi,gc-max-dig-gain
		0,      //gc_use_rx_fir_out_for_dec_pwr_meas_enable *** adi,gc-use-rx-fir-out-for-dec-pwr-meas-enable
		/* Gain MGC Control */
		2,      //mgc_dec_gain_step *** adi,mgc-dec-gain-step
		2,      //mgc_inc_gain_step *** adi,mgc-inc-gain-step
		0,      //mgc_rx1_ctrl_inp_enable *** adi,mgc-rx1-ctrl-inp-enable
		0,      //mgc_rx2_ctrl_inp_enable *** adi,mgc-rx2-ctrl-inp-enable
		0,      //mgc_split_table_ctrl_inp_gain_mode *** adi,mgc-split-table-ctrl-inp-gain-mode
		/* Gain AGC Control */
		10,      //agc_adc_large_overload_exceed_counter *** adi,agc-adc-large-overload-exceed-counter
		2,       //agc_adc_large_overload_inc_steps *** adi,agc-adc-large-overload-inc-steps
		0,       //agc_adc_lmt_small_overload_prevent_gain_inc_enable *** adi,agc-adc-lmt-small-overload-prevent-gain-inc-enable
		10,      //agc_adc_small_overload_exceed_counter *** adi,agc-adc-small-overload-exceed-counter
		4,       //agc_dig_gain_step_size *** adi,agc-dig-gain-step-size
		3,       //agc_dig_saturation_exceed_counter *** adi,agc-dig-saturation-exceed-counter
		1000,    //agc_gain_update_interval_us *** adi,agc-gain-update-interval-us
		0,       //agc_immed_gain_change_if_large_adc_overload_enable *** adi,agc-immed-gain-change-if-large-adc-overload-enable
		0,       //agc_immed_gain_change_if_large_lmt_overload_enable *** adi,agc-immed-gain-change-if-large-lmt-overload-enable
		10,      //agc_inner_thresh_high *** adi,agc-inner-thresh-high
		1,       //agc_inner_thresh_high_dec_steps *** adi,agc-inner-thresh-high-dec-steps
		12,      //agc_inner_thresh_low *** adi,agc-inner-thresh-low
		1,       //agc_inner_thresh_low_inc_steps *** adi,agc-inner-thresh-low-inc-steps
		10,      //agc_lmt_overload_large_exceed_counter *** adi,agc-lmt-overload-large-exceed-counter
		2,       //agc_lmt_overload_large_inc_steps *** adi,agc-lmt-overload-large-inc-steps
		10,      //agc_lmt_overload_small_exceed_counter *** adi,agc-lmt-overload-small-exceed-counter
		5,       //agc_outer_thresh_high *** adi,agc-outer-thresh-high
		2,       //agc_outer_thresh_high_dec_steps *** adi,agc-outer-thresh-high-dec-steps
		18,      //agc_outer_thresh_low *** adi,agc-outer-thresh-low
		2,       //agc_outer_thresh_low_inc_steps *** adi,agc-outer-thresh-low-inc-steps
		1,       //agc_attack_delay_extra_margin_us; *** adi,agc-attack-delay-extra-margin-us
		0,       //agc_sync_for_gain_counter_enable *** adi,agc-sync-for-gain-counter-enable
		/* Fast AGC */
		64,      //fagc_dec_pow_measuremnt_duration ***  adi,fagc-dec-pow-measurement-duration
		260,     //fagc_state_wait_time_ns ***  adi,fagc-state-wait-time-ns
		/* Fast AGC - Low Power */
		0,       //fagc_allow_agc_gain_increase ***  adi,fagc-allow-agc-gain-increase-enable
		5,       //fagc_lp_thresh_increment_time ***  adi,fagc-lp-thresh-increment-time
		1,       //fagc_lp_thresh_increment_steps ***  adi,fagc-lp-thresh-increment-steps
		/* Fast AGC - Lock Level (Lock Level is set via slow AGC inner high threshold) */
		1,       //fagc_lock_level_lmt_gain_increase_en ***  adi,fagc-lock-level-lmt-gain-increase-enable
		5,       //fagc_lock_level_gain_increase_upper_limit ***  adi,fagc-lock-level-gain-increase-upper-limit
		/* Fast AGC - Peak Detectors and Final Settling */
		1,       //fagc_lpf_final_settling_steps ***  adi,fagc-lpf-final-settling-steps
		1,       //fagc_lmt_final_settling_steps ***  adi,fagc-lmt-final-settling-steps
		3,       //fagc_final_overrange_count ***  adi,fagc-final-overrange-count
		/* Fast AGC - Final Power Test */
		0,       //fagc_gain_increase_after_gain_lock_en ***  adi,fagc-gain-increase-after-gain-lock-enable
		/* Fast AGC - Unlocking the Gain */
		0,       //fagc_gain_index_type_after_exit_rx_mode ***  adi,fagc-gain-index-type-after-exit-rx-mode
		1,       //fagc_use_last_lock_level_for_set_gain_en ***  adi,fagc-use-last-lock-level-for-set-gain-enable
		1,       //fagc_rst_gla_stronger_sig_thresh_exceeded_en ***  adi,fagc-rst-gla-stronger-sig-thresh-exceeded-enable
		5,       //fagc_optimized_gain_offset ***  adi,fagc-optimized-gain-offset
		10,      //fagc_rst_gla_stronger_sig_thresh_above_ll ***  adi,fagc-rst-gla-stronger-sig-thresh-above-ll
		1,       //fagc_rst_gla_engergy_lost_sig_thresh_exceeded_en ***  adi,fagc-rst-gla-engergy-lost-sig-thresh-exceeded-enable
		1,       //fagc_rst_gla_engergy_lost_goto_optim_gain_en ***  adi,fagc-rst-gla-engergy-lost-goto-optim-gain-enable
		10,      //fagc_rst_gla_engergy_lost_sig_thresh_below_ll ***  adi,fagc-rst-gla-engergy-lost-sig-thresh-below-ll
		8,       //fagc_energy_lost_stronger_sig_gain_lock_exit_cnt ***  adi,fagc-energy-lost-stronger-sig-gain-lock-exit-cnt
		1,       //fagc_rst_gla_large_adc_overload_en ***  adi,fagc-rst-gla-large-adc-overload-enable
		1,       //fagc_rst_gla_large_lmt_overload_en ***  adi,fagc-rst-gla-large-lmt-overload-enable
		0,       //fagc_rst_gla_en_agc_pulled_high_en ***  adi,fagc-rst-gla-en-agc-pulled-high-enable
		0,       //fagc_rst_gla_if_en_agc_pulled_high_mode ***  adi,fagc-rst-gla-if-en-agc-pulled-high-mode
		64,      //fagc_power_measurement_duration_in_state5 ***  adi,fagc-power-measurement-duration-in-state5
		2,       //fagc_large_overload_inc_steps *** adi,fagc-adc-large-overload-inc-steps
		/* RSSI Control */
		1,       //rssi_delay *** adi,rssi-delay
		1000,    //rssi_duration *** adi,rssi-duration
		3,       //rssi_restart_mode *** adi,rssi-restart-mode
		0,       //rssi_unit_is_rx_samples_enable *** adi,rssi-unit-is-rx-samples-enable
		1,       //rssi_wait *** adi,rssi-wait
		/* Aux ADC Control */
		256,        //aux_adc_decimation *** adi,aux-adc-decimation
		40000000UL, //aux_adc_rate *** adi,aux-adc-rate
		/* AuxDAC Control */
		1,       //aux_dac_manual_mode_enable ***  adi,aux-dac-manual-mode-enable
		0,       //aux_dac1_default_value_mV ***  adi,aux-dac1-default-value-mV
		0,       //aux_dac1_active_in_rx_enable ***  adi,aux-dac1-active-in-rx-enable
		0,       //aux_dac1_active_in_tx_enable ***  adi,aux-dac1-active-in-tx-enable
		0,       //aux_dac1_active_in_alert_enable ***  adi,aux-dac1-active-in-alert-enable
		0,       //aux_dac1_rx_delay_us ***  adi,aux-dac1-rx-delay-us
		0,       //aux_dac1_tx_delay_us ***  adi,aux-dac1-tx-delay-us
		0,       //aux_dac2_default_value_mV ***  adi,aux-dac2-default-value-mV
		0,       //aux_dac2_active_in_rx_enable ***  adi,aux-dac2-active-in-rx-enable
		0,       //aux_dac2_active_in_tx_enable ***  adi,aux-dac2-active-in-tx-enable
		0,       //aux_dac2_active_in_alert_enable ***  adi,aux-dac2-active-in-alert-enable
		0,       //aux_dac2_rx_delay_us ***  adi,aux-dac2-rx-delay-us
		0,       //aux_dac2_tx_delay_us ***  adi,aux-dac2-tx-delay-us
		/* Temperature Sensor Control */
		256,     //temp_sense_decimation *** adi,temp-sense-decimation
		1000,    //temp_sense_measurement_interval_ms *** adi,temp-sense-measurement-interval-ms
		(int8_t)0xCE, //temp_sense_offset_signed *** adi,temp-sense-offset-signed
		1,       //temp_sense_periodic_measurement_enable *** adi,temp-sense-periodic-measurement-enable
		/* Control Out Setup */
		0xFF,    //ctrl_outs_enable_mask *** adi,ctrl-outs-enable-mask
		0,       //ctrl_outs_index *** adi,ctrl-outs-index
		/* External LNA Control */
		0,       //elna_settling_delay_ns *** adi,elna-settling-delay-ns
		0,       //elna_gain_mdB *** adi,elna-gain-mdB
		0,       //elna_bypass_loss_mdB *** adi,elna-bypass-loss-mdB
		0,       //elna_rx1_gpo0_control_enable *** adi,elna-rx1-gpo0-control-enable
		0,       //elna_rx2_gpo1_control_enable *** adi,elna-rx2-gpo1-control-enable
		0,       //elna_gaintable_all_index_enable *** adi,elna-gaintable-all-index-enable
		/* Digital Interface Control */
		0,       //digital_interface_tune_skip_mode *** adi,digital-interface-tune-skip-mode
		0,       //digital_interface_tune_fir_disable *** adi,digital-interface-tune-fir-disable
		1,       //pp_tx_swap_enable *** adi,pp-tx-swap-enable
		1,       //pp_rx_swap_enable *** adi,pp-rx-swap-enable
		0,       //tx_channel_swap_enable *** adi,tx-channel-swap-enable
		0,       //rx_channel_swap_enable *** adi,rx-channel-swap-enable
		1,       //rx_frame_pulse_mode_enable *** adi,rx-frame-pulse-mode-enable
		0,       //two_t_two_r_timing_enable *** adi,2t2r-timing-enable
		0,       //invert_data_bus_enable *** adi,invert-data-bus-enable
		0,       //invert_data_clk_enable *** adi,invert-data-clk-enable
		0,       //fdd_alt_word_order_enable *** adi,fdd-alt-word-order-enable
		0,       //invert_rx_frame_enable *** adi,invert-rx-frame-enable
		0,       //fdd_rx_rate_2tx_enable *** adi,fdd-rx-rate-2tx-enable
		0,       //swap_ports_enable *** adi,swap-ports-enable
		0,       //single_data_rate_enable *** adi,single-data-rate-enable
		0,       //lvds_mode_enable *** adi,lvds-mode-enable
		0,       //half_duplex_mode_enable *** adi,half-duplex-mode-enable
		0,       //single_port_mode_enable *** adi,single-port-mode-enable
		0,       //full_port_enable *** adi,full-port-enable
		0,       //full_duplex_swap_bits_enable *** adi,full-duplex-swap-bits-enable
		0,       //delay_rx_data *** adi,delay-rx-data
		0,       //rx_data_clock_delay *** adi,rx-data-clock-delay
		4,       //rx_data_delay *** adi,rx-data-delay
		7,       //tx_fb_clock_delay *** adi,tx-fb-clock-delay
		0,       //tx_data_delay *** adi,tx-data-delay
		150,     //lvds_bias_mV *** adi,lvds-bias-mV
		0,       //lvds_rx_onchip_termination_enable *** adi,lvds-rx-onchip-termination-enable
		0,       //rx1rx2_phase_inversion_en *** adi,rx1-rx2-phase-inversion-enable
		0xFF,    //lvds_invert1_control *** adi,lvds-invert1-control
		0x0F,    //lvds_invert2_control *** adi,lvds-invert2-control
		/* GPO Control */
		0,       //gpo_manual_mode_enable *** adi,gpo-manual-mode-enable
		0,       //gpo_manual_mode_enable_mask *** adi,gpo-manual-mode-enable-mask
		0,       //gpo0_inactive_state_high_enable *** adi,gpo0-inactive-state-high-enable
		0,       //gpo1_inactive_state_high_enable *** adi,gpo1-inactive-state-high-enable
		0,       //gpo2_inactive_state_high_enable *** adi,gpo2-inactive-state-high-enable
		0,       //gpo3_inactive_state_high_enable *** adi,gpo3-inactive-state-high-enable
		0,       //gpo0_slave_rx_enable *** adi,gpo0-slave-rx-enable
		0,       //gpo0_slave_tx_enable *** adi,gpo0-slave-tx-enable
		0,       //gpo1_slave_rx_enable *** adi,gpo1-slave-rx-enable
		0,       //gpo1_slave_tx_enable *** adi,gpo1-slave-tx-enable
		0,       //gpo2_slave_rx_enable *** adi,gpo2-slave-rx-enable
		0,       //gpo2_slave_tx_enable *** adi,gpo2-slave-tx-enable
		0,       //gpo3_slave_rx_enable *** adi,gpo3-slave-rx-enable
		0,       //gpo3_slave_tx_enable *** adi,gpo3-slave-tx-enable
		0,       //gpo0_rx_delay_us *** adi,gpo0-rx-delay-us
		0,       //gpo0_tx_delay_us *** adi,gpo0-tx-delay-us
		0,       //gpo1_rx_delay_us *** adi,gpo1-rx-delay-us
		0,       //gpo1_tx_delay_us *** adi,gpo1-tx-delay-us
		0,       //gpo2_rx_delay_us *** adi,gpo2-rx-delay-us
		0,       //gpo2_tx_delay_us *** adi,gpo2-tx-delay-us
		0,       //gpo3_rx_delay_us *** adi,gpo3-rx-delay-us
		0,       //gpo3_tx_delay_us *** adi,gpo3-tx-delay-us
		/* Tx Monitor Control */
		37000,   //low_high_gain_threshold_mdB *** adi,txmon-low-high-thresh
		0,       //low_gain_dB *** adi,txmon-low-gain
		24,      //high_gain_dB *** adi,txmon-high-gain
		0,       //tx_mon_track_en *** adi,txmon-dc-tracking-enable
		0,       //one_shot_mode_en *** adi,txmon-one-shot-mode-enable
		511,     //tx_mon_delay *** adi,txmon-delay
		8192,    //tx_mon_duration *** adi,txmon-duration
		2,       //tx1_mon_front_end_gain *** adi,txmon-1-front-end-gain
		2,       //tx2_mon_front_end_gain *** adi,txmon-2-front-end-gain
		48,      //tx1_mon_lo_cm *** adi,txmon-1-lo-cm
		48,      //tx2_mon_lo_cm *** adi,txmon-2-lo-cm
		/* GPIO definitions */
		{
			.number = 100,
			.platform_ops = &genode_gpio_ops,
			.extra = NULL
		},		//gpio_resetb *** reset-gpios
		/* MCS Sync */
		{
			.number = -1,
			.platform_ops = &genode_gpio_ops,
			.extra = NULL
		},		//gpio_sync *** sync-gpios

		{
			.number = -1,
			.platform_ops = &genode_gpio_ops,
			.extra = NULL
		},		//gpio_cal_sw1 *** cal-sw1-gpios

		{
			.number = -1,
			.platform_ops = &genode_gpio_ops,
			.extra = NULL
		},		//gpio_cal_sw2 *** cal-sw2-gpios

		{
			.device_id = 0,
			.max_speed_hz = 0,
			.chip_select = 0,
			.mode = SPI_MODE_1,
			.bit_order = (spi_bit_order)0,
			.platform_ops = &genode_spi_ops,
			.extra = NULL
		},

		/* External LO clocks */
		NULL,  //(*ad9361_rfpll_ext_recalc_rate)()
		NULL,  //(*ad9361_rfpll_ext_round_rate)()
		NULL,  //(*ad9361_rfpll_ext_set_rate)()
		&rx_adc_init,   // *rx_adc_init
		&tx_dac_init,   // *tx_dac_init
	};

	template<typename T>
	void apply_value(Genode::Xml_node const & node, const char * name, T & target)
	{
		node.with_sub_node(name, [&] (Genode::Xml_node const & param) {
			target = param.attribute_value("value", target);
		});
	}

	void apply_value(Genode::Xml_node const & node, const char * name, int32_t & target)
	{
		node.with_sub_node(name, [&] (Genode::Xml_node const & param) {
			target = (int32_t)param.attribute_value<unsigned>("value", target);
		});
	}

	void apply_value(Genode::Xml_node const & node, const char * name, int8_t & target)
	{
		node.with_sub_node(name, [&] (Genode::Xml_node const & param) {
			target = (int8_t)param.attribute_value<unsigned>("value", target);
		});
	}

	template<typename T>
	void apply_boolean(Genode::Xml_node const & node, const char * name, T & target)
	{
		node.with_sub_node(name, [&] (Genode::Xml_node const & param) {
			target = param.attribute_value("value", (bool)target);
		});
	}

	template<typename T>
	void apply_array(Genode::Xml_node const & node, const char * name, T & target)
	{
		node.with_sub_node(name, [&] (Genode::Xml_node const & param) {
			size_t i = 0;
			param.for_each_sub_node("field", [&] (Genode::Xml_node const & field) {
				if (i >= sizeof(target))
					 return;

				target[i] = field.attribute_value("value", target[i]);

				++i;
			});
		});
	}

};

Ad::Ad9361_config &Ad::Ad9361::_ad9361_config()
{
	/**
	 * To hide the implementation details from the header files, make
	 * this static rather than a member.
	 */
	static Ad9361_config cfg;

	return cfg;
}

void Ad::Ad9361::_update_init_params(Xml_node const & node)
{
	Ad9361_config &cfg = _ad9361_config();
	AD9361_InitParam &p = cfg.default_init_param;

	cfg.apply_value  (node, "digital-interface-tune-skip-mode",  p.digital_interface_tune_skip_mode);
	cfg.apply_boolean(node, "pp-tx-swap-enable",                 p.pp_tx_swap_enable);
	cfg.apply_boolean(node, "pp-rx-swap-enable",                 p.pp_rx_swap_enable);
	cfg.apply_boolean(node, "rx-frame-pulse-mode-enable",        p.rx_frame_pulse_mode_enable);
	cfg.apply_value  (node, "rx-data-delay",                     p.rx_data_delay);
	cfg.apply_value  (node, "tx-fb-clock-delay",                 p.tx_fb_clock_delay);
	cfg.apply_boolean(node, "full-port-enable",                  p.full_port_enable);
	cfg.apply_boolean(node, "xo-disable-use-ext-refclk-enable",  p.xo_disable_use_ext_refclk_enable);
	cfg.apply_boolean(node, "two_rx_two_tx_mode_enable",         p.two_rx_two_tx_mode_enable);
	cfg.apply_value  (node, "one-rx-one-tx-mode-use-rx-num",     p.one_rx_one_tx_mode_use_rx_num);
	cfg.apply_value  (node, "one-rx-one-tx-mode-use-tx-num",     p.one_rx_one_tx_mode_use_tx_num);
	cfg.apply_boolean(node, "frequency-division-duplex-mode-enable",
	                       p.frequency_division_duplex_mode_enable);
	cfg.apply_value  (node, "rx-rf-port-input-select",           p.rx_rf_port_input_select);
	cfg.apply_value  (node, "tx-rf-port-input-select",           p.tx_rf_port_input_select);
	cfg.apply_value  (node, "tx-attenuation-mdB",                p.tx_attenuation_mdB);
	cfg.apply_boolean(node, "tx-lo-powerdown-managed-enable",    p.tx_lo_powerdown_managed_enable);
	cfg.apply_value  (node, "rf-rx-bandwidth-hz",                p.rf_rx_bandwidth_hz);
	cfg.apply_value  (node, "rf-tx-bandwidth-hz",                p.rf_tx_bandwidth_hz);
	cfg.apply_value  (node, "rx-synthesizer-frequency-hz",       p.rx_synthesizer_frequency_hz);
	cfg.apply_value  (node, "tx-synthesizer-frequency-hz",       p.tx_synthesizer_frequency_hz);
	cfg.apply_array  (node, "rx-path-clock-frequencies",         p.rx_path_clock_frequencies);
	cfg.apply_array  (node, "tx-path-clock-frequencies",         p.tx_path_clock_frequencies);
	cfg.apply_value  (node, "gc-rx1-mode",                       p.gc_rx1_mode);
	cfg.apply_value  (node, "gc-rx2-mode",                       p.gc_rx2_mode);
	cfg.apply_value  (node, "gc-adc-ovr-sample-size",            p.gc_adc_small_overload_thresh);
	cfg.apply_value  (node, "gc-adc-ovr-sample-size",            p.gc_adc_large_overload_thresh);
	cfg.apply_value  (node, "gc-lmt-overload-high-thresh",       p.gc_lmt_overload_high_thresh);
	cfg.apply_value  (node, "gc-lmt-overload-low-thresh",        p.gc_lmt_overload_low_thresh);
	cfg.apply_value  (node, "gc-dec-pow-measurement-duration",   p.gc_dec_pow_measurement_duration);
	cfg.apply_value  (node, "gc-low-power-thresh",               p.gc_low_power_thresh);
	cfg.apply_value  (node, "mgc-inc-gain-step",                 p.mgc_inc_gain_step);
	cfg.apply_value  (node, "mgc-dec-gain-step",                 p.mgc_dec_gain_step);
	cfg.apply_value  (node, "mgc-split-table-ctrl-inp-gain-mode",
	                       p.mgc_split_table_ctrl_inp_gain_mode);
	cfg.apply_value  (node, "agc-attack-delay-extra-margin-us",  p.agc_attack_delay_extra_margin_us);
	cfg.apply_value  (node, "agc-outer-thresh-high",             p.agc_outer_thresh_high);
	cfg.apply_value  (node, "agc-outer-thresh-high-dec-steps",   p.agc_outer_thresh_high_dec_steps);
	cfg.apply_value  (node, "agc-inner-thresh-high",             p.agc_inner_thresh_high);
	cfg.apply_value  (node, "agc-inner-thresh-high-dec-steps",   p.agc_inner_thresh_high_dec_steps);
	cfg.apply_value  (node, "agc-inner-thresh-low",              p.agc_inner_thresh_low);
	cfg.apply_value  (node, "agc-inner-thresh-low-dec-steps",    p.agc_inner_thresh_low_inc_steps);
	cfg.apply_value  (node, "agc-outer-thresh-low",              p.agc_outer_thresh_low);
	cfg.apply_value  (node, "agc-outer-thresh-low-inc-steps",    p.agc_outer_thresh_low_inc_steps);
	cfg.apply_value  (node, "agc-adc-small-overload-exceed-counter",
	                       p.agc_adc_small_overload_exceed_counter);
	cfg.apply_value  (node, "agc-adc-large-overload-exceed-counter",
	                       p.agc_adc_large_overload_exceed_counter);
	cfg.apply_value  (node, "agc-adc-large-overload-inc-steps",  p.agc_adc_large_overload_inc_steps);
	cfg.apply_value  (node, "agc-lmt-overload-large-exceed-counter",
	                       p.agc_lmt_overload_large_exceed_counter);
	cfg.apply_value  (node, "agc-lmt-overload-small-exceed-counter",
	                       p.agc_lmt_overload_small_exceed_counter);
	cfg.apply_value  (node, "agc-lmt-overload-large-inc-steps",  p.agc_lmt_overload_large_inc_steps);
	cfg.apply_value  (node, "agc-gain-update-interval-us",       p.agc_gain_update_interval_us);
	cfg.apply_value  (node, "fagc-dec-pow-measurement-duration", p.fagc_dec_pow_measuremnt_duration);
	cfg.apply_value  (node, "fagc-lp-thresh-increment-steps",    p.fagc_lp_thresh_increment_steps);
	cfg.apply_value  (node, "fagc-lp-thresh-increment-time",     p.fagc_lp_thresh_increment_time);
	cfg.apply_value  (node, "fagc-energy-lost-stronger-sig-gain-lock-exit-cnt",
	                       p.fagc_energy_lost_stronger_sig_gain_lock_exit_cnt);
	cfg.apply_value  (node, "fagc-final-overrange-count",        p.fagc_final_overrange_count);
	cfg.apply_value  (node, "fagc-gain-index-type-after-exit-rx-mode",
	                       p.fagc_gain_index_type_after_exit_rx_mode);
	cfg.apply_value  (node, "fagc-lmt-final-settling-steps",     p.fagc_lmt_final_settling_steps);
	cfg.apply_value  (node, "fagc-lock-level-gain-increase-upper-limit",
	                       p.fagc_lock_level_gain_increase_upper_limit);
	cfg.apply_boolean(node, "fagc-lock-level-lmt-gain-increase-enable",
	                       p.fagc_lock_level_lmt_gain_increase_en);
	cfg.apply_value  (node, "fagc-lpf-final-settling-steps",     p.fagc_lpf_final_settling_steps);
	cfg.apply_value  (node, "fagc-optimized-gain-offset",        p.fagc_optimized_gain_offset);
	cfg.apply_value  (node, "fagc-power-measurement-duration-in-state5",
	                       p.fagc_power_measurement_duration_in_state5);
	cfg.apply_boolean(node, "fagc-rst-gla-engergy-lost-goto-optim-gain-enable",
	                       p.fagc_rst_gla_engergy_lost_goto_optim_gain_en);
	cfg.apply_value  (node, "fagc-rst-gla-engergy-lost-sig-thresh-below-ll",
	                       p.fagc_rst_gla_engergy_lost_sig_thresh_below_ll);
	cfg.apply_boolean(node, "fagc-rst-gla-engergy-lost-sig-thresh-exceeded-enable",
	                       p.fagc_rst_gla_engergy_lost_sig_thresh_exceeded_en);
	cfg.apply_value  (node, "fagc-rst-gla-if-en-agc-pulled-high-mode",
	                       p.fagc_rst_gla_if_en_agc_pulled_high_mode);
	cfg.apply_boolean(node, "fagc-rst-gla-large-adc-overload-enable",
	                       p.fagc_rst_gla_large_adc_overload_en);
	cfg.apply_boolean(node, "fagc-rst-gla-large-lmt-overload-enable",
	                       p.fagc_rst_gla_large_lmt_overload_en);
	cfg.apply_value  (node, "fagc-rst-gla-stronger-sig-thresh-above-ll",
	                       p.fagc_rst_gla_stronger_sig_thresh_above_ll);
	cfg.apply_boolean(node, "fagc-rst-gla-stronger-sig-thresh-exceeded-enable",
	                       p.fagc_rst_gla_stronger_sig_thresh_exceeded_en);
	cfg.apply_value  (node, "fagc-state-wait-time-ns",           p.fagc_state_wait_time_ns);
	cfg.apply_boolean(node, "fagc-use-last-lock-level-for-set-gain-enable",
	                       p.fagc_use_last_lock_level_for_set_gain_en);
	cfg.apply_value  (node, "rssi-restart-mode",                 p.rssi_restart_mode);
	cfg.apply_value  (node, "rssi-delay",                        p.rssi_delay);
	cfg.apply_value  (node, "rssi-wait",                         p.rssi_wait);
	cfg.apply_value  (node, "rssi-duration",                     p.rssi_duration);
	cfg.apply_value  (node, "ctrl-outs-index",                   p.ctrl_outs_index);
	cfg.apply_value  (node, "ctrl-outs-enable-mask",             p.ctrl_outs_enable_mask);
	cfg.apply_value  (node, "temp-sense-measurement-interval-ms",
	                       p.temp_sense_measurement_interval_ms);
	cfg.apply_value  (node, "temp-sense-offset-signed",          p.temp_sense_offset_signed);
	cfg.apply_boolean(node, "temp-sense-periodic-measurement-enable",
	                       p.temp_sense_periodic_measurement_enable);
	cfg.apply_boolean(node, "aux-dac-manual-mode-enable",        p.aux_dac_manual_mode_enable);
	cfg.apply_value  (node, "aux-dac1-default-value-mV",         p.aux_dac1_default_value_mV);
	cfg.apply_value  (node, "aux-dac1-rx-delay-us",              p.aux_dac1_rx_delay_us);
	cfg.apply_value  (node, "aux-dac1-tx-delay-us",              p.aux_dac1_tx_delay_us);
	cfg.apply_value  (node, "aux-dac2-default-value-mV",         p.aux_dac2_default_value_mV);
	cfg.apply_value  (node, "aux-dac2-rx-delay-us",              p.aux_dac2_rx_delay_us);
	cfg.apply_value  (node, "aux-dac2-tx-delay-us",              p.aux_dac2_tx_delay_us);
}

void Ad::Ad9361::_restart_driver(Xml_node const & config)
{
	using Device = ::Platform::Device;

	enum { BUF_SIZE = 1500 };

	try {
		/* construct dmac devices */
		_dmac_rx.acquire(_env, _platform, BUF_SIZE);
		_dmac_tx.acquire(_env, _platform, BUF_SIZE);

		/* apply config */
		Libc::with_libc([&] () {
			Ad9361_config &cfg = _ad9361_config();
	
			/* set device addresses */
			cfg.rx_adc_init.base = Ad::platform().addr_by_name(cfg.rx_adc_init.name);
			cfg.tx_dac_init.base = Ad::platform().addr_by_name(cfg.tx_dac_init.name);
	
			/* revert previous initialisation */
			if (cfg.ad9361_phy) {
				ad9361_remove(cfg.ad9361_phy);
				cfg.ad9361_phy = 0;
			}
	
			/* update default_init_params from config */
			config.with_sub_node("adi", [&] (Xml_node const & node) {
				_update_init_params(node);
			});
	
			/* initialize ad9361 */
			int32_t status = ad9361_init(&cfg.ad9361_phy, &cfg.default_init_param);
			if (status < 0) {
				error("ad9361_init() failed");
				_state = State::STOPPED;
				return;
			}
	
			/* TODO FIR config */
	
			/* DAC init */
			axi_dac_init(&cfg.ad9361_phy->tx_dac, &cfg.tx_dac_init);
			axi_dac_set_datasel(cfg.ad9361_phy->tx_dac, -1, AXI_DAC_DATA_SEL_DMA);
	
			/**
			 * set loopback mode according to <config>
			 */
	
			Device regif_device { _platform, Device::Type { "regif" } };
			Regif  regif        { regif_device };
	
			String<32> loopback = config.attribute_value("loopback", String<32>(""));
			if        (loopback == "dmac") {
				log("Enabling DMA TX->RX loopback");
				regif.enable_loopback();
	
			} else if (loopback == "txrx") {
				log("Enabling FPGA-internal TX->RX loopback");
				regif.disable_loopback();
				ad9361_bist_loopback(cfg.ad9361_phy, 0);
	
				/* not implemented in no-OS */
				struct axiadc_converter *conv   = cfg.ad9361_phy->adc_conv;
				struct axi_adc          *rx_adc = cfg.ad9361_phy->rx_adc;
				const uint32_t           addr   = 0x418;
				for (int32_t chan = 0; chan < conv->chip_info->num_channels; chan++) {
					uint32_t reg;
					axi_adc_read(rx_adc, addr + (chan) * 0x40, &reg);
					reg |= 0x1;
					axi_adc_write(rx_adc, addr + (chan) * 0x40, reg);
					warning("Writing ", Hex(reg), " to ", Hex(rx_adc->base + addr + (chan) * 0x40));
				}
	
			} else if (loopback == "rxtx") {
				log("Enabling FPGA-internal RX->TX loopback");
				regif.disable_loopback();
				ad9361_bist_loopback(cfg.ad9361_phy, 2);
	
			} else if (loopback == "rf") {
				log("Enabling ad9361 TX->RX loopback");
				regif.disable_loopback();
				ad9361_bist_loopback(cfg.ad9361_phy, 1);
	
			} else {
				/* disable all loopback options */
				regif.disable_loopback();
				ad9361_bist_loopback(cfg.ad9361_phy, 0);
			}

			_state = State::STARTED;
		});

	} catch (...) {
		error("Unable to start driver");
		_state = State::STOPPED;
	}
}


Ad::Ad9361::State Ad::Ad9361::update_config(Xml_node const & config)
{
	/**
	 * restart driver because the config changed
	 */
	_restart_driver(config);
	return _state;
}


Ad::Ad9361::State Ad::Ad9361::update_devices(Xml_node const & config)
{
	/**
	 * only try restarting driver if it was not started yet
	 */
	if (_state == State::STOPPED)
		_restart_driver(config);

	return _state;
}


Ad::Ad9361::Ad9361(Genode::Env &env)
: _env(env),
  _platform(Ad::platform(&env).platform())
{ }
