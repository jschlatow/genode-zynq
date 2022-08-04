/*
 * \brief  Manage two sets of objects with opposite counterparts
 * \author Johannes Schlatow
 * \date   2022-08-08
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _LINKED_OBJECTS_H_
#define _LINKED_OBJECTS_H_

/* Genode includes */
#include <base/registry.h>
#include <base/log.h>
#include <util/interface.h>
#include <util/meta.h>

class Linked_object : public Genode::Interface
{
	public:
		using Name = Genode::String<100>;

	private:

		Linked_object *_linked_obj { nullptr };

	protected:

		template <typename T, typename FUNC>
		bool _with_link(FUNC && fn)
		{
			if (_linked_obj) {
				fn(*reinterpret_cast<T*>(_linked_obj));
				return true;
			}

			return false;
		}

	public:

		~Linked_object()
		{
			if (_linked_obj)
				_linked_obj->unlink();
		}

		void link(Linked_object &obj) {
			_linked_obj = &obj;
		}

		/* called by ~Linked_object */
		void unlink() {
			_linked_obj = nullptr;
		}

		virtual Name name() const = 0;
};


template <typename LEFT, typename RIGHT>
class Linked_objects
{
	private:
		Genode::Registry<LEFT>  _left_registry  { };
		Genode::Registry<RIGHT> _right_registry { };

	public:

		void link(LEFT & left)
		{
			_right_registry.for_each([&] (RIGHT & right) {
				if (right.name() == left.name()) {
					right.link(left);
					left.link(right);
				}
			});
		}

		void link(RIGHT & right)
		{
			_left_registry.for_each([&] (LEFT & left) {
				if (right.name() == left.name()) {
					right.link(left);
					left.link(right);
				}
			});
		}

		Genode::Registry<LEFT> &registry(Genode::Meta::Overload_selector<LEFT>) {
			return _left_registry;
		}

		Genode::Registry<RIGHT> &registry(Genode::Meta::Overload_selector<RIGHT>) {
			return _right_registry;
		}
};

#endif /* _LINKED_OBJECTS_H_ */
