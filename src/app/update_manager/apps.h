/*
 * \brief  Manage application deployment, update, monitoring, and fallback.
 * \author Johannes Schlatow
 * \date   2022-08-01
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _APPS_H_
#define _APPS_H_

/* Genode includes */
#include <util/list_model.h>
#include <util/xml_generator.h>

/* local includes */
#include "app.h"
#include "download_queue.h"
#include "state_report.h"

namespace Update_manager {
	using Genode::Allocator;
	using Genode::List_model;

	class Apps;
}


class Update_manager::Apps
{
	private:

		Genode::Env       &_env;
		Allocator         &_alloc;
		Timer::Connection &_timer;
		Deploy            &_deploy;
		Download_queue    &_download_queue;
		Report::Pool      &_state_report_pool;

		List_model<App>    _apps { };

		struct Model_update_policy : List_model<App>::Update_policy
		{
			Genode::Env       &_env;
			Allocator         &_alloc;
			Timer::Connection &_timer;
			Deploy            &_deploy;
			Download_queue    &_download_queue;
			Report::Pool      &_state_report_pool;

			Model_update_policy(Genode::Env       &env,
			                    Allocator         &alloc,
			                    Timer::Connection &timer,
			                    Deploy            &deploy,
			                    Download_queue    &download_queue,
			                    Report::Pool      &state_report_pool)
			: _env(env), _alloc(alloc), _timer(timer), _deploy(deploy),
			 _download_queue(download_queue), _state_report_pool(state_report_pool)
			{ }

			void destroy_element(App &app) {
				destroy(_alloc, &app); }

			App &create_element(Xml_node node) {
				return *new (_alloc) App(_env, _alloc, _timer, _deploy,
				                         node, _download_queue, _state_report_pool);
			}

			void update_element(App &app, Xml_node node) {
				app.apply_config(node); }

			static bool element_matches_xml_node(App const &app, Xml_node node) {
				return node.attribute_value("name", App::Name()) == app.name(); }

			static bool node_is_element(Xml_node node) { return node.has_type("app"); }

		} _model_update_policy { _env, _alloc, _timer, _deploy, _download_queue, _state_report_pool };

	public:

		Apps(Genode::Env       &env,
		     Allocator         &alloc,
		     Timer::Connection &timer,
		     Deploy            &deploy,
		     Download_queue    &download_queue,
		     Report::Pool      &state_report_pool)
		: _env(env), _alloc(alloc), _timer(timer), _deploy(deploy),
		  _download_queue(download_queue), _state_report_pool(state_report_pool)
		{ }

		void apply_config(Xml_node config) {
			_apps.update_from_xml(_model_update_policy, config); }

		void apply_installation()
		{
			_apps.for_each([&] (App &app) {
				app.apply_installation();
			});
		}

		void gen_start_entries(Xml_generator &xml)
		{
			_apps.for_each([&] (App &app) {
				app.gen_start_entries(xml);
			});
		}
};

#endif /* _APPS_H_ */
