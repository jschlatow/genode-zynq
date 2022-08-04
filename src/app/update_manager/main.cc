/*
 * \brief  Manage depot installation, deployment and rollback
 * \author Johannes Schlatow
 * \date   2022-07-29
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/heap.h>
#include <base/component.h>
#include <base/attached_rom_dataspace.h>
#include <os/reporter.h>
#include <timer_session/connection.h>

/* local includes */
#include "apps.h"
#include "download_queue.h"
#include "report_service.h"

namespace Update_manager {
	using Genode::Expanding_reporter;

	struct Main;
}


struct Update_manager::Main : Deploy
{
	Genode::Env       &_env;

	Genode::Heap       _heap              { _env.ram(), _env.rm() };

	Download_queue     _download_queue    { _heap };

	Timer::Connection  _timer             { _env };

	Report::Pool       _state_report_pool { };

	Apps               _apps              { _env, _heap, _timer, *this, _download_queue, _state_report_pool };

	Report::Root       _report_root       { _env, _heap, _state_report_pool };

	Genode::Attached_rom_dataspace _config { _env, "config" };

	Genode::Signal_handler<Main> _config_handler =
		{ _env.ep(), *this, &Main::_handle_config };

	Genode::Attached_rom_dataspace _download_state { _env, "download_state" };

	Genode::Signal_handler<Main> _download_state_handler =
		{ _env.ep(), *this, &Main::_handle_download_state };

	Expanding_reporter _installation_reporter      { _env, "installation", "installation" };
	Expanding_reporter _deploy_reporter            { _env, "config",       "deploy.config" };
	Expanding_reporter _report_rom_config_reporter { _env, "config",       "report_rom.config" };

	void _gen_deploy_config()
	{
		_deploy_reporter.generate([&] (Xml_generator &xml) {
			Xml_node config_xml = _config.xml();

			xml.attribute("arch", config_xml.attribute_value("arch", Genode::String<8>()));

			auto append_xml_node = [&] (Xml_node const &node) {
				xml.append("\t");
				node.with_raw_node([&] (char const *start, Genode::size_t length) {
					xml.append(start, length); });
				xml.append("\n");
			};

			/* copy <static> from config*/
			config_xml.with_sub_node("static", [&] (Xml_node const &node) {
				append_xml_node(node); });

			/* copy <common_routes> from config*/
			config_xml.with_sub_node("common_routes", [&] (Xml_node const &node) {
				append_xml_node(node); });

			_apps.gen_start_entries(xml);
		});
	}

	void _handle_config()
	{
		/* cleanup download queue to retry failed downloads */
		_download_queue.remove_failed_downloads();

		/* apply new config */
		_config.update();
		_apps.apply_config(_config.xml());

		/* generate config for monitor_report_rom */
		_report_rom_config_reporter.generate([&] (Xml_generator &xml) {
			xml.attribute("verbose", "yes");
			_apps.gen_monitor_report_config(xml);
		});

		/* generate installation report from download queue */
		_installation_reporter.generate([&] (Xml_generator &xml) {
			xml.attribute("arch", _config.xml().attribute_value("arch", Genode::String<8>()));
			_download_queue.gen_installation_entries(xml);
		});
	}

	void _handle_download_state()
	{
		_download_state.update();

		_download_queue.apply_update_state(_download_state.xml());

		/* wait until every download has finished */
		if (_download_queue.any_active_download())
			return;

		_apps.apply_installation();

		_gen_deploy_config();
	}

	Main(Genode::Env &env) : _env(env)
	{
		_env.parent().announce(_env.ep().manage(_report_root));

		_config.sigh(_config_handler);
		_download_state.sigh(_download_state_handler);

		_handle_config();
	}


	/**
	 * Deploy interface
	 */
	void trigger() override { _gen_deploy_config(); }
};


void Component::construct(Genode::Env &env) { static Update_manager::Main main(env); }

