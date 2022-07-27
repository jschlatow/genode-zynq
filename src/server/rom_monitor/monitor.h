/*
 * \brief  Utility for monitoring ROM modules
 * \author Johannes Schlatow
 * \date   2022-07-15
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _MONITOR_H_
#define _MONITOR_H_

/* Genode includes */
#include <util/xml_node.h>
#include <base/attached_rom_dataspace.h>
#include <base/allocator.h>
#include <timer_session/connection.h>

/* local includes */
#include "condition.h"

namespace Rom_monitor {
	class Monitor;

	typedef Genode::String<100> Rom_name;

	using Genode::Signal_context_capability;
	using Genode::Signal_handler;
	using Genode::Xml_node;
	using Genode::Interface;
	using Genode::Attached_rom_dataspace;
	using Genode::Microseconds;
}


class Rom_monitor::Monitor
{
	public:

		/**
		 * Callback type
		 */
		struct State_changed_fn : Interface
		{
			virtual void state_changed() = 0;
		};

		/**
		 * State type
		 */
		struct State
		{
			enum Status { OKAY, FAILED };

			Status    _status { OKAY };
			unsigned  _count  { 0 };

			void failed()
			{
				_status = FAILED;
				_count++;
			}

			void okay()
			{
				_status = OKAY;
				_count  = 0;
			}

			unsigned count()  const { return _count; }
			Status   status() const { return _status; }
		};

	private:

		using One_shot_timeout = Timer::One_shot_timeout<Monitor>;

		Genode::Env            &_env;
		State_changed_fn       &_state_changed_fn;
		Xml_node                _monitor_xml;

		Rom_name                _name;
		One_shot_timeout        _timeout;

		unsigned                _timeout_ms { 0 };
		State                   _state      {   };
		Attached_rom_dataspace  _rom_ds     { _env, _name.string() };

		void _handle_rom_changed()
		{
			_rom_ds.update();

			_evaluate(_rom_ds.xml());

			_state_changed_fn.state_changed();
		}

		Signal_handler<Monitor> _rom_changed_handler =
			{ _env.ep(), *this, &Monitor::_handle_rom_changed };

		void _handle_timeout(Genode::Duration)
		{
			_state.failed();
			_state_changed_fn.state_changed();
		}

		void _evaluate(Xml_node const &xml);

	public:

		/**
		 * Constructor
		 */
		Monitor(Genode::Env       &env,
		        Xml_node    const &monitor_xml,
		        Timer::Connection &timer,
		        State_changed_fn  &state_changed_fn)
		:
			_env              { env },
			_state_changed_fn { state_changed_fn },
			_monitor_xml      { monitor_xml },
			_name             { _monitor_xml.attribute_value("rom", Rom_name()) },
			_timeout          { timer, *this, &Monitor::_handle_timeout },
			_timeout_ms       { _monitor_xml.attribute_value("timeout_ms", 0U) }
		{
			_rom_ds.sigh(_rom_changed_handler);

			_evaluate(_rom_ds.xml());
		}

		virtual ~Monitor() { }

		Rom_name name()  const { return _name; }

		State    state() const { return _state; }
};


void Rom_monitor::Monitor::_evaluate(Xml_node const &xml)
{
	/* reschedule timeout */
	if (_timeout_ms)
		_timeout.schedule(Microseconds { _timeout_ms * 1000U });

	bool okay = true;
	_monitor_xml.for_each_sub_node([&] (Xml_node & node) {
		if (!okay)
			return;

		okay = Condition(node).evaluate(xml);
	});

	if (okay)
		_state.okay();
	else
		_state.failed();
}

#endif /* _MONITOR_H_ */
