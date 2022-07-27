/*
 * \brief  ROM server that monitors ROMs for certain conditions
 * \author Johannes Schlatow
 * \date   2022-07-15
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <util/reconstructible.h>
#include <util/xml_generator.h>
#include <util/retry.h>
#include <base/heap.h>
#include <base/component.h>
#include <base/attached_ram_dataspace.h>
#include <root/component.h>

/* local includes */
#include "monitor.h"

namespace Rom_monitor {
	using Genode::Entrypoint;
	using Genode::Rpc_object;
	using Genode::Sliced_heap;
	using Genode::Constructible;
	using Genode::Xml_generator;
	using Genode::size_t;
	using Genode::Interface;
	using Genode::Registered;

	class  Output_buffer;
	class  Session_component;
	class  Root;
	struct Main;

	using Monitor_registry = Genode::Registry<Registered<Monitor>>;

	typedef Genode::List<Session_component> Session_list;
}


/**
 * Interface used by the sessions to obtain the XML output data
 */
struct Rom_monitor::Output_buffer : Interface
{
	virtual size_t content_size() const = 0;
	virtual size_t export_content(char *dst, size_t dst_len) const = 0;
};


class Rom_monitor::Session_component : public Rpc_object<Genode::Rom_session>,
                                      private Session_list::Element
{
	private:

		friend class Genode::List<Session_component>;

		Genode::Env &_env;

		Signal_context_capability _sigh { };

		Output_buffer const &_output_buffer;

		Session_list &_sessions;

		Constructible<Genode::Attached_ram_dataspace> _ram_ds { };

	public:

		Session_component(Genode::Env &env, Session_list &sessions,
		                  Output_buffer const &output_buffer)
		:
			_env(env), _output_buffer(output_buffer), _sessions(sessions)
		{
			_sessions.insert(this);
		}

		~Session_component() { _sessions.remove(this); }

		using Session_list::Element::next;

		void notify_client()
		{
			if (!_sigh.valid())
				return;

			Genode::Signal_transmitter(_sigh).submit();
		}

		Genode::Rom_dataspace_capability dataspace() override
		{
			using namespace Genode;

			/* replace dataspace by new one as needed */
			if (!_ram_ds.constructed()
			 || _output_buffer.content_size() > _ram_ds->size()) {

				_ram_ds.construct(_env.ram(), _env.rm(), _output_buffer.content_size());
			}

			char             *dst = _ram_ds->local_addr<char>();
			size_t const dst_size = _ram_ds->size();

			/* fill with content of current evaluation result */
			size_t const copied_len = _output_buffer.export_content(dst, dst_size);

			/* clear remainder of dataspace */
			Genode::memset(dst + copied_len, 0, dst_size - copied_len);

			/* cast RAM into ROM dataspace capability */
			Dataspace_capability ds_cap = static_cap_cast<Dataspace>(_ram_ds->cap());
			return static_cap_cast<Rom_dataspace>(ds_cap);
		}

		void sigh(Genode::Signal_context_capability sigh) override
		{
			_sigh = sigh;
		}
};


class Rom_monitor::Root : public Genode::Root_component<Session_component>
{
	private:

		Genode::Env   &_env;
		Output_buffer &_output_buffer;
		Session_list   _sessions { };

	protected:

		Session_component *_create_session(const char *) override
		{
			/*
			 * We ignore the name of the ROM module requested
			 */
			return new (md_alloc()) Session_component(_env, _sessions, _output_buffer);
		}

	public:

		Root(Genode::Env       &env,
		     Output_buffer     &output_buffer,
		     Genode::Allocator &md_alloc)
		:
			Genode::Root_component<Session_component>(&env.ep().rpc_ep(), &md_alloc),
			_env(env), _output_buffer(output_buffer)
		{ }

		void notify_clients()
		{
			for (Session_component *s = _sessions.first(); s; s = s->next())
				s->notify_client();
		}
};


struct Rom_monitor::Main : Monitor::State_changed_fn,
                           Output_buffer
{
	Genode::Env      &_env;

	Timer::Connection _timer       { _env };
	Sliced_heap       _sliced_heap { _env.ram(), _env.rm() };
	Genode::Heap      _heap        { _env.ram(), _env.rm() };

	Monitor_registry  _monitor_registry { };

	Genode::Constructible<Genode::Attached_ram_dataspace> _xml_ds { };
	size_t _xml_output_len = 0;

	void _generate_output();

	Root _root = { _env, *this, _sliced_heap };

	Genode::Attached_rom_dataspace _config { _env, "config" };

	Genode::Signal_handler<Main> _config_handler =
		{ _env.ep(), *this, &Main::_handle_config };

	void _handle_config()
	{
		_config.update();

		/* destroy all monitors */
		_monitor_registry.for_each([&] (Registered<Monitor> &monitor) {
			destroy(_heap, &monitor);
		});

		/*
		 * Create buffer for generated XML data
		 */
		Genode::Number_of_bytes xml_ds_size = 4096;

		xml_ds_size = _config.xml().attribute_value("buffer", xml_ds_size);

		if (!_xml_ds.constructed() || xml_ds_size != _xml_ds->size())
			_xml_ds.construct(_env.ram(), _env.rm(), xml_ds_size);

		/*
		 * Create monitors
		 */
		_config.xml().for_each_sub_node("monitor", [&] (Xml_node &node) {
			new (_heap) Registered<Monitor>(_monitor_registry, _env, node, _timer, *this);
		});

		_generate_output();
	}

	/**
	 * Monitor::State_changed_fn interface
	 *
	 * Called each time one of the monitor state changes.
	 */
	void state_changed() override
	{
		_generate_output();
	}

	/**
	 * Output_buffer interface
	 */
	size_t content_size() const override { return _xml_output_len; }

	/**
	 * Output_buffer interface
	 */
	size_t export_content(char *dst, size_t dst_len) const override
	{
		size_t const len = Genode::min(dst_len, _xml_output_len);
		Genode::memcpy(dst, _xml_ds->local_addr<char>(), len);
		return len;
	}

	Main(Genode::Env &env) : _env(env)
	{
		_env.parent().announce(_env.ep().manage(_root));
		_config.sigh(_config_handler);
		_handle_config();
	}
};


void Rom_monitor::Main::_generate_output()
{
	auto generate_xml = [&] (Xml_generator &xml) {
		_monitor_registry.for_each([&] (Registered<Monitor> const &mon) {
			Monitor::State state = mon.state();
			switch (state.status()) {

			case Monitor::State::Status::OKAY:
				xml.node("okay", [&] () { xml.attribute("name", mon.name()); });
				break;

			case Monitor::State::Status::FAILED:
				xml.node("failed", [&] () {
					xml.attribute("name", mon.name());
					xml.attribute("count", state.count());
				});
				break;
			}
		});
	};

	/*
	 * Generate output, expand dataspace on demand
	 */
	enum { UPGRADE = 4096, NUM_ATTEMPTS = ~0L };
	Genode::retry<Xml_generator::Buffer_exceeded>(
		[&] () {
			Xml_generator xml(_xml_ds->local_addr<char>(),
			                  _xml_ds->size(), "state",
			                  [&] () { generate_xml(xml); });
			_xml_output_len = xml.used();
		},
		[&] () {
			_xml_ds.construct(_env.ram(), _env.rm(), _xml_ds->size() + UPGRADE);
		},
		NUM_ATTEMPTS);

	_root.notify_clients();
}


void Component::construct(Genode::Env &env) { static Rom_monitor::Main main(env); }

