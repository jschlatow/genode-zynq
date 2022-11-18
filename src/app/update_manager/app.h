/*
 * \brief  App representation
 * \author Johannes Schlatow
 * \date   2022-08-01
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _APP_H_
#define _APP_H_

/* Genode includes */
#include <base/allocator.h>
#include <base/attached_rom_dataspace.h>
#include <os/buffered_xml.h>
#include <util/xml_generator.h>
#include <util/reconstructible.h>
#include <util/list_model.h>
#include <util/interface.h>
#include <timer_session/connection.h>

/* local includes */
#include "download_queue.h"
#include "state_report.h"

namespace Update_manager {
	using Genode::Allocator;
	using Genode::Reconstructible;
	using Genode::Constructible;
	using Genode::Xml_node;
	using Genode::Xml_generator;
	using Genode::Buffered_xml;
	using Genode::Attached_rom_dataspace;

	struct Deploy;
	struct Update_state_reporter;
	class Variant;
	class Variants;
	class App;
}


/**
 * Interface for letting Main know we need to generate a new deploy config.
 */
struct Update_manager::Deploy : Genode::Interface
{
	virtual void trigger() = 0;
};


/**
 * Interface for letting Main know we need to regenerate the update state report
 */
struct Update_manager::Update_state_reporter : Genode::Interface
{
	virtual void update() = 0;
};


class Update_manager::Variant : public Genode::List_model<Variant>::Element
{
	private:
		friend class Variants;

		Xml_node _xml;
		Path     _pkg;

		unsigned _max_retries;
		unsigned _delay_ms;

		unsigned _version     { 0 };

		enum State {
			NEEDS_INSTALL,
			INSTALLED,
			FAILED
		} _state { NEEDS_INSTALL };

		bool _needs_install() const { return _state == State::NEEDS_INSTALL; }
		bool _deployable()    const { return _state == State::INSTALLED; }

		void _installed(bool success) { _state = success ? State::INSTALLED : State::FAILED; }

		/**
		 * Return true if error can be handled by this Variant.
		 */
		bool _handle_error()
		{
			if (_version < _max_retries)
				_version++;
			else
				_state = State::FAILED;

			return _state != State::FAILED;
		}

	public:
		Variant(Xml_node const &variant_node)
		: _xml        (variant_node),
		  _pkg        (_xml.attribute_value("pkg",      Path())),
		  _max_retries(_xml.attribute_value("retry",    0U)),
		  _delay_ms   (_xml.attribute_value("delay_ms", 0U)),
		  _version    (_xml.attribute_value("version",  0U))
		{ }

		Path const &pkg()   const { return _pkg; }

		unsigned version()  const { return _version; }

		unsigned delay_ms() const { return _delay_ms; }

		void append_xml_content(Xml_generator &xml_gen) const
		{
			_xml.for_each_sub_node([&] (Xml_node const &n) {
				xml_gen.append("\t");
				n.with_raw_node([&] (char const *start, Genode::size_t length) {
					xml_gen.append(start, length); });
				xml_gen.append("\n");
			});
		}

};


class Update_manager::Variants : public Genode::List_model<Variant>
{
	private:
		Variant   *_deployed_variant { nullptr };

		struct Update_policy : Genode::List_model<Variant>::Update_policy
		{
			Allocator &_alloc;

			Update_policy(Allocator &alloc) : _alloc(alloc) { }

			void destroy_element(Variant & v) {
				destroy(_alloc, &v); }

			Variant & create_element(Xml_node const &xml) {
				return *new (_alloc) Variant(xml); }

			void update_element(Variant &, Xml_node const &) { };

			static bool node_is_element(Xml_node const &node) {
				return node.has_type("variant"); }

			static bool element_matches_xml_node(Variant const &, Xml_node const &) {
				return false; }

		} _policy;

		/* Noncopyable */
		Variants(Variants const &) = delete;
		Variants &operator=(Variants const &) = delete;
		

		void _find_first_deployable()
		{
			_deployed_variant = nullptr;
			for_each([&] (Variant &v) {
				if (!_deployed_variant && v._deployable())
					_deployed_variant = &v;
			});
		}

	public:
		Variants(Allocator &alloc) : _policy(alloc) { }

		/**
		 * Recreate list model from xml
		 */
		void create_from_xml(Xml_node const &app_node)
		{
			clear();
			update_from_xml(_policy, app_node);
		}

		void clear()
		{
			_deployed_variant = nullptr;
			destroy_all_elements(_policy);
		}

		/**
		 * Handle execution error.
		 *
		 * If the same variant is retried, 'retry_fn' is called.
		 * If it is switched to the next variant, 'next_fn' is called.
		 * Otherwise, if there is no further variant to be edeployed, 'stop_fn'
		 * is called.
		 */
		template <typename RETRY_FN, typename NEXT_FN, typename STOP_FN>
		void handle_error(RETRY_FN && retry_fn, NEXT_FN && next_fn, STOP_FN && stop_fn)
		{
			if (!_deployed_variant)
				return;

			if (_deployed_variant->_handle_error())
				retry_fn(*_deployed_variant);
			else {
				_find_first_deployable();

				if (_deployed_variant)
					next_fn(*_deployed_variant);
				else
					stop_fn();
			}
		}

		/**
		 * Apply the installation state of each variant (provided by 'fn').
		 */
		template <typename FUNC>
		void apply_installation(FUNC && fn)
		{
			for_each([&] (Variant &v) {
				v._installed(fn(v));
			});

			_find_first_deployable();
		}

		bool install_finished()
		{
			bool finished = true;
			for_each([&] (Variant &v) {
				if (v._needs_install())
					finished = false;
			});

			return finished;
		}

		template <typename FUNC>
		void with_current_variant(FUNC && fn)
		{
			if (_deployed_variant && _deployed_variant->_deployable())
				fn(*_deployed_variant);
		}

		template <typename FUNC>
		void with_current_variant(FUNC && fn) const
		{
			if (_deployed_variant && _deployed_variant->_deployable())
				fn(*_deployed_variant);
		}
};


class Update_manager::App : public Genode::List_model<App>::Element
{
	public:
		using Name = Report::Consumer::Name;

		enum State {
			INSTALLING,
			STARTING,
			RUNNING,
			FAILED
		};

	private:

		Genode::Env                           &_env;
		Allocator                             &_alloc;
		Deploy                                &_deploy;
		Name                                   _name;
		Path                                   _last_running_pkg { };
		Download_queue                        &_download_queue;
		Timer::One_shot_timeout<App>           _timeout;
		State                                  _state { INSTALLING };
		Update_state_reporter                 &_update_state_reporter;

		Constructible<Buffered_xml>            _app_xml { };

		Variants                               _variants { _alloc };

		struct Consumer : Report::Consumer
		{
			App &_app;

			Consumer(Report::Pool &state_report_pool, App &app)
			: Report::Consumer(state_report_pool.registry(Genode::Meta::Overload_selector<Report::Consumer>())),
			  _app(app)
			{
				state_report_pool.link(*this);
			}

			/**
			 * Linked_object interface
			 */
			Name name() const override { return _app.name(); }

			/**
			 * Consumer interface
			 */
			void handle_state(Xml_node const &xml) override {
				_app.handle_state(xml); }

		} _consumer ;

		void _handle_error();

		void _handle_timeout(Genode::Duration)
		{
			Genode::error(_name, ": startup failed");
			_handle_error();
		}

	public:

		/**
		 * Constructor
		 */
		App(Genode::Env           &env,
		    Allocator             &alloc,
		    Timer::Connection     &timer,
		    Deploy                &deploy,
		    Xml_node const        &app_node,
		    Download_queue        &download_queue,
		    Report::Pool          &state_report_pool,
		    Update_state_reporter &update_state_reporter)
		:
			_env                   { env },
			_alloc                 { alloc },
			_deploy                { deploy },
			_name                  { app_node.attribute_value("name", Name()) },
			_download_queue        { download_queue },
			_timeout               { timer, *this, &App::_handle_timeout },
			_update_state_reporter { update_state_reporter },
			_consumer              { state_report_pool, *this }
		{
			apply_config(app_node);
		}

		void apply_config(Xml_node const &app_node);

		void apply_installation();

		void gen_start_entries(Xml_generator &xml) const;

		void gen_state_entry(Xml_generator &xml) const;

		/**
		 * Linked_object interface
		 */
		Name name() const { return _name; }

		/**
		 * Consumer interface
		 */
		void handle_state(Xml_node const &xml)
		{
			_timeout.discard();

			State old_state = _state;

			if (xml.has_sub_node("failed"))
				_handle_error();
			else if (old_state == STARTING) {
				_state = RUNNING;
				_variants.with_current_variant([&] (Variant const &v) {
					_last_running_pkg = v.pkg();
				});
			}

			if (old_state != _state)
				_update_state_reporter.update();
		}
};


void Update_manager::App::apply_config(Xml_node const &app_node)
{
	if (_app_xml.constructed() && !app_node.differs_from(_app_xml->xml()))
		return;

	/* cleanup */
	_timeout.discard();

	/* clear variants before reconstructing _app_xml */
	_variants.clear();

	_app_xml.construct(_alloc, app_node);

	/* rebuild list model */
	_variants.create_from_xml(_app_xml->xml());

	/* add referenced pkgs to download queue */
	_variants.for_each([&] (Variant const &v) {
		_download_queue.add(v.pkg());
	});

	if (!_variants.install_finished())
		_state = State::INSTALLING;
}


void Update_manager::App::_handle_error()
{
	_state = State::FAILED;

	_variants.with_current_variant([&] (Variant &v) {
		Genode::warning(_name, ": execution failure in ", v.pkg());
	});

	unsigned delay_ms = 0;
	_variants.handle_error(
		/* restart the same variant */
		[&] (Variant &v) {
			delay_ms = v.delay_ms();
		},
		/* variant changed */
		[&] (Variant &v) {
			Genode::warning(_name, ": switching to ", v.pkg());
			delay_ms = v.delay_ms();
		},
		/* no next variant error */
		[&] () {
			Genode::error(_name, ": stopped");
		}
	);

	if (delay_ms)
		_timeout.schedule(Genode::Microseconds { delay_ms * 1000U });

	/* trigger update of deploy config */
	_deploy.trigger();
}


void Update_manager::App::apply_installation()
{
	_variants.apply_installation([&] (Variant &v) {
		Path const &path = v.pkg();
		bool success = _download_queue.state(path) == Download_queue::Download::State::DONE;
		if (!success)
			Genode::error(_name, ": Download of variant ", path, " failed.");

		return success;
	});

	if (_variants.install_finished())
		_state = STARTING;

	_variants.with_current_variant([&] (Variant &v) {
		if (_last_running_pkg == v.pkg())
			_state = RUNNING;
		else if (v.delay_ms())
			_timeout.schedule(Genode::Microseconds { v.delay_ms() * 1000U });
	});
}


void Update_manager::App::gen_start_entries(Xml_generator &xml) const
{
	_variants.with_current_variant([&] (Variant const &v) {
		/* create start node */
		xml.node("start", [&] () {
			xml.attribute("name",    _name);
			xml.attribute("pkg",     v.pkg());
			xml.attribute("version", v.version());

			/* copy content from <variant> into start node */
			v.append_xml_content(xml);
		});

		Genode::log(_name, ": deploying ", v.pkg());
	});
}


void Update_manager::App::gen_state_entry(Xml_generator &xml) const
{
	xml.node("app", [&] () {
		xml.attribute("name", _name);
		_variants.with_current_variant([&] (Variant const &v) {
			xml.attribute("variant", v.pkg());
			xml.attribute("version", v.version());
		});

		switch (_state) {
		case State::INSTALLING:
			xml.attribute("state", "INSTALLING");
			break;
		case State::STARTING:
			xml.attribute("state", "STARTING");
			break;
		case State::RUNNING:
			xml.attribute("state", "RUNNING");
			break;
		case State::FAILED:
			xml.attribute("state", "FAILED");
			break;
		}
	});
}

#endif /* _APP_H_ */
