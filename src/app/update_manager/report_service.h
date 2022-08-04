/*
 * \brief  Report service that accepts the App's state reports
 * \author Johannes Schlatow
 * \date   2022-08-08
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _REPORT_SERVICE_H_
#define _REPORT_SERVICE_H_

/* Genode includes */
#include <base/attached_ram_dataspace.h>
#include <report_session/report_session.h>
#include <root/component.h>
#include <base/log.h>
#include <util/xml_node.h>

/* local includes */
#include "state_report.h"

namespace Report {
	using Genode::Xml_node;

	struct Session_component;
	struct Root;
}


struct Report::Session_component : Genode::Rpc_object<Session>,
                                   Producer
{
	private:

		Genode::Session_label const _label;

		Genode::Attached_ram_dataspace _ds;

		template <typename FUNC>
		void _with_xml(size_t length, FUNC && fn)
		{
			try {
				if (length && _ds.local_addr<void const>()) {
					fn(Xml_node(_ds.local_addr<char>(), length));
					return;
				}
			} catch (Xml_node::Invalid_syntax) { }

			fn(Xml_node("<empty/>"));
		}

	public:

		Session_component(Genode::Env &env,
		                  Genode::Session_label const &label, size_t buffer_size,
		                  Pool &state_report_pool)
		:
			Producer(state_report_pool.registry(Genode::Meta::Overload_selector<Producer>())),
			_label(label), _ds(env.ram(), env.rm(), buffer_size)
		{
			state_report_pool.link(*this);
		}

		/**
		 * Producer interface
		 */
		Name name() const override { return _label.prefix().last_element(); }

		/**
		 * Report_session interface
		 */

		Dataspace_capability dataspace() override { return _ds.cap(); }

		void submit(size_t length) override
		{
			length = Genode::min(length, _ds.size());

			with_consumer([&] (Consumer & consumer) {
				_with_xml(length, [&] (Xml_node xml) {
					consumer.handle_state(xml);
				});
			});
		}

		void response_sigh(Genode::Signal_context_capability) override { }

		size_t obtain_response() override { return 0; }
};


struct Report::Root : Genode::Root_component<Session_component>
{
	private:
		Genode::Env       &_env;
		Pool              &_pool;

	protected:

		Session_component *_create_session(const char *args) override
		{
			using namespace Genode;

			Session_label const label = label_from_args(args);

			if (label.last_element() != "state")
				throw Service_denied();

			size_t const ram_quota =
				Arg_string::find_arg(args, "ram_quota").aligned_size();

			/* read report buffer size from session arguments */
			size_t const buffer_size =
				Arg_string::find_arg(args, "buffer_size").aligned_size();

			if (ram_quota < buffer_size) {
				Genode::error("insufficient ram donation from ", label.string());
				throw Insufficient_ram_quota();
			}

			if (buffer_size == 0) {
				Genode::error("zero-length report requested by ", label.string());
				throw Service_denied();
			}

			return new (md_alloc()) Session_component(_env, label, buffer_size, _pool);
		}

	public:

		Root(Genode::Env              &env,
		     Genode::Allocator        &md_alloc,
		     Pool                     &pool)
		:
			Genode::Root_component<Session_component>(&env.ep().rpc_ep(), &md_alloc),
			_env(env),
			_pool(pool)
		{ }
};

#endif /* _REPORT_SERVICE_H_ */
