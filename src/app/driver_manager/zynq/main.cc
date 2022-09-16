/*
 * \brief  Driver manager for Zynq (loads user-defined drivers after FPGA reconfiguration)
 * \author Johannes Schlatow
 * \date   2021-11-15
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <base/registry.h>
#include <base/heap.h>
#include <base/attached_rom_dataspace.h>
#include <os/reporter.h>
#include <rm_session/rm_session.h>
#include <io_mem_session/io_mem_session.h>
#include <timer_session/timer_session.h>
#include <log_session/log_session.h>
#include <platform_session/platform_session.h>
#include <nic_session/nic_session.h>
#include <uplink_session/uplink_session.h>
#include <pin_control_session/pin_control_session.h>
#include <pin_state_session/pin_state_session.h>
#include <irq_session/irq_session.h>

namespace Driver_manager {
	using namespace Genode;
	struct Main;
	struct User_logic_driver;
}


struct Driver_manager::User_logic_driver : Registry<User_logic_driver>::Element
{
	using Name = String<64>;

	Xml_node driver_xml;
	bool     start { false };

	static void _gen_forwarded_service(Xml_generator & xml,
	                                   Name    const & service,
	                                   Name    const & child)
	{
		xml.node("service", [&] () {
			xml.attribute("name", service);
			xml.node("default-policy", [&] () {
				xml.node("child", [&] () {
					xml.attribute("name", child);
				});
			});
		});
	}

	void generate_start_node(Xml_generator &xml) const
	{
		if (!start) return;

		Name const name = driver_xml.attribute_value("name", Name());

		try {
			Xml_node provides = driver_xml.sub_node("provides");

			/* generate forward rules for every provided service */
			provides.for_each_sub_node("service", [&](Xml_node service) {
				_gen_forwarded_service(xml,
				                       service.attribute_value("name", Name()),
				                       name);
			});

		} catch (...) { }

		/* copy content from driver_xml */
		xml.node("start", [&] () {
			xml.attribute("name", name);
			driver_xml.with_raw_content([&] (char const *start, Genode::size_t length) {
				xml.append(start, length); });
		});
	}

	User_logic_driver(Registry<User_logic_driver> &registry, Xml_node const &xml)
	: Registry<User_logic_driver>::Element(registry, *this),
	  driver_xml(xml)
	{ }
};


struct Driver_manager::Main
{
	Env                   &_env;
	Heap                   _heap    { _env.ram(), _env.rm() };

	Attached_rom_dataspace _devices { _env, "devices" };
	Attached_rom_dataspace _config  { _env, "config" };

	Reporter _init_config    { _env, "config", "init.config", 8192 };

	Registry<User_logic_driver> _user_logic_drivers { };

	void _handle_devices_update();

	Signal_handler<Main> _devices_update_handler {
		_env.ep(), *this, &Main::_handle_devices_update };

	static void _gen_parent_service_xml(Xml_generator &xml, char const *name)
	{
		xml.node("service", [&] () { xml.attribute("name", name); });
	};

	void _apply_config();

	void _generate_init_config    (Reporter &) const;

	Main(Env &env) : _env(env)
	{
		_init_config.enabled(true);

		_devices.sigh(_devices_update_handler);

		_apply_config();

		_generate_init_config(_init_config);

		_handle_devices_update();
	}
};


void Driver_manager::Main::_apply_config()
{
	_config.update();

	if (!_config.valid())
		return;

	/* clear registry */
	_user_logic_drivers.for_each([&] (User_logic_driver &d) { destroy(_heap, &d); });

	/* fill registry from config */
	_config.xml().for_each_sub_node("driver", [&] (Xml_node driver) {
		if (!driver.has_attribute("device"))
			return;
		if (!driver.has_attribute("name"))
			return;

		new (_heap) User_logic_driver(_user_logic_drivers, driver);
	});
}


void Driver_manager::Main::_handle_devices_update()
{
	_devices.update();

	if (!_devices.valid())
		return;

	bool changed = false;

	_user_logic_drivers.for_each([&] (User_logic_driver &d) {

		bool found = false;
		_devices.xml().for_each_sub_node([&] (Xml_node device) {
			if (device      .attribute_value("type",   User_logic_driver::Name()) ==
			    d.driver_xml.attribute_value("device", User_logic_driver::Name()))
			{
				changed = changed || !d.start;
				d.start = true;
				found = true;
			}
		});

		if (!found) {
			changed = changed || d.start;
			d.start = false;
		}

	});

	if (changed)
		_generate_init_config(_init_config);

}


void Driver_manager::Main::_generate_init_config(Reporter &init_config) const
{
	Reporter::Xml_generator xml(init_config, [&] () {

		xml.attribute("verbose", false);

		xml.node("heartbeat", [&] () { xml.attribute("rate_ms", 2500); });

		xml.node("parent-provides", [&] () {
			_gen_parent_service_xml(xml, Rom_session::service_name());
			_gen_parent_service_xml(xml, Io_mem_session::service_name());
			_gen_parent_service_xml(xml, Cpu_session::service_name());
			_gen_parent_service_xml(xml, Pd_session::service_name());
			_gen_parent_service_xml(xml, Rm_session::service_name());
			_gen_parent_service_xml(xml, Log_session::service_name());
			_gen_parent_service_xml(xml, Timer::Session::service_name());
			_gen_parent_service_xml(xml, Platform::Session::service_name());
			_gen_parent_service_xml(xml, Report::Session::service_name());
			_gen_parent_service_xml(xml, Uplink::Session::service_name());
			_gen_parent_service_xml(xml, Nic::Session::service_name());
			_gen_parent_service_xml(xml, Pin_state::Session::service_name());
			_gen_parent_service_xml(xml, Pin_control::Session::service_name());
			_gen_parent_service_xml(xml, Irq_session::service_name());
		});

		xml.node("default-route", [&] () {
			xml.node("any-service", [&] () {
				xml.node("parent", [&] () { });
			});
		});

		xml.node("default", [&] () {
			xml.attribute("caps", 300);
		});

		_user_logic_drivers.for_each([&] (User_logic_driver const &d) {
			d.generate_start_node(xml);
		});
	});
}


void Component::construct(Genode::Env &env) { static Driver_manager::Main main(env); }
