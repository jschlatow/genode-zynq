/*
 * \brief  Helper classes for packet fragmentation/defragmention
 * \author Johannes Schlatow
 * \date   2021-11-08
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__DRIVERS__NIC__RF__AD9361__FRAGMENT_H_
#define _INCLUDE__DRIVERS__NIC__RF__AD9361__FRAGMENT_H_

/* Genode includes */
#include <net/size_guard.h>

namespace Rf {
	using namespace Genode;
	using Size_guard = Net::Size_guard;

	struct Fragment;

	template <size_t MAX_SIZE>
	class Fragment_writer;

	template <size_t MAX_SIZE>
	class Packet_writer;
}

struct Rf::Fragment
{
	uint16_t size;
	uint16_t remaining_fragments;
	unsigned _data[0];

	struct Out_of_bounds : Genode::Exception { };

	Fragment(size_t bytes, unsigned fragments)
	: size(static_cast<uint16_t>(bytes)),
	  remaining_fragments(static_cast<uint16_t>(fragments))
	{
		if (bytes > (uint16_t)~0U)
			throw Out_of_bounds();

		if (fragments > (uint16_t)~0U)
			throw Out_of_bounds();
	}

	static Fragment &cast_from(void       *base,
		                        Size_guard &size_guard)
	{
		size_guard.consume_head(sizeof(Fragment));
		return *(Fragment *)base;
	}

	const void *payload(Size_guard & size_guard, size_t size) const
	{
		size_guard.consume_head(size);
		return static_cast<const void *>(_data);
	}

	void *payload(Size_guard & size_guard, size_t size)
	{
		size_guard.consume_head(size);
		return static_cast<void*>(_data);
	}

	size_t payload_size() const { return size - sizeof(Fragment); }

} __attribute__((packed));


template <Genode::size_t MAX_SIZE>
class Rf::Fragment_writer
{
	private:
		static constexpr size_t MAX_PAYLOAD { MAX_SIZE - sizeof(Rf::Fragment) };

		typedef Rf::Fragment_writer<MAX_SIZE> Self;

		const uint8_t *_payload;
		Size_guard    &_size_guard;
		size_t         _curr_fragment_size  { Genode::min(_size_guard.total_size(), MAX_PAYLOAD) };
		unsigned       _fragments_remaining { Self::_fragment_count(_size_guard.total_size()) };

		static unsigned _fragment_count(size_t total_size)
		{
			if (total_size % MAX_PAYLOAD)
				return total_size / MAX_PAYLOAD;
			else
				return total_size / MAX_PAYLOAD - 1;
		}

	public:
		Fragment_writer(const void * ptr, Size_guard & size_guard)
		: _payload(static_cast<const uint8_t*>(ptr)),
		  _size_guard(size_guard)
		{ }

		Fragment & construct_header(void * dst)
		{
			return *Genode::construct_at<Fragment>(dst,
			                                       _curr_fragment_size + sizeof(Fragment),
			                                       _fragments_remaining);
		}

		size_t copy_payload(Fragment & fragment)
		{
			Size_guard size_guard(fragment.size);
			size_guard.consume_head(sizeof(Fragment));

			memcpy(fragment.payload(size_guard, _curr_fragment_size),
			       _payload,
			       _curr_fragment_size);
			return _curr_fragment_size;
		}

		bool more()
		{
			if (!_curr_fragment_size)
				return false;

			_size_guard.consume_head(_curr_fragment_size);

			_payload += _curr_fragment_size;
			_curr_fragment_size = Genode::min(_size_guard.unconsumed(), MAX_PAYLOAD);
			_fragments_remaining--;

			return (bool)_curr_fragment_size;
		}
};


template <Genode::size_t PACKET_SIZE>
class Rf::Packet_writer
{
	public:
		enum Consume_result {
			PACKET_INVALID,
			FRAGMENT_INVALID,
			FRAGMENT_LOST,
			NEW_PACKET,
			BUFFER_EXCEEDED,
			SUCCEEDED
		};

	private:
		size_t    _bytes_written { 0 };
		unsigned  _last_fragment;
		uint8_t   _buffer[PACKET_SIZE];

	public:
		Packet_writer()
		: _last_fragment(0)
		{ }

		Packet_writer(Fragment const & fragment)
		: _last_fragment(fragment.remaining_fragments+1)
		{ }

		Consume_result try_consume(Fragment const & fragment)
		{
			if (!fragment.size)
				return FRAGMENT_INVALID;

			if (!valid())
				return PACKET_INVALID;

			if (fragment.remaining_fragments > _last_fragment-1)
				return NEW_PACKET;

			if (fragment.remaining_fragments < _last_fragment-1)
				return FRAGMENT_LOST;

			if (_bytes_written + fragment.size >= PACKET_SIZE)
				return BUFFER_EXCEEDED;

			return SUCCEEDED;
		}

		bool consume_and_try_finish(Fragment const & fragment, Size_guard fragment_guard)
		{
			const size_t size = fragment.payload_size();

			memcpy(&_buffer[_bytes_written], fragment.payload(fragment_guard, size), size);
			_bytes_written += size;

			if (--_last_fragment == 0)
				return true;

			return false;
		}

		bool valid()    { return (bool)_last_fragment; }

		void discard() { _last_fragment = 0; }

		size_t size() const { return _bytes_written; }
		void * data()       { return _buffer; }
};


#endif /* _INCLUDE__DRIVERS__NIC__RF__AD9361__FRAGMENT_H_ */
