#
# Build
#

if {[expr ![have_board zynq_usrp_e31x]]} {
	puts "Run script is not supported on this platform."
	exit 0
}

create_boot_directory

import_from_depot [depot_user]/src/[base_src] \
                  [depot_user]/src/init \
                  [depot_user]/src/vfs \
                  [depot_user]/src/libc \
                  [depot_user]/src/zynq_platform_drv

# generic components
set build_components {}

if {$binary_name == ""} {
	puts "binary_name is unspecified"
	exit 0
}

if {$test_component == ""} {
	puts "test_component is unspecified"
	exit 0
}

append build_components " $test_component "

build $build_components


#
# Config
#

set config  {
	<config verbose="yes">
		<parent-provides>
			<service name="ROM"/>
			<service name="PD"/>
			<service name="CPU"/>
			<service name="LOG"/>
			<service name="IO_MEM"/>
			<service name="IRQ"/>
		</parent-provides>
		<default-route>
			<any-service> <parent/> <any-child/> </any-service>
		</default-route>
		<default caps="200"/>

		<start name="timer">
			<resource name="RAM" quantum="1M"/>
			<provides><service name="Timer"/></provides>
		</start>

		<start name="platform_drv" caps="100" managing_system="yes">
			<binary name="zynq_platform_drv"/>
			<resource name="RAM" quantum="1M"/>
			<provides> <service name="Platform"/> </provides>
			<route> <any-service> <parent/> <any-child/> </any-service> </route>
			<config devices_rom="config">
				<device name="spi0" type="zynq-spi">
					<io_mem address="0xe0006000" size="0x1000"/>
					<irq number="58"/>
					<property name="spi-max-frequency" value="0x989680"/>
					<property name="spi-cpha" value="yes"/>
					<clock name="spi0"/>
				</device>
				<device name="gpio0" type="zynq-gpio">
					<io_mem address="0xe000a000" size="0x1000"/>;
				</device>
				<device name="axi-dmac-rx" type="rx_dmac">
					<io_mem address="0x7c400000" size="0x10000"/>;
					<irq number="89"/>
				</device>
				<device name="axi-dmac-tx" type="tx_dmac">
					<io_mem address="0x7c420000" size="0x10000"/>;
					<irq number="88"/>
				</device>
				<device name="axi-adc-rx" type="cf-ad9361-lpc">
					<io_mem address="0x79020000" size="0x4000"/>;
				</device>
				<device name="axi-dac-tx" type="cf-ad9361-dds-core-lpc">
					<io_mem address="0x79024000" size="0x4000"/>;
				</device>
				<policy label="ad9361 -> " info="yes">
					<device name="axi-dmac-rx"/>
					<device name="axi-dmac-tx"/>
					<device name="axi-adc-rx"/>
					<device name="axi-dac-tx"/>
					<device name="gpio0"/>
					<device name="spi0"/>
				</policy>
			</config>
		</start>
}
append config {
		<start name="ad9361">
			<binary name="} $binary_name {"/>
			<resource name="RAM" quantum="2M"/>
			<config>
				<libc stdout="/dev/log" stderr="/dev/log"/>
				<vfs>
					<dir name="dev">
						<log/>
					</dir>
				</vfs>
				<adi>
					<digital-interface-tune-skip-mode value="0x00"/>
					<pp-tx-swap-enable value="yes"/>
					<pp-rx-swap-enable value="yes"/>
					<rx-frame-pulse-mode-enable value="yes"/>
					<rx-data-delay value="0x04"/>
					<tx-fb-clock-delay value="0x07"/>
					<full-port-enable value="yes"/>
					<xo-disable-use-ext-refclk-enable value="yes"/>
					<one-rx-one-tx-mode-use-rx-num value="0x00"/>
					<one-rx-one-tx-mode-use-tx-num value="0x00"/>
					<frequency-division-duplex-mode-enable value="yes"/>
					<rx-rf-port-input-select value="0x00"/>
					<tx-rf-port-input-select value="0x00"/>
					<tx-attenuation-mdB value="0x2710"/>
					<tx-lo-powerdown-managed-enable value="yes"/>
					<rf-rx-bandwidth-hz value="0x112a880"/>
					<rf-tx-bandwidth-hz value="0x112a880"/>
					<rx-synthesizer-frequency-hz value="0x8f0d1800"/>
					<tx-synthesizer-frequency-hz value="0x92080880"/>
					<rx-path-clock-frequencies>
						<field value="0x3a980000"/>
						<field value="0xea60000"/>
						<field value="0x7530000"/>
						<field value="0x3a98000"/>
						<field value="0x1d4c000"/>
						<field value="0x1d4c000"/>
					</rx-path-clock-frequencies>
					<tx-path-clock-frequencies>
						<field value="0x3a980000"/>
						<field value="0x7530000"/>
						<field value="0x7530000"/>
						<field value="0x3a98000"/>
						<field value="0x1d4c000"/>
						<field value="0x1d4c000"/>
					</tx-path-clock-frequencies>
					<gc-rx1-mode value="0x02"/>
					<gc-rx2-mode value="0x02"/>
					<gc-adc-ovr-sample-size value="0x04"/>
					<gc-adc-small-overload-thresh value="0x2f"/>
					<gc-adc-large-overload-thresh value="0x3a"/>
					<gc-lmt-overload-high-thresh value="0x320"/>
					<gc-lmt-overload-low-thresh value="0x2c0"/>
					<gc-dec-pow-measurement-duration value="0x2000"/>
					<gc-low-power-thresh value="0x18"/>
					<mgc-inc-gain-step value="0x02"/>
					<mgc-dec-gain-step value="0x02"/>
					<mgc-split-table-ctrl-inp-gain-mode value="0x00"/>
					<agc-attack-delay-extra-margin-us value="0x01"/>
					<agc-outer-thresh-high value="0x05"/>
					<agc-outer-thresh-high-dec-steps value="0x02"/>
					<agc-inner-thresh-high value="0x0a"/>
					<agc-inner-thresh-high-dec-steps value="0x01"/>
					<agc-inner-thresh-low value="0x0c"/>
					<agc-inner-thresh-low-inc-steps value="0x01"/>
					<agc-outer-thresh-low value="0x12"/>
					<agc-outer-thresh-low-inc-steps value="0x02"/>
					<agc-adc-small-overload-exceed-counter value="0x0a"/>
					<agc-adc-large-overload-exceed-counter value="0x0a"/>
					<agc-adc-large-overload-inc-steps value="0x02"/>
					<agc-lmt-overload-large-exceed-counter value="0x0a"/>
					<agc-lmt-overload-small-exceed-counter value="0x0a"/>
					<agc-lmt-overload-large-inc-steps value="0x02"/>
					<agc-gain-update-interval-us value="0x3e8"/>
					<fagc-dec-pow-measurement-duration value="0x40"/>
					<fagc-lp-thresh-increment-steps value="0x01"/>
					<fagc-lp-thresh-increment-time value="0x05"/>
					<fagc-energy-lost-stronger-sig-gain-lock-exit-cnt value="0x08"/>
					<fagc-final-overrange-count value="0x03"/>
					<fagc-gain-index-type-after-exit-rx-mode value="0x00"/>
					<fagc-lmt-final-settling-steps value="0x01"/>
					<fagc-lock-level value="0x0a"/>
					<fagc-lock-level-gain-increase-upper-limit value="0x05"/>
					<fagc-lock-level-lmt-gain-increase-enable value="yes"/>
					<fagc-lpf-final-settling-steps value="0x01"/>
					<fagc-optimized-gain-offset value="0x05"/>
					<fagc-power-measurement-duration-in-state5 value="0x40"/>
					<fagc-rst-gla-engergy-lost-goto-optim-gain-enable value="yes"/>
					<fagc-rst-gla-engergy-lost-sig-thresh-below-ll value="0x0a"/>
					<fagc-rst-gla-engergy-lost-sig-thresh-exceeded-enable value="yes"/>
					<fagc-rst-gla-if-en-agc-pulled-high-mode value="0x00"/>
					<fagc-rst-gla-large-adc-overload-enable value="yes"/>
					<fagc-rst-gla-large-lmt-overload-enable value="yes"/>
					<fagc-rst-gla-stronger-sig-thresh-above-ll value="0x0a"/>
					<fagc-rst-gla-stronger-sig-thresh-exceeded-enable value="yes"/>
					<fagc-state-wait-time-ns value="0x104"/>
					<fagc-use-last-lock-level-for-set-gain-enable value="yes"/>
					<rssi-restart-mode value="0x03"/>
					<rssi-delay value="0x01"/>
					<rssi-wait value="0x01"/>
					<rssi-duration value="0x3e8"/>
					<ctrl-outs-index value="0x00"/>
					<ctrl-outs-enable-mask value="0xff"/>
					<temp-sense-measurement-interval-ms value="0x3e8"/>
					<temp-sense-offset-signed value="0xce"/>
					<temp-sense-periodic-measurement-enable value="yes"/>
					<aux-dac-manual-mode-enable value="yes"/>
					<aux-dac1-default-value-mV value="0x00"/>
					<aux-dac1-rx-delay-us value="0x00"/>
					<aux-dac1-tx-delay-us value="0x00"/>
					<aux-dac2-default-value-mV value="0x00"/>
					<aux-dac2-rx-delay-us value="0x00"/>
					<aux-dac2-tx-delay-us value="0x00"/>
					<rx-rf-port-input-select-lock-enable value="yes"/>
					<tx-rf-port-input-select-lock-enable value="yes"/>
				</adi>
			</config>
		</start>
	</config>
}

install_config $config


#
# Boot modules
#

# generic modules
set boot_modules { }
append boot_modules $binary_name

build_boot_image $boot_modules

append qemu_args " -nographic "
run_genode_until forever

