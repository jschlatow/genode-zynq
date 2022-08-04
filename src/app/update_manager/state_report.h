/*
 * \brief  Types shared between report service and app
 * \author Johannes Schlatow
 * \date   2022-08-08
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _STATE_REPORT_H_
#define _STATE_REPORT_H_

/* Genode includes */
#include <base/registry.h>
#include <base/log.h>
#include <util/interface.h>
#include <util/xml_node.h>

/* local includes */
#include "linked_objects.h"

namespace Report {
	using Genode::Interface;
	using Genode::Registry;

	class Producer;
	class Consumer;

	using Pool = Linked_objects<Producer, Consumer>;
}


class Report::Producer : public Linked_object
{
	private:

		Registry<Producer>::Element _element;

	public:

		Producer(Registry<Producer> &registry)
		: _element(registry, *this)
		{ }

		template <typename FUNC>
		bool with_consumer(FUNC && fn) {
			return _with_link<Consumer>(fn); }
};


class Report::Consumer : public Linked_object
{
	private:

		Registry<Consumer>::Element _element;

	public:

		Consumer(Registry<Consumer> &registry)
		: _element(registry, *this)
		{ }

		/**
		 * Interface for submitting reports
		 */
		virtual void handle_state(Genode::Xml_node const &) = 0;
};

#endif /* _STATE_REPORT_H_ */
