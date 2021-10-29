#
# Build
#

if {[expr ![have_board zynq_usrp_e31x]]} {
	puts "Run script is not supported on this platform."
	exit 0
}

# generic components
set build_components {
	core init timer
	drivers/platform
}

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
create_boot_directory


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

		<start name="platform_drv" caps="100">
			<resource name="RAM" quantum="1M"/>
			<provides> <service name="Platform"/> </provides>
			<route> <any-service> <parent/> </any-service> </route>
			<config>
				<device name="spi0" type="zynq-spi">
					<io_mem address="0xe0006000" size="0x1000"/>
					<property name="input-clock" value="166666665"/>
					<property name="spi-max-frequency" value="0x989680"/>
					<property name="spi-cpha" value="yes"/>
				</device>
				<device name="regif" type="regif">
					<io_mem address="0x43c20000" size="0x10000"/>;
				</device>
				<device name="zynq-gpio" type="zynq-gpio">
					<io_mem address="0xe000a000" size="0x1000"/>;
				</device>
				<device name="axi-spi" type="spi">
					<io_mem address="0x43c00000" size="0x10000"/>;
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
					<device name="regif"/>
					<device name="zynq-gpio"/>
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
			</config>
		</start>
	</config>
}

install_config $config


#
# Boot modules
#

# generic modules
set boot_modules {
	core libc.lib.so vfs.lib.so
	init timer platform_drv
}
append boot_modules $binary_name

build_boot_image $boot_modules

append qemu_args " -nographic "
run_genode_until forever

