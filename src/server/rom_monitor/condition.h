/*
 * \brief  Helper for checking monitoring conditions
 * \author Johannes Schlatow
 * \date   2022-07-28
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CONDITION_H_
#define _CONDITION_H_

/* Genode includes */
#include <util/xml_node.h>

namespace Rom_monitor {
	class Condition;

	typedef Genode::String<100> Node_path;
	typedef Genode::String<80> Attribute_name;

	using Genode::Xml_node;
}


class Rom_monitor::Condition
{
	private:

		Xml_node                _condition_xml;
		Node_path               _path;
		Attribute_name          _attribute;

		template <typename STRING, typename FUNC>
		bool _with_first_path_element(STRING const &path, FUNC && fn) const;

		/**
		 * Iterate leaf nodes in '_path' and return true if the given function
		 * 'fn' evalutes to true for ANY node.
		 */
		template <typename FUNC>
		bool _has_node_in_path(Xml_node const &xml, FUNC && fn) const;

		bool _expected_attribute() const
		{
			if (!_condition_xml.has_attribute("attribute")) {
				Genode::warning("Missing 'attribute' attribute in node <", _condition_xml.type(), ">");

				return false;
			}

			return true;
		}

		bool _expected_value() const
		{
			if (!_condition_xml.has_attribute("value")) {
				Genode::warning("Missing 'value' attribute in node <", _condition_xml.type(), ">");

				return false;
			}

			return true;
		}

		bool _present      (Xml_node const &xml) const;
		bool _has_attribute(Xml_node const &xml) const;
		bool _has_value    (Xml_node const &xml) const;
		bool _above_value  (Xml_node const &xml) const;
		bool _below_value  (Xml_node const &xml) const;

	public:

		/**
		 * Constructor
		 */
		Condition(Xml_node const &condition_xml)
		:
			_condition_xml { condition_xml },
			_path          { _condition_xml.attribute_value("path",      Node_path { }) },
			_attribute     { _condition_xml.attribute_value("attribute", Attribute_name { }) }
		{ }

		bool evaluate(Xml_node const &xml) const;
};


template <typename STRING, typename FUNC>
bool Rom_monitor::Condition::_with_first_path_element(STRING const &path, FUNC && fn) const
{
	char const *s = path.string();

	/* skip leading '/' */
	if (*s == '/')
		s++;

	unsigned i = 0;
	while (s[i] != 0 && s[i] != '/')
		i++;

	STRING current = STRING(Genode::Cstring(s, i));

	unsigned end = i;
	while (s[end] != 0)
		end++;

	if (i == end)
		return fn(current, STRING());
	else
		return fn(current, STRING(Genode::Cstring(&s[i], end)));
}


template <typename FUNC>
bool Rom_monitor::Condition::_has_node_in_path(Xml_node const &xml, FUNC && fn) const
{
	/* define recursive lambda for path traversal */
	auto traverse = [&] (Xml_node const &node, Node_path path, auto && traverse) -> bool {
		return _with_first_path_element(path, [&] (Node_path const &first, Node_path const &remainder) {
			bool okay = false;

			node.for_each_sub_node(first.string(), [&] (Xml_node const &subnode) {
				if (okay) return;

				if (remainder.length())
					okay = traverse(subnode, remainder, traverse);
				else
					okay = fn(subnode);
			});

			return okay;
		});
	};

	return _with_first_path_element(_path, [&] (Node_path const &first, Node_path const &remainder) {
		if (!xml.has_type(first.string()))
			return false;

		if (!remainder.length())
			return fn(xml);

		return traverse(xml, remainder, traverse);
	});
}


bool Rom_monitor::Condition::_present(Xml_node const &xml) const
{
	bool find_attribute = _condition_xml.has_attribute("attribute");

	return _has_node_in_path(xml, [&] (Xml_node const &node) {
		if (find_attribute)
			return node.has_attribute(_attribute.string());

		return true;
	});
}


bool Rom_monitor::Condition::_has_attribute(Xml_node const &xml) const
{
	/* ignore condition if expected attribute is missing */
	if (!_expected_attribute())
		return true;

	/* return false if there is a node that does not have the expected attribute */
	return !_has_node_in_path(xml, [&] (Xml_node const & node) {
		if (!node.has_attribute(_attribute.string()))
			return true;

		return false;
	});
}


bool Rom_monitor::Condition::_has_value(Xml_node const &xml) const
{
	/* ignore condition if expected attribute or expected value is missing */
	if (!_expected_attribute() || !_expected_value())
		return true;

	/* return false if there is a node that does not have the expected value */
	return !_has_node_in_path(xml, [&] (Xml_node const & node) {
		if (node.has_attribute(_attribute.string())) {
			Genode::String<100> expected_value = _condition_xml.attribute_value("value", Genode::String<100>());
			if (node.attribute(_attribute.string()).has_value(expected_value.string()))
				return false;

			return true;
		}

		/* okay if the attribute is not present */
		return false;
	});
}


bool Rom_monitor::Condition::_below_value(Xml_node const &xml) const
{
	/* ignore condition if expected attribute or expected value is missing */
	if (!_expected_attribute() || !_expected_value())
		return true;

	return !_has_node_in_path(xml, [&] (Xml_node const & node) {
		if (!node.has_attribute(_attribute.string()))
			return true;

		unsigned expected_value = _condition_xml.attribute_value(_attribute.string(), 0U);
		unsigned value          = node.attribute_value("value", 0U);
		if (value < expected_value)
			return false;

		return true;
	});
}


bool Rom_monitor::Condition::_above_value(Xml_node const &xml) const
{
	/* ignore condition if expected attribute or expected value is missing */
	if (!_expected_attribute() || !_expected_value())
		return true;

	return !_has_node_in_path(xml, [&] (Xml_node const & node) {
		if (!node.has_attribute(_attribute.string()))
			return true;

		unsigned expected_value = _condition_xml.attribute_value(_attribute.string(), 0U);
		unsigned value          = node.attribute_value("value", 0U);
		if (value > expected_value)
			return false;

		return true;
	});
}


bool Rom_monitor::Condition::evaluate(Xml_node const &xml) const
{
	if (!_condition_xml.has_attribute("path")) {
		Genode::warning("Missing 'path' attribute in node <", xml.type(), ">");

		/* ignore condition */
		return true;
	}

	if      (_condition_xml.has_type("present"))       return _present      (xml);
	else if (_condition_xml.has_type("has_attribute")) return _has_attribute(xml);
	else if (_condition_xml.has_type("has_value"))     return _has_value    (xml);
	else if (_condition_xml.has_type("above_value"))   return _above_value  (xml);
	else if (_condition_xml.has_type("below_value"))   return _below_value  (xml);

	Genode::warning("Unknown condition '<", xml.type(), ">'");
	return true;
}

#endif /* _CONDITION_H_ */
