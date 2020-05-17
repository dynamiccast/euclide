#ifndef __EUCLIDE_ITERATOR_H
#define __EUCLIDE_ITERATOR_H

#include "../includes.h"

namespace Euclide
{

/* -------------------------------------------------------------------------- */

template <typename Enum, int NumValues, int FirstValue = 0>
struct EnumRange
{
	class EnumIterator
	{
		public:
			typedef std::forward_iterator_tag iterator_category;
			typedef Enum value_type;
			typedef int difference_type;
			typedef Enum *pointer;
			typedef Enum& reference;

		public:
			EnumIterator(int value) : m_value(value) {}

		public:
			inline bool operator==(const EnumIterator& iterator) const
				{ return m_value == iterator.m_value; }
			inline bool operator!=(const EnumIterator& iterator) const
				{ return m_value != iterator.m_value; }

			inline void operator++()
				{ m_value += 1; }

			inline Enum operator*() const
				{ return static_cast<Enum>(m_value); }

		private:
			int m_value;    /**< Enum value, as integer. */
	};

	inline EnumIterator begin() const
		{ return EnumIterator(FirstValue); }
	inline EnumIterator end() const
		{ return EnumIterator(NumValues); }
};

/* -------------------------------------------------------------------------- */

}

#endif
