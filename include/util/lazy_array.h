/*
 * \brief  Utility for holding an array of constructible objects
 * \author Johannes Schlatow
 * \date   2022-01-14
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__UTIL__LAZY_ARRAY_H_
#define _INCLUDE__UTIL__LAZY_ARRAY_H_

#include <util/reconstructible.h>
#include <util/array.h>

namespace Genode {
	template<typename, unsigned> class Lazy_array;
}


/**
 * Holds an array of MAX entries of T. The backend storage is initialised by
 * using Constructible<T>, so that T can be noncopyable and does not need a
 * default constructor.
 *
 * Adding elements to the array is implemented by calling construct().
 */
template <typename T, unsigned MAX>
class Genode::Lazy_array : protected Genode::Array<Genode::Constructible<T>,MAX>
{
	private:
		typedef Array<Constructible<T>,MAX> Base;

	public:
		struct Index_out_of_bounds : Exception { };

		using Base::count;

		/**
		 * Constructs an empty array
		 */
		Lazy_array() : Base() { }

		/**
		 * Constructs a partially filled array
		 *
		 * \param num   number of elements to add
		 * \param args  arguments used for element construction
		 */
		template <typename ... ARGS>
		Lazy_array(unsigned num, ARGS &&... args)
		: Base::Array()
		{
			if (num > MAX) num = MAX;

			for (; num; num--)
				add(args...);
		}

		/**
		 * Return the array element specified by index
		 *
		 * \param idx  the index of the array element
		 *
		 * \throw Index_out_of_bounds
		 */
		T & value(unsigned idx)
		{
			return *Base::value(idx);
		}

		/**
		 * Adds a single element to the array.
		 *
		 * The element gets inserted at position 'count()',
		 * and 'count()' is incremented.
		 *
		 * \param args  arguments used for construction
		 *
		 * \throw Index_out_of_bounds
		 */
		template <typename ... ARGS>
		void add(ARGS &&... args)
		{
			if ((Base::_count + 1) > MAX)
				throw Index_out_of_bounds();

			Base::_objs[Base::_count++].construct(args...);
		}

		template <typename FUNC>
		void for_each(FUNC const &f)
		{
			for (unsigned idx = 0; idx < Base::_count; idx++)
				f(idx, *Base::_objs[idx]);
		}

		template <typename FUNC>
		void for_each(FUNC const &f) const
		{
			for (unsigned idx = 0; idx < Base::_count; idx++) {
				T const & obj = *Base::_objs[idx];
				f(idx, obj);
			}
		}
};

#endif /* _INCLUDE__UTIL__RECONSTRUCTIBLE_H_ */
